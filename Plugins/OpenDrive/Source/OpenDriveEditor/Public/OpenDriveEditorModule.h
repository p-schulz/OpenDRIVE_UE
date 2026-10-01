#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FOpenDriveEditorContext;
class FOpenDriveMapVisualizer;
class SDockTab;
class FSpawnTabArgs;

/** Editor module: asset factory/definition, dockable authoring tabs and the OpenDRIVE editor mode. */
class OPENDRIVEEDITOR_API FOpenDriveEditorModule : public IModuleInterface
{
public:
	static const FName RoadListTabId;
	static const FName ElevationTabId;
	static const FName SuperelevationTabId;
	static const FName LaneOffsetTabId;
	static const FName CrossfallTabId;
	static const FName SignalsTabId;
	static const FName PlanViewTabId;
	static const FName ObjectsTabId;
	static const FName JunctionGroupsTabId;
	static const FName RoundaboutTabId;

	static FOpenDriveEditorModule& Get();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** State shared by the editor mode panel, the authoring tabs and the viewport visualisation. */
	FOpenDriveEditorContext& GetContext() const { return *Context; }
	FOpenDriveMapVisualizer& GetVisualizer() const { return *Visualizer; }

	void OpenRoadListTab() const;
	void OpenElevationTab() const;
	void OpenSuperelevationTab() const;
	void OpenLaneOffsetTab() const;
	void OpenCrossfallTab() const;
	void OpenSignalsTab() const;
	void OpenPlanViewTab() const;
	void OpenObjectsTab() const;
	void OpenJunctionGroupsTab() const;
	void OpenRoundaboutTab() const;

private:
	TSharedRef<SDockTab> SpawnRoadListTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnElevationTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSuperelevationTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnLaneOffsetTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnCrossfallTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSignalsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnPlanViewTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnObjectsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnJunctionGroupsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnRoundaboutTab(const FSpawnTabArgs& Args);

	TSharedPtr<FOpenDriveEditorContext> Context;
	TSharedPtr<FOpenDriveMapVisualizer> Visualizer;
};
