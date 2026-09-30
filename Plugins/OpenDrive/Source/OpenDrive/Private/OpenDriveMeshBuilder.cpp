#include "OpenDriveMeshBuilder.h"

namespace
{
	FVector ToUnreal(double X, double Y, double Z)
	{
		// Same ASAM (right-handed, Z up, X forward, Y left, metres) -> Unreal (left-handed, Z up, Y right,
		// centimetres) conversion as UOpenDriveAsset::GetRoadTransform and the editor's debug visualizer.
		return FVector(X * 100.0, -Y * 100.0, Z * 100.0);
	}

	double GetLaneHeightAt(const FOpenDriveLane& Lane, double AbsS)
	{
		const FOpenDriveLaneHeightEntry* Best = nullptr;
		for (const FOpenDriveLaneHeightEntry& Entry : Lane.Heights)
		{
			if (Entry.S <= AbsS + 1e-9)
			{
				Best = &Entry;
			}
			else
			{
				break;
			}
		}
		// Simplification: average inner/outer into one uniform lift across the lane's width at this s,
		// rather than sloping the surface between them.
		return Best ? 0.5 * (Best->InnerHeight + Best->OuterHeight) : 0.0;
	}

	/** Builds a quad strip between two cross-sections at S0/S1, each spanning [Center - HalfWidth, Center +
	 *  HalfWidth] in t (metres) with an additional vertical lift (metres). Ordering the two t-edges as
	 *  Center-HalfWidth then Center+HalfWidth at both ends (rather than "inner"/"outer", which swaps
	 *  numeric order between the road's left and right sides) keeps the resulting normal pointing up
	 *  consistently regardless of which side of the road the strip is on. */
	void AddQuadStrip(FOpenDriveMeshSection& Section, const FOpenDriveMap& Map, const FOpenDriveRoad& Road,
		double S0, double S1, double Center0, double HalfWidth0, double ZLift0, double Center1, double HalfWidth1, double ZLift1,
		double V0, double V1)
	{
		auto MakePoint = [&](double S, double T, double ZLift)
		{
			const FOpenDrivePose Pose = Map.EvaluatePose(Road, S, T);
			return ToUnreal(Pose.X, Pose.Y, Pose.Z + ZLift);
		};
		const FVector A = MakePoint(S0, Center0 - HalfWidth0, ZLift0);
		const FVector B = MakePoint(S0, Center0 + HalfWidth0, ZLift0);
		const FVector C = MakePoint(S1, Center1 + HalfWidth1, ZLift1);
		const FVector D = MakePoint(S1, Center1 - HalfWidth1, ZLift1);
		Section.AddQuad(A, B, C, D, FVector2D(0.0, V0), FVector2D(1.0, V0), FVector2D(1.0, V1), FVector2D(0.0, V1));
	}

	/** Every s at which the road's cross-section can change: geometry/lane-section boundaries, lane width
	 *  and road-mark breakpoints, plus a uniform fill at Step so long, unbroken stretches still get enough
	 *  cross-sections to follow curvature/elevation. */
	TArray<double> BuildSampleSValues(const FOpenDriveRoad& Road, double Step)
	{
		TArray<double> Raw;
		Raw.Add(0.0);
		Raw.Add(Road.Length);
		for (const FOpenDriveGeometry& Geo : Road.Geometry)
		{
			if (Geo.S > 1e-6 && Geo.S < Road.Length - 1e-6)
			{
				Raw.Add(Geo.S);
			}
		}
		for (const FOpenDriveLaneSection& Sec : Road.LaneSections)
		{
			if (Sec.S > 1e-6 && Sec.S < Road.Length - 1e-6)
			{
				Raw.Add(Sec.S);
			}
			for (const FOpenDriveLane& Lane : Sec.Lanes)
			{
				for (const FOpenDriveCubic& W : Lane.Widths)
				{
					if (W.S > 1e-6 && W.S < Road.Length - 1e-6)
					{
						Raw.Add(W.S);
					}
				}
				for (const FOpenDriveRoadMarkEntry& Mark : Lane.RoadMarks)
				{
					if (Mark.S > 1e-6 && Mark.S < Road.Length - 1e-6)
					{
						Raw.Add(Mark.S);
					}
				}
			}
		}
		double S = Step;
		while (S < Road.Length - 1e-6)
		{
			Raw.Add(S);
			S += Step;
		}
		Raw.Sort([](double A, double B) { return A < B; });

		TArray<double> Cleaned;
		for (const double V : Raw)
		{
			if (Cleaned.Num() == 0 || V - Cleaned.Last() > 1e-6)
			{
				Cleaned.Add(V);
			}
		}
		return Cleaned;
	}
}

