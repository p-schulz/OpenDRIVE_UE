#pragma once

#include "CoreMinimal.h"
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

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	FDelegateHandle StructureHandle;
	FDelegateHandle SelectionHandle;
	bool bUpdatingSelection = false;
};
