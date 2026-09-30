#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"

class FOpenDriveEditorContext;
class SEditableTextBox;
template <typename NumericType> class SSpinBox;

/**
 * Dockable road/junction browser: select, add, duplicate and remove roads, and edit the basics (name,
 * start pose, length) of simple single-segment roads. Elevation and superelevation are edited in their
 * own tabs once a road is selected here.
 */
class SOpenDriveRoadListTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveRoadListTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDriveRoadListTab() override;

private:
	using FRowPtr = TSharedPtr<FString>;

	TSharedRef<ITableRow> OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo);
	void RebuildList();

	FReply OnAddClicked();
	FReply OnDuplicateClicked();
	FReply OnRemoveClicked();
	bool HasSelection() const;

	// Basics editor (only meaningful for a road with a single straight geometry segment)
	bool IsBasicsEditable() const;
	FText GetNameText() const;
	void OnNameCommitted(const FText& NewText, ETextCommit::Type);
	double GetStartX() const;
	double GetStartY() const;
	double GetStartHeadingDeg() const;
	double GetLength() const;
	void ApplyBasics(double NewX, double NewY, double NewHeadingDeg, double NewLength);
	FText GetInfoText() const;

	// Road type
	FText GetRoadTypeText() const;
	TSharedRef<SWidget> BuildRoadTypeMenu();
	void OnRoadTypePicked(EOpenDriveRoadType Type);

	// Cross-section shape ("road carving")
	double GetCrownHeight() const { return PendingCrownHeight; }
	double GetCrownHalfWidth() const { return PendingCrownHalfWidth; }
	void OnCrownHeightChanged(double V) { PendingCrownHeight = V; }
	void OnCrownHalfWidthChanged(double V) { PendingCrownHalfWidth = FMath::Max(0.1, V); }
	FReply OnApplyCrownShapeClicked();
	FReply OnClearCrownShapeClicked();

	// Road marks
	FText GetSelectedLaneText() const;
	TSharedRef<SWidget> BuildLaneMenu();
	void OnLanePicked(int32 LaneId);
	FText GetMarkTypeText() const;
	TSharedRef<SWidget> BuildMarkTypeMenu();
	void OnMarkTypePicked(EOpenDriveRoadMarkType Type);
	FText GetMarkColorText() const;
	TSharedRef<SWidget> BuildMarkColorMenu();
	void OnMarkColorPicked(EOpenDriveRoadMarkColor Color);
	ECheckBoxState GetMarkBoldState() const;
	void OnMarkBoldChanged(ECheckBoxState NewState);
	bool HasLaneSelection() const;
	FReply OnApplyRoadMarkClicked();

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	FDelegateHandle StructureHandle;
	FDelegateHandle SelectionHandle;
	bool bUpdatingSelection = false;
	double PendingCrownHeight = 0.05;
	double PendingCrownHalfWidth = 1.75;

	static constexpr int32 NoLaneSelected = MAX_int32;
	int32 SelectedLaneId = NoLaneSelected;
	EOpenDriveRoadMarkType PendingMarkType = EOpenDriveRoadMarkType::Solid;
	EOpenDriveRoadMarkColor PendingMarkColor = EOpenDriveRoadMarkColor::Standard;
	bool bPendingMarkBold = false;
};
