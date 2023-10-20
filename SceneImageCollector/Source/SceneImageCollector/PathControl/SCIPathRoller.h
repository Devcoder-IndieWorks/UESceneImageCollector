// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include "SCIPathRoller.generated.h"

UENUM( BlueprintType ) 
enum class ESCIRollerInterpType : uint8
{
    RIT_Constant     UMETA(DisplayName="Constant"), 
    RIT_Linear       UMETA(DisplayName="Linear"), 
    RIT_LinearStable UMETA(DisplayName="Linear Stable"), 
    RIT_Cubic        UMETA(DisplayName="Cubic"), 
    RIT_CubicClamped UMETA(DisplayName="Cubic Clamped")
};

//-----------------------------------------------------------------------------

struct FSCIRotationComputeContext
{
    class USplineComponent* Spline        = nullptr;
    bool ComputeOnSplineControlPoints     = false;
    int32 RollStepsNum                    = 1;
    float RollSampleLength                = 0.0f;
    bool ClampAngles                      = false;
    float MinAngle                        = -360.0f;
    float MaxAngle                        = 360.0f;
    ESCIRollerInterpType RollerInterpType = ESCIRollerInterpType::RIT_Linear;
};

//-----------------------------------------------------------------------------

USTRUCT( BlueprintType, Blueprintable ) 
struct FSCIPathRoller
{
    GENERATED_BODY()

public:
    FRotator GetRollRotationAtDistance( const class USplineComponent* InSplineComp, float InDistance, bool InUsePitchAndYawFromPath ) const;
    FRotator GetRotationAtDistanceStableLinear( float InDistance, const class USplineComponent* InSplineComp ) const;

    void GenerateRollAngles( const FSCIRotationComputeContext& InContext );
    void UpdateRollAngles( const FSCIRotationComputeContext& InContext );

    void SetInterpolationType( ESCIRollerInterpType InInterpolationType, const class USplineComponent* InSplineComp );

    void Clear();
    void Dump();

    const FInterpCurveVector& GetRollCurve() const;
    FInterpCurveVector& GetRollCurve();

private:
    typedef TArray<float, TInlineAllocator<16>> TFloatArray;

    float CalcRotationSampleLen( float InRollSampleLength, float InSplineLength, int InStepsNum );
    void ComputeAngles( class USplineComponent* InSplineComp, bool InComputeOnSplineControlPoints
    , int32 InRollStepsNum, float InRollSampleLength, TFloatArray& OutAngles, TFloatArray& OutDistances );
    float CalcRollAngleAtDistance( class USplineComponent* InSplineComp, float InDistance, float InSampleLength );
    void SetRollCurvePoints( const class USplineComponent* InSplineComp, const TFloatArray& InAngles, const TFloatArray& InDistances );

    static void NormalizeAngles( TFloatArray& OutAngles, float InMinAngle, float InMaxAngle );

public:
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    FInterpCurveVector RollAnglesCurve;

private:
    UPROPERTY()
    TArray<TObjectPtr<class UObject>> Curves;
};
