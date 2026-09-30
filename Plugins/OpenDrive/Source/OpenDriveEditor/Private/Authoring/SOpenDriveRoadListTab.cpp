#include "Authoring/SOpenDriveRoadListTab.h"
#include "OpenDriveEditorContext.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "OpenDriveRoadListTab"

namespace
{
	constexpr double RadToDeg = 180.0 / UE_DOUBLE_PI;
	constexpr double DegToRad = UE_DOUBLE_PI / 180.0;
}

void SOpenDriveRoadListTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	List = SNew(SListView<FRowPtr>)
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SOpenDriveRoadListTab::OnGenerateRow)
		.OnSelectionChanged(this, &SOpenDriveRoadListTab::OnSelectionChanged)
		.SelectionMode(ESelectionMode::Single);

	auto MakeButton = [](const FText& Label, const FText& Tooltip, TFunction<FReply()> OnClick, TFunction<bool()> IsEnabled)
	{
		return SNew(SButton)
			.Text(Label)
			.ToolTipText(Tooltip)
			.OnClicked_Lambda([OnClick]() { return OnClick(); })
			.IsEnabled_Lambda([IsEnabled]() { return IsEnabled(); });
	};
	auto Always = []() { return true; };

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(LOCTEXT("Add", "Add Road"), LOCTEXT("AddTip", "Add a new straight road"),
					[this]() { return OnAddClicked(); }, Always)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(LOCTEXT("Duplicate", "Duplicate"), LOCTEXT("DuplicateTip", "Duplicate the selected road"),
					[this]() { return OnDuplicateClicked(); }, [this]() { return HasSelection(); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(LOCTEXT("Remove", "Remove"), LOCTEXT("RemoveTip", "Remove the selected road"),
					[this]() { return OnRemoveClicked(); }, [this]() { return HasSelection(); })
			]
		]

		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.45f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[ List.ToSharedRef() ]
			]
			+ SSplitter::Slot().Value(0.55f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Name", "Name")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SEditableTextBox)
							.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
							.Text(this, &SOpenDriveRoadListTab::GetNameText)
							.OnTextCommitted(this, &SOpenDriveRoadListTab::OnNameCommitted)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("StartX", "Start X (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>)
							.IsEnabled(this, &SOpenDriveRoadListTab::IsBasicsEditable)
							.Value(this, &SOpenDriveRoadListTab::GetStartX)
							.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { ApplyBasics(V, GetStartY(), GetStartHeadingDeg(), GetLength()); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("StartY", "Start Y (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>)
							.IsEnabled(this, &SOpenDriveRoadListTab::IsBasicsEditable)
							.Value(this, &SOpenDriveRoadListTab::GetStartY)
							.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { ApplyBasics(GetStartX(), V, GetStartHeadingDeg(), GetLength()); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Heading", "Heading (deg)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>)
							.IsEnabled(this, &SOpenDriveRoadListTab::IsBasicsEditable)
							.Value(this, &SOpenDriveRoadListTab::GetStartHeadingDeg)
							.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { ApplyBasics(GetStartX(), GetStartY(), V, GetLength()); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Length", "Length (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>)
							.MinValue(1.0)
							.IsEnabled(this, &SOpenDriveRoadListTab::IsBasicsEditable)
							.Value(this, &SOpenDriveRoadListTab::GetLength)
							.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { ApplyBasics(GetStartX(), GetStartY(), GetStartHeadingDeg(), V); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetInfoText).AutoWrapText(true)
					]
				]
			]
		]
	];

	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDriveRoadListTab::RebuildList);
	SelectionHandle = Context->OnSelectionChanged.AddLambda([this]()
	{
		if (bUpdatingSelection || !List.IsValid())
		{
			return;
		}
		for (const FRowPtr& Row : Rows)
		{
			if (Row.IsValid() && *Row == Context->GetSelectedRoadId())
			{
				List->SetSelection(Row);
				return;
			}
		}
		List->ClearSelection();
	});

	RebuildList();
}

SOpenDriveRoadListTab::~SOpenDriveRoadListTab()
{
	if (Context)
	{
		Context->OnStructureChanged.Remove(StructureHandle);
		Context->OnSelectionChanged.Remove(SelectionHandle);
	}
}

