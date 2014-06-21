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

#include "Prefix.h"

#include "BEClip.h"
#include "BESequence.h"
#include "BETrack.h"
#include "BEMasterClip.h"
#include "PLClipDurationMap.h"
#include "MZSequence.h"
#include "BEContent.h"
#include "BEMedia.h"
#include "MZBEUtilities.h"

namespace PL
{

ClipDurationMap::ClipDurationMap()
	: mLastOneStartTime(dvamediatypes::kTime_Invalid)
{

}

ClipDurationMap::~ClipDurationMap()
{

}

void ClipDurationMap::AddTrackItems(const BE::TrackItemVector& inTrackItems)
{
	for (BE::TrackItemVector::const_iterator trackIter = inTrackItems.begin(); 
		trackIter != inTrackItems.end(); ++trackIter)
	{
		BE::ITrackItemRef trackItem = *trackIter;

		if (trackItem->GetType() == BE::kTrackItemType_Clip)
		{
			BE::IChildClipRef childClip(BE::IClipTrackItemRef(trackItem)->GetChildClip());
			BE::IClipRef clipRef = childClip->GetClip();
			BE::IContentRef content(clipRef->GetContent());
			BE::IMediaContentRef mediaContent(content);

			TrackItemInfo info;
			info.mDuration = trackItem->GetDuration();
			info.mMasterClip = childClip->GetMasterClip();
			info.mClipInPoint = clipRef->GetInPoint();
			content->GetBoundaries(info.mContentBoundaryStart, info.mContentBoundaryEnd);
			info.mTrackItem = trackItem;
			//	If this is a still, don't use the arbitrary timecode that has been set, use 0.
			if (mediaContent)
			{
				BE::IMediaRef media(mediaContent->GetDeprecatedMedia());
				if (media)
				{
					BE::IVideoStreamRef videoStream(media->GetMediaStream(BE::kMediaType_Video));
					if (videoStream)
					{
						if (videoStream->IsStill())
						{
							info.mClipInPoint = dvamediatypes::kTime_Zero;
						}
					}
				}
			}

			dvamediatypes::TickTime itemStart = trackItem->GetStart();
			mClipDurationMap.insert(std::make_pair(trackItem->GetStart(), info));

			if (mLastOneStartTime == dvamediatypes::kTime_Invalid || 
				itemStart > mLastOneStartTime)
			{
				mLastOneStartTime = itemStart;
			}
		}
	}
}

void ClipDurationMap::SetSequence(
	BE::ISequenceRef inSequence)
{
	// Here's an assumption that if the clip has both video and audio track, they should have the same in point.
	mClipDurationMap.clear();
	mLastOneStartTime = dvamediatypes::kTime_Invalid;

	if (inSequence)
	{
		BE::TrackItemVector trackItems = MZ::BEUtilities::GetTrackItemsFromTheFirstAvailableTrack(inSequence);
		AddTrackItems(trackItems);
	}
}

BE::IMasterClipRef ClipDurationMap::GetClipStartAndDurationFromSequenceTime(
	const dvamediatypes::TickTime& inSequenceTime,
	dvamediatypes::TickTime& outClipStart,
	dvamediatypes::TickTime& outDuration,
	bool* outIsLastOne) const
{
	outClipStart = dvamediatypes::kTime_Zero;
	outDuration = dvamediatypes::kTime_Zero;
	BE::IMasterClipRef masterClip;
	if (!mClipDurationMap.empty())
	{
		ClipDurationInternalMap::const_iterator iter = mClipDurationMap.upper_bound(inSequenceTime);
		if (iter != mClipDurationMap.begin())
		{
			--iter;
		}
		outClipStart = iter->first;
		outDuration = iter->second.mDuration;
		masterClip = iter->second.mMasterClip;
	}
	if (outIsLastOne != NULL)
	{
		(*outIsLastOne) = (mLastOneStartTime == outClipStart);
	}
	return masterClip;
}

void ClipDurationMap::GetTrackItemInfoFromSequenceTime(
		const dvamediatypes::TickTime& inSequenceTime,
		TrackItemInfo& outTrackIteminfo) const
{
	if (!mClipDurationMap.empty())
	{
		ClipDurationInternalMap::const_iterator iter = mClipDurationMap.upper_bound(inSequenceTime);
		if (iter != mClipDurationMap.begin())
		{
			--iter;
		}
		outTrackIteminfo = iter->second;
	}
}


}