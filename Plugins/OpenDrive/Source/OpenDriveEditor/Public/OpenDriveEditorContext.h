#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakObjectPtr.h"

class UOpenDriveAsset;
class UOpenDriveEditorSettings;

/**
 * Shared editing state: the active OpenDRIVE asset, an editable working copy of its map, the current
 * road selection and the visualisation settings. Edits happen on the working copy; Apply writes it back
 * to the asset (regenerating its XML, undoable), Revert discards it. Mirrors OpenScenario_UE's
 * FOpenScenarioEditorContext so both plugins' editor modes behave consistently.
 */
class FOpenDriveEditorContext
{
public:
	FOpenDriveEditorContext();
	~FOpenDriveEditorContext();

	// --- Asset & model ---------------------------------------------------------------------
	UOpenDriveAsset* GetAsset() const { return Asset.Get(); }
	void SetAsset(UOpenDriveAsset* NewAsset);

	FOpenDriveMap& GetWorking() { return Working; }
	const FOpenDriveMap& GetWorking() const { return Working; }

	bool IsDirty() const { return bDirty; }
	uint32 GetModelRevision() const { return ModelRevision; }

	/** Structure changed (roads/junctions/lanes added, removed or reshaped). */
	void NotifyStructureChanged();
	/** Only values changed (e.g. a profile curve edit). */
	void NotifyValueChanged();

	/** Writes the working copy to the asset (regenerates its XML and reparses it). */
	bool Apply();
	void Revert();

	// --- Selection -------------------------------------------------------------------------
	const FString& GetSelectedRoadId() const { return SelectedRoadId; }
	void SetSelectedRoadId(const FString& RoadId);
	const FOpenDriveRoad* GetSelectedRoad() const { return Working.FindRoad(SelectedRoadId); }
	FOpenDriveRoad* GetSelectedRoadMutable() { return Working.FindRoadMutable(SelectedRoadId); }

	// --- Import / create / export ------------------------------------------------------------
	UObject* ImportFile(const FString& FilePath);
	UOpenDriveAsset* CreateNewRoadNetwork();
	bool ExportFile();

	// --- Authoring commands (operate on the working copy) -----------------------------------
	FString AddRoad();
	bool RemoveSelectedRoad();
	FString DuplicateSelectedRoad();

	// --- Visualisation ---------------------------------------------------------------------
	UOpenDriveEditorSettings* GetSettings() const { return Settings.Get(); }
	FTransform ResolveOrigin() const;

	// --- Events ------------------------------------------------------------------------------
	FSimpleMulticastDelegate OnAssetChanged;
	FSimpleMulticastDelegate OnStructureChanged;
	FSimpleMulticastDelegate OnValueChanged;
	FSimpleMulticastDelegate OnSelectionChanged;
	/** Anything that affects the viewport drawing. */
	FSimpleMulticastDelegate OnVisualizationChanged;

private:
	void HandleAssetReparsed();
	void ReloadWorking();
	void UnbindAsset();

	TWeakObjectPtr<UOpenDriveAsset> Asset;
	FOpenDriveMap Working;
	FString SelectedRoadId;
	bool bDirty = false;
	bool bApplying = false;
	uint32 ModelRevision = 0;
	FDelegateHandle ReparsedHandle;

	TStrongObjectPtr<UOpenDriveEditorSettings> Settings;
};
