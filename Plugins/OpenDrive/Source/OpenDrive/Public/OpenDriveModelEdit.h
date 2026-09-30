#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"

/**
 * Mutation helpers used by the OpenDRIVE editor tool mode. These operate directly on an in-memory
 * FOpenDriveMap ("working copy" pattern, mirroring OpenScenario_UE's FOSCModelEdit): the editor edits
 * the map, then serialises it back to XML with FOpenDriveWriter when the user applies the changes.
 *
 * Geometry editing is intentionally limited to single straight segments (roads created or repositioned
 * from the tool); roads imported with arcs/spirals/polynomials keep their original planView untouched
 * unless explicitly replaced.
 */
class OPENDRIVE_API FOpenDriveModelEdit
{
public:
	/** An id not currently used by any road in Map ("1", "2", ... by default). */
	static FString MakeUniqueRoadId(const FOpenDriveMap& Map);
	/** An id not currently used by any junction in Map. */
	static FString MakeUniqueJunctionId(const FOpenDriveMap& Map);

	/** Appends a new single-segment straight road with one lane section (one driving lane per side). Returns its Id. */
	static FString AddStraightRoad(FOpenDriveMap& Map, const FString& Name, double StartX, double StartY, double StartHeadingRad, double Length);
	/** Removes a road (and any junction connections that reference it) and rebuilds the index. */
	static bool RemoveRoad(FOpenDriveMap& Map, const FString& RoadId);
	/** Duplicates a road under a new Id, offset by (OffsetX, OffsetY) in the OpenDRIVE plane. Returns the new Id. */
	static FString DuplicateRoad(FOpenDriveMap& Map, const FString& RoadId, double OffsetX, double OffsetY);

	/**
	 * Edits name/start-pose/length of a road whose reference line is a single straight segment (as created
	 * by AddStraightRoad). Does nothing and returns false for roads with more than one geometry segment or
	 * a non-Line segment.
	 */
	static bool SetStraightRoadBasics(FOpenDriveMap& Map, const FString& RoadId, const FString& NewName, double StartX, double StartY, double StartHeadingRad, double NewLength);
	/** Renames a road without touching its geometry. */
	static bool RenameRoad(FOpenDriveMap& Map, const FString& RoadId, const FString& NewName);

