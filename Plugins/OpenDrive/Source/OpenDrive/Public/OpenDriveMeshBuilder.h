#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDrive/OpenDriveMeshTypes.h"

/**
 * Generates road-surface geometry from a parsed OpenDRIVE map: one FOpenDriveRoadMesh per road, split into
 * material-slot sections (asphalt, road markings, curbs, sidewalks, grass median). Engine-independent --
 * plain vertex/index arrays, no UProceduralMeshComponent/UStaticMesh dependency -- so it's usable both by
 * a live preview component and by a static-mesh bake, and is unit-testable without a UE install.
 *
 * Scope (v1, matching the plan's mesh-generation phase): lane surfaces and road marks/curbs only, sampled
 * along a piecewise-linear cross-section grid (no adaptive tessellation). Objects, signals and junction
 * blending are not meshed here -- junction roads render like any other road, so the overlapping fan of
 * connecting-road surfaces at a junction is not trimmed/merged into a single continuous pad. A lane's
 * <height> (inner/outer) is applied as a simple vertical lift rather than one perpendicular to the
 * superelevated road plane -- negligible at realistic bank angles, but not exact.
 */
class OPENDRIVE_API FOpenDriveMeshBuilder
{
public:
	static FOpenDriveRoadMesh BuildRoad(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FOpenDriveMeshBuildParams& Params = FOpenDriveMeshBuildParams());
	static TArray<FOpenDriveRoadMesh> BuildMap(const FOpenDriveMap& Map, const FOpenDriveMeshBuildParams& Params = FOpenDriveMeshBuildParams());

	/** Lane <type> -> material slot. Anything not recognised as "sidewalk" or "median" is treated as a
	 *  paved (asphalt) surface, which covers driving lanes and the various ASAM lane-type variants
	 *  (shoulder, parking, biking, bus, ...) without needing one slot per type. */
	static EOpenDriveMeshMaterialSlot ClassifyLaneType(const FString& LaneType);
};
