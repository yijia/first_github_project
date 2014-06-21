/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2010 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

#ifndef PLMARKER_H
#define PLMARKER_H

#ifndef __XMPTemporalMetadataServices_h__
#include "dvatemporalxmp/XMPTemporalMetadataServices.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef DVATEMPORALXMP_MARKER_H
#include "dvatemporalxmp/Marker.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLOBJECTPTR_H
#include "ASLObjectPtr.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef IPLMARKEROWNERFWD_H
#include "IPLMarkerOwnerFwd.h"
#endif

#ifndef BEPROPERTYKEY_H
#include "BEPropertyKey.h"
#endif

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef PLMARKERTYPEREGISTRY_H
#include "PLMarkerTypeRegistry.h"
#endif

#ifndef DVAUI_DRAWBOT_DRAWBOTTYPES_H
#include "dvaui/drawbot/DrawbotTypes.h"
#endif

#ifndef DVACORE_PROPLIST_PROPLIST_H
#include "dvacore/proplist/PropList.h"
#endif

#include "boost/shared_ptr.hpp"

namespace PL
{

/**
**	A bool property which if true means that if there is a selected marker, add a new
**	marker will set the out point on the previous marker to the in point of the new
**	marker. If false, the selected marker will be unchanged.
*/
PL_EXPORT extern const BE::PropertyKey kPropertyKey_CloseOnNewMarker;

/**
**	A bool property which if true means playback stops on marker addition.
*/
PL_EXPORT extern const BE::PropertyKey kPropertyKey_StopOnNewMarker;

/**
**  Property if true: when adding a new marker or double clicking on a marker, auto-switch focus
**  to the marker pane. Then switch back when done with the edit.
*/
PL_EXPORT extern const BE::PropertyKey kPropertyKey_SwitchFocusOnDoubleClickMarker;

PL_EXPORT extern const BE::PropertyKey kPropertyKey_SwitchUseTagColorMarker;

PL_EXPORT extern const BE::PropertyKey kPropertyKey_ShowRelatedPanelsOnOpenAsset;

PL_EXPORT extern const BE::PropertyKey kPropertyKey_SetMarkerDurationByTypeBase;

PL_EXPORT extern const BE::PropertyKey kPropertyKey_MarkerDurationByTypeBase;

PL_EXPORT extern const BE::PropertyKey kPropertyKey_MarkerPreroll;


class CottonwoodMarker;
class CottonwoodMarkerSort;

typedef boost::shared_ptr<CottonwoodMarker> CottonwoodMarkerPtr;
typedef std::vector<CottonwoodMarker> CottonwoodMarkerList;
typedef std::map<ASL::Guid, CottonwoodMarkerPtr, std::less<ASL::Guid>, dvacore::utility::SmallBlockAllocator::STLAllocator<std::map<ASL::Guid, CottonwoodMarkerPtr >::value_type> > CottonwoodMarkerMap;

#ifndef XMPText
	typedef boost::shared_ptr<ASL::StdString> XMPText;
#endif
    
// TrackInfo
struct TrackInfo
{
    dvacore::UTF16String trackName;
    dvamediatypes::FrameRate frameRate;
};
typedef std::map<dvacore::UTF16String, TrackInfo> TrackTypes;
    
// by start time
typedef std::multimap<dvamediatypes::TickTime, CottonwoodMarker> MarkerTrack;
// map from marker type to MarkerTrack
typedef std::map<dvacore::UTF16String, MarkerTrack> MarkerTracks;
typedef std::set<dvacore::UTF16String> MarkerTypeFilterList;

class TagParam
{
public:
	PL_EXPORT
	TagParam();

	PL_EXPORT
	TagParam(
		const ASL::String& inName,
		const ASL::String& inPayload,
		const dvaui::drawbot::ColorRGBA& inColor);

	// when we build tag from xmpKeyword, we should keep all data from it, such as guid
	PL_EXPORT
	TagParam(
		const dvatemporalxmp::XMPKeywordData& inXMPKeywordData);

	/**
	** We MUST need this kind of constructor because CWMarker struct is copied in every place and many times
	**	we don't need really create a new marker but only a data copy.
	*/
	PL_EXPORT
	TagParam(const TagParam& inTagParam, bool inMakeUniqueInstance);

	PL_EXPORT
	~TagParam();

	PL_EXPORT
	bool operator ==(const TagParam& inTagParam);

	PL_EXPORT
	ASL::String GetName() const;

	//PL_EXPORT
	//void SetName(const ASL::String& inName);

	PL_EXPORT
	ASL::String GetPayload() const;

	//PL_EXPORT
	//void SetPayload(const ASL::String& inPayload);

