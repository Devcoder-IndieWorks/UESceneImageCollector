// Copyright Devcoder.
#pragma once
#include "SCIPathRoller.h"
#include "SCIPathSpeedCurve.h"
#include "SCIEventPoints.h"
#include "SCIAutoRollVisualConfig.h"
#include "SCIEasingType.h"
#include <Components/ActorComponent.h>
#include <Components/SplineComponent.h>
#include "SCIFollowerComponent.generated.h"

UENUM( BlueprintType ) 
enum class ESCILoopType : uint8
{
    LT_Replay          UMETA(DisplayName="Replay"),
    LT_ReplayFromStart UMETA(DisplayName="Replay From Start Distance"),
    LT_PingPong        UMETA(DisplayName="Ping Pong")
};

UENUM( BlueprintType ) 
enum class ESCIFactorOperation : uint8
{
    FO_None UMETA(DisplayName="None"),
    FO_Add  UMETA(DisplayName="Add"),
    FO_Mul  UMETA(DisplayName="Multiply")
};

//-----------------------------------------------------------------------------

USTRUCT() 
struct FSCIFollowerInstanceData : public FActorComponentInstanceData
{
    GENERATED_BODY()

    FSCIPathRoller PathRoller;
    FSCIEventPoints EventPoints;
    FInterpCurveFloat SpeedCurve;
#if WITH_EDITORONLY_DATA
    bool IsAlwaysOpenRollCurveEditor;
    FInterpCurveVector LastInfo;
#endif

    FSCIFollowerInstanceData() = default;
    FSCIFollowerInstanceData( const class USCIFollowerComponent* InSourceComponent );

    virtual void ApplyToComponent( UActorComponent* InComponent, const ECacheApplyPhase InCacheApplyPhase ) override;
};

//-----------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FSCIReachedEndSignature, class USCIFollowerComponent*, FollowerComp );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FSCIReachedStartSignature, class USCIFollowerComponent*, FollowerComp );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FSCIStartPathSignature, class USCIFollowerComponent*, FollowerComp );

//-----------------------------------------------------------------------------

UCLASS( BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent) )
class USCIFollowerComponent : public UActorComponent
{
    GENERATED_BODY()
    friend struct FSCIFollowerInstanceData;
public:
    USCIFollowerComponent();

    virtual void BeginPlay() override;
    virtual void InitializeComponent() override;

    virtual void TickComponent( float InDeltaTime, ELevelTick InTickType, FActorComponentTickFunction* InThisTickFunction ) override;

public:
    const FSCIEventPoints& GetEventPoints() const;
    FSCIEventPoints& GetEventPoints();

    const class USCIPathComponent* GetPathToFollow() const;
    class USCIPathComponent* GetPathToFollow();

    void SetPathToFollow( class USplineComponent* InSplineComp );
    class USplineComponent* GetSplineToFollow() const;

    FVector GetLocationAtInputKey( float InKey, ESplineCoordinateSpace::Type InCoordinateSpace ) const;

    void UpdateAutoRotationPoints();

    const FSCIPathRoller& GetPathRoller() const;
    FSCIPathRoller& GetPathRoller();

    const FInterpCurveFloat& GetSpeedCurve() const;
    FInterpCurveFloat& GetSpeedCurve();

#if WITH_EDITOR
    virtual void PostEditChangeProperty( FPropertyChangedEvent& InPropertyChangedEvent ) override;

    UObject* GetRollAnglesDistribution() const;
    UObject* GetSpeedDistribution() const;

    void FixAutoRollAndFollowRotationClash();
    void InforPathAboutSelection();
    void MonitorSpline();
#endif

    UFUNCTION( BlueprintCallable, Category=Path )
    void FollowPath( float InFollowStep );
    UFUNCTION( BlueprintCallable, Category=Path )
    void SetPathOwner( class AActor* InPathOwner );
    UFUNCTION( BlueprintCallable, Category=Path )
    class AActor* GetPathOwner();

    UFUNCTION( BlueprintCallable, Category=Path )
    void Start( float InStartDelay = 0.0f );
    UFUNCTION( BlueprintCallable, Category=Path )
    void Pause();
    UFUNCTION( BlueprintCallable, Category=Path )
    void Stop();
    UFUNCTION( BlueprintCallable, Category=Path )
    void Reverse( bool InIsReverse );

    UFUNCTION( BlueprintCallable, Category=Path )
    USCIEventPointDelegateHolder* GetEventPointDelegateByName( const FName& InName );
    UFUNCTION( BlueprintCallable, Category=Path )
    USCIEventPointDelegateHolder* GetEventPointDelegateByIndex( int32 InIndex );
    UFUNCTION( BlueprintCallable, Category=Path )
    USCIEventPointDelegateHolder* GetEventPointDelegateAll();

