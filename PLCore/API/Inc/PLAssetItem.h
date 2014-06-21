/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2011 Adobe Systems Incorporated
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

#ifndef PLASSETITEM_H
#define PLASSETITEM_H

//	MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

#ifndef PLTRANSITIONITEM_H
#include "PLTransitionItem.h"
#endif

//	ASL
#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef ASLDATE_H
#include "ASLDate.h"
#endif

//	dva
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

namespace PL
{

	class AssetItem;
	typedef boost::shared_ptr<AssetItem> AssetItemPtr;
	typedef std::list<AssetItemPtr> AssetItemList;

	/*
	**
	*/
	class AssetItem
	{
	public:

		/*
		**
		*/
		PL_EXPORT
		AssetItem(
			AssetMediaType inType,
			ASL::String const& inMediaPath,
			ASL::Guid	const& inGUID,
			ASL::Guid   const& inParentGUID,
			ASL::Guid	const& inAssetMediaInfoGUID,
			ASL::String const& inAssetName,
			ASL::String const& inAssetMetadata = ASL::String(),
			dvamediatypes::TickTime	const& inPoint = dvamediatypes::kTime_Invalid,
			dvamediatypes::TickTime	const& inDuration = dvamediatypes::kTime_Invalid,
			dvamediatypes::TickTime	const& inCustomInPoint = dvamediatypes::kTime_Invalid,
			dvamediatypes::TickTime	const& inCustomOutPoint = dvamediatypes::kTime_Invalid,
			ASL::Guid	const& inClipItemGuid = ASL::Guid(),
			ASL::Guid   const& inMarkerGuid = ASL::Guid(),
			ASL::String const& inRelativePath = ASL::String());


		/*
		**
		*/
		PL_EXPORT
		~AssetItem();

		/*
		**
		*/
		PL_EXPORT
		AssetMediaType GetAssetMediaType() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetMediaPath() const;

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetInPoint() const;

		/*
		**
		*/
		PL_EXPORT
		void SetInPoint(const dvamediatypes::TickTime& inInPoint);

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetDuration() const;

		/*
		**
		*/
		PL_EXPORT
		void SetDuration(const dvamediatypes::TickTime& inDuration);

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetAssetName() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetAssetMetadata() const;

		/*
		**
		*/
		PL_EXPORT
		void SetAssetMetadata(const ASL::String& inMetadata);

		/*
		**
		*/
		PL_EXPORT
		ASL::Guid GetAssetGUID() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::Guid GetParentGUID() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::Guid GetAssetMediaInfoGUID() const;

		/*
		**
		*/
		PL_EXPORT
		void SetSubAssetItemList(AssetItemList const& inSubAssetItemList);

		/*
		**
		*/
		PL_EXPORT
		AssetItemList const& GetSubAssetItemList() const;

		/*
		**
		*/
		PL_EXPORT
		static bool IsAssetItemSamePath(
				AssetItemPtr const& inLeftAssetItem,
				AssetItemPtr const& inRightAssetItem);

		/*
		**
		*/
		PL_EXPORT
		static bool IsAssetItemSameGuid(
				AssetItemPtr const& inLeftAssetItem,
				AssetItemPtr const& inRightAssetItem);

		/*
		**
		*/
		PL_EXPORT
		ASL::Guid GetAssetClipItemGuid() const;

		PL_EXPORT
		ASL::Guid GetAssetMarkerGuid() const;

		PL_EXPORT
		ASL::String GetParentBinPath() const;

		/*
		**
		*/
		PL_EXPORT
		void SetAssetName(ASL::String const& inNewName);

		/*
		**
		*/
		PL_EXPORT
		void SetAssetMediaInfoGUID(ASL::Guid const& inMediaInfoID);

		/*
		**
		*/
		PL_EXPORT
		void SetCustomInPoint(const dvamediatypes::TickTime& inPoint);

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetCustomInPoint() const;

		/*
		**
		*/
		PL_EXPORT
		void SetCustomOutPoint(const dvamediatypes::TickTime& inCustomOutPoint);

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetCustomOutPoint() const;

		/*
		**
		*/
		PL_EXPORT
		void AddTransitionItem(
			BE::MediaType inType,
			BE::TrackIndex inTrackIndex,
			const TransitionItemPtr& inTransitionItem);

		/*
		**
		*/
		PL_EXPORT
		void SetTrackTransitions(
			const PL::TrackTransitionMap& inTrackTransitions,
			BE::MediaType inType);

		/*
		**
		*/
		PL_EXPORT
		PL::TrackTransitionMap GetTrackTransitions(BE::MediaType inType) const;


	private:
		void AddTransitionItem(
			TrackTransitionMap& ioTransitionMap,
			BE::TrackIndex inTrackIndex,
			const TransitionItemPtr& inTransitionItem);

	private:
		//	AssetItem type
		AssetMediaType			mType;

		//	AssetItem GUID
		ASL::Guid				mGuid;

		//	AssetItem Name
		ASL::String				mAssetName;

		//	link to primary clip
		ASL::Guid				mParentGuid;

		//	link to AssetMediaInfo 
		//	Only used for judging if coming AssetItems share same AssetMediaInfo
		ASL::Guid				mAssetMediaInfoGUID;

		//	[TODO] This class member should be renamed, but we haven't got a proper name for it yet.
		//	Currently, it is responsible for creating/getting BE::ProjectItem and used as key in AssetMediaInfoMap
		//	[TODO] We should use mAssetMediaInfoGUID for finding AssetMediaInfo.
		ASL::String				mMediaPath;

		//	In Point for this Item
		dvamediatypes::TickTime	mInPoint;

		//	Duration for this Item
		dvamediatypes::TickTime	mDuration;

		//	AssetMetadata
		ASL::String				mAssetMetadata;

		//	subAssetItemList for RC
		AssetItemList			mSubAssetItemList;

		//	transitions for RC
		TrackTransitionMap		mVideoTransitions;
		TrackTransitionMap		mAudioTransitions;


		//  Asset clip GUID for RC, which should only be used in core side and be persisted in RC XML file.
		//	Always been created from core side when AppendAssetItem/insertAssetItem/D&DAssetItem been called.
		ASL::Guid				mClipItemGuid;

		//  Asset marker GUID for subclip
		ASL::Guid				mMarkerGuid;

		// Parent bin information
		ASL::String				mParentBinPath;

		// Custom In Point 
		dvamediatypes::TickTime	mCustomInPoint;

		// Custom Out Point
		dvamediatypes::TickTime	mCustomOutPoint;
	};
}

#endif
