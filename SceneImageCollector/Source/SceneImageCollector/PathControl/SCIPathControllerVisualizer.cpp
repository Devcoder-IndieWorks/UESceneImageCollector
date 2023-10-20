#include "SCIPathControllerVisualizer.h"
#include <Engine/Engine.h>
#if WITH_EDITOR
#include <Engine/Canvas.h>
#include <Components/SplineComponent.h>
#include <Widgets/Input/SNumericEntryBox.h>
#include <Widgets/Input/SVectorInputBox.h>
#include "SCIFollowerComponent.h"

FSCIPathControllerVisualizer::FSCIPathControllerVisualizer()
{
    IsHideCurrentRotationPointWidget = false;
    WidgetMode = UE::Widget::WM_None;
}

void FSCIPathControllerVisualizer::DrawVisualization( const UActorComponent* InComponent, const FSceneView* InView, FPrimitiveDrawInterface* InPDI )
{
    auto followerComp = Cast<const USCIFollowerComponent>( InComponent );
    if ( followerComp == nullptr )
        return;

    auto splineToFollow = followerComp->GetSplineToFollow();
    if ( splineToFollow == nullptr )
        return;

    DrawRollCurveVisualization( splineToFollow, InPDI, followerComp );

    const auto& eventViz = followerComp->EventPointsVisualization;
    if ( !eventViz.IsHideEventPoints ) {
        auto eventPointColor = eventViz.EventPointsColor;
        auto hitProxySize    = eventViz.EventPointHitProxySize;
        auto eventPointTex   = eventViz.EventPointSpriteTexture;
        auto& points         = followerComp->GetEventPoints().Points;
        for ( int32 i = 0; i < points.Num(); ++i ) {
            InPDI->SetHitProxy( nullptr );
            InPDI->SetHitProxy( new HSCIEventPointKeyProxy( InComponent, i ) );

            auto location = followerComp->GetLocationAtDistance( points[ i ].Distance, ESplineCoordinateSpace::World );
            if ( eventPointTex != nullptr )
                InPDI->DrawSprite( location, hitProxySize, hitProxySize, eventPointTex->GetResource(), eventPointColor, SDPG_Foreground
                , 0, eventPointTex->GetResource()->GetSizeX(), 0, eventPointTex->GetResource()->GetSizeY(), SE_BLEND_Masked );
            else
                InPDI->DrawPoint( location, eventPointColor, hitProxySize, SDPG_Foreground );

            InPDI->SetHitProxy( nullptr );
        }
    }

    const auto& speedConfig = followerComp->SpeedPointDrawConfig;
    if ( !speedConfig.IsHideSpeedPoints ) {
        auto speedPointColor = speedConfig.SpeedPointsColor;
        auto hitProxySize    = speedConfig.SpeedPointHitProxySize;
        auto speedPointTex   = speedConfig.SpeedPointSpriteTexture;
        auto& points         = followerComp->GetSpeedCurve().Points;
        for ( int32 i = 0; i < points.Num(); ++i ) {
            InPDI->SetHitProxy( nullptr );
            InPDI->SetHitProxy( new HSCISpeedPointKeyProxy( InComponent, i ) );

            auto speedPointDistance = followerComp->IsTimeBased ? points[ i ].OutVal : points[ i ].InVal;
            auto location = followerComp->GetLocationAtDistance( speedPointDistance, ESplineCoordinateSpace::World );
            if ( speedPointTex != nullptr )
                InPDI->DrawSprite( location, hitProxySize, hitProxySize, speedPointTex->GetResource(), speedPointColor, SDPG_Foreground
                , 0, speedPointTex->GetResource()->GetSizeX(), 0, speedPointTex->GetResource()->GetSizeY(), SE_BLEND_Masked );
            else 
                InPDI->DrawPoint( location, speedPointColor, hitProxySize, SDPG_Foreground );

            InPDI->SetHitProxy( nullptr );
        }
    }
}

