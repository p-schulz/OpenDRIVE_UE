#include "OpenDrive/OpenDriveRoadMeshActor.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveMeshBuilder.h"
#include "ProceduralMeshComponent.h"

AOpenDriveRoadMeshActor::AOpenDriveRoadMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	SlotMaterials.SetNum(OpenDriveMeshMaterialSlotCount);
}

void AOpenDriveRoadMeshActor::BuildFromRoad(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FOpenDriveMeshBuildParams& Params)
{
	SourceRoadId = Road.Id;
	if (SlotMaterials.Num() != OpenDriveMeshMaterialSlotCount)
	{
		SlotMaterials.SetNum(OpenDriveMeshMaterialSlotCount);
	}

	const FOpenDriveRoadMesh RoadMesh = FOpenDriveMeshBuilder::BuildRoad(Map, Road, Params);

	for (int32 Slot = 0; Slot < OpenDriveMeshMaterialSlotCount; ++Slot)
	{
		const FOpenDriveMeshSection& Section = RoadMesh.Sections[Slot];
		if (Section.IsEmpty())
		{
			MeshComponent->ClearMeshSection(Slot);
			continue;
		}

		MeshComponent->CreateMeshSection(Slot, Section.Positions, Section.Indices, Section.Normals, Section.UVs,
			TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision=*/false);

		if (SlotMaterials[Slot])
		{
			MeshComponent->SetMaterial(Slot, SlotMaterials[Slot]);
		}
	}
}
