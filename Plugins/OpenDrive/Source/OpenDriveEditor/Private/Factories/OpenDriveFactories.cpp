#include "Factories/OpenDriveFactories.h"
#include "OpenDrive/OpenDriveAsset.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveModelEdit.h"
#include "OpenDriveModule.h"
#include "OpenDriveWriter.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// ------------------------------------------------------------------------------------------------
// Import
// ------------------------------------------------------------------------------------------------

UOpenDriveImportFactory::UOpenDriveImportFactory()
{
	SupportedClass = UOpenDriveAsset::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	bText = false;
	Formats.Add(TEXT("xodr;ASAM OpenDRIVE"));
}

UObject* UOpenDriveImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	FString Xml;
	if (!FFileHelper::LoadFileToString(Xml, *Filename))
	{
		UE_LOG(LogOpenDrive, Error, TEXT("Could not read '%s'."), *Filename);
		return nullptr;
	}
	UOpenDriveAsset* Asset = NewObject<UOpenDriveAsset>(InParent, InClass, InName, Flags);
	if (!Asset->SetSource(Xml, FPaths::ConvertRelativePathToFull(Filename)))
	{
		UE_LOG(LogOpenDrive, Error, TEXT("'%s' is not a valid OpenDRIVE file: %s"), *Filename, *Asset->ParseStatus);
		return nullptr;
	}
	if (Asset->AssetImportData)
	{
		Asset->AssetImportData->Update(Filename);
	}
	return Asset;
}

bool UOpenDriveImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UOpenDriveAsset* Asset = Cast<UOpenDriveAsset>(Obj);
	if (!Asset || !Asset->AssetImportData)
	{
		return false;
	}
	Asset->AssetImportData->ExtractFilenames(OutFilenames);
	return true;
}

void UOpenDriveImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UOpenDriveAsset* Asset = Cast<UOpenDriveAsset>(Obj);
	if (Asset && Asset->AssetImportData && NewReimportPaths.Num() > 0)
	{
		Asset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UOpenDriveImportFactory::Reimport(UObject* Obj)
{
	UOpenDriveAsset* Asset = Cast<UOpenDriveAsset>(Obj);
	if (!Asset || !Asset->AssetImportData)
	{
		return EReimportResult::Failed;
	}
	const FString Path = Asset->AssetImportData->GetFirstFilename();
	FString Xml;
	if (Path.IsEmpty() || !FFileHelper::LoadFileToString(Xml, *Path) || !Asset->SetSource(Xml, FPaths::ConvertRelativePathToFull(Path)))
	{
		return EReimportResult::Failed;
	}
	Asset->AssetImportData->Update(Path);
	Asset->MarkPackageDirty();
	return EReimportResult::Succeeded;
}

// ------------------------------------------------------------------------------------------------
// New
// ------------------------------------------------------------------------------------------------

UOpenDriveNewFactory::UOpenDriveNewFactory()
{
	SupportedClass = UOpenDriveAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UOpenDriveNewFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	FOpenDriveMap Map;
	Map.SetName(InName.ToString());
	FOpenDriveModelEdit::AddStraightRoad(Map, TEXT("Road1"), 0.0, 0.0, 0.0, 100.0);

	UOpenDriveAsset* Asset = NewObject<UOpenDriveAsset>(InParent, InClass, InName, Flags | RF_Transactional);
	Asset->SetSource(FOpenDriveWriter::Write(Map), FString());
	return Asset;
}
