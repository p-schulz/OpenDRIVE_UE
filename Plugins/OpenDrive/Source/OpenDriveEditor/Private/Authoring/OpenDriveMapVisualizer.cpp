#include "Authoring/OpenDriveMapVisualizer.h"
#include "OpenDriveEditorContext.h"
#include "Authoring/OpenDriveEditorSettings.h"
#include "OpenDrive/OpenDriveMap.h"
#include "DrawDebugHelpers.h"
#include "Editor.h"
#include "Engine/World.h"

namespace
{
	FVector ToUnreal(double X, double Y, double Z)
	{
		return FVector(X * 100.0, -Y * 100.0, Z * 100.0);
	}

	FColor RoadMarkFColor(EOpenDriveRoadMarkColor Color)
	{
		switch (Color)
		{
		case EOpenDriveRoadMarkColor::Yellow: return FColor(230, 200, 40);
		case EOpenDriveRoadMarkColor::Red: return FColor(220, 60, 60);
		case EOpenDriveRoadMarkColor::Blue: return FColor(60, 120, 220);
		case EOpenDriveRoadMarkColor::Green: return FColor(60, 200, 90);
		case EOpenDriveRoadMarkColor::Orange: return FColor(230, 140, 40);
		case EOpenDriveRoadMarkColor::Violet: return FColor(170, 90, 220);
		case EOpenDriveRoadMarkColor::Standard:
		default:
			return FColor(230, 230, 230);
		}
	}

	/** Approximate dash pattern for "broken" marks: ~3 m painted, ~3 m gap. Every other type (aside from
	 *  None) is drawn as a continuous line -- distinguishing double lines/botts dots/curbs is Phase 7's job. */
	bool ShouldDrawMarkSegment(EOpenDriveRoadMarkType Type, double SegmentMidS)
	{
		if (Type == EOpenDriveRoadMarkType::None)
		{
			return false;
		}
		if (Type == EOpenDriveRoadMarkType::Broken || Type == EOpenDriveRoadMarkType::BrokenBroken)
		{
			return (static_cast<int64>(FMath::FloorToDouble(SegmentMidS / 3.0)) % 2) == 0;
		}
		return true;
	}
}

void FOpenDriveMapVisualizer::AddArrow(const FVector& From, const FVector& To, const FColor& Color, float Thickness)
{
	Lines.Add({ From, To, Color, Thickness });
	const FVector Dir = (To - From).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
	const double Head = (To - From).Size() * 0.3;
	Lines.Add({ To, To - Dir * Head + Side * Head * 0.5, Color, Thickness });
	Lines.Add({ To, To - Dir * Head - Side * Head * 0.5, Color, Thickness });
}

