#include "SCICameraActor.h"
#include "SCISceneCaptureActor.h"
#include "../PathControl/SCIPathBase.h"
#include "../PathControl/SCIFollowerComponent.h"
#include "../PathControl/SCIPathComponent.h"
#include "../VLog.h"
#include <EngineUtils.h>

ASCICameraActor::ASCICameraActor( const FObjectInitializer& ObjectInitializer )
: Super( ObjectInitializer )
{
    FollowerComponent = CreateDefaultSubobject<USCIFollowerComponent>( TEXT( "PathFollowerComp" ) );
    AddOwnedComponent( FollowerComponent );

    PathIndex = 0;
    IsLoop    = false;
}

void ASCICameraActor::BeginPlay()
{
    Super::BeginPlay();

    if ( !CaptureActor.IsValid() ) {
        for ( TActorIterator<ASCISceneCaptureActor> iter( GetWorld() ); iter; ++iter ) {
            auto actor = *iter;
            if ( actor != nullptr ) {
                CaptureActor = actor;
                break;
            }
        }
    }
    VCLOG( !CaptureActor.IsValid(), Warning, TEXT( "ASCISceneCaptureActor is not found." ) );

    if ( !PathActors.IsEmpty() ) {
        for ( auto actor : PathActors ) {
            auto pathActor = Cast<ASCIPathBase>( actor );
            if ( ensure( pathActor != nullptr ) ) {
                auto pathComp     = pathActor->GetPathComponent();
                auto& eventPoints = pathComp->EventPoints;
                for ( auto& point : eventPoints.Points )
                    eventPoints.GetEventPointDelegateByName( point.Name )->OnEventPointReached.AddUniqueDynamic( this, &ASCICameraActor::OnEventAction );
            }
        }

        PathIndex = !FollowerComponent->IsReverse ? 0 : (PathActors.Num() - 1);
        IsLoop    = FollowerComponent->IsLoop;

        FollowerComponent->IsLoop = false;
        FollowerComponent->SetPathOwner( !FollowerComponent->IsReverse ? PathActors[ 0 ] : PathActors.Last() );
        FollowerComponent->Start();

        FollowerComponent->OnReachedStart.AddUniqueDynamic( this, &ASCICameraActor::OnChangePath );
        FollowerComponent->OnReachedEnd.AddUniqueDynamic( this, &ASCICameraActor::OnChangePath );
    }
}

void ASCICameraActor::OnEventAction( USCIFollowerComponent*, float, UObject* )
{
    if ( CaptureActor.IsValid() ) {
        VLOG( Log, TEXT( "Scene Captured." ) );
        CaptureActor->Capture();
    }
}

void ASCICameraActor::OnChangePath( USCIFollowerComponent* )
{
    if ( IsValidPathsIndex() ) {
        UpdatePathIndex();

        FollowerComponent->IsLoop = false;
        FollowerComponent->SetPathOwner( PathActors[ PathIndex ] );
        if ( FollowerComponent->IsReverse )
            FollowerComponent->HandleLoopingType( false );
        FollowerComponent->Start();
    }
    else if ( !IsLoop ) {
        FollowerComponent->Stop();
    }
    else {
        FollowerComponent->IsLoop = true;
    }
}

bool ASCICameraActor::IsValidPathsIndex() const
{
    return FollowerComponent->IsReverse 
    ? PathActors.IsValidIndex( PathIndex - 1 ) 
    : PathActors.IsValidIndex( PathIndex + 1 );
}

int32 ASCICameraActor::UpdatePathIndex()
{
    return FollowerComponent->IsReverse ? PathIndex-- : PathIndex++;
}
