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
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 4.f, 4.f)
					[
						SNew(SExpandableArea)
						.AreaTitle(LOCTEXT("LinksSection", "Road Links"))
						.InitiallyCollapsed(true)
						.BodyContent()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetLinkSummaryText).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("LinkTarget", "Target Road")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.ButtonContent()[ SNew(STextBlock).Text_Lambda([this]() { return PendingLinkTargetRoad.IsEmpty() ? LOCTEXT("PickRoad", "(pick a road)") : FText::FromString(PendingLinkTargetRoad); }) ]
									.OnGetMenuContent_Lambda([this]() { return BuildRoadPickerMenu([this](FString Id) { PendingLinkTargetRoad = Id; }); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("LinkTargetEnd", "at its End")) ]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SCheckBox)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection)
									.IsChecked_Lambda([this]() { return bPendingLinkTargetEnd ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bPendingLinkTargetEnd = (S == ECheckBoxState::Checked); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("SetPred", "Set as My Predecessor"))
									.ToolTipText(LOCTEXT("SetPredTip", "Connect this road's start to the target road/end, and infer lane links"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection).OnClicked(this, &SOpenDriveRoadListTab::OnSetPredecessorClicked)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("SetSucc", "Set as My Successor"))
									.ToolTipText(LOCTEXT("SetSuccTip", "Connect this road's end to the target road/end, and infer lane links"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection).OnClicked(this, &SOpenDriveRoadListTab::OnSetSuccessorClicked)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("ClearPred", "Clear Predecessor"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection).OnClicked(this, &SOpenDriveRoadListTab::OnClearPredecessorClicked)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton).Text(LOCTEXT("ClearSucc", "Clear Successor"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasSelection).OnClicked(this, &SOpenDriveRoadListTab::OnClearSuccessorClicked)
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 4.f, 4.f)
					[
						SNew(SExpandableArea)
						.AreaTitle(LOCTEXT("JunctionsSection", "Junctions"))
						.InitiallyCollapsed(true)
						.BodyContent()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Junction", "Junction")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.ButtonContent()[ SNew(STextBlock).Text_Lambda([this]() { return SelectedJunctionId.IsEmpty() ? LOCTEXT("PickJunction", "(pick or add a junction)") : FText::FromString(SelectedJunctionId); }) ]
									.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildJunctionMenu)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("AddJunction", "Add")).OnClicked(this, &SOpenDriveRoadListTab::OnAddJunctionClicked)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("RemoveJunction", "Remove"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection).OnClicked(this, &SOpenDriveRoadListTab::OnRemoveJunctionClicked)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 2.f)
							[
								SNew(STextBlock).Text(this, &SOpenDriveRoadListTab::GetJunctionSummaryText).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
							[
								SNew(STextBlock).Text(LOCTEXT("AddConnectionTitle", "Add connection:")).Font(FAppStyle::GetFontStyle("BoldFont"))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Incoming", "Incoming Road")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection)
									.ButtonContent()[ SNew(STextBlock).Text_Lambda([this]() { return PendingIncomingRoad.IsEmpty() ? LOCTEXT("PickRoad", "(pick a road)") : FText::FromString(PendingIncomingRoad); }) ]
									.OnGetMenuContent_Lambda([this]() { return BuildRoadPickerMenu([this](FString Id) { PendingIncomingRoad = Id; }); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("AtEnd", "at its End")) ]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SCheckBox)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection)
									.IsChecked_Lambda([this]() { return bPendingIncomingEnd ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bPendingIncomingEnd = (S == ECheckBoxState::Checked); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Connecting", "Connecting Road")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection)
									.ButtonContent()[ SNew(STextBlock).Text_Lambda([this]() { return PendingConnectingRoad.IsEmpty() ? LOCTEXT("PickRoad", "(pick a road)") : FText::FromString(PendingConnectingRoad); }) ]
									.OnGetMenuContent_Lambda([this]() { return BuildRoadPickerMenu([this](FString Id) { PendingConnectingRoad = Id; }); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("EnteredAtEnd", "entered at its End")) ]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SCheckBox)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection)
									.IsChecked_Lambda([this]() { return bPendingConnectingEnd ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bPendingConnectingEnd = (S == ECheckBoxState::Checked); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SButton).Text(LOCTEXT("AddConnection", "Add Connection"))
								.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection).OnClicked(this, &SOpenDriveRoadListTab::OnAddConnectionClicked)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Connection", "Connection")) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SComboButton)
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection)
									.ButtonContent()[ SNew(STextBlock).Text_Lambda([this]() { return SelectedConnectionId.IsEmpty() ? LOCTEXT("PickConnection", "(pick a connection)") : FText::FromString(SelectedConnectionId); }) ]
									.OnGetMenuContent(this, &SOpenDriveRoadListTab::BuildConnectionMenu)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("RemoveConnection", "Remove"))
									.IsEnabled(this, &SOpenDriveRoadListTab::HasJunctionSelection).OnClicked(this, &SOpenDriveRoadListTab::OnRemoveConnectionClicked)
								]
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

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildRoadPickerMenu(TFunction<void(FString)> OnPicked) const
{
	FMenuBuilder Menu(true, nullptr);
	if (Context)
	{
		for (const FOpenDriveRoad& Road : Context->GetWorking().GetRoads())
		{
			if (Road.Id == Context->GetSelectedRoadId())
			{
				continue;
			}
			const FString Id = Road.Id;
			Menu.AddMenuEntry(FText::FromString(FString::Printf(TEXT("%s: %s"), *Road.Id, Road.Name.IsEmpty() ? TEXT("(unnamed)") : *Road.Name)), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([OnPicked, Id]() { OnPicked(Id); })));
		}
	}
	return Menu.MakeWidget();
}