void FOpenDriveMapVisualizer::Rebuild(FOpenDriveEditorContext& Context)
{
	Lines.Reset();
	Labels.Reset();

	const UOpenDriveEditorSettings& Opt = *Context.GetSettings();
	const FOpenDriveMap& Map = Context.GetWorking();
	const FTransform Origin = Context.ResolveOrigin();
	const float Thickness = Opt.LineThickness;
	const FVector Lift(0.0, 0.0, Opt.ZOffsetCm);

	auto ToWorld = [&](const FOpenDrivePose& P, double Extra = 0.0)
	{
		return Origin.TransformPosition(ToUnreal(P.X, P.Y, P.Z) + Lift + FVector(0, 0, Extra));
	};

	const FColor RefColor(255, 210, 0);
	const FColor BorderColor(230, 230, 230);
	const FColor JunctionColor(255, 130, 20);
	const FColor CenterColor(0, 200, 220);
	const FColor ArrowColor(60, 220, 90);
	const FColor SelectedColor(255, 60, 220);

	const double Step = FMath::Max(0.25, static_cast<double>(Opt.SampleStep));
	for (const FOpenDriveRoad& Road : Map.GetRoads())
	{
		const bool bSelected = Opt.bHighlightSelection && Road.Id == Context.GetSelectedRoadId();
		const bool bJunction = Opt.bHighlightJunctionRoads && Road.IsJunctionRoad();
		const FColor RoadRefColor = bSelected ? SelectedColor : (bJunction ? JunctionColor : RefColor);
		const int32 N = FMath::Max(1, FMath::CeilToInt(Road.Length / Step));

		bool bHavePrev = false;
		FVector PrevRef = FVector::ZeroVector;
		double PrevS = 0.0;
		TMap<int32, FVector> PrevBorder;
		TMap<int32, FVector> PrevCenter;

		for (int32 i = 0; i <= N; ++i)
		{
			const double S = FMath::Min(Road.Length, i * Step);
			const double MidS = 0.5 * (PrevS + S);

			const FVector Ref = ToWorld(Map.EvaluatePose(Road, S, Map.GetLaneOffset(Road, S)));
			if (bHavePrev && Opt.bDrawReferenceLines)
			{
				Lines.Add({ PrevRef, Ref, RoadRefColor, bSelected ? Thickness * 1.5f : Thickness });
			}
			PrevRef = Ref;
			bHavePrev = true;

			TMap<int32, FVector> CurBorder;
			TMap<int32, FVector> CurCenter;
			TMap<int32, const FOpenDriveRoadMarkEntry*> CurMarks;
			if (const FOpenDriveLaneSection* Section = Map.FindLaneSection(Road, S))
			{
				for (const FOpenDriveLane& Lane : Section->Lanes)
				{
					const double Center = Map.GetLaneCenterT(Road, S, Lane.Id);
					const double Half = 0.5 * Lane.GetWidth(S);
					const double Outer = Center + (Lane.Id > 0 ? Half : -Half);
					CurBorder.Add(Lane.Id, ToWorld(Map.EvaluatePose(Road, S, Outer)));
					CurCenter.Add(Lane.Id, ToWorld(Map.EvaluatePose(Road, S, Center)));
					CurMarks.Add(Lane.Id, Lane.FindRoadMarkAt(MidS));
				}
			}
			if (Opt.bDrawLaneBorders)
			{
				for (const TPair<int32, FVector>& Pair : CurBorder)
				{
					const FVector* Prev = PrevBorder.Find(Pair.Key);
					if (!Prev)
					{
						continue;
					}
					const auto* MarkPtr = CurMarks.Find(Pair.Key);
					const FOpenDriveRoadMarkEntry* Mark = MarkPtr ? *MarkPtr : nullptr;
					if (Mark && !ShouldDrawMarkSegment(Mark->Type, MidS))
					{
						continue;
					}
					const FColor MarkColor = bSelected ? SelectedColor : (bJunction ? JunctionColor : (Mark ? RoadMarkFColor(Mark->Color) : BorderColor));
					Lines.Add({ *Prev, Pair.Value, MarkColor, Thickness * 0.75f });
				}
			}
			if (Opt.bDrawLaneCenters)
			{
				for (const TPair<int32, FVector>& Pair : CurCenter)
				{
					if (const FVector* Prev = PrevCenter.Find(Pair.Key))
					{
						Lines.Add({ *Prev, Pair.Value, CenterColor, Thickness * 0.5f });
					}
				}
			}
			PrevBorder = MoveTemp(CurBorder);
			PrevCenter = MoveTemp(CurCenter);
			PrevS = S;
		}

		if (Opt.bDrawDirectionArrows)
		{
			const double ArrowSpacing = FMath::Max(10.0, Step * 5.0);
			for (double S = 0.5 * ArrowSpacing; S < Road.Length; S += ArrowSpacing)
			{
				const FOpenDriveLaneSection* Section = Map.FindLaneSection(Road, S);
				if (!Section)
				{
					continue;
				}
				for (const FOpenDriveLane& Lane : Section->Lanes)
				{
					if (!Lane.IsDriving())
					{
						continue;
					}
					const double T = Map.GetLaneCenterT(Road, S, Lane.Id);
					const double Dir = Lane.Id < 0 ? 1.0 : -1.0;
					const double Len = FMath::Min(3.0, 0.5 * Lane.GetWidth(S) + 1.5);
					AddArrow(ToWorld(Map.EvaluatePose(Road, S - Dir * 0.5 * Len, T), 2.0), ToWorld(Map.EvaluatePose(Road, FMath::Clamp(S + Dir * 0.5 * Len, 0.0, Road.Length), T), 2.0), ArrowColor, Thickness);
				}
			}
		}

		if (Opt.bDrawRoadLabels)
		{
			const double S = 0.5 * Road.Length;
			Labels.Add({ ToWorld(Map.EvaluatePose(Road, S, Map.GetLaneOffset(Road, S)), 30.0),
				FString::Printf(TEXT("road %s%s"), *Road.Id, Road.IsJunctionRoad() ? TEXT(" (junction)") : TEXT("")), RoadRefColor });
		}
	}

	CachedModelRevision = Context.GetModelRevision();
	CachedSettingsRevision = Opt.Revision;
	CachedSelection = Context.GetSelectedRoadId();
	CachedOrigin = Origin;
	bHasCache = true;
}

void FOpenDriveMapVisualizer::Clear()
{
	if (UWorld* World = DrawnWorld.Get())
	{
		FlushPersistentDebugLines(World);
		FlushDebugStrings(World);
	}
	DrawnWorld.Reset();
	bHasCache = false;
}

void FOpenDriveMapVisualizer::Update(FOpenDriveEditorContext& Context)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}
	if (!Context.GetAsset())
	{
		if (DrawnWorld.IsValid())
		{
			Clear();
		}
		return;
	}

	const UOpenDriveEditorSettings& Opt = *Context.GetSettings();
	const FTransform Origin = Context.ResolveOrigin();
	const bool bChanged = !bHasCache || DrawnWorld.Get() != World || CachedModelRevision != Context.GetModelRevision()
		|| CachedSettingsRevision != Opt.Revision || CachedSelection != Context.GetSelectedRoadId() || !CachedOrigin.Equals(Origin);
	if (!bChanged)
	{
		return;
	}

	Rebuild(Context);
	FlushPersistentDebugLines(World);
	FlushDebugStrings(World);
	for (const FLine& L : Lines)
	{
		DrawDebugLine(World, L.A, L.B, L.Color, true, -1.f, SDPG_World, L.Thickness);
	}
	int32 Drawn = 0;
	for (const FLabel& L : Labels)
	{
		if (Drawn++ >= 400)
		{
			break;
		}
		DrawDebugString(World, L.Position, L.Text, nullptr, L.Color, -1.f, true);
	}
	DrawnWorld = World;
}
