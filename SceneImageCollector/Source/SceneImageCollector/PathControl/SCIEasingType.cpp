#include "SCIEasingType.h"

namespace SCI
{
    float Ease( ESCIEasingType InEasingType, float InCurrentAlpha, float InCurveDegree )
    {
        switch ( InEasingType ) {
            case ESCIEasingType::ET_Linear:
                return InCurrentAlpha;

            case ESCIEasingType::ET_CircularIn:
                return FMath::InterpCircularIn( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_CircularOut:
                return FMath::InterpCircularOut( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_CircularInOut:
                return FMath::InterpCircularInOut( 0.0f, 1.0f, InCurrentAlpha );

            case ESCIEasingType::ET_EaseIn:
                return FMath::InterpEaseIn( 0.0f, 1.0f, InCurrentAlpha, InCurveDegree );
            case ESCIEasingType::ET_EaseOut:
                return FMath::InterpEaseOut( 0.0f, 1.0f, InCurrentAlpha, InCurveDegree );
            case ESCIEasingType::ET_EaseInOut:
                return FMath::InterpEaseInOut( 0.0f, 1.0f, InCurrentAlpha, InCurveDegree );

            case ESCIEasingType::ET_ExpoIn:
                return FMath::InterpExpoIn( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_ExpoOut:
                return FMath::InterpExpoOut( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_ExpoInOut:
                return FMath::InterpExpoInOut( 0.0f, 1.0f, InCurrentAlpha );

            case ESCIEasingType::ET_SinIn:
                return FMath::InterpSinIn( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_SinOut:
                return FMath::InterpSinOut( 0.0f, 1.0f, InCurrentAlpha );
            case ESCIEasingType::ET_SinInOut:
                return FMath::InterpSinInOut( 0.0f, 1.0f, InCurrentAlpha );
        }
        return InCurrentAlpha;
    }
}
