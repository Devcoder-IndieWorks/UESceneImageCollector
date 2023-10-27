// Copyright Devcoder.
#pragma once
#include <GameFramework/Actor.h>
#include "SCISceneCaptureActor.generated.h"

UENUM()
enum class ESCIImageFormat
{
    PNG,
    JPG,
    EXR
};

UCLASS( Blueprintable, ClassGroup=(CameraSystem) ) 
class ASCISceneCaptureActor : public AActor
{
    GENERATED_UCLASS_BODY()
public:
    UFUNCTION( BlueprintCallable, Category=Capture )
    void Capture();

public:
    virtual void Tick( float InDeltaTime ) override;

    class UCameraComponent* GetCameraComponent() const;
    FIntPoint GetRenderResolution() const;
    ESCIImageFormat GetImageFormat() const;

protected:
    virtual void BeginPlay() override;

private:
    void SetupCameraActor();
    void SetupImageWrapper();
    void SetupForceGlobalLOD();

    void CaptureImage();
    void CaptureExrImage();

    void SaveImage();
    void SaveExrImage();

    void AsyncSaveImageTask( const TArray64<uint8>& InImage, const FString& InImageName );
    FString ToStringWithLeadingZeros( int32 InIndex );

    void InitializeDefaultInputBindings();
    void AddXPositionOfLocation( float InValue );
    void AddYPositionOfLocation( float InValue );
    void AddZPositionOfLocation( float InValue );
    void AddTurnOfRotation( float InValue );
    void AddLookUpOfRotation( float InValue );

    void ChangeGlobalLOD();
    void ResetGlobalLOD();
    void ForceGlobalLOD( UWorld* InWorld );

private:
    UPROPERTY( EditAnywhere, Category="SCI|Capture" )
    FString SubDirectoryName;
    UPROPERTY( EditAnywhere, Category="SCI|Capture" )
    int32 MaxDigits;
    UPROPERTY( EditAnywhere, Category="SCI|Capture" )
    FIntPoint RenderResolution;
    UPROPERTY( EditAnywhere, Category="SCI|Capture" )
    ESCIImageFormat ImageFormat;
    UPROPERTY( EditAnywhere, Category="SCI|Capture" )
    TWeakObjectPtr<class ACameraActor> CameraActor;

    UPROPERTY( VisibleAnywhere, Category="SCI|Capture" )
    TObjectPtr<class USceneCaptureComponent2D> SceneCaptureComponent;

    UPROPERTY( EditAnywhere, Category="SCI|Settings" )
    bool EnableDefaultInputBindings;
    UPROPERTY( EditAnywhere, Category="SCI|Settings" )
    float MovementSpeed;
    UPROPERTY( EditAnywhere, Category="SCI|Settings" )
    float RotationSpeed;
    UPROPERTY( EditAnywhere, Category="SCI|Settings" )
    bool IsForceLODAtPlay;
    UPROPERTY( EditAnywhere, Category="SCI|Settings" )
    int32 LOD;

    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey ForwardKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey BackwardKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey RightKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey LeftKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey UpKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey DownKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey CaptureKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey ChangedLODKey;
    UPROPERTY( EditAnywhere, Category="SCI|Settings|Keys" )
    FKey ResetLODKey;

    TSharedPtr<class IImageWrapper> ImageWrapper;

    int32 ImageCounter;

    TQueue<struct FSCIRenderRequest*> RenderRequestQueue;
    TQueue<struct FSCIFloatRenderRequest*> ExrRenderRequestQueue;
};
