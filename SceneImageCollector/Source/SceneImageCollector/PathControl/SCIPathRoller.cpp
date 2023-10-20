#include "SCIPathRoller.h"
#include "SCIFollowerComponent.h"
#include "SCIPathComponent.h"
#include "../VLog.h"
#include <Distributions/DistributionFloatConstant.h>

namespace SCI
{
    template<class T, class U> 
    T Min( const TArray<T, U>& InValues, int32* OutMinIndex = nullptr )
    {
        if ( InValues.Num() == 0 ) {
            if ( OutMinIndex != nullptr )
                *OutMinIndex = INDEX_NONE;
            return T();
        }

        auto curMin = InValues[ 0 ];
        int32 curMinIndex = 0;
        for ( int32 idx = 1; idx < InValues.Num(); ++idx ) {
            const auto value = InValues[ idx ];
            if ( value < curMin ) {
                curMin      = value;
                curMinIndex = idx;
            }
        }

        if ( OutMinIndex != nullptr )
            *OutMinIndex = curMinIndex;

        return curMin;
    }

    template<class T, class U> 
    T Max( const TArray<T, U>& InValues, int32* OutMaxIndex = nullptr )
    {
        if ( InValues.Num() == 0 ) {
            if ( OutMaxIndex != nullptr )
                *OutMaxIndex = INDEX_NONE;
            return T();
        }

        auto curMax = InValues[ 0 ];
        int32 curMaxIndex = 0;
        for ( int32 idx = 1; idx < InValues.Num(); ++idx ) {
            const auto value = InValues[ idx ];
            if ( curMax < value ) {
                curMax      = value;
                curMaxIndex = idx;
            }
        }

        if ( OutMaxIndex != nullptr )
            *OutMaxIndex = curMaxIndex;

        return curMax;
    }
}

//-----------------------------------------------------------------------------

FRotator FSCIPathRoller::GetRollRotationAtDistance( const USplineComponent* InSplineComp, float InDistance, bool InUsePitchAndYawFromPath ) const
{
    if ( ensure( InSplineComp != nullptr ) ) {
        auto eulerAngles = RollAnglesCurve.Eval( InDistance, FVector::ZeroVector );
        FRotator rotation( FQuat::MakeFromEuler( eulerAngles ) );

        if ( InUsePitchAndYawFromPath ) {
            auto splineRotation = InSplineComp->GetRotationAtDistanceAlongSpline( InDistance, ESplineCoordinateSpace::Local );
            rotation.Pitch      = splineRotation.Pitch;
            rotation.Yaw        = splineRotation.Yaw;
        }

        auto pathComp = Cast<USCIPathComponent>( InSplineComp );
        if ( pathComp != nullptr ) {
            auto mirrorAroundX = pathComp->IsMirrorAroundX;
            if ( mirrorAroundX ) {
                rotation.Roll = -rotation.Roll;
                rotation.Yaw  = -rotation.Yaw;
            }
        }

        return rotation;
    }

    return FRotator::ZeroRotator;
}

FRotator FSCIPathRoller::GetRotationAtDistanceStableLinear( float InDistance, const USplineComponent* InSplineComp ) const
{
    auto numPoints = RollAnglesCurve.Points.Num();
    auto& points   = RollAnglesCurve.Points;

    FQuat keyQuat;
    if ( numPoints == 0 ) {
        keyQuat = FQuat::Identity;
    }
    else if ( (numPoints < 2) || (InDistance <= points[ 0 ].InVal) ) {
        auto outEulerRot = points[ 0 ].OutVal;
        keyQuat = FQuat::MakeFromEuler( outEulerRot );
    }
    else if ( InDistance >= points[ numPoints - 1 ].InVal ) {
        auto outEulerRot = points.Last().OutVal;
        keyQuat = FQuat::MakeFromEuler( outEulerRot );
    }
    else {
        for ( int32 keyIdx = 1; keyIdx < numPoints; keyIdx++ ) {
            if ( InDistance < points[ keyIdx ].InVal ) {
                auto delta      = points[ keyIdx ].InVal - points[ keyIdx - 1 ].InVal;
                auto alpha      = FMath::Clamp( (InDistance - points[ keyIdx - 1 ].InVal) / delta, 0.0f, 1.0f );
                auto currentRot = points[ keyIdx ].OutVal;
                auto prevRot    = points[ keyIdx - 1 ].OutVal;
                auto key1Quat   = FQuat::MakeFromEuler( prevRot );
                auto key2Quat   = FQuat::MakeFromEuler( currentRot );
                keyQuat         = FQuat::Slerp( key1Quat, key2Quat, alpha );
                break;
            }
        }
    }

    FRotator rotation( keyQuat );
    auto pathComp = Cast<USCIPathComponent>( InSplineComp );
    if ( pathComp != nullptr ) {
        auto mirrorAroundX = pathComp->IsMirrorAroundX;
        if ( mirrorAroundX ) {
            rotation.Roll = -rotation.Roll;
            rotation.Yaw  = -rotation.Yaw;
        }
    }

    return rotation;
}

