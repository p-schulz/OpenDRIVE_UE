#include "Authoring/SOpenDriveSignalsTab.h"
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

#define LOCTEXT_NAMESPACE "OpenDriveSignalsTab"

namespace
{
	constexpr double RadToDeg = 180.0 / UE_DOUBLE_PI;
	constexpr double DegToRad = UE_DOUBLE_PI / 180.0;
}

void SOpenDriveSignalsTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	List = SNew(SListView<FRowPtr>)
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SOpenDriveSignalsTab::OnGenerateRow)
		.OnSelectionChanged(this, &SOpenDriveSignalsTab::OnSelectionChanged)
		.SelectionMode(ESelectionMode::Single);

	auto MakeAddButton = [this](const FText& Label, FOpenDriveModelEdit::ESignalPreset Preset)
	{
		return SNew(SButton)
			.Text(Label)
			.IsEnabled(this, &SOpenDriveSignalsTab::HasSelection)
			.OnClicked_Lambda([this, Preset]() { return OnAddClicked(Preset); });
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock).Text(this, &SOpenDriveSignalsTab::GetHeaderText)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)[ MakeAddButton(LOCTEXT("AddStop", "Add Stop Sign"), FOpenDriveModelEdit::ESignalPreset::StopSign) ]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)[ MakeAddButton(LOCTEXT("AddYield", "Add Yield Sign"), FOpenDriveModelEdit::ESignalPreset::YieldSign) ]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)[ MakeAddButton(LOCTEXT("AddSpeed", "Add Speed Limit"), FOpenDriveModelEdit::ESignalPreset::SpeedLimit) ]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)[ MakeAddButton(LOCTEXT("AddLight", "Add Traffic Light"), FOpenDriveModelEdit::ESignalPreset::TrafficLight) ]
			+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
			[
				SNew(SButton).Text(LOCTEXT("Remove", "Remove"))
				.IsEnabled(this, &SOpenDriveSignalsTab::HasSelection).OnClicked(this, &SOpenDriveSignalsTab::OnRemoveClicked)
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
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveSignalsTab::HasSelection)
							.Value(this, &SOpenDriveSignalsTab::GetS).OnValueChanged(this, &SOpenDriveSignalsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("T", "T (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveSignalsTab::HasSelection)
							.Value(this, &SOpenDriveSignalsTab::GetT).OnValueChanged(this, &SOpenDriveSignalsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("ZOffset", "Z Offset (m)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveSignalsTab::HasSelection)
							.Value(this, &SOpenDriveSignalsTab::GetZOffset).OnValueChanged(this, &SOpenDriveSignalsTab::OnPoseChanged)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("HOffset", "Heading Offset (deg)")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpinBox<double>).IsEnabled(this, &SOpenDriveSignalsTab::HasSelection)
							.Value(this, &SOpenDriveSignalsTab::GetHOffsetDeg).OnValueChanged(this, &SOpenDriveSignalsTab::OnPoseChanged)
						]
					]
				]
			]
		]
	];

	SelectionHandle = Context->OnSelectionChanged.AddSP(this, &SOpenDriveSignalsTab::RefreshFromRoadSelection);
	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDriveSignalsTab::RebuildList);
	RebuildList();
}

SOpenDriveSignalsTab::~SOpenDriveSignalsTab()
{
	if (Context)
	{
		Context->OnSelectionChanged.Remove(SelectionHandle);
		Context->OnStructureChanged.Remove(StructureHandle);
	}
}

void SOpenDriveSignalsTab::RefreshFromRoadSelection()
{
	SelectedSignalId.Reset();
	RebuildList();
}

