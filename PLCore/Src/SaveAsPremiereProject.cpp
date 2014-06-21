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

//	Prefix
#include "Prefix.h"

//	PRM
#include "PRMApplicationTarget.h"
#include "PRMApplicationVersionForResource.h"

//	ASL
#include "ASLAtomic.h"
#include "ASLBatchOperation.h"
#include "ASLClass.h"
#include "ASLClassFactory.h"
#include "ASLDate.h"
#include "ASLPathUtils.h"
#include "ASLResourceUtils.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLStringUtils.h"
#include "ASLThreadedWorkQueue.h"
#include "ASLSleep.h"
#include "ASLFile.h"
#include "ASLListener.h"
#include "ASLMessageMap.h"
#include "ASLResults.h"

//	BE
#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProgress.h"
#include "BEProject.h"
#include "BESequence.h"
#include "BEProperties.h"
#include "BEResults.h"

#include "BEExecutor.h"
#include "BEMedia.h"
#include "BENotification.h"
#include "BEPreferences.h"
#include "BEProject.h"
#include "BEProjectItem.h"
#include "BEProjectLoadStatus.h"
#include "BEProjectSettings.h"
#include "BEProperties.h"
#include "BEProgressFwd.h"
#include "BEClip.h"
#include "BEMarker.h"
#include "BEXML.h"

#include "BE/Sequence/ISequenceEditor.h"
#include "BE/Sequence/ITrackItem.h"
#include "BE/Core/IUniqueSerializeable.h"

// ML
#include "Media/IMediaAcceleratorDatabase.h"
#include "MLFoundation.h"
#include "MLMarkers.h"
#include "SDKErrors.h"

//	UIF
#include "UIFApplication.h"

//	Local
//#include "MezzanineResources.h"
//#include "MezzanineModule.h"
#include "MZProject.h"
#include "PLSaveAsPremiereProject.h"
#include "MZActivation.h"
#include "MZProgressImpl.h"
//#include "Results.h"
#include "MZProjectActions.h"
#include "MZSequence.h"
#include "MZSequenceActions.h"
#include "MZUtilities.h"
#include "MZDynamicLink.h"
#include "MZBEUtilities.h"
#include "MZClipActions.h"
#include "PLExportClipItem.h"

// PL
#include "PLClipActions.h"
#include "PLSequenceActions.h"

// SR
#include "PLRoughCut.h"
#include "PLMarker.h"
#include "PLUtilities.h"
#include "PLLibrarySupport.h"
#include "PLProject.h"
#include "PLProject.h"
#include "PLFactory.h"
#include "IPLPrimaryClipPlayback.h"
#include "IPLLoggingClip.h"
#include "IPLRoughCut.h"
#include "PLUtilitiesPrivate.h"
#include "PLLoggingClip.h"
#include "PLRoughCut.h"
#include "IPLMarkers.h"

// dvacore
#include "dvacore/config/Localizer.h"
#include "dvacore/threads/SharedThreads.h"

// XMLUtils
#include "XMLUtils.h"

// Boost
#include <boost/foreach.hpp>

