#include "SCISceneCaptureComponent.h"
#include "SCISceneCaptureActor.h"
#include <CineCameraComponent.h>
#include <Camera/CameraComponent.h>
#include <Engine/TextureRenderTarget2D.h>
#include <Engine/Engine.h>
#include <Misc/App.h>

USCISceneCaptureComponent::USCISceneCaptureComponent( const FObjectInitializer& ObjectInitializer )
: Super( ObjectInitializer )
{
    CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    ShowFlags.SetTemporalAA( true );
}

void USCISceneCaptureComponent::BeginPlay()
{
    Super::BeginPlay();

    SetupTextureTarget();
}

void USCISceneCaptureComponent::SetupTextureTarget()
{
    if ( ensure( OwnerSceneCapture.IsValid() ) ) {
        auto imageFormat = OwnerSceneCapture->GetImageFormat();
        auto resolution  = OwnerSceneCapture->GetRenderResolution();
		auto pixelFormat = EPixelFormat::PF_Unknown;
        if ( TextureTarget == nullptr ) {
            auto renderTarget = NewObject<UTextureRenderTarget2D>( this );

            renderTarget->RenderTargetFormat = (imageFormat == ESCIImageFormat::EXR) 
            ? ETextureRenderTargetFormat::RTF_RGBA16f 
            : ETextureRenderTargetFormat::RTF_RGBA8;

            auto pixelFormat = (imageFormat == ESCIImageFormat::EXR) 
            ? PF_FloatRGBA 
            : PF_B8G8R8A8;

            renderTarget->TargetGamma = (imageFormat == ESCIImageFormat::EXR) 
            ? GEngine->GetDisplayGamma() 
            : 1.2f;
		    
            // Demand buffer on GPU
            renderTarget->bGPUSharedFlag = true;
		    
            // Assign render target
            TextureTarget = renderTarget;
        }
        else {
            TextureTarget->RenderTargetFormat = (imageFormat == ESCIImageFormat::EXR) 
            ? ETextureRenderTargetFormat::RTF_RGBA16f 
            : ETextureRenderTargetFormat::RTF_RGBA8;

            TextureTarget->TargetGamma = (imageFormat == ESCIImageFormat::EXR) 
            ? GEngine->GetDisplayGamma() 
            : 1.2f;
            TextureTarget->bForceLinearGamma = true;

			pixelFormat = (imageFormat == ESCIImageFormat::EXR) 
            ? PF_FloatRGBA 
            : PF_B8G8R8A8;
        }

		TextureTarget->InitCustomFormat( resolution.X, resolution.Y, pixelFormat, false );
        TextureTarget->UpdateResourceImmediate( true );
    }
}

void USCISceneCaptureComponent::UpdateSceneCaptureContents( FSceneInterface* InScene )
{
    UpdateCaptureCameraSettings();

    Super::UpdateSceneCaptureContents( InScene );
}

void USCISceneCaptureComponent::UpdateCaptureCameraSettings()
{
    if ( ensure( OwnerSceneCapture.IsValid() ) )
            CopyCameraSettingsToSceneCapture();
}

void USCISceneCaptureComponent::CopyCameraSettingsToSceneCapture()
{
    auto cameraComponent = OwnerSceneCapture->GetCameraComponent();
    if ( cameraComponent != nullptr ) {
        FMinimalViewInfo cameraViewInfo;
        cameraComponent->GetCameraView( 0.0f, cameraViewInfo );

        const auto& srcPPSettings = cameraViewInfo.PostProcessSettings;
        auto& destPPSettings      = PostProcessSettings;

        auto destWeightedBlendables       = destPPSettings.WeightedBlendables;
        // Copy all of the post processing settings
        destPPSettings                    = srcPPSettings;
        destPPSettings.WeightedBlendables = destWeightedBlendables;

        destPPSettings.bOverride_DynamicGlobalIlluminationMethod = true;
        destPPSettings.bOverride_ReflectionMethod                = true;
        destPPSettings.bOverride_LumenSurfaceCacheResolution     = true;
        destPPSettings.bOverride_AutoExposureMethod              = true;
        destPPSettings.bOverride_LensFlareIntensity              = true;
        destPPSettings.bOverride_ColorGradingIntensity           = true;
        destPPSettings.bOverride_AmbientCubemapIntensity         = true;

        auto cineCameraComponent = Cast<UCineCameraComponent>( cameraComponent );
        if ( cineCameraComponent != nullptr ) {
            FOVAngle = 0.0f;
            if ( cineCameraComponent->CurrentFocalLength > 0.0f ) {
                const auto OVER_SCAN_FACTOR       = 1.0f;
                const auto ORIGINAL_FOCAL_LENGTH  = 35.0f;
                const auto OVER_SCAN_SENSOR_WIDTH = cineCameraComponent->Filmback.SensorWidth * OVER_SCAN_FACTOR;
                FOVAngle = FMath::RadiansToDegrees( 2.0f * FMath::Atan( OVER_SCAN_SENSOR_WIDTH / (2.0f * ORIGINAL_FOCAL_LENGTH) ) );
            }
        }
        else {
            FOVAngle = cameraComponent->FieldOfView;
        }

        SetWorldLocationAndRotation( cameraComponent->GetComponentLocation(), cameraComponent->GetComponentRotation() );
    }
}

void USCISceneCaptureComponent::SetOwnerSceneCapture( ASCISceneCaptureActor* InActor )
{
    OwnerSceneCapture = InActor;
}
