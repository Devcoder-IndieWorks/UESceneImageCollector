#include "SCIEventPoints.h"
#include "SCIFollowerComponent.h"
#include "../VLog.h"
#include <Engine/Texture2D.h>
#include <Components/SplineComponent.h>
#include <Distributions/DistributionFloatConstant.h>

namespace SCI
{
    bool CanBroadcastEventPoint( ESCIEventMode InMode, bool InIsReverse )
    {
        auto bAlways  = InMode == ESCIEventMode::EM_Always;
        auto bReverse = InIsReverse && (InMode == ESCIEventMode::EM_Reverse);
        auto bForward = !InIsReverse && (InMode == ESCIEventMode::EM_Forward);
        return bAlways || bForward || bReverse;
    }
}

FSCIEventPoint FSCIEventPoint::Invalid = FSCIEventPoint();

//-----------------------------------------------------------------------------

FSCIEventPointsVisualization::FSCIEventPointsVisualization()
{
    EventPointSpriteTexture = LoadObject<UTexture2D>( nullptr, TEXT( "/Game/event_sprite.event_sprite" ) );
}

//-----------------------------------------------------------------------------

void FSCIEventPoints::Init()
{
    auto eventPointsNum = Points.Num();
    if ( eventPointsNum == 0 )
        return;

    Holders.Reset( eventPointsNum );
    for ( int32 i = 0; i < eventPointsNum; ++i ) {
        Points[ i ].Index = i;
        auto holder = NewObject<USCIEventPointDelegateHolder>();
        Holders.Add( holder );
    }

    AllEventHolder = NewObject<USCIEventPointDelegateHolder>();

    SortPointsByDistance();
}

void FSCIEventPoints::SortPointsByDistance()
{
    DistanceSorted = Points;
    DistanceSorted.Sort( []( const FSCIEventPoint& InMax, const FSCIEventPoint& InItem ){
        return InItem.Distance > InMax.Distance;
    } );

    for ( const auto& point : DistanceSorted )
        VLOG( Log, TEXT( "Distance: [%f]" ), point.Distance );
}

void FSCIEventPoints::Reset( float InCurrentDistanceOnPath, bool InIsReverse, int32& OutLastPassedEventIndex )
{
    OutLastPassedEventIndex = FindPassedEventPointIndex( InCurrentDistanceOnPath, InIsReverse );
}

void FSCIEventPoints::ProcessEvents( float InCurrentDistance, USCIFollowerComponent* InFollowerComp, bool InIsReverse, int32& OutLastPassedEventIndex )
{
    auto indexPassed = FindPassedEventPointIndex( InCurrentDistance, InIsReverse );
    if ( (indexPassed != OutLastPassedEventIndex) && (indexPassed != -1) ) {
        OutLastPassedEventIndex = indexPassed;
        auto& point = DistanceSorted[ indexPassed ];
        VLOG( Log, TEXT( "Reached event point: [%s]" ), *(point.Name.ToString()) );

        if ( SCI::CanBroadcastEventPoint( point.Mode, InIsReverse ) )
            BroadcastEventPointReached( point, InFollowerComp );
    }
}

int32 FSCIEventPoints::FindPassedEventPointIndex( float InCurrentDistance, bool InIsReverse )
{
    if ( !InIsReverse ) {
        for ( int32 i = DistanceSorted.Num() - 1; i >= 0; --i ) {
            const auto& point = DistanceSorted[ i ];
            auto distance     = point.Distance;
            if ( InCurrentDistance > distance )
                return i;
        }
    }
    else {
        for ( int32 i = 0; i < DistanceSorted.Num(); ++i ) {
            const auto& point = DistanceSorted[ i ];
            auto distance = point.Distance;
            if ( InCurrentDistance < distance )
                return i;
        }
    }

    return -1;
}

void FSCIEventPoints::BroadcastEventPointReached( FSCIEventPoint& InEventPoint, class USCIFollowerComponent* InFollowerComp )
{
    if ( InEventPoint.Count == 0 )
        return;

    if ( InEventPoint.Count > 0 )
        --InEventPoint.Count;

    auto pointIdx = InEventPoint.Index;
    check( (pointIdx < Holders.Num()) && (Holders[ pointIdx ] != nullptr) );
    auto holder = Holders[ pointIdx ];
    if ( holder->OnEventPointReached.IsBound() )
        holder->OnEventPointReached.Broadcast( InFollowerComp, InEventPoint.Distance, InEventPoint.UserData.GetDefaultObject() );

    if ( AllEventHolder != nullptr ) {
        if ( AllEventHolder->OnEventPointReached.IsBound() )
            AllEventHolder->OnEventPointReached.Broadcast( InFollowerComp, InEventPoint.Distance, InEventPoint.UserData.GetDefaultObject() );
    }
}

USCIEventPointDelegateHolder* FSCIEventPoints::GetEventPointDelegateByName( const FName& InName )
{
    for ( int32 i = 0; i < Points.Num(); ++i ) {
        if ( Points[ i ].Name == InName ) {
            check( (i < Holders.Num()) && (Holders[ i ] != nullptr) );
            return Holders[ i ];
        }
    }

    return nullptr;
}

USCIEventPointDelegateHolder* FSCIEventPoints::GetEventPointDelegateByIndex( int32 InIndex )
{
    if ( Points.IsValidIndex( InIndex ) ) {
        check( Holders.IsValidIndex( InIndex ) && (Holders[ InIndex ] != nullptr) );
        return Holders[ InIndex ];
    }

    return nullptr;
}

USCIEventPointDelegateHolder* FSCIEventPoints::GetEventPointDelegateAll() const
{
    return AllEventHolder;
}
