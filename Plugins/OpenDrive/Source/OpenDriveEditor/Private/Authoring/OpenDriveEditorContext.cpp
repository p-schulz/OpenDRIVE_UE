#include "OpenDriveEditorContext.h"
#include "Authoring/OpenDriveEditorSettings.h"
#include "Authoring/OpenDriveMeshBaker.h"
#include "Factories/OpenDriveFactories.h"
#include "OpenDrive/OpenDriveAsset.h"
#include "OpenDrive/OpenDriveRoadMeshActor.h"
#include "OpenDriveModelEdit.h"
#include "AssetToolsModule.h"
#include "AssetImportTask.h"
#include "DesktopPlatformModule.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Actor.h"
#include "IAssetTools.h"
#include "IDesktopPlatform.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "OpenDriveEditorContext"

FOpenDriveEditorContext::FOpenDriveEditorContext()
{
	Settings.Reset(NewObject<UOpenDriveEditorSettings>(GetTransientPackage(), NAME_None, RF_Transient));
	Settings->OnChanged.AddLambda([this]() { OnVisualizationChanged.Broadcast(); });
}

FOpenDriveEditorContext::~FOpenDriveEditorContext()
{
	UnbindAsset();
}

// ------------------------------------------------------------------------------------------------
// Asset & model
// ------------------------------------------------------------------------------------------------

void FOpenDriveEditorContext::UnbindAsset()
{
	if (UOpenDriveAsset* Old = Asset.Get())
	{
		Old->OnReparsed.Remove(ReparsedHandle);
	}
	ReparsedHandle.Reset();
}

void FOpenDriveEditorContext::SetAsset(UOpenDriveAsset* NewAsset)
{
	if (Asset.Get() == NewAsset)
	{
		return;
	}
	UnbindAsset();
	Asset = NewAsset;
	SelectedRoadId.Reset();
	if (NewAsset)
	{
		ReparsedHandle = NewAsset->OnReparsed.AddRaw(this, &FOpenDriveEditorContext::HandleAssetReparsed);
	}
	ReloadWorking();
	OnAssetChanged.Broadcast();
	OnSelectionChanged.Broadcast();
	OnVisualizationChanged.Broadcast();
}

void FOpenDriveEditorContext::ReloadWorking()
{
	const UOpenDriveAsset* A = Asset.Get();
	const TSharedPtr<const FOpenDriveMap> Map = A ? A->GetMap() : nullptr;
	Working = Map.IsValid() ? *Map : FOpenDriveMap();
	bDirty = false;
	++ModelRevision;
}

void FOpenDriveEditorContext::HandleAssetReparsed()
{
	if (bApplying)
	{
		return;
	}
	// The asset text changed from outside (details panel, undo, reimport): follow it unless the user has
	// unapplied edits, which stay until they Apply or Revert.
	if (!bDirty)
	{
		ReloadWorking();
		SelectedRoadId.Reset();
		OnStructureChanged.Broadcast();
		OnSelectionChanged.Broadcast();
	}
	OnVisualizationChanged.Broadcast();
}

void FOpenDriveEditorContext::NotifyStructureChanged()
{
	bDirty = true;
	++ModelRevision;
	OnStructureChanged.Broadcast();
	OnVisualizationChanged.Broadcast();
}

void FOpenDriveEditorContext::NotifyValueChanged()
{
	bDirty = true;
	++ModelRevision;
	OnValueChanged.Broadcast();
	OnVisualizationChanged.Broadcast();
}

bool FOpenDriveEditorContext::Apply()
{
	UOpenDriveAsset* A = Asset.Get();
	if (!A)
	{
		return false;
	}
	const FScopedTransaction Transaction(LOCTEXT("ApplyEdits", "Apply OpenDRIVE edits"));
	A->Modify();
	bApplying = true;
	A->ApplyMap(Working);
	bApplying = false;
	A->MarkPackageDirty();

	ReloadWorking();
	OnStructureChanged.Broadcast();
	OnVisualizationChanged.Broadcast();
	return true;
}

void FOpenDriveEditorContext::Revert()
{
	ReloadWorking();
	SelectedRoadId.Reset();
	OnStructureChanged.Broadcast();
	OnSelectionChanged.Broadcast();
	OnVisualizationChanged.Broadcast();
}

void FOpenDriveEditorContext::SetSelectedRoadId(const FString& RoadId)
{
	if (SelectedRoadId == RoadId)
	{
		return;
	}
	SelectedRoadId = RoadId;
	OnSelectionChanged.Broadcast();
}

// ------------------------------------------------------------------------------------------------
// Import / create / export
// ------------------------------------------------------------------------------------------------

UObject* FOpenDriveEditorContext::ImportFile(const FString& FilePath)
{
	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->Filename = FilePath;
	Task->DestinationPath = Settings->ImportDestination;
	Task->bAutomated = true;
	Task->bReplaceExisting = true;
	Task->bSave = false;

	FAssetToolsModule::GetModule().Get().ImportAssetTasks({ Task });
	for (UObject* Imported : Task->GetObjects())
	{
		if (Imported)
		{
			if (UOpenDriveAsset* RoadNetwork = Cast<UOpenDriveAsset>(Imported))
			{
				SetAsset(RoadNetwork);
			}
			return Imported;
		}
	}
	return nullptr;
}

