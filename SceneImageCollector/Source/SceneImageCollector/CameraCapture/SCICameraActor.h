// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <Camera/CameraActor.h>
#include "SCICameraActor.generated.h"

UCLASS() 
class ASCICameraActor : public ACameraActor
{
    GENERATED_UCLASS_BODY()
protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnChangePath( class USCIFollowerComponent* InComp );
    UFUNCTION()
    void OnEventAction( class USCIFollowerComponent* InComp, float InDistance, UObject* InExtra );

    bool IsValidPathsIndex() const;
    int32 UpdatePathIndex();

protected:
    UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category=Follower )
    TObjectPtr<class USCIFollowerComponent> FollowerComponent;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Path )
    TArray<TObjectPtr<class AActor>> PathActors;

    TWeakObjectPtr<class ASCISceneCaptureActor> CaptureActor;
    int32 PathIndex;
    bool IsLoop;
};