void SOpenDriveRoadListTab::RebuildList()
{
	Rows.Reset();
	if (Context)
	{
		for (const FOpenDriveRoad& Road : Context->GetWorking().GetRoads())
		{
			Rows.Add(MakeShared<FString>(Road.Id));
		}
	}
	if (List.IsValid())
	{
		List->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SOpenDriveRoadListTab::OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FOpenDriveEditorContext* Ctx = Context;
	const FString Id = Item.IsValid() ? *Item : FString();
	return SNew(STableRow<FRowPtr>, Owner)
		[
			SNew(STextBlock).Text_Lambda([Ctx, Id]()
			{
				const FOpenDriveRoad* Road = Ctx->GetWorking().FindRoad(Id);
				return Road ? FText::FromString(FString::Printf(TEXT("%s: %s (%.0f m)%s"), *Road->Id, Road->Name.IsEmpty() ? TEXT("(unnamed)") : *Road->Name, Road->Length, Road->IsJunctionRoad() ? TEXT(" [junction]") : TEXT(""))) : FText::FromString(Id);
			})
		];
}

void SOpenDriveRoadListTab::OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingSelection || !Context)
	{
		return;
	}
	bUpdatingSelection = true;
	Context->SetSelectedRoadId(Item.IsValid() ? *Item : FString());
	bUpdatingSelection = false;
}

FReply SOpenDriveRoadListTab::OnAddClicked()
{
	if (Context)
	{
		Context->AddRoad();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnDuplicateClicked()
{
	if (Context)
	{
		Context->DuplicateSelectedRoad();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnRemoveClicked()
{
	if (Context)
	{
		Context->RemoveSelectedRoad();
	}
	return FReply::Handled();
}

bool SOpenDriveRoadListTab::HasSelection() const
{
	return Context && Context->GetSelectedRoad() != nullptr;
}

bool SOpenDriveRoadListTab::IsBasicsEditable() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return Road && Road->Geometry.Num() == 1 && Road->Geometry[0].Type == EOpenDriveGeometryType::Line;
}

FText SOpenDriveRoadListTab::GetNameText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return Road ? FText::FromString(Road->Name) : FText::GetEmpty();
}

void SOpenDriveRoadListTab::OnNameCommitted(const FText& NewText, ETextCommit::Type)
{
	if (Context && Context->GetSelectedRoad())
	{
		FOpenDriveModelEdit::RenameRoad(Context->GetWorking(), Context->GetSelectedRoadId(), NewText.ToString());
		Context->NotifyValueChanged();
	}
}

double SOpenDriveRoadListTab::GetStartX() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return (Road && Road->Geometry.Num() > 0) ? Road->Geometry[0].X : 0.0;
}

double SOpenDriveRoadListTab::GetStartY() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return (Road && Road->Geometry.Num() > 0) ? Road->Geometry[0].Y : 0.0;
}

double SOpenDriveRoadListTab::GetStartHeadingDeg() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return (Road && Road->Geometry.Num() > 0) ? Road->Geometry[0].Hdg * RadToDeg : 0.0;
}

double SOpenDriveRoadListTab::GetLength() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return Road ? Road->Length : 0.0;
}

void SOpenDriveRoadListTab::ApplyBasics(double NewX, double NewY, double NewHeadingDeg, double NewLength)
{
	if (!Context || !IsBasicsEditable())
	{
		return;
	}
	FOpenDriveModelEdit::SetStraightRoadBasics(Context->GetWorking(), Context->GetSelectedRoadId(), GetNameText().ToString(), NewX, NewY, NewHeadingDeg * DegToRad, NewLength);
	Context->NotifyValueChanged();
}

FText SOpenDriveRoadListTab::GetInfoText() const
{
	if (!HasSelection())
	{
		return LOCTEXT("NoSelection", "Select a road to edit it, or Add Road to create one.");
	}
	if (!IsBasicsEditable())
	{
		return LOCTEXT("NotEditable", "This road's reference line has more than one geometry segment (e.g. imported arcs/spirals) and is not editable here; edit its elevation/superelevation instead, or remove and re-add it as a straight road.");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
