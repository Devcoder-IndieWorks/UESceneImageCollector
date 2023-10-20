#include "SCIPathComponent.h"
#include "SCIFollowerComponent.h"
#include <Engine.h>
#include <SceneManagement.h>
#if WITH_EDITOR
#include <UnrealEd.h>
#endif

USCIPathComponent::USCIPathComponent()
{
    bWantsInitializeComponent = true;
    Duration                  = 6.0f;
    bSplineHasBeenEdited      = true;
    IsMirrorAroundX           = false;
    IsFollowerSelected        = false;
}

void USCIPathComponent::InitializeComponent()
{
    Super::InitializeComponent();

    EventPoints.Init();
}

FVector USCIPathComponent::GetLocationAtDistanceAlongSplineMirrored( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    if ( IsMirrorAroundX ) {
        auto location = GetLocationAtDistanceAlongSpline( InDistance, ESplineCoordinateSpace::Local );
        location.Y = -location.Y;
        return (InCoordinateSpace == ESplineCoordinateSpace::World) 
        ? GetComponentTransform().TransformPosition( location ) 
        : location;
    }

    return GetLocationAtDistanceAlongSpline( InDistance, InCoordinateSpace );
}

FVector USCIPathComponent::GetLocationAtSplinePointMirrored( int32 InPointIndex, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    if ( IsMirrorAroundX ) {
        auto location = GetLocationAtSplinePoint( InPointIndex, ESplineCoordinateSpace::Local );
        location.Y = -location.Y;
        return (InCoordinateSpace == ESplineCoordinateSpace::World) 
        ? GetComponentTransform().TransformPosition( location ) 
        : location;
    }

    return GetLocationAtSplinePoint( InPointIndex, InCoordinateSpace );
}

FVector USCIPathComponent::GetLocationAtSplineInputKeyMirrored( float InKey, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    if ( IsMirrorAroundX ) {
        auto location = GetLocationAtSplineInputKey( InKey, ESplineCoordinateSpace::Local );
        location.Y = -location.Y;
        return (InCoordinateSpace == ESplineCoordinateSpace::World) 
        ? GetComponentTransform().TransformPosition( location ) 
        : location;
    }

    return GetLocationAtSplineInputKey( InKey, InCoordinateSpace );
}

#if WITH_EDITOR
void USCIPathComponent::OnFollowerSelected()
{
    IsFollowerSelected = true;
}

void USCIPathComponent::OnFollowerDeselected()
{
    IsFollowerSelected = false;
}
#endif

FBoxSphereBounds USCIPathComponent::CalcBounds( const FTransform& InLocalToWorld ) const
{
    FVector min, max;
    SplineCurves.Position.CalcBounds( min, max, FVector::ZeroVector );

    FBoxSphereBounds newBounds( FBox( min, max ) );
    auto trans = InLocalToWorld;
    if ( IsMirrorAroundX )
        trans.Mirror( EAxis::None, EAxis::Y );

    return newBounds.TransformBy( trans );
}

