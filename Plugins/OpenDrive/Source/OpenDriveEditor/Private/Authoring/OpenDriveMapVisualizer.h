#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class FOpenDriveEditorContext;
class UWorld;

/**
 * Draws the working OpenDRIVE map into the editor world as persistent debug lines and strings, so edits
 * (including unapplied ones) are visible while the OpenDRIVE editor mode is active. Redrawn only when the
 * map, the settings, the selection or the origin change.
 */
class FOpenDriveMapVisualizer
{
public:
	/** Call regularly (e.g. every mode tick); redraws when something changed. */
	void Update(FOpenDriveEditorContext& Context);
	/** Removes the drawing from the editor world. */
	void Clear();
	/** Forces a redraw on the next Update. */
	void Invalidate() { bHasCache = false; }

private:
	struct FLine
	{
		FVector A;
		FVector B;
		FColor Color;
		float Thickness;
	};

	struct FLabel
	{
		FVector Position;
		FString Text;
		FColor Color;
	};

	void Rebuild(FOpenDriveEditorContext& Context);
	void AddArrow(const FVector& From, const FVector& To, const FColor& Color, float Thickness);

	TArray<FLine> Lines;
	TArray<FLabel> Labels;

	uint32 CachedModelRevision = MAX_uint32;
	uint32 CachedSettingsRevision = MAX_uint32;
	FString CachedSelection;
	FTransform CachedOrigin;
	bool bHasCache = false;
	TWeakObjectPtr<UWorld> DrawnWorld;
};
