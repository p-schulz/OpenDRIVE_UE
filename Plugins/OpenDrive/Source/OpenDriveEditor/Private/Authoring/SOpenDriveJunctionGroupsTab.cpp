#include "Authoring/SOpenDriveJunctionGroupsTab.h"
#include "OpenDriveEditorContext.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "OpenDriveJunctionGroupsTab"

void SOpenDriveJunctionGroupsTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	List = SNew(SListView<FRowPtr>)
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SOpenDriveJunctionGroupsTab::OnGenerateGroupRow)
		.OnSelectionChanged(this, &SOpenDriveJunctionGroupsTab::OnGroupSelectionChanged)
		.SelectionMode(ESelectionMode::Single);

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.5f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Name", "Name")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SEditableTextBox).Text(this, &SOpenDriveJunctionGroupsTab::GetPendingName).OnTextChanged(this, &SOpenDriveJunctionGroupsTab::OnPendingNameChanged)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Type", "Type")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SEditableTextBox).Text(this, &SOpenDriveJunctionGroupsTab::GetPendingType).OnTextChanged(this, &SOpenDriveJunctionGroupsTab::OnPendingTypeChanged)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)[ SNew(SButton).Text(LOCTEXT("AddGroup", "Add Group")).OnClicked(this, &SOpenDriveJunctionGroupsTab::OnAddGroupClicked) ]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("RemoveGroup", "Remove Group")).IsEnabled(this, &SOpenDriveJunctionGroupsTab::HasGroupSelection)
					.OnClicked(this, &SOpenDriveJunctionGroupsTab::OnRemoveGroupClicked)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[ List.ToSharedRef() ]
			]
		]
		+ SSplitter::Slot().Value(0.5f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SNew(STextBlock).Text(this, &SOpenDriveJunctionGroupsTab::GetMembersText).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Junction", "Junction")) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SComboButton).IsEnabled(this, &SOpenDriveJunctionGroupsTab::HasGroupSelection)
						.OnGetMenuContent(this, &SOpenDriveJunctionGroupsTab::BuildJunctionMenu)
						.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDriveJunctionGroupsTab::GetPickedJunctionText) ]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton).Text(LOCTEXT("AddMember", "Add to Group")).IsEnabled(this, &SOpenDriveJunctionGroupsTab::HasGroupSelection)
						.OnClicked(this, &SOpenDriveJunctionGroupsTab::OnAddMemberClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(LOCTEXT("RemoveMember", "Remove from Group")).IsEnabled(this, &SOpenDriveJunctionGroupsTab::HasGroupSelection)
						.OnClicked(this, &SOpenDriveJunctionGroupsTab::OnRemoveMemberClicked)
					]
				]
			]
		]
	];

	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDriveJunctionGroupsTab::RebuildGroupList);
	RebuildGroupList();
}

SOpenDriveJunctionGroupsTab::~SOpenDriveJunctionGroupsTab()
{
	if (Context)
	{
		Context->OnStructureChanged.Remove(StructureHandle);
	}
}

void SOpenDriveJunctionGroupsTab::RebuildGroupList()
{
	Rows.Reset();
	if (Context)
	{
		for (const FOpenDriveJunctionGroup& Group : Context->GetWorking().GetJunctionGroups())
		{
			Rows.Add(MakeShared<FString>(Group.Id));
		}
	}
	if (List.IsValid())
	{
		List->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SOpenDriveJunctionGroupsTab::OnGenerateGroupRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FOpenDriveEditorContext* Ctx = Context;
	const FString Id = Item.IsValid() ? *Item : FString();
	return SNew(STableRow<FRowPtr>, Owner)
		[
			SNew(STextBlock).Text_Lambda([Ctx, Id]()
			{
				const FOpenDriveJunctionGroup* Group = Ctx ? Ctx->GetWorking().FindJunctionGroup(Id) : nullptr;
				return Group ? FText::FromString(FString::Printf(TEXT("%s: %s (%s), %d junction(s)"), *Group->Id, Group->Name.IsEmpty() ? TEXT("(unnamed)") : *Group->Name, *Group->Type, Group->JunctionRefs.Num())) : FText::FromString(Id);
			})
		];
}

void SOpenDriveJunctionGroupsTab::OnGroupSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo)
{
	SelectedGroupId = Item.IsValid() ? *Item : FString();
}

