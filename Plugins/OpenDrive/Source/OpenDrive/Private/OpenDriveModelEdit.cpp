#include "OpenDriveModelEdit.h"

namespace
{
	/**
	 * Matches lanes between two touching lane sections and sets Predecessor/Successor on both sides: a lane
	 * keeps its Id when the far section is entered at its start (straight continuation), or gets the sign-
	 * flipped Id when entered at its end (direction of travel reverses, so left/right swap). Lane 0 (the
	 * centre lane) is never linked. Pairs with no matching lane on the far side are left unlinked.
	 */
	void InferAndApplyLaneLinks(FOpenDriveLaneSection& SecA, bool bAtAEnd, FOpenDriveLaneSection& SecB, bool bAtBEnd)
	{
		const bool bFlip = bAtBEnd;
		for (FOpenDriveLane& LaneA : SecA.Lanes)
		{
			if (LaneA.Id == 0)
			{
				continue;
			}
			const int32 TargetId = bFlip ? -LaneA.Id : LaneA.Id;
			FOpenDriveLane* LaneB = SecB.FindLaneMutable(TargetId);
			if (!LaneB)
			{
				continue;
			}
			if (bAtAEnd) { LaneA.Successor = TargetId; } else { LaneA.Predecessor = TargetId; }
			if (bAtBEnd) { LaneB->Successor = LaneA.Id; } else { LaneB->Predecessor = LaneA.Id; }
		}
	}

	FString MakeUniqueSignalId(const FOpenDriveRoad& Road)
	{
		int32 MaxId = 0;
		for (const FOpenDriveSignal& Sig : Road.Signals)
		{
			if (Sig.Id.IsNumeric())
			{
				MaxId = FMath::Max(MaxId, FCString::Atoi(*Sig.Id));
			}
		}
		return FString::FromInt(MaxId + 1);
	}
}

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

	FOpenDriveLane Center;
	Center.Id = 0;
	Center.Type = TEXT("none");
	Center.RoadMarks.Add(FOpenDriveRoadMarkEntry{ 0.0, EOpenDriveRoadMarkType::Broken, EOpenDriveRoadMarkWeight::Standard, EOpenDriveRoadMarkColor::Standard, -1.0, EOpenDriveLaneChange::Both, 0.0 });
	Section.Lanes.Add(Center);

	FOpenDriveLane Left;
	Left.Id = 1;
	Left.Type = TEXT("driving");
	Left.Widths.Add(FOpenDriveCubic{ 0.0, 3.5, 0.0, 0.0, 0.0 });
	Left.RoadMarks.Add(FOpenDriveRoadMarkEntry{ 0.0, EOpenDriveRoadMarkType::Solid, EOpenDriveRoadMarkWeight::Standard, EOpenDriveRoadMarkColor::Standard, -1.0, EOpenDriveLaneChange::None, 0.0 });
	Section.Lanes.Add(Left);

	FOpenDriveLane Right;
	Right.Id = -1;
	Right.Type = TEXT("driving");
	Right.Widths.Add(FOpenDriveCubic{ 0.0, 3.5, 0.0, 0.0, 0.0 });
	Right.RoadMarks.Add(FOpenDriveRoadMarkEntry{ 0.0, EOpenDriveRoadMarkType::Solid, EOpenDriveRoadMarkWeight::Standard, EOpenDriveRoadMarkColor::Standard, -1.0, EOpenDriveLaneChange::None, 0.0 });
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

void FOpenDriveModelEdit::AppendLineSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length)
{
	double X, Y, H;
	Map.EvaluateReferenceLine(Road, Road.Length, X, Y, H);

	FOpenDriveGeometry Geo;
	Geo.Type = EOpenDriveGeometryType::Line;
	Geo.S = Road.Length;
	Geo.X = X;
	Geo.Y = Y;
	Geo.Hdg = H;
	Geo.Length = FMath::Max(0.1, Length);
	Road.Geometry.Add(Geo);

	Road.Length += Geo.Length;
	if (Road.LaneSections.Num() > 0)
	{
		Road.LaneSections.Last().EndS = Road.Length;
	}
	Map.ComputeRoadBounds(Road);
}