void SOpenDriveSignalsTab::RebuildList()
{
	Rows.Reset();
	if (const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr)
	{
		for (const FOpenDriveSignal& Sig : Road->Signals)
		{
			Rows.Add(MakeShared<FString>(Sig.Id));
		}
	}
	if (List.IsValid())
	{
		List->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SOpenDriveSignalsTab::OnGenerateRow(FRowPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FOpenDriveEditorContext* Ctx = Context;
	const FString Id = Item.IsValid() ? *Item : FString();
	return SNew(STableRow<FRowPtr>, Owner)
		[
			SNew(STextBlock).Text_Lambda([Ctx, Id]()
			{
				const FOpenDriveRoad* Road = Ctx ? Ctx->GetSelectedRoad() : nullptr;
				const FOpenDriveSignal* Sig = Road ? Road->Signals.FindByPredicate([&](const FOpenDriveSignal& S) { return S.Id == Id; }) : nullptr;
				return Sig ? FText::FromString(FString::Printf(TEXT("%s: %s (s=%.1f, t=%.1f)%s"), *Sig->Id, Sig->Name.IsEmpty() ? TEXT("(unnamed)") : *Sig->Name, Sig->S, Sig->T, Sig->bDynamic ? TEXT(" [dynamic]") : TEXT(""))) : FText::FromString(Id);
			})
		];
}

void SOpenDriveSignalsTab::OnSelectionChanged(FRowPtr Item, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingSelection)
	{
		return;
	}
	SelectedSignalId = Item.IsValid() ? *Item : FString();
}

const FOpenDriveSignal* SOpenDriveSignalsTab::GetSelectedSignal() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return (Road && !SelectedSignalId.IsEmpty()) ? Road->Signals.FindByPredicate([&](const FOpenDriveSignal& S) { return S.Id == SelectedSignalId; }) : nullptr;
}

bool SOpenDriveSignalsTab::HasSelection() const
{
	return Context && Context->GetSelectedRoad() != nullptr;
}

FReply SOpenDriveSignalsTab::OnAddClicked(FOpenDriveModelEdit::ESignalPreset Preset)
{
	if (Context && Context->GetSelectedRoadMutable())
	{
		FOpenDriveRoad* Road = Context->GetSelectedRoadMutable();
		const double S = 0.5 * Road->Length;
		const double T = Context->GetWorking().GetOuterBorderT(*Road, S, false) - 1.0;
		SelectedSignalId = FOpenDriveModelEdit::AddSignal(*Road, Preset, S, T);
		Context->NotifyStructureChanged();
		bUpdatingSelection = true;
		for (const FRowPtr& Row : Rows)
		{
			if (Row.IsValid() && *Row == SelectedSignalId)
			{
				List->SetSelection(Row);
				break;
			}
		}
		bUpdatingSelection = false;
	}
	return FReply::Handled();
}

FReply SOpenDriveSignalsTab::OnRemoveClicked()
{
	if (Context && Context->GetSelectedRoadMutable() && !SelectedSignalId.IsEmpty())
	{
		FOpenDriveModelEdit::RemoveSignal(*Context->GetSelectedRoadMutable(), SelectedSignalId);
		SelectedSignalId.Reset();
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

double SOpenDriveSignalsTab::GetS() const
{
	const FOpenDriveSignal* Sig = GetSelectedSignal();
	return Sig ? Sig->S : 0.0;
}

double SOpenDriveSignalsTab::GetT() const
{
	const FOpenDriveSignal* Sig = GetSelectedSignal();
	return Sig ? Sig->T : 0.0;
}

double SOpenDriveSignalsTab::GetZOffset() const
{
	const FOpenDriveSignal* Sig = GetSelectedSignal();
	return Sig ? Sig->ZOffset : 0.0;
}

double SOpenDriveSignalsTab::GetHOffsetDeg() const
{
	const FOpenDriveSignal* Sig = GetSelectedSignal();
	return Sig ? Sig->HOffset * RadToDeg : 0.0;
}

void SOpenDriveSignalsTab::OnPoseChanged(double)
{
	if (!Context || !Context->GetSelectedRoadMutable() || SelectedSignalId.IsEmpty())
	{
		return;
	}
	FOpenDriveModelEdit::SetSignalPose(*Context->GetSelectedRoadMutable(), SelectedSignalId, GetS(), GetT(), GetZOffset(), GetHOffsetDeg() * DegToRad);
	Context->NotifyValueChanged();
}

FText SOpenDriveSignalsTab::GetHeaderText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	if (!Road)
	{
		return LOCTEXT("NoRoad", "No road selected. Pick one in the Road List tab.");
	}
	return FText::Format(LOCTEXT("Header", "Signals on road '{0}' (id {1})"), FText::FromString(Road->Name), FText::FromString(Road->Id));
}

#undef LOCTEXT_NAMESPACE
