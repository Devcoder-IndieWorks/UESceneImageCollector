// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include "SCIPathSpeedCurve.generated.h"

USTRUCT() 
struct FSCIPathSpeedPointsDrawConfig
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, Category=Speed )
    bool IsHideSpeedPoints        = false;
    UPROPERTY( EditAnywhere, Category=Speed )
    bool IsHideSpeedPointInfoText = false;
    UPROPERTY( EditAnywhere, Category=Speed )
    FColor SpeedPointsColor       = FColor::Green;
    UPROPERTY( EditAnywhere, Category=Speed )
    float SpeedPointHitProxySize  = 20.0f;
    UPROPERTY( EditAnywhere, Category=Speed )
    bool IsVisualizeSpeed         = false;
    UPROPERTY( EditAnywhere, Category=Speed )
    FLinearColor LowSpeedColor    = FLinearColor::Green;
    UPROPERTY( EditAnywhere, Category=Speed )
    FLinearColor HighSpeedColor   = FLinearColor::Red;
    UPROPERTY( EditAnywhere, Category=Speed, meta=(DisplayName="Sprite") )
    TObjectPtr<class UTexture2D> SpeedPointSpriteTexture;

    FSCIPathSpeedPointsDrawConfig();
};

//-----------------------------------------------------------------------------

USTRUCT( BlueprintType ) 
struct FSCIPathSpeedCurve
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, Category=Speed )
    FInterpCurveFloat SpeedCurve;

    static float GetSpeedAtDistance( const FInterpCurveFloat& InCurve, float InDistance );
    static TTuple<float, float> GetMinMaxSpeed( const FInterpCurveFloat& InCurve );

    const FInterpCurveFloat& GetCurve() const { return SpeedCurve; }
    FInterpCurveFloat& GetCurve()             { return SpeedCurve; }
};