    UFUNCTION( BlueprintCallable, Category=Path )
    FSCIEventPoint& GetEventPointByName( const FName& InName );
    UFUNCTION( BlueprintCallable, Category=Path )
    bool EventPointExistByName( const FName& InName );

    UFUNCTION( BlueprintCallable, Category=Path )
    void SetCurrentDistance( float InDistance );
    UFUNCTION( BlueprintCallable, Category=Path )
    float GetSpeedAtDistance( float InDistance ) const;
    UFUNCTION( BlueprintCallable, Category=Path )
    float GetSpeedAtSpeedPoint( int32 InPointIndex ) const;
    UFUNCTION( BlueprintCallable, Category=Path )
    FRotator GetRotationAtDistance( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const;
    UFUNCTION( BlueprintCallable, Category=Path )
    FVector GetLocationAtDistance( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const;
    UFUNCTION( BlueprintCallable, Category=Path )
    void ComputeAutoRotationPoints();
    UFUNCTION( BlueprintCallable, Category=Path )
    bool HasPath() const;

    UFUNCTION( BlueprintPure, Category=Path )
    FVector GetMoveDirection() const;
    UFUNCTION( BlueprintPure, Category=Path )
    FVector GetVelocity() const;

    UFUNCTION( BlueprintNativeEvent, Category=Path )
    FVector ModifyFinalLocation( const FVector& InLocation );
    UFUNCTION( BlueprintNativeEvent, Category=Path )
    FRotator ModifyFinalRotation( const FRotator& InRotation );
    UFUNCTION( BlueprintNativeEvent, Category=Path )
    FRotator ComputeLookAtRotation( class USceneComponent* InTargetComponent, const FVector& InFollowerLocation );

protected:
    virtual void Activate( bool InIsReset = false ) override;
    virtual void Deactivate() override;

private:
    virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;

    void Init();
    void InitCurrentDistance( class USplineComponent* InSplineComp );
    void UpdateCurrentDistanceOnPath( float InDeltaTime );

    float MeasurePathDuration() const;
    void SetupUpdatedComponents();
    void HandleEndOfPath();

    void StartImpl();
    float DistanceToTime( float InDistance );
    void ConfigurePath();

    FVector ComputeNewLocation( float InCurrentDistance ) const;
    FRotator ComputeNewRotation( float InCurrentDistance ) const;

    void UpdateLastMoveDirection( const FVector& InNewLocation, const float InDeltaTime );

    void AlignToCurrentDistance();
    void LookAt();
    void HandleLoopingType();
    void ProcessEvents( float InCurrentDistance );
    float UpdateSpeed( float InDistance );

    ETeleportType GetTeleportType() const;
    FVector MaskLocation( const FVector& InCurrentLocation, const FVector& InComputedNewLocation ) const;
    FRotator MaskRotation( const FRotator& InCurrentRotation, const FRotator& InComputedNewRotation ) const;
    FRotator MaskRotation( const FRotator& InRotation ) const;

    FSCIRotationComputeContext CreateRotationComputeContext();

    float ModulateSpeed( float InSpeed ) const;
    FRotator ModulateRotation( const FRotator& InRotation ) const;

    template< typename TValue, typename TFactor >
    TValue Modulate( TValue InValueToModulate, ESCIFactorOperation InModulateOperation, const TFactor& InModulateFactor ) const
    {
        switch ( InModulateOperation ) {
            case ESCIFactorOperation::FO_Add:
                InValueToModulate += InModulateFactor;
                break;
            case ESCIFactorOperation::FO_Mul:
                InValueToModulate *= InModulateFactor;
                break;
        }

        return InValueToModulate;
    }

public:
    UPROPERTY( BlueprintAssignable, Category=Path )
    FSCIReachedStartSignature OnReachedStart;
    UPROPERTY( BlueprintAssignable, Category=Path )
    FSCIReachedEndSignature OnReachedEnd;
    UPROPERTY( BlueprintAssignable, Category=Path )
    FSCIStartPathSignature OnStartPath;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsTeleportPhysics;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Path )
    float TickInterval;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Path )
    bool IsHidePathInfoText;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsLoop;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    ESCILoopType LoopType;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsStartAtPlay;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Path, meta=(EditCondition="IsStartAtPlay") )
    float StartDelay;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path, meta=(UIMin=0, ClampMin=0) )
    float StartDistance;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Path )
    bool IsReverse;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsInvertRotationOnReverse;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path, meta=(DisplayName="Apply Rotation Mask in Local Space") )
    bool IsRotationMaskLocal;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    FIntVector RotationUpdateMask;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    FIntVector LocationUpdateMask;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsLookAtEventIfNotStarted;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Path )
    bool IsAlignToPathStart;

    UPROPERTY( BlueprintReadWrite, Category=Path )
    TObjectPtr<class USceneComponent> LocationComponent;
    UPROPERTY( BlueprintReadWrite, Category=Path )
    TObjectPtr<class USceneComponent> RotationComponent;
    UPROPERTY( BlueprintReadWrite, Category=Path )
    TObjectPtr<class USceneComponent> RollComponent;
    UPROPERTY( BlueprintReadWrite, Category=Path )
    TObjectPtr<class USceneComponent> LookAtComponent;
    UPROPERTY( BlueprintReadOnly, Category=Path )
    bool IsStarted;
    UPROPERTY( BlueprintReadOnly, Category=Path )
    bool IsPause;
    UPROPERTY( BlueprintReadOnly, Interp, Category=Path )
    float CurrentDistanceOnPath;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed, meta=(DisplayName="Speed", EditCondition="!IsUseSpeedCurve") )
    float SpeedDuration;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed )
    bool IsUseSpeedCurve;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed )
    bool IsTimeBased;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed )
    bool IsOverridePathSpeedPoints;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed, meta=(EditCondition="IsUseSpeedCurve") )
    ESCIFactorOperation SpeedFactorOperation;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed, meta=(EditCondition="IsUseSpeedCurve") )
    float SpeedFactor;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed, meta=(DisplayName="Curve", EditCondition="IsOverridePathSpeedPoints") )
    FInterpCurveFloat InterpSpeedCurve;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Speed, meta=(EditCondition="!IsUseSpeedCurve") )
    ESCIEasingType EasingType;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Speed, meta=(EditCondition="!IsUseSpeedCurve") )
    float CurveDegree;
