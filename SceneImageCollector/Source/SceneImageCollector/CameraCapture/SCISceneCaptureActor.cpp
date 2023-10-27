#include "SCISceneCaptureActor.h"
#include "SCISceneCaptureComponent.h"
#include "SCIAsyncSaveImageTask.h"
#include "SCIRenderRequestTypes.h"
#include "../VLog.h"
#include <Camera/CameraComponent.h>
#include <Camera/CameraActor.h>
#include <Engine/TextureRenderTarget2D.h>
#include <Engine/Engine.h>
#include <Modules/ModuleManager.h>
#include <Kismet/GameplayStatics.h>
#include <GameFramework/PlayerInput.h>
#include <GameFramework/InputSettings.h>
#include <ImageWrapper/Public/IImageWrapperModule.h>
#include <ImageWrapper/Public/IImageWrapper.h>
#include <ImageUtils.h>
#include <EngineUtils.h>
#include <RHICommandList.h>

ASCISceneCaptureActor::ASCISceneCaptureActor( const FObjectInitializer& ObjectInitializer )
: Super( ObjectInitializer )
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent  = CreateDefaultSubobject<USceneComponent>( TEXT( "Root" ) );
    auto component = CreateDefaultSubobject<USCISceneCaptureComponent>( TEXT( "SceneCaptureComponent" ) );
    component->SetOwnerSceneCapture( this );
    SceneCaptureComponent = component;
    SceneCaptureComponent->SetupAttachment( RootComponent );

    MaxDigits        = 5;
    RenderResolution = FIntPoint( 1920, 1080 );
    ImageFormat      = ESCIImageFormat::PNG;
    ImageCounter     = 0;
    LOD              = 0;
    IsForceLODAtPlay = false;

    EnableDefaultInputBindings = false;
    MovementSpeed = 100.0f;
    RotationSpeed = 50.0f;

    ForwardKey    = EKeys::Up;
    BackwardKey   = EKeys::Down;
    RightKey      = EKeys::Right;
    LeftKey       = EKeys::Left;
    UpKey         = EKeys::U;
    DownKey       = EKeys::I;
    CaptureKey    = EKeys::C;
    ChangedLODKey = EKeys::L;
    ResetLODKey   = EKeys::R;
}

void ASCISceneCaptureActor::BeginPlay()
{
    Super::BeginPlay();

    InitializeDefaultInputBindings();
    SetupImageWrapper();
    SetupCameraActor();
    SetupForceGlobalLOD();
}

void ASCISceneCaptureActor::InitializeDefaultInputBindings()
{
    if ( !EnableDefaultInputBindings )
        return;

    static bool BindingsAdded = false;
    if ( !BindingsAdded ) {
        BindingsAdded = true;

        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveForward" ), ForwardKey,   1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveForward" ), BackwardKey, -1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveRight" ),   RightKey,     1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveRight" ),   LeftKey,     -1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveUp" ),      UpKey,        1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_MoveUp" ),      DownKey,     -1.0f ) );

        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_Turn" ),   EKeys::MouseX,  1.0f ) );
        UPlayerInput::AddEngineDefinedAxisMapping( FInputAxisKeyMapping( TEXT( "CameraActor_LookUp" ), EKeys::MouseY, -1.0f ) );

        UPlayerInput::AddEngineDefinedActionMapping( FInputActionKeyMapping( TEXT( "CameraActor_Catpure" ), CaptureKey ) );
        UPlayerInput::AddEngineDefinedActionMapping( FInputActionKeyMapping( TEXT( "CameraActor_ChangedLOD" ), ChangedLODKey ) );
        UPlayerInput::AddEngineDefinedActionMapping( FInputActionKeyMapping( TEXT( "CameraActor_ResetLOD" ), ResetLODKey ) );
    }

    auto playerController = UGameplayStatics::GetPlayerController( GetWorld(), 0 );
    if ( ensure( playerController->IsValidLowLevel() ) ) {
        EnableInput( playerController );

        InputComponent->BindAxis( TEXT( "CameraActor_MoveRight" ),   this, &ASCISceneCaptureActor::AddYPositionOfLocation );
        InputComponent->BindAxis( TEXT( "CameraActor_MoveUp" ),      this, &ASCISceneCaptureActor::AddZPositionOfLocation );
        InputComponent->BindAxis( TEXT( "CameraActor_MoveForward" ), this, &ASCISceneCaptureActor::AddXPositionOfLocation );

        InputComponent->BindAxis( TEXT( "CameraActor_Turn" ),        this, &ASCISceneCaptureActor::AddTurnOfRotation );
        InputComponent->BindAxis( TEXT( "CameraActor_LookUp" ),      this, &ASCISceneCaptureActor::AddLookUpOfRotation );

        InputComponent->BindAction( TEXT( "CameraActor_Catpure" ), EInputEvent::IE_Pressed, this,    &ASCISceneCaptureActor::Capture );
        InputComponent->BindAction( TEXT( "CameraActor_ChangedLOD" ), EInputEvent::IE_Pressed, this, &ASCISceneCaptureActor::ChangeGlobalLOD );
        InputComponent->BindAction( TEXT( "CameraActor_ResetLOD" ), EInputEvent::IE_Pressed, this,   &ASCISceneCaptureActor::ResetGlobalLOD );

        GEngine->GameViewport->SetMouseCaptureMode( EMouseCaptureMode::CaptureDuringMouseDown );
    }
}

