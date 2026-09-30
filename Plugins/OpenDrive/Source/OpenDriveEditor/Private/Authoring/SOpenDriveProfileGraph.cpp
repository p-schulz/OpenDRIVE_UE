#include "Authoring/SOpenDriveProfileGraph.h"
#include "Brushes/SlateColorBrush.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OpenDriveProfileGraph"

// ------------------------------------------------------------------------------------------------
// SOpenDriveProfileCanvas
// ------------------------------------------------------------------------------------------------

void SOpenDriveProfileCanvas::Construct(const FArguments& InArgs)
{
	MinXAttr = InArgs._MinX;
	MaxXAttr = InArgs._MaxX;
	ValueUnit = InArgs._ValueUnit;
	OnPointsChanged = InArgs._OnPointsChanged;
	OnSelectionChangedDelegate = InArgs._OnSelectionChanged;
}

void SOpenDriveProfileCanvas::SetPoints(TArray<FVector2D> InPoints)
{
	InPoints.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
	Points = MoveTemp(InPoints);
	SelectedIndex = INDEX_NONE;
	DraggedIndex = INDEX_NONE;
}

bool SOpenDriveProfileCanvas::GetSelectedPoint(FVector2D& OutPoint) const
{
	if (!Points.IsValidIndex(SelectedIndex))
	{
		return false;
	}
	OutPoint = Points[SelectedIndex];
	return true;
}

void SOpenDriveProfileCanvas::SetSelectedPoint(double NewX, double NewValue)
{
	if (!Points.IsValidIndex(SelectedIndex))
	{
		return;
	}
	if (SelectedIndex > 0 && SelectedIndex < Points.Num() - 1)
	{
		const double Lo = Points[SelectedIndex - 1].X + 0.01;
		const double Hi = Points[SelectedIndex + 1].X - 0.01;
		Points[SelectedIndex].X = FMath::Clamp(NewX, FMath::Min(Lo, Hi), FMath::Max(Lo, Hi));
	}
	Points[SelectedIndex].Y = NewValue;
	NotifyChanged();
}

void SOpenDriveProfileCanvas::AddPointAtCenter()
{
	const double MinX = MinXAttr.Get();
	const double MaxX = MaxXAttr.Get();
	const double MidX = 0.5 * (MinX + MaxX);
	double Value = 0.0;
	if (Points.Num() > 0)
	{
		Value = Points.Last().Y;
		for (int32 i = 0; i + 1 < Points.Num(); ++i)
		{
			if (MidX >= Points[i].X && MidX <= Points[i + 1].X)
			{
				const double Span = Points[i + 1].X - Points[i].X;
				const double T = Span > 1e-9 ? (MidX - Points[i].X) / Span : 0.0;
				Value = FMath::Lerp(Points[i].Y, Points[i + 1].Y, T);
				break;
			}
		}
	}
	InsertPoint(FVector2D(MidX, Value));
}

bool SOpenDriveProfileCanvas::RemoveSelectedPoint()
{
	if (SelectedIndex <= 0 || !Points.IsValidIndex(SelectedIndex) || Points.Num() <= 1)
	{
		return false;
	}
	Points.RemoveAt(SelectedIndex);
	SelectedIndex = INDEX_NONE;
	OnSelectionChangedDelegate.ExecuteIfBound();
	NotifyChanged();
	return true;
}

void SOpenDriveProfileCanvas::InsertPoint(const FVector2D& DataPos)
{
	const double MinX = MinXAttr.Get();
	const double MaxX = MaxXAttr.Get();
	const double X = FMath::Clamp(DataPos.X, MinX, MaxX);
	const double MinSeparation = FMath::Max(0.02, (MaxX - MinX) * 0.01);
	for (const FVector2D& P : Points)
	{
		if (FMath::Abs(P.X - X) < MinSeparation)
		{
			return;
		}
	}
	int32 InsertAt = Points.Num();
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		if (Points[i].X > X)
		{
			InsertAt = i;
			break;
		}
	}
	Points.Insert(FVector2D(X, DataPos.Y), InsertAt);
	SelectedIndex = InsertAt;
	OnSelectionChangedDelegate.ExecuteIfBound();
	NotifyChanged();
}