	PL_EXPORT
	dvaui::drawbot::ColorRGBA GetColor() const;

	//PL_EXPORT
	//void SetColor(const dvaui::drawbot::ColorRGBA& inColor);

	PL_EXPORT
	// we don't have setter for instance guid because it is for every tag instance and created in constructor
	ASL::Guid GetInstanceGuid() const;

	PL_EXPORT
	void SerializeOut(ASL::OutputStringStream& outStream);

	PL_EXPORT
	void SerializeIn(ASL::InputStringStream& inStream);

	PL_EXPORT
	void ToProperty(dvacore::proplist::PropList* ioTagListPropList) const;

	PL_EXPORT
	void FromProperty(ASL::Guid const& inGuid, dvacore::proplist::PropList* inTagPropList);

protected:

	/**
	** We will implement this if we need them, now we force caller to specify if make unique
	*/
	TagParam(const TagParam& inTagParam);

	TagParam& operator =(const TagParam& inTagParam);

private:
	ASL::Guid					mInstanceGuid;
	ASL::String					mName;
	ASL::String					mPayload;
	dvaui::drawbot::ColorRGBA	mColor;
};
typedef boost::shared_ptr<TagParam>	TagParamPtr;

/**
**	first:	tag index
**	second:	tag data
**
**	The new added tagParam should be appended to the end, with the incremental index from current last element
**	Key in the map may not be sequential but index of keyword in xmp is. For example:
**
**
**	4 tags: [1, tag1], [2, tag2], [3, tag3], [4, tag4], if user delete tag3, it becomes
**	[1, tag1], [2, tag2], [4, tag4].
**
**	When write to xmp, we make the index sequential: [1, tag1], [2, tag2], [3, tag4].
**
**	Next time I read the xmp to CWMarker, tags will be [1, tag1], [2, tag2], [3, tag4] as initial state
**
**	User may add or delete more tags then the key in the map become non-sequential again, it doesn't matter, 
**	we just keep one thing: the new added tag always has the largest key.
**	The index will be adjusted again when it is written to xmp.
*/
typedef std::map<std::uint64_t, TagParamPtr>	TagParamMap;
typedef ASL::ReferenceBrokerPtr SRMarkerOwnerWeakRef;
    
class CottonwoodMarker
{
    
public:
    PL_EXPORT
	CottonwoodMarker();
    
	PL_EXPORT
	CottonwoodMarker(SRMarkerOwnerWeakRef inMarkerOwner);

	PL_EXPORT
	CottonwoodMarker(
		const CottonwoodMarker& inMarker,
		bool inMakeUnique);

	PL_EXPORT
	~CottonwoodMarker();
		
	PL_EXPORT
	static void SerializeToString(
		const CottonwoodMarkerList& inMarkerList,
		dvacore::UTF16String& outString);

	PL_EXPORT
	static void SerializeFromString(
		const dvacore::UTF16String& inString,
		const dvamediatypes::TickTime& inCurrentCTI,
		CottonwoodMarkerList& outMarkerList);
	
	PL_EXPORT
	dvamediatypes::TickTime GetStartTime() const;

	PL_EXPORT
	dvamediatypes::TickTime GetDuration() const;
	
	PL_EXPORT
	dvacore::UTF16String GetType() const;
	
	PL_EXPORT
	dvacore::UTF16String GetName() const;

	/**
	** For some UIs, show marker name if it's non-empty, otherwise show the first tag's name
	*/
	PL_EXPORT
	dvacore::UTF16String GetNameOrTheFirstKeyword() const;

	PL_EXPORT
	dvacore::UTF16String GetComment() const;

	PL_EXPORT
	dvacore::UTF16String GetLocation() const;

	PL_EXPORT
	dvacore::UTF16String GetTarget() const;

	PL_EXPORT
	dvacore::UTF16String GetCuePointType() const;

	PL_EXPORT
	const dvatemporalxmp::CustomMarkerParamList& GetCuePointList() const;
	
	PL_EXPORT
	dvacore::UTF16String GetSpeaker() const;

	PL_EXPORT
	dvacore::UTF16String GetProbability() const;

	PL_EXPORT
	dvacore::UTF16String GetPayload() const;

	PL_EXPORT
	dvacore::UTF16String GetSummary() const;

	PL_EXPORT
	ASL::Guid GetGUID() const;

	PL_EXPORT
	void SetGUID(
		ASL::Guid inMarkerID);
	
	PL_EXPORT
	void SetStartTime(
		const dvamediatypes::TickTime& inTime);

	PL_EXPORT
	void SetDuration(
		const dvamediatypes::TickTime& inTime);