void ASCISceneCaptureActor::AddXPositionOfLocation( float InValue )
{
    if ( CameraActor.IsValid() ) {
        auto loc = CameraActor->GetActorLocation();
        FVector newLoc( loc.X + (InValue * MovementSpeed * GetWorld()->GetDeltaSeconds()), loc.Y, loc.Z );
        CameraActor->SetActorLocation( newLoc );
    }
}

void ASCISceneCaptureActor::AddYPositionOfLocation( float InValue )
{
    if ( CameraActor.IsValid() ) {
        auto loc = CameraActor->GetActorLocation();
        FVector newLoc( loc.X, loc.Y + (InValue * MovementSpeed * GetWorld()->GetDeltaSeconds()), loc.Z );
        CameraActor->SetActorLocation( newLoc );
    }
}

void ASCISceneCaptureActor::AddZPositionOfLocation( float InValue )
{
    if ( CameraActor.IsValid() ) {
        auto loc = CameraActor->GetActorLocation();
        FVector newLoc( loc.X, loc.Y, loc.Z + (InValue * MovementSpeed * GetWorld()->GetDeltaSeconds()) );
        CameraActor->SetActorLocation( newLoc );
    }
}

void ASCISceneCaptureActor::AddTurnOfRotation( float InValue )
{
    if ( CameraActor.IsValid() ) {
        auto rot = CameraActor->GetActorRotation();
        FRotator newRot( rot.Pitch, rot.Yaw + (InValue * RotationSpeed * GetWorld()->GetDeltaSeconds()), rot.Roll );
        CameraActor->SetActorRotation( newRot );
    }
}

void ASCISceneCaptureActor::AddLookUpOfRotation( float InValue )
{
    if ( CameraActor.IsValid() ) {
        auto rot = CameraActor->GetActorRotation();
        FRotator newRot( rot.Pitch + (InValue * RotationSpeed * GetWorld()->GetDeltaSeconds()), rot.Yaw, rot.Roll );
        CameraActor->SetActorRotation( newRot );
    }
}

void ASCISceneCaptureActor::Capture()
{
    if ( ImageFormat == ESCIImageFormat::EXR )
        CaptureExrImage();
    else
        CaptureImage();
}

void ASCISceneCaptureActor::CaptureExrImage()
{
    auto component = Cast<USCISceneCaptureComponent>( SceneCaptureComponent );
    if ( ensure( component != nullptr ) ) {
        // Scene capture
        component->CaptureScene();

        // Get RenderContext
        auto renderTargetResource = component->TextureTarget->GameThread_GetRenderTargetResource();

        // New render request
        auto renderRequest        = new FSCIFloatRenderRequest();

        // Read the render target surface data back.
        struct FSCIReadSurfaceFloatContext
        {
            FRenderTarget* SrcRenderTarget;
            TArray<FFloat16Color>* OutData;
            FIntRect Rect;
            ECubeFace CubeFace;
        };

        // Setup GPU command
        FSCIReadSurfaceFloatContext context = {
            renderTargetResource,
            &(renderRequest->Image),
            FIntRect( 0, 0, RenderResolution.X, RenderResolution.Y ),
            ECubeFace::CubeFace_MAX // No cubeface
        };

        ENQUEUE_RENDER_COMMAND( FSCIReadSurfaceCommand )(
        [context]( FRHICommandListImmediate& RHICmdList ){
            RHICmdList.ReadSurfaceFloatData(
                context.SrcRenderTarget->GetRenderTargetTexture(), 
                context.Rect, 
                *context.OutData, 
                context.CubeFace, 
                0, 
                0 );
        });

        ExrRenderRequestQueue.Enqueue( renderRequest );
        renderRequest->RenderFence.BeginFence();
    }
}

