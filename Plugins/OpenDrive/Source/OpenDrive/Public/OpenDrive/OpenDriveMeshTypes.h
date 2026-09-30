#pragma once

#include "CoreMinimal.h"

/**
 * Material slots a generated road mesh is split into (see FOpenDriveMeshBuilder). Kept as a plain fixed
 * set rather than a data-driven material map -- a v1 scope reduction; a project wanting more slots (e.g.
 * separate gutter/shoulder materials) would extend this enum and FOpenDriveMeshBuilder together.
 */
enum class EOpenDriveMeshMaterialSlot : uint8
{
	Asphalt,
	RoadMarking,
	Curb,
	Sidewalk,
	GrassMedian
};

constexpr int32 OpenDriveMeshMaterialSlotCount = 5;

/** One material slot's worth of triangle-list geometry (plain arrays -- no UE mesh-buffer dependency, so
 *  this is usable both by the engine-independent builder/tests and by whatever consumes it: a preview
 *  UProceduralMeshComponent section, or an FMeshDescription build for baking to a UStaticMesh). */
struct OPENDRIVE_API FOpenDriveMeshSection
{
	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Indices;

	bool IsEmpty() const { return Positions.Num() == 0; }

	/** Appends a quad (A, B, C, D wound so consecutive corners share an edge) as two triangles, with a
	 *  single flat normal from the quad's own plane (cross(B-A, D-A)). */
	void AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
		const FVector2D& UvA, const FVector2D& UvB, const FVector2D& UvC, const FVector2D& UvD)
	{
		const int32 Base = Positions.Num();
		Positions.Add(A);
		Positions.Add(B);
		Positions.Add(C);
		Positions.Add(D);
		const FVector Normal = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		UVs.Add(UvA);
		UVs.Add(UvB);
		UVs.Add(UvC);
		UVs.Add(UvD);
		Indices.Add(Base + 0);
		Indices.Add(Base + 1);
		Indices.Add(Base + 2);
		Indices.Add(Base + 0);
		Indices.Add(Base + 2);
		Indices.Add(Base + 3);
	}
};

/** A single road's generated mesh, one section per material slot (see FOpenDriveMeshBuilder). Positions
 *  are in Unreal convention (X forward, Y right, Z up, centimetres), local to the map -- the same
 *  ASAM-to-Unreal conversion FOpenDriveMap-consuming code already uses elsewhere (e.g.
 *  UOpenDriveAsset::GetRoadTransform), so callers combine it with their own "map origin" actor transform
 *  the same way they already do for that. */
struct OPENDRIVE_API FOpenDriveRoadMesh
{
	FString RoadId;
	FOpenDriveMeshSection Sections[OpenDriveMeshMaterialSlotCount];

	bool IsEmpty() const
	{
		for (const FOpenDriveMeshSection& Section : Sections)
		{
			if (!Section.IsEmpty())
			{
				return false;
			}
		}
		return true;
	}
};

/** Tuning knobs for FOpenDriveMeshBuilder. Widths/heights/lifts are metres (ASAM units); the builder
 *  converts to centimetres at the point it writes out positions. */
struct OPENDRIVE_API FOpenDriveMeshBuildParams
{
	/** Maximum spacing between cross-section samples along s; geometry/lane-section/width/roadmark
	 *  breakpoints are always sampled exactly in addition to this. */
	double SampleStep = 5.0;
	/** Default painted-line width when a <roadMark> doesn't specify one (weight "standard"). */
	double RoadMarkWidthStandard = 0.12;
	/** Default painted-line width when a <roadMark> doesn't specify one (weight "bold"). */
	double RoadMarkWidthBold = 0.20;
	/** Vertical lift applied to a painted (non-curb) road mark so it doesn't z-fight with the asphalt below. */
	double RoadMarkZLift = 0.005;
	/** Default width for a <roadMark type="curb"> that doesn't specify one. */
	double CurbWidth = 0.15;
	/** Default height for a <roadMark type="curb"> that doesn't specify one. */
	double CurbHeight = 0.12;
};
