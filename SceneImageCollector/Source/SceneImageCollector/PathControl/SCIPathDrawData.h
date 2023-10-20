// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include "SCIPathDrawData.generated.h"

USTRUCT() 
struct FSCIPathDrawData
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, Category=Visualization )
    bool IsDrawIfNotSelected = true;
    UPROPERTY( EditAnywhere, Category=Visualization )
    bool IsDrawIfSelected    = true;
    UPROPERTY( EditAnywhere, Category=Visualization )
    FColor Color             = FColor::Green;
    UPROPERTY( EditAnywhere, Category=Visualization )
    float Thickness          = 5.0f;
    UPROPERTY( EditAnywhere, Category=Visualization )
    float ControlPointSize   = 30.0f;
};
