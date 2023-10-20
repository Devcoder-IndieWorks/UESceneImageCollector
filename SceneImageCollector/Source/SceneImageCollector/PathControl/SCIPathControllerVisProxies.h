// Copyright Devcoder.
#pragma once
#if WITH_EDITOR
#include <ComponentVisualizer.h>

// 클릭 가능한 스플라인 편집 프록시를 위한 베이스 타입
struct HSCIPathControllerVisProxy : public HComponentVisProxy
{
    DECLARE_HIT_PROXY();

    int32 KeyIndex;

    HSCIPathControllerVisProxy( const UActorComponent* InComponent, int32 InKeyIndex )
    : HComponentVisProxy( InComponent, HPP_Wireframe )
    , KeyIndex( InKeyIndex )
    {
    }
};

// 스플라인 키를 위한 Roll Angle 프록시 타입
struct HSCIRollAngleKeyProxy : public HSCIPathControllerVisProxy
{
    DECLARE_HIT_PROXY();

    HSCIRollAngleKeyProxy( const UActorComponent* InComponent, int32 InKeyIndex )
    : HSCIPathControllerVisProxy( InComponent, InKeyIndex )
    {
    }
};

// 스플라인 키를 위한 Event Point 프록시 타입
struct HSCIEventPointKeyProxy : public HSCIPathControllerVisProxy
{
    DECLARE_HIT_PROXY();

    HSCIEventPointKeyProxy( const UActorComponent* InComponent, int32 InKeyIndex )
    : HSCIPathControllerVisProxy( InComponent, InKeyIndex )
    {
    }
};

// 스프라인 키를 위한 Speed Pont 프록시 타입
struct HSCISpeedPointKeyProxy : public HSCIPathControllerVisProxy
{
    DECLARE_HIT_PROXY();

    HSCISpeedPointKeyProxy( const UActorComponent* InComponent, int32 InKeyIndex )
    : HSCIPathControllerVisProxy( InComponent, InKeyIndex )
    {
    }
};
#endif
