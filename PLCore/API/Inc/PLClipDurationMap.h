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

#ifndef PLCLIPDURATIONMAP_H
#define PLCLIPDURATIONMAP_H

#ifndef BESEQUENCEFWD_H
#include "BESequenceFwd.h"
#endif

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef BE_SEQUENCE_ITRACKITEMFWD_H
#include "BE/Sequence/ITrackItem.h"
#endif

namespace PL
{

class ClipDurationMap
{
public:

	struct TrackItemInfo
	{
		dvamediatypes::TickTime mClipInPoint;
		dvamediatypes::TickTime mContentBoundaryStart;
		dvamediatypes::TickTime mContentBoundaryEnd;
		dvamediatypes::TickTime mDuration;
		BE::IMasterClipRef mMasterClip;
		BE::ITrackItemRef  mTrackItem;	  

		TrackItemInfo()
			:
		mClipInPoint(dvamediatypes::kTime_Zero),
		mContentBoundaryStart(dvamediatypes::kTime_Zero),
		mContentBoundaryEnd(dvamediatypes::kTime_Max),
		mDuration(dvamediatypes::kTime_Max)
		{

		}
	};


	PL_EXPORT	
	ClipDurationMap();
	
	PL_EXPORT	
	~ClipDurationMap();
	
	PL_EXPORT	
	void SetSequence(
		BE::ISequenceRef inSequence);

	PL_EXPORT	
	BE::IMasterClipRef GetClipStartAndDurationFromSequenceTime(
		const dvamediatypes::TickTime& inSequenceTime,
		dvamediatypes::TickTime& outClipStart,
		dvamediatypes::TickTime& outDuration,
		bool* outIsLastOne = NULL)const;

	PL_EXPORT	
	void GetTrackItemInfoFromSequenceTime(
		const dvamediatypes::TickTime& inSequenceTime,
		TrackItemInfo& outTrackIteminfo) const;

private:
	void AddTrackItems(const BE::TrackItemVector& inTrackItems);
	
private:

	typedef std::map<dvamediatypes::TickTime, TrackItemInfo> ClipDurationInternalMap;
	ClipDurationInternalMap		mClipDurationMap;
	dvamediatypes::TickTime		mLastOneStartTime;
};

}

#endif