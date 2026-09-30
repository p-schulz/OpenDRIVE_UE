#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OpenDriveEditorSettings.generated.h"

class AActor;

/** Options of the OpenDRIVE editor mode: import location and viewport visualisation. */
UCLASS()
class UOpenDriveEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Content folder used when importing or creating road networks from the editor mode. */
	UPROPERTY(EditAnywhere, Category = "Import")
	FString ImportDestination = TEXT("/Game/OpenDrive");

	// --- Visualisation ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawReferenceLines = true;

	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawLaneBorders = true;

	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawLaneCenters = false;

	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawDirectionArrows = true;

	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawRoadLabels = true;

	/** Connecting roads inside junctions get their own colour. */
	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bHighlightJunctionRoads = true;

	/** Highlights the selected road (Road List tab) in the viewport. */
	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bHighlightSelection = true;

	/** Draws a pole + label for each road's signs and traffic lights. */
	UPROPERTY(EditAnywhere, Category = "Visualization")
	bool bDrawSignals = true;

	/** Sampling distance along roads in metres. Larger values draw faster and coarser. */
	UPROPERTY(EditAnywhere, Category = "Visualization", meta = (ClampMin = "0.25", ClampMax = "50.0"))
	float SampleStep = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Visualization", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float LineThickness = 2.0f;

	/** Lifts the lines above the ground (cm). */
	UPROPERTY(EditAnywhere, Category = "Visualization", meta = (ClampMin = "0.0"))
	float ZOffsetCm = 10.0f;

	/** Actor whose transform is the road network origin. If empty, the world origin is used. */
	UPROPERTY(EditAnywhere, Category = "Visualization")
	TSoftObjectPtr<AActor> OriginActor;

	/** Bumped on every change so cached geometry can be rebuilt. */
	uint32 Revision = 0;

	FSimpleMulticastDelegate OnChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};
