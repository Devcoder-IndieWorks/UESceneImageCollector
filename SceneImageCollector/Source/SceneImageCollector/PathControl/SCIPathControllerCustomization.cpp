#if WITH_EDITOR
#include "SCIPathControllerCustomization.h"
#include "SCIFollowerComponent.h"
#include <Engine/InterpCurveEdSetup.h>
#include <Components/SplineComponent.h>
#include <Distributions/DistributionFloatConstantCurve.h>
#include <Distributions/DistributionVectorConstantCurve.h>
#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>
#include <PropertyCustomizationHelpers.h>
#include <DistCurveEditorModule.h>
#include <SlateBasics.h>
#include <SlateExtras.h>

struct FSCICurveDistPair
{
    FInterpCurveFloat& Curve;
    TObjectPtr<const UObject> Distrib;

    FSCICurveDistPair( FInterpCurveFloat& InCurve, const UObject* InDistrib )
    : Curve( InCurve )
    , Distrib( InDistrib )
    {
    }
};
using FSCICurveDistributionPairArray = TArray<FSCICurveDistPair>;

//-----------------------------------------------------------------------------

class FSCICurveEdNotify : public FCurveEdNotifyInterface
{
public:
    FSCICurveEdNotify( FInterpCurveFloat& InSpeedCurve, FInterpCurveVector& InRotationCurve )
    : SpeedCurve( InSpeedCurve )
    , RotationCurve( InRotationCurve )
    {
    }
    virtual ~FSCICurveEdNotify() = default;

    virtual void PreEditCurve( TArray<UObject*> InCurvesAboutToChange ) override
    {
        EdittedDistributions = InCurvesAboutToChange;
    }

    virtual void PostEditCurve() override
    {
        for ( auto& distrib : EdittedDistributions ) {
            auto edittedDistrib = Cast<UDistributionFloatConstantCurve>( distrib );
            if ( edittedDistrib != nullptr )
                SpeedCurve = edittedDistrib->ConstantCurve;

            auto rollDistrib = Cast<UDistributionVectorConstantCurve>( distrib );
            if ( rollDistrib != nullptr ) {
                RotationCurve.Reset();
                for ( auto& point : rollDistrib->ConstantCurve.Points ) {
                    auto index = RotationCurve.AddPoint( point.InVal, point.OutVal );
                    RotationCurve.Points[ index ].InterpMode    = point.InterpMode;
                    RotationCurve.Points[ index ].ArriveTangent = point.ArriveTangent;
                    RotationCurve.Points[ index ].LeaveTangent  = point.LeaveTangent;
                }
            }
        }
    }

private:
    FInterpCurveFloat& SpeedCurve;
    FInterpCurveVector& RotationCurve;
    TArray<UObject*> EdittedDistributions;
};

//-----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FSCIPathControllerCustomization"

const FName FSCIPathControllerCustomization::CurveTabId( TEXT( "PathController" ) );

FSCIPathControllerCustomization::FSCIPathControllerCustomization()
{
    FollowerComponent = nullptr;
}

FSCIPathControllerCustomization::~FSCIPathControllerCustomization()
{
    if ( FGlobalTabmanager::Get()->HasTabSpawner( CurveTabId ) ) {
        auto rollCurveEditorTab = FGlobalTabmanager::Get()->TryInvokeTab( CurveTabId );
        rollCurveEditorTab->RequestCloseTab();
        FGlobalTabmanager::Get()->UnregisterTabSpawner( CurveTabId );
    }
}

TSharedRef<IDetailCustomization> FSCIPathControllerCustomization::MakeInstance()
{
    return MakeShareable( new FSCIPathControllerCustomization() );
}