void FOpenDriveModelEdit::AppendArcSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length, double Curvature)
{
	double X, Y, H;
	Map.EvaluateReferenceLine(Road, Road.Length, X, Y, H);

	FOpenDriveGeometry Geo;
	Geo.Type = EOpenDriveGeometryType::Arc;
	Geo.S = Road.Length;
	Geo.X = X;
	Geo.Y = Y;
	Geo.Hdg = H;
	Geo.Length = FMath::Max(0.1, Length);
	Geo.Curvature = Curvature;
	Road.Geometry.Add(Geo);

	Road.Length += Geo.Length;
	if (Road.LaneSections.Num() > 0)
	{
		Road.LaneSections.Last().EndS = Road.Length;
	}
	Map.ComputeRoadBounds(Road);
}

void FOpenDriveModelEdit::AppendSpiralSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length, double CurvStart, double CurvEnd)
{
	double X, Y, H;
	Map.EvaluateReferenceLine(Road, Road.Length, X, Y, H);

	FOpenDriveGeometry Geo;
	Geo.Type = EOpenDriveGeometryType::Spiral;
	Geo.S = Road.Length;
	Geo.X = X;
	Geo.Y = Y;
	Geo.Hdg = H;
	Geo.Length = FMath::Max(0.1, Length);
	Geo.CurvStart = CurvStart;
	Geo.CurvEnd = CurvEnd;
	Road.Geometry.Add(Geo);

	Road.Length += Geo.Length;
	if (Road.LaneSections.Num() > 0)
	{
		Road.LaneSections.Last().EndS = Road.Length;
	}
	Map.ComputeRoadBounds(Road);
}

bool FOpenDriveModelEdit::RemoveLastGeometrySegment(FOpenDriveMap& Map, FOpenDriveRoad& Road)
{
	if (Road.Geometry.Num() <= 1)
	{
		return false;
	}
	const FOpenDriveGeometry Removed = Road.Geometry.Last();
	Road.Geometry.RemoveAt(Road.Geometry.Num() - 1);
	Road.Length = Removed.S;
	if (Road.LaneSections.Num() > 0)
	{
		Road.LaneSections.Last().EndS = Road.Length;
	}
	Map.ComputeRoadBounds(Road);
	return true;
}

FString FOpenDriveModelEdit::DescribeGeometrySegment(const FOpenDriveGeometry& Geo)
{
	switch (Geo.Type)
	{
	case EOpenDriveGeometryType::Line:
		return FString::Printf(TEXT("Line   s=%.1f len=%.1f"), Geo.S, Geo.Length);
	case EOpenDriveGeometryType::Arc:
		return FString::Printf(TEXT("Arc    s=%.1f len=%.1f curv=%.4f"), Geo.S, Geo.Length, Geo.Curvature);
	case EOpenDriveGeometryType::Spiral:
		return FString::Printf(TEXT("Spiral s=%.1f len=%.1f curv=%.4f->%.4f"), Geo.S, Geo.Length, Geo.CurvStart, Geo.CurvEnd);
	case EOpenDriveGeometryType::Poly3:
		return FString::Printf(TEXT("Poly3  s=%.1f len=%.1f"), Geo.S, Geo.Length);
	case EOpenDriveGeometryType::ParamPoly3:
		return FString::Printf(TEXT("ParamPoly3 s=%.1f len=%.1f"), Geo.S, Geo.Length);
	default:
		return FString::Printf(TEXT("? s=%.1f len=%.1f"), Geo.S, Geo.Length);
	}
}

