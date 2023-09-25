// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <RenderCore/Public/RenderCommandFence.h>

struct FSCIRenderRequest
{
    TArray<FColor> Image;
    FRenderCommandFence RenderFence;
};

struct FSCIFloatRenderRequest
{
    TArray<FFloat16Color> Image;
    FRenderCommandFence RenderFence;
};