	PL_EXPORT
	void SetType(
		const dvacore::UTF16String& inType);

	PL_EXPORT
	void SetName(
		const dvacore::UTF16String& inName);

	PL_EXPORT
	void SetComment(
		const dvacore::UTF16String& inComment);

	PL_EXPORT
	void SetLocation(
		const dvacore::UTF16String& inLocation);

	PL_EXPORT
	void SetTarget(
		const dvacore::UTF16String& inTarget);

	PL_EXPORT
	void SetCuePointType(
		const dvacore::UTF16String& inCuePointType);

	PL_EXPORT
	void SetCuePointList(
		const dvatemporalxmp::CustomMarkerParamList& inCuePointList);

	PL_EXPORT
	void SetSpeaker(
		const dvacore::UTF16String& inSpeaker);

	PL_EXPORT
	void SetProbability(
		const dvacore::UTF16String& inProbability);


	PL_EXPORT
	CottonwoodMarker& operator = (const CottonwoodMarker& inRHS);

	PL_EXPORT
	bool operator ==(const CottonwoodMarker& inRHS) const;

	PL_EXPORT
	bool operator <(const CottonwoodMarker& inRHS) const;

	PL_EXPORT
	void AddTagParam(const TagParamPtr& inTagParam);

	PL_EXPORT
	void RemoveTagParamByInstanceGuid(const ASL::Guid& inInstanceGuid);

	PL_EXPORT
	bool TagsEqual(const CottonwoodMarker& inMarker) const;

	PL_EXPORT
	const TagParamMap& GetTagParams() const;

	PL_EXPORT
	dvacore::UTF8String ToPropertyXML() const;

	PL_EXPORT
	dvacore::UTF16String ToPropertyXML(dvamediatypes::FrameRate const& inFrameRate) const;

	PL_EXPORT
	bool FromPropertyXML(dvacore::UTF16String const& inPropertyXML, dvamediatypes::FrameRate const& inFrameRate);

	PL_EXPORT
	PL::ISRMarkerOwnerRef GetMarkerOwner() const;
    
    PL_EXPORT
	void SetMarkerOwner(PL::ISRMarkerOwnerRef inMarkerOwner);

	PL_EXPORT
	ASL::Color GetColor() const;

private:
	ASL::Guid						mGUID;
	dvamediatypes::TickTime			mStartTime;
	dvamediatypes::TickTime			mDuration;
	dvacore::UTF16String			mType;
	dvacore::UTF16String			mName;
	dvacore::UTF16String			mComment;
	dvacore::UTF16String			mLocation;
	dvacore::UTF16String			mTarget;
	dvacore::UTF16String			mCuePointType;
    dvatemporalxmp::CustomMarkerParamList	mCuePointList;
	dvacore::UTF16String			mSpeaker;
	dvacore::UTF16String			mProbability;
	TagParamMap						mTagParams;


    PL::SRMarkerOwnerWeakRef        mMarkerOwner;

	friend class CottonwoodMarkerSort;

};

class CottonwoodMarkerSort
{
public:

	bool operator() (
		const CottonwoodMarker& inLHS,
		const CottonwoodMarker& inRHS) const
	{
		return inLHS.mGUID < inRHS.mGUID;
	}
};

typedef std::set<CottonwoodMarker, CottonwoodMarkerSort> MarkerSet;


// used for updating a list of markers, the pair represents oldMaker vs. newMarker
typedef std::pair<PL::CottonwoodMarker, PL::CottonwoodMarker> MarkerToUpdatePair;
typedef std::vector<MarkerToUpdatePair> MarkerToUpdatePairList;

inline CottonwoodMarkerPtr CreateInOutMarker(
	const ASL::Guid& inMarkerID,
	const ASL::String& inMarkerName,
	const ASL::String& inMarkerComment,
	const dvamediatypes::TickTime& inStartTime,
	const dvamediatypes::TickTime& inDuration)
{
	CottonwoodMarkerPtr newMarker(new CottonwoodMarker(SRMarkerOwnerWeakRef()));
	newMarker->SetType(PL::MarkerType::kInOutMarkerType);
	newMarker->SetGUID(inMarkerID);
	newMarker->SetName(inMarkerName);
	newMarker->SetComment(inMarkerComment);
	newMarker->SetStartTime(inStartTime);
	newMarker->SetDuration(inDuration);
	return newMarker;
}

PL_EXPORT
bool IsMarkerFilteredByString(CottonwoodMarker const& inMarker, dvacore::UTF16String const& inFilterText);

PL_EXPORT
PL::CottonwoodMarkerList BuildMarkersFromSelection(const PL::MarkerSet & inMarkerSelection);
}

#endif
