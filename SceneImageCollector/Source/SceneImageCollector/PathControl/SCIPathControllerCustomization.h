// Copyright Devcoder.
#pragma once
#if WITH_EDITOR
#include <IDetailCustomization.h>
#include <DetailLayoutBuilder.h>
#include <IDistCurveEditor.h>

class FSCIPathControllerCustomization : public IDetailCustomization
{
public:
    FSCIPathControllerCustomization();
    ~FSCIPathControllerCustomization();

    static TSharedRef<IDetailCustomization> MakeInstance();

public:
    virtual void CustomizeDetails( IDetailLayoutBuilder& InDetailBuilder ) override;

    TSharedRef<class IDistributionCurveEditor> CreateCurveEditor();

private:
    FReply AddNewEventPoint();
    TSharedRef<SDockTab> OnSpawnCurveEditor( const FSpawnTabArgs& InSpawnTabArgs );
    FReply OpenRollCurveEditor();
    FReply GenerateRollAngles();

    bool CanAddEventPointBeExecuted() const;
    bool IsOpenCurveEditorEnabled() const;
    bool IsAlwaysOpenRollEditorEnabled() const;

    void OnSplineSelectionChanged( TSharedPtr<FString> InSelectedText, ESelectInfo::Type InSelectInfo );
    TSharedPtr<FString> GetSelectedSplineName();
    void RefreshSplineNames();

private:
    static const FName CurveTabId;

    TObjectPtr<class USCIFollowerComponent> FollowerComponent;
    TArray<TSharedPtr<FString>> SplineNames;
};
#endif
