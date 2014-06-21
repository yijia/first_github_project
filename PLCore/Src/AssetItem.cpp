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

//	Self
#include "PLAssetItem.h"

//	ASL
#include "ASLStringCompare.h"

namespace PL
{

/*
**
*/
AssetItem::AssetItem(
				AssetMediaType	   inType,
				ASL::String const& inMediaPath,
				ASL::Guid	const& inGUID,
				ASL::Guid   const& inParentGUID,
				ASL::Guid	const& inAssetMediaInfoGUID,
				ASL::String const& inAssetName,
				ASL::String const& inAssetMetadata,
				dvamediatypes::TickTime	const& InPoint,
				dvamediatypes::TickTime	const& inDuration,
				dvamediatypes::TickTime	const& inCustomInPoint,
				dvamediatypes::TickTime	const& inCustomOutPoint,
				ASL::Guid	const& inClipItemGuid,
				ASL::Guid	const& inMarkerGuid,
				ASL::String const& inParentBinPath)
				:
				mType(inType),
				mMediaPath(inMediaPath),
				mGuid(inGUID),
				mParentGuid(inParentGUID),
				mAssetMediaInfoGUID(inAssetMediaInfoGUID),
				mInPoint(InPoint),
				mDuration(inDuration),
				mCustomInPoint(inCustomInPoint),
				mCustomOutPoint(inCustomOutPoint),
				mAssetName(inAssetName),
				mClipItemGuid(inClipItemGuid),
				mMarkerGuid(inMarkerGuid),
				mParentBinPath(inParentBinPath)
{

}

/*
**
*/
AssetItem::~AssetItem()
{

}

/*
**
*/
AssetMediaType AssetItem::GetAssetMediaType() const
{
	return mType;
}

/*
**
*/
ASL::String AssetItem::GetMediaPath() const
{
	return mMediaPath;
}

/*
**
*/
dvamediatypes::TickTime AssetItem::GetInPoint() const
{
	return mInPoint;
}

void AssetItem::SetInPoint(const dvamediatypes::TickTime& inInPoint)
{
	mInPoint = inInPoint;
}

/*
**
*/
dvamediatypes::TickTime AssetItem::GetDuration() const
{
	return mDuration;
}

void AssetItem::SetDuration(const dvamediatypes::TickTime& inDuration)
{
	mDuration = inDuration;
}

/*
**
*/
ASL::String AssetItem::GetAssetMetadata() const
{
	return mAssetMetadata;
}

void AssetItem::SetAssetMetadata(const ASL::String& inMetadata)
{
	mAssetMetadata = inMetadata;
}


/*
**
*/
ASL::String AssetItem::GetAssetName() const
{
	return mAssetName;
}

void AssetItem::SetAssetName(ASL::String const& inNewName)
{
	mAssetName = inNewName;
}

void AssetItem::SetAssetMediaInfoGUID(ASL::Guid const& inMediaInfoID)
{
	mAssetMediaInfoGUID = inMediaInfoID;
}

/*
**
*/
ASL::Guid AssetItem::GetAssetGUID() const
{
	return mGuid;
}

/*
**
*/
ASL::Guid AssetItem::GetParentGUID() const
{
	return mParentGuid;
}

/*
**
*/
ASL::Guid AssetItem::GetAssetMediaInfoGUID() const
{
	return mAssetMediaInfoGUID;
}

/*
**
*/
void AssetItem::SetSubAssetItemList(AssetItemList const& inSubAssetItemList)
{
	mSubAssetItemList = inSubAssetItemList;
}

/*
**
*/
AssetItemList const& AssetItem::GetSubAssetItemList() const
{
	return mSubAssetItemList;
}

/*
**
*/
bool AssetItem::IsAssetItemSamePath(
				AssetItemPtr const& inLeftAssetItem,
				AssetItemPtr const& inRightAssetItem)
{
	bool isEqual(true);
	if (inLeftAssetItem != inRightAssetItem)
	{
		ASL::String leftMediaPath; 
		ASL::String rightMediaPath; 

		if (inLeftAssetItem)
		{
			leftMediaPath = inLeftAssetItem->GetMediaPath();
		}

		if (inRightAssetItem)
		{
			rightMediaPath = inRightAssetItem->GetMediaPath();
		}

		isEqual =  ASL::CaseInsensitive::StringEquals(leftMediaPath, rightMediaPath);
	}

	return isEqual;
}

/*
**
*/
bool  AssetItem::IsAssetItemSameGuid(
				AssetItemPtr const& inLeftAssetItem,
				AssetItemPtr const& inRightAssetItem)
{
	ASL_ASSERT(inLeftAssetItem != NULL && inRightAssetItem != NULL);
	return inLeftAssetItem->GetAssetGUID() == inRightAssetItem->GetAssetGUID();
}

/*
**
*/
ASL::Guid AssetItem::GetAssetClipItemGuid() const
{
	return mClipItemGuid;
}

/*
**
*/
ASL::Guid AssetItem::GetAssetMarkerGuid() const
{
	return mMarkerGuid;
}

/*
** The returned value has been encoded by PL::Utilities::EncodeNameCell(),
** So, we should decode the returned value with PL::Utilities::DecodeNameCell() at last.
*/
ASL::String AssetItem::GetParentBinPath() const
{
	return mParentBinPath;
}

/*
**
*/
void AssetItem::SetCustomInPoint(const dvamediatypes::TickTime& inPoint)
{
	mCustomInPoint = inPoint;
}

/*
**
*/
dvamediatypes::TickTime AssetItem::GetCustomInPoint() const
{
	return mCustomInPoint;
}

/*
**
*/
void AssetItem::SetCustomOutPoint(const dvamediatypes::TickTime& inOutPoint)
{
	mCustomOutPoint = inOutPoint;
}

/*
**
*/
dvamediatypes::TickTime AssetItem::GetCustomOutPoint() const
{
	return mCustomOutPoint;
}

/*
**
*/
void AssetItem::AddTransitionItem(
	BE::MediaType inType, 
	BE::TrackIndex inTrackIndex, 
	const TransitionItemPtr& inTransitionItem)
{
	if (inType != BE::kMediaType_Audio && inType != BE::kMediaType_Video)
	{
		DVA_ASSERT_MSG(0, "invalid media type");
		return;
	}

	if (inType == BE::kMediaType_Video)
	{
		AddTransitionItem(mVideoTransitions, inTrackIndex, inTransitionItem);
	}
	else
	{
		AddTransitionItem(mAudioTransitions, inTrackIndex, inTransitionItem);
	}
}

/*
**
*/
void AssetItem::AddTransitionItem(
	TrackTransitionMap& ioTransitionMap, 
	BE::TrackIndex inTrackIndex, 
	const TransitionItemPtr& inTransitionItem)
{
	TrackTransitionMap::iterator iter = ioTransitionMap.find(inTrackIndex);
	if (iter == ioTransitionMap.end())
	{
		TransitionItemSet transitionItems;
		transitionItems.insert(inTransitionItem);
		ioTransitionMap.insert(TrackTransitionMap::value_type(inTrackIndex, transitionItems));
	}
	else
	{
		(*iter).second.insert(inTransitionItem);
	}
}

/*
**
*/
void AssetItem::SetTrackTransitions(
	const PL::TrackTransitionMap& inTrackTransitions,
	BE::MediaType inType)
{
	if (inType != BE::kMediaType_Audio && inType != BE::kMediaType_Video)
	{
		DVA_ASSERT_MSG(0, "invalid media type");
		return;
	}
	
	if (inType == BE::kMediaType_Video)
	{
		mVideoTransitions = inTrackTransitions;
	}
	else
	{
		mAudioTransitions = inTrackTransitions;
	}
}

/*
**
*/
PL::TrackTransitionMap AssetItem::GetTrackTransitions(BE::MediaType inType) const
{
	if (inType != BE::kMediaType_Audio && inType != BE::kMediaType_Video)
	{
		DVA_ASSERT_MSG(0, "invalid media type");
		return PL::TrackTransitionMap();
	}

	return (inType == BE::kMediaType_Video) ? mVideoTransitions : mAudioTransitions;
}

} // namespace PL