namespace PL
{
typedef std::vector<PL::CottonwoodMarker> CWMarkerVector;
typedef std::map<dvamediatypes::TickTime, CWMarkerVector> StartTimeMarkerMap;
typedef std::pair<dvamediatypes::TickTime, CWMarkerVector> StartTimeMarkerPair;

namespace
{
typedef std::vector<ASL::String> MediaPathVec;

//first: masterclip should be exported
//second: the BE.MasterClip be exported
typedef std::map<AssetItemPtr, BE::IMasterClipRef> RelatedMasterClipMap;

static size_t WriteFileCallback(
	void *ptr,
	size_t size,
	size_t nmemb,
	void *data)
{
	ASL::File* file = reinterpret_cast<ASL::File*>(data);
	ASL::UInt32 bytesWritten;
	size_t realSize = size * nmemb;
	file->Write(ptr, realSize, bytesWritten);
	return realSize;
}

enum LibraryItemType
{
	kClip,
	kSubClip,
	kRoughCut,
};

struct ClipNodeData
{
	ASL::String mClipURL;
	LibraryItemType mLibraryItemType;
};

static void AddMarkerInCoumpoundAction(PL::CottonwoodMarker const& inCWMarker, 
	BE::IMarkersRef inMarkers, 
	BE::ICompoundActionRef inCompoundAction,
	bool isRoughCut)
{
	dvacore::UTF16String const& markerType = inCWMarker.GetType();
	if (markerType != PL::MarkerType::kChapterMarkerType
			&& markerType != PL::MarkerType::kCommentMarkerType
			&& markerType != PL::MarkerType::kFLVCuePointMarkerType
			&& markerType != PL::MarkerType::kWebLinkMarkerType
			&& markerType != PL::MarkerType::kInOutMarkerType) 
		return;
	
	if (markerType == PL::MarkerType::kInOutMarkerType && isRoughCut)
		return;

	BE::IMarkerRef marker = BE::MarkerFactory::CreateMarker(
		inCWMarker.GetName(),
		inCWMarker.GetStartTime(),
		inCWMarker.GetDuration(),
		inCWMarker.GetComment());
	ASL_ASSERT(marker != NULL);
	if (!marker)
		return;

	inCompoundAction->AddAction(inMarkers->CreateAddMarkerAction(marker));

	if (markerType == PL::MarkerType::kChapterMarkerType)
	{
		inCompoundAction->AddAction(ML::CreateSetIsChapterMarkerAction(marker, true));
	}
	else if (markerType == PL::MarkerType::kFLVCuePointMarkerType)
	{
		inCompoundAction->AddAction(ML::CreateEnableFlashCueMarkerAction(marker, true));

		ASL::String const& cueType = inCWMarker.GetCuePointType();
		bool isNavigationType = (cueType.compare(dvacore::utility::AsciiToUTF16(kXMP_CuePointType_Navigation)) == 0);

		BE::NameValuePairs nameValuePairs;
		dvatemporalxmp::CustomMarkerParamList const& cuePointParams = inCWMarker.GetCuePointList();
		//const dvatemporalxmp::XMPCuePointParamList   &cuePointParams = metadataInfo.GetCuePointParamList();
		dvatemporalxmp::CustomMarkerParamList::const_iterator cueParamIter, cueParamEndIter = cuePointParams.end();
		for (cueParamIter = cuePointParams.begin(); cueParamIter != cueParamEndIter; ++cueParamIter)
		{
			ASL::String const& key = cueParamIter->mKey.c_str();
			ASL::String const& value = cueParamIter->mValue.c_str();
			nameValuePairs.push_back(std::make_pair(key, value));
		}
		BE::IFlashCueMarkerDataRef flashCueMarkerData = BE::MarkerFactory::CreateFlashCueMarkerData(isNavigationType, nameValuePairs);
		inCompoundAction->AddAction(ML::CreateSetFlashCueMarkerDataAction(marker, flashCueMarkerData));

		inCompoundAction->AddAction(ML::CreateSetMarkerTextNameAction(marker, inCWMarker.GetName()));
	}
	else if (markerType == PL::MarkerType::kWebLinkMarkerType)
	{
		inCompoundAction->AddAction(ML::CreateEnableWebLinkMarkerAction(marker, true));
		inCompoundAction->AddAction(ML::CreateSetWebLinkMarkerDataAction(marker, inCWMarker.GetLocation(), inCWMarker.GetTarget()));

	}
}

static void AddMarkerVectorInCoumpoundAction(CWMarkerVector const& inCWMarkerVector, 
	BE::IMarkersRef inMarkers, 
	BE::ICompoundActionRef inCompoundAction,
	bool isRoughCut)
{
	DVA_ASSERT(!inCWMarkerVector.empty());
	dvamediatypes::TickTime nativeTime = inCWMarkerVector[0].GetStartTime();
	dvamediatypes::TickTime nativeDuration  = dvamediatypes::kTime_Zero;
	dvacore::UTF16String cwMarkerComments;
	dvacore::UTF16String cwMarkerName;

	for (CWMarkerVector::const_iterator f = inCWMarkerVector.begin(), l = inCWMarkerVector.end(); f != l; ++f)
	{
		dvacore::UTF16String const& markerType = f->GetType();
		if (markerType != PL::MarkerType::kChapterMarkerType
				&& markerType != PL::MarkerType::kCommentMarkerType
				&& markerType != PL::MarkerType::kFLVCuePointMarkerType
				&& markerType != PL::MarkerType::kWebLinkMarkerType
				&& markerType != PL::MarkerType::kInOutMarkerType) 
			continue;
		
		// Using InOutMarker name as SequenceMarker name.
		if (f->GetType() == PL::MarkerType::kInOutMarkerType)
		{
			if (cwMarkerName.empty())
			{
				cwMarkerName = f->GetName();
			}
			else
			{
				cwMarkerName +=  dvacore::utility::AsciiToUTF16(", ") + f->GetName();
			}
		}
		
		if (!f->GetComment().empty())
			cwMarkerComments += f->GetComment() + dvacore::utility::AsciiToUTF16("\n");

		if (nativeDuration < f->GetDuration())
		{
			if (isRoughCut && f->GetType() == PL::MarkerType::kInOutMarkerType)
				continue;
			nativeDuration = f->GetDuration();			
		}
	}

	BE::IMarkerRef marker = BE::MarkerFactory::CreateMarker(
		cwMarkerName,
		nativeTime,
		nativeDuration,
		cwMarkerComments);
	ASL_ASSERT(marker != NULL);
	if (!marker)
		return;

	bool hasChapterMarker = false;
	bool needCreateMarker = false;
	dvamediatypes::TickTime fcMarkerDuration  = dvamediatypes::kTime_Zero;
	dvamediatypes::TickTime webLinkMarkerDuration  = dvamediatypes::kTime_Zero;
	PL::CottonwoodMarker fcMarker;
	PL::CottonwoodMarker webLinkMarker;

	for (CWMarkerVector::const_iterator f = inCWMarkerVector.begin(), l = inCWMarkerVector.end(); f != l; ++f)
	{
		dvacore::UTF16String const& markerType = f->GetType();
		if (markerType != PL::MarkerType::kChapterMarkerType
			&& markerType != PL::MarkerType::kCommentMarkerType
			&& markerType != PL::MarkerType::kFLVCuePointMarkerType
			&& markerType != PL::MarkerType::kWebLinkMarkerType
			&& markerType != PL::MarkerType::kInOutMarkerType) 
			continue;

		if (markerType == PL::MarkerType::kInOutMarkerType && isRoughCut)
			continue;
	
		needCreateMarker = true;
		
		if (markerType == PL::MarkerType::kChapterMarkerType)
		{
			hasChapterMarker = true;
		}
		else if (markerType == PL::MarkerType::kFLVCuePointMarkerType)
		{
			if (fcMarkerDuration < f->GetDuration())
			{
				fcMarkerDuration = f->GetDuration();
				fcMarker = *f;
			}							
		}
		else if (markerType == PL::MarkerType::kWebLinkMarkerType)
		{
			if (webLinkMarkerDuration < f->GetDuration())
			{
				webLinkMarkerDuration = f->GetDuration();
				webLinkMarker = *f;
			}	
		}
	}

	if (needCreateMarker)
	{
		inCompoundAction->AddAction(inMarkers->CreateAddMarkerAction(marker));
	
		if(hasChapterMarker)
		{
			inCompoundAction->AddAction(ML::CreateSetIsChapterMarkerAction(marker, true));
		}
		
		if (webLinkMarkerDuration > dvamediatypes::kTime_Zero)
		{
			inCompoundAction->AddAction(ML::CreateEnableWebLinkMarkerAction(marker, true));
			inCompoundAction->AddAction(ML::CreateSetWebLinkMarkerDataAction(marker, webLinkMarker.GetLocation(), webLinkMarker.GetTarget()));
		}

		if (fcMarkerDuration > dvamediatypes::kTime_Zero)
		{
			inCompoundAction->AddAction(ML::CreateEnableFlashCueMarkerAction(marker, true));

			ASL::String const& cueType = fcMarker.GetCuePointType();
			bool isNavigationType = (cueType.compare(dvacore::utility::AsciiToUTF16(kXMP_CuePointType_Navigation)) == 0);

			BE::NameValuePairs nameValuePairs;
			dvatemporalxmp::CustomMarkerParamList const& cuePointParams = fcMarker.GetCuePointList();
			dvatemporalxmp::CustomMarkerParamList::const_iterator cueParamIter, cueParamEndIter = cuePointParams.end();
			for (cueParamIter = cuePointParams.begin(); cueParamIter != cueParamEndIter; ++cueParamIter)
			{
				ASL::String const& key = cueParamIter->mKey.c_str();
				ASL::String const& value = cueParamIter->mValue.c_str();
				nameValuePairs.push_back(std::make_pair(key, value));
			}
			BE::IFlashCueMarkerDataRef flashCueMarkerData = BE::MarkerFactory::CreateFlashCueMarkerData(isNavigationType, nameValuePairs);
			inCompoundAction->AddAction(ML::CreateSetFlashCueMarkerDataAction(marker, flashCueMarkerData));

			inCompoundAction->AddAction(ML::CreateSetMarkerTextNameAction(marker, fcMarker.GetName()));
		}
	}
}

static bool GetParentBinInfo(ASL::String& ioRelativeBinPath, ASL::Guid& outParentBinGUID, ASL::String& outParentBinName)
{
	if (ioRelativeBinPath.empty())
	{
		outParentBinGUID = ASL::Guid();
		outParentBinName = ASL::String();
		return false;
	}

	const size_t kGuidLength = ASL::Guid().AsString().size();
	const ASL::String separator = ASL_STR("\\");

	ASL::String::size_type binPathStart = ioRelativeBinPath.find_first_of(separator);
	ioRelativeBinPath.erase(binPathStart, 1);
	ASL::String::size_type binPathEnd = ioRelativeBinPath.find_first_of(separator);

	// parentBinInfo is the encoded string "GUID+bin name"
	ASL::String parentBinInfo = ioRelativeBinPath.substr(0, binPathEnd);
	ioRelativeBinPath.erase(0, binPathEnd);

	if (parentBinInfo.empty())
	{
		outParentBinGUID = ASL::Guid();
		outParentBinName = ASL::String();
		return false;
	}

	ASL::String decodedBinInfo = Utilities::DecodeNameCell(parentBinInfo);

	outParentBinGUID = ASL::Guid(decodedBinInfo.substr(0, kGuidLength));
	outParentBinName = decodedBinInfo.substr(kGuidLength, decodedBinInfo.length() - kGuidLength + 1);
	return true;
}

static BE::IProjectItemRef FindBinProjectItem(
	const BE::IProjectItemContainerRef& inContainer,
	dvacore::utility::Guid inID)
{
	if (inID == ASL::Guid())
	{
		return BE::IProjectItemRef();
	}

	BE::IProjectItemRef projectItem;

	if (inContainer)
	{
		BE::ProjectItemVector items;
		inContainer->GetItems(items);

		for (BE::ProjectItemVector::const_iterator it = items.begin();
			it != items.end() && !projectItem;
			++it)
		{
			if ((*it)->GetType() != BE::kProjectItemType_Bin)
			{
				continue;
			}

			ASL::Guid projectItemGuid = BE::IUniqueSerializeableRef(*it)->GetUniqueID();
			if (inID == projectItemGuid)
			{
				projectItem = *it;
			}
			else
			{
				projectItem = FindBinProjectItem(BE::IProjectItemContainerRef(*it), inID);
			}
		}
	}

	return projectItem;
}

static BE::IProjectItemRef BuildProjectContainerForItem(
	BE::IProjectRef const& inProject, 
	AssetItemPtr const& inAssetItem,
	BE::ProjectItemVector& outAddedProjectItemVector)
{
	BE::IProjectItemRef binProjectItem = inProject->GetRootProjectItem();

	ASL::Guid parentBinGUID;
	ASL::String parentBinName;

	ASL::String parentBinPath = inAssetItem->GetParentBinPath();
	BE::IProjectItemRef parentContainer = inProject->GetRootProjectItem();
	while (GetParentBinInfo(parentBinPath, parentBinGUID, parentBinName))
	{
		binProjectItem = FindBinProjectItem((BE::IProjectItemContainerRef)parentContainer, parentBinGUID);
		if (binProjectItem == BE::IProjectItemRef())
		{
			binProjectItem = MZ::ProjectActions::CreateBin(inProject, parentContainer, parentBinName, false);
			BE::IUniqueSerializeableRef(binProjectItem)->SetUniqueID(parentBinGUID);
			outAddedProjectItemVector.push_back(binProjectItem);
		}
		parentContainer = binProjectItem;
	}

	return binProjectItem;
}

static void ImportSubclipFileForExport(
	BE::IProjectRef inProject, 
	BE::IProjectItemRef inBin,
	PL::AssetItemPtr const& subAssetItem,
	bool inIsExportSequenceMarkers,
	BE::ProjectItemVector& outAddedProjectItemVector)
{
	DVA_ASSERT(subAssetItem->GetAssetMediaType() == kAssetLibraryType_SubClip);

	ExportClipItemPtr exportItem = ExportClipItemPtr(new ExportClipItem(subAssetItem));

	BE::IMasterClipRef masterClip = exportItem->GetMasterClip();
	ASL_ASSERT(masterClip != NULL);
	if (!masterClip)
	{
		return;
	}

	MZ::BEUtilities::ForceReadXMPMarkers(masterClip);

	ASL::String mediaAliasName = exportItem->GetAliasName();
	MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(masterClip), BE::IActionRef(masterClip->CreateSetNameAction(mediaAliasName)), false);

