// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "SCIPathBase.generated.h"

UCLASS() 
class ASCIPathBase : public AActor
{
    GENERATED_BODY()
public:
    ASCIPathBase();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Path )
    TObjectPtr<class USCIPathComponent> PathToFollowerComp;
};
