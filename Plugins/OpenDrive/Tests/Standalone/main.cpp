// Standalone sanity test for the OpenDrive plugin's engine-independent runtime code (FOpenDriveMap,
// FOpenDriveWriter, FOpenDriveModelEdit, UOpenDriveAsset), compiled against a tiny mock of the Unreal
// core types. No Unreal Engine installation required. Mirrors OpenScenario_UE's Tests/Standalone harness.
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDrive/OpenDriveAsset.h"
#include "OpenDriveWriter.h"
#include "OpenDriveModelEdit.h"
#include "OpenDriveMeshBuilder.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
#include <filesystem>

FVector FVector::OneVector(1, 1, 1);
FVector FVector::UpVector(0, 0, 1);
FTransform FTransform::Identity;

namespace
{
	int Failures = 0;

	void Check(bool Cond, const char* Msg)
	{
		if (!Cond)
		{
			std::printf("FAIL: %s\n", Msg);
			++Failures;
		}
	}

	void CheckNear(double A, double B, double Eps, const char* Msg)
	{
		if (std::fabs(A - B) > Eps)
		{
			std::printf("FAIL: %s (got %.6f, expected %.6f)\n", Msg, A, B);
			++Failures;
		}
	}

	double SectionAreaCm2(const FOpenDriveMeshSection& Section)
	{
		double Area = 0.0;
		for (int32 i = 0; i + 2 < Section.Indices.Num(); i += 3)
		{
			const FVector& A = Section.Positions[Section.Indices[i]];
			const FVector& B = Section.Positions[Section.Indices[i + 1]];
			const FVector& C = Section.Positions[Section.Indices[i + 2]];
			Area += 0.5 * FVector::CrossProduct(B - A, C - A).Size();
		}
		return Area;
	}

	const char* SampleXodr()
	{
		return R"XODR(<?xml version="1.0"?>
<OpenDRIVE>
	<header name="Test" />
	<road name="Main" length="100.0" id="1" junction="-1">
		<planView>
			<geometry s="0" x="0" y="0" hdg="0" length="100">
				<line/>
			</geometry>
		</planView>
		<elevationProfile>
			<elevation s="0" a="0" b="0.05" c="0" d="0"/>
			<elevation s="50" a="2.5" b="0" c="0" d="0"/>
		</elevationProfile>
		<lateralProfile>
			<superelevation s="0" a="0" b="0" c="0" d="0"/>
			<superelevation s="50" a="0.08726646" b="0" c="0" d="0"/>
		</lateralProfile>
		<lanes>
			<laneSection s="0">
				<left>
					<lane id="1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
						<roadMark sOffset="0" type="solid" weight="standard" color="standard" laneChange="none"/>
					</lane>
				</left>
				<center>
					<lane id="0" type="none" level="false">
						<roadMark sOffset="0" type="broken" weight="standard" color="yellow" laneChange="both"/>
					</lane>
				</center>
				<right>
					<lane id="-1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
						<roadMark sOffset="0" type="solid" weight="bold" color="standard" laneChange="none"/>
					</lane>
				</right>
			</laneSection>
		</lanes>
	</road>
</OpenDRIVE>
)XODR";
	}

	void CheckRoadMarks(const FOpenDriveMap& RMap, const char* Tag)
	{
		const FOpenDriveRoad* RRoad = RMap.FindRoad(FString("1"));
		if (!RRoad || RRoad->LaneSections.Num() == 0)
		{
			Check(false, (FString("find road 1 for roadmarks (") + Tag + ")").S.c_str());
			return;
		}
		const FOpenDriveLaneSection& Sec = RRoad->LaneSections[0];

		const FOpenDriveLane* CenterLane = Sec.FindLane(0);
		Check(CenterLane != nullptr, (FString("center lane (id 0) is stored (") + Tag + ")").S.c_str());
		if (CenterLane)
		{
			Check(CenterLane->RoadMarks.Num() == 1, (FString("center lane has one roadmark (") + Tag + ")").S.c_str());
			if (CenterLane->RoadMarks.Num() == 1)
			{
				Check(CenterLane->RoadMarks[0].Type == EOpenDriveRoadMarkType::Broken, (FString("center roadmark type (") + Tag + ")").S.c_str());
				Check(CenterLane->RoadMarks[0].Color == EOpenDriveRoadMarkColor::Yellow, (FString("center roadmark color (") + Tag + ")").S.c_str());
				Check(CenterLane->RoadMarks[0].LaneChange == EOpenDriveLaneChange::Both, (FString("center roadmark laneChange (") + Tag + ")").S.c_str());
			}
		}

		const FOpenDriveLane* LeftLane = Sec.FindLane(1);
		Check(LeftLane != nullptr && LeftLane->RoadMarks.Num() == 1 && LeftLane->RoadMarks[0].Type == EOpenDriveRoadMarkType::Solid,
			(FString("left lane roadmark (") + Tag + ")").S.c_str());

		const FOpenDriveLane* RightLane = Sec.FindLane(-1);
		Check(RightLane != nullptr && RightLane->RoadMarks.Num() == 1 && RightLane->RoadMarks[0].Weight == EOpenDriveRoadMarkWeight::Bold,
			(FString("right lane roadmark weight (") + Tag + ")").S.c_str());
	}

	const char* SignalXodr()
	{
		return R"XODR(<?xml version="1.0"?>
<OpenDRIVE>
	<header name="SignalTest" />
	<road name="WithSignals" length="60.0" id="9" junction="-1">
		<planView>
			<geometry s="0" x="0" y="0" hdg="0" length="60">
				<line/>
			</geometry>
		</planView>
		<lanes>
			<laneSection s="0">
				<left>
					<lane id="1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
					</lane>
				</left>
				<center>
					<lane id="0" type="none" level="false"/>
				</center>
				<right>
					<lane id="-1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
					</lane>
				</right>
			</laneSection>
		</lanes>
		<signals>
			<signal s="30" t="-4.5" id="1" name="Stop" dynamic="no" orientation="+" zOffset="0" country="DE" type="206" subtype="" value="0" unit="" height="2" width="0.6" hOffset="0" pitch="0" roll="0"/>
			<signal s="10" t="-4.5" id="2" name="Light" dynamic="yes" orientation="+" zOffset="0" country="OpenDRIVE" type="1000001" subtype="1" value="0" unit="" height="3" width="0.3" hOffset="0" pitch="0" roll="0"/>
		</signals>
	</road>
	<controller id="1" name="MainLight">
		<control signalId="2" type=""/>
	</controller>
</OpenDRIVE>
)XODR";
	}

	void CheckSignals(const FOpenDriveMap& SigMap, const char* Tag)
	{
		const FOpenDriveRoad* Road = SigMap.FindRoad(FString("9"));
		if (!Road)
		{
			Check(false, (FString("find road 9 (") + Tag + ")").S.c_str());
			return;
		}
		Check(Road->Signals.Num() == 2, (FString("two signals parsed (") + Tag + ")").S.c_str());
		if (Road->Signals.Num() == 2)
		{
			// Sorted by S ascending: id 2 (s=10) then id 1 (s=30).
			Check(Road->Signals[0].Id == FString("2") && Road->Signals[0].bDynamic && Road->Signals[0].Type == FString("1000001"),
				(FString("traffic light signal fields (") + Tag + ")").S.c_str());
			Check(Road->Signals[1].Id == FString("1") && !Road->Signals[1].bDynamic && Road->Signals[1].Country == FString("DE") && Road->Signals[1].Type == FString("206"),
				(FString("stop sign fields (") + Tag + ")").S.c_str());
			CheckNear(Road->Signals[1].T, -4.5, 1e-9, (FString("stop sign t (") + Tag + ")").S.c_str());
		}
		Check(SigMap.GetControllers().Num() == 1, (FString("one controller parsed (") + Tag + ")").S.c_str());
		if (SigMap.GetControllers().Num() == 1)
		{
			const FOpenDriveController& Ctrl = SigMap.GetControllers()[0];
			Check(Ctrl.Id == FString("1") && Ctrl.Controls.Num() == 1 && Ctrl.Controls[0].SignalId == FString("2"),
				(FString("controller fields (") + Tag + ")").S.c_str());
		}
		Check(SigMap.FindController(FString("1")) != nullptr, (FString("FindController (") + Tag + ")").S.c_str());
	}

	const char* CrossfallXodr()
	{
		return R"XODR(<?xml version="1.0"?>
<OpenDRIVE>
	<header name="CrossfallTest" />
	<road name="Banked" length="60.0" id="2" junction="-1">
		<type s="0" type="motorway" country="US">
			<speed max="120" unit="km/h"/>
		</type>
		<planView>
			<geometry s="0" x="0" y="0" hdg="0" length="60">
				<line/>
			</geometry>
		</planView>
		<lateralProfile>
			<crossfall side="left" s="0" a="0.02" b="0" c="0" d="0"/>
			<crossfall side="right" s="0" a="-0.03" b="0" c="0" d="0"/>
			<shape s="0" t="-2" a="0" b="0.05" c="0" d="0"/>
			<shape s="0" t="0" a="0.1" b="-0.05" c="0" d="0"/>
			<shape s="0" t="2" a="0" b="0" c="0" d="0"/>
		</lateralProfile>
		<lanes>
			<laneSection s="0">
				<left>
					<lane id="1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
					</lane>
				</left>
				<center>
					<lane id="0" type="none" level="false"/>
				</center>
				<right>
					<lane id="-1" type="driving" level="false">
						<width sOffset="0" a="3.5" b="0" c="0" d="0"/>
					</lane>
				</right>
			</laneSection>
		</lanes>
	</road>
</OpenDRIVE>
)XODR";
	}

	void CheckCrossfallRoad(const FOpenDriveMap& CMap, const char* Tag)
	{
		const FOpenDriveRoad* CRoad = CMap.FindRoad(FString("2"));
		if (!CRoad)
		{
			Check(false, (FString("find road 2 (") + Tag + ")").S.c_str());
			return;
		}
		Check(CRoad->Types.Num() == 1, (FString("type entry count (") + Tag + ")").S.c_str());
		if (CRoad->Types.Num() == 1)
		{
			Check(CRoad->Types[0].Type == EOpenDriveRoadType::Motorway, (FString("road type is motorway (") + Tag + ")").S.c_str());
			Check(CRoad->Types[0].Country == FString("US"), (FString("road type country (") + Tag + ")").S.c_str());
		}
		CheckNear(CMap.GetSpeedLimit(*CRoad, 10.0, -1), 120.0 / 3.6, 1e-6, (FString("road-type speed limit (") + Tag + ")").S.c_str());

		CheckNear(CMap.GetCrossfallAngle(*CRoad, 10.0, true), 0.02, 1e-9, (FString("crossfall left angle (") + Tag + ")").S.c_str());
		CheckNear(CMap.GetCrossfallAngle(*CRoad, 10.0, false), -0.03, 1e-9, (FString("crossfall right angle (") + Tag + ")").S.c_str());

		CheckNear(CMap.GetShapeZ(*CRoad, 10.0, 1.0), 0.05, 1e-9, (FString("shape z at t=1 (") + Tag + ")").S.c_str());
		CheckNear(CMap.GetShapeZ(*CRoad, 10.0, -1.0), 0.05, 1e-9, (FString("shape z at t=-1 (") + Tag + ")").S.c_str());

		// No superelevation on this road, so EvaluatePose must fall back to the per-side crossfall angle,
		// plus the additive shape ("road carving") term.
		const FOpenDrivePose Left = CMap.EvaluatePose(*CRoad, 10.0, 1.0);
		CheckNear(Left.Z, 1.0 * std::sin(0.02) + 0.05, 1e-6, (FString("pose z, left side (") + Tag + ")").S.c_str());
		const FOpenDrivePose Right = CMap.EvaluatePose(*CRoad, 10.0, -1.0);
		CheckNear(Right.Z, -1.0 * std::sin(-0.03) + 0.05, 1e-6, (FString("pose z, right side (") + Tag + ")").S.c_str());
	}
}

