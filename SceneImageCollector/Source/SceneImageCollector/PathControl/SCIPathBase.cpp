#include "SCIPathBase.h"
#include "SCIPathComponent.h"

ASCIPathBase::ASCIPathBase()
{
    PrimaryActorTick.bCanEverTick = false;
    PathToFollowerComp = CreateDefaultSubobject<USCIPathComponent>( TEXT( "PathToFollowerComp" ) );
    RootComponent      = PathToFollowerComp;

    SetHidden( true );
}

USCIPathComponent* ASCIPathBase::GetPathComponent() const
{
    return PathToFollowerComp;
}
