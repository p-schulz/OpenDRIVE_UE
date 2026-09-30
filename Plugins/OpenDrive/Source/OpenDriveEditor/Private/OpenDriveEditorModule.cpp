#include "OpenDriveEditorModule.h"
#include "OpenDriveEditorContext.h"
#include "Authoring/OpenDriveMapVisualizer.h"
#include "Authoring/SOpenDrivePlanViewTab.h"
#include "Authoring/SOpenDriveProfileTab.h"
#include "Authoring/SOpenDriveRoadListTab.h"
#include "Authoring/SOpenDriveSignalsTab.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "OpenDriveEditorModule"

const FName FOpenDriveEditorModule::RoadListTabId(TEXT("OpenDriveRoadList"));
const FName FOpenDriveEditorModule::ElevationTabId(TEXT("OpenDriveElevation"));
const FName FOpenDriveEditorModule::SuperelevationTabId(TEXT("OpenDriveSuperelevation"));
const FName FOpenDriveEditorModule::LaneOffsetTabId(TEXT("OpenDriveLaneOffset"));
const FName FOpenDriveEditorModule::CrossfallTabId(TEXT("OpenDriveCrossfall"));
const FName FOpenDriveEditorModule::SignalsTabId(TEXT("OpenDriveSignals"));
const FName FOpenDriveEditorModule::PlanViewTabId(TEXT("OpenDrivePlanView"));

FOpenDriveEditorModule& FOpenDriveEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FOpenDriveEditorModule>("OpenDriveEditor");
}

void FOpenDriveEditorModule::StartupModule()
{
	Context = MakeShared<FOpenDriveEditorContext>();
	Visualizer = MakeShared<FOpenDriveMapVisualizer>();

	Context->OnVisualizationChanged.AddLambda([]()
	{
		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports(false);
		}
	});

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(RoadListTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnRoadListTab))
		.SetDisplayName(LOCTEXT("RoadListTabTitle", "OpenDRIVE Road List"))
		.SetTooltipText(LOCTEXT("RoadListTabTip", "Browse, select, add, duplicate and remove roads"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ElevationTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnElevationTab))
		.SetDisplayName(LOCTEXT("ElevationTabTitle", "OpenDRIVE Elevation"))
		.SetTooltipText(LOCTEXT("ElevationTabTip", "Edit the elevation profile of the selected road"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SuperelevationTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnSuperelevationTab))
		.SetDisplayName(LOCTEXT("SuperelevationTabTitle", "OpenDRIVE Superelevation"))
		.SetTooltipText(LOCTEXT("SuperelevationTabTip", "Edit the superelevation (banking) profile of the selected road"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LaneOffsetTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnLaneOffsetTab))
		.SetDisplayName(LOCTEXT("LaneOffsetTabTitle", "OpenDRIVE Lane Offset"))
		.SetTooltipText(LOCTEXT("LaneOffsetTabTip", "Edit the lane offset profile of the selected road"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CrossfallTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnCrossfallTab))
		.SetDisplayName(LOCTEXT("CrossfallTabTitle", "OpenDRIVE Crossfall"))
		.SetTooltipText(LOCTEXT("CrossfallTabTip", "Edit the crossfall (drainage banking) profile of the selected road"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SignalsTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnSignalsTab))
		.SetDisplayName(LOCTEXT("SignalsTabTitle", "OpenDRIVE Signals"))
		.SetTooltipText(LOCTEXT("SignalsTabTip", "Add and edit signs and traffic lights on the selected road"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PlanViewTabId, FOnSpawnTab::CreateRaw(this, &FOpenDriveEditorModule::SpawnPlanViewTab))
		.SetDisplayName(LOCTEXT("PlanViewTabTitle", "OpenDRIVE Plan View"))
		.SetTooltipText(LOCTEXT("PlanViewTabTip", "Author the selected road's reference-line geometry (line/arc/spiral segments) and split lane sections"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FOpenDriveEditorModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RoadListTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ElevationTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SuperelevationTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LaneOffsetTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CrossfallTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SignalsTabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PlanViewTabId);
	}
	Visualizer.Reset();
	Context.Reset();
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnRoadListTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("RoadListTabLabel", "OpenDRIVE Road List"))
		[
			SNew(SOpenDriveRoadListTab, *Context)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnElevationTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("ElevationTabLabel", "OpenDRIVE Elevation"))
		[
			SNew(SOpenDriveProfileTab, *Context, EOpenDriveProfileKind::Elevation)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnSuperelevationTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("SuperelevationTabLabel", "OpenDRIVE Superelevation"))
		[
			SNew(SOpenDriveProfileTab, *Context, EOpenDriveProfileKind::Superelevation)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnLaneOffsetTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("LaneOffsetTabLabel", "OpenDRIVE Lane Offset"))
		[
			SNew(SOpenDriveProfileTab, *Context, EOpenDriveProfileKind::LaneOffset)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnCrossfallTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("CrossfallTabLabel", "OpenDRIVE Crossfall"))
		[
			SNew(SOpenDriveProfileTab, *Context, EOpenDriveProfileKind::Crossfall)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnSignalsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("SignalsTabLabel", "OpenDRIVE Signals"))
		[
			SNew(SOpenDriveSignalsTab, *Context)
		];
}

TSharedRef<SDockTab> FOpenDriveEditorModule::SpawnPlanViewTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("PlanViewTabLabel", "OpenDRIVE Plan View"))
		[
			SNew(SOpenDrivePlanViewTab, *Context)
		];
}

void FOpenDriveEditorModule::OpenRoadListTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(RoadListTabId);
}

void FOpenDriveEditorModule::OpenElevationTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(ElevationTabId);
}

void FOpenDriveEditorModule::OpenSuperelevationTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(SuperelevationTabId);
}

void FOpenDriveEditorModule::OpenLaneOffsetTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(LaneOffsetTabId);
}

void FOpenDriveEditorModule::OpenCrossfallTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(CrossfallTabId);
}

void FOpenDriveEditorModule::OpenSignalsTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(SignalsTabId);
}

void FOpenDriveEditorModule::OpenPlanViewTab() const
{
	FGlobalTabmanager::Get()->TryInvokeTab(PlanViewTabId);
}

IMPLEMENT_MODULE(FOpenDriveEditorModule, OpenDriveEditor)

#undef LOCTEXT_NAMESPACE