void FSCIPathControllerCustomization::CustomizeDetails( IDetailLayoutBuilder& InDetailBuilder )
{
    TArray<TWeakObjectPtr<UObject>> outObjects;
    InDetailBuilder.GetObjectsBeingCustomized( outObjects );
    if ( ensure( outObjects.Num() > 0 ) ) {
        FollowerComponent   = CastChecked<USCIFollowerComponent>( outObjects[ 0 ].Get() );
        auto& pathCategory  = InDetailBuilder.EditCategory( TEXT( "Path" ), FText::FromString( TEXT( "General" ) ), ECategoryPriority::Important );
        pathCategory.AddCustomRow( FText::FromString( TEXT( "OpenCurveEditorRow" ) ) )
        .NameContent() [
            SNew( SButton )
            .IsEnabled_Raw( this, &FSCIPathControllerCustomization::IsOpenCurveEditorEnabled )
            .Text( FText::FromString( TEXT( "Open Curve Editor" ) ) )
            .OnClicked_Raw( this, &FSCIPathControllerCustomization::OpenRollCurveEditor )
            .ToolTipText( FText::FromString( TEXT( "Click to open curve editor to fine tune rotation and speed." ) ) )
        ]
        .ValueContent() [
            SNew( SGridPanel )
            .ToolTipText( FText::FromString( TEXT( "Check this to automatically open curve editor everytime the FSCIFollowerComponent gets selected" ) ) )

            +SGridPanel::Slot( 0, 0 )
            .HAlign( HAlign_Center )
            .VAlign( VAlign_Center ) [
                SNew( STextBlock )
                .Text( FText::FromString( TEXT( "Always Open" ) ) )
            ]

            +SGridPanel::Slot( 1, 0 )
            .Padding( 5 ) [
                SNew( SCheckBox )
                .IsChecked( FollowerComponent->IsAlwaysOpenRollCurveEditor ? ECheckBoxState::Checked : ECheckBoxState::Unchecked )
                .IsEnabled_Raw( this, &FSCIPathControllerCustomization::IsAlwaysOpenRollEditorEnabled )
                .OnCheckStateChanged_Lambda( [this](ECheckBoxState InNewState){
                    FollowerComponent->IsAlwaysOpenRollCurveEditor = (InNewState == ECheckBoxState::Checked);
                } )
            ]
        ];

        auto& eventCategory = InDetailBuilder.EditCategory( TEXT( "Events" ), FText::FromString( TEXT( "Events" ) ), ECategoryPriority::Important );
        eventCategory.AddCustomRow( FText::FromString( TEXT( "EventPointRow" ) ) )
        .WholeRowContent() [
            SNew( SButton )
            .IsEnabled_Raw( this, &FSCIPathControllerCustomization::CanAddEventPointBeExecuted )
            .Text( FText::FromString( TEXT( "Create new event point" ) ) )
            .OnClicked_Raw( this, &FSCIPathControllerCustomization::AddNewEventPoint )
        ];

        auto& speedCategory = InDetailBuilder.EditCategory( TEXT( "Speed" ), FText::FromString( TEXT( "Speed" ) ), ECategoryPriority::Important );
        speedCategory.AddCustomRow( FText::FromString( TEXT( "SpeedPointRow" ) ) )
        .WholeRowContent() [
            SNew( SButton )
            .IsEnabled_Lambda( [this]{
                return FollowerComponent->IsUseSpeedCurve && FollowerComponent->HasPath();
            } )
            .Text( FText::FromString( TEXT( "Create New Speed Point" ) ) )
            .OnClicked_Lambda( [this]{
                if ( ensure( FollowerComponent != nullptr ) ) {
                    auto splineComp = FollowerComponent->GetSplineToFollow();
                    if ( ensure( splineComp != nullptr ) ) {
                        auto distance = splineComp->GetSplineLength() / 5.0f;
                        auto& curve   = FollowerComponent->GetSpeedCurve();
                        curve.AddPoint( distance, 0.0f );
                    }
                }
                return FReply::Handled();
            } )
        ];

        RefreshSplineNames();

        if ( SplineNames.Num() > 0 ) {
            auto selectedPathName = GetSelectedSplineName();
            pathCategory.AddCustomRow( FText::FromString( TEXT( "PathSelectionRow" ) ) )
            .NameContent() [
                SNew( STextBlock )
                .Text( FText::FromString( TEXT( "Path Selection" ) ) )
            ]
            .ValueContent() [
                SNew( STextComboBox )
                .OptionsSource( &SplineNames )
                .InitiallySelectedItem( selectedPathName )
                .OnSelectionChanged( this, &FSCIPathControllerCustomization::OnSplineSelectionChanged )
            ];
        }
        else if ( FollowerComponent->GetPathOwner() != nullptr ) {
            auto msg = FText::FromString( TEXT( "No FSCIPathComponent found." ) );
            FollowerComponent->SetPathOwner( nullptr );
            FMessageDialog::Open( EAppMsgType::Ok, FText::FromString( TEXT( "Path owner does not contain any Path/Spline Component. Please choose different actor." ) ), &msg );
        }

        if ( FollowerComponent->IsAlwaysOpenRollCurveEditor && (FollowerComponent->IsUseRotationCurve || FollowerComponent->IsUseSpeedCurve) )
            OpenRollCurveEditor();

        auto& rotationCategory = InDetailBuilder.EditCategory( TEXT( "Rotation" ), FText::FromString( TEXT( "Rotation" ) ), ECategoryPriority::Important );
        rotationCategory.AddCustomRow( FText::FromString( TEXT( "RotationPointRow" ) ) )
        .WholeRowContent() [
            SNew( SButton )
            .IsEnabled_Lambda( [this]{
                return FollowerComponent->IsUseRotationCurve && FollowerComponent->HasPath();
            } )
            .Text( FText::FromString( TEXT( "Create New Rotation Point" ) ) )
            .OnClicked_Lambda( [this]{
                if ( ensure( FollowerComponent != nullptr )  ) {
                    auto splineComp = FollowerComponent->GetSplineToFollow();
                    if ( ensure( splineComp != nullptr ) ) {
                        auto distance = splineComp->GetSplineLength() / 5.0f;
                        auto rotation = splineComp->GetRotationAtDistanceAlongSpline( distance, ESplineCoordinateSpace::World );
                        auto& curve   = FollowerComponent->GetPathRoller().GetRollCurve();
                        curve.AddPoint( distance, rotation.Euler() );
                        if ( curve.Points.Num() == 1 )
                            FollowerComponent->IsFollowerRotation = false;
                    }
                }
                return FReply::Handled();
            } )
        ];
        rotationCategory.AddCustomRow( FText::FromString( TEXT( "GenerateRotationPointRow" ) ) )
        .WholeRowContent() [
            SNew( SButton )
            .Text( FText::FromString( TEXT( "Generate Rotation Points" ) ) )
            .ToolTipText( FText::FromString( TEXT( "Pressing this will regenerate rotation points. All custom changes to rotation points will be lost." ) ) )
            .OnClicked_Raw( this, &FSCIPathControllerCustomization::GenerateRollAngles )
            .IsEnabled_Lambda( [this]{
                return (FollowerComponent != nullptr) && (FollowerComponent->HasPath() || (FollowerComponent->GetPathOwner() != nullptr));
            } )
        ];
    }
}

