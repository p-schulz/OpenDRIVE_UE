#pragma once

#include "CoreMinimal.h"
#include "OpenDriveModelEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FOpenDriveEditorContext;

/**
 * Dockable tab: one-click creation of a German/European-style roundabout -- a one-way circulatory ring
 * (see FOpenDriveModelEdit::AddRoundabout) split into N arc segments around a single junction, placed at a
 * chosen centre/radius. After creation, lists each leg's attachment point (position + outward heading) so
 * the user knows where/how to orient their own roads; actually wiring a road into the ring is left to the
 * existing Road List tab's Junction section (an ordinary junction connection), rather than duplicating
 * that picker UI here.
 */
class SOpenDriveRoundaboutTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveRoundaboutTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext);

private:
	FText GetName() const { return FText::FromString(PendingName); }
	void OnNameChanged(const FText& NewText) { PendingName = NewText.ToString(); }

	double GetCenterX() const { return PendingCenterX; }
	void SetCenterX(double V) { PendingCenterX = V; }
	double GetCenterY() const { return PendingCenterY; }
	void SetCenterY(double V) { PendingCenterY = V; }
	double GetRadius() const { return PendingRadius; }
	void SetRadius(double V) { PendingRadius = FMath::Max(3.0, V); }
	int32 GetNumLegs() const { return PendingNumLegs; }
	void SetNumLegs(int32 V) { PendingNumLegs = FMath::Clamp(V, 3, 12); }
	double GetLaneWidth() const { return PendingLaneWidth; }
	void SetLaneWidth(double V) { PendingLaneWidth = FMath::Max(2.5, V); }

	FReply OnCreateClicked();
	FText GetResultText() const;
	bool HasAsset() const;

	FOpenDriveEditorContext* Context = nullptr;
	FString PendingName = TEXT("Roundabout");
	double PendingCenterX = 0.0;
	double PendingCenterY = 0.0;
	double PendingRadius = 15.0;
	int32 PendingNumLegs = 4;
	double PendingLaneWidth = 5.5;

	FString LastJunctionId;
	TArray<FOpenDriveRoundaboutLeg> LastLegs;
};
