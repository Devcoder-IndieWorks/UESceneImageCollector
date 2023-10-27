#include "SCIFollowerComponent.h"
#include "SCIPathComponent.h"
#include "../VLog.h"
#include <Distributions/DistributionFloatConstantCurve.h>
#include <Distributions/DistributionVectorConstantCurve.h>

FSCIFollowerInstanceData::FSCIFollowerInstanceData( const USCIFollowerComponent* InSourceComponent )
: FActorComponentInstanceData( InSourceComponent )
{
    PathRoller  = InSourceComponent->PathRoller;
    SpeedCurve  = InSourceComponent->InterpSpeedCurve;
    EventPoints = InSourceComponent->EventPoints;
#if WITH_EDITORONLY_DATA
    IsAlwaysOpenRollCurveEditor = InSourceComponent->IsAlwaysOpenRollCurveEditor;
    LastInfo = InSourceComponent->LastSplineInfo;
#endif
}

void FSCIFollowerInstanceData::ApplyToComponent( UActorComponent* InComponent, const ECacheApplyPhase InCacheApplyPhase )
{
    FActorComponentInstanceData::ApplyToComponent( InComponent, InCacheApplyPhase );
    auto follower = Cast<USCIFollowerComponent>( InComponent );
    if ( ensure( follower != nullptr ) ) {
        follower->PathRoller       = PathRoller;
        follower->InterpSpeedCurve = SpeedCurve;
        follower->EventPoints      = EventPoints;
    #if WITH_EDITORONLY_DATA
        follower->LastSplineInfo = LastInfo;
        follower->IsAlwaysOpenRollCurveEditor = IsAlwaysOpenRollCurveEditor;
    #endif
    }
}

//-----------------------------------------------------------------------------

USCIFollowerComponent::USCIFollowerComponent()
{
    bTickInEditor = true;
    bWantsInitializeComponent         = true;
    PrimaryComponentTick.bCanEverTick = true;
#if WITH_EDITOR
    PrimaryComponentTick.bStartWithTickEnabled = true;
#else
    PrimaryComponentTick.bStartWithTickEnabled = false;
#endif

    IsTeleportPhysics   = false;
    TickInterval        = 0.0f;
    IsHidePathInfoText  = false;
    IsLoop              = false;
    LoopType            = ESCILoopType::LT_Replay;
    IsStartAtPlay       = true;
    StartDelay          = 0.0f;
    StartDistance       = 0.0f;
    IsReverse           = false;
    IsRotationMaskLocal = true;
    RotationUpdateMask  = FIntVector(1, 1, 1);
    LocationUpdateMask  = FIntVector(1, 1, 1);
    IsAlignToPathStart  = true;
    IsLookAtEventIfNotStarted = false;
    IsInvertRotationOnReverse = false;

    IsStarted = false;
    IsPause   = false;
    CurrentDistanceOnPath = 0.0f;

    SpeedDuration   = 50.0f;
    IsUseSpeedCurve = false;
    IsTimeBased     = false;
    SpeedFactor     = 1.0f;
    EasingType      = ESCIEasingType::ET_Linear;
    CurveDegree     = 2.0f;
    SpeedFactorOperation      = ESCIFactorOperation::FO_None;
    IsOverridePathSpeedPoints = false;

    IsFollowerRotation   = true;
    IsUseRotationCurve   = false;
    IsUsePathPitchAndYaw = false;
    LookAhead            = 0.0f;
    IsAutoRecompute      = false;
    RotationPointSteps   = 10;
    RollSampleDistance   = -1.0f;
    IsClampGeneratedAngles = false;
    RollAnglesClampMax   = 90.0f;
    RollAnglesClampMin   = -90.0f;
    RollInterpType       = ESCIRollerInterpType::RIT_CubicClamped;
    RotationFactor       = FRotator( 1.0 );
    IsKeepRelativeDistance          = false;
    IsGenerateOnSplineControlPoints = false;
    RotationFactorOperation         = ESCIFactorOperation::FO_None;
    IsOverrideRotationCurve         = false;
    IsOverridePathEventPoints       = false;

    PathDuration         = 0.0f;
    CurrentSplineLength  = 0.0f;
    ElapsedTime          = 0.0f;
    LastTickTime         = 0.0f;
    LastTargetLocation   = FVector::ZeroVector;
    LastMoveDirection    = FVector::ZeroVector;
    Velocity             = FVector::ZeroVector;
    LastPassedEventIndex = -1;

#if WITH_EDITORONLY_DATA
    IsAlwaysOpenRollCurveEditor = false;
    LastTransform = FTransform::Identity;
    LastLength    = -1.0f;
    LastCurrentDistance = -1.0f;
#endif
}

