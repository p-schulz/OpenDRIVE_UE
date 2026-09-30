#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"

class FOpenDriveEditorContext;

/**
 * Dockable tab: lists the static objects (poles, trees, barriers, ...) on the road selected in the Road
 * List tab, with presets to add a round or box-footprint object, and pose fields for the selected one.
 * Mirrors SOpenDriveSignalsTab's structure; objects and signals share the same per-road list + pose-editor
 * shape but are kept as separate model types (FOpenDriveObject has no traffic-control semantics).
 */
class SOpenDriveObjectsTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveObjectsTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDriveObjectsTab() override;

private:
	using FRowPtr = TSharedPtr<FString>;

	TSharedRef<ITableRow> OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo);
	void RebuildList();
	void RefreshFromRoadSelection();

	FReply OnAddPoleClicked();
	FReply OnAddTreeClicked();
	FReply OnAddBarrierClicked();
	FReply OnRemoveClicked();
	bool HasSelection() const;
	void AddPreset(const FString& Type, bool bRound);

	double GetS() const;
	double GetT() const;
	double GetZOffset() const;
	double GetHOffsetDeg() const;
	void OnPoseChanged(double);

	FText GetHeaderText() const;
	const FOpenDriveObject* GetSelectedObject() const;

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	FDelegateHandle SelectionHandle;
	FDelegateHandle StructureHandle;
	FString SelectedObjectId;
	bool bUpdatingSelection = false;
};
