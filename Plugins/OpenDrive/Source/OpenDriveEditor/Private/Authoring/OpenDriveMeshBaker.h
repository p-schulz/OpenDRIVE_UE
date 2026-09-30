#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMeshTypes.h"

class FOpenDriveMap;
struct FOpenDriveRoad;
class UStaticMesh;
class UMaterialInterface;

/**
 * Bakes one road's generated mesh (FOpenDriveMeshBuilder) into a persistent UStaticMesh asset: one
 * FMeshDescription section per material slot, built via UStaticMesh::BuildFromMeshDescriptions. Per the
 * plan, this is a mandatory part of mesh generation (not an optional extra) -- the live preview
 * (AOpenDriveRoadMeshActor) is for iterating on the road network, baking is for using the result as a
 * normal static mesh asset elsewhere (placed by hand, referenced by other tools, etc).
 *
 * Scope: one static mesh per road (no cross-road merging), and collision defaults to "use complex as
 * simple" -- the render geometry doubles as collision, so the baked asset is drivable/walkable without the
 * user adding a separate simple collision hull. The created asset is left dirty in its package rather than
 * saved to disk immediately, matching how other UE editor tools that generate content (e.g. Merge Actors)
 * leave the result for the user to review and save.
 */
class FOpenDriveMeshBaker
{
public:
	/** Bakes Road into a new UStaticMesh named "SM_Road_<Id>" under PackagePath (e.g. "/Game/GeneratedMeshes"),
	 *  with SlotMaterials[i] assigned to material slot i (see EOpenDriveMeshMaterialSlot); a null or missing
	 *  entry leaves that slot with no material. Returns nullptr if the road produces no geometry. */
	static UStaticMesh* BakeRoadToStaticMesh(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FString& PackagePath,
		const TArray<UMaterialInterface*>& SlotMaterials, const FOpenDriveMeshBuildParams& Params = FOpenDriveMeshBuildParams());
};