const FOpenDriveJunctionGroup* SOpenDriveJunctionGroupsTab::GetSelectedGroup() const
{
	return (Context && !SelectedGroupId.IsEmpty()) ? Context->GetWorking().FindJunctionGroup(SelectedGroupId) : nullptr;
}

bool SOpenDriveJunctionGroupsTab::HasGroupSelection() const
{
	return GetSelectedGroup() != nullptr;
}

FReply SOpenDriveJunctionGroupsTab::OnAddGroupClicked()
{
	if (Context)
	{
		SelectedGroupId = FOpenDriveModelEdit::AddJunctionGroup(Context->GetWorking(), PendingName, PendingType);
		Context->NotifyStructureChanged();
		for (const FRowPtr& Row : Rows)
		{
			if (Row.IsValid() && *Row == SelectedGroupId)
			{
				List->SetSelection(Row);
				break;
			}
		}
	}
	return FReply::Handled();
}

FReply SOpenDriveJunctionGroupsTab::OnRemoveGroupClicked()
{
	if (Context && !SelectedGroupId.IsEmpty())
	{
		FOpenDriveModelEdit::RemoveJunctionGroup(Context->GetWorking(), SelectedGroupId);
		SelectedGroupId.Reset();
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FText SOpenDriveJunctionGroupsTab::GetMembersText() const
{
	const FOpenDriveJunctionGroup* Group = GetSelectedGroup();
	if (!Group)
	{
		return LOCTEXT("NoGroup", "No junction group selected.");
	}
	if (Group->JunctionRefs.Num() == 0)
	{
		return LOCTEXT("NoMembers", "Members: (none)");
	}
	FString Joined;
	for (const FString& Ref : Group->JunctionRefs)
	{
		Joined += (Joined.IsEmpty() ? TEXT("") : TEXT(", ")) + Ref;
	}
	return FText::Format(LOCTEXT("Members", "Members: {0}"), FText::FromString(Joined));
}

TSharedRef<SWidget> SOpenDriveJunctionGroupsTab::BuildJunctionMenu()
{
	FMenuBuilder Menu(true, nullptr);
	if (Context)
	{
		for (const FOpenDriveJunction& Junction : Context->GetWorking().GetJunctions())
		{
			const FString Id = Junction.Id;
			Menu.AddMenuEntry(FText::FromString(FString::Printf(TEXT("%s: %s"), *Junction.Id, Junction.Name.IsEmpty() ? TEXT("(unnamed)") : *Junction.Name)), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveJunctionGroupsTab::OnJunctionPicked, Id)));
		}
	}
	return Menu.MakeWidget();
}

void SOpenDriveJunctionGroupsTab::OnJunctionPicked(FString JunctionId)
{
	PendingJunctionId = JunctionId;
}

FText SOpenDriveJunctionGroupsTab::GetPickedJunctionText() const
{
	return PendingJunctionId.IsEmpty() ? LOCTEXT("PickJunction", "(pick a junction)") : FText::FromString(PendingJunctionId);
}

FReply SOpenDriveJunctionGroupsTab::OnAddMemberClicked()
{
	if (Context && !SelectedGroupId.IsEmpty() && !PendingJunctionId.IsEmpty())
	{
		FOpenDriveModelEdit::AddJunctionToGroup(Context->GetWorking(), SelectedGroupId, PendingJunctionId);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveJunctionGroupsTab::OnRemoveMemberClicked()
{
	if (Context && !SelectedGroupId.IsEmpty() && !PendingJunctionId.IsEmpty())
	{
		FOpenDriveModelEdit::RemoveJunctionFromGroup(Context->GetWorking(), SelectedGroupId, PendingJunctionId);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
