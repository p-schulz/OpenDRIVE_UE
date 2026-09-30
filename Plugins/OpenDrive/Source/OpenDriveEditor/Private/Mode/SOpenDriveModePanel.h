#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FOpenDriveEditorContext;
class IDetailsView;
struct FAssetData;

/** Panel of the OpenDRIVE editor mode. */
class SOpenDriveModePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveModePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDriveModePanel() override;

private:
	void OnAssetPicked(const FAssetData& AssetData);
	FString GetAssetPath() const;
	FReply OnImport();
	FReply OnGenerateMeshes();
	FReply OnBakeSelectedRoad();
	FText GetInfoText() const;

	FOpenDriveEditorContext* Context = nullptr;
	TSharedPtr<IDetailsView> SettingsView;
};
