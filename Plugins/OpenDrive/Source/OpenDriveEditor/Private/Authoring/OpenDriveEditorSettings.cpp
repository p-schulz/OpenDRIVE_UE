#include "Authoring/OpenDriveEditorSettings.h"

void UOpenDriveEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	++Revision;
	OnChanged.Broadcast();
}
