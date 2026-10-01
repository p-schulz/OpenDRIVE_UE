#include "Authoring/SOpenDriveRoundaboutTab.h"
#include "OpenDriveEditorContext.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OpenDriveRoundaboutTab"

namespace
{
	constexpr double RadToDeg = 180.0 / UE_DOUBLE_PI;
}

void SOpenDriveRoundaboutTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Intro", "Creates a German/European-style roundabout: a one-way circulatory ring (single lane, travelling counter-clockwise) split into equal arcs around one junction. Build your own roads up to the listed leg points, then connect them in the Road List tab's Junction section -- one connection into the leg's entry segment, one out of its exit segment."))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Name", "Name")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SEditableTextBox).Text(this, &SOpenDriveRoundaboutTab::GetName).OnTextChanged(this, &SOpenDriveRoundaboutTab::OnNameChanged)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("CenterX", "Center X (m)")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<double>).Value(this, &SOpenDriveRoundaboutTab::GetCenterX).OnValueChanged(this, &SOpenDriveRoundaboutTab::SetCenterX)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("CenterY", "Center Y (m)")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<double>).Value(this, &SOpenDriveRoundaboutTab::GetCenterY).OnValueChanged(this, &SOpenDriveRoundaboutTab::SetCenterY)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Radius", "Ring Radius (m)")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<double>).MinValue(3.0).Value(this, &SOpenDriveRoundaboutTab::GetRadius).OnValueChanged(this, &SOpenDriveRoundaboutTab::SetRadius)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("LaneWidth", "Circulatory Lane Width (m)")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<double>).MinValue(2.5).Value(this, &SOpenDriveRoundaboutTab::GetLaneWidth).OnValueChanged(this, &SOpenDriveRoundaboutTab::SetLaneWidth)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("NumLegs", "Number of Legs")) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<int32>).MinValue(3).MaxValue(12).Value(this, &SOpenDriveRoundaboutTab::GetNumLegs).OnValueChanged(this, &SOpenDriveRoundaboutTab::SetNumLegs)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SButton).Text(LOCTEXT("Create", "Create Roundabout")).IsEnabled(this, &SOpenDriveRoundaboutTab::HasAsset)
			.OnClicked(this, &SOpenDriveRoundaboutTab::OnCreateClicked)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 12.f, 4.f, 4.f)
		[
			SNew(STextBlock).Text(this, &SOpenDriveRoundaboutTab::GetResultText).AutoWrapText(true)
		]
	];
}

bool SOpenDriveRoundaboutTab::HasAsset() const
{
	return Context && Context->GetAsset() != nullptr;
}

FReply SOpenDriveRoundaboutTab::OnCreateClicked()
{
	if (Context && Context->GetAsset())
	{
		LastLegs.Reset();
		LastJunctionId = FOpenDriveModelEdit::AddRoundabout(Context->GetWorking(), PendingName, PendingCenterX, PendingCenterY, PendingRadius, PendingNumLegs, PendingLaneWidth, LastLegs);
		if (!LastJunctionId.IsEmpty() && LastLegs.Num() > 0)
		{
			Context->SetSelectedRoadId(LastLegs[0].EntryRingSegmentId);
		}
		Context->NotifyStructureChanged();
	}
	return FReply::Handled();
}

FText SOpenDriveRoundaboutTab::GetResultText() const
{
	if (LastJunctionId.IsEmpty())
	{
		return LOCTEXT("NoResult", "Not created yet.");
	}
	FString Text = FString::Printf(TEXT("Created junction '%s' with %d leg(s):"), *LastJunctionId, LastLegs.Num());
	for (int32 i = 0; i < LastLegs.Num(); ++i)
	{
		const FOpenDriveRoundaboutLeg& Leg = LastLegs[i];
		Text += FString::Printf(TEXT("\nLeg %d: (%.1f, %.1f), facing outward %.0f deg -- entry segment '%s', exit segment '%s'"),
			i, Leg.X, Leg.Y, Leg.OutwardHeadingRad * RadToDeg, *Leg.EntryRingSegmentId, *Leg.ExitRingSegmentId);
	}
	return FText::FromString(Text);
}

#undef LOCTEXT_NAMESPACE
