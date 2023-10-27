// Copyright Devcoder.
#pragma once
#include <UnrealWidget.h>
#if WITH_EDITOR
#include <Launch/Resources/Version.h>
#include "SCIPathControllerVisProxies.h"

class FSCIPathControllerVisualizer : public FComponentVisualizer
{
    struct FTimeDistance
    {
        float Distance = -1.0f;
        float Time     = -1.0f;
    };
public:
    FSCIPathControllerVisualizer();
    ~FSCIPathControllerVisualizer() = default;

    virtual void DrawVisualization( const UActorComponent* InComponent, const FSceneView* InView, FPrimitiveDrawInterface* InPDI ) override;
    virtual void DrawVisualizationHUD( const UActorComponent* InComponent, const FViewport* InViewport, const FSceneView* InView, FCanvas* InCanvas ) override;
    virtual bool VisProxyHandleClick( FEditorViewportClient* InViewportClient, HComponentVisProxy* InVisProxy, const FViewportClick& InClick ) override;

    virtual bool GetWidgetLocation( const FEditorViewportClient* InViewportClient, FVector& OutLocation ) const override;
    virtual bool HandleInputDelta( FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDeltaTranslate, FRotator& InDeltaRotate, FVector& InDeltaScale ) override;
    virtual bool HandleInputKey( FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) override;
    virtual bool GetCustomInputCoordinateSystem( const FEditorViewportClient* InViewportClient, FMatrix& OutMatrix ) const override;
    virtual TSharedPtr<SWidget> GenerateContextMenu() const override;
    virtual void EndEditing() override;

    FReply DeleteSpeedPoints();
    FReply DeleteRotationPoint();

private:
    const class USCIFollowerComponent* GetEditedPathFollower( const UActorComponent* InComponent );
    const class USCIFollowerComponent* GetEditedPathFollower() const;
    class USCIFollowerComponent* GetEditedPathFollower();

    void DrawRollCurveVisualization( class USplineComponent* InSplineComp, FPrimitiveDrawInterface* InPDI, const class USCIFollowerComponent* InFolowerComp );
    void ClearSelectionIndexes();

    float GetNewDistance( float InCurrentDistance, FVector InDeltaTranslate );
    TArray<FTimeDistance> GetTimeDistanceArray( const class USCIFollowerComponent* InFollowerComp, class USplineComponent* InSplineComp );

    template< class TProxyType > 
    bool HandleSelection( HComponentVisProxy* InVisProxy, const FViewportClick& InClick, TArray<int32>& OutAddTo )
    {
        if ( InVisProxy->IsA( TProxyType::StaticGetType() ) ) {
            auto proxy = (TProxyType*)InVisProxy;
            if ( !InClick.IsControlDown() )
                ClearSelectionIndexes();

            OutAddTo.Add( proxy->KeyIndex );
            return true;
        }

        return false;
    }

    void InvalidateViewports();

private:
    using TIndexArray = TArray<int32>;
    TIndexArray SelectedRotationPointIndex;
    TIndexArray SelectedEventPointIndex;
    TIndexArray SelectedSpeedPointIndex;

    FComponentPropertyPath FollowerPropertyPath;

    mutable bool IsHideCurrentRotationPointWidget;
    mutable UE::Widget::EWidgetMode WidgetMode;
    mutable FRotator StartRotation;
};
#endif