	// if subclip has description, add it to logginginfo, to fix bug #3596899
	ASL::String description;
	AssetMediaInfoPtr itemMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(subAssetItem->GetMediaPath());
	if (itemMediaInfo)
	{
		PL::MarkerSet markers;
		PL::TrackTypes trackTypes;
		PL::Utilities::BuildMarkersFromXMPString(*(itemMediaInfo->GetXMPString()), trackTypes, markers);
		for (PL::MarkerSet::iterator it = markers.begin(); it != markers.end(); it++)
		{
			if ((*it).GetGUID() == subAssetItem->GetAssetMarkerGuid())
			{
				description = (*it).GetComment();
				break;
			}
		}
		if (!description.empty())
		{
			PL::ClipActions::SetMasterClipDescription(masterClip, description, MZ::ActionUndoableFlags::kNotUndoable);
		}
	}


	MZ::ProjectActions::AddMasterClipToBin(inProject, inBin, masterClip, false);

	outAddedProjectItemVector.push_back(MZ::Project(inProject).FindMasterClip(masterClip));
}

static void ImportClipFileForExport(
	BE::IProjectRef inProject, 
	BE::IProjectItemRef inBin,
	PL::AssetItemPtr const& assetItem,
	bool inIsExportSequenceMarkers,
	RelatedMasterClipMap const& inRelatedMasterClips,
	BE::ProjectItemVector& outAddedProjectItemVector)
{
	DVA_ASSERT(assetItem->GetAssetMediaType() == kAssetLibraryType_MasterClip);

	RelatedMasterClipMap::const_iterator it_end = inRelatedMasterClips.end();
	for (RelatedMasterClipMap::const_iterator it = inRelatedMasterClips.begin(); it != it_end; it++)
	{
		// if find the assetItem in related masterclips, rc export will handle its export
		if (AssetItem::IsAssetItemSamePath((*it).first, assetItem))
		{
			return;
		}
	}

	ExportClipItemPtr exportItem = ExportClipItemPtr(new ExportClipItem(assetItem));

	BE::IMasterClipRef masterClip = exportItem->GetMasterClip();
	ASL_ASSERT(masterClip != NULL);
	if (!masterClip)
	{
		return;
	}

	MZ::BEUtilities::ForceReadXMPMarkers(masterClip);

	ASL::String mediaAliasName = exportItem->GetAliasName();
	MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(masterClip), BE::IActionRef(masterClip->CreateSetNameAction(mediaAliasName)), false);