	static void SetElevationProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile);
	static void SetSuperelevationProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile);
	static void SetLaneOffsetProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile);
	/** Replaces the crossfall profile with a single-sided-Both profile built from NewProfile (radians). Any
	 *  existing asymmetric (left/right only) crossfall data is discarded -- see FOpenDriveCrossfallEntry. */
	static void SetCrossfallProfile(FOpenDriveMap& Map, FOpenDriveRoad& Road, TArray<FOpenDriveCubic> NewProfile);
	/** Best-effort flattening of Road.Crossfall to a single per-s value (for display only) -- picks the "Both"
	 *  entry where present, otherwise averages Left/Right. Use SetCrossfallProfile to write it back. */
	static TArray<FOpenDriveCubic> ExtractCrossfallCubics(const TArray<FOpenDriveCrossfallEntry>& Crossfall);

	/**
	 * Replaces the lateral profile "shape" (road carving) with a simple symmetric crown: CrownHeight (metres,
	 * can be negative for a gutter) at the road centre, falling linearly to 0 at +/-HalfWidth, flat beyond.
	 * This is a deliberately simplified authoring path over the fully general (s,t) shape table -- imported
	 * files with a real per-side shape are preserved until this is called.
	 */
	static void SetSymmetricCrownShape(FOpenDriveMap& Map, FOpenDriveRoad& Road, double CrownHeight, double HalfWidth);

	/** Sets (or replaces) a single road-type entry at s=0, applying to the whole road. */
	static void SetRoadType(FOpenDriveMap& Map, FOpenDriveRoad& Road, EOpenDriveRoadType Type, const FString& Country = FString());

	/** Sets a constant width (metres) for LaneId across every lane section of Road. */
	static bool SetLaneWidthConstant(FOpenDriveRoad& Road, int32 LaneId, double Width);
	/** Sets a single, constant road mark for LaneId across every lane section of Road. */
	static bool SetLaneRoadMarkConstant(FOpenDriveRoad& Road, int32 LaneId, const FOpenDriveRoadMarkEntry& Mark);
	/** The road mark active at S for LaneId in Road's first lane section, or a default Solid mark if none. */
	static FOpenDriveRoadMarkEntry GetLaneRoadMark(const FOpenDriveRoad& Road, int32 LaneId);
	/** Every distinct lane Id present in Road's first lane section, left-to-right (descending Id). */
	static TArray<int32> GetLaneIds(const FOpenDriveRoad& Road);
	/** Adds a new outermost driving lane on the given side to every lane section. Returns the new lane Id. */
	static int32 AddLane(FOpenDriveRoad& Road, bool bLeft, double Width);
	/** Removes a lane (by Id) from every lane section. */
	static bool RemoveLane(FOpenDriveRoad& Road, int32 LaneId);

	// --- Road & lane links ----------------------------------------------------------------------
	/**
	 * Connects RoadA's end (bAtAEnd: true = A's successor/end, false = A's predecessor/start) to RoadB's
	 * end (bAtBEnd likewise), setting both roads' road-level link fields, and infers + applies matching
	 * lane-to-lane predecessor/successor links on the two touching lane sections: a lane keeps its Id when
	 * the target is entered at its start (a straight continuation), or gets the sign-flipped Id when the
	 * target is entered at its end (the direction of travel reverses, so left/right swap); pairs with no
	 * matching lane on the other side are left unlinked. Overwrites whatever link each road already had on
	 * the given end. Returns false if either road id is unknown.
	 */
	static bool ConnectRoadEnds(FOpenDriveMap& Map, const FString& RoadAId, bool bAtAEnd, const FString& RoadBId, bool bAtBEnd);
	/** Clears Road's predecessor or successor link (road-level only; existing lane links are left as-is). */
	static void ClearRoadLink(FOpenDriveRoad& Road, bool bSuccessor);

	// --- Junctions --------------------------------------------------------------------------------
	/** Creates an empty junction. Returns its Id. */
	static FString AddJunction(FOpenDriveMap& Map, const FString& Name);
	/** Removes a junction and clears the junction="" flag it may have set on its connecting roads. */
	static bool RemoveJunction(FOpenDriveMap& Map, const FString& JunctionId);
	/**
	 * Adds a connection from IncomingRoadId (entered at its end given by bAtIncomingEnd) to ConnectingRoadId
	 * (entered at Contact) inside Junction: sets IncomingRoad's road-level link on that end to point at the
	 * junction, marks ConnectingRoad as belonging to it (Road.JunctionId), and infers + applies lane links
	 * the same way ConnectRoadEnds does (matching Id, or sign-flipped when Contact is End). Returns the new
	 * connection's Id, or an empty string if the junction or either road is unknown.
	 */
	static FString AddJunctionConnection(FOpenDriveMap& Map, const FString& JunctionId, const FString& IncomingRoadId, bool bAtIncomingEnd, const FString& ConnectingRoadId, EOpenDriveContactPoint Contact);
	static bool RemoveJunctionConnection(FOpenDriveMap& Map, const FString& JunctionId, const FString& ConnectionId);

	// --- Profile <-> editable point conversion (piecewise-linear: each stored segment has C = D = 0) ----
	/** One point per stored segment start, plus a trailing point at RoadLength holding the last segment's value. */
	static TArray<FVector2D> ProfileToPoints(const TArray<FOpenDriveCubic>& Profile, double RoadLength);
	/** Points must be sorted by X (S); rebuilds a piecewise-linear profile spanning [Points[0].X, Points.Last().X]. */
	static TArray<FOpenDriveCubic> PointsToProfile(TArray<FVector2D> Points);
};
