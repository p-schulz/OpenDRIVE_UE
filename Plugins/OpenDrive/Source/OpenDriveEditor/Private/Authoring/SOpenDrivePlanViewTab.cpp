#include "Authoring/SOpenDrivePlanViewTab.h"
#include "OpenDriveEditorContext.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "OpenDrivePlanViewTab"

void SOpenDrivePlanViewTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	List = SNew(SListView<FRowPtr>)
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SOpenDrivePlanViewTab::OnGenerateRow)
		.SelectionMode(ESelectionMode::None);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock).Text(this, &SOpenDrivePlanViewTab::GetHeaderText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[ List.ToSharedRef() ]
			]
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(STextBlock).Text(LOCTEXT("AppendHeader", "Append Segment"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Type", "Type")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SAssignNew(TypeCombo, SComboButton)
							.IsEnabled(this, &SOpenDrivePlanViewTab::HasRoad)
							.OnGetMenuContent(this, &SOpenDrivePlanViewTab::BuildTypeMenu)
							.ButtonContent()[ SNew(STextBlock).Text(this, &SOpenDrivePlanViewTab::GetTypeLabel) ]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Length", "Length (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDrivePlanViewTab::HasRoad).MinValue(0.1)
							.Value(this, &SOpenDrivePlanViewTab::GetLength).OnValueChanged(this, &SOpenDrivePlanViewTab::SetLength)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(STextBlock).Text_Lambda([this]() { return PendingType == EOpenDriveGeometryType::Spiral ? LOCTEXT("CurvStart", "Curvature Start (1/m)") : LOCTEXT("Curvature", "Curvature (1/m)"); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDrivePlanViewTab::IsCurvatureEnabled).Delta(0.001)
							.Value(this, &SOpenDrivePlanViewTab::GetCurvature).OnValueChanged(this, &SOpenDrivePlanViewTab::SetCurvature)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("CurvEnd", "Curvature End (1/m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDrivePlanViewTab::IsCurvEndEnabled).Delta(0.001)
							.Value(this, &SOpenDrivePlanViewTab::GetCurvEnd).OnValueChanged(this, &SOpenDrivePlanViewTab::SetCurvEnd)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton).Text(LOCTEXT("Append", "Append")).IsEnabled(this, &SOpenDrivePlanViewTab::HasRoad)
							.OnClicked(this, &SOpenDrivePlanViewTab::OnAppendClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(LOCTEXT("RemoveLast", "Remove Last Segment")).IsEnabled(this, &SOpenDrivePlanViewTab::CanRemoveLast)
							.OnClicked(this, &SOpenDrivePlanViewTab::OnRemoveLastClicked)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 16.f, 4.f, 4.f)
					[
						SNew(STextBlock).Text(LOCTEXT("SplitHeader", "Split Lane Section"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("SplitS", "S (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDrivePlanViewTab::HasRoad)
							.Value(this, &SOpenDrivePlanViewTab::GetSplitS).OnValueChanged(this, &SOpenDrivePlanViewTab::SetSplitS)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
						[
							SNew(SButton).Text(LOCTEXT("Split", "Split Here")).IsEnabled(this, &SOpenDrivePlanViewTab::HasRoad)
							.OnClicked(this, &SOpenDrivePlanViewTab::OnSplitClicked)
						]
					]
				]
			]
		]
	];

	SelectionHandle = Context->OnSelectionChanged.AddSP(this, &SOpenDrivePlanViewTab::RefreshFromRoadSelection);
	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDrivePlanViewTab::RebuildList);
	RebuildList();
}

SOpenDrivePlanViewTab::~SOpenDrivePlanViewTab()
{
	if (Context)
	{
		Context->OnSelectionChanged.Remove(SelectionHandle);
		Context->OnStructureChanged.Remove(StructureHandle);
	}
}

void SOpenDrivePlanViewTab::RefreshFromRoadSelection()
{
	PendingSplitS = 0.0;
	RebuildList();
}

