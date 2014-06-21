/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2011 Adobe Systems Incorporated                       */
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

#pragma once

#ifndef PLMESSAGESTRUCT_H
#define PLMESSAGESTRUCT_H

// PL
#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

// ASL
#ifndef ASLRESULTS_H
#include "ASLResults.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef DVAMEDIATYPES_FRAMERATE_H
#include "dvamediatypes/FrameRate.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

// dva
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

//	boost
#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

namespace PL
{
#ifndef XMPText
	typedef boost::shared_ptr<ASL::StdString> XMPText;
#endif

struct ThumbnailRequest
{
	ASL::Guid					mAssetGuid;
	ASL::String					mClipPath;
	ASL::String					mThumbnailPath;
	dvamediatypes::TickTime		mStartTime;
	ThumbnailRequest() : mStartTime(dvamediatypes::kTime_Invalid) {};
};
typedef std::list<ASL::Guid> TaskIDList;

struct ActionResult
{
	ASL::Guid					mAssetGuid;
	ASL::String					mFilePath;
	ASL::Result					mResult;
	ASL::String					mErrorMsg;
	ASL::String					mCreateTime;
	ASL::String					mLastModifyTime;
	ActionResult(ASL::Guid const& inAssetId, 
				ASL::String const& inFilePath, 
				ASL::Result const& inResult = ASL::ResultFlags::kResultTypeFailure,
				ASL::String const& inErrorMsg = ASL::String(),
				ASL::String const& inCreateTime = ASL::String(),
				ASL::String const& inModifyTime = ASL::String()) 
				: mAssetGuid(inAssetId), mFilePath(inFilePath), mResult(inResult), mErrorMsg(inErrorMsg),
				mCreateTime(inCreateTime), mLastModifyTime(inModifyTime)
	{};
};
typedef boost::shared_ptr<ActionResult> ActionResultPtr;
typedef std::list<ActionResultPtr> ActionResultList;

struct FileInfo
{
	ASL::Guid					mAssetGuid;
	ASL::String					mFilePath;
	ASL::String					mLastModifyTime;
	FileInfo(ASL::Guid const& inAssetId,
			ASL::String const& inFilePath,
			ASL::String const& inModifyTime)
			: mAssetGuid(inAssetId), mFilePath(inFilePath), mLastModifyTime(inModifyTime) {};
};
typedef boost::shared_ptr<FileInfo> FileInfoPtr;
typedef std::list<FileInfoPtr> FileInfoList;

struct IngestItem
{
	ASL::String					mFilePath;
	ASL::String					mAliasName;
	ASL::String					mItemFormat;
	ASL::PathnameList			mExtraPathList;
	dvamediatypes::TickTime		mInPoint;
	dvamediatypes::TickTime		mOutPoint;
	dvamediatypes::FrameRate	mFrameRate;

	IngestItem(ASL::String const& inFilePath,
				ASL::String const& inAliasName = DVA_STR(""),
				ASL::String const& inItemFormat = DVA_STR(""),
				dvamediatypes::TickTime const& inInPoint = dvamediatypes::kTime_Invalid,
				dvamediatypes::TickTime const& inOutPoint = dvamediatypes::kTime_Invalid,
				dvamediatypes::FrameRate const& inFrameRate = dvamediatypes::kFrameRate_Zero)
				: 
	mFilePath(inFilePath), 
	mAliasName(inAliasName), 
	mItemFormat(inItemFormat),
	mInPoint(inInPoint),
	mOutPoint(inOutPoint),
	mFrameRate(inFrameRate)
	{};
};
typedef boost::shared_ptr<IngestItem> IngestItemPtr;
typedef std::list<IngestItemPtr> IngestItemList;


typedef std::set<ASL::Guid> MarkerGuidSet;
/*
**	MetadataID have to be unique
**		In local mode and CCM it is file path
**		In EA mode, it is AssetName+AssetID
**	MetadataPath is a readable path to display in UI, it may equal to ID, but we should not assume that
**		In local mode,it is file path
**		In CCM, it is the relative path on the cloud
**		In EA, it is /ProductionName/AssetName
**	AliasName is the name shown to user
**		It may be the file name or others but does not depend on which current mode is.
**
**	"mIncludedMarkerIDs" will be considered only if "mIsApplyAll" is false.
*/
struct UnassociatedMetadata
{
	UnassociatedMetadata(const ASL::String& inMetadataId, const ASL::String& inMetadataPath, XMPText inXMP, const ASL::String& inAliasName)
		: 
		mMetadataId(inMetadataId), 
		mMetadataPath(inMetadataPath),
		mAliasName(inAliasName), 
		mXMP(inXMP) 
		{ mIsApplyAll = true; };

