#include "OpenDriveModelEdit.h"

FString FOpenDriveModelEdit::MakeUniqueRoadId(const FOpenDriveMap& Map)
{
	int32 MaxId = 0;
	for (const FOpenDriveRoad& Road : Map.GetRoads())
	{
		if (Road.Id.IsNumeric())
		{
			MaxId = FMath::Max(MaxId, FCString::Atoi(*Road.Id));
		}
	}
	return FString::FromInt(MaxId + 1);
}

FString FOpenDriveModelEdit::MakeUniqueJunctionId(const FOpenDriveMap& Map)
{
	int32 MaxId = 0;
	for (const FOpenDriveJunction& Junction : Map.GetJunctions())
	{
		if (Junction.Id.IsNumeric())
		{
			MaxId = FMath::Max(MaxId, FCString::Atoi(*Junction.Id));
		}
	}
	return FString::FromInt(MaxId + 1);
}

FString FOpenDriveModelEdit::AddStraightRoad(FOpenDriveMap& Map, const FString& Name, double StartX, double StartY, double StartHeadingRad, double Length)
{
	FOpenDriveRoad Road;
	Road.Id = MakeUniqueRoadId(Map);
	Road.Name = Name;
	Road.Length = FMath::Max(1.0, Length);

	FOpenDriveGeometry Geo;
	Geo.Type = EOpenDriveGeometryType::Line;
	Geo.S = 0.0;
	Geo.X = StartX;
	Geo.Y = StartY;
	Geo.Hdg = StartHeadingRad;
	Geo.Length = Road.Length;
	Road.Geometry.Add(Geo);

	FOpenDriveLaneSection Section;
	Section.S = 0.0;
	Section.EndS = Road.Length;
	FOpenDriveLane Left;
	Left.Id = 1;
	Left.Type = TEXT("driving");
	Left.Widths.Add(FOpenDriveCubic{ 0.0, 3.5, 0.0, 0.0, 0.0 });
	Section.Lanes.Add(Left);
	FOpenDriveLane Right;
	Right.Id = -1;
	Right.Type = TEXT("driving");
	Right.Widths.Add(FOpenDriveCubic{ 0.0, 3.5, 0.0, 0.0, 0.0 });
	Section.Lanes.Add(Right);
	Road.LaneSections.Add(MoveTemp(Section));

	Map.ComputeRoadBounds(Road);
	const FString NewId = Road.Id;
	Map.GetRoadsMutable().Add(MoveTemp(Road));
	Map.RebuildIndex();
	return NewId;
}

bool FOpenDriveModelEdit::RemoveRoad(FOpenDriveMap& Map, const FString& RoadId)
{
	TArray<FOpenDriveRoad>& Roads = Map.GetRoadsMutable();
	const int32 Index = Roads.IndexOfByPredicate([&](const FOpenDriveRoad& R) { return R.Id == RoadId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Roads.RemoveAt(Index);

	for (FOpenDriveRoad& Road : Roads)
	{
		if (Road.PredecessorId == RoadId) { Road.PredecessorType = EOpenDriveElementType::None; Road.PredecessorId.Reset(); }
		if (Road.SuccessorId == RoadId) { Road.SuccessorType = EOpenDriveElementType::None; Road.SuccessorId.Reset(); }
	}
	for (FOpenDriveJunction& Junction : Map.GetJunctionsMutable())
	{
		Junction.Connections.RemoveAll([&](const FOpenDriveJunctionConnection& C) { return C.IncomingRoad == RoadId || C.ConnectingRoad == RoadId; });
	}

	Map.RebuildIndex();
	return true;
}

FString FOpenDriveModelEdit::DuplicateRoad(FOpenDriveMap& Map, const FString& RoadId, double OffsetX, double OffsetY)
{
	const FOpenDriveRoad* Source = Map.FindRoad(RoadId);
	if (!Source)
	{
		return FString();
	}
	FOpenDriveRoad Copy = *Source;
	Copy.Id = MakeUniqueRoadId(Map);
	Copy.Name = Source->Name + TEXT("_Copy");
	Copy.JunctionId.Reset();
	Copy.PredecessorType = EOpenDriveElementType::None;
	Copy.PredecessorId.Reset();
	Copy.SuccessorType = EOpenDriveElementType::None;
	Copy.SuccessorId.Reset();
	for (FOpenDriveGeometry& Geo : Copy.Geometry)
	{
		Geo.X += OffsetX;
		Geo.Y += OffsetY;
	}
	Map.ComputeRoadBounds(Copy);
	const FString NewId = Copy.Id;
	Map.GetRoadsMutable().Add(MoveTemp(Copy));
	Map.RebuildIndex();
	return NewId;
}

bool FOpenDriveModelEdit::SetStraightRoadBasics(FOpenDriveMap& Map, const FString& RoadId, const FString& NewName, double StartX, double StartY, double StartHeadingRad, double NewLength)
{
	FOpenDriveRoad* Road = Map.FindRoadMutable(RoadId);
	if (!Road || Road->Geometry.Num() != 1 || Road->Geometry[0].Type != EOpenDriveGeometryType::Line)
	{
		return false;
	}
	Road->Name = NewName;
	Road->Length = FMath::Max(1.0, NewLength);
	Road->Geometry[0].X = StartX;
	Road->Geometry[0].Y = StartY;
	Road->Geometry[0].Hdg = StartHeadingRad;
	Road->Geometry[0].Length = Road->Length;
	if (Road->LaneSections.Num() > 0)
	{
		Road->LaneSections.Last().EndS = Road->Length;
	}
	Map.ComputeRoadBounds(*Road);
	return true;
}

bool FOpenDriveModelEdit::RenameRoad(FOpenDriveMap& Map, const FString& RoadId, const FString& NewName)
{
	FOpenDriveRoad* Road = Map.FindRoadMutable(RoadId);
	if (!Road)
	{
		return false;
	}
	Road->Name = NewName;
	return true;
}

void FOpenDriveModelEdit::SetElevationProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile)
{
	Road.Elevation = MoveTemp(NewProfile);
	Map.ComputeRoadBounds(Road);
}

void FOpenDriveModelEdit::SetSuperelevationProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile)
{
	Road.Superelevation = MoveTemp(NewProfile);
	Map.ComputeRoadBounds(Road);
}