bool FOpenDriveModelEdit::InsertLaneSection(FOpenDriveMap& Map, FOpenDriveRoad& Road, double S)
{
	if (S <= 1e-6 || S >= Road.Length - 1e-6)
	{
		return false;
	}
	int32 Index = INDEX_NONE;
	for (int32 i = 0; i < Road.LaneSections.Num(); ++i)
	{
		if (S > Road.LaneSections[i].S + 1e-6 && S < Road.LaneSections[i].EndS - 1e-6)
		{
			Index = i;
			break;
		}
	}
	if (Index == INDEX_NONE)
	{
		return false;
	}
	FOpenDriveLaneSection NewSection = Road.LaneSections[Index];
	NewSection.S = S;
	Road.LaneSections[Index].EndS = S;
	Road.LaneSections.Insert(MoveTemp(NewSection), Index + 1);
	Map.ComputeRoadBounds(Road);
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

void FOpenDriveModelEdit::SetCrossfallProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile)
{
	Road.Crossfall.Reset();
	Road.Crossfall.Reserve(NewProfile.Num());
	for (FOpenDriveCubic& Cubic : NewProfile)
	{
		FOpenDriveCrossfallEntry Entry;
		Entry.Side = EOpenDriveCrossfallSide::Both;
		Entry.Cubic = MoveTemp(Cubic);
		Road.Crossfall.Add(MoveTemp(Entry));
	}
	Map.ComputeRoadBounds(Road);
}

TArray<FOpenDriveCubic> FOpenDriveModelEdit::ExtractCrossfallCubics(const TArray<FOpenDriveCrossfallEntry>& Crossfall)
{
	TArray<FOpenDriveCubic> Out;
	Out.Reserve(Crossfall.Num());
	for (const FOpenDriveCrossfallEntry& Entry : Crossfall)
	{
		if (Entry.Side == EOpenDriveCrossfallSide::Both)
		{
			Out.Add(Entry.Cubic);
		}
	}
	if (Out.Num() == 0)
	{
		// No symmetric entries (only left/right-only data): expose the average as a flat starting point.
		for (const FOpenDriveCrossfallEntry& Entry : Crossfall)
		{
			Out.Add(Entry.Cubic);
		}
	}
	return Out;
}

void FOpenDriveModelEdit::SetSymmetricCrownShape(FOpenDriveMap& Map, FOpenDriveRoad& Road, double CrownHeight, double HalfWidth)
{
	Road.Shape.Reset();
	HalfWidth = FMath::Max(0.1, HalfWidth);

	// Rising from 0 at t=-HalfWidth to CrownHeight at t=0, falling back to 0 at t=+HalfWidth, flat beyond.
	FOpenDriveShapeEntry Left;
	Left.S = 0.0;
	Left.T = -HalfWidth;
	Left.A = 0.0;
	Left.B = CrownHeight / HalfWidth;
	Road.Shape.Add(Left);

	FOpenDriveShapeEntry Right;
	Right.S = 0.0;
	Right.T = 0.0;
	Right.A = CrownHeight;
	Right.B = -CrownHeight / HalfWidth;
	Road.Shape.Add(Right);

	FOpenDriveShapeEntry Cap;
	Cap.S = 0.0;
	Cap.T = HalfWidth;
	Cap.A = 0.0;
	Road.Shape.Add(Cap);

	Map.ComputeRoadBounds(Road);
}