#if WITH_EDITOR
void USCIFollowerComponent::PostEditChangeProperty( FPropertyChangedEvent& InPropertyChangedEvent )
{
    if ( InPropertyChangedEvent.Property != nullptr ) {
        if ( InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USCIFollowerComponent, PathOwner) ) {
            SplineToFollow = nullptr;
            Init();
        }

        if ( InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USCIFollowerComponent, RollInterpType) ) {
            auto* splineComp = GetSplineToFollow();
            if ( splineComp != nullptr )
                GetPathRoller().SetInterpolationType( RollInterpType, splineComp );
        }

        if ( (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USCIFollowerComponent, StartDistance)) 
        || (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USCIFollowerComponent, EasingType)) )
            Init();
    }

    Super::PostEditChangeProperty( InPropertyChangedEvent );
}
#endif

void USCIFollowerComponent::InitializeComponent()
{
    if ( !FMath::IsNearlyZero( TickInterval ) )
        PrimaryComponentTick.TickInterval = TickInterval;

    Super::InitializeComponent();

    EventPoints.Init();
    Init();
}

void USCIFollowerComponent::BeginPlay()
{
    Super::BeginPlay();

    Init();

    if ( IsStartAtPlay )
        Start( StartDelay );
}

void USCIFollowerComponent::Activate( bool InIsReset )
{
    Super::Activate( InIsReset );

    if ( InIsReset )
        Init();

    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.SetTickFunctionEnable( true );
}

void USCIFollowerComponent::Deactivate()
{
    Super::Deactivate();

    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.SetTickFunctionEnable( false );
}

AActor* USCIFollowerComponent::GetPathOwner()
{
    return PathOwner;
}

void USCIFollowerComponent::SetPathOwner( AActor* InPathOwner )
{
    PathOwner      = InPathOwner;
    SplineToFollow = nullptr;
    Init();
}

void USCIFollowerComponent::SetPathToFollow( USplineComponent* InSplineComp )
{
    SplineToFollow = InSplineComp;
    Init();
}

void USCIFollowerComponent::Init()
{
    SetupUpdatedComponents();
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return;

    InitCurrentDistance( SplineToFollow.Get() );
    ConfigurePath();
    GetEventPoints().Reset( CurrentDistanceOnPath, IsReverse, LastPassedEventIndex );

    if ( IsAlignToPathStart )
        AlignToCurrentDistance();
}

namespace SCI
{
    void SetUpdatedComponent( TObjectPtr<USceneComponent>& InOutComponentToSet, USceneComponent* InDefaultComp )
    {
        if ( InOutComponentToSet == nullptr )
            InOutComponentToSet = InDefaultComp;
    }
}

void USCIFollowerComponent::SetupUpdatedComponents()
{
    auto owner = GetOwner();
    if ( ensure( owner != nullptr ) ) {
        auto ownerRootComp = owner->GetRootComponent();
        SCI::SetUpdatedComponent( LocationComponent, ownerRootComp );
        SCI::SetUpdatedComponent( RotationComponent, ownerRootComp );
        SCI::SetUpdatedComponent( RollComponent, RotationComponent );
    }
}

USplineComponent* USCIFollowerComponent::GetSplineToFollow() const
{
    USplineComponent* splineComp = nullptr;
    if ( SplineToFollow.IsValid() ) {
        splineComp = SplineToFollow.Get();
    }
    else {
        if ( PathOwner != nullptr ) {
            splineComp = Cast<USplineComponent>( PathOwner->GetComponentByClass( USplineComponent::StaticClass() ) );
            if ( splineComp != nullptr )
                SplineToFollow = splineComp;
        }
    }

    auto pathComp = Cast<USCIPathComponent>( splineComp );
    if ( (pathComp != nullptr) && (pathComp->FollowerComponent != this) )
        pathComp->FollowerComponent = this;

    return splineComp;
}

void USCIFollowerComponent::ConfigurePath()
{
    PathDuration             = FMath::IsNearlyZero( SpeedDuration ) ? 0.0f : MeasurePathDuration();
    SplineToFollow->Duration = PathDuration;
    CurrentSplineLength      = SplineToFollow->GetSplineLength();
}

float USCIFollowerComponent::MeasurePathDuration() const
{
    const int32 STEP_COUNT = 20;
    const float STEP_SIZE  = 1.0f / (float)STEP_COUNT;
    auto averageEase = 0.0f;
    auto alpha       = 0.0f;
    for ( int32 i = 0; i <= STEP_COUNT; ++i, alpha += STEP_SIZE )
        averageEase += SCI::Ease( EasingType, alpha, CurveDegree );

    averageEase      /= (float)(STEP_COUNT + 1);
    auto averageSpeed = SpeedDuration * averageEase;
    auto splineLength = SplineToFollow->GetSplineLength();
    auto duration     = splineLength / averageSpeed;

    return duration;
}