void ASCISceneCaptureActor::CaptureImage()
{
    auto component = Cast<USCISceneCaptureComponent>( SceneCaptureComponent );
    if ( ensure( component != nullptr ) ) {
        // Scene capture
        component->CaptureScene();

        // Get RenderContext
        auto renderTargetResource = component->TextureTarget->GameThread_GetRenderTargetResource();

        // New render request
        auto renderRequest        = new FSCIRenderRequest();

        // Read the render target surface data back.
        struct FSCIReadSurfaceContext
        {
            FRenderTarget* SrcRenderTarget;
            TArray<FColor>* OutData;
            FIntRect Rect;
            FReadSurfaceDataFlags Flags;
        };

        // Setup GPU command
        FSCIReadSurfaceContext context = {
            renderTargetResource,
            &(renderRequest->Image),
            FIntRect( 0, 0, RenderResolution.X, RenderResolution.Y ),
            FReadSurfaceDataFlags( RCM_UNorm, CubeFace_MAX )
        };

        ENQUEUE_RENDER_COMMAND( FSCIReadSurfaceCommand )(
        [context]( FRHICommandListImmediate& RHICmdList ){
            RHICmdList.ReadSurfaceData(
                context.SrcRenderTarget->GetRenderTargetTexture(), 
                context.Rect, 
                *context.OutData, 
                context.Flags );
        });

        RenderRequestQueue.Enqueue( renderRequest );
        renderRequest->RenderFence.BeginFence();
    }
}

void ASCISceneCaptureActor::ChangeGlobalLOD()
{
    auto world = GetWorld();
    if ( world != nullptr ) {
        LOD++;
        ForceGlobalLOD( world );
    }
}

void ASCISceneCaptureActor::ResetGlobalLOD()
{
    auto world = GetWorld();
    if ( world != nullptr ) {
        LOD = 0;
        ForceGlobalLOD( world );
    }
}

void ASCISceneCaptureActor::SetupImageWrapper()
{
    // Load the image wrapper module
    auto& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>( FName( TEXT( "ImageWrapper" ) ) );

    if ( ImageFormat == ESCIImageFormat::PNG )
        ImageWrapper = imageWrapperModule.CreateImageWrapper( EImageFormat::PNG );
    else if ( ImageFormat == ESCIImageFormat::JPG )
        ImageWrapper = imageWrapperModule.CreateImageWrapper( EImageFormat::JPEG );
    else if ( ImageFormat == ESCIImageFormat::EXR )
        ImageWrapper = imageWrapperModule.CreateImageWrapper( EImageFormat::EXR );
}

void ASCISceneCaptureActor::SetupCameraActor()
{
    if ( !CameraActor.IsValid() ) {
        for ( TActorIterator<ACameraActor> iter( GetWorld() ); iter; ++iter ) {
            auto actor = *iter;
            if ( actor != nullptr ) {
                CameraActor = actor;
                break;
            }
        }
    }
    VCLOG( CameraActor.IsValid(), Log, TEXT( "Selected camera actor: %s" ), *(CameraActor->GetName()) );

    if ( CameraActor.IsValid() ) {
        auto cameraComponent = CameraActor->GetCameraComponent();
        if ( cameraComponent != nullptr ) {
            cameraComponent->PostProcessSettings.bOverride_BloomMethod = true;
            cameraComponent->PostProcessSettings.BloomMethod           = EBloomMethod::BM_FFT;
        }

        auto playerController = UGameplayStatics::GetPlayerController( GetWorld(), 0 );
        if ( ensure( playerController->IsValidLowLevel() ) ) {
            auto viewTarget = playerController->GetViewTarget();
            playerController->SetViewTarget( CameraActor.Get() );
            if ( viewTarget != nullptr )
                viewTarget->SetActorHiddenInGame( true );
        }
    }
}

void ASCISceneCaptureActor::SetupForceGlobalLOD()
{
    auto world = GetWorld();

    if ( IsForceLODAtPlay && (world != nullptr) )
        ForceGlobalLOD( world );
}