	MZ::ProjectActions::AddMasterClipToBin(inProject, inBin, masterClip, false);

	outAddedProjectItemVector.push_back(MZ::Project(inProject).FindMasterClip(masterClip));
}

/*
** [TODO] Should use SilkRoad workflow.
*/
static void MapSubclipWithTrackItem(
									BE::ITrackItemSelectionRef inTrackItemSelection,
									PL::SRClipItemPtr ioCWSubclip)
{
	BE::ITrackItemGroupRef group(inTrackItemSelection);
	BE::TrackItemVector trackItems;
	group->GetItems(BE::kMediaType_Video, trackItems);

	DVA_ASSERT(trackItems.size() == 1);

	ioCWSubclip->SetTrackItem(trackItems[0]);

}

/**
**	Create a new BE sequence from input assetitem list.
**	Return the sequence, NULL if fails.
**
**  @param  inSubAssetItems			Sub assetitems from a RC.
**  @param  inProject				Which BE project the sequence should be in.
**  @param  inBin					Which Bin project item the sequence should be in.
**  @param  outPathList				All media paths, used in export.
**  @param  ioRelatedMasterClips	Masterclips and assets related to every sub assetitem, used to link the 
**									Masterclip and Sequence in export.
**  @param  outAddedProjectItemVector	All created project items, used in export.
**  @param  outErrorInfo			Error info during create sequence, empty if return a valid sequence.
*/
static BE::ISequenceRef CreateBESequenceFromRCSubClipsInternal(
	PL::AssetItemList const& inSubAssetItems,
	BE::IProjectRef const& inProject,
	BE::IProjectItemRef const& inBin,	
	ASL::String const& inSequenceName,
	MediaPathVec& outPathList,
	RelatedMasterClipMap& ioRelatedMasterClips,
	BE::ProjectItemVector& outAddedProjectItemVector,
	ASL::String& outErrorInfo,
	ASL::String& outFailedMediaName,
	bool inScaleToFrame,
	MZ::SequenceAudioTrackRule inUseType)
{
	outErrorInfo.clear();
	ExportClipItems exportClipItems;

	AssetItemList::const_iterator assetItemIterEnd = inSubAssetItems.end();
	for (AssetItemList::const_iterator assetItemIter = inSubAssetItems.begin(); assetItemIter != assetItemIterEnd; assetItemIter++)
	{
		ASL::String const& subMediaPath = (*assetItemIter)->GetMediaPath();
		ExportClipItemPtr exportItem;

		if (!ioRelatedMasterClips.empty())
		{
			//Handle the linked masterclips
			RelatedMasterClipMap::iterator it_end = ioRelatedMasterClips.end();
			for (RelatedMasterClipMap::iterator it = ioRelatedMasterClips.begin(); it != it_end; it++)
			{
				if (AssetItem::IsAssetItemSamePath((*it).first, (*assetItemIter)))
				{
					//if the BE.MasterClip has not been exported
					if ((*it).second == BE::IMasterClipRef())
					{
						exportItem = ExportClipItemPtr(new ExportClipItem(*assetItemIter));

						BE::IMasterClipRef masterClip = exportItem->GetMasterClip();
						ASL_ASSERT(masterClip != NULL);
						MZ::BEUtilities::ForceReadXMPMarkers(masterClip);

						(*it).second = masterClip;

						ASL::String mediaAliasName = PL::SRProject::GetInstance()->GetAssetMediaInfo(subMediaPath)->GetAliasName();
						mediaAliasName = mediaAliasName.empty() ? ((*it).first)->GetAssetName() : mediaAliasName;
						MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(masterClip), BE::IActionRef(masterClip->CreateSetNameAction(mediaAliasName)), false);

						MZ::ClipActions::SetScaleToFrameSize(masterClip, inScaleToFrame, false);
						BE::IProjectItemRef masterClipBinItem = BuildProjectContainerForItem(inProject, (*it).first, outAddedProjectItemVector);
						MZ::ProjectActions::AddMasterClipToBin(inProject, masterClipBinItem, masterClip, false);

						outAddedProjectItemVector.push_back(MZ::Project(inProject).FindMasterClip(masterClip));
					}
					else
					{
						//if the BE.MasterClip has been exported, just create the SRMedia to add to sequence
						exportItem = ExportClipItemPtr(new ExportClipItem(*assetItemIter, (*it).second));
					}
					break;
				}
			}
		}
		else
		{
			exportItem = ExportClipItemPtr(new ExportClipItem(*assetItemIter));

			BE::IMasterClipRef masterClip = exportItem->GetMasterClip();
			if (masterClip != NULL)
			{
				// In the creation of srMedia, if media is offline or BE.Masterclip is created failed,
				// an offline BE.MasterClip will be created.
				BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
				if (media->IsOffline())
				{
					if (ASL::PathUtils::ExistsOnDisk(subMediaPath))
					{
						// If BE.MasterClip is offline type && the file is on the disk, that means the file is un-importable.
						outErrorInfo = dvacore::ZString(
							"$$$/Prelude/Mezzanine/SaveAsPproProject/UnsupportedFileError=Unimportable file: @0", subMediaPath);
					}
					else
					{
						outErrorInfo = dvacore::ZString(
							"$$$/Prelude/Mezzanine/SaveAsPproProject/MediaOfflineError=Offline media is not supported: @0", subMediaPath);
					}
				}
				else
				{
					if (ASL::ResultFailed(SRLibrarySupport::IsSupportedMedia(masterClip)))
					{
						outErrorInfo = dvacore::ZString(
							"$$$/Prelude/Mezzanine/SaveAsPproProject/UnsupportedFileError=Unimportable file: @0", subMediaPath);
					}
				}
			}
			else
			{
				outErrorInfo = dvacore::ZString(
					"$$$/Prelude/Mezzanine/SaveAsPproProject/ImportMediaFailed=Import media failed with unknown reason: @0", subMediaPath);
			}

			if (!outErrorInfo.empty())
			{
				outFailedMediaName = ASL::PathUtils::GetFullFilePart(subMediaPath);
				return BE::ISequenceRef();
			}

			MZ::ClipActions::SetScaleToFrameSize(masterClip, inScaleToFrame, false);
			MZ::ProjectActions::AddMasterClipToBin(inProject, inProject->GetRootProjectItem(), masterClip, false);
		}

		exportClipItems.push_back(exportItem);
		outPathList.push_back(subMediaPath);
	}

	BE::IProjectItemRef seqProjectItem;
	ASL::String const& sequenceName = inSequenceName.empty() ? ASL_STR("Untitled_Sequence") : inSequenceName;
	if (!exportClipItems.empty())
	{
		BE::IMasterClipRef	firstMasterClip = exportClipItems.front()->GetMasterClip();
		// Now build a new sequence based on the first master clip
		seqProjectItem = Utilities::CreateSequenceItemFromMasterClip(
							firstMasterClip, inProject, inBin, inUseType, sequenceName);
	}
	else
	{
		seqProjectItem = PL::SRFactory::CreateEmptySequence(inProject, inBin, sequenceName);
	}
	outAddedProjectItemVector.push_back(seqProjectItem);
	if (exportClipItems.empty())
	{
		return BE::ISequenceRef();
	}

	BE::IClipProjectItemRef projectClipItem(seqProjectItem);
	ASL_ASSERT(projectClipItem != NULL);

	BE::IMasterClipRef projectMasterClip = projectClipItem->GetMasterClip();
	ASL_ASSERT(projectMasterClip != NULL);

	BE::ISequenceRef sequence(BE::MasterClipUtils::GetSequence(projectMasterClip));
	ASL_ASSERT(sequence != NULL);

	if (!projectClipItem || !projectMasterClip || !sequence)
	{
		outErrorInfo = dvacore::ZString(
			"$$$/Prelude/Mezzanine/SaveAsPproProject/CreatePremiereSequenceFail=Create PremierePro sequence failed!");
		outFailedMediaName = inSequenceName;
		return BE::ISequenceRef();
	}

	// Add CottonWood Clips to sequence
	if (SilkRoadPrivateCreator::AddClipItemsToSequenceForExport(sequence, exportClipItems, dvamediatypes::kTime_Zero))
	{
		BE::ITrackItemSelectionRef selection = MZ::BEUtilities::SelectAllTrackItems(sequence);
		if (selection)
		{
			BE::ITrackItemGroupRef itemGroup(selection);
			if (itemGroup)
			{
				dvamediatypes::TickTime startTime = itemGroup->GetStart();
				dvamediatypes::TickTime endTime = itemGroup->GetEnd();
				MZ::SequenceActions::SetWorkInOutPoint(sequence, dvamediatypes::kTime_Zero, endTime - startTime, true);
			}
		}
	}
	else
	{
		sequence = BE::ISequenceRef();
	}

	return sequence;
}