const FSCIEventPoints& USCIFollowerComponent::GetEventPoints() const
{
    return (const_cast<USCIFollowerComponent*>(this))->GetEventPoints();
}

FSCIEventPoints& USCIFollowerComponent::GetEventPoints()
{
    if ( !IsOverridePathEventPoints ) {
        auto pathComp = GetPathToFollow();
        if ( pathComp != nullptr )
            return pathComp->EventPoints;
    }

    return EventPoints;
}

const USCIPathComponent* USCIFollowerComponent::GetPathToFollow() const
{
    return (const_cast<USCIFollowerComponent*>(this))->GetPathToFollow();
}

USCIPathComponent* USCIFollowerComponent::GetPathToFollow()
{
    return Cast<USCIPathComponent>( SplineToFollow.Get() );
}

void USCIFollowerComponent::AlignToCurrentDistance()
{
    FollowPath( 0.0f );
}

void USCIFollowerComponent::TickComponent( float InDeltaTime, ELevelTick InTickType, FActorComponentTickFunction* InThisTickFunction )
{
    Super::TickComponent( InDeltaTime, InTickType, InThisTickFunction );

    if ( !FMath::IsNearlyZero( TickInterval ) ) {
        auto currentTime = GetWorld()->GetTimeSeconds();
        InDeltaTime  = currentTime - LastTickTime;
        LastTickTime = currentTime;
    }

#if WITH_EDITOR
    if ( GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::PIE )
        MonitorSpline();

    InforPathAboutSelection();
    FixAutoRollAndFollowRotationClash();
#endif

    if ( IsLookAtEventIfNotStarted && (LookAtComponent != nullptr) )
        LookAt();

    GetSplineToFollow();
    if ( !IsStarted || IsPause || !SplineToFollow.IsValid() || FMath::IsNearlyZero( SplineToFollow->GetSplineLength() ) )
        return;

    FollowPath( InDeltaTime );
}

void USCIFollowerComponent::MonitorSpline()
{
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return;

    const auto TRAJECTORY_EQUAL    = (LastSplineInfo == SplineToFollow->SplineCurves.Position);
    const auto TRANSFORM_EQUAL     = LastTransform.Equals( SplineToFollow->GetComponentToWorld() );
    const auto DISTANCE_EQUAL      = (LastCurrentDistance == CurrentDistanceOnPath);
    const auto NO_CHANGES_DETECTED = (TRAJECTORY_EQUAL && TRANSFORM_EQUAL && DISTANCE_EQUAL);
    if ( NO_CHANGES_DETECTED )
        return;

    ConfigurePath();

    if ( IsAlignToPathStart && (!TRANSFORM_EQUAL || !TRANSFORM_EQUAL) && (GetWorld()->WorldType == EWorldType::Editor) )
        InitCurrentDistance( SplineToFollow.Get() );

    AlignToCurrentDistance();

    LastCurrentDistance = CurrentDistanceOnPath;
    LastTransform       = SplineToFollow->GetComponentToWorld();
    LastSplineInfo      = SplineToFollow->SplineCurves.Position;

    if ( !TRAJECTORY_EQUAL ) {
        if ( IsKeepRelativeDistance ) {
            auto currentLength = SplineToFollow->GetSplineLength();
            if ( LastLength < 0.0f ) {
                LastLength = currentLength;
            }
            else {
                auto factor = currentLength / LastLength;
                LastLength  = currentLength;
                auto& rotPoints = GetPathRoller().GetRollCurve().Points;
                for ( auto& rotPoint : rotPoints )
                    rotPoint.InVal *= factor;
            }
        }

        if ( IsAutoRecompute )
            UpdateAutoRotationPoints();
    }
}