void SOpenDriveProfileCanvas::NotifyChanged()
{
	Points.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
	OnPointsChanged.ExecuteIfBound(Points);
}

void SOpenDriveProfileCanvas::ComputeValueRange(double& OutMin, double& OutMax) const
{
	double Lo = 0.0, Hi = 0.0;
	bool bFirst = true;
	for (const FVector2D& P : Points)
	{
		if (bFirst)
		{
			Lo = Hi = P.Y;
			bFirst = false;
		}
		Lo = FMath::Min(Lo, P.Y);
		Hi = FMath::Max(Hi, P.Y);
	}
	Lo = FMath::Min(Lo, 0.0);
	Hi = FMath::Max(Hi, 0.0);
	const double Pad = FMath::Max(0.5, (Hi - Lo) * 0.15);
	OutMin = Lo - Pad;
	OutMax = Hi + Pad;
	if (OutMax - OutMin < 1e-6)
	{
		OutMax = OutMin + 1.0;
	}
}

FVector2D SOpenDriveProfileCanvas::DataToLocal(const FGeometry& Geo, const FVector2D& Data) const
{
	const FVector2D Size = Geo.GetLocalSize();
	const double MinX = MinXAttr.Get();
	const double MaxX = MaxXAttr.Get();
	const double SpanX = FMath::Max(1e-6, MaxX - MinX);
	double ValueMin, ValueMax;
	ComputeValueRange(ValueMin, ValueMax);
	const double SpanY = FMath::Max(1e-6, ValueMax - ValueMin);

	const float PlotLeft = Margin;
	const float PlotRight = FMath::Max(PlotLeft + 1.f, Size.X - 4.f);
	const float PlotTop = 4.f;
	const float PlotBottom = FMath::Max(PlotTop + 1.f, Size.Y - Margin);

	const float X = PlotLeft + static_cast<float>((Data.X - MinX) / SpanX) * (PlotRight - PlotLeft);
	const float Y = PlotBottom - static_cast<float>((Data.Y - ValueMin) / SpanY) * (PlotBottom - PlotTop);
	return FVector2D(X, Y);
}

FVector2D SOpenDriveProfileCanvas::LocalToData(const FGeometry& Geo, const FVector2D& Local) const
{
	const FVector2D Size = Geo.GetLocalSize();
	const double MinX = MinXAttr.Get();
	const double MaxX = MaxXAttr.Get();
	double ValueMin, ValueMax;
	ComputeValueRange(ValueMin, ValueMax);

	const float PlotLeft = Margin;
	const float PlotRight = FMath::Max(PlotLeft + 1.f, Size.X - 4.f);
	const float PlotTop = 4.f;
	const float PlotBottom = FMath::Max(PlotTop + 1.f, Size.Y - Margin);

	const double Tx = (Local.X - PlotLeft) / FMath::Max(1.0, static_cast<double>(PlotRight - PlotLeft));
	const double Ty = (PlotBottom - Local.Y) / FMath::Max(1.0, static_cast<double>(PlotBottom - PlotTop));
	return FVector2D(MinX + Tx * (MaxX - MinX), ValueMin + Ty * (ValueMax - ValueMin));
}

int32 SOpenDriveProfileCanvas::FindPointNear(const FGeometry& Geo, const FVector2D& LocalPos, float PixelRadius) const
{
	int32 Best = INDEX_NONE;
	float BestDistSq = PixelRadius * PixelRadius;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector2D Local = DataToLocal(Geo, Points[i]);
		const float DistSq = static_cast<float>(FVector2D::DistSquared(Local, LocalPos));
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = i;
		}
	}
	return Best;
}

