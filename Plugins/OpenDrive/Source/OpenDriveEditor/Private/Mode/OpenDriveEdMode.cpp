#include "Mode/OpenDriveEdMode.h"
#include "Mode/OpenDriveModeToolkit.h"
#include "OpenDriveEditorContext.h"
#include "Authoring/OpenDriveMapVisualizer.h"
#include "OpenDriveEditorModule.h"
#include "Editor.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "OpenDriveEdMode"

const FEditorModeID UOpenDriveEdMode::EM_OpenDriveId = TEXT("EM_OpenDrive");

UOpenDriveEdMode::UOpenDriveEdMode()
{
	Info = FEditorModeInfo(
		UOpenDriveEdMode::EM_OpenDriveId,
		LOCTEXT("ModeName", "OpenDRIVE"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.BspMode"),
		true,
		5010);
}

void UOpenDriveEdMode::Enter()
{
	Super::Enter();
	FOpenDriveEditorModule::Get().GetVisualizer().Invalidate();
}

void UOpenDriveEdMode::Exit()
{
	FOpenDriveEditorModule::Get().GetVisualizer().Clear();
	Super::Exit();
}

void UOpenDriveEdMode::CreateToolkit()
{
	Toolkit = MakeShared<FOpenDriveModeToolkit>();
}

void UOpenDriveEdMode::ModeTick(float DeltaTime)
{
	Super::ModeTick(DeltaTime);
	FOpenDriveEditorModule& Module = FOpenDriveEditorModule::Get();
	Module.GetVisualizer().Update(Module.GetContext());
}

#undef LOCTEXT_NAMESPACE
