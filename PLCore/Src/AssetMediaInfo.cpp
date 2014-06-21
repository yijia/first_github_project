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
#include "PLAssetMediaInfo.h"

//	ASL
#include "ASLStringCompare.h"

// XMP
#include "IncludeXMP.h"

namespace PL
{

namespace
{

	/*
	**
	*/
	bool TestValidXMPString(XMPText inXMPString)
	{
		try
		{
			SXMPMeta metadata;
			metadata.ParseFromBuffer(inXMPString->c_str(), inXMPString->length());
		}
		catch (...)
		{
			return false;
		}
		
		return true;
	}
}

/*
**
*/
AssetMediaInfoPtr AssetMediaInfo::Create(
		AssetMediaType		inType,
		ASL::Guid	const&	inGuid,
		ASL::String	const&	inMediaPath,
		XMPText				inXMP,
		ASL::String const&  inFileContent,
		ASL::String	const&	inCustomMetadata,
		ASL::String const&  inCreateTime,
		ASL::String const&  inLastModifyTime,
		ASL::String const&  inTimeBase,
		ASL::String const&  inNtsc,
		ASL::String const&  inAliasName,
		bool inNeedSavePreCheck)
{
	XMPText xmp = inXMP;
	if (inXMP == NULL || inXMP->empty() || !TestValidXMPString(inXMP))
	{
		SXMPMeta metadata;
		XMPText fileXMPBuffer(new ASL::StdString);
		metadata.SerializeToBuffer(fileXMPBuffer.get(), kXMP_OmitPacketWrapper);
		xmp = fileXMPBuffer;
	}

	return AssetMediaInfoPtr(new AssetMediaInfo(
								inType,
								inGuid,
								inMediaPath,
								xmp,
								inFileContent,
								inCustomMetadata,
								inCreateTime,
								inLastModifyTime,
								inTimeBase,
								inNtsc,
								inAliasName,
								inNeedSavePreCheck));
}

AssetMediaInfoPtr AssetMediaInfo::CreateMasterClipMediaInfo(
	ASL::Guid	const&	inGuid,
	ASL::String	const&	inMediaPath,
	ASL::String const&  inAliasName,
	XMPText				inXMP,
	ASL::String	const&	inCustomMetadata,
	ASL::String const&  inCreateTime,
	ASL::String const&  inLastModifyTime)
{
	return Create(
		PL::kAssetLibraryType_MasterClip,
		inGuid,
		inMediaPath,
		inXMP,
		ASL::String(),
		inCustomMetadata,
		inCreateTime,
		inLastModifyTime,
		ASL::String(),
		ASL::String(),
		inAliasName);
}

AssetMediaInfoPtr AssetMediaInfo::CreateRoughCutMediaInfo(
	ASL::Guid	const&	inGuid,
	ASL::String	const&	inMediaPath,
	ASL::String const&  inFileContent,
	ASL::String const&  inCreateTime,
	ASL::String const&  inLastModifyTime,
	ASL::String const&  inTimeBase,
	ASL::String const&  inNtsc)
{
	return Create(
		PL::kAssetLibraryType_RoughCut,
		inGuid,
		inMediaPath,
		XMPText(),
		inFileContent,
		ASL::String(),
		inCreateTime,
		inLastModifyTime,
		inTimeBase,
		inNtsc);
}

/*
**
*/
AssetMediaInfo::AssetMediaInfo(	
	AssetMediaType		inType,
	ASL::Guid	const&	inGuid,
	ASL::String	const&	inMediaPath,
	XMPText				inXMP,
	ASL::String const&  inFileContent,
	ASL::String	const&	inCustomMetadata,
	ASL::String const&  inCreateTime,
	ASL::String const&  inLastModifyTime,
	ASL::String const&  inTimeBase,
	ASL::String const&  inNtsc,
	ASL::String const&  inAliasName,
	bool				inNeedSavePreCheck,
	ASL::SInt32 inForceLoadFromLocalDisk)
	:
	mType(inType),
	mAssetMediaInfoGUID(inGuid),
	mMediaPath(inMediaPath),
	mXMP(inXMP),
	mCustomMetadata(inCustomMetadata),
	mFileContent(inFileContent),
	mCreateTime(inCreateTime),
	mLastModificationTime(inLastModifyTime),
	mRateTimeBase(inTimeBase),
	mRateNtsc(inNtsc),
	mAliasName(inAliasName),
	mNeedSavePreCheck(inNeedSavePreCheck),
	mForceLoadFromLocalDisk(inForceLoadFromLocalDisk)
{

}

/*
**
*/
AssetMediaInfo::~AssetMediaInfo()
{

}

/*
**
*/
AssetMediaType AssetMediaInfo::GetAssetMediaType() const
{
	return mType;
}

/*
**
*/
ASL::String AssetMediaInfo::GetMediaPath() const
{
	return mMediaPath;
}

/*
**
*/
XMPText AssetMediaInfo::GetXMPString() const
{
	return mXMP;
}

/*
**
*/
ASL::String AssetMediaInfo::GetCustomMetadata() const
{
	return mCustomMetadata;
}

/*
**
*/
ASL::String AssetMediaInfo::GetFileContent() const
{
	return mFileContent;
}

/*
**
*/
ASL::Guid AssetMediaInfo::GetAssetMediaInfoGUID() const
{
	return mAssetMediaInfoGUID;
}

/*
**
*/
bool AssetMediaInfo::IsAssetMediaInfoSameGuid(
	AssetMediaInfoPtr const& inLeftMediaInfo,
	AssetMediaInfoPtr const& inRightMediaInfo)
{
	ASL_ASSERT(inLeftMediaInfo != NULL && inRightMediaInfo != NULL);
	return inLeftMediaInfo->GetAssetMediaInfoGUID() == inRightMediaInfo->GetAssetMediaInfoGUID();
}

/*
**
*/
ASL::String AssetMediaInfo::GetCreateTime() const
{
	return mCreateTime;
}

/*
**
*/
ASL::String AssetMediaInfo::GetLastModifyTime() const
{
	return mLastModificationTime;
}

/*
**
*/
ASL::String AssetMediaInfo::GetRateTimeBase() const
{
	return mRateTimeBase;
}

/*
**
*/
ASL::String AssetMediaInfo::GetRateNtsc() const
{
	return mRateNtsc;
}

/*
**
*/
ASL::String AssetMediaInfo::GetAliasName() const
{
	return mAliasName;
}

/*
**
*/
void AssetMediaInfo::SetAliasName(const ASL::String& inNewAliasName)
{
	mAliasName = inNewAliasName;
}

/*
**
*/
bool AssetMediaInfo::GetNeedSavePreCheck() const
{
	return mNeedSavePreCheck;
}

/*
**
*/
void AssetMediaInfo::SetForceLoadFromLocalDisk(ASL::SInt32 inForceLoadFromLocalDisk)
{
	mForceLoadFromLocalDisk = inForceLoadFromLocalDisk;
}

/*
**
*/
ASL::SInt32	AssetMediaInfo::GetForceLoadFromLocalDisk() const
{
	return mForceLoadFromLocalDisk;
}


/*
**
*/
void AssetMediaInfo::SetNeedSavePreCheck(bool isNeedSavePreCheck)
{
	mNeedSavePreCheck = isNeedSavePreCheck;
}


} // namespace PL
