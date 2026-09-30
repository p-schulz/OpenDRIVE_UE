#include "Mode/OpenDriveModeToolkit.h"
#include "Mode/SOpenDriveModePanel.h"
#include "OpenDriveEditorModule.h"

#define LOCTEXT_NAMESPACE "OpenDriveModeToolkit"

#if ODR_UE_AT_LEAST(5, 1)
void FOpenDriveModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
#else
void FOpenDriveModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	FModeToolkit::Init(InitToolkitHost);
#endif
	Panel = SNew(SOpenDriveModePanel, FOpenDriveEditorModule::Get().GetContext());
}

FText FOpenDriveModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "OpenDRIVE");
}

#undef LOCTEXT_NAMESPACE
