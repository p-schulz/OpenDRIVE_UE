#include "Authoring/SOpenDriveObjectsTab.h"
#include "OpenDriveEditorContext.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "OpenDriveObjectsTab"

namespace
{
	constexpr double RadToDeg = 180.0 / UE_DOUBLE_PI;
	constexpr double DegToRad = UE_DOUBLE_PI / 180.0;
}

void SOpenDriveObjectsTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	List = SNew(SListView<FRowPtr>)
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SOpenDriveObjectsTab::OnGenerateRow)
		.OnSelectionChanged(this, &SOpenDriveObjectsTab::OnSelectionChanged)
		.SelectionMode(ESelectionMode::Single);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock).Text(this, &SOpenDriveObjectsTab::GetHeaderText)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
			[
				SNew(SButton).Text(LOCTEXT("AddPole", "Add Pole")).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
				.OnClicked(this, &SOpenDriveObjectsTab::OnAddPoleClicked)
			]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
			[
				SNew(SButton).Text(LOCTEXT("AddTree", "Add Tree")).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
				.OnClicked(this, &SOpenDriveObjectsTab::OnAddTreeClicked)
			]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
			[
				SNew(SButton).Text(LOCTEXT("AddBarrier", "Add Barrier")).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
				.OnClicked(this, &SOpenDriveObjectsTab::OnAddBarrierClicked)
			]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
			[
				SNew(SButton).Text(LOCTEXT("Remove", "Remove"))
				.IsEnabled(this, &SOpenDriveObjectsTab::HasSelection).OnClicked(this, &SOpenDriveObjectsTab::OnRemoveClicked)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.55f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[ List.ToSharedRef() ]
			]
			+ SSplitter::Slot().Value(0.45f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("S", "S (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
							.Value(this, &SOpenDriveObjectsTab::GetS).OnValueChanged(this, &SOpenDriveObjectsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("T", "T (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
							.Value(this, &SOpenDriveObjectsTab::GetT).OnValueChanged(this, &SOpenDriveObjectsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("ZOffset", "Z Offset (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
							.Value(this, &SOpenDriveObjectsTab::GetZOffset).OnValueChanged(this, &SOpenDriveObjectsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("HOffset", "Heading Offset (deg)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveObjectsTab::HasSelection)
							.Value(this, &SOpenDriveObjectsTab::GetHOffsetDeg).OnValueChanged(this, &SOpenDriveObjectsTab::OnPoseChanged)
						]
					]
				]
			]
		]
	];

	SelectionHandle = Context->OnSelectionChanged.AddSP(this, &SOpenDriveObjectsTab::RefreshFromRoadSelection);
	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDriveObjectsTab::RebuildList);
	RebuildList();
}

SOpenDriveObjectsTab::~SOpenDriveObjectsTab()
{
	if (Context)
	{
		Context->OnSelectionChanged.Remove(SelectionHandle);
		Context->OnStructureChanged.Remove(StructureHandle);
	}
}

void SOpenDriveObjectsTab::RefreshFromRoadSelection()
{
	SelectedObjectId.Reset();
	RebuildList();
}

void SOpenDriveObjectsTab::RebuildList()
{
	Rows.Reset();
	if (const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr)
	{
		for (const FOpenDriveObject& Obj : Road->Objects)
		{
			Rows.Add(MakeShared<FString>(Obj.Id));
		}
	}
	if (List.IsValid())
	{
		List->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SOpenDriveObjectsTab::OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FOpenDriveEditorContext* Ctx = Context;
	const FString Id = Item.IsValid() ? *Item : FString();
	return SNew(STableRow<FRowPtr>, Owner)
		[
			SNew(STextBlock).Text_Lambda([Ctx, Id]()
			{
				const FOpenDriveRoad* Road = Ctx ? Ctx->GetSelectedRoad() : nullptr;
				const FOpenDriveObject* Obj = Road ? Road->Objects.FindByPredicate([&](const FOpenDriveObject& O) { return O.Id == Id; }) : nullptr;
				return Obj ? FText::FromString(FString::Printf(TEXT("%s: %s (s=%.1f, t=%.1f)"), *Obj->Id, Obj->Type.IsEmpty() ? TEXT("(untyped)") : *Obj->Type, Obj->S, Obj->T)) : FText::FromString(Id);
			})
		];
}

void SOpenDriveObjectsTab::OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingSelection)
	{
		return;
	}
	SelectedObjectId = Item.IsValid() ? *Item : FString();
}