void USCIFollowerComponent::FollowPath( float InFollowStep )
{
    check( (IsFollowerRotation != IsUseRotationCurve) || (!IsFollowerRotation && !IsUseRotationCurve ) );
    check( SplineToFollow.IsValid() );

    SetupUpdatedComponents();
    check( (LocationComponent != nullptr) && (RotationComponent != nullptr) && (RollComponent != nullptr) );

    UpdateCurrentDistanceOnPath( InFollowStep );
    ProcessEvents( CurrentDistanceOnPath );

    auto newLocation = ComputeNewLocation( CurrentDistanceOnPath );
    auto newRotation = LookAtComponent == nullptr ? ComputeNewRotation( CurrentDistanceOnPath ) 
    : !IsLookAtEventIfNotStarted ? ComputeLookAtRotation( LookAtComponent, newLocation ) 
    : FRotator::ZeroRotator;

    newLocation = ModifyFinalLocation( newLocation );
    newRotation = ModifyFinalRotation( newRotation );

    auto currentLocation = LocationComponent->GetComponentToWorld().GetLocation();
    auto currentRotation = RotationComponent->GetComponentToWorld().GetRotation().Rotator();
    auto finalLocation   = MaskLocation( currentLocation, newLocation );
    auto teleportType    = GetTeleportType();

    auto bUpdatingSameComponent = (LocationComponent == RotationComponent) && (RotationComponent == RollComponent);
    auto bUpdateRotation        = IsFollowerRotation || IsUseRotationCurve || (LookAtComponent != nullptr && !IsLookAtEventIfNotStarted);
    if ( bUpdatingSameComponent && bUpdateRotation ) {
        auto finalRotation = IsRotationMaskLocal ? MaskRotation( currentRotation, newRotation ) : newRotation;

        //VLOG( Log, TEXT( "Location: [%s], Rotation: [%s]" ), *(finalLocation.ToCompactString()), *(finalRotation.ToCompactString()) );
        LocationComponent->SetWorldLocationAndRotation( finalLocation, finalRotation, false, nullptr, teleportType );
    }
    else {
        LocationComponent->SetWorldLocation( finalLocation, false, nullptr, teleportType );
        if ( bUpdateRotation ) {
            auto finalRotation = IsRotationMaskLocal ? MaskRotation( currentRotation, newRotation ) : newRotation;

            if ( RotationComponent == RollComponent ) {
                RotationComponent->SetWorldRotation( finalRotation, false, nullptr, teleportType );
            }
            else {
                FRotator rotationWithoutRoll( finalRotation.Pitch, finalRotation.Yaw, 0.0f );
                RotationComponent->SetWorldRotation( rotationWithoutRoll, false, nullptr, teleportType );
                FRotator roll( 0.0f, 0.0f, finalRotation.Roll );
                RotationComponent->SetRelativeRotation( roll, false, nullptr, teleportType );
            }
        }
    }

    UpdateLastMoveDirection( newLocation, InFollowStep );

    LastTargetLocation = newLocation;
    HandleEndOfPath();
}

void USCIFollowerComponent::UpdateCurrentDistanceOnPath( float InDeltaTime )
{
    const auto CURRENT_LENGTH = SplineToFollow->GetSplineLength();
    const auto REVERSE_SIGN   = IsReverse ? -1.0f : 1.0f;

    if ( IsUseSpeedCurve && !IsTimeBased ) {
        auto newSpeed = UpdateSpeed( CurrentDistanceOnPath );
        newSpeed      = FMath::Max( 1.0f, newSpeed );
        CurrentDistanceOnPath += (REVERSE_SIGN * newSpeed * InDeltaTime);
    }
    else if ( IsUseSpeedCurve && IsTimeBased ) {
        ElapsedTime += (REVERSE_SIGN * InDeltaTime);
        CurrentDistanceOnPath = FSCIPathSpeedCurve::GetSpeedAtDistance( GetSpeedCurve(), ElapsedTime );
    }
    else {
        if ( EasingType != ESCIEasingType::ET_Linear ) {
            ElapsedTime += (REVERSE_SIGN * InDeltaTime);
            auto currentAlpha     = FMath::Clamp( ElapsedTime / PathDuration, 0.0f, 1.0f );
            currentAlpha          = SCI::Ease( EasingType, currentAlpha, CurveDegree );
            CurrentDistanceOnPath = CURRENT_LENGTH * currentAlpha;
        }
        else {
            CurrentDistanceOnPath += (REVERSE_SIGN * InDeltaTime * ModulateSpeed( SpeedDuration ));
        }
    }
}

void USCIFollowerComponent::ProcessEvents( float InCurrentDistance )
{
    GetEventPoints().ProcessEvents( InCurrentDistance, this, IsReverse, LastPassedEventIndex );
}

FVector USCIFollowerComponent::ComputeNewLocation( const float InCurrentDistance ) const
{
    check( SplineToFollow.IsValid() );

    constexpr auto COORD_SPACE = ESplineCoordinateSpace::World;
    const auto pathComp        = Cast<USCIPathComponent>( SplineToFollow.Get() );

    return (pathComp != nullptr) 
    ? pathComp->GetLocationAtDistanceAlongSplineMirrored( InCurrentDistance, COORD_SPACE ) 
    : SplineToFollow->GetLocationAtDistanceAlongSpline( InCurrentDistance, COORD_SPACE );
}