void FOpenDriveModelEdit::SetLaneOffsetProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile)
{
	Road.LaneOffset = MoveTemp(NewProfile);
	Map.ComputeRoadBounds(Road);
}

bool FOpenDriveModelEdit::SetLaneWidthConstant(FOpenDriveRoad& Road, int32 LaneId, double Width)
{
	bool bFound = false;
	for (FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		for (FOpenDriveLane& Lane : Section.Lanes)
		{
			if (Lane.Id == LaneId)
			{
				Lane.Widths.Reset();
				Lane.Widths.Add(FOpenDriveCubic{ Section.S, FMath::Max(0.0, Width), 0.0, 0.0, 0.0 });
				bFound = true;
			}
		}
	}
	return bFound;
}

int32 FOpenDriveModelEdit::AddLane(FOpenDriveRoad& Road, bool bLeft, double Width)
{
	int32 NewId = bLeft ? 1 : -1;
	for (const FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		for (const FOpenDriveLane& Lane : Section.Lanes)
		{
			if ((Lane.Id > 0) == bLeft)
			{
				NewId = bLeft ? FMath::Max(NewId, Lane.Id + 1) : FMath::Min(NewId, Lane.Id - 1);
			}
		}
	}
	for (FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		FOpenDriveLane Lane;
		Lane.Id = NewId;
		Lane.Type = TEXT("driving");
		Lane.Widths.Add(FOpenDriveCubic{ Section.S, FMath::Max(0.0, Width), 0.0, 0.0, 0.0 });
		Section.Lanes.Add(Lane);
		Section.Lanes.Sort([](const FOpenDriveLane& A, const FOpenDriveLane& B) { return A.Id < B.Id; });
	}
	return NewId;
}

bool FOpenDriveModelEdit::RemoveLane(FOpenDriveRoad& Road, int32 LaneId)
{
	if (LaneId == 0)
	{
		return false;
	}
	bool bFound = false;
	for (FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		bFound |= (Section.Lanes.RemoveAll([&](const FOpenDriveLane& L) { return L.Id == LaneId; }) > 0);
	}
	return bFound;
}

// ------------------------------------------------------------------------------------------------
// Profile <-> points
// ------------------------------------------------------------------------------------------------

TArray<FVector2D> FOpenDriveModelEdit::ProfileToPoints(const TArray<FOpenDriveCubic>& Profile, double RoadLength)
{
	TArray<FVector2D> Points;
	Points.Reserve(Profile.Num() + 1);
	for (const FOpenDriveCubic& C : Profile)
	{
		Points.Add(FVector2D(C.S, C.A));
	}
	if (Points.Num() == 0)
	{
		Points.Add(FVector2D(0.0, 0.0));
	}
	if (Points.Last().X < RoadLength - 1e-6)
	{
		// Trailing point: the last stored segment is flat (B = C = D = 0) up to the road end.
		Points.Add(FVector2D(RoadLength, Points.Last().Y));
	}
	return Points;
}

TArray<FOpenDriveCubic> FOpenDriveModelEdit::PointsToProfile(TArray<FVector2D> Points)
{
	Points.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
	TArray<FOpenDriveCubic> Profile;
	if (Points.Num() == 0)
	{
		return Profile;
	}
	// The last point only marks where the flat tail begins (EvalPiecewise extrapolates the segment before
	// it), so it gets its own breakpoint only when it is the sole point.
	const int32 Num = (Points.Num() > 1) ? Points.Num() - 1 : 1;
	for (int32 i = 0; i < Num; ++i)
	{
		FOpenDriveCubic C;
		C.S = Points[i].X;
		C.A = Points[i].Y;
		if (i + 1 < Points.Num())
		{
			const double Dx = Points[i + 1].X - Points[i].X;
			C.B = Dx > 1e-9 ? (Points[i + 1].Y - Points[i].Y) / Dx : 0.0;
		}
		Profile.Add(C);
	}
	return Profile;
}
