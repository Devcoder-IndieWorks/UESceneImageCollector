// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <Components/SceneCaptureComponent2D.h>
#include "SCISceneCaptureComponent.generated.h"

UCLASS( hidecategories=(Projection), ClassGroup=(CameraSystem) ) 
class USCISceneCaptureComponent : public USceneCaptureComponent2D
{
    GENERATED_UCLASS_BODY()
public:
    virtual void BeginPlay() override;

    void SetOwnerSceneCapture( class ASCISceneCaptureActor* InActor );

protected:
    virtual void UpdateSceneCaptureContents( FSceneInterface* InScene ) override;

private:
    void SetupTextureTarget();
    void UpdateCaptureCameraSettings();
    void CopyCameraSettingsToSceneCapture();

private:
    TWeakObjectPtr<class ASCISceneCaptureActor> OwnerSceneCapture;
};