static void ImportRoughCutDataForExport(
	BE::IProjectRef inProject, 
	BE::IProjectItemRef inBin,
	PL::AssetItemPtr const& inAssetItem, 
	MediaPathVec& outPathList,
	bool inIsExportSequenceMarkers,
	RelatedMasterClipMap& ioRelatedMasterClips,
	BE::ProjectItemVector& outAddedProjectItemVector)
{
	DVA_ASSERT(inAssetItem->GetAssetMediaType() == kAssetLibraryType_RoughCut);

	AssetItemList subAssetItems = inAssetItem->GetSubAssetItemList();
	ASL::String const& sequenceName = inAssetItem->GetAssetName();
	ASL::String errorInfo, failedMediaName;

	// As comment of Wes, we will not scale the clip when add to sequence
	BE::ISequenceRef sequence = CreateBESequenceFromRCSubClipsInternal(
		subAssetItems,
		inProject,
		inBin,
		sequenceName,
		outPathList,
		ioRelatedMasterClips,
		outAddedProjectItemVector,
		errorInfo,
		failedMediaName,
		false,
		MZ::kSequenceAudioTrackRule_ExportToPpro);

	PL::SequenceActions::AddTransitionItemsToSequence(
		sequence,
		inAssetItem->GetTrackTransitions(BE::kMediaType_Video),
		BE::kMediaType_Video);
	PL::SequenceActions::AddTransitionItemsToSequence(
		sequence,
		inAssetItem->GetTrackTransitions(BE::kMediaType_Audio),
		BE::kMediaType_Audio);
}

