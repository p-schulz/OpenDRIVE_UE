#include "Authoring/SOpenDriveProfileTab.h"
#include "OpenDriveEditorContext.h"
#include "Authoring/SOpenDriveProfileGraph.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OpenDriveProfileTab"

namespace
{
	constexpr double RadToDeg = 180.0 / UE_DOUBLE_PI;
	constexpr double DegToRad = UE_DOUBLE_PI / 180.0;

	bool IsAngleKind(EOpenDriveProfileKind Kind)
	{
		return Kind == EOpenDriveProfileKind::Superelevation || Kind == EOpenDriveProfileKind::Crossfall;
	}

	FText KindTitle(EOpenDriveProfileKind Kind)
	{
		switch (Kind)
		{
		case EOpenDriveProfileKind::Superelevation: return NSLOCTEXT("OpenDriveProfileTab", "Superelevation", "Superelevation");
		case EOpenDriveProfileKind::LaneOffset: return NSLOCTEXT("OpenDriveProfileTab", "LaneOffset", "Lane Offset");
		case EOpenDriveProfileKind::Crossfall: return NSLOCTEXT("OpenDriveProfileTab", "Crossfall", "Crossfall");
		case EOpenDriveProfileKind::Elevation:
		default:
			return NSLOCTEXT("OpenDriveProfileTab", "Elevation", "Elevation");
		}
	}

	/** Read-only snapshot of the profile as plain cubics, for populating the graph. */
	TArray<FOpenDriveCubic> GetProfileCubics(EOpenDriveProfileKind Kind, const FOpenDriveRoad& Road)
	{
		switch (Kind)
		{
		case EOpenDriveProfileKind::Superelevation: return Road.Superelevation;
		case EOpenDriveProfileKind::LaneOffset: return Road.LaneOffset;
		case EOpenDriveProfileKind::Crossfall: return FOpenDriveModelEdit::ExtractCrossfallCubics(Road.Crossfall);
		case EOpenDriveProfileKind::Elevation:
		default:
			return Road.Elevation;
		}
	}

	void SetProfileCubics(EOpenDriveProfileKind Kind, FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile)
	{
		switch (Kind)
		{
		case EOpenDriveProfileKind::Superelevation: FOpenDriveModelEdit::SetSuperelevationProfile(Map, Road, MoveTemp(NewProfile)); break;
		case EOpenDriveProfileKind::LaneOffset: FOpenDriveModelEdit::SetLaneOffsetProfile(Map, Road, MoveTemp(NewProfile)); break;
		case EOpenDriveProfileKind::Crossfall: FOpenDriveModelEdit::SetCrossfallProfile(Map, Road, MoveTemp(NewProfile)); break;
		case EOpenDriveProfileKind::Elevation:
		default:
			FOpenDriveModelEdit::SetElevationProfile(Map, Road, MoveTemp(NewProfile));
			break;
		}
	}
}

void SOpenDriveProfileTab::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext, EOpenDriveProfileKind InKind)
{
	Context = &InContext;
	Kind = InKind;
	const bool bAngle = IsAngleKind(Kind);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock).Text(this, &SOpenDriveProfileTab::GetHeaderText)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(Graph, SOpenDriveProfileGraph)
				.MinX(this, &SOpenDriveProfileTab::GetMinX)
				.MaxX(this, &SOpenDriveProfileTab::GetMaxX)
				.ValueUnit(bAngle ? TEXT("°") : TEXT("m"))
				.OnPointsChanged(this, &SOpenDriveProfileTab::OnGraphPointsChanged)
			]
		]
	];

	SelectionHandle = Context->OnSelectionChanged.AddSP(this, &SOpenDriveProfileTab::RefreshFromSelection);
	StructureHandle = Context->OnStructureChanged.AddSP(this, &SOpenDriveProfileTab::RefreshFromSelection);
	RefreshFromSelection();
}

SOpenDriveProfileTab::~SOpenDriveProfileTab()
{
	if (Context)
	{
		Context->OnSelectionChanged.Remove(SelectionHandle);
		Context->OnStructureChanged.Remove(StructureHandle);
	}
}

double SOpenDriveProfileTab::GetMaxX() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	return Road ? FMath::Max(1.0, Road->Length) : 1.0;
}

FText SOpenDriveProfileTab::GetHeaderText() const
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	const FText Title = KindTitle(Kind);
	if (!Road)
	{
		return FText::Format(LOCTEXT("NoRoad", "{0}: no road selected. Pick one in the Road List tab."), Title);
	}
	return FText::Format(LOCTEXT("Header", "{0} of road '{1}' (id {2}, length {3} m)"), Title, FText::FromString(Road->Name), FText::FromString(Road->Id), FText::AsNumber(Road->Length));
}

void SOpenDriveProfileTab::RefreshFromSelection()
{
	const FOpenDriveRoad* Road = Context ? Context->GetSelectedRoad() : nullptr;
	DisplayedRoadId = Road ? Road->Id : FString();
	if (!Road || !Graph.IsValid())
	{
		if (Graph.IsValid())
		{
			Graph->SetPoints({ FVector2D(0.0, 0.0) });
		}
		return;
	}

	const TArray<FOpenDriveCubic> Profile = GetProfileCubics(Kind, *Road);
	TArray<FVector2D> Points = FOpenDriveModelEdit::ProfileToPoints(Profile, Road->Length);
	if (IsAngleKind(Kind))
	{
		for (FVector2D& P : Points)
		{
			P.Y *= RadToDeg;
		}
	}
	Graph->SetPoints(MoveTemp(Points));
}

void SOpenDriveProfileTab::OnGraphPointsChanged(const TArray<FVector2D>& Points)
{
	if (!Context)
	{
		return;
	}
	FOpenDriveRoad* Road = Context->GetSelectedRoadMutable();
	if (!Road || Road->Id != DisplayedRoadId)
	{
		return;
	}

	TArray<FVector2D> Converted = Points;
	if (IsAngleKind(Kind))
	{
		for (FVector2D& P : Converted)
		{
			P.Y *= DegToRad;
		}
	}
	TArray<FOpenDriveCubic> NewProfile = FOpenDriveModelEdit::PointsToProfile(MoveTemp(Converted));
	SetProfileCubics(Kind, Context->GetWorking(), *Road, MoveTemp(NewProfile));
	Context->NotifyValueChanged();
}

#undef LOCTEXT_NAMESPACE
