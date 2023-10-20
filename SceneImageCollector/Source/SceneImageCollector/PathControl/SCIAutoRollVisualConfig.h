// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include "SCIAutoRollVisualConfig.generated.h"

USTRUCT() 
struct FSCIAutoRollVisualConfig
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, Category=Path )
    bool IsHidePointsVisualization = false;
    UPROPERTY( EditAnywhere, Category=Path )
    bool IsHideTextInfo = false;
    UPROPERTY( EditAnywhere, Category=Path )
    float LineLength = 50.0f;
    UPROPERTY( EditAnywhere, Category=Path )
    FLinearColor PointColor = FLinearColor::Blue;
    UPROPERTY( EditAnywhere, Category=Path )
    FLinearColor SelectedPointColor = FLinearColor::Yellow;
    UPROPERTY( EditAnywhere, Category=Path )
    float PointSize = 20.0f;
    UPROPERTY( EditAnywhere, Category=Path )
    float LineTickness = 3.0f;
};
