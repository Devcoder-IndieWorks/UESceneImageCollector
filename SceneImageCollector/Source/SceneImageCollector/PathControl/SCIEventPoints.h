// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <UObject/Object.h>
#include <Templates/SubclassOf.h>
#include "SCIEventPoints.generated.h"

UENUM( BlueprintType ) 
enum class ESCIEventMode : uint8
{
    EM_Forward UMETA(DisplayName="Forward"),
    EM_Reverse UMETA(DisplayName="Reverse"),
    EM_Always  UMETA(DisplayName="Always")
};

//-----------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FSCIEventPointReachedSignature, class USCIFollowerComponent*, FollowerComp, float, Distance, UObject*, ExtraData );

//-----------------------------------------------------------------------------

USTRUCT( BlueprintType ) 
struct FSCIEventPoint
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    FName Name;
    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    float Distance = 0.0f;
    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    TSubclassOf<UObject> UserData;
    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    ESCIEventMode Mode = ESCIEventMode::EM_Always;
    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    int32 Count = -1;
    UPROPERTY( EditAnywhere, BlueprintReadonly, Category=Path )
    int32 Index = -1;

    static FSCIEventPoint Invalid;
};

//-----------------------------------------------------------------------------

USTRUCT( BlueprintType ) 
struct FSCIEventPointsVisualization
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, Category=Events )
    bool IsHideEventPoints = false;
    UPROPERTY( EditAnywhere, Category=Events )
    bool IsHideEventPointInfoText = false;
    UPROPERTY( EditAnywhere, Category=Events )
    FColor EventPointsColor = FColor::Blue;
    UPROPERTY( EditAnywhere, Category=Events )
    float EventPointHitProxySize = 20.0f;
    UPROPERTY( EditAnywhere, Category=Events )
    TObjectPtr<class UTexture2D> EventPointSpriteTexture;

    FSCIEventPointsVisualization();
};

//-----------------------------------------------------------------------------

UCLASS() 
class USCIEventPointDelegateHolder : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY( BlueprintAssignable, Category=Path )
    FSCIEventPointReachedSignature OnEventPointReached;
};

//-----------------------------------------------------------------------------

USTRUCT( BlueprintType ) 
struct FSCIEventPoints
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Events )
    TArray<FSCIEventPoint> Points;

private:
    UPROPERTY()
    TObjectPtr<USCIEventPointDelegateHolder> AllEventHolder;
    UPROPERTY()
    TArray<TObjectPtr<USCIEventPointDelegateHolder>> Holders;

    TArray<FSCIEventPoint> DistanceSorted;

public:
    void Init();
    void Reset( float InCurrentDistanceOnPath, bool InIsReverse, int32& OutLastPassedEventIndex );
    void ProcessEvents( float InCurrentDistance, class USCIFollowerComponent* InFollowerComp, bool InIsReverse, int32& OutLastPassedEventIndex );

    USCIEventPointDelegateHolder* GetEventPointDelegateByName( const FName& InName );
    USCIEventPointDelegateHolder* GetEventPointDelegateByIndex( int32 InIndex );
    USCIEventPointDelegateHolder* GetEventPointDelegateAll() const;

private:
    int32 FindPassedEventPointIndex( float InCurrentDistance, bool InIsReverse );
    void BroadcastEventPointReached( FSCIEventPoint& InEventPoint, class USCIFollowerComponent* InFollowerComp );
    void SortPointsByDistance();
};