UOpenDriveAsset* FOpenDriveEditorContext::CreateNewRoadNetwork()
{
	IAssetTools& Tools = FAssetToolsModule::GetModule().Get();
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(Settings->ImportDestination / TEXT("NewRoadNetwork"), FString(), PackageName, AssetName);

	UObject* Created = Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UOpenDriveAsset::StaticClass(), NewObject<UOpenDriveNewFactory>());
	UOpenDriveAsset* RoadNetwork = Cast<UOpenDriveAsset>(Created);
	if (RoadNetwork)
	{
		SetAsset(RoadNetwork);
	}
	return RoadNetwork;
}

bool FOpenDriveEditorContext::ExportFile()
{
	UOpenDriveAsset* A = Asset.Get();
	IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
	if (!A || !Platform)
	{
		return false;
	}
	TArray<FString> Files;
	const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	if (!Platform->SaveFileDialog(Parent, TEXT("Export OpenDRIVE"), FPaths::ProjectDir(), A->GetName() + TEXT(".xodr"), TEXT("OpenDRIVE (*.xodr)|*.xodr"), EFileDialogFlags::None, Files) || Files.Num() == 0)
	{
		return false;
	}
	if (bDirty)
	{
		A->ApplyMap(Working);
		bDirty = false;
		ModelRevision++;
	}
	return A->ExportToFile(Files[0]);
}

// ------------------------------------------------------------------------------------------------
// Authoring commands
// ------------------------------------------------------------------------------------------------

FString FOpenDriveEditorContext::AddRoad()
{
	const FString NewId = FOpenDriveModelEdit::AddStraightRoad(Working, TEXT("NewRoad"), 0.0, 0.0, 0.0, 50.0);
	NotifyStructureChanged();
	SetSelectedRoadId(NewId);
	return NewId;
}

bool FOpenDriveEditorContext::RemoveSelectedRoad()
{
	if (!FOpenDriveModelEdit::RemoveRoad(Working, SelectedRoadId))
	{
		return false;
	}
	SelectedRoadId.Reset();
	NotifyStructureChanged();
	OnSelectionChanged.Broadcast();
	return true;
}

FString FOpenDriveEditorContext::DuplicateSelectedRoad()
{
	const FString NewId = FOpenDriveModelEdit::DuplicateRoad(Working, SelectedRoadId, 5.0, 5.0);
	if (!NewId.IsEmpty())
	{
		NotifyStructureChanged();
		SetSelectedRoadId(NewId);
	}
	return NewId;
}

// ------------------------------------------------------------------------------------------------
// Visualisation
// ------------------------------------------------------------------------------------------------

FTransform FOpenDriveEditorContext::ResolveOrigin() const
{
	if (const AActor* Explicit = Settings->OriginActor.Get())
	{
		FTransform T = Explicit->GetActorTransform();
		T.SetScale3D(FVector::OneVector);
		return T;
	}
	return FTransform::Identity;
}

// ------------------------------------------------------------------------------------------------
// Mesh generation
// ------------------------------------------------------------------------------------------------

void FOpenDriveEditorContext::GenerateRoadMeshes(UWorld* World)
{
	if (!World)
	{
		return;
	}

	TMap<FString, AOpenDriveRoadMeshActor*> ExistingByRoadId;
	for (TActorIterator<AOpenDriveRoadMeshActor> It(World); It; ++It)
	{
		if (!It->SourceRoadId.IsEmpty())
		{
			ExistingByRoadId.Add(It->SourceRoadId, *It);
		}
	}

	const FTransform Origin = ResolveOrigin();
	TSet<FString> LiveRoadIds;
	for (const FOpenDriveRoad& Road : Working.GetRoads())
	{
		LiveRoadIds.Add(Road.Id);

		AOpenDriveRoadMeshActor* Actor = nullptr;
		if (AOpenDriveRoadMeshActor** Found = ExistingByRoadId.Find(Road.Id))
		{
			Actor = *Found;
		}
		else
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = MakeUniqueObjectName(World->GetCurrentLevel(), AOpenDriveRoadMeshActor::StaticClass(), FName(*FString::Printf(TEXT("OpenDriveRoadMesh_%s"), *Road.Id)));
			Actor = World->SpawnActor<AOpenDriveRoadMeshActor>(SpawnParams);
		}
		if (!Actor)
		{
			continue;
		}
		Actor->SetActorTransform(Origin);
		Actor->BuildFromRoad(Working, Road);
	}

	for (const TPair<FString, AOpenDriveRoadMeshActor*>& Pair : ExistingByRoadId)
	{
		if (!LiveRoadIds.Contains(Pair.Key) && IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
}

UStaticMesh* FOpenDriveEditorContext::BakeSelectedRoadToStaticMesh(UWorld* World)
{
	UOpenDriveAsset* A = Asset.Get();
	const FOpenDriveRoad* Road = GetSelectedRoad();
	if (!A || !Road)
	{
		return nullptr;
	}

	TArray<UMaterialInterface*> SlotMaterials;
	if (World)
	{
		for (TActorIterator<AOpenDriveRoadMeshActor> It(World); It; ++It)
		{
			if (It->SourceRoadId == Road->Id)
			{
				SlotMaterials.Reserve(It->SlotMaterials.Num());
				for (const TObjectPtr<UMaterialInterface>& Mat : It->SlotMaterials)
				{
					SlotMaterials.Add(Mat.Get());
				}
				break;
			}
		}
	}

	const FString PackagePath = FPackageName::GetLongPackagePath(A->GetOutermost()->GetName()) / TEXT("GeneratedMeshes");
	return FOpenDriveMeshBaker::BakeRoadToStaticMesh(Working, *Road, PackagePath, SlotMaterials);
}

#undef LOCTEXT_NAMESPACE
