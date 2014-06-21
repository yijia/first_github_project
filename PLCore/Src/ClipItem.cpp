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

//	MZ
#include "PLProject.h"
#include "PLClipItem.h"
#include "MZBEUtilities.h"
#include "MZMedia.h"
#include "PLFactory.h"
#include "MZProject.h"
#include "MZAction.h"

//  PL
#include "PLMedia.h"

//	ASL
#include "ASLPathUtils.h"

//	DVA
#include "dvamediatypes/TickTime.h"

//	BE
#include "BE/Sequence/IClipTrackItem.h"
#include "BE/Clip/IChildClip.h"
#include "BE/Clip/IClip.h"
#include "BE/Content/IContent.h"

namespace PL
{

/*
**
*/
SRClipItem::SRClipItem(PL::AssetItemPtr const& inAssetItem)
	:
	mAssetItem(inAssetItem)
{
	mSRMedia = SRProject::GetInstance()->GetSRMedia(mAssetItem->GetMediaPath());
	if (mSRMedia == NULL)
	{
		mSRMedia = SRMedia::Create(mAssetItem->GetMediaPath());
		SRProject::GetInstance()->AddSRMedia(mSRMedia);
	}
	else if(!mSRMedia->IsDirty())
	{
		PL::AssetMediaInfoPtr assetMediaInfo 
					= SRProject::GetInstance()->GetAssetMediaInfo(mAssetItem->GetMediaPath());
		ISRMarkerOwnerRef(mSRMedia)->GetMarkers()->BuildMarkersFromXMPString(assetMediaInfo->GetXMPString(), false);
	}

	ASL_ASSERT(mSRMedia != NULL);
}

/*
**
*/
SRClipItem::SRClipItem(ISRMediaRef inSRMedia, AssetItemPtr const& inAssetItem)
	: 
	mSRMedia(inSRMedia),
	mAssetItem(inAssetItem)
{
	ASL_ASSERT(mSRMedia != NULL && mAssetItem != NULL);
}

/*
**
*/
SRClipItem::~SRClipItem()
{
}

/*
**
*/
dvamediatypes::TickTime SRClipItem::GetStartTime(bool ignoreHardBoundaries) const
{
	dvamediatypes::TickTime inPoint = mAssetItem->GetInPoint();
	inPoint = (inPoint != dvamediatypes::kTime_Invalid) ? 
				inPoint : 
				dvamediatypes::kTime_Zero;

	if (ignoreHardBoundaries && mTrackItem)
	{
		dvamediatypes::TickTime boundaryStart = dvamediatypes::kTime_Zero;
		dvamediatypes::TickTime boundaryEnd = dvamediatypes::kTime_Zero;
		BE::IClipTrackItemRef clipTrackItem(mTrackItem);
		BE::IChildClipRef childClip(clipTrackItem->GetChildClip());
		childClip->GetClip()->GetContent()->GetBoundaries(boundaryStart, boundaryEnd);

		inPoint += boundaryStart;
	}

	return inPoint;
}

/*
**
*/
dvamediatypes::TickTime SRClipItem::GetDuration() const
{
	const dvamediatypes::TickTime duration = mAssetItem->GetDuration();
	BE::IMasterClipRef masterClip = GetMasterClip();
	return (duration != dvamediatypes::kTime_Invalid)
				? duration
				: MZ::BEUtilities::IsMasterClipStill(masterClip)
					? masterClip->GetMaxTrimmedDuration(BE::kMediaType_Any)
					: masterClip->GetMaxCurrentUntrimmedDuration(BE::kMediaType_Any);
}

/*
**
*/
ISRMediaRef SRClipItem::GetSRMedia() const
{
	return mSRMedia;
}
	
/*
**
*/	
BE::IMasterClipRef SRClipItem::GetMasterClip() const
{
	ASL_ASSERT(mSRMedia != NULL);
	return mSRMedia != NULL ? mSRMedia->GetMasterClip() : BE::IMasterClipRef();
}
	
/*
**
*/
PL::AssetItemPtr SRClipItem::GetAssetItem() const
{
	return mAssetItem;
}
	
/*
**
*/
ASL::Guid SRClipItem::GetSubClipGUID() const
{
	return mGUID;
}

/*
**
*/
void SRClipItem::SetSubClipGUID(ASL::Guid inGuid)
{
	mGUID = inGuid;
}

/*
**
*/
ASL::String SRClipItem::GetSubClipName() const
{
	return mAssetItem != NULL ? mAssetItem->GetAssetName() : ASL::String();
}

/*
**
*/
BE::ITrackItemRef SRClipItem::GetTrackItem() const
{
	return mTrackItem;
}

/*
**
*/
void SRClipItem::SetTrackItem(BE::ITrackItemRef inTrackItem)
{
	mTrackItem = inTrackItem;
}

/*
**
*/
ASL::String SRClipItem::GetOriginalClipPath() const
{
	return mAssetItem->GetMediaPath();
}

}