FRotator USCIFollowerComponent::ComputeNewRotation( const float InCurrentDistance ) const
{
    check( SplineToFollow.IsValid() );

    const auto COORD_SPACE             = ESplineCoordinateSpace::Local;
    const auto ROTATION_CURVE_DISTANCE = FMath::Clamp( InCurrentDistance + LookAhead, 0.0f, SplineToFollow->GetSplineLength() );
    auto newRotation = IsFollowerRotation ? SplineToFollow->GetRotationAtDistanceAlongSpline( ROTATION_CURVE_DISTANCE, COORD_SPACE ) 
    : IsUseRotationCurve ? GetRotationAtDistance( ROTATION_CURVE_DISTANCE, COORD_SPACE ) : FRotator::ZeroRotator;
    newRotation = ModulateRotation( newRotation );

    auto newQuat = newRotation.Quaternion();
    if ( IsReverse && IsInvertRotationOnReverse )
        newQuat = newQuat * FQuat( newQuat.GetUpVector(), PI );

    auto worldSpaceRotator = SplineToFollow->GetComponentQuat() * newQuat;
    return worldSpaceRotator.Rotator();
}

FRotator USCIFollowerComponent::ModulateRotation( const FRotator& InRotation ) const
{
    auto euler = Modulate( InRotation.Euler(), RotationFactorOperation, RotationFactor.Euler() );
    return FRotator::MakeFromEuler( euler );
}

FRotator USCIFollowerComponent::ComputeLookAtRotation_Implementation( USceneComponent* InTargetComponent, const FVector& InFollowerLocation )
{
    check( InTargetComponent != nullptr );

    auto target    = InTargetComponent->GetComponentToWorld().GetLocation();
    auto lookAtRot = (target - InFollowerLocation).Rotation();
    return lookAtRot;
}

FVector USCIFollowerComponent::ModifyFinalLocation_Implementation( const FVector& InLocation )
{
    return InLocation;
}

FRotator USCIFollowerComponent::ModifyFinalRotation_Implementation( const FRotator& InRotation )
{
    return InRotation;
}

FVector USCIFollowerComponent::MaskLocation( const FVector& InCurrentLocation, const FVector& InComputedNewLocation ) const
{
    const FVector INVERTED_MASK( !LocationUpdateMask.X, !LocationUpdateMask.Y, !LocationUpdateMask.Z );
    const auto MASKED_CURRENT_LOCATION = InCurrentLocation * INVERTED_MASK;
    const auto MASKED_NEW_LOCATION     = InComputedNewLocation * FVector( LocationUpdateMask );
    return MASKED_NEW_LOCATION + MASKED_CURRENT_LOCATION;
}

FRotator USCIFollowerComponent::MaskRotation( const FRotator& InCurrentRotation, const FRotator& InComputedNewRotation ) const
{
    const FRotator MASKED_CURRENT_ROTATION( InCurrentRotation.Pitch * !RotationUpdateMask.X, InCurrentRotation.Yaw * !RotationUpdateMask.Y, InCurrentRotation.Roll * !RotationUpdateMask.Z );
    const auto MASKED_NEW_ROTATION = MaskRotation( InComputedNewRotation );
    return MASKED_NEW_ROTATION + MASKED_CURRENT_ROTATION;
}

FRotator USCIFollowerComponent::MaskRotation( const FRotator& InRotation ) const
{
    return FRotator( InRotation.Pitch * RotationUpdateMask.X, InRotation.Yaw * RotationUpdateMask.Y, InRotation.Roll * RotationUpdateMask.Z );
}

ETeleportType USCIFollowerComponent::GetTeleportType() const
{
    return IsTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None;
}

void USCIFollowerComponent::UpdateLastMoveDirection( const FVector& InNewLocation, const float InDeltaTime )
{
    if ( InNewLocation.Equals( LastTargetLocation ) ) {
        LastMoveDirection = FVector::ZeroVector;
        return;
    }

    const auto LAST_LOCATION_CHANGE = InNewLocation - LastTargetLocation;
    LastMoveDirection = LAST_LOCATION_CHANGE.GetSafeNormal();
    Velocity          = LAST_LOCATION_CHANGE / InDeltaTime;
}