void ASCISceneCaptureActor::ForceGlobalLOD( UWorld* InWorld )
{
    const FString LOD_COMMAND = FString::Printf( TEXT( "r.ForceLOD %i" ), LOD );
    GEngine->Exec( InWorld, *LOD_COMMAND );
}

void ASCISceneCaptureActor::Tick( float InDeltaTime )
{
    Super::Tick( InDeltaTime );

    if ( ImageFormat == ESCIImageFormat::EXR )
        SaveExrImage();
    else 
        SaveImage();
}

void ASCISceneCaptureActor::SaveExrImage()
{
    if ( !ExrRenderRequestQueue.IsEmpty() ) {
        // Peek the next render request from queue.
        FSCIFloatRenderRequest* nextRenderRequest = nullptr;
        ExrRenderRequestQueue.Peek( nextRenderRequest );
        if ( (nextRenderRequest != nullptr) && nextRenderRequest->RenderFence.IsFenceComplete() ) {
            auto fileName = FPaths::ProjectSavedDir() + SubDirectoryName + TEXT( "/img" ) + TEXT( "_" ) + ToStringWithLeadingZeros( ImageCounter );
            fileName     += TEXT( ".exr" );

            ImageWrapper->SetRaw( nextRenderRequest->Image.GetData(), nextRenderRequest->Image.GetAllocatedSize(), RenderResolution.X, RenderResolution.Y, ERGBFormat::RGBAF, 16 );
            const auto& imageData = ImageWrapper->GetCompressed( (int32)EImageCompressionQuality::Uncompressed );
            AsyncSaveImageTask( imageData, fileName );

            ImageCounter++;

            // Delete the first element from render request.
            ExrRenderRequestQueue.Pop();
            delete nextRenderRequest;
        }
    }
}

void ASCISceneCaptureActor::SaveImage()
{
    if ( !RenderRequestQueue.IsEmpty() ) {
        // Peek the next render request from queue.
        FSCIRenderRequest* nextRenderRequest = nullptr;
        RenderRequestQueue.Peek( nextRenderRequest );
        if ( (nextRenderRequest != nullptr) && nextRenderRequest->RenderFence.IsFenceComplete() ) {
            auto fileName = FPaths::ProjectSavedDir() + SubDirectoryName + TEXT( "/img" ) + TEXT( "_" ) + ToStringWithLeadingZeros( ImageCounter );
            fileName += (ImageFormat == ESCIImageFormat::PNG ? TEXT( ".png" ) : TEXT( ".jpeg" ));

            ImageWrapper->SetRaw( nextRenderRequest->Image.GetData(), nextRenderRequest->Image.GetAllocatedSize(), RenderResolution.X, RenderResolution.Y, ERGBFormat::BGRA, 8 );
            const auto& imageData = ImageWrapper->GetCompressed( ImageFormat == ESCIImageFormat::PNG ? (int32)EImageCompressionQuality::Uncompressed : 0 );
            AsyncSaveImageTask( imageData, fileName );

            ImageCounter++;

            // Delete the first element from render request.
            RenderRequestQueue.Pop();
            delete nextRenderRequest;
        }
    }
}

FString ASCISceneCaptureActor::ToStringWithLeadingZeros( int32 InIndex )
{
    auto result = FString::FromInt( InIndex );
    auto stringSize = result.Len();
    auto stringData = MaxDigits - stringSize;
    if ( stringData < 0 ) {
        VLOG( Error, TEXT( "MaxDigits of ImageCounter overflow." ) );
        return result;
    }

    FString leadingZeros = TEXT( "" );
    for ( int32 i = 0; i < stringData; i++ )
        leadingZeros += TEXT( "0" );

    return leadingZeros + result;
}

void ASCISceneCaptureActor::AsyncSaveImageTask( const TArray64<uint8>& InImage, const FString& InImageName )
{
    VLOG( Log, TEXT( "Running Async Task." ) );
    (new FAutoDeleteAsyncTask<FSCIAsyncSaveImageTask>( InImage, InImageName ))->StartBackgroundTask();
}

UCameraComponent* ASCISceneCaptureActor::GetCameraComponent() const
{
    return CameraActor.IsValid() ? CameraActor->GetCameraComponent() : nullptr;
}

FIntPoint ASCISceneCaptureActor::GetRenderResolution() const
{
    return RenderResolution;
}

ESCIImageFormat ASCISceneCaptureActor::GetImageFormat() const
{
    return ImageFormat;
}
