#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"

class FOpenDriveEditorContext;

/**
 * Dockable tab: lists the signs/traffic lights on the road selected in the Road List tab, with presets
 * to add common ones (stop, yield, speed limit, traffic light) and pose fields for the selected signal.
 */
class SOpenDriveSignalsTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveSignalsTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDriveSignalsTab() override;

private:
	using FRowPtr = TSharedPtr<FString>;

	TSharedRef<ITableRow> OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo);
	void RebuildList();
	void RefreshFromRoadSelection();

	FReply OnAddClicked(FOpenDriveModelEdit::ESignalPreset Preset);
	FReply OnRemoveClicked();
	bool HasSelection() const;

	double GetS() const;
	double GetT() const;
	double GetZOffset() const;
	double GetHOffsetDeg() const;
	void OnPoseChanged(double);

	// --- Lane validity (which lanes the selected signal governs; see FOpenDriveSignal::AppliesToLane) --
	/** "Applies to: all lanes" or the current ranges, one per line. Minimal UI: only a single [From, To]
	 *  range is editable here (Apply replaces whatever ranges the signal already had with just this one) --
	 *  a signal imported with several disjoint ranges keeps them until Apply or Clear is used. */
	FText GetValidityText() const;
	int32 GetValidityFromLane() const { return PendingFromLane; }
	void SetValidityFromLane(int32 V) { PendingFromLane = V; }
	int32 GetValidityToLane() const { return PendingToLane; }
	void SetValidityToLane(int32 V) { PendingToLane = V; }
	FReply OnApplyValidityClicked();
	FReply OnClearValidityClicked();

	FText GetHeaderText() const;
	const FOpenDriveSignal* GetSelectedSignal() const;

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	FDelegateHandle SelectionHandle;
	FDelegateHandle StructureHandle;
	FString SelectedSignalId;
	bool bUpdatingSelection = false;
	int32 PendingFromLane = -1;
	int32 PendingToLane = -1;
};