void FSCIPathRoller::GenerateRollAngles( const FSCIRotationComputeContext& InContext )
{
    if ( InContext.Spline == nullptr )
        return;

    TFloatArray angles, distances;
    ComputeAngles( InContext.Spline, InContext.ComputeOnSplineControlPoints, InContext.RollStepsNum, InContext.RollSampleLength, angles, distances );

    if ( InContext.ClampAngles )
        NormalizeAngles( angles, InContext.MinAngle, InContext.MaxAngle );

    SetRollCurvePoints( InContext.Spline, angles, distances );
    SetInterpolationType( InContext.RollerInterpType, InContext.Spline );

    VLOG( Log, TEXT( "Rotation Points:" ) );
    for ( int32 i = 0; i < RollAnglesCurve.Points.Num(); ++i )
        VLOG( Log, TEXT( "Distance: [%f] Euler: [%s]" ), RollAnglesCurve.Points[ i ].InVal, *(FRotator::MakeFromEuler( RollAnglesCurve.Points[ i ].OutVal ).ToCompactString()) );
}

void FSCIPathRoller::ComputeAngles( USplineComponent* InSplineComp, bool InComputeOnSplineControlPoints, int32 InRollStepsNum, float InRollSampleLength, TFloatArray& OutAngles, TFloatArray& OutDistances )
{
    auto splineLength       = InSplineComp->GetSplineLength();
    auto samplingStepLength = splineLength / (float)InRollStepsNum;
    InRollSampleLength      = CalcRotationSampleLen( InRollSampleLength, splineLength, InRollStepsNum );
    InRollStepsNum++;

    const auto SPLINE_CONTROL_POINTS_NUM = InSplineComp->SplineCurves.Position.Points.Num();
    const auto ACTUAL_STEPS_NUM          = InComputeOnSplineControlPoints ? SPLINE_CONTROL_POINTS_NUM : InRollStepsNum;
    check( ACTUAL_STEPS_NUM > 1 );
    for ( int32 i = 0; i < ACTUAL_STEPS_NUM; ++i ) {
        auto currentDist = InComputeOnSplineControlPoints ? InSplineComp->GetDistanceAlongSplineAtSplinePoint( i ) : (i * samplingStepLength);
        auto angle       = CalcRollAngleAtDistance( InSplineComp, currentDist, InRollSampleLength );
        OutAngles.Add( angle );
        OutDistances.Add( currentDist );
    }

    if ( InSplineComp->IsClosedLoop() )
        OutAngles.Last() = OutAngles[ 0 ];

    VLOG( Log, TEXT( "Compute Angles:" ) );
    for ( int32 i = 0; i < OutAngles.Num(); ++i )
        VLOG( Log, TEXT( "Distance: [%f] Angle: [%f]" ), OutDistances[ i ], OutAngles[ i ] );
}