	ASL::String					mMetadataId;
	ASL::String					mMetadataPath;
	ASL::String					mAliasName;
	XMPText						mXMP;

	bool						mIsApplyAll;
	MarkerGuidSet				mIncludedMarkerIDs;
};
typedef boost::shared_ptr<UnassociatedMetadata> UnassociatedMetadataPtr;
typedef std::list<UnassociatedMetadataPtr> UnassociatedMetadataList;

enum ApplyUnassociatedMetadataPositionType
{
	kApplyUnassociatedMetadataPosition_Undefine = 0,
	kApplyUnassociatedMetadataPosition_Marker,
	kApplyUnassociatedMetadataPosition_Player
};

struct FTPSettingsInfo
{
	FTPSettingsInfo()
		:
		mPort(0)
	{

	}

	FTPSettingsInfo(
		ASL::String const& inServerName,
		ASL::UInt32 inPort,
		ASL::String const& inUserName,
		ASL::String const& inPassword)
		:
		mServerName(inServerName),
		mPort(inPort),
		mUserName(inUserName),
		mPassword(inPassword)
		{

		}

	ASL::String		mServerName;
	ASL::UInt32		mPort;
	ASL::String		mUserName;
	ASL::String		mPassword;
};

struct TransferItem
{
	ASL::String mSrcPath;
	ASL::String mDstPath;
	bool mIsDstParent;
	TransferItem(const ASL::String& inSrcPath, const ASL::String& inDstPath, bool inIsDstParent)
		: mSrcPath(inSrcPath), mDstPath(inDstPath), mIsDstParent(inIsDstParent) {};
};
typedef boost::shared_ptr<TransferItem> TransferItemPtr;
typedef std::vector<TransferItemPtr> TransferItemList;

struct ConcatenationItem
{
	ASL::String mFilePath;
	dvamediatypes::FrameRate	mFrameRate;
	dvamediatypes::TickTime		mStartTime;
	dvamediatypes::TickTime		mDuration;

	ConcatenationItem(
		const ASL::String& inFilePath, 
		const dvamediatypes::FrameRate	inFrameRate = dvamediatypes::kFrameRate_Zero, 
		const dvamediatypes::TickTime	inStartTime = dvamediatypes::kTime_Invalid,
		const dvamediatypes::TickTime	inDuration = dvamediatypes::kTime_Invalid)
		: 
	mFilePath(inFilePath), 
	mFrameRate(inFrameRate),
	mStartTime(inStartTime), 
	mDuration(inDuration) 
	{};
};
typedef boost::shared_ptr<ConcatenationItem> ConcatenationItemPtr;		
typedef std::vector<ConcatenationItemPtr>  ConcatenationItemList;

struct TranscodeItem : ConcatenationItem
{
	ASL::String mOutputFileName;
	TranscodeItem(
		const ASL::String& inFilePath, 
		const ASL::String& inOutputFileName,
		const dvamediatypes::FrameRate	inFrameRate = dvamediatypes::kFrameRate_Zero, 
		const dvamediatypes::TickTime	inStartTime = dvamediatypes::kTime_Invalid,
		const dvamediatypes::TickTime	inDuration = dvamediatypes::kTime_Invalid)
		:
	ConcatenationItem(inFilePath, inFrameRate, inStartTime, inDuration),
	mOutputFileName(inOutputFileName)
	{};
};
typedef boost::shared_ptr<TranscodeItem> TranscodeItemPtr;
typedef std::vector<TranscodeItemPtr> TranscodeItemList;

typedef std::map<ASL::Guid, ASL::String> GuidStringMap;
typedef std::set<ASL::Guid> GuidSet;
// field value map has asset ID as key and (columnID, fieldValue) pair as value.
typedef std::map<ASL::Guid, GuidStringMap> FieldValuesMap;

enum TransferOptionType
{
	kTransferOptionType_Undefine = 0,
	kTransferOptionType_Overwrite
};

enum TransferType
{
	kTransferType_Local,
	kTransferType_FTP
};

typedef std::pair<ASL::String, ASL::String> RenameItem;
typedef std::vector<RenameItem> RenameItemList;

// TransferResult is used for TransferStatus and RenameStatus.
// For RenameStatus, mDstPath is empty.
struct TransferResult
{
	ASL::String					mSrcPath;
	ASL::String					mDstPath;
	ASL::Result					mResult;
	ASL::String					mErrorMsg;
	TransferResult(ASL::String const& inSrcPath, 
		ASL::String const& inDstPath,
		ASL::Result const& inResult,
		ASL::String const& inErrorMsg = ASL::String()) 
		: mSrcPath(inSrcPath), mDstPath(inDstPath), mResult(inResult), mErrorMsg(inErrorMsg) {};
};
typedef boost::shared_ptr<TransferResult> TransferResultPtr;
typedef std::list<TransferResultPtr> TransferResultList;


struct SubClipInfo
{
	dvamediatypes::FrameRate	mVideoFrameRate;
	dvamediatypes::TickTime		mInPoint;
	dvamediatypes::TickTime		mDuration;
	ASL::String					mMarkerGuid;
};
typedef boost::shared_ptr<SubClipInfo>		SubClipInfoPtr;

struct SelectedItemInfo
{
	SelectedItemInfo(const ASL::String& inPath, const ASL::String& inName, PL::AssetMediaType inType)
		: 
		mPath(inPath), 
		mAliasName(inName) ,
		mType(inType)
		{};

