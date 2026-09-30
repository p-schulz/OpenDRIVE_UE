#include "Authoring/SOpenDriveRoadListTab.h"
#include "OpenDriveEditorContext.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Styling/AppStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("RoadType", "Road Type")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SComboButton)
							.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
							.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetRoadTypeText) ]
							.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildRoadTypeMenu)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 4.f, 4.f)
					[
						SNew(SExpandableArea)
						.AreaTitle(LOCTEXT("CrownSection", "Cross-Section Shape (\"road carving\")"))
						.InitiallyCollapsed(true)
						.BodyContent()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("CrownHeight", "Crown Height (m)")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SSpinBox<double>)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.Value(this, &SOpenDriveRoadListTab::GetCrownHeight)
									.OnValueChanged(this, &SOpenDriveRoadListTab::OnCrownHeightChanged)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("CrownHalfWidth", "Half Width (m)")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SSpinBox<double>)
									.MinValue(0.1)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.Value(this, &SOpenDriveRoadListTab::GetCrownHalfWidth)
									.OnValueChanged(this, &SOpenDriveRoadListTab::OnCrownHalfWidthChanged)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
								[
									SNew(SButton)
									.Text(LOCTEXT("ApplyCrown", "Apply Crown"))
									.ToolTipText(LOCTEXT("ApplyCrownTip", "Replace the road's lateral profile shape with a symmetric crown of the given height and half width"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.OnClicked(this, &SOpenDriveRoadListTab::OnApplyCrownShapeClicked)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton)
									.Text(LOCTEXT("ClearCrown", "Clear"))
									.ToolTipText(LOCTEXT("ClearCrownTip", "Remove the road's lateral profile shape"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.OnClicked(this, &SOpenDriveRoadListTab::OnClearCrownShapeClicked)
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 4.f, 4.f)
					[
						SNew(SExpandableArea)
						.AreaTitle(LOCTEXT("RoadMarksSection", "Road Marks"))
						.InitiallyCollapsed(true)
						.BodyContent()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Lane", "Lane")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetSelectedLaneText) ]
									.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildLaneMenu)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("MarkType", "Type")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasLaneSelection)
									.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetMarkTypeText) ]
									.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildMarkTypeMenu)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("MarkColor", "Color")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasLaneSelection)
									.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetMarkColorText) ]
									.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildMarkColorMenu)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("MarkBold", "Bold")) ]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SCheckBox)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasLaneSelection)
									.IsChecked(this, &SOpenDriveRoadListTab::GetMarkBoldState)
									.OnCheckStateChanged(this, &SOpenDriveRoadListTab::OnMarkBoldChanged)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("ApplyMark", "Apply Road Mark"))
								.ToolTipText(LOCTEXT("ApplyMarkTip", "Set a single, constant road mark for the selected lane across the whole road"))
								.IsEnabled(this, &SOpenDriveRoadListTab::HasLaneSelection)
								.OnClicked(this, &SOpenDriveRoadListTab::OnApplyRoadMarkClicked)
							]
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
		SelectedLaneId = NoLaneSelected;
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

FText SOpenDriveRoadListTab::GetRoadTypeText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (!Road || Road->Types.Num() == 0)
	{
		return LOCTEXT("RoadTypeDefault", "town (default)");
	}
	return FText::FromString(OpenDriveRoadTypeToString(Road->Types[0].Type));
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildRoadTypeMenu()
{
	static const EOpenDriveRoadType Types[] = {
		EOpenDriveRoadType::Unknown, EOpenDriveRoadType::Rural, EOpenDriveRoadType::Motorway, EOpenDriveRoadType::Town,
		EOpenDriveRoadType::LowSpeed, EOpenDriveRoadType::Pedestrian, EOpenDriveRoadType::Bicycle,
		EOpenDriveRoadType::TownExpressway, EOpenDriveRoadType::TownCollector, EOpenDriveRoadType::TownArterial,
		EOpenDriveRoadType::TownPrivate, EOpenDriveRoadType::TownLocal, EOpenDriveRoadType::TownPlayStreet
	};
	FMenuBuilder Menu(true, nullptr);
	for (const EOpenDriveRoadType Type : Types)
	{
		Menu.AddMenuEntry(FText::FromString(OpenDriveRoadTypeToString(Type)), FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnRoadTypePicked, Type)));
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnRoadTypePicked(EOpenDriveRoadType Type)
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::SetRoadType(Context->GetWorking(), *Context->GetSelectedRoadMutable(), Type);
		Context->NotifyValueChanged();
	}
}

