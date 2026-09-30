#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "OpenDriveEdMode.generated.h"

/**
 * "OpenDRIVE" editor mode: import, create and edit road networks, visualise them in the level viewports
 * and open the Road List / Elevation / Superelevation editor tabs.
 */
UCLASS()
class UOpenDriveEdMode : public UEdMode
{
	GENERATED_BODY()

public:
	static const FEditorModeID EM_OpenDriveId;

	UOpenDriveEdMode();

	virtual void Enter() override;
	virtual void Exit() override;
	virtual void CreateToolkit() override;
	virtual void ModeTick(float DeltaTime) override;
};