void USCIFollowerComponent::HandleEndOfPath()
{
    const auto DISTANCE_PAST_PATH_END = CurrentDistanceOnPath > SplineToFollow->GetSplineLength();
    const auto DISTANCE_BEFORE_START  = CurrentDistanceOnPath < 0.0f;

    auto endPointReached = false;
    if ( IsUseSpeedCurve ) {
        endPointReached = DISTANCE_PAST_PATH_END || DISTANCE_BEFORE_START;
    }
    else {
        const auto DURATION_ELAPSED     = ElapsedTime > SplineToFollow->Duration;
        const auto ELAPSED_BEFORE_START = ElapsedTime < 0.0f;
        endPointReached = DISTANCE_PAST_PATH_END || DISTANCE_BEFORE_START || DURATION_ELAPSED || ELAPSED_BEFORE_START;
    }

    if ( endPointReached ) {
        IsStarted = false;

        if ( IsReverse )
            OnReachedStart.Broadcast( this );
        else 
            OnReachedEnd.Broadcast( this );

        if ( IsLoop ) {
            HandleLoopingType();
            IsStarted = true;
        }
    }
}

void USCIFollowerComponent::HandleLoopingType( bool InChangeReverse )
{
    check( SplineToFollow.IsValid() );

    switch ( LoopType ) {
        case ESCILoopType::LT_Replay:
            CurrentDistanceOnPath = IsReverse ? SplineToFollow->GetSplineLength() : 0.0f;
            ElapsedTime = DistanceToTime( CurrentDistanceOnPath );
            break;
        case ESCILoopType::LT_ReplayFromStart:
            InitCurrentDistance( SplineToFollow.Get() );
            ElapsedTime = DistanceToTime( CurrentDistanceOnPath );
            break;
        case ESCILoopType::LT_PingPong:
            if ( InChangeReverse )
                Reverse( !IsReverse );
            CurrentDistanceOnPath = IsReverse ? SplineToFollow->GetSplineLength() - 0.01f : 0.0f;
            ElapsedTime = IsReverse ? SplineToFollow->Duration - 0.01f : 0.0f;
            break;
    }

    GetEventPoints().Reset( CurrentDistanceOnPath, IsReverse, LastPassedEventIndex );
}

float USCIFollowerComponent::DistanceToTime( float InDistance )
{
    return (!FMath::IsNearlyZero( InDistance ) && InDistance > 0.0f) 
    ? InDistance / CurrentSplineLength * PathDuration 
    : 0.0f;
}

void USCIFollowerComponent::InitCurrentDistance( USplineComponent* InSplineComp )
{
    if ( ensure( InSplineComp != nullptr ) ) {
        auto initDistance     = IsReverse ? InSplineComp->GetSplineLength() - StartDistance : StartDistance;
        CurrentDistanceOnPath = FMath::Clamp( initDistance, 0.0f, InSplineComp->GetSplineLength() );
    }
}

void USCIFollowerComponent::Reverse( bool InIsReverse )
{
    IsReverse = InIsReverse;
    GetEventPoints().Reset( CurrentDistanceOnPath, IsReverse, LastPassedEventIndex );
}

void USCIFollowerComponent::UpdateAutoRotationPoints()
{
    auto ctx     = CreateRotationComputeContext();
    auto& roller = GetPathRoller();
    roller.UpdateRollAngles( ctx );
    roller.Dump();
}

FSCIRotationComputeContext USCIFollowerComponent::CreateRotationComputeContext()
{
    FSCIRotationComputeContext ctx;
    ctx.Spline           = GetSplineToFollow();
    ctx.RollStepsNum     = RotationPointSteps;
    ctx.RollSampleLength = RollSampleDistance;
    ctx.ClampAngles      = IsClampGeneratedAngles;
    ctx.MinAngle         = RollAnglesClampMin;
    ctx.MaxAngle         = RollAnglesClampMax;
    ctx.RollerInterpType = RollInterpType;
    ctx.ComputeOnSplineControlPoints = IsGenerateOnSplineControlPoints;
    return ctx;
}

void USCIFollowerComponent::LookAt()
{
    check( LookAtComponent != nullptr );
    check( RotationComponent != nullptr );
    check( LocationComponent != nullptr );
    auto lookAtRotation = ComputeLookAtRotation( LookAtComponent, LocationComponent->GetComponentLocation() );
    RotationComponent->SetWorldRotation( lookAtRotation, false, nullptr, GetTeleportType() );
}

#if WITH_EDITOR
void USCIFollowerComponent::InforPathAboutSelection()
{
    auto owner = GetOwner();
    if ( owner != nullptr ) {
        auto pathComp = GetPathToFollow();
        if ( pathComp != nullptr ) {
            if ( owner->IsSelected() )
                pathComp->OnFollowerSelected();
            else
                pathComp->OnFollowerDeselected();
        }
    }
}