int32 SOpenDriveProfileCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	static const FSlateColorBrush WhiteBrush(FLinearColor::White);
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.03f, 0.03f, 0.03f, 1.f));
	++LayerId;

	double ValueMin, ValueMax;
	ComputeValueRange(ValueMin, ValueMax);

	if (ValueMin < 0.0 && ValueMax > 0.0)
	{
		const float ZeroY = DataToLocal(AllottedGeometry, FVector2D(MinXAttr.Get(), 0.0)).Y;
		TArray<FVector2D> ZeroLine;
		ZeroLine.Add(FVector2D(Margin, ZeroY));
		ZeroLine.Add(FVector2D(LocalSize.X - 4.f, ZeroY));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ZeroLine, ESlateDrawEffect::None, FLinearColor(0.45f, 0.45f, 0.45f, 1.f), true, 1.0f);
	}
	++LayerId;

	{
		TArray<FVector2D> Border;
		Border.Add(FVector2D(Margin, 4.f));
		Border.Add(FVector2D(Margin, LocalSize.Y - Margin));
		Border.Add(FVector2D(LocalSize.X - 4.f, LocalSize.Y - Margin));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Border, ESlateDrawEffect::None, FLinearColor(0.25f, 0.25f, 0.25f, 1.f), true, 1.0f);
	}
	++LayerId;

	if (Points.Num() > 0)
	{
		TArray<FVector2D> Line;
		Line.Reserve(Points.Num());
		for (const FVector2D& P : Points)
		{
			Line.Add(DataToLocal(AllottedGeometry, P));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), true, 2.0f);
	}
	++LayerId;

	const float PointSize = 8.f;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector2D Local = DataToLocal(AllottedGeometry, Points[i]);
		const FLinearColor Color = (i == SelectedIndex) ? FLinearColor(1.0f, 0.7f, 0.1f, 1.0f) : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(PointSize, PointSize), FSlateLayoutTransform(Local - FVector2D(PointSize * 0.5f, PointSize * 0.5f))),
			&WhiteBrush, ESlateDrawEffect::None, Color);
	}
	++LayerId;

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 8);
	const FLinearColor TextColor(0.85f, 0.85f, 0.85f, 1.f);
	auto DrawLabel = [&](const FVector2D& Pos, const FString& Text)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(120.f, 14.f), FSlateLayoutTransform(Pos)), Text, Font, ESlateDrawEffect::None, TextColor);
	};
	DrawLabel(FVector2D(2.f, 2.f), FString::Printf(TEXT("%.2f%s"), ValueMax, *ValueUnit));
	DrawLabel(FVector2D(2.f, LocalSize.Y - Margin - 8.f), FString::Printf(TEXT("%.2f%s"), ValueMin, *ValueUnit));
	DrawLabel(FVector2D(Margin, LocalSize.Y - Margin + 2.f), FString::Printf(TEXT("s=%.1f"), MinXAttr.Get()));
	DrawLabel(FVector2D(FMath::Max(Margin, LocalSize.X - 60.f), LocalSize.Y - Margin + 2.f), FString::Printf(TEXT("s=%.1f"), MaxXAttr.Get()));
	++LayerId;

	return LayerId;
}

FReply SOpenDriveProfileCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const int32 Found = FindPointNear(MyGeometry, Local);
	if (Found != SelectedIndex)
	{
		SelectedIndex = Found;
		OnSelectionChangedDelegate.ExecuteIfBound();
	}
	if (Found != INDEX_NONE)
	{
		DraggedIndex = Found;
		return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
	}
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SOpenDriveProfileCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DraggedIndex != INDEX_NONE)
	{
		DraggedIndex = INDEX_NONE;
		NotifyChanged();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SOpenDriveProfileCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DraggedIndex == INDEX_NONE || !Points.IsValidIndex(DraggedIndex))
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	FVector2D Data = LocalToData(MyGeometry, Local);

	const double MinX = MinXAttr.Get();
	const double MaxX = MaxXAttr.Get();
	if (DraggedIndex == 0)
	{
		Data.X = MinX;
	}
	else if (DraggedIndex == Points.Num() - 1)
	{
		Data.X = MaxX;
	}
	else
	{
		const double Lo = Points[DraggedIndex - 1].X + 0.01;
		const double Hi = Points[DraggedIndex + 1].X - 0.01;
		Data.X = FMath::Clamp(Data.X, FMath::Min(Lo, Hi), FMath::Max(Lo, Hi));
	}
	Points[DraggedIndex] = Data;
	return FReply::Handled();
}