bool FSCIPathControllerCustomization::IsOpenCurveEditorEnabled() const
{
    return IsAlwaysOpenRollEditorEnabled() && FollowerComponent->IsUseRotationCurve;
}

bool FSCIPathControllerCustomization::IsAlwaysOpenRollEditorEnabled() const
{
    return (FollowerComponent != nullptr);
}

FReply FSCIPathControllerCustomization::OpenRollCurveEditor()
{
    if ( !FGlobalTabmanager::Get()->HasTabSpawner( CurveTabId ) )
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner( CurveTabId, FOnSpawnTab::CreateRaw( this, &FSCIPathControllerCustomization::OnSpawnCurveEditor ) )
        .SetDisplayName( LOCTEXT( "SCIPathControllerCustomization", "Path Curve Editor" ) )
        .SetMenuType( ETabSpawnerMenuType::Hidden );

    FGlobalTabmanager::Get()->TryInvokeTab( CurveTabId );
    return FReply::Handled();
}

TSharedRef<SDockTab> FSCIPathControllerCustomization::OnSpawnCurveEditor( const FSpawnTabArgs& InSpawnTabArgs )
{
    auto curveEditor = CreateCurveEditor();
    return SNew( SDockTab )
    .TabRole( ETabRole::NomadTab ) [
        SNew( SScrollBox )
        +SScrollBox::Slot()
        .Padding( 10, 5 ) [
            curveEditor
        ]
    ];
}