#if WITH_EDITORONLY_DATA
    UPROPERTY( EditAnywhere, Category=Speed, meta=(DisplayName="Visualization") )
    FSCIPathSpeedPointsDrawConfig SpeedPointDrawConfig;
#endif

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(DisplayName="Follow Path Rotation", EditCondition="!IsUseRotationCurve") )
    bool IsFollowerRotation;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(EditCondition="!IsFollowerRotation") )
    bool IsUseRotationCurve;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(EditCondition="IsUseRotationCurve") )
    bool IsUsePathPitchAndYaw;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    float LookAhead;
    UPROPERTY( EditAnywhere, Category=Rotation )
    bool IsAutoRecompute;
    UPROPERTY( EditAnywhere, Category=Rotation )
    bool IsKeepRelativeDistance;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation )
    bool IsGenerateOnSplineControlPoints;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(EditCondition="!IsGenerateOnSplineControlPoints") )
    int32 RotationPointSteps;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    float RollSampleDistance;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation )
    bool IsClampGeneratedAngles;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation )
    float RollAnglesClampMax;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation )
    float RollAnglesClampMin;
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(DisplayName="Rotation Interpolation Mode") )
    ESCIRollerInterpType RollInterpType;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    ESCIFactorOperation RotationFactorOperation;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    FRotator RotationFactor;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation )
    bool IsOverrideRotationCurve;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(DisplayName="Rotation Points", EditCondition="IsOverrideRotationCurve") )
    FSCIPathRoller PathRoller;
#if WITH_EDITORONLY_DATA
    UPROPERTY( EditAnywhere, Category=Rotation, meta=(DisplayName="Visualization") )
    FSCIAutoRollVisualConfig AutoRollVisualConfig;

    UPROPERTY( EditAnywhere, Category=Events, meta=(DisplayName="Visualization") )
    FSCIEventPointsVisualization EventPointsVisualization;
#endif
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Events )
    bool IsOverridePathEventPoints;
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category=Events, meta=(EditCondition=IsOverridePathEventPoints) )
    FSCIEventPoints EventPoints;

#if WITH_EDITORONLY_DATA
    bool IsAlwaysOpenRollCurveEditor;
#endif

private:
    UPROPERTY( EditAnywhere, Category=Path, meta=(DisplayName="Path Owner") )
    TObjectPtr<class AActor> PathOwner;

    mutable TWeakObjectPtr<class USplineComponent> SplineToFollow;
    float PathDuration;
    float CurrentSplineLength;
    float ElapsedTime;

    FTimerHandle DelayTimer;

    float LastTickTime;
    FVector LastTargetLocation;
    FVector LastMoveDirection;
    FVector Velocity;

    int32 LastPassedEventIndex;

#if WITH_EDITORONLY_DATA
    FInterpCurveVector LastSplineInfo;
    FTransform LastTransform;
    float LastLength;
    float LastCurrentDistance;
#endif
};