void FSCIPathRoller::UpdateRollAngles( const FSCIRotationComputeContext& InContext )
{
    if ( InContext.Spline == nullptr )
        return;

    auto sampleLength = CalcRotationSampleLen( InContext.RollSampleLength, InContext.Spline->GetSplineLength(), InContext.RollStepsNum );

    TFloatArray angles;
    for ( auto& point : RollAnglesCurve.Points ) {
        auto distance = point.InVal;
        auto angle    = CalcRollAngleAtDistance( InContext.Spline, distance, sampleLength );
        angles.Add( angle );
    }

    if ( InContext.ClampAngles )
        NormalizeAngles( angles, InContext.MinAngle, InContext.MaxAngle );

    check( RollAnglesCurve.Points.Num() == angles.Num() );
    for ( int32 i = 0; i < RollAnglesCurve.Points.Num(); ++i ) {
        auto& point    = RollAnglesCurve.Points[ i ];
        auto rotation  = InContext.Spline->GetRotationAtDistanceAlongSpline( point.InVal, ESplineCoordinateSpace::Local );
        rotation.Roll += angles[ i ];
        point.OutVal   = rotation.Euler();
    }

    SetInterpolationType( InContext.RollerInterpType, InContext.Spline );
}

float FSCIPathRoller::CalcRotationSampleLen( float InRollSampleLength, float InSplineLength, int InStepNum )
{
    if ( InRollSampleLength < 0 ) {
        auto samplingStepLength = InSplineLength / (float)InStepNum;
        return samplingStepLength / (float)InStepNum;
    }

    return InRollSampleLength;
}

float FSCIPathRoller::CalcRollAngleAtDistance( USplineComponent* InSplineComp, float InDistance, float InSampleLength )
{
    auto startDir     = InSplineComp->GetDirectionAtDistanceAlongSpline( InDistance, ESplineCoordinateSpace::Local );
    auto endDir       = startDir;
    auto endDist      = InDistance + InSampleLength;
    auto splineLength = InSplineComp->GetSplineLength();
    if ( endDist > splineLength ) {
        if ( !InSplineComp->IsClosedLoop() ) {
            startDir = InSplineComp->GetDirectionAtDistanceAlongSpline( splineLength - 0.0001f - InSampleLength, ESplineCoordinateSpace::Local );
            endDir   = InSplineComp->GetDirectionAtDistanceAlongSpline( splineLength - 0.0001f, ESplineCoordinateSpace::Local );
        }
        else {
            endDir   = InSplineComp->GetDirectionAtDistanceAlongSpline( InSampleLength, ESplineCoordinateSpace::Local );
        }
    }
    else {
        endDir = InSplineComp->GetDirectionAtDistanceAlongSpline( endDist, ESplineCoordinateSpace::Local );
    }

    auto angle = endDir.Rotation().Yaw - startDir.Rotation().Yaw;
    return angle;
}

void FSCIPathRoller::NormalizeAngles( TFloatArray& OutAngles, float InMinAngle, float InMaxAngle )
{
    const auto CURVE_MIN_ANGLE = SCI::Min( OutAngles );
    const auto CURVE_MAX_ANGLE = SCI::Max( OutAngles );
    const auto POSITIVE_NORMALIZE_VAL = InMaxAngle / CURVE_MAX_ANGLE;
    const auto NEGATIVE_NORMALIZE_VAL = InMinAngle / CURVE_MIN_ANGLE;

    for ( auto& rollAngle : OutAngles )
        rollAngle *= (rollAngle < 0.f) ? NEGATIVE_NORMALIZE_VAL : POSITIVE_NORMALIZE_VAL;
}

void FSCIPathRoller::SetRollCurvePoints( const USplineComponent* InSplineComp, const TFloatArray& InAngles, const TFloatArray& InDistances )
{
    check( InAngles.Num() == InDistances.Num() );
    RollAnglesCurve.Points.Reset();
    for ( int32 i = 0; i < InAngles.Num(); ++i ) {
        auto rotation  = InSplineComp->GetRotationAtDistanceAlongSpline( InDistances[ i ], ESplineCoordinateSpace::Local );
        rotation.Roll += InAngles[ i ];
        RollAnglesCurve.AddPoint( InDistances[ i ], rotation.Euler() );
    }
}

