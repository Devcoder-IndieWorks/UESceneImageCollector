#include "SCIPathSpeedCurve.h"
#include <Engine/Texture2D.h>

FSCIPathSpeedPointsDrawConfig::FSCIPathSpeedPointsDrawConfig()
{
    SpeedPointSpriteTexture = LoadObject<UTexture2D>( nullptr, TEXT( "/Game/S_Speed.S_Speed" ) );
}

float FSCIPathSpeedCurve::GetSpeedAtDistance( const FInterpCurveFloat& InCurve, float InDistance )
{
    auto speed = InCurve.Eval( InDistance, 0.0f );
    return speed;
}

TTuple<float, float> FSCIPathSpeedCurve::GetMinMaxSpeed( const FInterpCurveFloat& InCurve )
{
    if ( InCurve.Points.Num() == 0 )
        return TTuple<float, float>( 0.0f, 0.0f );

    if ( InCurve.Points.Num() == 1 ) {
        auto speed = InCurve.Points[ 0 ].OutVal;
        return TTuple<float, float>( speed, speed );
    }

    auto speedPoints = InCurve.Points;
    speedPoints.Sort( []( const FInterpCurvePoint<float>& InMax, const FInterpCurvePoint<float>& InItem ){
        return InMax.OutVal < InItem.OutVal;
    } );

    auto minSpeed = speedPoints[ 0 ].OutVal;
    auto maxSpeed = speedPoints.Last().OutVal;
    return TTuple<float, float>( minSpeed, maxSpeed );
}
