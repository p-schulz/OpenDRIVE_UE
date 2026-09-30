#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE_OneParam(FOnOpenDriveProfilePointsChanged, const TArray<FVector2D>& /*Points, sorted by X*/);

/**
 * Draggable (S, Value) point graph: the actual drawing/interaction surface. Values are connected by
 * straight segments, which round-trips exactly through OpenDRIVE's cubic polynomial profile segments
 * (b = slope between points, c = d = 0). Point 0 is pinned to MinX (road start); the last point is pinned
 * to MaxX (road end); both can still be dragged vertically.
 */
class SOpenDriveProfileCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveProfileCanvas)
		: _MinX(0.0)
		, _MaxX(100.0)
		, _ValueUnit(TEXT("m"))
	{}
		SLATE_ATTRIBUTE(double, MinX)
		SLATE_ATTRIBUTE(double, MaxX)
		SLATE_ARGUMENT(FString, ValueUnit)
		SLATE_EVENT(FOnOpenDriveProfilePointsChanged, OnPointsChanged)
		SLATE_EVENT(FSimpleDelegate, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Replaces the displayed points. Does not fire OnPointsChanged. Sorted by X and clamped to [MinX, MaxX]. */
	void SetPoints(TArray<FVector2D> InPoints);
	const TArray<FVector2D>& GetPoints() const { return Points; }

	int32 GetSelectedIndex() const { return SelectedIndex; }
	bool GetSelectedPoint(FVector2D& OutPoint) const;
	/** Moves the selected point's value (and X, if it is not the first or last point), then fires OnPointsChanged. */
	void SetSelectedPoint(double NewX, double NewValue);
	void AddPointAtCenter();
	bool RemoveSelectedPoint();

	//~ SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(400.f, 220.f); }

private:
	static constexpr float Margin = 36.f;

	FVector2D DataToLocal(const FGeometry& Geo, const FVector2D& Data) const;
	FVector2D LocalToData(const FGeometry& Geo, const FVector2D& Local) const;
	int32 FindPointNear(const FGeometry& Geo, const FVector2D& LocalPos, float PixelRadius = 8.f) const;
	void ComputeValueRange(double& OutMin, double& OutMax) const;
	void InsertPoint(const FVector2D& DataPos);
	void NotifyChanged();

	TArray<FVector2D> Points;
	TAttribute<double> MinXAttr;
	TAttribute<double> MaxXAttr;
	FString ValueUnit;
	FOnOpenDriveProfilePointsChanged OnPointsChanged;
	FSimpleDelegate OnSelectionChangedDelegate;

	int32 SelectedIndex = INDEX_NONE;
	int32 DraggedIndex = INDEX_NONE;
};

/** Composite widget: a small toolbar (Add/Remove point, numeric S/Value fields) plus the graph canvas. */
class SOpenDriveProfileGraph : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOpenDriveProfileGraph)
		: _MinX(0.0)
		, _MaxX(100.0)
		, _ValueUnit(TEXT("m"))
	{}
		SLATE_ATTRIBUTE(double, MinX)
		SLATE_ATTRIBUTE(double, MaxX)
		SLATE_ARGUMENT(FString, ValueUnit)
		SLATE_EVENT(FOnOpenDriveProfilePointsChanged, OnPointsChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetPoints(TArray<FVector2D> InPoints) { Canvas->SetPoints(MoveTemp(InPoints)); }
	const TArray<FVector2D>& GetPoints() const { return Canvas->GetPoints(); }

private:
	TOptional<double> GetSelectedS() const;
	TOptional<double> GetSelectedValue() const;
	void OnSSpinBoxCommitted(double NewS);
	void OnValueSpinBoxCommitted(double NewValue);
	FReply OnAddClicked();
	FReply OnRemoveClicked();
	bool HasSelection() const;

	TSharedPtr<SOpenDriveProfileCanvas> Canvas;
};