FReply SOpenDriveRoadListTab::OnApplyCrownShapeClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::SetSymmetricCrownShape(Context->GetWorking(), *Context->GetSelectedRoadMutable(), PendingCrownHeight, PendingCrownHalfWidth);
		Context->NotifyValueChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnClearCrownShapeClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		Context->GetSelectedRoadMutable()->Shape.Reset();
		Context->NotifyValueChanged();
	}
	return FReply::Handled();
}

bool SOpenDriveRoadListTab::HasLaneSelection() const
{
	return HasSelection() && SelectedLaneId != NoLaneSelected;
}

FText SOpenDriveRoadListTab::GetSelectedLaneText() const
{
	if (SelectedLaneId == NoLaneSelected)
	{
		return LOCTEXT("NoLane", "(pick a lane)");
	}
	return FText::AsNumber(SelectedLaneId);
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildLaneMenu()
{
	FMenuBuilder Menu(true, nullptr);
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (Road)
	{
		for (const int32 LaneId : FOpenDriveModelEdit::GetLaneIds(*Road))
		{
			Menu.AddMenuEntry(FText::AsNumber(LaneId), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnLanePicked, LaneId)));
		}
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnLanePicked(int32 LaneId)
{
	SelectedLaneId = LaneId;
	if (const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr)
	{
		const FOpenDriveRoadMarkEntry Mark = FOpenDriveModelEdit::GetLaneRoadMark(*Road, LaneId);
		PendingMarkType = Mark.Type;
		PendingMarkColor = Mark.Color;
		bPendingMarkBold = Mark.Weight == EOpenDriveRoadMarkWeight::Bold;
	}
}

FText SOpenDriveRoadListTab::GetMarkTypeText() const
{
	return FText::FromString(OpenDriveRoadMarkTypeToString(PendingMarkType));
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildMarkTypeMenu()
{
	static const EOpenDriveRoadMarkType Types[] = {
		EOpenDriveRoadMarkType::None, EOpenDriveRoadMarkType::Solid, EOpenDriveRoadMarkType::Broken,
		EOpenDriveRoadMarkType::SolidSolid, EOpenDriveRoadMarkType::SolidBroken, EOpenDriveRoadMarkType::BrokenSolid,
		EOpenDriveRoadMarkType::BrokenBroken, EOpenDriveRoadMarkType::BottsDots, EOpenDriveRoadMarkType::Grass,
		EOpenDriveRoadMarkType::Curb, EOpenDriveRoadMarkType::Edge, EOpenDriveRoadMarkType::Custom
	};
	FMenuBuilder Menu(true, nullptr);
	for (const EOpenDriveRoadMarkType Type : Types)
	{
		Menu.AddMenuEntry(FText::FromString(OpenDriveRoadMarkTypeToString(Type)), FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnMarkTypePicked, Type)));
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnMarkTypePicked(EOpenDriveRoadMarkType Type)
{
	PendingMarkType = Type;
}

FText SOpenDriveRoadListTab::GetMarkColorText() const
{
	return FText::FromString(OpenDriveRoadMarkColorToString(PendingMarkColor));
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildMarkColorMenu()
{
	static const EOpenDriveRoadMarkColor Colors[] = {
		EOpenDriveRoadMarkColor::Standard, EOpenDriveRoadMarkColor::Yellow, EOpenDriveRoadMarkColor::Red,
		EOpenDriveRoadMarkColor::Blue, EOpenDriveRoadMarkColor::Green, EOpenDriveRoadMarkColor::Orange, EOpenDriveRoadMarkColor::Violet
	};
	FMenuBuilder Menu(true, nullptr);
	for (const EOpenDriveRoadMarkColor Color : Colors)
	{
		Menu.AddMenuEntry(FText::FromString(OpenDriveRoadMarkColorToString(Color)), FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnMarkColorPicked, Color)));
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnMarkColorPicked(EOpenDriveRoadMarkColor Color)
{
	PendingMarkColor = Color;
}

ECheckBoxState SOpenDriveRoadListTab::GetMarkBoldState() const
{
	return bPendingMarkBold ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SOpenDriveRoadListTab::OnMarkBoldChanged(ECheckBoxState NewState)
{
	bPendingMarkBold = (NewState == ECheckBoxState::Checked);
}

FReply SOpenDriveRoadListTab::OnApplyRoadMarkClicked()
{
	if (Context && HasLaneSelection())
	{
		FOpenDriveRoadMarkEntry Mark;
		Mark.Type = PendingMarkType;
		Mark.Weight = bPendingMarkBold ? EOpenDriveRoadMarkWeight::Bold : EOpenDriveRoadMarkWeight::Standard;
		Mark.Color = PendingMarkColor;
		FOpenDriveModelEdit::SetLaneRoadMarkConstant(*Context->GetSelectedRoadMutable(), SelectedLaneId, Mark);
		Context->NotifyValueChanged();
	}
	return FReply::Handled();
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