void FSCIPathControllerVisualizer::DrawRollCurveVisualization( USplineComponent* InSplineComp, FPrimitiveDrawInterface* InPDI, const USCIFollowerComponent* InFollowerComp )
{
    if ( InSplineComp == nullptr )
        return;
    if ( InFollowerComp == nullptr )
        return;

    auto& debugConfig = InFollowerComp->AutoRollVisualConfig;
    if ( !debugConfig.IsHidePointsVisualization ) {
        auto& points = InFollowerComp->GetPathRoller().GetRollCurve().Points;
        for ( int32 i = 0; i < points.Num(); ++i ) {
            auto& point   = points[ i ];
            auto startPos = InFollowerComp->GetLocationAtDistance( point.InVal, ESplineCoordinateSpace::World );
            auto rotation = InFollowerComp->GetRotationAtDistance( point.InVal, ESplineCoordinateSpace::World );
            InPDI->SetHitProxy( nullptr );
            InPDI->SetHitProxy( new HSCIRollAngleKeyProxy( InFollowerComp, i ) );

            auto isSelected = SelectedRotationPointIndex.Contains( i );
            auto pointColor = isSelected ? debugConfig.SelectedPointColor : debugConfig.PointColor;
            InPDI->DrawPoint( startPos, pointColor, debugConfig.PointSize, SDPG_Foreground );
            InPDI->SetHitProxy( nullptr );

            if ( SelectedRotationPointIndex.Num() == 0 || i != SelectedRotationPointIndex.Last() || !IsHideCurrentRotationPointWidget )
                DrawCoordinateSystem( InPDI, startPos, rotation, debugConfig.LineLength, SDPG_Foreground, debugConfig.LineTickness );
        }
    }
}

void FSCIPathControllerVisualizer::DrawVisualizationHUD( const UActorComponent* InComponent, const FViewport* InViewport, const FSceneView* InView, FCanvas* InCanvas )
{
    auto followerComp = GetEditedPathFollower( InComponent );
    if ( followerComp == nullptr )
        return;

    auto splineToFollow = followerComp->GetSplineToFollow();
    if ( splineToFollow == nullptr )
        return;

    auto font = GEngine->GetLargeFont();
    if ( !followerComp->IsHidePathInfoText ) {
        auto timeDistance = GetTimeDistanceArray( followerComp, splineToFollow );
        auto pointsNum    = splineToFollow->GetNumberOfSplinePoints();
        check( pointsNum == timeDistance.Num() );
        for ( int32 i = 0; i < pointsNum; ++i ) {
            auto point = followerComp->GetLocationAtInputKey( (float)i, ESplineCoordinateSpace::World );
            FVector2D screenPos;
            if ( InView->WorldToPixel( point, screenPos ) ) {
                auto& temp      = timeDistance[ i ];
                auto formatText = FText::FromString( TEXT( "Time: {0} Distance: {1}" ) );
                auto text       = FText::Format( formatText, FText::AsNumber( temp.Time ), FText::AsNumber( temp.Distance ) );
                InCanvas->DrawShadowedText( screenPos.X, screenPos.Y - 45.0, text, font, FLinearColor::Red );
                InCanvas->DrawShadowedText( screenPos.X, screenPos.Y - 60.0, FText::FromString( point.ToCompactString() ), font, FLinearColor::Red );
            }
        }
    }

    if ( !followerComp->SpeedPointDrawConfig.IsHideSpeedPointInfoText ) {
        auto color   = followerComp->SpeedPointDrawConfig.SpeedPointsColor;
        auto& points = followerComp->GetSpeedCurve().Points;
        for ( int32 i = 0; i < points.Num(); ++i ) {
            auto distance = followerComp->IsTimeBased ? points[ i ].OutVal : points[ i ].InVal;
            auto point    = followerComp->GetLocationAtDistance( distance, ESplineCoordinateSpace::World );
            FVector2D screenPos;
            if ( InView->WorldToPixel( point, screenPos ) ) {
                auto formatText = followerComp->IsTimeBased ? FText::FromString( TEXT( "Time: {0}" ) ) : FText::FromString( TEXT( "Speed: {0}" ) );
                auto text       = FText::Format( formatText, FText::AsNumber( followerComp->IsTimeBased ? points[ i ].InVal : points[ i ].OutVal ) );
                InCanvas->DrawShadowedText( screenPos.X, screenPos.Y - 30.0, text, font, color.ReinterpretAsLinear() );

                int32 distanceTextYOffset = 0;
                if ( followerComp->SpeedFactorOperation != ESCIFactorOperation::FO_None ) {
                    formatText = FText::FromString( TEXT( "Modulated: {0}" ) );
                    text       = FText::Format( formatText, FText::AsNumber( followerComp->GetSpeedAtDistance( (float)i ) ) );
                    InCanvas->DrawShadowedText( screenPos.X, screenPos.Y - 15.0, text, font, color.ReinterpretAsLinear() );
                    distanceTextYOffset = 0;
                }
                else {
                    distanceTextYOffset = 15;
                }

                formatText = FText::FromString( TEXT( "Distance: {0}" ) );
                text       = FText::Format( formatText, FText::AsNumber( distance ) );
                InCanvas->DrawShadowedText( screenPos.X, screenPos.Y - distanceTextYOffset, text, font, color.ReinterpretAsLinear() );
            }
        }
    }

    if ( !followerComp->AutoRollVisualConfig.IsHideTextInfo ) {
        auto& points = followerComp->GetPathRoller().GetRollCurve().Points;
        for ( int32 i = 0; i < points.Num(); ++i ) {
            auto point = followerComp->GetLocationAtDistance( points[ i ].InVal, ESplineCoordinateSpace::World );
            FVector2D screenPos;
            if ( InView->WorldToPixel( point, screenPos ) ) {
                auto formatText = FText::FromString( TEXT( "Rotation: {0}" ) );
                auto text       = FText::Format( formatText, FText::FromString( FRotator::MakeFromEuler( points[ i ].OutVal ).ToCompactString() ) );
                InCanvas->DrawShadowedText( screenPos.X + 20.0, screenPos.Y - 30.0, text, font, followerComp->AutoRollVisualConfig.PointColor );

                formatText = FText::FromString( TEXT( "Distance: {0}" ) );
                text       = FText::Format( formatText, FText::AsNumber( points[ i ].InVal ) );
                InCanvas->DrawShadowedText( screenPos.X + 20.0, screenPos.Y - 15.0, text, font, followerComp->AutoRollVisualConfig.PointColor );
            }
        }
    }
}