EOpenDriveMeshMaterialSlot FOpenDriveMeshBuilder::ClassifyLaneType(const FString& LaneType)
{
	if (LaneType.Equals(TEXT("sidewalk"), ESearchCase::IgnoreCase))
	{
		return EOpenDriveMeshMaterialSlot::Sidewalk;
	}
	if (LaneType.Equals(TEXT("median"), ESearchCase::IgnoreCase))
	{
		return EOpenDriveMeshMaterialSlot::GrassMedian;
	}
	return EOpenDriveMeshMaterialSlot::Asphalt;
}

FOpenDriveRoadMesh FOpenDriveMeshBuilder::BuildRoad(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FOpenDriveMeshBuildParams& Params)
{
	FOpenDriveRoadMesh Mesh;
	Mesh.RoadId = Road.Id;
	if (Road.Length <= 0.0 || Road.LaneSections.Num() == 0)
	{
		return Mesh;
	}

	const TArray<double> Samples = BuildSampleSValues(Road, FMath::Max(0.1, Params.SampleStep));
	for (int32 i = 0; i + 1 < Samples.Num(); ++i)
	{
		const double S0 = Samples[i];
		const double S1 = Samples[i + 1];
		const double SMid = 0.5 * (S0 + S1);
		const double V0 = S0 / Road.Length;
		const double V1 = S1 / Road.Length;

		const FOpenDriveLaneSection* Sec = Map.FindLaneSection(Road, SMid);
		if (!Sec)
		{
			continue;
		}

		for (const FOpenDriveLane& Lane : Sec->Lanes)
		{
			if (Lane.Id != 0)
			{
				const double W0 = Map.GetLaneWidth(Road, S0, Lane.Id);
				const double W1 = Map.GetLaneWidth(Road, S1, Lane.Id);
				if (W0 > 1e-6 || W1 > 1e-6)
				{
					const EOpenDriveMeshMaterialSlot Slot = ClassifyLaneType(Lane.Type);
					AddQuadStrip(Mesh.Sections[(int32)Slot], Map, Road, S0, S1,
						Map.GetLaneCenterT(Road, S0, Lane.Id), 0.5 * W0, GetLaneHeightAt(Lane, S0),
						Map.GetLaneCenterT(Road, S1, Lane.Id), 0.5 * W1, GetLaneHeightAt(Lane, S1),
						V0, V1);
				}
			}

			const FOpenDriveRoadMarkEntry* Mark = Lane.FindRoadMarkAt(SMid);
			if (Mark && Mark->Type != EOpenDriveRoadMarkType::None)
			{
				const bool bCurb = Mark->Type == EOpenDriveRoadMarkType::Curb;
				const double DefaultWidth = bCurb ? Params.CurbWidth : (Mark->Weight == EOpenDriveRoadMarkWeight::Bold ? Params.RoadMarkWidthBold : Params.RoadMarkWidthStandard);
				const double Width = Mark->Width >= 0.0 ? Mark->Width : DefaultWidth;
				const double ZLift = Mark->Height > 0.0 ? Mark->Height : (bCurb ? Params.CurbHeight : Params.RoadMarkZLift);
				const int32 Sign = Lane.Id >= 0 ? 1 : -1;
				auto OuterT = [&](double S) { return Map.GetLaneCenterT(Road, S, Lane.Id) + Sign * 0.5 * Map.GetLaneWidth(Road, S, Lane.Id); };
				const EOpenDriveMeshMaterialSlot Slot = bCurb ? EOpenDriveMeshMaterialSlot::Curb : EOpenDriveMeshMaterialSlot::RoadMarking;
				AddQuadStrip(Mesh.Sections[(int32)Slot], Map, Road, S0, S1,
					OuterT(S0), 0.5 * Width, ZLift, OuterT(S1), 0.5 * Width, ZLift, V0, V1);
			}
		}
	}

	return Mesh;
}

TArray<FOpenDriveRoadMesh> FOpenDriveMeshBuilder::BuildMap(const FOpenDriveMap& Map, const FOpenDriveMeshBuildParams& Params)
{
	TArray<FOpenDriveRoadMesh> Result;
	for (const FOpenDriveRoad& Road : Map.GetRoads())
	{
		FOpenDriveRoadMesh RoadMesh = BuildRoad(Map, Road, Params);
		if (!RoadMesh.IsEmpty())
		{
			Result.Add(MoveTemp(RoadMesh));
		}
	}
	return Result;
}