void SOpenDrivePlanViewTab::RebuildList()
{
	Rows.Reset();
	if (const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr)
	{
		for (int32 i = 0; i < Road->Geometry.Num(); ++i)
		{
			Rows.Add(MakeShared<int32>(i));
		}
	}
	if (List.IsValid())
	{
		List->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SOpenDrivePlanViewTab::OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FOpenDriveEditorContext* Ctx = Context;
	const int32 Index = Item.IsValid() ? *Item : INDEX_NONE;
	return SNew(STableRow<FRowPtr>, Owner)
		[
			SNew(STextBlock).Text_Lambda([Ctx, Index]()
			{
				const FOpenDriveRoad* Road = Ctx ? Ctx->GetSelectedRoad() : nullptr;
				if (Road && Road->Geometry.IsValidIndex(Index))
				{
					return FText::FromString(FString::Printf(TEXT("[%d] %s"), Index, *FOpenDriveModelEdit::DescribeGeometrySegment(Road->Geometry[Index])));
				}
				return FText::GetEmpty();
			})
		];
}

bool SOpenDrivePlanViewTab::HasRoad() const
{
	return Context && Context->GetSelectedRoad() != nullptr;
}

FText SOpenDrivePlanViewTab::GetHeaderText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (!Road)
	{
		return LOCTEXT("NoRoad", "No road selected. Pick one in the Road List tab.");
	}
	return FText::Format(LOCTEXT("Header", "Plan view for road '{0}' (id {1}), length {2} m, {3} segment(s)"),
		FText::FromString(Road->Name), FText::FromString(Road->Id), FText::AsNumber(Road->Length), FText::AsNumber(Road->Geometry.Num()));
}

TSharedRef<SWidget> SOpenDrivePlanViewTab::BuildTypeMenu()
{
	FMenuBuilder Menu(true, nullptr);
	Menu.AddMenuEntry(LOCTEXT("Line", "Line"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SOpenDrivePlanViewTab::SetType, EOpenDriveGeometryType::Line)));
	Menu.AddMenuEntry(LOCTEXT("Arc", "Arc"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SOpenDrivePlanViewTab::SetType, EOpenDriveGeometryType::Arc)));
	Menu.AddMenuEntry(LOCTEXT("Spiral", "Spiral"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SOpenDrivePlanViewTab::SetType, EOpenDriveGeometryType::Spiral)));
	return Menu.MakeWidget();
}

FText SOpenDrivePlanViewTab::GetTypeLabel() const
{
	switch (PendingType)
	{
	case EOpenDriveGeometryType::Arc: return LOCTEXT("ArcLabel", "Arc");
	case EOpenDriveGeometryType::Spiral: return LOCTEXT("SpiralLabel", "Spiral");
	default: return LOCTEXT("LineLabel", "Line");
	}
}

void SOpenDrivePlanViewTab::SetType(EOpenDriveGeometryType NewType)
{
	PendingType = NewType;
}

FReply SOpenDrivePlanViewTab::OnAppendClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveRoad& Road = *Context->GetSelectedRoadMutable();
		switch (PendingType)
		{
		case EOpenDriveGeometryType::Arc:
			FOpenDriveModelEdit::AppendArcSegment(Context->GetWorking(), Road, PendingLength, PendingCurvature);
			break;
		case EOpenDriveGeometryType::Spiral:
			FOpenDriveModelEdit::AppendSpiralSegment(Context->GetWorking(), Road, PendingLength, PendingCurvature, PendingCurvEnd);
			break;
		default:
			FOpenDriveModelEdit::AppendLineSegment(Context->GetWorking(), Road, PendingLength);
			break;
		}
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

bool SOpenDrivePlanViewTab::CanRemoveLast() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return Road && Road->Geometry.Num() > 1;
}

FReply SOpenDrivePlanViewTab::OnRemoveLastClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::RemoveLastGeometrySegment(Context->GetWorking(), *Context->GetSelectedRoadMutable());
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDrivePlanViewTab::OnSplitClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::InsertLaneSection(Context->GetWorking(), *Context->GetSelectedRoadMutable(), PendingSplitS);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