TArray<FSCIPathControllerVisualizer::FTimeDistance> 
FSCIPathControllerVisualizer::GetTimeDistanceArray( const USCIFollowerComponent* InFollowerComp, USplineComponent* InSplineComp )
{
    auto length = InSplineComp->GetSplineLength();
    if ( FMath::IsNearlyZero( length ) )
        return TArray<FTimeDistance>();

    TArray<FVector> samplesCenters;
    TArray<float> samplesRadius;
    TArray<FTimeDistance> samplesTimeDistance;

    const int32 STEP_NUM      = 20;
    const auto DISTANCE_DELTA = length / (float)STEP_NUM;
    auto time = 0.0f;
    for ( int32 i = 0; i < STEP_NUM; ++i ) {
        auto currentDistance = i * DISTANCE_DELTA;
        auto speed = InFollowerComp->GetSpeedAtDistance( currentDistance );
        auto delta = DISTANCE_DELTA / FMath::Max( speed, 0.1f );

        FTimeDistance temp;
        temp.Distance = currentDistance;
        temp.Time     = time;
        samplesTimeDistance.Add( temp );

        auto currentDistanceLocation = InSplineComp->GetLocationAtDistanceAlongSpline( currentDistance, ESplineCoordinateSpace::World );
        auto nextDistance            = currentDistance + DISTANCE_DELTA;
        if ( nextDistance > length )
            nextDistance = length;

        auto nextDistanceLocation = InSplineComp->GetLocationAtDistanceAlongSpline( nextDistance, ESplineCoordinateSpace::World );
        auto diff   = nextDistanceLocation - currentDistanceLocation;
        auto center = currentDistanceLocation + (diff * 0.5f);
        samplesCenters.Add( center );
        samplesRadius.Add( diff.Size() * 0.5f );

        time += delta;
    }

    TArray<FTimeDistance> timeDistArray;
    // First spline point
    FTimeDistance timeDist;
    timeDist.Distance = 0.0f;
    timeDist.Time     = 0.0f;
    timeDistArray.Add( timeDist );

    const auto SPLINE_POINTS_NUM = InSplineComp->GetNumberOfSplinePoints();
    for ( int32 i = 1; i < (SPLINE_POINTS_NUM - 1); ++i ) {
        FTimeDistance splinePointTimeDistance;
        splinePointTimeDistance.Distance = InSplineComp->GetDistanceAlongSplineAtSplinePoint( i );
        auto splinePointLocation         = InSplineComp->GetWorldLocationAtSplinePoint( i );
        for ( int32 idx = 0; idx < (samplesRadius.Num() - 1); ++idx ) {
            if ( (splinePointLocation - samplesCenters[ idx ]).Size() < samplesRadius[ idx ] ) {
                splinePointTimeDistance.Time = samplesTimeDistance[ idx ].Time;
                break;
            }
        }

        timeDistArray.Add( splinePointTimeDistance );
    }

    // Last spline point
    timeDist.Distance = length;
    timeDist.Time     = time;
    timeDistArray.Add( timeDist );

    return timeDistArray;
}

