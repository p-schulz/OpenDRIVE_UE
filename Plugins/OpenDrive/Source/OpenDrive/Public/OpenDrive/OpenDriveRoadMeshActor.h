#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenDrive/OpenDriveMeshTypes.h"
#include "OpenDriveRoadMeshActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class FOpenDriveMap;
struct FOpenDriveRoad;

/**
 * Live preview of one road's generated mesh (see FOpenDriveMeshBuilder), rendered with a
 * UProceduralMeshComponent -- one mesh section per material slot (Asphalt, RoadMarking, Curb, Sidewalk,
 * GrassMedian). Meant to be spawned/updated by the editor's "Generate Meshes" tool-mode button, one actor
 * per road, and later converted to a UStaticMesh by the "Bake to Static Mesh" action; it is not itself
 * collidable (bCreateCollision is off for the preview section build) since it exists to look right, not to
 * be driven on -- collision belongs to the baked static mesh, which the plan calls for explicitly.
 */
UCLASS(BlueprintType)
class OPENDRIVE_API AOpenDriveRoadMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AOpenDriveRoadMeshActor();

	/** The OpenDRIVE road this actor's mesh was last built from, so the tool mode can find and update the
	 *  matching actor instead of spawning a duplicate when "Generate Meshes" is run again. */
	UPROPERTY(VisibleAnywhere, Category = "OpenDRIVE")
	FString SourceRoadId;

	/** One material per EOpenDriveMeshMaterialSlot, in slot order (Asphalt, RoadMarking, Curb, Sidewalk,
	 *  GrassMedian). A slot with no geometry is left with the component's default material. */
	UPROPERTY(EditAnywhere, Category = "OpenDRIVE")
	TArray<TObjectPtr<UMaterialInterface>> SlotMaterials;

	UPROPERTY(VisibleAnywhere, Category = "OpenDRIVE")
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	/** Rebuilds MeshComponent's sections from Map/Road via FOpenDriveMeshBuilder and applies SlotMaterials.
	 *  Sections with no geometry are cleared rather than left stale from a previous build. */
	void BuildFromRoad(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FOpenDriveMeshBuildParams& Params = FOpenDriveMeshBuildParams());
};
