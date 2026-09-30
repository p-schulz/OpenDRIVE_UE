#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FOpenDriveEditorContext;
class SOpenDriveProfileGraph;

enum class EOpenDriveProfileKind : uint8
{
	Elevation,
	Superelevation
};

/**
 * Dockable tab: picks up the road selected in the Road List tab and edits its elevation or
 * superelevation profile as a 2D graph (see SOpenDriveProfileGraph).
 */
class SOpenDriveProfileTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveProfileTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext, EOpenDriveProfileKind InKind);
	virtual ~SOpenDriveProfileTab() override;

private:
	double GetMinX() const { return 0.0; }
	double GetMaxX() const;
	FText GetHeaderText() const;
	void RefreshFromSelection();
	void OnGraphPointsChanged(const TArray<FVector2D>& Points);

	FOpenDriveEditorContext* Context = nullptr;
	EOpenDriveProfileKind Kind = EOpenDriveProfileKind::Elevation;
	TSharedPtr<SOpenDriveProfileGraph> Graph;
	FDelegateHandle SelectionHandle;
	FDelegateHandle StructureHandle;
	FString DisplayedRoadId;
};
