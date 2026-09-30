#include "Authoring/OpenDriveMeshBaker.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveMeshBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "IAssetTools.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

UStaticMesh* FOpenDriveMeshBaker::BakeRoadToStaticMesh(const FOpenDriveMap& Map, const FOpenDriveRoad& Road, const FString& PackagePath,
	const TArray<UMaterialInterface*>& SlotMaterials, const FOpenDriveMeshBuildParams& Params)
{
	const FOpenDriveRoadMesh RoadMesh = FOpenDriveMeshBuilder::BuildRoad(Map, Road, Params);
	if (RoadMesh.IsEmpty())
	{
		return nullptr;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString PackageName, AssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / FString::Printf(TEXT("SM_Road_%s"), *Road.Id), FString(), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone);

	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> GroupMaterialSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

	static const TCHAR* SlotNames[OpenDriveMeshMaterialSlotCount] = { TEXT("Asphalt"), TEXT("RoadMarking"), TEXT("Curb"), TEXT("Sidewalk"), TEXT("GrassMedian") };

	TArray<FStaticMaterial> StaticMaterials;
	for (int32 Slot = 0; Slot < OpenDriveMeshMaterialSlotCount; ++Slot)
	{
		const FOpenDriveMeshSection& Section = RoadMesh.Sections[Slot];
		const FName SlotName(SlotNames[Slot]);
		const FPolygonGroupID GroupId = MeshDescription.CreatePolygonGroup();
		GroupMaterialSlotNames[GroupId] = SlotName;

		UMaterialInterface* Material = SlotMaterials.IsValidIndex(Slot) ? SlotMaterials[Slot] : nullptr;
		StaticMaterials.Add(FStaticMaterial(Material, SlotName, SlotName));

		if (Section.IsEmpty())
		{
			continue;
		}

		TArray<FVertexID> VertexIds;
		VertexIds.Reserve(Section.Positions.Num());
		for (const FVector& Position : Section.Positions)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = FVector3f(Position);
			VertexIds.Add(VertexId);
		}

		for (int32 i = 0; i + 2 < Section.Indices.Num(); i += 3)
		{
			TArray<FVertexInstanceID> InstanceIds;
			InstanceIds.Reserve(3);
			for (int32 k = 0; k < 3; ++k)
			{
				const int32 VertexIndex = Section.Indices[i + k];
				const FVertexInstanceID InstanceId = MeshDescription.CreateVertexInstance(VertexIds[VertexIndex]);
				InstanceNormals[InstanceId] = FVector3f(Section.Normals[VertexIndex]);
				InstanceUVs.Set(InstanceId, 0, FVector2f(Section.UVs[VertexIndex]));
				InstanceIds.Add(InstanceId);
			}
			MeshDescription.CreatePolygon(GroupId, InstanceIds);
		}
	}

	StaticMesh->SetStaticMaterials(StaticMaterials);

	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	BuildParams.bBuildSimpleCollision = false;
	BuildParams.bFastBuild = true;

	TArray<const FMeshDescription*> MeshDescriptionPtrs;
	MeshDescriptionPtrs.Add(&MeshDescription);
	StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs, BuildParams);

	// Complex-as-simple collision by default: the render geometry doubles as collision, so the baked asset
	// is drivable/walkable without a separately authored simple collision hull.
	if (!StaticMesh->GetBodySetup())
	{
		StaticMesh->CreateBodySetup();
	}
	if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
	{
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		BodySetup->InvalidatePhysicsData();
		BodySetup->CreatePhysicsMeshes();
	}

	StaticMesh->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(StaticMesh);
	return StaticMesh;
}
