// Copyright Epic Games, Inc. All Rights Reserved.

#include "SceneImageCollector.h"
#include "PathControl/SCIFollowerComponent.h"
#if WITH_EDITORONLY_DATA
#include "PathControl/SCIPathControllerCustomization.h"
#include "PathControl/SCIPathControllerVisualizer.h"
#include <EditorStyleSet.h>
#include <LevelEditor.h>
#include <UnrealEdGlobals.h>
#include <Selection.h>
#include <PropertyEditorModule.h>
#include <Editor/UnrealEdEngine.h>
#endif
#include <Modules/ModuleManager.h>

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
        UI_COMMAND( LockActorSelection, "Lock Selection", "Lock currently slected actor selection i.e. actor is stays always selected. Press again to release."
        , EUserInterfaceActionType::Button, FInputChord( EModifierKey::Alt, EKeys::E ) );
        UI_COMMAND( SelectPathFollowerComponent, "Select PathFollower Component if in list.", "Tryies to find the follower component. If it does it selects the component"
        , EUserInterfaceActionType::Button, FInputChord( EModifierKey::Shift|EModifierKey::Control, EKeys::S ) );
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
    #if WITH_EDITORONLY_DATA
        if ( GUnrealEd == nullptr )
            return;

        FSCICommands::Register();

        auto& commands = FModuleManager::LoadModuleChecked<FLevelEditorModule>( TEXT( "LevelEditor" ) ).GetGlobalLevelEditorActions();

        commands->MapAction( FSCICommands::Get().LockActorSelection
        , FExecuteAction::CreateLambda( [this]{
            auto& lockedActor = FSCICommands::Get().LockedActor;
            if ( lockedActor.IsValid() ) {
                lockedActor = nullptr;
            }
            else if ( GEditor != nullptr ) {
                auto actorsSelection = GEditor->GetSelectedActors();
                if ( actorsSelection != nullptr && actorsSelection->Num() == 1 ) {
                    auto selectedActor = CastChecked<AActor>( actorsSelection->GetSelectedObject( 0 ) );
                    lockedActor        = selectedActor;
                }
            }
        } )
        , FCanExecuteAction::CreateLambda( []{
            return true;
        } ) );

        commands->MapAction( FSCICommands::Get().SelectPathFollowerComponent
        , FExecuteAction::CreateLambda( [this]{
            if ( GEditor != nullptr ) {
                auto selectedActor = GEditor->GetSelectedActors()->GetTop<AActor>();
                if ( selectedActor != nullptr ) {
                    auto followerComp = selectedActor->GetComponentByClass( USCIFollowerComponent::StaticClass() );
                    if ( followerComp != nullptr )
                        GEditor->SelectComponent( followerComp, true, true, true );
                }
            }
        } )
        , FCanExecuteAction::CreateLambda( []{
            return true;
        } ) );

        USelection::SelectionChangedEvent.AddRaw( this, &FSceneImageCollectorModule::OnSelectObject );

        RegisterVisualizer();
        RegisterDetailsCustomization();
    #endif
    }

    virtual void ShutdownModule() override
    {
    #if WITH_EDITORONLY_DATA
        if ( GUnrealEd != nullptr )
            GUnrealEd->UnregisterComponentVisualizer( USCIFollowerComponent::StaticClass()->GetFName() );
    #endif
    }

#if WITH_EDITORONLY_DATA
private:
    void RegisterDetailsCustomization()
    {
        auto& propertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>( TEXT( "PropertyEditor" ) );
        propertyModule.RegisterCustomClassLayout( USCIFollowerComponent::StaticClass()->GetFName()
        , FOnGetDetailCustomizationInstance::CreateStatic( &FSCIPathControllerCustomization::MakeInstance ) );
    }

    void RegisterVisualizer()
    {
        if ( ensure( GUnrealEd != nullptr ) ) {
            TSharedPtr<FComponentVisualizer> visualizer = MakeShareable( new FSCIPathControllerVisualizer() );
            if ( visualizer.IsValid() ) {
                GUnrealEd->RegisterComponentVisualizer( USCIFollowerComponent::StaticClass()->GetFName(), visualizer );
                visualizer->OnRegister();
            }
        }
    }

    void OnSelectObject( UObject* InSelected )
    {
        auto lockedActor = FSCICommands::Get().LockedActor;
        if ( lockedActor.IsValid() ) {
            auto selection = Cast<USelection>( InSelected );
            if ( selection != nullptr ) {
                TArray<AActor*> selectedActors;
                selection->GetSelectedObjects( selectedActors );
                auto bContainsLockedActor = selectedActors.Contains( lockedActor.Get() );
                if ( !bContainsLockedActor || selectedActors.Num() != 1 ) {
                    if ( GEditor != nullptr ) {
                        GEditor->GetSelectedActors()->DeselectAll();
                        GEditor->SelectActor( lockedActor.Get(), true, true );
                    }
                }
            }
        }
    }

    void SetFollowerActor( AActor* InFollowerActor )
    {
        FSCICommands::Get().LockedActor = InFollowerActor;
    }

private:
    TSharedPtr<FUICommandList> Commands;
#endif
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_PRIMARY_GAME_MODULE( FSceneImageCollectorModule, SceneImageCollector, "SceneImageCollector" );
