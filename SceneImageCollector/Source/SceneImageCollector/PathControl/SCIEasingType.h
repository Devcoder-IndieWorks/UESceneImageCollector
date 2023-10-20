// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include <Math/UnrealMathUtility.h>
#include "SCIEasingType.generated.h"

UENUM( BlueprintType ) 
enum class ESCIEasingType : uint8
{
    ET_Linear        UMETA(DisplayName="Linear"),

    ET_CircularIn    UMETA(DisplayName="Circular In"),
    ET_CircularOut   UMETA(DisplayName="Circular Out"),
    ET_CircularInOut UMETA(DisplayName="Circular InOut"),

    ET_EaseIn        UMETA(DisplayName="Ease In"),
    ET_EaseOut       UMETA(DisplayName="Ease Out"),
    ET_EaseInOut     UMETA(DisplayName="Ease InOut"),

    ET_ExpoIn        UMETA(DisplayName="Expo In"),
    ET_ExpoOut       UMETA(DisplayName="Expo Out"),
    ET_ExpoInOut     UMETA(DisplayName="Expo InOut"),

    ET_SinIn         UMETA(DisplayName="Sin In"),
    ET_SinOut        UMETA(DisplayName="Sin Out"),
    ET_SinInOut      UMETA(DisplayName="Sin InOut")
};

namespace SCI
{
    float Ease( ESCIEasingType InEasingType, float InCurrentAlpha, float InCurveDegree = 2.0f );
}
