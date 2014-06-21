/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2014 Adobe Systems Incorporated                       */
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

#include "Prefix.h"

//	Self
#include "PLTransitionItem.h"

// dva
#include "dvacore/utility/StringCompare.h"


namespace PL
{

namespace
{
	ASL::String kTransitionAlignmentString_StartBlack = ASL_STR("start-black");
	ASL::String kTransitionAlignmentString_EndBlack = ASL_STR("end-black");
	ASL::String kTransitionAlignmentString_Center = ASL_STR("center");
}

/*
**
*/
TransitionItem::TransitionItem(
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
	bool inReverse)
	:
	mStart(inStart),
	mEnd(inEnd),
	mFrameRate(inFrameRate),
	mCutPoint(inCutPoint),
	mTrackIndex(inTrackIndex),
	mEffectParam(
		inEffectID, 
		inMediaType, 
		inStartRatio, 
		inEndRatio, 
		inReverse)
{
	SetTransitionAlignmentType(inAlignmentTypeStr);
}

/*
**
*/
TransitionItem::~TransitionItem()
{

}

/*
**
*/
dvamediatypes::TickTime TransitionItem::GetStart() const
{
	return mStart;
}

/*
**
*/
dvamediatypes::TickTime TransitionItem::GetEnd() const
{
	return mEnd;
}

/*
**
*/
dvamediatypes::FrameRate TransitionItem::GetFrameRate() const
{
	return mFrameRate;
}

/*
**
*/
ASL::String TransitionItem::GetAlignmentTypeString() const
{
	switch(mAlignmentType)
	{
	case kTransitionAlignmentType_Start:
		return kTransitionAlignmentString_StartBlack;

	case kTransitionAlignmentType_End:
		return kTransitionAlignmentString_EndBlack;

	case kTransitionAlignmentType_Center:
		return kTransitionAlignmentString_Center;
	}

	DVA_ASSERT(0);
	return ASL_STR("");
}

/*
**
*/
void TransitionItem::SetTransitionAlignmentType(const ASL::String& inTypeString)
{
	if (dvacore::utility::StringEquals(inTypeString, kTransitionAlignmentString_StartBlack))
	{
		mAlignmentType = kTransitionAlignmentType_Start;
	}
	else if (dvacore::utility::StringEquals(inTypeString, kTransitionAlignmentString_EndBlack))
	{
		mAlignmentType = kTransitionAlignmentType_End;
	}
	else if (dvacore::utility::StringEquals(inTypeString, kTransitionAlignmentString_Center))
	{
		mAlignmentType = kTransitionAlignmentType_Center;
	}
	else
	{
		DVA_ASSERT(0);
		mAlignmentType = kTransitionAlignmentType_Center;
	}
}
/*
**
*/
dvamediatypes::TickTime TransitionItem::GetCutPoint() const
{
	return mCutPoint;
}

/*
**
*/
ASL::String TransitionItem::GetEffectMatchName() const
{
	return mEffectParam.mEffectMatchName;
}

/*
**
*/
BE::MediaType TransitionItem::GetMediaType() const
{
	return mEffectParam.mMediaType;
}

/*
**
*/
ASL::Float32 TransitionItem::GetEffectStartRatio() const
{
	return mEffectParam.mStartRatio;
}

/*
**
*/
ASL::Float32 TransitionItem::GetEffectEndRatio() const
{
	return mEffectParam.mEndRatio;
}

/*
**
*/
bool TransitionItem::IsEffectReverse() const
{
	return mEffectParam.mReverse;
}

/*
**
*/
BE::TrackIndex TransitionItem::GetTrackIndex() const
{
	return mTrackIndex;
}

/*
**
*/
PL::TrackTransitionMap CloneTrackTransitionMap(const PL::TrackTransitionMap& inTrackTransitionMap)
{
	PL::TrackTransitionMap newTransitions;
	for (PL::TrackTransitionMap::const_iterator mapIter = inTrackTransitionMap.begin();
		mapIter != inTrackTransitionMap.end();
		++mapIter)
	{
		PL::TransitionItemSet newTransitionItems;

		BE::TrackIndex trackIndex = (*mapIter).first;
		PL::TransitionItemSet transitionItems = (*mapIter).second;

		for (PL::TransitionItemSet::const_iterator iter = transitionItems.begin();
			iter != transitionItems.end();
			++iter)
		{
			PL::TransitionItemPtr theTransitionItem = *iter;

			PL::TransitionItemPtr newTransitionItem(new PL::TransitionItem(
				theTransitionItem->GetStart(),
				theTransitionItem->GetEnd(),
				theTransitionItem->GetFrameRate(),
				theTransitionItem->GetAlignmentTypeString(),
				theTransitionItem->GetCutPoint(),
				theTransitionItem->GetTrackIndex(),
				theTransitionItem->GetEffectMatchName(),
				theTransitionItem->GetMediaType(),
				theTransitionItem->GetEffectStartRatio(),
				theTransitionItem->GetEffectEndRatio(),
				theTransitionItem->IsEffectReverse()));

			newTransitionItems.insert(newTransitionItem);
		}

		newTransitions.insert(PL::TrackTransitionMap::value_type(trackIndex, newTransitionItems));
	}

	return newTransitions;
}

/*
**
*/
bool TrackTransitionsAreMatch(
	const PL::TrackTransitionMap& inLeftTransitions,
	const PL::TrackTransitionMap& inRightTransitions)
{
	if (inLeftTransitions.size() != inRightTransitions.size())
	{
		// track count not equal
		return false;
	}

	PL::TrackTransitionMap::const_iterator leftMapIter = inLeftTransitions.begin();
	PL::TrackTransitionMap::const_iterator leftMapEnd = inLeftTransitions.end();

	PL::TrackTransitionMap::const_iterator rightMapIter = inRightTransitions.begin();
	PL::TrackTransitionMap::const_iterator rightMapEnd = inRightTransitions.end();

	for (; leftMapIter != leftMapEnd && rightMapIter != rightMapEnd; ++leftMapIter, ++rightMapIter)
	{
		if ((*leftMapIter).first != (*rightMapIter).first)
		{
			// track idx not equal
			return false;
		}

		if ((*leftMapIter).second.size() != (*rightMapIter).second.size())
		{
			// transition count in current track not equal
			return false;
		}

		const PL::TransitionItemSet& leftTransitionSet = (*leftMapIter).second;
		const PL::TransitionItemSet& rightTransitionSet = (*rightMapIter).second;

		PL::TransitionItemSet::const_iterator leftSetIter = leftTransitionSet.begin();
		PL::TransitionItemSet::const_iterator leftSetEnd = leftTransitionSet.end();

		PL::TransitionItemSet::const_iterator rightSetIter = rightTransitionSet.begin();
		PL::TransitionItemSet::const_iterator rightSetEnd = rightTransitionSet.end();

		for (; leftSetIter != leftSetEnd && rightSetIter != rightSetEnd; ++leftSetIter, ++rightSetIter)
		{
			const PL::TransitionItemPtr& leftTransition = *leftSetIter;
			const PL::TransitionItemPtr& rightTransition = *rightSetIter;
			if (!dvacore::utility::StringEquals(leftTransition->GetEffectMatchName(), rightTransition->GetEffectMatchName()) ||
				leftTransition->GetStart() != rightTransition->GetStart() ||
				leftTransition->GetEnd() != rightTransition->GetEnd())
			{
				return false;
			}
		}
	}

	return true;
}

} // namespace PL