int main()
{
	// --- Parsing ------------------------------------------------------------------------------
	FOpenDriveMap Map;
	FString Error;
	Check(Map.LoadFromString(SampleXodr(), Error), "parse sample xodr");
	Check(Map.GetRoads().Num() == 1, "one road parsed");

	const FOpenDriveRoad* Road = Map.FindRoad(FString("1"));
	Check(Road != nullptr, "find road 1");
	if (Road)
	{
		CheckNear(Road->Length, 100.0, 1e-9, "road length");

		// Elevation: climbing at 5% grade to s=50 (z=2.5), flat after.
		const FOpenDrivePose P25 = Map.EvaluatePose(*Road, 25.0, 0.0);
		CheckNear(P25.Z, 1.25, 1e-6, "elevation at s=25");
		const FOpenDrivePose P75 = Map.EvaluatePose(*Road, 75.0, 0.0);
		CheckNear(P75.Z, 2.5, 1e-6, "elevation at s=75 (flat tail)");

		// Superelevation: 0 before s=50, ~5 degrees (0.08726646 rad) after.
		CheckNear(Map.GetSuperelevation(*Road, 25.0), 0.0, 1e-9, "superelevation before s=50");
		CheckNear(Map.GetSuperelevation(*Road, 75.0), 0.08726646, 1e-6, "superelevation after s=50");

		const double ExpectedBankZ = 2.5 + 1.75 * std::sin(0.08726646);
		const FOpenDrivePose P75Banked = Map.EvaluatePose(*Road, 75.0, 1.75);
		CheckNear(P75Banked.Z, ExpectedBankZ, 1e-6, "banked z at t=1.75, s=75");

		CheckNear(Map.GetLaneWidth(*Road, 10.0, 1), 3.5, 1e-9, "left lane width");
		CheckNear(Map.GetLaneWidth(*Road, 10.0, -1), 3.5, 1e-9, "right lane width");
	}

	// --- Writer round trip ----------------------------------------------------------------------
	const FString Written = FOpenDriveWriter::Write(Map);
	Check(Written.S.find("<OpenDRIVE>") != std::string::npos, "writer emits root element");

	FOpenDriveMap Reparsed;
	FString ReparseError;
	Check(Reparsed.LoadFromString(Written, ReparseError), "reparse written xml");
	Check(Reparsed.GetRoads().Num() == 1, "round trip: one road");
	if (const FOpenDriveRoad* R2 = Reparsed.FindRoad(FString("1")))
	{
		CheckNear(R2->Length, 100.0, 1e-6, "round trip: road length");
		const FOpenDrivePose Q75 = Reparsed.EvaluatePose(*R2, 75.0, 1.75);
		CheckNear(Q75.Z, 2.5 + 1.75 * std::sin(0.08726646), 1e-4, "round trip: banked z preserved");
		CheckNear(Reparsed.GetLaneWidth(*R2, 10.0, 1), 3.5, 1e-6, "round trip: left lane width preserved");
	}
	else
	{
		Check(false, "round trip: find road 1");
	}

	// --- Road marks: parse + writer round trip --------------------------------------------------
	CheckRoadMarks(Map, "parsed");
	Check(Written.S.find("roadMark") != std::string::npos, "writer emits roadMark");
	CheckRoadMarks(Reparsed, "round trip");

	// --- Crossfall, shape ("road carving") and road type: parse + writer round trip ----------------
	{
		FOpenDriveMap CMap;
		FString CErr;
		Check(CMap.LoadFromString(CrossfallXodr(), CErr), "parse crossfall xodr");
		CheckCrossfallRoad(CMap, "parsed");

		const FString CWritten = FOpenDriveWriter::Write(CMap);
		Check(CWritten.S.find("crossfall") != std::string::npos, "writer emits crossfall");
		Check(CWritten.S.find("shape") != std::string::npos, "writer emits shape");
		Check(CWritten.S.find("motorway") != std::string::npos, "writer emits road type");

		FOpenDriveMap CReparsed;
		FString CReparseErr;
		Check(CReparsed.LoadFromString(CWritten, CReparseErr), "reparse crossfall xodr");
		CheckCrossfallRoad(CReparsed, "round trip");
	}

	// --- Model editing ----------------------------------------------------------------------------
	{
		FOpenDriveMap EditMap;
		FString Err;
		EditMap.LoadFromString(SampleXodr(), Err);

		const FString NewId = FOpenDriveModelEdit::AddStraightRoad(EditMap, TEXT("NewRoad"), 10.0, 20.0, 0.3, 80.0);
		Check(!NewId.IsEmpty() && NewId != FString("1"), "AddStraightRoad returns a fresh id");
		const FOpenDriveRoad* NewRoad = EditMap.FindRoad(NewId);
		Check(NewRoad != nullptr, "AddStraightRoad: road findable");
		if (NewRoad)
		{
			CheckNear(NewRoad->Length, 80.0, 1e-9, "AddStraightRoad: length");
			Check(NewRoad->Geometry.Num() == 1 && NewRoad->Geometry[0].Type == EOpenDriveGeometryType::Line, "AddStraightRoad: single line geometry");
			CheckNear(NewRoad->Geometry[0].X, 10.0, 1e-9, "AddStraightRoad: start X");

			const TArray<int32> LaneIds = FOpenDriveModelEdit::GetLaneIds(*NewRoad);
			Check(LaneIds.Num() == 3 && LaneIds[0] == 1 && LaneIds[1] == 0 && LaneIds[2] == -1, "AddStraightRoad: GetLaneIds is [1, 0, -1]");
			Check(FOpenDriveModelEdit::GetLaneRoadMark(*NewRoad, 0).Type == EOpenDriveRoadMarkType::Broken, "AddStraightRoad: default centre mark is broken");
			Check(FOpenDriveModelEdit::GetLaneRoadMark(*NewRoad, 1).Type == EOpenDriveRoadMarkType::Solid, "AddStraightRoad: default left mark is solid");
		}

		Check(FOpenDriveModelEdit::SetStraightRoadBasics(EditMap, NewId, TEXT("Renamed"), 15.0, 25.0, 0.5, 90.0), "SetStraightRoadBasics succeeds");
		if (const FOpenDriveRoad* Renamed = EditMap.FindRoad(NewId))
		{
			Check(Renamed->Name == FString("Renamed"), "SetStraightRoadBasics: name");
			CheckNear(Renamed->Length, 90.0, 1e-9, "SetStraightRoadBasics: length");
			CheckNear(Renamed->Geometry[0].X, 15.0, 1e-9, "SetStraightRoadBasics: X");
		}

		const FString DupId = FOpenDriveModelEdit::DuplicateRoad(EditMap, NewId, 5.0, 5.0);
		Check(!DupId.IsEmpty() && DupId != NewId, "DuplicateRoad returns a fresh id");
		if (const FOpenDriveRoad* Dup = EditMap.FindRoad(DupId))
		{
			CheckNear(Dup->Geometry[0].X, 20.0, 1e-9, "DuplicateRoad: offset X");
			CheckNear(Dup->Geometry[0].Y, 30.0, 1e-9, "DuplicateRoad: offset Y");
		}
		Check(FOpenDriveModelEdit::RemoveRoad(EditMap, DupId), "RemoveRoad succeeds");
		Check(EditMap.FindRoad(DupId) == nullptr, "RemoveRoad: road gone");

		if (FOpenDriveRoad* Road1 = EditMap.FindRoadMutable(FString("1")))
		{
			const int32 NewLaneId = FOpenDriveModelEdit::AddLane(*Road1, true, 3.0);
			Check(NewLaneId == 2, "AddLane: next left id is 2");
			CheckNear(EditMap.GetLaneWidth(*Road1, 10.0, NewLaneId), 3.0, 1e-9, "AddLane: width");
			Check(FOpenDriveModelEdit::RemoveLane(*Road1, NewLaneId), "RemoveLane succeeds");
			Check(Road1->LaneSections[0].FindLane(NewLaneId) == nullptr, "RemoveLane: lane gone");

			Check(FOpenDriveModelEdit::SetLaneWidthConstant(*Road1, -1, 4.0), "SetLaneWidthConstant succeeds");
			CheckNear(EditMap.GetLaneWidth(*Road1, 60.0, -1), 4.0, 1e-9, "SetLaneWidthConstant: new width");

			TArray<FOpenDriveCubic> CrossfallCubics;
			CrossfallCubics.Add(FOpenDriveCubic{ 0.0, 0.04, 0.0, 0.0, 0.0 });
			FOpenDriveModelEdit::SetCrossfallProfile(EditMap, *Road1, CrossfallCubics);
			Check(Road1->Crossfall.Num() == 1 && Road1->Crossfall[0].Side == EOpenDriveCrossfallSide::Both, "SetCrossfallProfile: single Both-side entry");
			CheckNear(EditMap.GetCrossfallAngle(*Road1, 10.0, true), 0.04, 1e-9, "SetCrossfallProfile: left angle");
			CheckNear(EditMap.GetCrossfallAngle(*Road1, 10.0, false), 0.04, 1e-9, "SetCrossfallProfile: right angle (Both side)");
			const TArray<FOpenDriveCubic> ExtractedCrossfall = FOpenDriveModelEdit::ExtractCrossfallCubics(Road1->Crossfall);
			Check(ExtractedCrossfall.Num() == 1, "ExtractCrossfallCubics: round trips Both entries");

			FOpenDriveModelEdit::SetSymmetricCrownShape(EditMap, *Road1, 0.1, 2.0);
			CheckNear(EditMap.GetShapeZ(*Road1, 10.0, 0.0), 0.1, 1e-9, "SetSymmetricCrownShape: peak at centre");
			CheckNear(EditMap.GetShapeZ(*Road1, 10.0, -2.0), 0.0, 1e-9, "SetSymmetricCrownShape: zero at -HalfWidth");
			CheckNear(EditMap.GetShapeZ(*Road1, 10.0, 2.0), 0.0, 1e-9, "SetSymmetricCrownShape: zero at +HalfWidth");
			CheckNear(EditMap.GetShapeZ(*Road1, 10.0, 1.0), 0.05, 1e-9, "SetSymmetricCrownShape: halfway down the falling side");

			FOpenDriveModelEdit::SetRoadType(EditMap, *Road1, EOpenDriveRoadType::Rural, TEXT("DE"));
			Check(Road1->Types.Num() == 1 && Road1->Types[0].Type == EOpenDriveRoadType::Rural && Road1->Types[0].Country == FString("DE"), "SetRoadType");

			// --- Lane material/access/rule/height (data model + writer round trip) --------------------
			if (FOpenDriveLane* Lane1 = Road1->LaneSections[0].FindLaneMutable(1))
			{
				Lane1->Materials.Add(FOpenDriveLaneMaterialEntry{ 0.0, 0.8, 0.02, TEXT("asphalt") });
				Lane1->Access.Add(FOpenDriveLaneAccessEntry{ 0.0, false, TEXT("bicycle") });
				Lane1->Rules.Add(FOpenDriveLaneRuleEntry{ 0.0, TEXT("no overtaking") });
				Lane1->Heights.Add(FOpenDriveLaneHeightEntry{ 0.0, 0.0, 0.15 });
			}
			{
				const FString LaneFieldsXml = FOpenDriveWriter::Write(EditMap);
				Check(LaneFieldsXml.S.find("material") != std::string::npos, "writer emits lane material");
				Check(LaneFieldsXml.S.find("asphalt") != std::string::npos, "writer emits material surface");
				Check(LaneFieldsXml.S.find("<access") != std::string::npos, "writer emits lane access");
				Check(LaneFieldsXml.S.find("no overtaking") != std::string::npos, "writer emits lane rule");
				Check(LaneFieldsXml.S.find("<height") != std::string::npos, "writer emits lane height");

				FOpenDriveMap LaneFieldsReparsed;
				FString LaneFieldsErr;
				Check(LaneFieldsReparsed.LoadFromString(LaneFieldsXml, LaneFieldsErr), "reparse lane fields xml");
				if (const FOpenDriveLane* Reparsed1 = LaneFieldsReparsed.FindRoad(FString("1"))
					? LaneFieldsReparsed.FindRoad(FString("1"))->LaneSections[0].FindLane(1) : nullptr)
				{
					Check(Reparsed1->Materials.Num() == 1 && Reparsed1->Materials[0].Surface == FString("asphalt"), "round trip: material surface");
					CheckNear(Reparsed1->Materials.Num() == 1 ? Reparsed1->Materials[0].Friction : -1.0, 0.8, 1e-9, "round trip: material friction");
					Check(Reparsed1->Access.Num() == 1 && !Reparsed1->Access[0].bAllow && Reparsed1->Access[0].Restriction == FString("bicycle"), "round trip: access deny bicycle");
					Check(Reparsed1->Rules.Num() == 1 && Reparsed1->Rules[0].Value == FString("no overtaking"), "round trip: rule value");
					Check(Reparsed1->Heights.Num() == 1, "round trip: height entry present");
					if (Reparsed1->Heights.Num() == 1)
					{
						CheckNear(Reparsed1->Heights[0].OuterHeight, 0.15, 1e-9, "round trip: height outer");
					}
				}
				else
				{
					Check(false, "round trip: find lane 1 for lane-field checks");
				}
			}

			FOpenDriveRoadMarkEntry NewMark;
			NewMark.Type = EOpenDriveRoadMarkType::BottsDots;
			NewMark.Color = EOpenDriveRoadMarkColor::Blue;
			NewMark.Weight = EOpenDriveRoadMarkWeight::Bold;
			Check(FOpenDriveModelEdit::SetLaneRoadMarkConstant(*Road1, 1, NewMark), "SetLaneRoadMarkConstant succeeds");
			const FOpenDriveRoadMarkEntry Readback = FOpenDriveModelEdit::GetLaneRoadMark(*Road1, 1);
			Check(Readback.Type == EOpenDriveRoadMarkType::BottsDots && Readback.Color == EOpenDriveRoadMarkColor::Blue && Readback.Weight == EOpenDriveRoadMarkWeight::Bold,
				"GetLaneRoadMark reflects SetLaneRoadMarkConstant");
		}
		else
		{
			Check(false, "find road 1 for lane edits");
		}
	}

	// --- Signals & controllers: parse + writer round trip ------------------------------------------
	{
		FOpenDriveMap SigMap;
		FString SigErr;
		Check(SigMap.LoadFromString(SignalXodr(), SigErr), "parse signal xodr");
		CheckSignals(SigMap, "parsed");

		const FString SigWritten = FOpenDriveWriter::Write(SigMap);
		Check(SigWritten.S.find("<signal ") != std::string::npos, "writer emits signal");
		Check(SigWritten.S.find("<controller ") != std::string::npos, "writer emits controller");

		FOpenDriveMap SigReparsed;
		FString SigReparseErr;
		Check(SigReparsed.LoadFromString(SigWritten, SigReparseErr), "reparse signal xodr");
		CheckSignals(SigReparsed, "round trip");

		// --- Signal model-edit helpers ---------------------------------------------------------
		if (FOpenDriveRoad* Road = SigMap.FindRoadMutable(FString("9")))
		{
			const FString NewId = FOpenDriveModelEdit::AddSignal(*Road, FOpenDriveModelEdit::ESignalPreset::SpeedLimit, 45.0, -4.5, 80.0);
			Check(!NewId.IsEmpty() && NewId != FString("1") && NewId != FString("2"), "AddSignal returns a fresh id");
			const FOpenDriveSignal* NewSig = Road->Signals.FindByPredicate([&](const FOpenDriveSignal& S) { return S.Id == NewId; });
			Check(NewSig != nullptr && NewSig->Type == FString("274"), "AddSignal: speed limit preset type");
			CheckNear(NewSig ? NewSig->Value : -1.0, 80.0, 1e-9, "AddSignal: speed limit value");

			Check(FOpenDriveModelEdit::SetSignalPose(*Road, NewId, 20.0, -5.0, 0.1, 0.2), "SetSignalPose succeeds");
			const FOpenDriveSignal* Moved = Road->Signals.FindByPredicate([&](const FOpenDriveSignal& S) { return S.Id == NewId; });
			Check(Moved != nullptr, "moved signal still findable");
			if (Moved)
			{
				CheckNear(Moved->S, 20.0, 1e-9, "SetSignalPose: S");
				CheckNear(Moved->T, -5.0, 1e-9, "SetSignalPose: T");
			}

			Check(FOpenDriveModelEdit::RemoveSignal(*Road, NewId), "RemoveSignal succeeds");
			Check(Road->Signals.FindByPredicate([&](const FOpenDriveSignal& S) { return S.Id == NewId; }) == nullptr, "RemoveSignal: signal gone");
			Check(Road->Signals.Num() == 2, "RemoveSignal: back to original two signals");
		}
		else
		{
			Check(false, "find road 9 for signal model-edit checks");
		}
	}

	// --- Road links & junction authoring -----------------------------------------------------------
	{
		FOpenDriveMap LinkMap;
		const FString RoadA = FOpenDriveModelEdit::AddStraightRoad(LinkMap, TEXT("A"), 0.0, 0.0, 0.0, 50.0);
		const FString RoadB = FOpenDriveModelEdit::AddStraightRoad(LinkMap, TEXT("B"), 50.0, 0.0, 0.0, 50.0);
		const FString RoadC = FOpenDriveModelEdit::AddStraightRoad(LinkMap, TEXT("C"), 100.0, 0.0, 0.0, 50.0);
		Check(RoadA == FString("1") && RoadB == FString("2") && RoadC == FString("3"), "link test: expected road ids");

		// Straight continuation: A's end -> B's start.
		Check(FOpenDriveModelEdit::ConnectRoadEnds(LinkMap, RoadA, true, RoadB, false), "ConnectRoadEnds A-end to B-start");
		FOpenDriveRoad* A = LinkMap.FindRoadMutable(RoadA);
		FOpenDriveRoad* B = LinkMap.FindRoadMutable(RoadB);
		Check(A && A->SuccessorType == EOpenDriveElementType::Road && A->SuccessorId == RoadB && A->SuccessorContact == EOpenDriveContactPoint::Start, "A.Successor set");
		Check(B && B->PredecessorType == EOpenDriveElementType::Road && B->PredecessorId == RoadA && B->PredecessorContact == EOpenDriveContactPoint::End, "B.Predecessor set");
		if (A && B)
		{
			Check(A->LaneSections.Last().FindLane(1)->Successor == 1, "A lane 1 -> B lane 1 (no flip)");
			Check(A->LaneSections.Last().FindLane(-1)->Successor == -1, "A lane -1 -> B lane -1 (no flip)");
			Check(B->LaneSections[0].FindLane(1)->Predecessor == 1, "B lane 1 <- A lane 1");
		}

		TArray<FOpenDriveSuccessor> Succ;
		LinkMap.GetSuccessors(*A, true, Succ);
		Check(Succ.Num() == 1 && Succ[0].RoadId == RoadB && Succ[0].bForward, "GetSuccessors resolves the authored A->B link");
		if (Succ.Num() == 1)
		{
			const int32* Mapped = Succ[0].LaneMap.Find(1);
			Check(Mapped && *Mapped == 1, "GetSuccessors lane map A.1 -> B.1");
		}

		TArray<FOpenDriveRouteStep> Route;
		Check(LinkMap.FindRoute(RoadA, true, RoadB, Route) && Route.Num() == 2, "FindRoute across the authored link");

		// Reversed continuation: B's end -> C's end (C entered backwards, so lane ids flip).
		Check(FOpenDriveModelEdit::ConnectRoadEnds(LinkMap, RoadB, true, RoadC, true), "ConnectRoadEnds B-end to C-end");
		FOpenDriveRoad* C = LinkMap.FindRoadMutable(RoadC);
		if (B && C)
		{
			Check(B->LaneSections.Last().FindLane(1)->Successor == -1, "B lane 1 -> C lane -1 (flipped)");
			Check(C->LaneSections.Last().FindLane(-1)->Successor == 1, "C lane -1 -> B lane 1 (flipped, back reference)");
		}

		FOpenDriveModelEdit::ClearRoadLink(*A, true);
		Check(A->SuccessorType == EOpenDriveElementType::None && A->SuccessorId.IsEmpty(), "ClearRoadLink clears successor");

		// Junction authoring.
		const FString RoadD = FOpenDriveModelEdit::AddStraightRoad(LinkMap, TEXT("D"), 200.0, 0.0, 0.0, 30.0);
		const FString RoadE = FOpenDriveModelEdit::AddStraightRoad(LinkMap, TEXT("E"), 230.0, 0.0, 0.0, 30.0);
		const FString JunctionId = FOpenDriveModelEdit::AddJunction(LinkMap, TEXT("TestJunction"));
		Check(!JunctionId.IsEmpty() && LinkMap.FindJunction(JunctionId) != nullptr, "AddJunction");

		const FString ConId = FOpenDriveModelEdit::AddJunctionConnection(LinkMap, JunctionId, RoadD, true, RoadE, EOpenDriveContactPoint::Start);
		Check(!ConId.IsEmpty(), "AddJunctionConnection returns an id");
		const FOpenDriveJunction* Junction = LinkMap.FindJunction(JunctionId);
		Check(Junction && Junction->Connections.Num() == 1, "junction has one connection");
		if (Junction && Junction->Connections.Num() == 1)
		{
			const FOpenDriveJunctionConnection& Con = Junction->Connections[0];
			Check(Con.IncomingRoad == RoadD && Con.ConnectingRoad == RoadE && Con.ContactPoint == EOpenDriveContactPoint::Start, "connection fields");
			Check(Con.LaneLinks.Num() == 2, "connection has 2 lane links (id 1 and -1, no flip)");
		}
		const FOpenDriveRoad* D = LinkMap.FindRoad(RoadD);
		const FOpenDriveRoad* E = LinkMap.FindRoad(RoadE);
		Check(D && D->SuccessorType == EOpenDriveElementType::Junction && D->SuccessorId == JunctionId, "incoming road's successor is the junction");
		Check(E && E->JunctionId == JunctionId, "connecting road is marked as belonging to the junction");

		TArray<FOpenDriveSuccessor> JuncSucc;
		LinkMap.GetSuccessors(*D, true, JuncSucc);
		Check(JuncSucc.Num() == 1 && JuncSucc[0].RoadId == RoadE, "GetSuccessors resolves through the authored junction");

		Check(FOpenDriveModelEdit::RemoveJunctionConnection(LinkMap, JunctionId, ConId), "RemoveJunctionConnection");
		Check(LinkMap.FindJunction(JunctionId)->Connections.Num() == 0, "connection removed");
		Check(FOpenDriveModelEdit::RemoveJunction(LinkMap, JunctionId), "RemoveJunction");
		Check(LinkMap.FindJunction(JunctionId) == nullptr, "junction removed");
		Check(LinkMap.FindRoad(RoadE)->JunctionId.IsEmpty(), "RemoveJunction clears connecting road's JunctionId");
		Check(LinkMap.FindRoad(RoadD)->SuccessorType == EOpenDriveElementType::None, "RemoveJunction clears incoming road's link to it");
	}

	// --- Plan-view geometry authoring (Phase 5) ----------------------------------------------------
	{
		FOpenDriveMap GeoMap;
		const FString RoadId = FOpenDriveModelEdit::AddStraightRoad(GeoMap, TEXT("Geo"), 0.0, 0.0, 0.0, 50.0);
		FOpenDriveRoad* Road = GeoMap.FindRoadMutable(RoadId);
		Check(Road != nullptr, "geo test: find road");
		if (Road)
		{
			double EndX0, EndY0, EndH0;
			GeoMap.EvaluateReferenceLine(*Road, Road->Length, EndX0, EndY0, EndH0);

			// Append a line: continues straight from the end of segment 0.
			FOpenDriveModelEdit::AppendLineSegment(GeoMap, *Road, 20.0);
			Check(Road->Geometry.Num() == 2, "AppendLineSegment: adds a segment");
			CheckNear(Road->Length, 70.0, 1e-9, "AppendLineSegment: extends road length");
			CheckNear(Road->LaneSections.Last().EndS, 70.0, 1e-9, "AppendLineSegment: extends last lane section EndS");
			CheckNear(Road->Geometry[1].S, 50.0, 1e-9, "AppendLineSegment: new segment starts at old length");
			CheckNear(Road->Geometry[1].X, EndX0, 1e-6, "AppendLineSegment: continuous X");
			CheckNear(Road->Geometry[1].Y, EndY0, 1e-6, "AppendLineSegment: continuous Y");
			CheckNear(Road->Geometry[1].Hdg, EndH0, 1e-6, "AppendLineSegment: continuous heading");

			double MidX, MidY, MidH;
			GeoMap.EvaluateReferenceLine(*Road, 70.0, MidX, MidY, MidH);

			// Append an arc: a quarter turn of radius 10 (curvature 0.1).
			const double Curvature = 0.1;
			const double ArcLength = M_PI / 2.0 / Curvature; // quarter circle
			FOpenDriveModelEdit::AppendArcSegment(GeoMap, *Road, ArcLength, Curvature);
			Check(Road->Geometry.Num() == 3, "AppendArcSegment: adds a segment");
			CheckNear(Road->Geometry[2].X, MidX, 1e-6, "AppendArcSegment: continuous X");
			CheckNear(Road->Geometry[2].Y, MidY, 1e-6, "AppendArcSegment: continuous Y");
			CheckNear(Road->Geometry[2].Hdg, MidH, 1e-6, "AppendArcSegment: continuous heading");
			CheckNear(Road->Length, 70.0 + ArcLength, 1e-6, "AppendArcSegment: extends road length");

			double ArcEndX, ArcEndY, ArcEndH;
			GeoMap.EvaluateReferenceLine(*Road, Road->Length, ArcEndX, ArcEndY, ArcEndH);
			CheckNear(ArcEndH, MidH + M_PI / 2.0, 1e-6, "AppendArcSegment: quarter turn changes heading by 90 degrees");

			// Append a spiral (clothoid) from curvature 0 back to 0.1 over 10 m, continuous with the arc's end.
			FOpenDriveModelEdit::AppendSpiralSegment(GeoMap, *Road, 10.0, 0.0, 0.1);
			Check(Road->Geometry.Num() == 4, "AppendSpiralSegment: adds a segment");
			CheckNear(Road->Geometry[3].X, ArcEndX, 1e-6, "AppendSpiralSegment: continuous X");
			CheckNear(Road->Geometry[3].Y, ArcEndY, 1e-6, "AppendSpiralSegment: continuous Y");
			CheckNear(Road->Geometry[3].Hdg, ArcEndH, 1e-6, "AppendSpiralSegment: continuous heading");

			const double LengthBeforeRemove = Road->Length;
			Check(FOpenDriveModelEdit::RemoveLastGeometrySegment(GeoMap, *Road), "RemoveLastGeometrySegment: removes the spiral");
			Check(Road->Geometry.Num() == 3, "RemoveLastGeometrySegment: segment count back to 3");
			CheckNear(Road->Length, LengthBeforeRemove - 10.0, 1e-9, "RemoveLastGeometrySegment: length shrinks back");
			CheckNear(Road->LaneSections.Last().EndS, Road->Length, 1e-9, "RemoveLastGeometrySegment: lane section EndS follows");

			const FString Desc = FOpenDriveModelEdit::DescribeGeometrySegment(Road->Geometry[2]);
			Check(Desc.S.find("Arc") != std::string::npos, "DescribeGeometrySegment: names the arc segment");

			// Down to a single segment: refuse to remove the last one.
			FOpenDriveModelEdit::RemoveLastGeometrySegment(GeoMap, *Road); // removes the arc -> 2 left
			FOpenDriveModelEdit::RemoveLastGeometrySegment(GeoMap, *Road); // removes the appended line -> 1 left
			Check(!FOpenDriveModelEdit::RemoveLastGeometrySegment(GeoMap, *Road), "RemoveLastGeometrySegment: refuses to remove the only segment");
			Check(Road->Geometry.Num() == 1, "RemoveLastGeometrySegment: one segment remains");
		}

		// --- Lane section splitting ---
		FOpenDriveRoad* LaneRoad = GeoMap.FindRoadMutable(RoadId);
		if (LaneRoad)
		{
			LaneRoad->Length = 50.0;
			LaneRoad->LaneSections.Reset();
			FOpenDriveLaneSection Section;
			Section.S = 0.0;
			Section.EndS = 50.0;
			FOpenDriveLane L;
			L.Id = 1;
			L.Type = TEXT("driving");
			L.Widths.Add(FOpenDriveCubic{ 0.0, 3.5, 0.0, 0.0, 0.0 });
			Section.Lanes.Add(L);
			LaneRoad->LaneSections.Add(Section);

			Check(!FOpenDriveModelEdit::InsertLaneSection(GeoMap, *LaneRoad, 0.0), "InsertLaneSection: refuses at S=0 (already a boundary)");
			Check(!FOpenDriveModelEdit::InsertLaneSection(GeoMap, *LaneRoad, 50.0), "InsertLaneSection: refuses at S=length (already a boundary)");
			Check(FOpenDriveModelEdit::InsertLaneSection(GeoMap, *LaneRoad, 20.0), "InsertLaneSection: splits at S=20");
			Check(LaneRoad->LaneSections.Num() == 2, "InsertLaneSection: now two sections");
			if (LaneRoad->LaneSections.Num() == 2)
			{
				CheckNear(LaneRoad->LaneSections[0].S, 0.0, 1e-9, "InsertLaneSection: first section starts at 0");
				CheckNear(LaneRoad->LaneSections[0].EndS, 20.0, 1e-9, "InsertLaneSection: first section ends at split");
				CheckNear(LaneRoad->LaneSections[1].S, 20.0, 1e-9, "InsertLaneSection: second section starts at split");
				CheckNear(LaneRoad->LaneSections[1].EndS, 50.0, 1e-9, "InsertLaneSection: second section ends at road length");
				Check(LaneRoad->LaneSections[1].FindLane(1) != nullptr, "InsertLaneSection: lane structure duplicated into the new section");
			}
			Check(!FOpenDriveModelEdit::InsertLaneSection(GeoMap, *LaneRoad, 20.0), "InsertLaneSection: refuses at an existing boundary");
		}
	}

	// --- Objects & junction groups (Phase 6) -------------------------------------------------------
	{
		FOpenDriveMap ObjMap;
		const FString RoadId = FOpenDriveModelEdit::AddStraightRoad(ObjMap, TEXT("ObjRoad"), 0.0, 0.0, 0.0, 50.0);
		FOpenDriveRoad* Road = ObjMap.FindRoadMutable(RoadId);
		Check(Road != nullptr, "objects test: find road");
		if (Road)
		{
			const FString PoleId = FOpenDriveModelEdit::AddRoundObject(*Road, TEXT("pole"), 10.0, -4.0, 0.15, 6.0);
			Check(!PoleId.IsEmpty(), "AddRoundObject returns an id");
			const FString TreeId = FOpenDriveModelEdit::AddObject(*Road, TEXT("tree"), 20.0, 6.0, 2.0, 2.0, 8.0);
			Check(!TreeId.IsEmpty() && TreeId != PoleId, "AddObject returns a distinct id");
			Check(Road->Objects.Num() == 2, "two objects on the road");

			Check(FOpenDriveModelEdit::SetObjectPose(*Road, PoleId, 12.0, -4.5, 0.0, 0.1), "SetObjectPose succeeds");
			const FOpenDriveObject* Pole = Road->Objects.FindByPredicate([&](const FOpenDriveObject& O) { return O.Id == PoleId; });
			Check(Pole != nullptr, "find pole after pose update");
			if (Pole)
			{
				CheckNear(Pole->S, 12.0, 1e-9, "SetObjectPose updates S");
				CheckNear(Pole->T, -4.5, 1e-9, "SetObjectPose updates T");
			}

			Check(FOpenDriveModelEdit::RemoveObject(*Road, TreeId), "RemoveObject removes the tree");
			Check(Road->Objects.Num() == 1, "one object remains after removal");
			Check(!FOpenDriveModelEdit::RemoveObject(*Road, TreeId), "RemoveObject is a no-op for an already-removed id");
		}

		// Round trip through the writer/parser: attribute names must match on both sides.
		const FString Xml = FOpenDriveWriter::Write(ObjMap);
		Check(Xml.S.find("<objects>") != std::string::npos, "writer emits <objects>");
		Check(Xml.S.find("radius=") != std::string::npos, "writer emits round-object radius");

		FOpenDriveMap Reparsed;
		FString ReparseErr;
		Check(Reparsed.LoadFromString(Xml, ReparseErr), "objects xml reparses");
		const FOpenDriveRoad* ReparsedRoad = Reparsed.FindRoad(RoadId);
		Check(ReparsedRoad && ReparsedRoad->Objects.Num() == 1, "reparsed road keeps the surviving object");
		if (ReparsedRoad && ReparsedRoad->Objects.Num() == 1)
		{
			const FOpenDriveObject& Pole = ReparsedRoad->Objects[0];
			Check(Pole.Type == FString("pole"), "reparsed object type");
			CheckNear(Pole.S, 12.0, 1e-6, "reparsed object S");
			CheckNear(Pole.Radius, 0.15, 1e-6, "reparsed object radius");
			CheckNear(Pole.Height, 6.0, 1e-6, "reparsed object height");
		}

		// Junction groups.
		FOpenDriveMap GroupMap;
		FOpenDriveModelEdit::AddStraightRoad(GroupMap, TEXT("R1"), 0.0, 0.0, 0.0, 10.0);
		const FString J1 = FOpenDriveModelEdit::AddJunction(GroupMap, TEXT("J1"));
		const FString J2 = FOpenDriveModelEdit::AddJunction(GroupMap, TEXT("J2"));
		const FString GroupId = FOpenDriveModelEdit::AddJunctionGroup(GroupMap, TEXT("Roundabout"), TEXT("roundabout"));
		Check(!GroupId.IsEmpty(), "AddJunctionGroup returns an id");
		Check(FOpenDriveModelEdit::AddJunctionToGroup(GroupMap, GroupId, J1), "AddJunctionToGroup J1");
		Check(FOpenDriveModelEdit::AddJunctionToGroup(GroupMap, GroupId, J2), "AddJunctionToGroup J2");
		Check(!FOpenDriveModelEdit::AddJunctionToGroup(GroupMap, FString("nope"), J1), "AddJunctionToGroup fails for an unknown group");
		const FOpenDriveJunctionGroup* Group = GroupMap.FindJunctionGroup(GroupId);
		Check(Group && Group->JunctionRefs.Num() == 2, "junction group has two members");
		FOpenDriveModelEdit::AddJunctionToGroup(GroupMap, GroupId, J1);
		Check(Group->JunctionRefs.Num() == 2, "AddJunctionToGroup is idempotent (AddUnique)");

		Check(FOpenDriveModelEdit::RemoveJunctionFromGroup(GroupMap, GroupId, J1), "RemoveJunctionFromGroup J1");
		Check(Group->JunctionRefs.Num() == 1 && Group->JunctionRefs[0] == J2, "only J2 remains");

		const FString GroupXml = FOpenDriveWriter::Write(GroupMap);
		Check(GroupXml.S.find("<junctionGroup") != std::string::npos, "writer emits <junctionGroup>");
		FOpenDriveMap GroupReparsed;
		FString GroupReparseErr;
		Check(GroupReparsed.LoadFromString(GroupXml, GroupReparseErr), "junction group xml reparses");
		const FOpenDriveJunctionGroup* ReparsedGroup = GroupReparsed.FindJunctionGroup(GroupId);
		Check(ReparsedGroup && ReparsedGroup->Type == FString("roundabout"), "reparsed group type");
		Check(ReparsedGroup && ReparsedGroup->JunctionRefs.Num() == 1 && ReparsedGroup->JunctionRefs[0] == J2, "reparsed group membership");

		Check(FOpenDriveModelEdit::RemoveJunctionGroup(GroupMap, GroupId), "RemoveJunctionGroup");
		Check(GroupMap.FindJunctionGroup(GroupId) == nullptr, "junction group removed");
	}

	// --- Mesh generation (Phase 7) ------------------------------------------------------------------
	{
		Check(FOpenDriveMeshBuilder::ClassifyLaneType(TEXT("driving")) == EOpenDriveMeshMaterialSlot::Asphalt, "ClassifyLaneType: driving -> Asphalt");
		Check(FOpenDriveMeshBuilder::ClassifyLaneType(TEXT("shoulder")) == EOpenDriveMeshMaterialSlot::Asphalt, "ClassifyLaneType: unrecognised type falls back to Asphalt");
		Check(FOpenDriveMeshBuilder::ClassifyLaneType(TEXT("Sidewalk")) == EOpenDriveMeshMaterialSlot::Sidewalk, "ClassifyLaneType: sidewalk is case-insensitive");
		Check(FOpenDriveMeshBuilder::ClassifyLaneType(TEXT("median")) == EOpenDriveMeshMaterialSlot::GrassMedian, "ClassifyLaneType: median -> GrassMedian");

		FOpenDriveMap MeshMap;
		const FString RoadId = FOpenDriveModelEdit::AddStraightRoad(MeshMap, TEXT("MeshRoad"), 0.0, 0.0, 0.0, 50.0);
		const FOpenDriveRoad* Road = MeshMap.FindRoad(RoadId);
		Check(Road != nullptr, "mesh test: find road");
		if (Road)
		{
			FOpenDriveMeshBuildParams Params;
			const FOpenDriveRoadMesh Mesh = FOpenDriveMeshBuilder::BuildRoad(MeshMap, *Road, Params);
			Check(Mesh.RoadId == RoadId, "BuildRoad: mesh carries the road id");
			Check(!Mesh.IsEmpty(), "BuildRoad: mesh is non-empty");

			const FOpenDriveMeshSection& Asphalt = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::Asphalt];
			const FOpenDriveMeshSection& Marking = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::RoadMarking];
			const FOpenDriveMeshSection& Curb = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::Curb];
			const FOpenDriveMeshSection& Sidewalk = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::Sidewalk];
			const FOpenDriveMeshSection& Grass = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::GrassMedian];

			Check(!Asphalt.IsEmpty(), "flat straight road: Asphalt section is non-empty (two driving lanes)");
			Check(!Marking.IsEmpty(), "flat straight road: RoadMarking section is non-empty (default lane marks)");
			Check(Curb.IsEmpty(), "flat straight road: Curb section is empty (no curb-type marks)");
			Check(Sidewalk.IsEmpty(), "flat straight road: Sidewalk section is empty (no sidewalk lanes)");
			Check(Grass.IsEmpty(), "flat straight road: GrassMedian section is empty (no median lanes)");

			Check(Asphalt.Indices.Num() % 3 == 0, "Asphalt index count is a multiple of 3");
			Check(Asphalt.Positions.Num() == Asphalt.Normals.Num() && Asphalt.Positions.Num() == Asphalt.UVs.Num(), "Asphalt position/normal/UV counts match");
			for (const int32 Index : Asphalt.Indices)
			{
				Check(Asphalt.Positions.IsValidIndex(Index), "Asphalt index is within range");
			}

			// Two 3.5 m driving lanes over 50 m, dead flat (no elevation/superelevation/shape): exact area.
			CheckNear(SectionAreaCm2(Asphalt), 2.0 * 3.5 * 50.0 * 100.0 * 100.0, 1.0, "Asphalt area matches 2 lanes * 3.5m * 50m");
			// Three default marks (centre broken + two side solid), all at the default 0.12 m width, no dashing.
			CheckNear(SectionAreaCm2(Marking), 3.0 * 0.12 * 50.0 * 100.0 * 100.0, 1.0, "RoadMarking area matches 3 marks * 0.12m * 50m");

			// Flat, unbanked road: every generated normal should point straight up.
			bool bAllNormalsUp = true;
			for (const FVector& N : Asphalt.Normals)
			{
				if (FMath::Abs(N.X) > 1e-6 || FMath::Abs(N.Y) > 1e-6 || N.Z < 1.0 - 1e-6)
				{
					bAllNormalsUp = false;
					break;
				}
			}
			Check(bAllNormalsUp, "Asphalt normals point straight up on a flat, unbanked road");
		}

		// A sidewalk lane with a curb-type road mark at its inner border.
		FOpenDriveMap CurbMap;
		const FString CurbRoadId = FOpenDriveModelEdit::AddStraightRoad(CurbMap, TEXT("CurbRoad"), 0.0, 0.0, 0.0, 40.0);
		FOpenDriveRoad* CurbRoad = CurbMap.FindRoadMutable(CurbRoadId);
		Check(CurbRoad != nullptr, "curb test: find road");
		if (CurbRoad && CurbRoad->LaneSections.Num() > 0)
		{
			FOpenDriveLane Sidewalk;
			Sidewalk.Id = 2;
			Sidewalk.Type = TEXT("sidewalk");
			Sidewalk.Widths.Add(FOpenDriveCubic{ 0.0, 2.0, 0.0, 0.0, 0.0 });
			Sidewalk.RoadMarks.Add(FOpenDriveRoadMarkEntry{ 0.0, EOpenDriveRoadMarkType::Curb, EOpenDriveRoadMarkWeight::Standard, EOpenDriveRoadMarkColor::Standard, -1.0, EOpenDriveLaneChange::None, 0.0 });
			CurbRoad->LaneSections[0].Lanes.Add(Sidewalk);
			CurbRoad->LaneSections[0].Lanes.Sort([](const FOpenDriveLane& A, const FOpenDriveLane& B) { return A.Id < B.Id; });

			const FOpenDriveMeshBuildParams Params;
			const FOpenDriveRoadMesh Mesh = FOpenDriveMeshBuilder::BuildRoad(CurbMap, *CurbRoad, Params);
			const FOpenDriveMeshSection& Sidewalk2 = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::Sidewalk];
			const FOpenDriveMeshSection& Curb2 = Mesh.Sections[(int32)EOpenDriveMeshMaterialSlot::Curb];
			Check(!Sidewalk2.IsEmpty(), "sidewalk lane produces Sidewalk section geometry");
			Check(!Curb2.IsEmpty(), "curb-type road mark produces Curb section geometry");
			if (!Curb2.IsEmpty())
			{
				double MinZ = Curb2.Positions[0].Z;
				for (const FVector& P : Curb2.Positions)
				{
					MinZ = FMath::Min(MinZ, P.Z);
				}
				CheckNear(MinZ, Params.CurbHeight * 100.0, 1e-3, "curb geometry is lifted by the default curb height");
			}
		}

		// BuildMap collects every road that produces geometry.
		const TArray<FOpenDriveRoadMesh> AllMeshes = FOpenDriveMeshBuilder::BuildMap(MeshMap);
		Check(AllMeshes.Num() == 1, "BuildMap returns one mesh for the one-road map");
	}

	// --- Profile <-> points round trip -----------------------------------------------------------
	{
		TArray<FOpenDriveCubic> Profile;
		Profile.Add(FOpenDriveCubic{ 0.0, 0.0, 0.05, 0.0, 0.0 });
		Profile.Add(FOpenDriveCubic{ 50.0, 2.5, 0.0, 0.0, 0.0 });
		const TArray<FVector2D> Points = FOpenDriveModelEdit::ProfileToPoints(Profile, 100.0);
		Check(Points.Num() == 3, "ProfileToPoints: 2 segments + trailing point");
		CheckNear(Points[0].X, 0.0, 1e-9, "ProfileToPoints: point0 s");
		CheckNear(Points[0].Y, 0.0, 1e-9, "ProfileToPoints: point0 value");
		CheckNear(Points[1].X, 50.0, 1e-9, "ProfileToPoints: point1 s");
		CheckNear(Points[1].Y, 2.5, 1e-9, "ProfileToPoints: point1 value");
		CheckNear(Points[2].X, 100.0, 1e-9, "ProfileToPoints: trailing point s");
		CheckNear(Points[2].Y, 2.5, 1e-9, "ProfileToPoints: trailing point value (flat)");

		const TArray<FOpenDriveCubic> RoundTripped = FOpenDriveModelEdit::PointsToProfile(Points);
		Check(RoundTripped.Num() == 2, "PointsToProfile: 2 segments");
		for (double S = 0.0; S <= 100.0; S += 10.0)
		{
			CheckNear(FOpenDriveCubic::EvalPiecewise(RoundTripped, S), FOpenDriveCubic::EvalPiecewise(Profile, S), 1e-9, "profile round trip matches at sample point");
		}
	}

	// --- UOpenDriveAsset ---------------------------------------------------------------------------
	{
		UOpenDriveAsset Asset;
		Check(Asset.SetSource(SampleXodr(), FString("test.xodr")), "asset SetSource parses ok");
		Check(Asset.IsMapValid(), "asset map valid");
		Check(Asset.RoadCount == 1, "asset road count");
		const TArray<FString> Ids = Asset.GetRoadIds();
		Check(Ids.Num() == 1 && Ids[0] == FString("1"), "asset GetRoadIds");

		FTransform T;
		Check(Asset.GetRoadTransform(FString("1"), 10.0, 0.0, T), "asset GetRoadTransform");
		CheckNear(T.T.X, 1000.0, 1e-6, "asset transform X (m -> cm)");

		FOpenDriveMap NewMap;
		FString Err;
		NewMap.LoadFromString(SampleXodr(), Err);
		FOpenDriveModelEdit::AddStraightRoad(NewMap, TEXT("Extra"), 0.0, 0.0, 0.0, 30.0);
		Asset.ApplyMap(NewMap);
		Check(Asset.RoadCount == 2, "asset ApplyMap updates road count");
		Check(Asset.SourceXml.S.find("Extra") != std::string::npos, "asset ApplyMap regenerates XML");

		const std::string TmpPath = (std::filesystem::temp_directory_path() / "opendrive_standalone_test.xodr").string();
		Check(Asset.ExportToFile(FString(TmpPath.c_str())), "asset ExportToFile succeeds");
		FString ReadBack;
		Check(FFileHelper::LoadFileToString(ReadBack, TmpPath.c_str()) && ReadBack.S.find("<OpenDRIVE>") != std::string::npos, "asset ExportToFile wrote valid xml");
		std::remove(TmpPath.c_str());
	}

	if (Failures == 0)
	{
		std::printf("All OpenDrive standalone tests passed.\n");
		return 0;
	}
	std::printf("%d OpenDrive standalone check(s) failed.\n", Failures);
	return 1;
}