bool FSCIPathControllerVisualizer::VisProxyHandleClick( FEditorViewportClient* InViewportClient, HComponentVisProxy* InVisProxy, const FViewportClick& InClick )
{
    if ( (InVisProxy == nullptr) || !InVisProxy->Component.IsValid() )
        return false;

    auto followerComp           = Cast<const USCIFollowerComponent>( InVisProxy->Component.Get() );
    auto oldFollowerOwningActor = FollowerPropertyPath.GetParentOwningActor();
    FollowerPropertyPath        = FComponentPropertyPath( followerComp );
    auto followerOwningActor    = FollowerPropertyPath.GetParentOwningActor();
    if ( FollowerPropertyPath.IsValid() ) {
        if ( oldFollowerOwningActor != followerOwningActor )
            ClearSelectionIndexes();

        if ( HandleSelection<HSCIRollAngleKeyProxy>( InVisProxy, InClick, SelectedRotationPointIndex ) )
            return true;
        if ( HandleSelection<HSCIEventPointKeyProxy>( InVisProxy, InClick, SelectedEventPointIndex ) )
            return true;
        if ( HandleSelection<HSCISpeedPointKeyProxy>( InVisProxy, InClick, SelectedSpeedPointIndex ) )
            return true;
    }

    return false;
}

void FSCIPathControllerVisualizer::EndEditing()
{
    ClearSelectionIndexes();
}

void FSCIPathControllerVisualizer::ClearSelectionIndexes()
{
    SelectedRotationPointIndex.Empty();
    SelectedEventPointIndex.Empty();
    SelectedSpeedPointIndex.Empty();
}

bool FSCIPathControllerVisualizer::GetWidgetLocation( const FEditorViewportClient* InViewportClient, FVector& OutLocation ) const
{
    auto followerComp = GetEditedPathFollower();
    if ( followerComp == nullptr )
        return false;

    auto pathComp = followerComp->GetSplineToFollow();
    if ( pathComp == nullptr )
        return false;

    if ( SelectedRotationPointIndex.Num() > 0 ) {
        auto& rollCurve = followerComp->GetPathRoller().GetRollCurve();
        if ( SelectedRotationPointIndex.Last() >= rollCurve.Points.Num() )
            return false;

        auto& point = rollCurve.Points[ SelectedRotationPointIndex.Last() ];
        OutLocation = pathComp->GetWorldLocationAtDistanceAlongSpline( point.InVal );
        return true;
    }

    if ( SelectedEventPointIndex.Num() > 0 ) {
        if ( SelectedEventPointIndex.Last() >= followerComp->GetEventPoints().Points.Num() )
            return false;

        auto& point = followerComp->GetEventPoints().Points[ SelectedEventPointIndex.Last() ];
        OutLocation = pathComp->GetWorldLocationAtDistanceAlongSpline( point.Distance );
        return true;
    }

    if ( SelectedSpeedPointIndex.Num() > 0 ) {
        auto& points = followerComp->GetSpeedCurve().Points;
        if ( SelectedSpeedPointIndex.Last() >= points.Num() )
            return false;

        auto& point = points[ SelectedSpeedPointIndex.Last() ];
        OutLocation = pathComp->GetWorldLocationAtDistanceAlongSpline( point.InVal );
        return true;
    }

    return false;
}

