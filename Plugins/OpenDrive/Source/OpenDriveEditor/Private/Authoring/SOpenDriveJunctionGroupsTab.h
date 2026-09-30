#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"

class FOpenDriveEditorContext;
class SEditableTextBox;

/**
 * Dockable tab: create/remove junction groups (map-level, not tied to a selected road) and manage which
 * junctions belong to the selected group -- typically used to tie together the several <junction> elements
 * a roundabout is split into. Deliberately minimal (sketch scope): a group's Type is a free-text field,
 * there is no validation that member junctions are geometrically adjacent, and there is no visualisation.
 */
class SOpenDriveJunctionGroupsTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveJunctionGroupsTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);
	virtual ~SOpenDriveJunctionGroupsTab() override;

private:
	using FRowPtr = TSharedPtr<FString>;

	TSharedRef<ITableRow> OnGenerateGroupRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnGroupSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo);
	void RebuildGroupList();

	FReply OnAddGroupClicked();
	FReply OnRemoveGroupClicked();
	bool HasGroupSelection() const;
	const FOpenDriveJunctionGroup* GetSelectedGroup() const;

	FText GetPendingName() const { return FText::FromString(PendingName); }
	void OnPendingNameChanged(const FText& NewText) { PendingName = NewText.ToString(); }
	FText GetPendingType() const { return FText::FromString(PendingType); }
	void OnPendingTypeChanged(const FText& NewText) { PendingType = NewText.ToString(); }

	FText GetMembersText() const;
	TSharedRef<SWidget> BuildJunctionMenu();
	void OnJunctionPicked(FString JunctionId);
	FText GetPickedJunctionText() const;
	FReply OnAddMemberClicked();
	FReply OnRemoveMemberClicked();

	FOpenDriveEditorContext* Context = nullptr;
	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> List;
	FDelegateHandle StructureHandle;
	FString SelectedGroupId;
	FString PendingName = TEXT("Roundabout");
	FString PendingType = TEXT("roundabout");
	FString PendingJunctionId;
};