static void BuildRCRelatedMasterClips(AssetItemList const& inAssetItems, RelatedMasterClipMap& outRelatedMasterClips)
{
	MediaPathVec masterclipPaths;
	
	//The logic will be refactored later.
	BOOST_FOREACH(AssetItemPtr const& eachAsset, inAssetItems)
	{
		if (eachAsset->GetAssetMediaType() == kAssetLibraryType_RoughCut)
		{
			AssetItemList const& subAssetItems = eachAsset->GetSubAssetItemList();
			BOOST_FOREACH(AssetItemPtr const& eachSubAsset, subAssetItems)
			{
				ASL::String const& mediaPath = eachSubAsset->GetMediaPath();
				MediaPathVec::iterator itr = std::find(masterclipPaths.begin(), masterclipPaths.end(), mediaPath);
				if (itr == masterclipPaths.end())
				{
					bool isMasterClipFound = false;
					BOOST_FOREACH(AssetItemPtr const& eachOrigAsset, inAssetItems)
					{
						if (eachOrigAsset->GetAssetMediaType() == kAssetLibraryType_MasterClip && eachOrigAsset->GetMediaPath() == mediaPath)
						{
							outRelatedMasterClips.insert(std::make_pair(eachOrigAsset, BE::IMasterClipRef()));
							masterclipPaths.push_back(mediaPath);
							isMasterClipFound = true;
							break;
						}
					}

					if (!isMasterClipFound)
					{
						AssetMediaInfoPtr itemMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mediaPath);
						if (itemMediaInfo)
						{
							PL::AssetItemPtr assetItemPtr(new PL::AssetItem(
								PL::kAssetLibraryType_MasterClip,
								mediaPath,
								itemMediaInfo->GetAssetMediaInfoGUID(),
								itemMediaInfo->GetAssetMediaInfoGUID(),
								itemMediaInfo->GetAssetMediaInfoGUID(),
								itemMediaInfo->GetAliasName().empty() ? ASL::PathUtils::GetFilePart(mediaPath) : itemMediaInfo->GetAliasName(),
								ASL::String(),
								dvamediatypes::kTime_Invalid,
								dvamediatypes::kTime_Invalid,
								dvamediatypes::kTime_Invalid,
								dvamediatypes::kTime_Invalid,
								itemMediaInfo->GetAssetMediaInfoGUID(),
								ASL::Guid(),
								eachAsset->GetParentBinPath()));

							ASL_ASSERT(assetItemPtr != NULL);
							if (assetItemPtr)
							{
								outRelatedMasterClips.insert(std::make_pair(assetItemPtr, BE::IMasterClipRef()));
								masterclipPaths.push_back(mediaPath);
							}
						}
					}
				}
			}
		}
	}
}

static void AppendMasterClipIfNecessary(PL::AssetItemList const& inAssetItems, PL::AssetItemList& outAssetItems)
{
	outAssetItems = inAssetItems;

	MediaPathVec masterclipPaths;
	BOOST_FOREACH(PL::AssetItemPtr const& eachAsset, inAssetItems)
	{
		if (eachAsset->GetAssetMediaType() == PL::kAssetLibraryType_MasterClip)
		{
			masterclipPaths.push_back(eachAsset->GetMediaPath());
		}
	}

	BOOST_FOREACH(PL::AssetItemPtr const& eachAsset, inAssetItems)
	{
		if (eachAsset->GetAssetMediaType() == PL::kAssetLibraryType_RoughCut)
		{
			PL::AssetItemList subAssetItems = eachAsset->GetSubAssetItemList();
			BOOST_FOREACH(PL::AssetItemPtr const& subAsset, subAssetItems)
			{
				ASL::String const& mediaPath = subAsset->GetMediaPath();
				MediaPathVec::iterator itr = std::find(masterclipPaths.begin(), masterclipPaths.end(), mediaPath);
				if (itr == masterclipPaths.end())
				{
					PL::AssetMediaInfoPtr itemMediaInfo = PL::SRProject::GetInstance()->GetAssetMediaInfo(mediaPath);
					ASL_ASSERT(itemMediaInfo != NULL);
					if (itemMediaInfo)
					{
						PL::AssetItemPtr assetItemPtr(new PL::AssetItem(
							PL::kAssetLibraryType_MasterClip,
							mediaPath,
							itemMediaInfo->GetAssetMediaInfoGUID(),
							itemMediaInfo->GetAssetMediaInfoGUID(),
							itemMediaInfo->GetAssetMediaInfoGUID(),
							itemMediaInfo->GetAliasName().empty() ? ASL::PathUtils::GetFilePart(mediaPath) : itemMediaInfo->GetAliasName(),
							ASL::String(),
							dvamediatypes::kTime_Invalid,
							dvamediatypes::kTime_Invalid,
							dvamediatypes::kTime_Invalid,
							dvamediatypes::kTime_Invalid,
							itemMediaInfo->GetAssetMediaInfoGUID(),
							ASL::Guid(),
							eachAsset->GetParentBinPath()));

						ASL_ASSERT(assetItemPtr != NULL);
						if (assetItemPtr)
						{
							outAssetItems.push_back(assetItemPtr);
							masterclipPaths.push_back(mediaPath);
						}
					}
				}
			}
		}
	}
}

ASL_DECLARE_MESSAGE_WITH_1_PARAM(SaveAsPremiereProjectMessage, PL::AssetItemList);