TSharedRef<IDistributionCurveEditor> FSCIPathControllerCustomization::CreateCurveEditor()
{
    auto curveEdSetup      = NewObject<UInterpCurveEdSetup>( FollowerComponent, NAME_None, RF_Transactional );
    auto curveEditorModule = &FModuleManager::LoadModuleChecked<IDistributionCurveEditorModule>( TEXT( "DistCurveEditor" ) );
    auto rollCurveDistrib  = FollowerComponent->GetRollAnglesDistribution();
    auto speedCurveDistrib = FollowerComponent->GetSpeedDistribution();
    curveEdSetup->AddCurveToCurrentTab( rollCurveDistrib, TEXT( "Rotation Curve" ), FColor::Red );
    curveEdSetup->AddCurveToCurrentTab( speedCurveDistrib, TEXT( "Speed Curve" ), FColor::Green );
    FollowerComponent->CurveEdSetup = curveEdSetup;

    auto notify = new FSCICurveEdNotify( FollowerComponent->GetSpeedCurve(), FollowerComponent->GetPathRoller().GetRollCurve() );
    auto curveEditor = curveEditorModule->CreateCurveEditorWidget( curveEdSetup, notify );
    curveEditor->FitViewHorizontally();
    curveEditor->FitViewVertically();
    curveEditor->RefreshViewport();

    return curveEditor;
}

bool FSCIPathControllerCustomization::CanAddEventPointBeExecuted() const
{
    return (FollowerComponent != nullptr) && FollowerComponent->HasPath();
}

FReply FSCIPathControllerCustomization::AddNewEventPoint()
{
    if ( ensure( FollowerComponent != nullptr ) ) {
        FSCIEventPoint newPoint;
        newPoint.Distance = 10.0f;
        FollowerComponent->GetEventPoints().Points.Add( newPoint );
    }
    return FReply::Handled();
}

void FSCIPathControllerCustomization::RefreshSplineNames()
{
    if ( FollowerComponent != nullptr && FollowerComponent->GetPathOwner() != nullptr ) {
        SplineNames.Reset();

        TArray<USplineComponent*> splines;
        FollowerComponent->GetPathOwner()->GetComponents( splines );

        for ( auto spline : splines )
            SplineNames.Add( MakeShareable( new FString( spline->GetName() ) ) );
    }
}

TSharedPtr<FString> FSCIPathControllerCustomization::GetSelectedSplineName()
{
    if ( ensure( FollowerComponent != nullptr ) ) {
        auto splineComp = FollowerComponent->GetSplineToFollow();
        if ( ensure( splineComp != nullptr ) ) {
            auto name      = splineComp->GetName();
            auto foundName = SplineNames.FindByPredicate( [name]( TSharedPtr<FString> InExistingName ){
                return *InExistingName == name;
            } );

            return (foundName != nullptr) ? *foundName : SplineNames[ 0 ];
        }
    }

    return TSharedPtr<FString>();
}

void FSCIPathControllerCustomization::OnSplineSelectionChanged( TSharedPtr<FString> InSelectedText, ESelectInfo::Type InSelectInfo )
{
    if ( ensure( (FollowerComponent != nullptr) && (FollowerComponent->GetPathOwner() != nullptr) ) ) {
        TArray<USplineComponent*> splines;
        FollowerComponent->GetPathOwner()->GetComponents( splines );

        auto selectedSpline = splines.FindByPredicate( [InSelectedText](USplineComponent* InSpline){
            return (InSpline != nullptr) && (InSpline->GetName() == *InSelectedText);
        } );

        if ( selectedSpline != nullptr )
            FollowerComponent->SetPathToFollow( *selectedSpline );
    }
}

FReply FSCIPathControllerCustomization::GenerateRollAngles()
{
    if ( ensure( FollowerComponent != nullptr ) ) {
        auto splineComp = FollowerComponent->GetSplineToFollow();
        if ( splineComp != nullptr ) {
            FollowerComponent->ComputeAutoRotationPoints();
            FollowerComponent->IsFollowerRotation = false;
            FollowerComponent->IsUseRotationCurve = true;
        }
        else {
            auto msg = FText::FromString( TEXT( "No Path Assigned!" ) );
            FMessageDialog::Open( EAppMsgType::Ok, FText::FromString( TEXT( "Please assign path owner before generating rotation points." ) ), &msg );
        }
    }

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
#endif
