// Copyright Devcoder.
#pragma once
#include <Components/SplineComponent.h>
#include "SCIPathDrawData.h"
#include "SCIPathRoller.h"
#include "SCIPathSpeedCurve.h"
#include "SCIEventPoints.h"
#include "SCIAutoRollVisualConfig.h"
#include "SCIPathComponent.generated.h"

UCLASS( meta=(BlueprintSpawnableComponent) )
class USCIPathComponent : public USplineComponent
{
    GENERATED_BODY()
public:
    USCIPathComponent();

    virtual void InitializeComponent() override;

#if WITH_EDITOR
    void OnFollowerSelected();
    void OnFollowerDeselected();
#endif

    FSCIPathRoller& GetPathRoller()             { return PathRoller; }
    const FSCIPathRoller& GetPathRoller() const { return PathRoller; }

    FSCIPathSpeedCurve& GetSpeedCurve()             { return SpeedCurve; }
    const FSCIPathSpeedCurve& GetSpeedCurve() const { return SpeedCurve; }

public:
    UFUNCTION( BlueprintCallable, Category=Spline )
    FVector GetLocationAtDistanceAlongSplineMirrored( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const;
    UFUNCTION( BlueprintCallable, Category=Spline )
    FVector GetLocationAtSplinePointMirrored( int32 InPointIndex, ESplineCoordinateSpace::Type InCoordinateSpace ) const;
    UFUNCTION( BlueprintCallable, Category=Spline )
    FVector GetLocationAtSplineInputKeyMirrored( float InKey, ESplineCoordinateSpace::Type InCoordinateSpace ) const;

private:
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds( const FTransform& InLocalToWorld ) const override;

public:
    UPROPERTY( EditAnywhere, Category=Path )
    FSCIPathDrawData DrawerConfig;
    UPROPERTY( EditAnywhere, Category=Path )
    bool IsMirrorAroundX;

    UPROPERTY( EditAnywhere, Category=Rotation )
    FSCIPathRoller PathRoller;
    UPROPERTY( EditAnywhere, Category=Rotation, meta=(DisplayName="Visualization") )
    FSCIAutoRollVisualConfig AutoRollVisualConfig;

    UPROPERTY( EditAnywhere, Category=Events, meta=(DisplayName="Visualization") )
    FSCIEventPointsVisualization EventPointsVisualization;
    UPROPERTY( EditAnywhere, Category=Events, meta=(DisplayName="Points") )
    FSCIEventPoints EventPoints;

    UPROPERTY( EditAnywhere, Category=Speed, meta=(DisplayName="Curve") )
    FSCIPathSpeedCurve SpeedCurve;
    UPROPERTY( EditAnywhere, Category=Speed, meta=(DisplayName="Visualization") )
    FSCIPathSpeedPointsDrawConfig SpeedPointDrawConfig;

    UPROPERTY()
    TObjectPtr<const class USCIFollowerComponent> FollowerComponent;

private:
    bool IsFollowerSelected;
};