//	This class implements the Save Project operation.
ASL_DEFINE_CLASSREF(SaveAsPremiereProjectOperation, PL::ISaveAsPremiereProjectOperation);
class SaveAsPremiereProjectOperation :
	public PL::ISaveAsPremiereProjectOperation,
	public ASL::IBatchOperation,
	public ASL::IThreadedQueueRequest,
	public MZ::ProgressImpl,
	public ASL::Listener
{
	ASL_BASIC_CLASS_SUPPORT(SaveAsPremiereProjectOperation);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(PL::ISaveAsPremiereProjectOperation)
		ASL_QUERY_ENTRY(ASL::IBatchOperation)
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_CHAIN(MZ::ProgressImpl)
	ASL_QUERY_MAP_END

	ASL_MESSAGE_MAP_BEGIN(SaveAsPremiereProjectOperation)
		ASL_MESSAGE_HANDLER(SaveAsPremiereProjectMessage, OnSaveAsPremiereProject)
	ASL_MESSAGE_MAP_END
public:
	static SaveAsPremiereProjectOperationRef Create(
					const ASL::String& inFilePath,
					const ASL::String& inTitleString,
					const PL::AssetItemList& inExportItems,
					bool inIsExportSequenceMarkers)
	{
		SaveAsPremiereProjectOperationRef operation(CreateClassRef());
		operation->mFilePath = inFilePath;
		operation->mTitleString = inTitleString;
		operation->mExportAssetItems = inExportItems;
		operation->mIsExportSequenceMarkers = inIsExportSequenceMarkers;

		return operation;
	}

	SaveAsPremiereProjectOperation() :
		mResult(BE::kSuccess),
		mStationID(ASL::StationRegistry::RegisterUniqueStation()),
		mThreadDone(0),
		mAbort(0),
		mSaveInProcess(0),
		mConfirmedAbort(false)
	{
		ASL::StationUtils::AddListener(mStationID, this);
	}

	virtual MediaPathSet const& GetExportMediaFiles() const
	{
		return mMediaPaths;
	}
    
	void AddMediaPaths(ASL::String const& inClipFile)
	{
		mMediaPaths.insert(inClipFile);
	}

	virtual ~SaveAsPremiereProjectOperation()
	{
		ASL::StationUtils::RemoveListener(mStationID, this);
		ASL::StationRegistry::DisposeStation(mStationID);
	}

protected:
	//	ISaveProjectOperation
	virtual ASL::Result GetResult() const
	{
		return mResult;
	}

	virtual void StartSynchronous()
	{
		Process();
	}

	virtual bool GetAbort()
	{
		if(!mConfirmedAbort)
		{
			mConfirmedAbort = ASL::AtomicCompareAndSet(mAbort, 1, 0);
		}
		return mConfirmedAbort;
	}

	virtual void NotifyProgress(ASL::Float32 inProgress)
	{
		ASL::StationUtils::PostMessageToUIThread(mStationID, 
			ASL::UpdateOperationProgressMessage(inProgress));
	}

	//	ASL::IBatchOperation
	virtual ASL::StationID GetStationID() const
	{
		return mStationID;
	}

	virtual ASL::String GetDescription() const
	{
		return mTitleString;
	}

	virtual bool Start()
	{
		ASL_REQUIRE(mThreadedWorkQueue == NULL);
		// [TODO] Does this need to be on its own thread????
		// If we don't mind using a shared thread, then this will do, but the WIN32 version
		// created it's own thread instead of using the thread pools.
		// To create our own thread we would need to do this:
		// mThreadedWorkQueue = ASL::ThreadedWorkQueue::CreateClassRef();
		// mThreadedWorkQueue->Initialize(ASL::ThreadPriorities::kNormal, ASL::ThreadAllocationPolicies::kSingleThreaded);
		//
		// Then, when we complete or in the destructor, we need to call mThreadedWorkQueue->Shutdown().
		mThreadedWorkQueue = dvacore::threads::CreateCPUBoundExecutor();
		ASL_ASSERT(mThreadedWorkQueue != NULL);

		return (mThreadedWorkQueue != NULL) && 
			mThreadedWorkQueue->CallAsynchronously(boost::bind(&ASL::IThreadedQueueRequest::Process, ASL::IThreadedQueueRequestRef(this)));
	}

	virtual void Abort()
	{
		ASL::AtomicCompareAndSet(mAbort, 0, 1);
		while(ASL::AtomicRead(mThreadDone) == 0)
		{
			//	Wait for the worker thread to finish.
			ASL::Sleep(1);
		}
	}

	void OnSaveAsPremiereProject(PL::AssetItemList const& selectedAssetItems)
	{
		BE::IProjectRef project;
		BE::ProjectItemVector addedProjectItemVector;
		mResult = CreatePremiereProject(
							selectedAssetItems, 
							mIsExportSequenceMarkers,
							BE::IProgressRef(this), 
							project, 
							mMediaPaths,
							addedProjectItemVector);

		if (ASL::ResultSucceeded(mResult))
		{
			project->Save(mFilePath, false, BE::IProgressRef(), false);
		}

		ASL::AtomicExchange(mSaveInProcess, 0);
	}

private:
	void Process()
	{
        if (mExportAssetItems.empty() )
        {
            mResult = ASL::eFileNotFound;
		}

        if ( ASL::ResultSucceeded(mResult) )
        {
            if (GetAbort())
            {
                mResult = ASL::eUserCanceled;
            }
            else
            {
                ASL::AtomicExchange(mSaveInProcess, 1);
                ASL::StationUtils::PostMessageToUIThread(mStationID, SaveAsPremiereProjectMessage(mExportAssetItems));

                while (ASL::AtomicRead(mSaveInProcess) != 0)
                {
                    ASL::Sleep(10);
                    if (GetAbort())
                    {
                        mResult = ASL::eUserCanceled;
                        break;
                    }
                }
            }
        }


		//	All done.
		ASL::StationUtils::PostMessageToUIThread(mStationID, 
			ASL::OperationCompleteMessage());

		ASL::AtomicCompareAndSet(mThreadDone, 0, 1);

	}

	ASL::String					mTitleString;
	ASL::String					mFilePath;
	ASL::Result					mResult;
	ASL::StationID				mStationID;
	dvacore::threads::AsyncExecutorPtr	mThreadedWorkQueue;
	volatile ASL::AtomicInt		mThreadDone;
	volatile ASL::AtomicInt		mAbort;
	volatile ASL::AtomicInt		mSaveInProcess;
	bool						mConfirmedAbort;

	bool						mIsExportSequenceMarkers;
	PL::AssetItemList			mExportAssetItems;

	MediaPathSet				mMediaPaths;
};
} // private namespace