	ASL::String					mPath;
	ASL::String					mAliasName;
	PL::AssetMediaType			mType;

	//	we may expand it if we want to support more types of assets
	SubClipInfoPtr				mSubClipInfo;

	ASL::Guid					mAssetItemID;
};
typedef boost::shared_ptr<SelectedItemInfo> SelectedItemInfoPtr;
typedef std::list<SelectedItemInfoPtr> SelectedItemInfoList;

struct MediaPropertyInfo
{
	ASL::Guid					mAssetID;
	ASL::String					mFilePath;
	dvamediatypes::FrameRate	mVideoFrameRate;
	dvamediatypes::TickTime		mStartTime;
	dvamediatypes::TickTime		mDuration;
	MediaPropertyInfo() : mVideoFrameRate(dvamediatypes::kFrameRate_Zero), mStartTime(dvamediatypes::kTime_Invalid), mDuration(dvamediatypes::kTime_Invalid) {};
	MediaPropertyInfo(
		const ASL::Guid& inAssetID, 
		const ASL::String& inPath,
		const dvamediatypes::FrameRate& inFrameRate,
		const dvamediatypes::TickTime& inStartTime,
		const dvamediatypes::TickTime& inDuration)
		: mAssetID(inAssetID), mFilePath(inPath), mVideoFrameRate(inFrameRate), mStartTime(inStartTime), mDuration(inDuration) {};
};

struct ExportResultInfo
{
	ASL::String mProjectFilePath;
	ASL::String mProjectType;

};

enum TagTemplateMessageOptionType
{
	kTagTemplateOptionType_Undefine = 0,
	kTagTemplateOptionType_Overwrite
};

struct TagTemplateMessageInfo
{
	ASL::String		mTagTemplateID;
	ASL::String		mTagTemplateContent;
	TagTemplateMessageInfo(const ASL::String& inTagTemplateID, const ASL::String& inTagTemplateContent) 
		: mTagTemplateID(inTagTemplateID), mTagTemplateContent(inTagTemplateContent) {};
};
typedef boost::shared_ptr<TagTemplateMessageInfo> TagTemplateMessageInfoPtr;
typedef std::list<TagTemplateMessageInfoPtr> TagTemplateMessageInfoList;

struct TagTemplateMessageResult
{
	ASL::String		mTagTemplateID;
	ASL::Result		mResult;
	TagTemplateMessageResult(const ASL::String& inTagTemplateID, const ASL::Result& inResult) 
		: mTagTemplateID(inTagTemplateID), mResult(inResult) {};
};
typedef boost::shared_ptr<TagTemplateMessageResult> TagTemplateMessageResultPtr;
typedef std::list<TagTemplateMessageResultPtr> TagTemplateMessageResultList;

struct ChangedMarkerInfo
{
	ASL::String mOwnerClipPath;
	dvamediatypes::FrameRate	mFrameRate;
	CottonwoodMarker mMarker;
	ChangedMarkerInfo(ASL::String const& inOwnerClipPath, dvamediatypes::FrameRate inFrameRate, CottonwoodMarker const& inMarker)
		: mOwnerClipPath(inOwnerClipPath), mFrameRate(inFrameRate), mMarker(inMarker) {};
};
typedef boost::shared_ptr<ChangedMarkerInfo> ChangedMarkerInfoPtr;
typedef std::list<ChangedMarkerInfoPtr> ChangedMarkerInfoList;

}
#endif
