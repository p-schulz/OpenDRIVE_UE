#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"

class FOpenDriveEditorContext;
class SComboButton;

/**
 * Dockable "Plan View" tab: lists the reference-line geometry segments of the selected road and lets
 * the user append new Line/Arc/Spiral segments (auto-continuous with the previous segment's end pose --
 * see FOpenDriveModelEdit::AppendLineSegment/AppendArcSegment/AppendSpiralSegment), remove the last one,
 * or split a lane section at a chosen S so its lane widths can be edited independently on either side of
 * the split. There is no free-form dragging of the centreline here: segments are authored by numeric
 * length/curvature, which is a deliberate scope reduction over solving live continuity constraints for
 * draggable control points.
 */
class SOpenDrivePlanViewTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDrivePlanViewTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDrivePlanViewTab() override;

private:
	using FRowPtr = TSharedPtr<int32>;

	TSharedRef<ITableRow> OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner);
	void RebuildList();
	void RefreshFromRoadSelection();

	bool HasRoad() const;
	FText GetHeaderText() const;

	TSharedRef<SWidget> BuildTypeMenu();
	FText GetTypeLabel() const;
	void SetType(EOpenDriveGeometryType NewType);

	double GetLength() const { return PendingLength; }
	void SetLength(double V) { PendingLength = V; }
	double GetCurvature() const { return PendingCurvature; }
	void SetCurvature(double V) { PendingCurvature = V; }
	double GetCurvEnd() const { return PendingCurvEnd; }
	void SetCurvEnd(double V) { PendingCurvEnd = V; }
	bool IsCurvatureEnabled() const { return PendingType != EOpenDriveGeometryType::Line; }
	bool IsCurvEndEnabled() const { return PendingType == EOpenDriveGeometryType::Spiral; }

	FReply OnAppendClicked();
	FReply OnRemoveLastClicked();
	bool CanRemoveLast() const;

	double GetSplitS() const { return PendingSplitS; }
	void SetSplitS(double V) { PendingSplitS = V; }
	FReply OnSplitClicked();

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	TSharedPtr<SComboButton> TypeCombo;
	FDelegateHandle SelectionHandle;
	FDelegateHandle StructureHandle;

	EOpenDriveGeometryType PendingType = EOpenDriveGeometryType::Line;
	double PendingLength = 20.0;
	double PendingCurvature = 0.05;
	double PendingCurvEnd = 0.1;
	double PendingSplitS = 0.0;
};