void USCIFollowerComponent::FixAutoRollAndFollowRotationClash()
{
    if ( IsUseRotationCurve && IsFollowerRotation ) {
        VLOG( Warning, TEXT( "IsFollowerRotation turned off. Actor: [%s]" ), *(GetOwner()->GetName()) );
        IsFollowerRotation = false;
    }
}

UObject* USCIFollowerComponent::GetSpeedDistribution() const
{
    auto speedDistribution = NewObject<UDistributionFloatConstantCurve>( const_cast<USCIFollowerComponent*>(this), TEXT( "SpeedCurveDistribution" ) );
    speedDistribution->ConstantCurve = GetSpeedCurve();
    return speedDistribution;
}

UObject* USCIFollowerComponent::GetRollAnglesDistribution() const
{
    auto rollDistribution = NewObject<UDistributionVectorConstantCurve>( const_cast<USCIFollowerComponent*>(this), TEXT( "RollAnglesCurveDistribution" ) );
    const auto& points = GetPathRoller().GetRollCurve().Points;
    for ( const auto& point : points ) {
        auto index      = rollDistribution->ConstantCurve.AddPoint( point.InVal, point.OutVal );
        auto& distPoint = rollDistribution->ConstantCurve.Points[ index ];
        distPoint.ArriveTangent = point.ArriveTangent;
        distPoint.LeaveTangent  = point.LeaveTangent;
        distPoint.InterpMode    = point.InterpMode;
    }

    return rollDistribution;
}
#endif

USCIEventPointDelegateHolder* USCIFollowerComponent::GetEventPointDelegateByName( const FName& InName )
{
    return GetEventPoints().GetEventPointDelegateByName( InName );
}

USCIEventPointDelegateHolder* USCIFollowerComponent::GetEventPointDelegateByIndex( int32 InIndex )
{
    return GetEventPoints().GetEventPointDelegateByIndex( InIndex );
}

USCIEventPointDelegateHolder* USCIFollowerComponent::GetEventPointDelegateAll()
{
    return GetEventPoints().GetEventPointDelegateAll();
}

FVector USCIFollowerComponent::GetLocationAtInputKey( float InKey, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return FVector::ZeroVector;

    auto pathComp = Cast<USCIPathComponent>( SplineToFollow.Get() );
    return (pathComp != nullptr) 
    ? pathComp->GetLocationAtSplineInputKeyMirrored( InKey, InCoordinateSpace ) 
    : SplineToFollow->GetLocationAtSplineInputKey( InKey, InCoordinateSpace );
}

const FSCIPathRoller& USCIFollowerComponent::GetPathRoller() const
{
    return (const_cast<USCIFollowerComponent*>(this))->GetPathRoller();
}

FSCIPathRoller& USCIFollowerComponent::GetPathRoller()
{
    if ( !IsOverrideRotationCurve ) {
        auto pathComp = GetPathToFollow();
        return (pathComp != nullptr) ? pathComp->GetPathRoller() : PathRoller;
    }

    return PathRoller;
}

float USCIFollowerComponent::UpdateSpeed( float InDistance )
{
    check( IsUseSpeedCurve );
    auto speed = FSCIPathSpeedCurve::GetSpeedAtDistance( GetSpeedCurve(), InDistance );
    return ModulateSpeed( speed );
}

float USCIFollowerComponent::GetSpeedAtDistance( float InDistance ) const
{
    auto speed = 0.0f;
    if ( IsUseSpeedCurve && (GetSpeedCurve().Points.Num() > 0) ) {
        speed = FSCIPathSpeedCurve::GetSpeedAtDistance( GetSpeedCurve(), InDistance );
        speed = ModulateSpeed( speed );
    }
    else {
        speed = SpeedDuration;
    }

    return speed;
}

float USCIFollowerComponent::GetSpeedAtSpeedPoint( int32 InPointIndex ) const
{
    auto speed = 0.0f;
    const auto& speedCurve = GetSpeedCurve();
    if ( speedCurve.Points.IsValidIndex( InPointIndex ) ) {
        speed = speedCurve.Points[ InPointIndex ].OutVal;
        speed = ModulateSpeed( speed );
    }

    return speed;
}

const FInterpCurveFloat& USCIFollowerComponent::GetSpeedCurve() const
{
    return (const_cast<USCIFollowerComponent*>(this))->GetSpeedCurve();
}

FInterpCurveFloat& USCIFollowerComponent::GetSpeedCurve()
{
    if ( !IsOverridePathSpeedPoints ) {
        auto pathComp = GetPathToFollow();
        return (pathComp != nullptr) ? pathComp->GetSpeedCurve().GetCurve() : InterpSpeedCurve;
    }

    return InterpSpeedCurve;
}