bool FSCIPathControllerVisualizer::GetCustomInputCoordinateSystem( const FEditorViewportClient* InViewportClient, FMatrix& OutMatrix ) const
{
    auto followerComp = GetEditedPathFollower();
    if ( followerComp == nullptr )
        return false;

    if ( InViewportClient->GetWidgetCoordSystemSpace() == COORD_Local ) {
        auto currentMode = InViewportClient->GetWidgetMode();
        if ( SelectedRotationPointIndex.Num() > 0 ) {
            IsHideCurrentRotationPointWidget = currentMode != UE::Widget::WM_Rotate;

            auto selectedRotationPointEuler = followerComp->GetPathRoller().GetRollCurve().Points[ SelectedRotationPointIndex.Last() ].OutVal;
            auto rotationPointLocalRotation = FRotator::MakeFromEuler( selectedRotationPointEuler );
            auto splineComp = followerComp->GetSplineToFollow();
            if ( splineComp == nullptr )
                return false;

            auto worldRotation = splineComp->GetComponentRotation() + rotationPointLocalRotation;
            if ( (currentMode == UE::Widget::WM_Rotate) && (WidgetMode != UE::Widget::WM_Rotate) )
                StartRotation = worldRotation;

            OutMatrix  = (WidgetMode == UE::Widget::WM_Rotate) ? FRotationMatrix::Make( StartRotation ) : FRotationMatrix::Make( worldRotation );
            WidgetMode = currentMode;
            return true;
        }

        if ( (SelectedSpeedPointIndex.Num() > 0) && (currentMode == UE::Widget::WM_Translate) ) {
            auto& point     = followerComp->GetSpeedCurve().Points[ SelectedSpeedPointIndex.Last() ];
            auto splineComp = followerComp->GetSplineToFollow();
            if ( splineComp == nullptr )
                return false;

            auto rotation = splineComp->GetRotationAtDistanceAlongSpline( point.InVal, ESplineCoordinateSpace::World );
            OutMatrix     = FRotationMatrix::Make( rotation );
            return true;
        }

        if ( (SelectedEventPointIndex.Num() > 0) && (currentMode == UE::Widget::WM_Translate) ) {
            auto& point     = followerComp->GetEventPoints().Points[ SelectedEventPointIndex.Last() ];
            auto splineComp = followerComp->GetSplineToFollow();
            if ( splineComp == nullptr )
                return false;

            auto rotation = splineComp->GetRotationAtDistanceAlongSpline( point.Distance, ESplineCoordinateSpace::World );
            OutMatrix     = FRotationMatrix::Make( rotation );
            return true;
        }
    }

    if ( InViewportClient->GetWidgetCoordSystemSpace() == COORD_World )
        IsHideCurrentRotationPointWidget = false;

    return false;
}