FReply SOpenDriveProfileCanvas::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	InsertPoint(LocalToData(InMyGeometry, Local));
	return FReply::Handled();
}

FReply SOpenDriveProfileCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if ((InKeyEvent.GetKey() == EKeys::Delete || InKeyEvent.GetKey() == EKeys::BackSpace) && RemoveSelectedPoint())
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ------------------------------------------------------------------------------------------------
// SOpenDriveProfileGraph
// ------------------------------------------------------------------------------------------------

void SOpenDriveProfileGraph::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Add", "Add Point"))
				.ToolTipText(LOCTEXT("AddTip", "Add a point midway along the road (or double-click the graph)"))
				.OnClicked(this, &SOpenDriveProfileGraph::OnAddClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Remove", "Remove Point"))
				.ToolTipText(LOCTEXT("RemoveTip", "Remove the selected point (or press Delete)"))
				.OnClicked(this, &SOpenDriveProfileGraph::OnRemoveClicked)
				.IsEnabled(this, &SOpenDriveProfileGraph::HasSelection)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 2.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("S", "s ="))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SSpinBox<double>)
				.MinDesiredWidth(70.f)
				.IsEnabled(this, &SOpenDriveProfileGraph::HasSelection)
				.Value_Lambda([this]() { return GetSelectedS().Get(0.0); })
				.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { OnSSpinBoxCommitted(V); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 2.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("Value", "value ="))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpinBox<double>)
				.MinDesiredWidth(70.f)
				.IsEnabled(this, &SOpenDriveProfileGraph::HasSelection)
				.Value_Lambda([this]() { return GetSelectedValue().Get(0.0); })
				.OnValueCommitted_Lambda([this](double V, ETextCommit::Type) { OnValueSpinBoxCommitted(V); })
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(Canvas, SOpenDriveProfileCanvas)
			.MinX(InArgs._MinX)
			.MaxX(InArgs._MaxX)
			.ValueUnit(InArgs._ValueUnit)
			.OnPointsChanged(InArgs._OnPointsChanged)
		]
	];
}

TOptional<double> SOpenDriveProfileGraph::GetSelectedS() const
{
	FVector2D P;
	if (Canvas.IsValid() && Canvas->GetSelectedPoint(P))
	{
		return P.X;
	}
	return TOptional<double>();
}

TOptional<double> SOpenDriveProfileGraph::GetSelectedValue() const
{
	FVector2D P;
	if (Canvas.IsValid() && Canvas->GetSelectedPoint(P))
	{
		return P.Y;
	}
	return TOptional<double>();
}

void SOpenDriveProfileGraph::OnSSpinBoxCommitted(double NewS)
{
	FVector2D P;
	if (Canvas.IsValid() && Canvas->GetSelectedPoint(P))
	{
		Canvas->SetSelectedPoint(NewS, P.Y);
	}
}

void SOpenDriveProfileGraph::OnValueSpinBoxCommitted(double NewValue)
{
	FVector2D P;
	if (Canvas.IsValid() && Canvas->GetSelectedPoint(P))
	{
		Canvas->SetSelectedPoint(P.X, NewValue);
	}
}

FReply SOpenDriveProfileGraph::OnAddClicked()
{
	if (Canvas.IsValid())
	{
		Canvas->AddPointAtCenter();
	}
	return FReply::Handled();
}

FReply SOpenDriveProfileGraph::OnRemoveClicked()
{
	if (Canvas.IsValid())
	{
		Canvas->RemoveSelectedPoint();
	}
	return FReply::Handled();
}

bool SOpenDriveProfileGraph::HasSelection() const
{
	return Canvas.IsValid() && Canvas->GetSelectedIndex() != INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE
