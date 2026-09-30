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
class OPENDRIVEEDITOR_API FOpenDriveEditorContext
{
public:
	FOpenDriveEditorContext();
	~FOpenDriveEditorContext();

	// Non-copyable: MSVC's dllexport forces instantiation of the implicit copy constructor/assignment
	// operator right where the class is defined, which would need the complete type of the
	// forward-declared UOpenDriveEditorSettings (via TStrongObjectPtr). Deleting them avoids that; this
	// type is always used through a reference/pointer to the one instance owned by the editor module.
	FOpenDriveEditorContext(const FOpenDriveEditorContext&) = delete;
	FOpenDriveEditorContext& operator=(const FOpenDriveEditorContext&) = delete;

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

	// --- Mesh generation ---------------------------------------------------------------------
	/** Spawns/updates one AOpenDriveRoadMeshActor per road in the working copy inside World (live preview),
	 *  placed at ResolveOrigin(); destroys actors left over from roads that no longer exist. */
	void GenerateRoadMeshes(class UWorld* World);
	/** Bakes the selected road to a new UStaticMesh asset alongside the current OpenDRIVE asset (see
	 *  FOpenDriveMeshBaker), reusing its live preview actor's per-slot materials if one exists in World.
	 *  Returns nullptr if there is no selected road, no asset, or the road has no geometry. */
	class UStaticMesh* BakeSelectedRoadToStaticMesh(class UWorld* World);

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