bool FSCIPathControllerVisualizer::HandleInputDelta( FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDeltaTranslate, FRotator& InDeltaRotate, FVector& InDeltaScale )
{
    auto followerComp = GetEditedPathFollower();
    if ( (followerComp == nullptr) || (followerComp->GetSplineToFollow() == nullptr) )
        return false;

    if ( SelectedRotationPointIndex.Num() > 0 ) {
        auto& curve        = followerComp->GetPathRoller().GetRollCurve();
        auto lastIndex     = SelectedRotationPointIndex.Last();
        auto& lastPoint    = curve.Points[ lastIndex ];
        auto distanceDelta = GetNewDistance( lastPoint.InVal, InDeltaTranslate ) - lastPoint.InVal;
        for ( int32 i = 0; i < SelectedRotationPointIndex.Num(); ++i ) {
            auto index    = SelectedRotationPointIndex[ i ];
            auto& point   = curve.Points[ index ];
            auto euler    = point.OutVal;
            auto rotation = FRotator::MakeFromEuler( euler );

            auto worldSpaceRotation = followerComp->GetSplineToFollow()->GetComponentQuat() * rotation.Quaternion();
            worldSpaceRotation      = InDeltaRotate.Quaternion() * worldSpaceRotation;
            auto newLocalRotation   = followerComp->GetSplineToFollow()->GetComponentQuat().Inverse() * worldSpaceRotation;
            point.OutVal = newLocalRotation.Euler();

            auto pointDistanceAlongCurve    = point.InVal;
            SelectedRotationPointIndex[ i ] = curve.MovePoint( index, pointDistanceAlongCurve + distanceDelta );
        }

        followerComp->GetPathRoller().SetInterpolationType( followerComp->RollInterpType, followerComp->GetSplineToFollow() );
    }

    for ( auto index : SelectedEventPointIndex ) {
        auto& points     = followerComp->GetEventPoints().Points;
        auto& point      = points[ index ];
        auto newDistance = GetNewDistance( point.Distance, InDeltaTranslate );
        point.Distance   = newDistance;
    }

    for ( int32 i = 0; i < SelectedSpeedPointIndex.Num(); ++i ) {
        auto index       = SelectedSpeedPointIndex[ i ];
        auto& curve      = followerComp->GetSpeedCurve();
        auto& points     = curve.Points;
        auto& point      = points[ index ];
        auto newDistance = GetNewDistance( point.InVal, InDeltaTranslate );
        SelectedSpeedPointIndex[ i ] = curve.MovePoint( index, newDistance );
    }

    return true;
}

float FSCIPathControllerVisualizer::GetNewDistance( float InCurrentDistance, FVector InDeltaTranslate )
{
    if ( InDeltaTranslate.IsNearlyZero() )
        return InCurrentDistance;

    auto followerComp  = GetEditedPathFollower();
    auto tanAtDistance = followerComp->GetSplineToFollow()->GetWorldDirectionAtDistanceAlongSpline( InCurrentDistance );
    auto dotProduct    = FVector::DotProduct( tanAtDistance, InDeltaTranslate.GetSafeNormal() );
    auto newDistance   = InCurrentDistance + (dotProduct * InDeltaTranslate.Size());
    return FMath::Clamp( newDistance, 0.0f, followerComp->GetSplineToFollow()->GetSplineLength() );
}

bool FSCIPathControllerVisualizer::HandleInputKey( FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent )
{
    if ( (InKey == EKeys::LeftMouseButton) && (InEvent == IE_Released) )
        WidgetMode = UE::Widget::WM_None;

    return false;
}