void FSCIPathRoller::SetInterpolationType( ESCIRollerInterpType InInterpolationType, const USplineComponent* InSplineComp )
{
    EInterpCurveMode interpMode;
    switch ( InInterpolationType ) {
        case ESCIRollerInterpType::RIT_Constant:
            interpMode = CIM_Constant;
            break;
        case ESCIRollerInterpType::RIT_Linear:
            interpMode = CIM_Linear;
            break;
        case ESCIRollerInterpType::RIT_Cubic:
            interpMode = CIM_CurveAuto;
            break;
        case ESCIRollerInterpType::RIT_CubicClamped:
            interpMode = CIM_CurveAutoClamped;
            break;
        default:
            interpMode = CIM_CurveAuto;
            break;
    }

    for ( auto& point : RollAnglesCurve.Points )
        point.InterpMode = interpMode;

    if ( RollAnglesCurve.Points.Num() > 1 ) {
        auto& point = RollAnglesCurve.Points[ 0 ];
        auto relativeSpaceEuler = point.OutVal;

        constexpr int32 ADJACENT_KEY_INDEX = 1;
        const auto ADJACENT_EULER = RollAnglesCurve.Points[ ADJACENT_KEY_INDEX ].OutVal;
        const auto EULER_DIFF     = relativeSpaceEuler - ADJACENT_EULER;
        if ( EULER_DIFF.X > 180.0f )
            relativeSpaceEuler.X -= 360.0f;
        else if ( EULER_DIFF.X < -180.0f )
            relativeSpaceEuler.X += 360.0f;

        if ( EULER_DIFF.Y > 180.0f )
            relativeSpaceEuler.Y -= 360.0f;
        else if ( EULER_DIFF.Y < -180.0f )
            relativeSpaceEuler.Y += 360.0f;

        if ( EULER_DIFF.Z > 180.0f )
            relativeSpaceEuler.Z -= 360.0f;
        else if ( EULER_DIFF.Z < -180.0f )
            relativeSpaceEuler.Z += 360.0f;

        point.OutVal = relativeSpaceEuler;
    }

    for ( int32 i = 1; i < RollAnglesCurve.Points.Num(); ++i ) {
        auto& point = RollAnglesCurve.Points[ i ];
        auto relativeSpaceEuler = point.OutVal;

        const int32 ADJACENT_KEY_INDEX = i - 1;
        const auto ADJACENT_EULER = RollAnglesCurve.Points[ ADJACENT_KEY_INDEX ].OutVal;
        const auto EULER_DIFF     = relativeSpaceEuler - ADJACENT_EULER;
        if ( EULER_DIFF.X > 180.0f )
            relativeSpaceEuler.X -= 360.0f;
        else if ( EULER_DIFF.X < -180.0f )
            relativeSpaceEuler.X += 360.0f;

        if ( EULER_DIFF.Y > 180.0f )
            relativeSpaceEuler.Y -= 360.0f;
        else if ( EULER_DIFF.Y < -180.0f )
            relativeSpaceEuler.Y += 360.0f;

        if ( EULER_DIFF.Z > 180.0f )
            relativeSpaceEuler.Z -= 360.0f;
        else if ( EULER_DIFF.Z < -180.0f )
            relativeSpaceEuler.Z += 360.0f;

        point.OutVal = relativeSpaceEuler;
    }

    RollAnglesCurve.AutoSetTangents( 0.0f, InSplineComp->bStationaryEndpoints );
}

const FInterpCurveVector& FSCIPathRoller::GetRollCurve() const
{
    return RollAnglesCurve;
}

FInterpCurveVector& FSCIPathRoller::GetRollCurve()
{
    return RollAnglesCurve;
}

void FSCIPathRoller::Clear()
{
    RollAnglesCurve.Points.Reset();
}

void FSCIPathRoller::Dump()
{
    VLOG( Log, TEXT( "Dump:" ) );
    VLOG( Log, TEXT( "Point Num: %i" ), RollAnglesCurve.Points.Num() );
    for ( auto& point : RollAnglesCurve.Points )
        VLOG( Log, TEXT( "Distance: [%f], Rotation: [%s]" ), point.InVal, *(FRotator::MakeFromEuler( point.OutVal ).ToCompactString()) );
}
