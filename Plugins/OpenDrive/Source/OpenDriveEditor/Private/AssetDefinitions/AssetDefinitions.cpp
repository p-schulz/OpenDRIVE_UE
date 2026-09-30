#include "AssetDefinitions/AssetDefinitions.h"
#include "OpenDrive/OpenDriveAsset.h"

#define LOCTEXT_NAMESPACE "OpenDriveAssetDefinitions"

FText UAssetDefinition_OpenDrive::GetAssetDisplayName() const
{
	return LOCTEXT("OpenDriveName", "OpenDRIVE");
}

FLinearColor UAssetDefinition_OpenDrive::GetAssetColor() const
{
	return FLinearColor(0.20f, 0.70f, 0.35f);
}

TSoftClassPtr<UObject> UAssetDefinition_OpenDrive::GetAssetClass() const
{
	return UOpenDriveAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_OpenDrive::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { FAssetCategoryPath(LOCTEXT("SimulationCategory", "Simulation")) };
	return Categories;
}

#undef LOCTEXT_NAMESPACE