const FOpenDriveObject* SOpenDriveObjectsTab::GetSelectedObject() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return (Road && !SelectedObjectId.IsEmpty()) ? Road->Objects.FindByPredicate([&](const FOpenDriveObject& O) { return O.Id == SelectedObjectId; }) : nullptr;
}

bool SOpenDriveObjectsTab::HasSelection() const
{
	return Context && Context->GetSelectedRoad() != nullptr;
}

void SOpenDriveObjectsTab::AddPreset(const FString& Type, bool bRound)
{
	if (!Context || !Context->GetSelectedRoadMutable())
	{
		return;
	}
	FOpenDriveRoad* Road = Context->GetSelectedRoadMutable();
	const double S = 0.5 * Road->Length;
	const double T = Context->GetWorking().GetOuterBorderT(*Road, S, false) - 1.0;
	SelectedObjectId = bRound
		? FOpenDriveModelEdit::AddRoundObject(*Road, Type, S, T, 0.15, 3.0)
		: FOpenDriveModelEdit::AddObject(*Road, Type, S, T, 1.0, 1.0, 1.0);
	Context->NotifyStructureChanged();
	bUpdatingSelection = true;
	for (const FRowPtr& Row : Rows)
	{
		if (Row.IsValid() && *Row == SelectedObjectId)
		{
			List->SetSelection(Row);
			break;
		}
	}
	bUpdatingSelection = false;
}

FReply SOpenDriveObjectsTab::OnAddPoleClicked()
{
	AddPreset(TEXT("pole"), true);
	return FReply::Handled();
}

FReply SOpenDriveObjectsTab::OnAddTreeClicked()
{
	AddPreset(TEXT("tree"), true);
	return FReply::Handled();
}

FReply SOpenDriveObjectsTab::OnAddBarrierClicked()
{
	AddPreset(TEXT("barrier"), false);
	return FReply::Handled();
}

FReply SOpenDriveObjectsTab::OnRemoveClicked()
{
	if (Context && Context->GetSelectedRoadMutable() && !SelectedObjectId.IsEmpty())
	{
		FOpenDriveModelEdit::RemoveObject(*Context->GetSelectedRoadMutable(), SelectedObjectId);
		SelectedObjectId.Reset();
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

double SOpenDriveObjectsTab::GetS() const
{
	const FOpenDriveObject* Obj = GetSelectedObject();
	return Obj ? Obj->S : 0.0;
}

double SOpenDriveObjectsTab::GetT() const
{
	const FOpenDriveObject* Obj = GetSelectedObject();
	return Obj ? Obj->T : 0.0;
}

double SOpenDriveObjectsTab::GetZOffset() const
{
	const FOpenDriveObject* Obj = GetSelectedObject();
	return Obj ? Obj->ZOffset : 0.0;
}

double SOpenDriveObjectsTab::GetHOffsetDeg() const
{
	const FOpenDriveObject* Obj = GetSelectedObject();
	return Obj ? Obj->HOffset * RadToDeg : 0.0;
}

void SOpenDriveObjectsTab::OnPoseChanged(double)
{
	if (!Context || !Context->GetSelectedRoadMutable() || SelectedObjectId.IsEmpty())
	{
		return;
	}
	FOpenDriveModelEdit::SetObjectPose(*Context->GetSelectedRoadMutable(), SelectedObjectId, GetS(), GetT(), GetZOffset(), GetHOffsetDeg() * DegToRad);
	Context->NotifyValueChanged();
}

FText SOpenDriveObjectsTab::GetHeaderText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (!Road)
	{
		return LOCTEXT("NoRoad", "No road selected. Pick one in the Road List tab.");
	}
	return FText::Format(LOCTEXT("Header", "Objects on road '{0}' (id {1})"), FText::FromString(Road->Name), FText::FromString(Road->Id));
}

#undef LOCTEXT_NAMESPACE