FText SOpenDriveRoadListTab::GetLinkSummaryText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (!Road)
	{
		return FText::GetEmpty();
	}
	auto Describe = [](EOpenDriveElementType Type, const FString& Id, EOpenDriveContactPoint Contact)
	{
		if (Type == EOpenDriveElementType::None || Id.IsEmpty())
		{
			return FString(TEXT("none"));
		}
		const TCHAR* TypeStr = Type == EOpenDriveElementType::Junction ? TEXT("junction") : TEXT("road");
		const TCHAR* ContactStr = Contact == EOpenDriveContactPoint::End ? TEXT(" (end)") : Contact == EOpenDriveContactPoint::Start ? TEXT(" (start)") : TEXT("");
		return FString::Printf(TEXT("%s %s%s"), TypeStr, *Id, ContactStr);
	};
	return FText::FromString(FString::Printf(TEXT("Predecessor: %s\nSuccessor: %s"),
		*Describe(Road->PredecessorType, Road->PredecessorId, Road->PredecessorContact),
		*Describe(Road->SuccessorType, Road->SuccessorId, Road->SuccessorContact)));
}

FReply SOpenDriveRoadListTab::OnSetPredecessorClicked()
{
	if (Context && HasSelection() && !PendingLinkTargetRoad.IsEmpty())
	{
		FOpenDriveModelEdit::ConnectRoadEnds(Context->GetWorking(), Context->GetSelectedRoadId(), false, PendingLinkTargetRoad, bPendingLinkTargetEnd);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnSetSuccessorClicked()
{
	if (Context && HasSelection() && !PendingLinkTargetRoad.IsEmpty())
	{
		FOpenDriveModelEdit::ConnectRoadEnds(Context->GetWorking(), Context->GetSelectedRoadId(), true, PendingLinkTargetRoad, bPendingLinkTargetEnd);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnClearPredecessorClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::ClearRoadLink(*Context->GetSelectedRoadMutable(), false);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnClearSuccessorClicked()
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveModelEdit::ClearRoadLink(*Context->GetSelectedRoadMutable(), true);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildJunctionMenu()
{
	FMenuBuilder Menu(true, nullptr);
	if (Context)
	{
		for (const FOpenDriveJunction& Junction : Context->GetWorking().GetJunctions())
		{
			const FString Id = Junction.Id;
			Menu.AddMenuEntry(FText::FromString(FString::Printf(TEXT("%s: %s"), *Junction.Id, Junction.Name.IsEmpty() ? TEXT("(unnamed)") : *Junction.Name)), FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnJunctionPicked, Id)));
		}
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnJunctionPicked(FString JunctionId)
{
	SelectedJunctionId = JunctionId;
	SelectedConnectionId.Reset();
}

bool SOpenDriveRoadListTab::HasJunctionSelection() const
{
	return Context && Context->GetWorking().FindJunction(SelectedJunctionId) != nullptr;
}

FReply SOpenDriveRoadListTab::OnAddJunctionClicked()
{
	if (Context)
	{
		SelectedJunctionId = FOpenDriveModelEdit::AddJunction(Context->GetWorking(), TEXT("Junction"));
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnRemoveJunctionClicked()
{
	if (Context && HasJunctionSelection())
	{
		FOpenDriveModelEdit::RemoveJunction(Context->GetWorking(), SelectedJunctionId);
		SelectedJunctionId.Reset();
		SelectedConnectionId.Reset();
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SOpenDriveRoadListTab::BuildConnectionMenu()
{
	FMenuBuilder Menu(true, nullptr);
	if (Context)
	{
		if (const FOpenDriveJunction* Junction = Context->GetWorking().FindJunction(SelectedJunctionId))
		{
			for (const FOpenDriveJunctionConnection& Con : Junction->Connections)
			{
				const FString Id = Con.Id;
				Menu.AddMenuEntry(FText::FromString(FString::Printf(TEXT("%s: %s -> %s"), *Con.Id, *Con.IncomingRoad, *Con.ConnectingRoad)), FText(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &SOpenDriveRoadListTab::OnConnectionPicked, Id)));
			}
		}
	}
	return Menu.MakeWidget();
}

void SOpenDriveRoadListTab::OnConnectionPicked(FString ConnectionId)
{
	SelectedConnectionId = ConnectionId;
}

FReply SOpenDriveRoadListTab::OnAddConnectionClicked()
{
	if (Context && HasJunctionSelection() && !PendingIncomingRoad.IsEmpty() && !PendingConnectingRoad.IsEmpty())
	{
		const EOpenDriveContactPoint Contact = bPendingConnectingEnd ? EOpenDriveContactPoint::End : EOpenDriveContactPoint::Start;
		FOpenDriveModelEdit::AddJunctionConnection(Context->GetWorking(), SelectedJunctionId, PendingIncomingRoad, bPendingIncomingEnd, PendingConnectingRoad, Contact);
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FReply SOpenDriveRoadListTab::OnRemoveConnectionClicked()
{
	if (Context && HasJunctionSelection() && !SelectedConnectionId.IsEmpty())
	{
		FOpenDriveModelEdit::RemoveJunctionConnection(Context->GetWorking(), SelectedJunctionId, SelectedConnectionId);
		SelectedConnectionId.Reset();
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FText SOpenDriveRoadListTab::GetJunctionSummaryText() const
{
	const FOpenDriveJunction* Junction = Context ? Context->GetWorking().FindJunction(SelectedJunctionId) : nullptr;
	if (!Junction)
	{
		return LOCTEXT("NoJunction", "Pick or add a junction to edit its connections.");
	}
	FString Text = FString::Printf(TEXT("%d connection(s):"), Junction->Connections.Num());
	for (const FOpenDriveJunctionConnection& Con : Junction->Connections)
	{
		Text += FString::Printf(TEXT("\n  %s: %s -> %s (%s, %d lane link(s))"), *Con.Id, *Con.IncomingRoad, *Con.ConnectingRoad,
			Con.ContactPoint == EOpenDriveContactPoint::End ? TEXT("end") : TEXT("start"), Con.LaneLinks.Num());
	}
	return FText::FromString(Text);
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
