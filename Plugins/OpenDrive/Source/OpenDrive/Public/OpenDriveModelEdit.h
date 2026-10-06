#pragma once

#include "CoreMinimal.h"
#include "OpenDrive/OpenDriveMap.h"

/** One point around a roundabout's ring where an external road can be connected (see
 *  FOpenDriveModelEdit::AddRoundabout). */
struct OPENDRIVE_API FOpenDriveRoundaboutLeg
{
	/** The ring segment that starts here -- connect an entering road's end to this segment's start
	 *  (EOpenDriveContactPoint::Start) so entering traffic joins the circulatory roadway. */
	FString EntryRingSegmentId;
	/** The ring segment that ends here -- connect this segment's end (bAtIncomingEnd = true) to an exiting
	 *  road so traffic leaving the roundabout has somewhere to go. */
	FString ExitRingSegmentId;
	double X = 0.0;
	double Y = 0.0;
	/** Heading (radians) pointing away from the roundabout's centre. A road meeting the ring here should
	 *  face back toward the centre, i.e. have a heading of OutwardHeadingRad + PI at its connecting end. */
	double OutwardHeadingRad = 0.0;
};

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

	// --- Plan-view geometry authoring -------------------------------------------------------------
	/**
	 * Appends a Line/Arc/Spiral segment to the end of Road's reference line, starting exactly where the
	 * current last segment ends (position and heading taken from Map.EvaluateReferenceLine at Road.Length),
	 * so appended segments are always position- and heading-continuous with what came before. Extends
	 * Road.Length and the last lane section's EndS to match. Curvature is in 1/m (positive = left turn);
	 * for AppendSpiral, CurvStart/CurvEnd are the curvature at the start/end of the transition. Poly3 and
	 * ParamPoly3 segments are not authorable this way -- append/insert only covers line/arc/spiral, per the
	 * plan's scope (numeric append, not free-form control-point dragging).
	 */
	static void AppendLineSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length);
	static void AppendArcSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length, double Curvature);
	static void AppendSpiralSegment(FOpenDriveMap& Map, FOpenDriveRoad& Road, double Length, double CurvStart, double CurvEnd);
	/** Removes the last geometry segment (undoes an Append*), unless it is the road's only segment. Shrinks
	 *  Road.Length and the last lane section's EndS to the new total length. Returns false if only one segment remains. */
	static bool RemoveLastGeometrySegment(FOpenDriveMap& Map, FOpenDriveRoad& Road);
	/** Human-readable one-line description of a geometry segment, for the plan-view segment list. */
	static FString DescribeGeometrySegment(const FOpenDriveGeometry& Geo);

	/**
	 * Splits the lane section active at S into two, duplicating its lane structure (ids/types/road marks;
	 * widths are copied as-is, so both halves start identical and can then be edited independently, e.g. to
	 * taper a lane in/out). No-op (returns false) if S is not strictly inside a section (i.e. already a
	 * section boundary, or outside [0, Road.Length]).
	 */
	static bool InsertLaneSection(FOpenDriveMap& Map, FOpenDriveRoad& Road, double S);

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

	/**
	 * Sets which of HighRoadId/LowRoadId has the <priority> at JunctionId, replacing any existing entry for
	 * this pair (either order). Both must already be a connecting-road id of one of the junction's
	 * connections (see FOpenDriveJunctionConnection::ConnectingRoad); returns false and makes no change if
	 * JunctionId is unknown or either road id doesn't belong to one of its connections.
	 */
	static bool SetJunctionPriority(FOpenDriveMap& Map, const FString& JunctionId, const FString& HighRoadId, const FString& LowRoadId);
	/** Removes the <priority> entry between RoadA and RoadB (either order) at JunctionId. Returns false if none existed. */
	static bool RemoveJunctionPriority(FOpenDriveMap& Map, const FString& JunctionId, const FString& RoadA, const FString& RoadB);

	/**
	 * Creates a German/European-style roundabout: a one-way circulatory ring split into NumLegs equal arc
	 * segments around one new junction, each already linked to its neighbours (single "driving" lane per
	 * segment, travelling counter-clockwise -- correct for right-hand traffic, where circulating vehicles
	 * keep the centre island on their left). Returns the new junction's Id; OutLegs is filled with one
	 * entry per leg giving the position/heading an external road should meet the ring at.
	 *
	 * This only builds the ring -- it does not create, move or connect any spoke roads, and it does not
	 * place Yield/roundabout signs (those belong on the entering roads, which don't exist yet at this
	 * point). Wiring a road in afterwards is an ordinary junction connection: call AddJunctionConnection
	 * twice (or use the Road List tab's Junction section) -- once with the spoke road as the incoming road
	 * and the leg's EntryRingSegmentId as the connecting road (entering movement), and once with the leg's
	 * ExitRingSegmentId as the incoming road (entered at its end) and the spoke road as the connecting road
	 * (exiting movement).
	 */
	static FString AddRoundabout(FOpenDriveMap& Map, const FString& Name, double CenterX, double CenterY, double Radius, int32 NumLegs, double LaneWidth, TArray<FOpenDriveRoundaboutLeg>& OutLegs);

	// --- Signals -------------------------------------------------------------------------------
	enum class ESignalPreset : uint8 { StopSign, YieldSign, SpeedLimit, TrafficLight };

	/**
	 * Adds a signal at (S, T) using a preset that fills in the ASAM/Vienna Convention codes the spec's own
	 * examples use, so the caller doesn't need to know them: StopSign/YieldSign use country "DE" (206/205);
	 * SpeedLimit uses "DE" 274 with SpeedLimitKmh as its value; TrafficLight uses the generic country
	 * "OpenDRIVE" type 1000001 (a standard 3-aspect signal) and is marked dynamic. Returns the new signal's Id.
	 */
	static FString AddSignal(FOpenDriveRoad& Road, ESignalPreset Preset, double S, double T, double SpeedLimitKmh = 50.0);
	static bool RemoveSignal(FOpenDriveRoad& Road, const FString& SignalId);
	static bool SetSignalPose(FOpenDriveRoad& Road, const FString& SignalId, double S, double T, double ZOffset, double HOffsetRad);

	/**
	 * Replaces SignalId's <validity> ranges on Road. Each range's FromLane/ToLane must both exist as a lane
	 * in the lane section active at the signal's own S (checked against Map); returns false and makes no
	 * change if SignalId is unknown on Road or any range fails that check. An empty NewValidity is always
	 * accepted (it means "applies to all lanes" -- see FOpenDriveSignal::AppliesToLane) and clears any
	 * existing ranges.
	 */
	static bool SetSignalValidity(const FOpenDriveMap& Map, FOpenDriveRoad& Road, const FString& SignalId, TArray<FOpenDriveSignalValidity> NewValidity);

	// --- Signal references (placing one signal/sign on more than one road or junction approach) --
	/** Adds a <signalReference> on Road pointing at SignalId, at (S, T), with the given orientation.
	 *  Returns false and adds nothing if SignalId doesn't resolve to an existing <signal> anywhere in Map. */
	static bool AddSignalReference(const FOpenDriveMap& Map, FOpenDriveRoad& Road, const FString& SignalId, double S, double T, EOpenDriveSignalOrientation Orientation);
	/** Removes the reference on Road matching (SignalId, S, T) -- its identifying tuple, since a
	 *  <signalReference> has no id of its own in the spec. Returns false if none matches. */
	static bool RemoveSignalReference(FOpenDriveRoad& Road, const FString& SignalId, double S, double T);
	/** Replaces the validity ranges of the reference matching (SignalId, S, T); same lane-existence check
	 *  as SetSignalValidity. Returns false if no reference matches or a range fails the check. */
	static bool SetSignalReferenceValidity(const FOpenDriveMap& Map, FOpenDriveRoad& Road, const FString& SignalId, double S, double T, TArray<FOpenDriveSignalValidity> NewValidity);

	// --- Objects (static props: poles, trees, barriers, ...) -------------------------------------
	/** Adds a box-footprint object (Length/Width/Height) at (S, T) with the given free-form Type string
	 *  (e.g. "pole", "tree", "barrier" -- the spec's suggested categories, but any string is accepted).
	 *  Returns the new object's Id. */
	static FString AddObject(FOpenDriveRoad& Road, const FString& Type, double S, double T, double Length, double Width, double Height);
	/** Adds a round-footprint object (Radius) at (S, T), e.g. a pole or tree trunk. Returns the new object's Id. */
	static FString AddRoundObject(FOpenDriveRoad& Road, const FString& Type, double S, double T, double Radius, double Height);
	static bool RemoveObject(FOpenDriveRoad& Road, const FString& ObjectId);
	static bool SetObjectPose(FOpenDriveRoad& Road, const FString& ObjectId, double S, double T, double ZOffset, double HOffsetRad);

	// --- Junction groups (e.g. splitting a roundabout into several <junction> elements) -----------
	/** Creates an empty junction group (Type is typically "roundabout", but stored verbatim). Returns its Id. */
	static FString AddJunctionGroup(FOpenDriveMap& Map, const FString& Name, const FString& Type);
	static bool RemoveJunctionGroup(FOpenDriveMap& Map, const FString& JunctionGroupId);
	/** Adds JunctionId to the group's member list if not already present. Returns false if the group is unknown. */
	static bool AddJunctionToGroup(FOpenDriveMap& Map, const FString& JunctionGroupId, const FString& JunctionId);
	static bool RemoveJunctionFromGroup(FOpenDriveMap& Map, const FString& JunctionGroupId, const FString& JunctionId);

	// --- Profile <-> editable point conversion (piecewise-linear: each stored segment has C = D = 0) ----
	/** One point per stored segment start, plus a trailing point at RoadLength holding the last segment's value. */
	static TArray<FVector2D> ProfileToPoints(const TArray<FOpenDriveCubic>& Profile, double RoadLength);
	/** Points must be sorted by X (S); rebuilds a piecewise-linear profile spanning [Points[0].X, Points.Last().X]. */
	static TArray<FOpenDriveCubic> PointsToProfile(TArray<FVector2D> Points);
};