FPrimitiveSceneProxy* USCIPathComponent::CreateSceneProxy()
{
#if WITH_EDITOR
    class FSCIPathComponentSceneProxy : public FPrimitiveSceneProxy
    {
    public:
        FSCIPathComponentSceneProxy( USCIPathComponent* InComp )
        : FPrimitiveSceneProxy( InComp )
        , PathComp( InComp )
        {
            bWillEverBeLit = false;
        }

        virtual SIZE_T GetTypeHash() const override
        {
            static SIZE_T UniquePointer;
            return reinterpret_cast<SIZE_T>( &UniquePointer );
        }

        virtual void GetDynamicMeshElements( const TArray<const FSceneView*>& InViews, const FSceneViewFamily& InViewFamily, uint32 InVisibilityMap, FMeshElementCollector& InCollector ) const override
        {
            for ( int32 viewIndex = 0; viewIndex < InViews.Num(); viewIndex++ ) {
                FPrimitiveDrawInterface* pdi = nullptr;
                const FSceneView* view = nullptr;
                if ( InVisibilityMap & (1 << viewIndex) ) {
                    pdi  = InCollector.GetPDI( viewIndex );
                    view = InViews[ viewIndex ];
                }
                else {
                    continue;
                }

                if ( PathComp != nullptr ) {
                    DrawPath( pdi );
                    DrawRotationCurve( pdi );
                    DrawEventPoints( pdi );
                    DrawSpeedPoints( pdi );
                }
            }
        }

        virtual uint32 GetMemoryFootprint() const override
        {
            return (sizeof(*this) + GetAllocatedSize());
        }

        virtual FPrimitiveViewRelevance GetViewRelevance( const FSceneView* InView ) const override
        {
            auto isVisible = IsSelected() 
            ? PathComp->DrawerConfig.IsDrawIfSelected 
            : PathComp->DrawerConfig.IsDrawIfNotSelected;

            isVisible = isVisible || PathComp->IsFollowerSelected;

            if ( (PathComp->GetWorld() == nullptr) && PathComp->GetWorld()->IsGameWorld() )
                isVisible = false;

            FPrimitiveViewRelevance relevance;
            relevance.bDrawRelevance    = isVisible && IsShown( InView );
            relevance.bDynamicRelevance = true;
            relevance.bShadowRelevance  = IsShadowCast( InView );
            relevance.bEditorPrimitiveRelevance = UseEditorCompositing( InView );
            return relevance;
        }

    private:
        void DrawPath( FPrimitiveDrawInterface* InPDI ) const
        {
            const auto& drawConfig          = PathComp->DrawerConfig;
            const auto DEPTH_PRIORITY_GROUP = SDPG_Foreground;

            FVector oldKeyPos( 0.0 );
            auto oldKeyTime = 0.0f;

            const auto NUM_POINTS = PathComp->SplineCurves.Position.Points.Num();
            auto numSegments      = PathComp->IsClosedLoop() ? NUM_POINTS + 1 : NUM_POINTS;
            for ( int32 keyIdx = 0; keyIdx < numSegments; keyIdx++ ) {
                auto newKeyPos       = PathComp->GetLocationAtSplinePointMirrored( keyIdx, ESplineCoordinateSpace::World );
                const auto KEY_COLOR = FColor( 255, 128, 0 );
                if ( keyIdx < numSegments )
                    InPDI->DrawPoint( newKeyPos, KEY_COLOR, drawConfig.ControlPointSize, DEPTH_PRIORITY_GROUP );

                if ( keyIdx > 0 ) {
                    const int32 NUM_STEPS = 30;
                    FVector oldPos = oldKeyPos;
                    for ( int32 stepIdx = 1; stepIdx < (NUM_STEPS + 1); stepIdx++ ) {
                        const float KEY = (keyIdx - 1) + (stepIdx / (float)NUM_STEPS);
                        auto newPos     = PathComp->GetLocationAtSplineInputKeyMirrored( KEY, ESplineCoordinateSpace::World );
                        auto pathColor  = drawConfig.Color;
                        auto bVisualizeSpeedPoints = (PathComp->FollowerComponent != nullptr) 
                        ? PathComp->FollowerComponent->SpeedPointDrawConfig.IsVisualizeSpeed 
                        : PathComp->SpeedPointDrawConfig.IsVisualizeSpeed;
                        if ( bVisualizeSpeedPoints )
                            pathColor = GetSpeedColor( drawConfig.Color, KEY / numSegments );

                        InPDI->DrawLine( oldPos, newPos, pathColor, DEPTH_PRIORITY_GROUP, drawConfig.Thickness );
                        oldPos = newPos;
                    }
                }

                oldKeyPos = newKeyPos;
            }
        }

        FColor GetSpeedColor( const FColor InDefaultColor, float InKey ) const
        {
            const auto& speedCurve = (PathComp->FollowerComponent != nullptr) 
            ? PathComp->FollowerComponent->GetSpeedCurve() 
            : PathComp->GetSpeedCurve().GetCurve();
            auto MinMaxSpeed = FSCIPathSpeedCurve::GetMinMaxSpeed( speedCurve );

            const auto DISTANCE      = InKey * PathComp->GetSplineLength();
            const auto CURRENT_SPEED = (PathComp->FollowerComponent != nullptr) 
            ? PathComp->FollowerComponent->GetSpeedAtDistance( DISTANCE ) 
            : FSCIPathSpeedCurve::GetSpeedAtDistance( PathComp->GetSpeedCurve().GetCurve(), DISTANCE );

            const auto SPEED_COLOR_SCALE = MinMaxSpeed.Get<1>() - MinMaxSpeed.Get<0>();
            if ( SPEED_COLOR_SCALE < 0.001f )
                return InDefaultColor;

            const auto& speedConfig = PathComp->FollowerComponent != nullptr 
            ? PathComp->FollowerComponent->SpeedPointDrawConfig 
            : PathComp->SpeedPointDrawConfig;

            const auto ALPHA = FMath::Min( (CURRENT_SPEED - MinMaxSpeed.Get<0>()) / SPEED_COLOR_SCALE, 1.0f );
            const auto FINAL_COLOR = FMath::Lerp( speedConfig.LowSpeedColor, speedConfig.HighSpeedColor, ALPHA );
            return FINAL_COLOR.ToRGBE();
        }

        void DrawRotationCurve( FPrimitiveDrawInterface* InPDI ) const
        {
            const auto& debugConfig = PathComp->AutoRollVisualConfig;
            if ( !debugConfig.IsHidePointsVisualization ) {
                const auto& points = PathComp->GetPathRoller().GetRollCurve().Points;
                for ( const auto& point : points ) {
                    auto startPos = PathComp->GetLocationAtDistanceAlongSplineMirrored( point.InVal, ESplineCoordinateSpace::World );
                    auto rotation = PathComp->GetComponentQuat() * PathComp->GetPathRoller().GetRollRotationAtDistance( PathComp, point.InVal, false ).Quaternion();
                    InPDI->DrawPoint( startPos, debugConfig.PointColor, debugConfig.PointSize, SDPG_Foreground );
                    DrawCoordinateSystem( InPDI, startPos, rotation.Rotator(), debugConfig.LineLength, SDPG_Foreground, debugConfig.LineTickness );
                }
            }
        }

        void DrawEventPoints( FPrimitiveDrawInterface* InPDI ) const
        {
            const auto& eventViz = PathComp->EventPointsVisualization;
            if ( !eventViz.IsHideEventPoints ) {
                auto color = eventViz.EventPointsColor;
                auto size  = eventViz.EventPointHitProxySize;
                auto tex   = eventViz.EventPointSpriteTexture;
                for ( const auto& point : PathComp->EventPoints.Points ) {
                    auto location = PathComp->GetLocationAtDistanceAlongSplineMirrored( point.Distance, ESplineCoordinateSpace::World );
                    if ( tex != nullptr )
                        InPDI->DrawSprite( location, size, size, tex->GetResource(), color, SDPG_Foreground
                        , 0, tex->GetResource()->GetSizeX(), 0, tex->GetResource()->GetSizeY(), SE_BLEND_Masked );
                    else
                        InPDI->DrawPoint( location, color, size, SDPG_Foreground );
                }
            }
        }

        void DrawSpeedPoints( FPrimitiveDrawInterface* InPDI ) const
        {
            if ( !PathComp->SpeedPointDrawConfig.IsHideSpeedPoints ) {
                auto color = PathComp->SpeedPointDrawConfig.SpeedPointsColor;
                auto size  = PathComp->SpeedPointDrawConfig.SpeedPointHitProxySize;
                auto tex   = PathComp->SpeedPointDrawConfig.SpeedPointSpriteTexture;
                const auto& points = PathComp->GetSpeedCurve().GetCurve().Points;
                for ( const auto& point : points ) {
                    auto location = PathComp->GetLocationAtDistanceAlongSplineMirrored( point.InVal, ESplineCoordinateSpace::World );
                    if ( tex != nullptr )
                        InPDI->DrawSprite( location, size, size, tex->GetResource(), color, SDPG_Foreground
                        , 0, tex->GetResource()->GetSizeX(), 0, tex->GetResource()->GetSizeY(), SE_BLEND_AlphaComposite );
                    else
                        InPDI->DrawPoint( location, color, size, SDPG_Foreground );
                }
            }
        }

    private:
        USCIPathComponent* PathComp;
    };

    return new FSCIPathComponentSceneProxy( this );
#else
    return nullptr;
#endif
}