TSharedPtr<SWidget> FSCIPathControllerVisualizer::GenerateContextMenu() const
{
    auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
    if ( followerComp == nullptr )
        return TSharedPtr<SWidget>();	

    if ( SelectedSpeedPointIndex.Num() > 0 ) {
        FMenuBuilder menuBuilder( true, nullptr );
        menuBuilder.BeginSection( NAME_None, FText::FromString( TEXT( "Speed Point Properties" ) ) );
        {
            auto& point = followerComp->GetSpeedCurve().Points[ SelectedSpeedPointIndex.Last() ];
            auto speed  = followerComp->IsTimeBased ? point.InVal : point.OutVal;
            auto widget = SNew( SNumericEntryBox<float> )
            .Value_Lambda( [speed]{
                return speed;
            } )
            .OnValueChanged_Lambda( [this](float InNewValue ){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                for ( auto index : SelectedSpeedPointIndex ) {
                    auto& point = followerComp->GetSpeedCurve().Points[ index ];
                    auto& value = followerComp->IsTimeBased ? point.InVal : point.OutVal;
                    value = InNewValue;
                }
            } );
            menuBuilder.AddWidget( widget, FText::FromString( followerComp->IsTimeBased ? TEXT( "Time" ) : TEXT( "Speed" ) ) );
        }
        {
            auto& point    = followerComp->GetSpeedCurve().Points[ SelectedSpeedPointIndex.Last() ];
            auto distance  = followerComp->IsTimeBased ? point.OutVal : point.InVal;
            auto widget    = SNew( SNumericEntryBox<float> )
            .Value_Lambda( [distance]{
                return distance;
            } )
            .OnValueChanged_Lambda( [this](float InNewValue){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                for ( auto index : SelectedSpeedPointIndex ) {
                    auto& point = followerComp->GetSpeedCurve().Points[ index ];
                    auto& value = followerComp->IsTimeBased ? point.OutVal : point.InVal;
                    value = InNewValue;
                }
            } );
            menuBuilder.AddWidget( widget, FText::FromString( TEXT( "Distance" ) ) );
        }
        menuBuilder.EndSection();

        menuBuilder.BeginSection( NAME_None, FText::FromString( TEXT( "Actions" ) ) );
        {
            auto widget = SNew( SButton )
            .OnClicked_Raw( const_cast<FSCIPathControllerVisualizer*>(this), &FSCIPathControllerVisualizer::DeleteSpeedPoints )
            .Text( FText::FromString( TEXT( "Delete All Selected Speed Points" ) ) );
            menuBuilder.AddWidget( widget, FText::GetEmpty() );
        }
        menuBuilder.EndSection();

        return menuBuilder.MakeWidget();
    }

    if ( SelectedRotationPointIndex.Num() > 0 ) {
        FMenuBuilder menuBuilder( true, nullptr );
        menuBuilder.BeginSection( NAME_None, FText::FromString( TEXT( "Roll Curve Point Properties" ) ) );
        {
            auto angle  = followerComp->GetPathRoller().GetRollCurve().Points[ SelectedRotationPointIndex.Last() ].OutVal;
            auto widget = SNew( SVectorInputBox )
            .X( angle.X )
            .Y( angle.Y )
            .Z( angle.Z )
            .OnXCommitted_Lambda( [this, angle](float InX, ETextCommit::Type InType){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto current = angle;
                auto& points  = followerComp->GetPathRoller().GetRollCurve().Points;
                for ( auto index : SelectedRotationPointIndex )
                    points[ index ].OutVal = FVector( InX, current.Y, current.Z );
            } )
            .OnYCommitted_Lambda( [this, angle](float InY, ETextCommit::Type InType){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto current = angle;
                auto& points  = followerComp->GetPathRoller().GetRollCurve().Points;
                for ( auto index : SelectedRotationPointIndex )
                    points[ index ].OutVal = FVector( current.X, InY, current.Z );
            } )
            .OnZCommitted_Lambda( [this, angle](float InZ, ETextCommit::Type InType){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto current = angle;
                auto& points  = followerComp->GetPathRoller().GetRollCurve().Points;
                for ( auto index : SelectedRotationPointIndex )
                    points[ index ].OutVal = FVector( current.X, current.Y, InZ );
            } );
            menuBuilder.AddWidget( widget, FText::FromString( TEXT( "Rotation" ) ) );
        }
        {
            auto distance = followerComp->GetPathRoller().GetRollCurve().Points[ SelectedRotationPointIndex.Last() ].InVal;
            auto widget = SNew( SNumericEntryBox<float> )
            .Value_Lambda( [distance]{
                return distance;
            } )
            .OnValueChanged_Lambda( [this, &distance](float InNewValue){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto& points = followerComp->GetPathRoller().GetRollCurve().Points;
                for ( auto index : SelectedRotationPointIndex ) {
                    if ( points.IsValidIndex( index ) )
                        points[ index ].InVal = InNewValue;
                }

                distance = InNewValue;
            } );
            menuBuilder.AddWidget( widget, FText::FromString( TEXT( "Distance" ) ) );
        }
        menuBuilder.EndSection();

        menuBuilder.BeginSection( NAME_None, FText::FromString( TEXT( "Actions" ) ) );
        {
            auto widget = SNew( SButton )
            .OnClicked_Raw( const_cast<FSCIPathControllerVisualizer*>(this), &FSCIPathControllerVisualizer::DeleteRotationPoint )
            .Text( FText::FromString( TEXT( "Delete Rotation Point" ) ) );
            menuBuilder.AddWidget( widget, FText::GetEmpty() );
        }
        menuBuilder.EndSection();

        return menuBuilder.MakeWidget();
    }

    if ( SelectedEventPointIndex.Num() > 0 ) {
        FMenuBuilder menuBuilder( true, nullptr );
        menuBuilder.BeginSection( NAME_None, FText::FromString( TEXT( "Event Point Properties" ) ) );
        {
            auto name   = followerComp->GetEventPoints().Points[ SelectedEventPointIndex.Last() ].Name;
            auto widget = SNew( SEditableTextBox )
            .Text_Lambda( [name]{
                return FText::FromName( name );
            } )
            .OnTextChanged_Lambda( [this](const FText& InNewValue){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto& points = followerComp->GetEventPoints().Points;
                for ( auto index : SelectedEventPointIndex )
                    points[ index ].Name = FName( *(InNewValue.ToString()) );
            } );
            menuBuilder.AddWidget( widget, FText::FromString( TEXT( "Name" ) ) );
        }
        {
            auto distance = followerComp->GetEventPoints().Points[ SelectedEventPointIndex.Last() ].Distance;
            auto widget   = SNew( SNumericEntryBox<float> )
            .Value_Lambda( [distance]{
                return distance;
            } )
            .OnValueChanged_Lambda( [this](float InNewValue){
                auto followerComp = (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
                if ( followerComp == nullptr )
                    return;

                auto& points = followerComp->GetEventPoints().Points;
                for ( auto index : SelectedEventPointIndex )
                    points[ index ].Distance = InNewValue;
            } );
            menuBuilder.AddWidget( widget, FText::FromString( TEXT( "Distance" ) ) );
        }
        menuBuilder.EndSection();
    }

    return TSharedPtr<SWidget>();
}

FReply FSCIPathControllerVisualizer::DeleteSpeedPoints()
{
    auto followerComp = GetEditedPathFollower();
    if ( followerComp != nullptr ) {
        auto temp = SelectedSpeedPointIndex;
        if ( temp.Num() > 0 ) {
            if ( temp.Num() > 1 ) {
                auto msg   = FText::FromString( TEXT( "More than one speed points selected. Operation will delete all of them. Continue?" ) );
                auto title = FText::FromString( TEXT( "Delete all selected" ) );
                auto ret   = FMessageDialog::Open( EAppMsgType::YesNo, msg, &title );
                if ( ret == EAppReturnType::No )
                    return FReply::Handled();
            }

            temp.Sort();
            auto& points = followerComp->GetSpeedCurve().Points;
            for ( auto index : temp )
                points.RemoveAt( index, 1, false );

            SelectedSpeedPointIndex.Empty();

            if ( points.IsEmpty() )
                followerComp->IsUseSpeedCurve = false;
        }
    }

    return FReply::Handled();
}

FReply FSCIPathControllerVisualizer::DeleteRotationPoint()
{
    auto followerComp = GetEditedPathFollower();
    if ( followerComp != nullptr ) {
        auto temp = SelectedRotationPointIndex;
        if ( temp.Num() > 0 ) {
            if ( temp.Num() > 1 ) {
                auto msg   = FText::FromString( TEXT( "More than one speed points selected. Operation will delete all of them. Continue?" ) );
                auto title = FText::FromString( TEXT( "Delete all selected" ) );
                auto ret   = FMessageDialog::Open( EAppMsgType::YesNo, msg, &title );
                if ( ret == EAppReturnType::No )
                    return FReply::Handled();
            }

            temp.Sort();
            auto& points = followerComp->GetPathRoller().GetRollCurve().Points;
            for ( auto index : temp )
                points.RemoveAt( index, 1, false );

            SelectedSpeedPointIndex.Empty();

            if ( points.IsEmpty() )
                followerComp->IsUseRotationCurve = false;
        }
    }

    return FReply::Handled();
}

const USCIFollowerComponent* FSCIPathControllerVisualizer::GetEditedPathFollower( const UActorComponent* InComponent ) 
{
    return Cast<const USCIFollowerComponent>( InComponent );
}

USCIFollowerComponent* FSCIPathControllerVisualizer::GetEditedPathFollower()
{
    return Cast<USCIFollowerComponent>( FollowerPropertyPath.GetComponent() );
}

const USCIFollowerComponent* FSCIPathControllerVisualizer::GetEditedPathFollower() const
{
    return (const_cast<FSCIPathControllerVisualizer*>(this))->GetEditedPathFollower();
}
#endif
