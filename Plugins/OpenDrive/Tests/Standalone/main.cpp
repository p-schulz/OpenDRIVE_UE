// Standalone sanity test for the OpenDrive plugin's engine-independent runtime code (FOpenDriveMap,
// FOpenDriveWriter, FOpenDriveModelEdit, UOpenDriveAsset), compiled against a tiny mock of the Unreal
// core types. No Unreal Engine installation required. Mirrors OpenScenario_UE's Tests/Standalone harness.
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDrive/OpenDriveAsset.h"
#include "OpenDriveWriter.h"
#include "OpenDriveModelEdit.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
#include <filesystem>

FVector FVector::OneVector(1, 1, 1);
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
		}
		else
		{
			Check(false, "find road 1 for lane edits");
		}
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