float USCIFollowerComponent::ModulateSpeed( float InSpeed ) const
{
    return Modulate( InSpeed, SpeedFactorOperation, SpeedFactor );
}

FSCIEventPoint& USCIFollowerComponent::GetEventPointByName( const FName& InName )
{
    auto& points = GetEventPoints().Points;
    for ( auto& point : points ) {
        if ( point.Name == InName )
            return point;
    }

    return FSCIEventPoint::Invalid;
}

bool USCIFollowerComponent::EventPointExistByName( const FName& InName )
{
    const auto& points = GetEventPoints().Points;
    for ( const auto& point : points ) {
        if ( point.Name == InName )
            return true;
    }

    return false;
}

void USCIFollowerComponent::SetCurrentDistance( float InDistance )
{
    auto splineComp = GetSplineToFollow();
    if ( splineComp != nullptr )
        CurrentDistanceOnPath = FMath::Clamp( InDistance, 0.0f, splineComp->GetSplineLength() );

    GetEventPoints().Reset( InDistance, IsReverse, LastPassedEventIndex );

    if ( !IsStarted )
        FollowPath( 0.0f );
}

FRotator USCIFollowerComponent::GetRotationAtDistance( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return FRotator::ZeroRotator;

    auto rotation = (RollInterpType == ESCIRollerInterpType::RIT_LinearStable) 
    ? GetPathRoller().GetRotationAtDistanceStableLinear( InDistance, SplineToFollow.Get() ) 
    : GetPathRoller().GetRollRotationAtDistance( SplineToFollow.Get(), InDistance, IsUsePathPitchAndYaw );

    if ( InCoordinateSpace == ESplineCoordinateSpace::World )
        rotation = (SplineToFollow->GetComponentQuat() * rotation.Quaternion()).Rotator();

    return rotation;
}

FVector USCIFollowerComponent::GetLocationAtDistance( float InDistance, ESplineCoordinateSpace::Type InCoordinateSpace ) const
{
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return FVector::ZeroVector;

    auto pathComp = GetPathToFollow();
    return (pathComp != nullptr) 
    ? pathComp->GetLocationAtDistanceAlongSplineMirrored( InDistance, InCoordinateSpace ) 
    : SplineToFollow->GetLocationAtDistanceAlongSpline( InDistance, InCoordinateSpace );
}

void USCIFollowerComponent::ComputeAutoRotationPoints()
{
    auto splineComp = GetSplineToFollow();
    if ( splineComp != nullptr ) {
        auto& roller = GetPathRoller();
        auto ctx     = CreateRotationComputeContext();
        roller.GenerateRollAngles( ctx );
        roller.Dump();
    }
}

bool USCIFollowerComponent::HasPath() const
{
    return SplineToFollow.IsValid();
}

FVector USCIFollowerComponent::GetMoveDirection() const
{
    return LastMoveDirection;
}

FVector USCIFollowerComponent::GetVelocity() const
{
    return Velocity;
}

void USCIFollowerComponent::Start( float InStartDelay )
{
    GetSplineToFollow();
    if ( !SplineToFollow.IsValid() )
        return;

    if ( FMath::IsNearlyZero( InStartDelay ) )
        StartImpl();
    else
        GetWorld()->GetTimerManager().SetTimer( DelayTimer, this, &USCIFollowerComponent::StartImpl, InStartDelay, false );
}

void USCIFollowerComponent::StartImpl()
{
    if ( IsPause )
        IsPause = false;

    if ( !IsStarted ) {
        InitCurrentDistance( SplineToFollow.Get() );
        GetEventPoints().Reset( CurrentDistanceOnPath, IsReverse, LastPassedEventIndex );
        ElapsedTime = DistanceToTime( CurrentDistanceOnPath );
        IsStarted   = true;
        Activate();

        OnStartPath.Broadcast( this );
    }
}

void USCIFollowerComponent::Pause()
{
    IsPause = true;
}

void USCIFollowerComponent::Stop()
{
    IsStarted = false;
    if ( DelayTimer.IsValid() ) {
        GetWorld()->GetTimerManager().ClearTimer( DelayTimer );
        DelayTimer.Invalidate();
    }

#if !WITH_EDITOR
    if ( !IsLookAtEventIfNotStarted )
        Deactivate();
#endif
}

TStructOnScope<FActorComponentInstanceData> USCIFollowerComponent::GetComponentInstanceData() const
{
    return MakeStructOnScope<FActorComponentInstanceData, FSCIFollowerInstanceData>( this );
}
