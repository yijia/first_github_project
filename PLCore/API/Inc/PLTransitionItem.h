/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2014 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

#pragma once

#ifndef PLTRANSITIONITEM_H
#define PLTRANSITIONITEM_H

//	PL
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

//	ASL
#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

//	BE
#ifndef BE_MEDIATYPES_H
#include "BEMediaType.h"
#endif

#ifndef BE_SEQUENCE_TYPES_H
#include "BE/Sequence/Types.h"
#endif

// dva
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif


namespace PL
{

/*
**
*/
enum TransitionAlignmentType
{
	kTransitionAlignmentType_Start,
	kTransitionAlignmentType_End,
	kTransitionAlignmentType_Center
};

/*
**
*/
struct EffectData
{
	EffectData()
	:
	mMediaType(BE::kMediaType_Any),
	mStartRatio(0),
	mEndRatio(0),
	mReverse(false)
	{
	}

	EffectData(
		const ASL::String& inEffectID,
		BE::MediaType inMediaType,
		ASL::Float32 inStartRatio,
		ASL::Float32 inEndRatio,
		bool inReverse)
		:
		mEffectMatchName(inEffectID),
		mMediaType(inMediaType),
		mStartRatio(inStartRatio),
		mEndRatio(inEndRatio),
		mReverse(inReverse)
	{
	}

	ASL::String		mEffectMatchName;
	BE::MediaType	mMediaType;
	ASL::Float32	mStartRatio;
	ASL::Float32	mEndRatio;
	bool			mReverse;
};

/*
**
*/
class TransitionItem
{
public:

	/*
	**
	*/
	PL_EXPORT
	TransitionItem(
		const dvamediatypes::TickTime& inStart,
		const dvamediatypes::TickTime& inEnd,
		const dvamediatypes::FrameRate& inFrameRate,
		const ASL::String& inAlignmentTypeStr,
		const dvamediatypes::TickTime& inCutPoint,
		BE::TrackIndex inTrackIndex,
		const ASL::String& inEffectID,
		BE::MediaType inMediaType,
		ASL::Float32 inStartRatio,
		ASL::Float32 inEndRatio,
		bool inReverse);

	/*
	**
	*/
	PL_EXPORT
	~TransitionItem();

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetStart() const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetEnd() const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::FrameRate GetFrameRate() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetAlignmentTypeString() const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetCutPoint() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetEffectMatchName() const;

	/*
	**
	*/
	PL_EXPORT
	BE::MediaType GetMediaType() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::Float32 GetEffectStartRatio() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::Float32 GetEffectEndRatio() const;

	/*
	**
	*/
	PL_EXPORT
	bool IsEffectReverse() const;

	/*
	**
	*/
	PL_EXPORT
	BE::TrackIndex GetTrackIndex() const;

	/*
	** no export 
	*/
	void SetTransitionAlignmentType(const ASL::String& inTypeString);

private:
	dvamediatypes::TickTime		mStart;
	dvamediatypes::TickTime		mEnd;
	dvamediatypes::FrameRate	mFrameRate;

	TransitionAlignmentType		mAlignmentType;
	dvamediatypes::TickTime		mCutPoint;

	EffectData					mEffectParam;
	BE::TrackIndex				mTrackIndex;
};

typedef boost::shared_ptr<TransitionItem>	TransitionItemPtr;


class TransitionItemCompare 
{
public:
	bool operator()(const TransitionItemPtr& left, const TransitionItemPtr& right) const
	{
		return (left->GetStart() < right->GetStart());
	}
	bool operator()(const dvamediatypes::TickTime& left, const TransitionItemPtr& right) const
	{
		return (left < right->GetStart());
	}
	bool operator()(const TransitionItemPtr& left, const dvamediatypes::TickTime& right) const
	{
		return (left->GetStart() < right);
	}
};

typedef std::set<TransitionItemPtr, TransitionItemCompare>		TransitionItemSet;
typedef std::map<BE::TrackIndex, PL::TransitionItemSet>			TrackTransitionMap;


/*
**
*/
PL_EXPORT
PL::TrackTransitionMap CloneTrackTransitionMap(const PL::TrackTransitionMap& inTrackTransitionMap);

/*
**
*/
PL_EXPORT
bool TrackTransitionsAreMatch(
	const PL::TrackTransitionMap& inLeftTransitions,
	const PL::TrackTransitionMap& inRightTransitions);

}

#endif
