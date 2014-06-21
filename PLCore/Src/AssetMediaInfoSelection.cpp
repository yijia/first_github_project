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
#include "PLAssetMediaInfoSelection.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLDate.h"
#include "ASLFile.h"
#include "ASLDiskUtils.h"

//	BE
#include "BEBackend.h"
#include "BEProperties.h"

//	ML
#include "MLProcessingMessages.h"

//	DVA
#include "dvametadata/MetadataOperations.h"
#include "dvacore/xml/XMLUtils.h"

//	MZ
#include "IncludeXMP.h"
#include "IngestMedia/PLIngestUtils.h"
#include "PLUtilities.h"
#include "PLConstants.h"
#include "MZMessages.h"
#include "PLMessage.h"
#include "PLProject.h"
#include "PLLibrarySupport.h"
#include "PLUtilitiesPrivate.h"
#include "MZUtilities.h"
#include "MZBEUtilities.h"
#include "MZProject.h"

// UIF
#include "UIFDocumentManager.h"

namespace PL
{

ASL_MESSAGE_MAP_DEFINE(AssetMediaInfoSelection)
ASL_MESSAGE_MAP_END

/*
**
*/
AssetMediaInfoSelection::AssetMediaInfoSelection()
{
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
AssetMediaInfoSelection::~AssetMediaInfoSelection()
{
	ASL::StationUtils::RemoveListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
void AssetMediaInfoSelection::Init(
				ASL::PathnameList const& inMediaURLList)
{
	DVA_ASSERT(!inMediaURLList.empty());
	mMediaURLList = inMediaURLList;
}

/*
**
*/
ASL::String AssetMediaInfoSelection::GetDisplayFilePath() const
{
	AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mMediaURLList.front());
	if (assetMediaInfo != NULL)
	{
		return assetMediaInfo->GetMediaPath();
	}
	else
	{
		return ASL::String();
	}
}

/*
**
*/
AssetMediaInfoSelectionRef AssetMediaInfoSelection::Create(
								ASL::PathnameList const& inMediaURLList)
{
	AssetMediaInfoSelectionRef assetSelection(AssetMediaInfoSelection::CreateClassRef());
	assetSelection->Init(inMediaURLList);

	return assetSelection;
}

/*
**
*/
ASL::Result AssetMediaInfoSelection::ReadMetaDatum(
	const MF::BinaryData& inPrefs,
	BE::MetaDataType inType,
	void* inID,
	MF::BinaryData& outDatum)
{
	AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mMediaURLList.front());
	outDatum = MF::BinaryData(assetMediaInfo->GetXMPString()->c_str(), assetMediaInfo->GetXMPString()->length());

	return ASL::kSuccess;
}

/*
**
*/
ASL::Result AssetMediaInfoSelection::WriteMetaDatum(
	const MF::BinaryData& inPrefs,
	BE::MetaDataType inType,
	void* inID,
	const MF::BinaryData& inDatum,
	bool inAlwaysPostToUIThread,
	bool inRefreshFile,
	ASL::SInt32* outRequestID,
	const dvametadata::MetaDataChangeList& inChangeList)
{
	MF::BinaryData tempXMPData  = inDatum;

	SaveMetadata(tempXMPData);
	return ASL::kSuccess;
}

/*
**
*/
bool AssetMediaInfoSelection::IsEAMediaInfo() const
{
	return MZ::Utilities::IsEAMediaPath(mMediaURLList.front());
}

/*
**
*/
ASL::Result AssetMediaInfoSelection::SaveMetadata(MF::BinaryData const& inBinaryData)
{
	XMPText xmpText(new ASL::StdString);
	SXMPMeta metadata(inBinaryData.GetData(), (XMP_StringLen)inBinaryData.GetSize());
	metadata.SerializeToBuffer(xmpText.get());

	AssetMediaInfoPtr oldAssetMediaInfo = 
		SRProject::GetInstance()->GetAssetMediaInfo(mMediaURLList.front());


	ASL::String fileCreateTime;
	ASL::String fileModifyTime;
	if (!MZ::Utilities::IsEAMediaPath(mMediaURLList.front()))
	{
		fileCreateTime = SRLibrarySupport::GetFileCreateTime(oldAssetMediaInfo->GetMediaPath());
		fileModifyTime = SRLibrarySupport::GetFileModifyTime(oldAssetMediaInfo->GetMediaPath());
	}

	BE::IProjectItemRef projectItem = 
		MZ::BEUtilities::GetProjectItem(MZ::GetProject(), oldAssetMediaInfo->GetMediaPath());
	BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);
	xmpText = Utilities::GetXMPTextWithAltTimecodeField(xmpText, masterClip);

	AssetMediaInfoList assetMediaInfoList;
	AssetMediaInfoPtr newAssetMediaInfo = AssetMediaInfo::Create(
											oldAssetMediaInfo->GetAssetMediaType(),
											oldAssetMediaInfo->GetAssetMediaInfoGUID(),
											oldAssetMediaInfo->GetMediaPath(),
											xmpText,
											oldAssetMediaInfo->GetFileContent(),
											oldAssetMediaInfo->GetCustomMetadata(),
											fileCreateTime,
											fileModifyTime,
											oldAssetMediaInfo->GetRateTimeBase(),
											oldAssetMediaInfo->GetRateNtsc(),
											oldAssetMediaInfo->GetAliasName(),
											oldAssetMediaInfo->GetNeedSavePreCheck());

	bool discardChanges = false;
	bool result = SRUtilitiesPrivate::PreCheckBeforeSendSaveMsg(mMediaURLList.front(), newAssetMediaInfo, discardChanges, true);
	if (result)
	{
		//	[Comment] We should update AssetMediaInfo cache.
		//	or the cache may out of data compared with library.
		AssetSelectionManagerQuieter selectionQuieter;
		SRProject::GetInstance()->GetAssetMediaInfoWrapper(mMediaURLList.front())->RefreshMediaXMP(newAssetMediaInfo->GetXMPString(), true);

		assetMediaInfoList.push_back(newAssetMediaInfo);

		SRProject::GetInstance()->GetAssetLibraryNotifier()->SaveAssetMediaInfo(
			ASL::Guid::CreateUnique(),
			ASL::Guid::CreateUnique(),
			assetMediaInfoList,
			AssetItemPtr()); // No in/out marker change, so asset item is not necessary.
	}
	else
	{
		// Right now Prelude doesn't set dirty flag for static metadata changes and always discard if save fails.
		// [TODO] should change to same with markers change after auto-save support.
		//if (discardChanges)
		{
			//	if we save action can't be performed, we need roll back metadata to original.
			SRProject::GetInstance()->GetAssetMediaInfoWrapper(mMediaURLList.front())->BackToCachedXMPData();
		}
	}

	return ASL::kSuccess;
}

/*
**
*/
bool AssetMediaInfoSelection::CanWriteXMPToFile() const
{
	// there will be only clips shared same one xmp in mMediaURLList, so only need checking the first one.
	AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mMediaURLList.front());
	if (assetMediaInfo->GetAssetMediaType() == kAssetLibraryType_RoughCut)
		return false;

	// EA mode xmp is always writable
	return PL::Utilities::IsXMPWritable(mMediaURLList.front());
}

} // namespace PL