void FOpenDriveModelEdit::SetRoadType(FOpenDriveMap& Map, FOpenDriveRoad& Road, EOpenDriveRoadType Type, const FString& Country)
{
	Road.Types.Reset();
	FOpenDriveRoadTypeEntry Entry;
	Entry.S = 0.0;
	Entry.Type = Type;
	Entry.Country = Country;
	Road.Types.Add(MoveTemp(Entry));
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

bool FOpenDriveModelEdit::SetLaneRoadMarkConstant(FOpenDriveRoad& Road, int32 LaneId, const FOpenDriveRoadMarkEntry& Mark)
{
	bool bFound = false;
	for (FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		for (FOpenDriveLane& Lane : Section.Lanes)
		{
			if (Lane.Id == LaneId)
			{
				Lane.RoadMarks.Reset();
				FOpenDriveRoadMarkEntry Entry = Mark;
				Entry.S = Section.S;
				Lane.RoadMarks.Add(MoveTemp(Entry));
				bFound = true;
			}
		}
	}
	return bFound;
}

FOpenDriveRoadMarkEntry FOpenDriveModelEdit::GetLaneRoadMark(const FOpenDriveRoad& Road, int32 LaneId)
{
	if (Road.LaneSections.Num() > 0)
	{
		if (const FOpenDriveLane* Lane = Road.LaneSections[0].FindLane(LaneId))
		{
			if (const FOpenDriveRoadMarkEntry* Mark = Lane->FindRoadMarkAt(Road.LaneSections[0].S))
			{
				return *Mark;
			}
		}
	}
	return FOpenDriveRoadMarkEntry();
}

TArray<int32> FOpenDriveModelEdit::GetLaneIds(const FOpenDriveRoad& Road)
{
	TArray<int32> Ids;
	if (Road.LaneSections.Num() > 0)
	{
		for (const FOpenDriveLane& Lane : Road.LaneSections[0].Lanes)
		{
			Ids.Add(Lane.Id);
		}
		Ids.Sort([](int32 A, int32 B) { return A > B; });
	}
	return Ids;
}

int32 FOpenDriveModelEdit::AddLane(FOpenDriveRoad& Road, bool bLeft, double Width)
{
	int32 NewId = bLeft ? 1 : -1;
	for (const FOpenDriveLaneSection& Section : Road.LaneSections)
	{
		for (const FOpenDriveLane& Lane : Section.Lanes)
		{
			if (Lane.Id != 0 && (Lane.Id > 0) == bLeft)
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
		Lane.RoadMarks.Add(FOpenDriveRoadMarkEntry{ Section.S, EOpenDriveRoadMarkType::Broken, EOpenDriveRoadMarkWeight::Standard, EOpenDriveRoadMarkColor::Standard, -1.0, EOpenDriveLaneChange::None, 0.0 });
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
// Road & lane links
// ------------------------------------------------------------------------------------------------

bool FOpenDriveModelEdit::ConnectRoadEnds(FOpenDriveMap& Map, const FString& RoadAId, bool bAtAEnd, const FString& RoadBId, bool bAtBEnd)
{
	FOpenDriveRoad* A = Map.FindRoadMutable(RoadAId);
	FOpenDriveRoad* B = Map.FindRoadMutable(RoadBId);
	if (!A || !B || A == B)
	{
		return false;
	}

	const EOpenDriveContactPoint ContactOnB = bAtBEnd ? EOpenDriveContactPoint::End : EOpenDriveContactPoint::Start;
	const EOpenDriveContactPoint ContactOnA = bAtAEnd ? EOpenDriveContactPoint::End : EOpenDriveContactPoint::Start;
	if (bAtAEnd) { A->SuccessorType = EOpenDriveElementType::Road; A->SuccessorId = RoadBId; A->SuccessorContact = ContactOnB; }
	else { A->PredecessorType = EOpenDriveElementType::Road; A->PredecessorId = RoadBId; A->PredecessorContact = ContactOnB; }
	if (bAtBEnd) { B->SuccessorType = EOpenDriveElementType::Road; B->SuccessorId = RoadAId; B->SuccessorContact = ContactOnA; }
	else { B->PredecessorType = EOpenDriveElementType::Road; B->PredecessorId = RoadAId; B->PredecessorContact = ContactOnA; }

	if (A->LaneSections.Num() > 0 && B->LaneSections.Num() > 0)
	{
		FOpenDriveLaneSection& SecA = bAtAEnd ? A->LaneSections.Last() : A->LaneSections[0];
		FOpenDriveLaneSection& SecB = bAtBEnd ? B->LaneSections.Last() : B->LaneSections[0];
		InferAndApplyLaneLinks(SecA, bAtAEnd, SecB, bAtBEnd);
	}
	return true;
}

void FOpenDriveModelEdit::ClearRoadLink(FOpenDriveRoad& Road, bool bSuccessor)
{
	if (bSuccessor)
	{
		Road.SuccessorType = EOpenDriveElementType::None;
		Road.SuccessorId.Reset();
		Road.SuccessorContact = EOpenDriveContactPoint::None;
	}
	else
	{
		Road.PredecessorType = EOpenDriveElementType::None;
		Road.PredecessorId.Reset();
		Road.PredecessorContact = EOpenDriveContactPoint::None;
	}
}

// ------------------------------------------------------------------------------------------------
// Junctions
// ------------------------------------------------------------------------------------------------

FString FOpenDriveModelEdit::AddJunction(FOpenDriveMap& Map, const FString& Name)
{
	FOpenDriveJunction Junction;
	Junction.Id = MakeUniqueJunctionId(Map);
	Junction.Name = Name;
	const FString NewId = Junction.Id;
	Map.GetJunctionsMutable().Add(MoveTemp(Junction));
	Map.RebuildIndex();
	return NewId;
}

bool FOpenDriveModelEdit::RemoveJunction(FOpenDriveMap& Map, const FString& JunctionId)
{
	TArray<FOpenDriveJunction>& Junctions = Map.GetJunctionsMutable();
	const int32 Index = Junctions.IndexOfByPredicate([&](const FOpenDriveJunction& J) { return J.Id == JunctionId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Junctions.RemoveAt(Index);
	for (FOpenDriveRoad& Road : Map.GetRoadsMutable())
	{
		if (Road.JunctionId == JunctionId)
		{
			Road.JunctionId.Reset();
		}
		if (Road.PredecessorType == EOpenDriveElementType::Junction && Road.PredecessorId == JunctionId) { ClearRoadLink(Road, false); }
		if (Road.SuccessorType == EOpenDriveElementType::Junction && Road.SuccessorId == JunctionId) { ClearRoadLink(Road, true); }
	}
	Map.RebuildIndex();
	return true;
}

FString FOpenDriveModelEdit::AddJunctionConnection(FOpenDriveMap& Map, const FString& JunctionId, const FString& IncomingRoadId, bool bAtIncomingEnd, const FString& ConnectingRoadId, EOpenDriveContactPoint Contact)
{
	FOpenDriveJunction* Junction = nullptr;
	for (FOpenDriveJunction& J : Map.GetJunctionsMutable())
	{
		if (J.Id == JunctionId) { Junction = &J; break; }
	}
	FOpenDriveRoad* Incoming = Map.FindRoadMutable(IncomingRoadId);
	FOpenDriveRoad* Connecting = Map.FindRoadMutable(ConnectingRoadId);
	if (!Junction || !Incoming || !Connecting || Incoming == Connecting)
	{
		return FString();
	}

	if (bAtIncomingEnd) { Incoming->SuccessorType = EOpenDriveElementType::Junction; Incoming->SuccessorId = JunctionId; Incoming->SuccessorContact = EOpenDriveContactPoint::None; }
	else { Incoming->PredecessorType = EOpenDriveElementType::Junction; Incoming->PredecessorId = JunctionId; Incoming->PredecessorContact = EOpenDriveContactPoint::None; }
	Connecting->JunctionId = JunctionId;

	FOpenDriveJunctionConnection Con;
	int32 MaxId = 0;
	for (const FOpenDriveJunctionConnection& Existing : Junction->Connections)
	{
		if (Existing.Id.IsNumeric()) { MaxId = FMath::Max(MaxId, FCString::Atoi(*Existing.Id)); }
	}
	Con.Id = FString::FromInt(MaxId + 1);
	Con.IncomingRoad = IncomingRoadId;
	Con.ConnectingRoad = ConnectingRoadId;
	Con.ContactPoint = Contact;

	if (Incoming->LaneSections.Num() > 0 && Connecting->LaneSections.Num() > 0)
	{
		FOpenDriveLaneSection& SecIncoming = bAtIncomingEnd ? Incoming->LaneSections.Last() : Incoming->LaneSections[0];
		FOpenDriveLaneSection& SecConnecting = (Contact == EOpenDriveContactPoint::End) ? Connecting->LaneSections.Last() : Connecting->LaneSections[0];
		InferAndApplyLaneLinks(SecIncoming, bAtIncomingEnd, SecConnecting, Contact == EOpenDriveContactPoint::End);
		for (const FOpenDriveLane& Lane : SecIncoming.Lanes)
		{
			const int32 Target = bAtIncomingEnd ? Lane.Successor : Lane.Predecessor;
			if (Target != 0)
			{
				Con.LaneLinks.Emplace(Lane.Id, Target);
			}
		}
	}

	const FString NewConnectionId = Con.Id;
	Junction->Connections.Add(MoveTemp(Con));
	return NewConnectionId;
}

bool FOpenDriveModelEdit::RemoveJunctionConnection(FOpenDriveMap& Map, const FString& JunctionId, const FString& ConnectionId)
{
	for (FOpenDriveJunction& J : Map.GetJunctionsMutable())
	{
		if (J.Id == JunctionId)
		{
			return J.Connections.RemoveAll([&](const FOpenDriveJunctionConnection& C) { return C.Id == ConnectionId; }) > 0;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Signals
// ------------------------------------------------------------------------------------------------

FString FOpenDriveModelEdit::AddSignal(FOpenDriveRoad& Road, ESignalPreset Preset, double S, double T, double SpeedLimitKmh)
{
	FOpenDriveSignal Sig;
	Sig.Id = MakeUniqueSignalId(Road);
	Sig.S = S;
	Sig.T = T;
	Sig.Orientation = EOpenDriveSignalOrientation::Plus;

	switch (Preset)
	{
	case ESignalPreset::StopSign:
		Sig.Name = TEXT("Stop");
		Sig.Country = TEXT("DE");
		Sig.Type = TEXT("206");
		Sig.Height = 2.0;
		Sig.Width = 0.6;
		break;
	case ESignalPreset::YieldSign:
		Sig.Name = TEXT("Yield");
		Sig.Country = TEXT("DE");
		Sig.Type = TEXT("205");
		Sig.Height = 2.0;
		Sig.Width = 0.6;
		break;
	case ESignalPreset::SpeedLimit:
		Sig.Name = FString::Printf(TEXT("Speed Limit %.0f"), SpeedLimitKmh);
		Sig.Country = TEXT("DE");
		Sig.Type = TEXT("274");
		Sig.Value = SpeedLimitKmh;
		Sig.Unit = TEXT("km/h");
		Sig.Height = 2.0;
		Sig.Width = 0.6;
		break;
	case ESignalPreset::TrafficLight:
		Sig.Name = TEXT("Traffic Light");
		Sig.Country = TEXT("OpenDRIVE");
		Sig.Type = TEXT("1000001");
		Sig.Subtype = TEXT("1");
		Sig.bDynamic = true;
		Sig.Height = 3.0;
		Sig.Width = 0.3;
		break;
	}

	const FString NewId = Sig.Id;
	Road.Signals.Add(MoveTemp(Sig));
	return NewId;
}

bool FOpenDriveModelEdit::RemoveSignal(FOpenDriveRoad& Road, const FString& SignalId)
{
	return Road.Signals.RemoveAll([&](const FOpenDriveSignal& S) { return S.Id == SignalId; }) > 0;
}

bool FOpenDriveModelEdit::SetSignalPose(FOpenDriveRoad& Road, const FString& SignalId, double S, double T, double ZOffset, double HOffsetRad)
{
	for (FOpenDriveSignal& Sig : Road.Signals)
	{
		if (Sig.Id == SignalId)
		{
			Sig.S = S;
			Sig.T = T;
			Sig.ZOffset = ZOffset;
			Sig.HOffset = HOffsetRad;
			Road.Signals.Sort([](const FOpenDriveSignal& A, const FOpenDriveSignal& B) { return A.S < B.S; });
			return true;
		}
	}
	return false;
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
