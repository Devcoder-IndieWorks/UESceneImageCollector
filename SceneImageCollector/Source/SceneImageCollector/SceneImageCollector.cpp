// Copyright Epic Games, Inc. All Rights Reserved.

#include "SceneImageCollector.h"
#include <Modules/ModuleManager.h>
#if WITH_EDITORONLY_DATA
#include <EditorStyleSet.h>
#include <LevelEditor.h>
#endif

#define LOCTEXT_NAMESPACE "SceneImageCollector"

#if WITH_EDITORONLY_DATA
class FSCICommands : public TCommands<FSCICommands>
{
public:
    FSCICommands()
    : TCommands<FSCICommands>( TEXT( "SceneImageCollector" ), NSLOCTEXT( "Contexts", "SceneImageCollector", "SceneImageCollector Module" ), NAME_None, FEditorStyle::GetStyleSetName() )
    {
    }

    virtual void RegisterCommands() override
    {
    }

public:
    TSharedPtr<FUICommandInfo> LockActorSelection;
    TSharedPtr<FUICommandInfo> SelectPathFollowerComponent;
    mutable TWeakObjectPtr<class AActor> LockedActor;
};
#endif

class FSceneImageCollectorModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
    }

    virtual void ShutdownModule() override
    {
    }

private:
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_PRIMARY_GAME_MODULE( FSceneImageCollectorModule, SceneImageCollector, "SceneImageCollector" );