/**
**
*/
ISaveAsPremiereProjectOperationRef CreateSaveAsPremiereProjectOperation(
    const ASL::String& inFilePath,
	const ASL::String& inTitleString,
	const PL::AssetItemList& inExportItems,
	bool inIsExportSequenceMarkers)
{
	ASL_REQUIRE(!inFilePath.empty());

	return ISaveAsPremiereProjectOperationRef(
                SaveAsPremiereProjectOperation::Create(
                    inFilePath,
		            inTitleString,
                    inExportItems,
					inIsExportSequenceMarkers));
}

/**
**
*/
 ASL::Result CreatePremiereProject(
				PL::AssetItemList const& inAssetItems,
				bool inTreatMarkerAsSequenceMarker,
				BE::IProgressRef  const& inProgress,
				BE::IProjectRef& outProject,
				MediaPathSet& outMediaPaths,
				BE::ProjectItemVector& outAddedProjectItemVector)
{
	if (inAssetItems.empty())
	{
		return ASL::eUnknown;
	}

	Utilities::SuppressPeakFileGeneration suppressPeakFile;
	Utilities::SuppressAudioConforming suppressAudioConform;

	BE::IProjectSettingsRef projectSettings(ASL::CreateClassInstanceRef(BE::kProjectSettingsClassID));
	BE::IPropertiesRef projectProperties(BE::GetBackend());
	projectProperties->GetOpaque(BE::kPrefsDefaultProjectSettings, projectSettings);

	//<note>The default project settings is empty. So we use a dummy video frameRate value to avoid PPro crash.
	projectSettings->GetCaptureSettings()->SetVideoFrameRate(dvamediatypes::kFrameRate_VideoNTSC);

	//	Create an empty project to pass to premiere during copy and paste
	BE::IProjectRef newProject = BE::GetBackend()->CreateProject(ASL_STR("ExportPrProject"), projectSettings);

	RelatedMasterClipMap relatedMasterClips;
	BuildRCRelatedMasterClips(inAssetItems, relatedMasterClips);

	if (inProgress)
	{
		inProgress->StartProgress(1,  inAssetItems.size());
	}

	PL::AssetItemList::const_iterator begin = inAssetItems.begin();
	PL::AssetItemList::const_iterator end = inAssetItems.end();
	for (PL::AssetItemList::const_iterator it = begin; it != end; ++it)
	{
		if ( *it != NULL )
		{
			BE::IProjectItemRef binProjectItem = BuildProjectContainerForItem(newProject, *it, outAddedProjectItemVector);
			const dvacore::UTF16String& path = (*it)->GetMediaPath();
			switch ( (*it)->GetAssetMediaType() )
			{
			case PL::kAssetLibraryType_MasterClip:
				ImportClipFileForExport(newProject, binProjectItem, *it, inTreatMarkerAsSequenceMarker, relatedMasterClips, outAddedProjectItemVector);
				outMediaPaths.insert(path);
				break;
			case PL::kAssetLibraryType_SubClip:
				ImportSubclipFileForExport(newProject, binProjectItem, *it, inTreatMarkerAsSequenceMarker, outAddedProjectItemVector);
				outMediaPaths.insert(path);
				break;
			case PL::kAssetLibraryType_RoughCut:
				{
					MediaPathVec pathList;
					ImportRoughCutDataForExport(newProject, binProjectItem, *it, pathList, inTreatMarkerAsSequenceMarker, relatedMasterClips, outAddedProjectItemVector);

					BOOST_FOREACH(ASL::String const& path, pathList)
					{
						outMediaPaths.insert(path);
					}
					
					break;
				}	
			case PL::kAssetLibraryType_RCSubClip:	
				{
					// Currently we do not handle them (whole info is in Rough Cut XML)
					DVA_ASSERT(0);
					break;
				}
			default:
				DVA_ASSERT(0);
				break;
			}
		}

		if (inProgress)
		{
			inProgress->StartProgress(1, 1);
			inProgress->EndProgress();
		}

		if (inProgress && inProgress->UpdateProgress(0))
		{
			//	Abort
			return ASL::eUserCanceled;
		}
	}

	// xinchen: to fix bug #3220525, we have to set scale flag for every masterclip, because if 
	// the video clip is scaled to frame, when the clip is sent from Ppro to AE, there will be many problems.
	BOOST_FOREACH(BE::IProjectItemRef eachItem, outAddedProjectItemVector)
	{
		BE::IClipProjectItemRef clipItem(eachItem);
		if (clipItem)
		{
			BE::IMasterClipRef masterClip = clipItem->GetMasterClip();
			if (masterClip)
			{
				MZ::ClipActions::SetScaleToFrameSize(masterClip, false, false);
			}
		}
	}

	outProject = newProject;
	return ASL::kSuccess;
}

BE::ISequenceRef CreateBESequenceFromRCSubClips(
	PL::AssetItemList const& inSubAssetItems,
	BE::IProjectRef const& inProject,
	ASL::String const& inSequenceName,
	ASL::String& outErrorInfo,
	ASL::String& outFailedMediaName,
	bool inScaleToFrame,
	MZ::SequenceAudioTrackRule inUseType)
{
	if (inProject != NULL)
	{
		MediaPathVec outPathList;
		RelatedMasterClipMap ioRelatedMasterClips;
		BE::ProjectItemVector outAddedProjectItemVector;

		return CreateBESequenceFromRCSubClipsInternal(
			inSubAssetItems,
			inProject,
			inProject->GetRootProjectItem(),
			inSequenceName,
			outPathList,
			ioRelatedMasterClips,
			outAddedProjectItemVector,
			outErrorInfo,
			outFailedMediaName,
			inScaleToFrame,
			inUseType);
	}

	return BE::ISequenceRef();
}

} // namespace PL
