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

// BE
#include "BEClip.h"
#include "IncludeXMP.h"

//	MZ
#include "PLLibrarySupport.h"
#include "PLModulePicker.h"
#include "PLUtilities.h"
#include "PLMessage.h"
#include "MZThumbnailFile.h"
#include "MZResults.h"
#include "MZBEUtilities.h"
#include "MZProjectItemHelpers.h"
#include "PLProject.h"
#include "PLUtilitiesPrivate.h"
#include "IPLRoughCut.h"
#include "IPLLoggingClip.h"
#include "PLAsyncCallAssembler.h"
#include "PLMediaMonitorCache.h"
#include "MZSequenceActions.h"
#include "MZThumbnailSupport.h"
#include "PLWriteXMPToDiskCache.h"
#include "IngestMedia/PLIngestJob.h"
#include "IngestMedia/PLIngestMessage.h"
#include "IngestMedia/ImporterTaskExecutor.h"
#include "PLFactory.h"
#include "IngestMedia/PLIngestUtils.h"
#include "MZProject.h"
#include "PLMetadataActions.h"
#include "MZUndo.h"
#include "MZUtilities.h"
#include "PLXMPilotUtils.h"
#include "PLFTPUtils.h"
#include "IPLUnassociatedMarkers.h"
#include "MZProjectActions.h"
#include "MZBEUtilities.h"
#include "MLMetadataManager.h"
#include "PLThreadUtils.h"
#include "PLBEUtilities.h"

//	ASL
#include "ASLStationUtils.h"
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLClass.h"
#include "ASLThreadedWorkQueue.h"
#include "ASLSharedThreadedWorkQueue.h"
#include "ASLThreadManager.h"
#include "ASLAsyncCallFromMainThread.h"
#include "ASLDirectoryRegistry.h"
#include "ASLDirectories.h"
#include "ASLStringCompare.h"

//	DVA
#include "dvacore/config/Localizer.h"
#include "dvamediatypes/TimeDisplay.h"
#include "dvacore/filesupport/file/File.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/utility/StringCompare.h"
#include "dvamediatypes/Timecode.h"

//	MediaCore
#include "BE/Sequence/TrackItemSelectionFactory.h"
#include "ImporterHost.h"
#include "BE/Project/ProjectItemUtils.h"

//	ML
#include "MLResults.h"
#include "MLSource.h"
#include "MLMedia.h"
#include "Player/IPlayer.h"

//	MF
#include "MFSource.h"

//  UIF
#include "UIFBitmaps.h"


#include "dvamediatypes/TimeDisplayUtils.h"

namespace PL
{
using namespace IngestTask;
namespace
{

dvamediatypes::TickTime GetInternalTickTimeFromSecondAlignedTickTime(
	dvamediatypes::TickTime inSecondAlignedTickTime,
	dvamediatypes::FrameRate inFrameRate,
	dvamediatypes::TimeDisplay inTimeDisplay)
{
	if (inSecondAlignedTickTime == dvamediatypes::kTime_Invalid)
	{
		return dvamediatypes::kTime_Invalid;
	}

	if (inTimeDisplay != dvamediatypes::kTimeDisplay_5994NonDropTimecode &&
		inTimeDisplay != dvamediatypes::kTimeDisplay_2997NonDropTimecode &&
		inTimeDisplay != dvamediatypes::kTimeDisplay_23976Timecode)
	{
		return inSecondAlignedTickTime;
	}

	dvamediatypes::TickTime positiveSecondAlignedTickTime = inSecondAlignedTickTime < dvamediatypes::kTime_Zero ?
															-inSecondAlignedTickTime :
															inSecondAlignedTickTime;
	// convert to correct time code
	ASL::String timecode = PL::Utilities::TickTimeToTimecodeWithoutRoundingFrameRate(
		positiveSecondAlignedTickTime,
		inFrameRate,
		inTimeDisplay);

	// Non-Drop version tick time
	dvamediatypes::TickTime positiveInternalTickTime =
		dvamediatypes::TimecodeToTime(timecode, inFrameRate, inTimeDisplay);

	return inSecondAlignedTickTime < dvamediatypes::kTime_Zero ? -positiveInternalTickTime : positiveInternalTickTime;
}


static bool IsZeroDurationSubClipMarker(const PL::CottonwoodMarker& inMarker)
{
	return (inMarker.GetDuration() == dvamediatypes::kTime_Zero && 
			dvacore::utility::StringEquals(inMarker.GetType(), PL::MarkerType::kInOutMarkerType));
}

// Per latest spec, we are not allowing re-applying marker now, so change the code back and backup my change of
// supporting re-applying markers to avoid spec changes again.
#define ALLOW_REAPPLYMARKER		0

	class AssetItemsAppending
	{
	public:
		AssetItemsAppending()
		{
			Utilities::SetAssetItemsAppending(true);
		}
		~AssetItemsAppending()
		{
			Utilities::SetAssetItemsAppending(false);
		}
	};

void PromptAppendOfflineError(AssetItemList const& inAssetList)
{
	if (inAssetList.size() > 0)
	{
		dvacore::UTF16String errorMsg = dvacore::ZString(
			"$$$/Prelude/SRLibrarySupport/AppendOffline=The following media(s) are off-line and cannot be added to Rough Cut.\n");
		for (AssetItemList::const_iterator it = inAssetList.begin(); it != inAssetList.end(); ++it)
			errorMsg += (*it)->GetAssetName() + DVA_STR("\n");
		
		// async to fix bug 3597111 since we have the hack ScDisableArrowCursorChanges in DataSource
		// and we need that hack to disable cursor change during dragging
		dvacore::threads::GetRegisteredMainThread()->CallAsynchronously(
			boost::bind<void>(SRUtilitiesPrivate::PromptForError,
							  dvacore::ZString("$$$/Prelude/SRLibrarySupport/AppendOfflineCaption=Media Offline"),
							  errorMsg),
			100
			);
	}
}
/*
**
*/
void ThumbnailCompleteCallback(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID, 
		ASL::String const& inImportFile,
		dvamediatypes::TickTime const& inClipTime,
		ASL::Guid const& inAssetID,
		ASL::String const& inThumbnailFileName,
		ASL::Result inResult
	)
{
	// TODO: call library notifier to return the status. we suppose the functions in notifier are thread safe.
	IAssetLibraryNotifier* assetLibraryNotifier = SRProject::GetInstance()->GetAssetLibraryNotifier();
	assetLibraryNotifier->RequestThumbnaiFinished(
												inTaskID, 
												inMsgID,
												inResult,
												inImportFile, 
												inAssetID,
												inClipTime,
												inThumbnailFileName,
												ASL::Guid::CreateUnique(),
												ASL::Guid::CreateUnique());	
}

/*
**
*/
ASL::Result ImportXMPImpl(
	const ASL::String& inFilePath,
	MZ::ImportResultVector& outFilesFailure,
	CustomMetadata& outCustomMetadata,
	XMPText outXMPBuffer)
{
	// should not happen, just for protection
	if (ASL::PathUtils::IsDirectory(inFilePath))
	{
		return ASL::kSuccess;
	}

	ASL::Result result = ASL::kSuccess;
	ASL::String errorInfo( dvacore::ZString("$$$/Prelude/MZ/ImportXMPFailure=Cannot import the file \"") + inFilePath + DVA_STR("\"") );

	//	We import file first, so that we can get necessary info of this file
	MZ::ImportResultVector failureVector;

	// Not ignore audio, not ignore XMP
	BE::IMasterClipRef masterClip = BEUtilities::ImportFile(inFilePath, BE::IProjectRef(), failureVector, false, false); 

	BE::IMasterClipRef masterClipInProject;
	if (masterClip == NULL)
	{
		if (failureVector.empty())
		{
			result = MZ::eAppUnSupportFormat;
		}
		else
		{
			result = failureVector.front().first;
			errorInfo = failureVector.front().second;
		}
	}
	else if (ASL::ResultFailed(SRLibrarySupport::IsSupportedMedia(masterClip)))
	{
		result = MZ::eAppUnSupportFormat;
	}

	if (ASL::ResultSucceeded(result))
	{
		outCustomMetadata = Utilities::GetCustomMedadata(masterClip);

		// Get XMP Data 
		ASL::String errorInfo;
		SRLibrarySupport::ReadXMPFromCache(inFilePath, outXMPBuffer, errorInfo);

		XMPilotEssenseMarkerList essenseMarkerList;
		NamespaceMetadataList planningMetadata;
		if (ParseXMPilotXmlFile(inFilePath, essenseMarkerList, planningMetadata))
		{
			bool isMarkerChanged = ApplyXMPilotEssenseMarker(*outXMPBuffer.get(), essenseMarkerList, outCustomMetadata.mVideoFrameRate);
			bool isPlanningMetadataChanged = MergeXMPWithPlanningMetadata(outXMPBuffer, planningMetadata);
			if (isMarkerChanged || isPlanningMetadataChanged)
			{
				AssetMediaInfoPtr assetMediaInfo = AssetMediaInfo::CreateMasterClipMediaInfo(
																				ASL::Guid::CreateUnique(),
																				inFilePath,
																				ASL::PathUtils::GetFilePart(inFilePath),
																				outXMPBuffer);
				if (!SRLibrarySupport::SaveXMPToDisk(assetMediaInfo))
				{
					const dvacore::UTF16String errorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/SRLibrarySupport/SaveFailed=The file @0 cannot be saved", ASL::PathUtils::GetFullFilePart(inFilePath.c_str()));
					ML::SDKErrors::SetSDKErrorString(errorMsg);
				}
			}
		}

	}

	if ( !ASL::ResultSucceeded(result))
	{
		outFilesFailure.push_back(std::make_pair(result, errorInfo));
	}

	return result;
}


/*
**
*/
class LoadingFileOperation;
ASL_DEFINE_CLASSREF(LoadingFileOperation, ASL::IThreadedQueueRequest);
// {5F6D8A70-E160-4ED8-8CF7-B1B3CEEDDA79}
ASL_DEFINE_IMPLID(LoadingFileOperation, 0x5f6d8a70, 0xe160, 0x4ed8, 0x8c, 0xf7, 0xb1, 0xb3, 0xce, 0xed, 0xda, 0x79);

class LoadingFileOperation
	:
	public ASL::IThreadedQueueRequest
{
public:
	ASL_BASIC_CLASS_SUPPORT(LoadingFileOperation);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(LoadingFileOperation)
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_MAP_END

	void Initialize(const ASL::Guid& inTaskGuid, 
					const ASL::Guid& inMsgGuid, 
					const ASL::String& inPath, 
					const ASL::Guid& inAssetGuid,
					bool inIsMediaPropertyOnly)
	{
		mTaskGuid = inTaskGuid;
		mMsgGuid = inMsgGuid;
		mPath = inPath;
		mAssetGuid = inAssetGuid;
		mIsMediaPropertyOnly = inIsMediaPropertyOnly;
	}

	virtual void Process()
	{
		IAssetLibraryNotifier* assetLibraryNotifier = SRProject::GetInstance()->GetAssetLibraryNotifier();
		if (assetLibraryNotifier == NULL)
		{
			return;
		}

		// Defense programming.
		if (!ASL::PathUtils::IsValidPath(mPath))
		{
			DVA_ASSERT_MSG(0, "Loading invalid file path!");
			return;
		}

		if (IsRoughCutFileExtension(mPath))
		{
			DVA_ASSERT_MSG(!mIsMediaPropertyOnly, "Should not get media property for rough cut");
			ASL::Result result = ASL::eUnknown;
			PL::AssetMediaInfoList assetMediaInfoList;
			PL::AssetItemList assetItemList;
			if (!ASL::PathUtils::ExistsOnDisk(mPath))
			{
				result = ASL::eFileNotFound;
			}
			else if (IsValidRoughCutFile(mPath))
			{
				PL::AssetItemPtr assetItem;
				PL::AssetMediaInfoPtr assetMediaInfo;
				result = LoadRoughCut(mPath, mAssetGuid, assetMediaInfo, assetItem);
				assetItemList.push_back(assetItem);
				assetMediaInfoList.push_back(assetMediaInfo);
			}
			assetLibraryNotifier->ImportRoughCutFinished(
					mTaskGuid, 
					mMsgGuid, 
					result, 
					ASL::Guid::CreateUnique(),
					ASL::Guid::CreateUnique(),
					assetMediaInfoList,
					assetItemList);
		}
		else
		{
			XMPText xmpBuffer(new ASL::StdString);
			//NOTE: import in main thread for SWF file.
			MZ::ImportResultVector importFailure;
			CustomMetadata customMetadata;
			// [TRICKY] Import in worker thread causes exit crash bug. So always import in main thread.
			ASL::Result result = ASL::PathUtils::ExistsOnDisk(mPath)
				? SRAsyncCallAssembler::ImportXMPInMainThread(mPath, importFailure, customMetadata, xmpBuffer, ImportXMPImpl)
				: ASL::eFileNotFound;

			ASL::String errorInfo(dvacore::ZString("$$$/Prelude/MZ/ImportErrorInfo=Cannot find the file \"") + mPath + DVA_STR("\"") );
			if ( importFailure.size() > 0 )
			{
				errorInfo = importFailure.front().second;
			}

			if (!mIsMediaPropertyOnly)
			{
				assetLibraryNotifier->ImportXMPFinished(
												mTaskGuid, 
												mMsgGuid, 
												result, 
												mPath, 
												mAssetGuid,
												customMetadata, 
												xmpBuffer, 
												SRLibrarySupport::GetFileCreateTime(mPath), 
												SRLibrarySupport::GetFileModifyTime(mPath),
												ASL::Guid::CreateUnique(),
												ASL::Guid::CreateUnique(),
												importFailure);
			}
			else
			{
				assetLibraryNotifier->GetMediaPropertyInfoStatus(
												mTaskGuid,
												mMsgGuid,
												result,
												MediaPropertyInfo(mAssetGuid, mPath, customMetadata.mVideoFrameRate, customMetadata.mMediaStart, customMetadata.mDuration));
			}
		}
	}

private:
	ASL::Guid	mTaskGuid;
	ASL::Guid	mMsgGuid;
	ASL::String	mPath;
	ASL::Guid   mAssetGuid;
	bool		mIsMediaPropertyOnly;
};

/*
**
*/
void RefreshSequenceEditLine(BE::ISequenceRef inSequence)
{
	dvamediatypes::TickTime editLine = MZ::SequenceActions::GetEditLine(inSequence);
	MZ::Info::RefreshMasterAtPosition(editLine);
}

/*
**
*/
ASL::Result DeleteRoughCut(AssetItemPtr const& inRoughCutItem)
{
	//	Close this roughcut if it is opened in time line
	ModulePicker::GetInstance()->CloseModule(inRoughCutItem->GetMediaPath());
	return ASL::kSuccess;
}

/*
**
*/
ASL::Result DeleteMasterClip(AssetItemPtr const& inMasterItem)
{
	//	Remove this masterClip from Monitor Cache
	SRMediaMonitor::GetInstance()->UnRegisterMonitorMediaXMP(inMasterItem->GetMediaPath());

	SRMediaMonitor::GetInstance()->UnRegisterMonitorFilePath(inMasterItem->GetMediaPath());

	//	Close this logging clip if it is opened in time line
	ModulePicker::GetInstance()->CloseModule(inMasterItem->GetMediaPath());
	return ASL::kSuccess;
}

/*
**
*/
ASL::Result DeleteEASubClip(AssetItemPtr const& inMasterItem)
{
	//	Close this logging clip if it is opened in time line
	ModulePicker::GetInstance()->CloseModule(inMasterItem->GetMediaPath());
	return ASL::kSuccess;
}

/*
**
*/
ASL::Result DeleteSubClipsInSameClip(AssetItemList const& inSubClipItemList, ActionResultList& outActionResultList)
{
	if (inSubClipItemList.empty())
		return ASL::ResultFlags::kResultTypeFailure;

	AssetItemList::const_iterator iter = inSubClipItemList.begin();
	for (; iter != inSubClipItemList.end(); ++iter)
	{
		ActionResultPtr actionResult(new ActionResult((*iter)->GetAssetGUID(), (*iter)->GetMediaPath()));
		outActionResultList.push_back(actionResult);
	}

	ASL::String mediaPath = inSubClipItemList.front()->GetMediaPath();
	ISRPrimaryClipRef primaryClip = SRProject::GetInstance()->GetPrimaryClip(mediaPath);
	if (NULL == primaryClip)
	{
		primaryClip = SRFactory::CreatePrimaryClip(inSubClipItemList.front());
	}
	ISRPrimaryClipPlaybackRef loggingPlayBack = ISRPrimaryClipPlaybackRef(primaryClip);
	if (primaryClip == NULL || loggingPlayBack == NULL)
		return ASL::ResultFlags::kResultTypeFailure;

	SRMarkersList const& markersList = loggingPlayBack->GetMarkersList();
	DVA_ASSERT(markersList.size() == 1);
	if (markersList.size() == 1)
	{
		ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = ModulePicker::GetInstance()->GetCurrentModuleData();
		ISRPrimaryClipRef activePrimaryClip = ISRPrimaryClipRef(activePrimaryClipPlayback);

        PL::ISRMarkersRef markers = markersList.front();
		AssetItemList::const_iterator it = inSubClipItemList.begin();
		ActionResultList::iterator resIt = outActionResultList.begin();
		ASL::Result result = ASL::ResultFlags::kResultTypeFailure;
		for (; it != inSubClipItemList.end(); ++it, ++resIt)
		{
			CottonwoodMarker markerToRemove;
			if (markers->GetMarker((*it)->GetAssetMarkerGuid(), markerToRemove))
			{
				markers->RemoveMarker(markerToRemove);
				(*resIt)->mResult = ASL::kSuccess;
				result = ASL::kSuccess;
			}
			else
			{
				(*resIt)->mErrorMsg = dvacore::ZString(
					"$$$/Prelude/SRLibrarySupport/DeleteSubClipFailed=Delete sub-clip failed.");
			}
		}
		if (result == ASL::kSuccess)
		{
			UIF::MBResult::Type returnValue = primaryClip->Save(true, false);
			if (returnValue == UIF::MBResult::kMBResultOK && primaryClip == activePrimaryClip)
			{
				MZ::ClearUndoStack(MZ::GetProject());
			}

			return ASL::kSuccess;
		}
	}
	return ASL::ResultFlags::kResultTypeFailure;
}

/*
**
*/
bool IsValidAssetItemList(AssetItemList const & inAssetItemList)
{
	std::set<AssetMediaType> assetTypeSet;
	AssetItemList::const_iterator iter = inAssetItemList.begin();
	AssetItemList::const_iterator end = inAssetItemList.end();

	for(; iter != end; ++iter)
	{
		assetTypeSet.insert((*iter)->GetAssetMediaType());
	}

	bool containsMasterClip = assetTypeSet.find(kAssetLibraryType_MasterClip) != assetTypeSet.end();
	bool containsMasterSubClip = assetTypeSet.find(kAssetLibraryType_SubClip) != assetTypeSet.end();
	bool containsRoughCut = assetTypeSet.find(kAssetLibraryType_RoughCut) != assetTypeSet.end();
	bool containsRoughCutSubClip = assetTypeSet.find(kAssetLibraryType_RCSubClip) != assetTypeSet.end();

	bool onlyContainsLoggingClip = (containsMasterClip || containsMasterSubClip) && !containsRoughCut && !containsRoughCutSubClip;
	bool onlyContainsRC = (containsRoughCut || containsRoughCutSubClip) && !containsMasterClip && !containsMasterSubClip;

	return onlyContainsRC || onlyContainsLoggingClip;
}


dvamediatypes::TickTime GetAssetItemDuration(AssetItemPtr const& inAssetItem)
{
	if (inAssetItem->GetAssetMediaType() == kAssetLibraryType_MasterClip)
	{
		ISRMediaRef srMedia = SRProject::GetInstance()->GetSRMedia(inAssetItem->GetMediaPath());
		DVA_ASSERT (srMedia != NULL);
		BE::IMasterClipRef masterClip = srMedia->GetMasterClip();
		DVA_ASSERT (masterClip != NULL);
		return masterClip->GetMaxUntrimmedDuration(BE::kMediaType_Any);
	}
	else
	{
		return inAssetItem->GetDuration();
	}
}

const char* const kMarkerHaveExistedMsg = 
	"$$$/Prelude/Mezzanine/PLLibrarySupport/MarkerHaveBeenAddedWarning=The marker @0 from the Unassociated Metadata panel already exists in clip \"@1\" and cannot be added again.";
const char* const kMarkerFailToAddMsg = 
	"$$$/Prelude/Mezzanine/PLLibrarySupport/MarkerFailToAddWarning=The marker @0 from the Unassociated Metadata panel is not in the time range of clip \"@1\" and cannot be added.";
const char* const kMarkerUpdatedMsg = 
	"$$$/Prelude/Mezzanine/PLLibrarySupport/MarkerHaveBeenUpdatedWarning=The marker @0 in the Timeline has been updated to clip \"@1\" from the Unassociated Metadata panel.";
const char* const kZeroDurationSubClipMarkerMsg = 
	"$$$/Prelude/Mezzanine/PLLibrarySupport/ZeroSubClipMarkerWarning=The marker @0 from the Unassociated Metadata panel is (or will be truncated to) a zero duration subclip marker so cannot be applied to clip \"@1\".";

typedef std::map<ASL::Guid, PL::CottonwoodMarker> GuidToCWMarkerMap;

/**
**	The function's work:
**		1. build out all cottonwood markers from unassociated metadata list
**		2. Consider dayoffset (timecode offset in future) of the cottonwood markers
**		3. Consider case that marker is out of clip's range
**
**	inXMP:	xmp content of clip that will be applied unassociated markers.
*/
void BuildAvailableUnassociatedMarkers(
	const PL::UnassociatedMetadataList& inUnassociatedMetadataList,
	XMPText inXMP,
	const BE::IMasterClipRef& inMasterClip,
	const dvamediatypes::FrameRate& inVideoFrameRate,
	const dvamediatypes::TickTime& inMediaDuration,
	const ASL::String& inClipName,
	GuidToCWMarkerMap& outCWMarkerMap)
{
	BOOST_FOREACH(const PL::UnassociatedMetadataPtr& unassociatedMetadata, inUnassociatedMetadataList)
	{
		PL::SRUnassociatedMetadataRef srUnassociatedMetadata = NULL;
		if (unassociatedMetadata->mXMP != NULL)
		{
			srUnassociatedMetadata = SRFactory::CreateSRUnassociatedMetadata(unassociatedMetadata->mMetadataId, unassociatedMetadata->mMetadataPath, unassociatedMetadata->mXMP);
		}
		else
		{
			srUnassociatedMetadata = PL::SRProject::GetInstance()->FindSRUnassociatedMetadata(unassociatedMetadata->mMetadataId);		
		}
		if (srUnassociatedMetadata == NULL)
		{
			continue;
		}

		PL::ISRUnassociatedMarkersRef srMarkers = srUnassociatedMetadata->GetMarkers();
		if (srMarkers == NULL)
		{
			continue;
		}

		PL::MarkerTrack markerTrack = srMarkers->GetMarkers();
		if (markerTrack.empty())
		{
			continue;
		}

		dvamediatypes::FrameRate videoFrameRate;
		BE::IClipRef videoClip = inMasterClip->GetClip(BE::kMediaType_Video, 0);
		if (videoClip)
		{
			videoFrameRate = videoClip->GetFrameRate();
		}
		else
		{
			videoFrameRate = Utilities::GetDefaultVideoFrameRate();
		}
		bool isDropFrame = MZ::Utilities::IsDropFrame(inMasterClip);
		dvamediatypes::TimeDisplay videoTimeDisplay = 
			MZ::Utilities::GetTimeDisplayForFrameRate(videoFrameRate, isDropFrame);

		dvamediatypes::TickTime offsetFromSrcStartToDstStart = dvamediatypes::kTime_Zero;
		PL::SRUnassociatedMetadata::OffsetType offsetType = srUnassociatedMetadata->GetOffsetType();
		if (offsetType == PL::SRUnassociatedMetadata::OffsetType_Timecode && inMasterClip)
		{
			BE::IContentRef content = BE::MasterClipUtils::GetContent(inMasterClip, BE::kMediaType_Any, 0);
			if (content)
			{
				// *NOTICE* Because of rounding frame rate problem, MediaCore will give a wrong tick time
				//	based on a clip's timecode if clip has frame rate of 23976, 2997NonDrop or 5994NonDrop,
				//	so here we need to transform tick back to timecode, and do a correct transform from timecode
				//	to tick.
				//
				//	If we always use dvamediatypes code, it will be:
				//		correct timecode --> wrong tick --> correct timecode, or
				//		correct tick --> wrong timecode --> correct tick
				//	Nothing wrong will happen if we go through the full path, but we should be careful
				//	if we only touch any half.
				ASL::String destMediaTimecodeString = 
					Utilities::ConvertTimecodeToString(content->GetOffset(), videoFrameRate, isDropFrame);

				dvamediatypes::TickTime destMediaStart = PL::Utilities::TimecodeToTickTimeWithoutRoundingFrameRate(
															destMediaTimecodeString,
															videoFrameRate,
															videoTimeDisplay);

				offsetFromSrcStartToDstStart = srUnassociatedMetadata->GetOffset() - destMediaStart;
			}
		}
		else if (offsetType == PL::SRUnassociatedMetadata::OffsetType_Date && inXMP)
		{
			dvamediatypes::TickTime shotTicks = dvamediatypes::kTime_Zero;
			// Calculate the open clip's shot time in Ticks (do not use a framerate yet, just
			//    do this by the time of day starting from 00:00:00. This way the marker time and
			//    the clip time reference the same base time.
			ASL::Date::ExtendedTimeType shotTime;
			if (PL::Utilities::ExtractDateFromXMP(inXMP, kXMP_NS_DM, "shotDate", shotTime)
				|| PL::Utilities::ExtractDateFromXMP(inXMP, kXMP_NS_XMP, "CreateDate", shotTime))
			{
				shotTicks = Utilities::ConvertDayTimeToTick(shotTime);
				offsetFromSrcStartToDstStart = srUnassociatedMetadata->GetOffset() - shotTicks;
			}
		}


		// Walk each marker. Any marker start time within open clip's range will be applied at that
		// time automatically
		for (PL::MarkerTrack::iterator trackItr = markerTrack.begin(); trackItr != markerTrack.end(); ++trackItr)
		{
			PL::CottonwoodMarker marker = (*trackItr).second;

			if (!unassociatedMetadata->mIsApplyAll && 
				unassociatedMetadata->mIncludedMarkerIDs.find(marker.GetGUID()) == unassociatedMetadata->mIncludedMarkerIDs.end())
			{
				continue;
			}

#if 0
			// Subtract the clip shot time from the marker time - this gives us the marker offset in
			//    raw ticks from Time Of Day
			dvamediatypes::TickTime dstMarkerStartTime = marker.GetStartTime() + offsetFromSrcStartToDstStart;
			dstMarkerStartTime.AlignToFrame(inVideoFrameRate);

			dvamediatypes::TickTime dstMarkerEndTime = dstMarkerStartTime + marker.GetDuration();
			dstMarkerEndTime.AlignToFrame(inVideoFrameRate);
#else
			// Subtract the clip shot time from the marker time - this gives us the marker offset in
			//    raw ticks from Time Of Day
			dvamediatypes::TickTime dstMarkerStartTime = marker.GetStartTime() + offsetFromSrcStartToDstStart;
			dstMarkerStartTime = GetInternalTickTimeFromSecondAlignedTickTime(
				dstMarkerStartTime,
				videoFrameRate,
				videoTimeDisplay);
			dstMarkerStartTime.AlignToFrame(inVideoFrameRate);

			dvamediatypes::TickTime dstMarkerEndTime = dstMarkerStartTime + GetInternalTickTimeFromSecondAlignedTickTime(
				marker.GetDuration(),
				videoFrameRate,
				videoTimeDisplay);
			dstMarkerEndTime.AlignToFrame(inVideoFrameRate);
#endif
	

			if (dstMarkerStartTime > inMediaDuration || dstMarkerEndTime < dvamediatypes::kTime_Zero)
			{
				ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerFailToAddMsg, marker.GetName(), inClipName));
			}
			else
			{
				// [Todo]: if we dont need marker handle, we may allow a negative marker start
				if (dstMarkerStartTime < dvamediatypes::kTime_Zero)
				{
					dstMarkerStartTime = dvamediatypes::kTime_Zero;
				}
				if (dstMarkerStartTime != marker.GetStartTime())
				{
					marker.SetStartTime(dstMarkerStartTime);
				}

				if (dstMarkerEndTime > inMediaDuration)
				{
					// Adjust marker duration to end at clip's out point
					marker.SetDuration(inMediaDuration - dstMarkerStartTime);
				}
				else
				{
					marker.SetDuration(dstMarkerEndTime - dstMarkerStartTime);
				}

				if (IsZeroDurationSubClipMarker(marker))
				{
					ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kZeroDurationSubClipMarkerMsg, marker.GetName(), inClipName));
					continue;
				}

				outCWMarkerMap[marker.GetGUID()] = marker;
			}
		}
	}
}

void ApplyUnassociatedMetadataToOneMedia(
	const PL::UnassociatedMetadataList& inUnassociatedMetadataList,
	const ASL::String& inPath)
{
	BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), inPath);
	BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);
	if (!masterClip)
	{
		DVA_ASSERT_MSG(0, "Cannot find MasterClip to apply!!");
		return;
	}

	dvamediatypes::FrameRate videoFrameRate;
	BE::IClipRef videoClip = masterClip->GetClip(BE::kMediaType_Video, 0);
	if (videoClip)
	{
		videoFrameRate = videoClip->GetFrameRate();
	}
	else
	{
		videoFrameRate = Utilities::GetDefaultVideoFrameRate();
	}

	dvamediatypes::TickTime mediaDuration = masterClip->GetMaxUntrimmedDuration(BE::kMediaType_Any);

	PL::AssetMediaInfoPtr assetMediaInfo = PL::SRProject::GetInstance()->GetAssetMediaInfo(inPath);
	if (!assetMediaInfo)
	{
		DVA_ASSERT_MSG(0, "Empty asset media info!!");
		return;
	}
	XMPText xmp = assetMediaInfo->GetXMPString();

	const ASL::String& clipName = MZ::BEUtilities::IsEAMasterClip(masterClip) ? masterClip->GetName() : inPath;

	// 1. build srmarkers
	GuidToCWMarkerMap unassociatedMarkers;
	BuildAvailableUnassociatedMarkers(
		inUnassociatedMetadataList, xmp, masterClip, videoFrameRate, mediaDuration, clipName, unassociatedMarkers);
	if (unassociatedMarkers.empty())
	{
		return;
	}

	// 2. Merge with existing markers
	PL::TrackTypes trackTypes;
	PL::MarkerSet oldMarkers, newMarkers;
	ASL::StdString xmpContent = *xmp;
	Utilities::BuildMarkersFromXMPString(xmpContent, trackTypes, oldMarkers);

#if ALLOW_REAPPLYMARKER
	for (GuidToCWMarkerMap::iterator it = unassociatedMarkers.begin(); it != unassociatedMarkers.end(); it++)
	{
		newMarkers.insert((*it).second);
	}
	for (PL::MarkerSet::iterator it = oldMarkers.begin(); it != oldMarkers.end(); it++)
	{
		if (unassociatedMarkers.find((*it).GetGUID()) != unassociatedMarkers.end())
		{
			ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerUpdatedMsg, (*it).GetName()));
		}
		else
		{
			newMarkers.insert((*it));
		}
	}
#else
	for (GuidToCWMarkerMap::iterator it = unassociatedMarkers.begin(); it != unassociatedMarkers.end(); it++)
	{
		PL::MarkerSet::iterator iterOld = oldMarkers.begin();
		for (; iterOld != oldMarkers.end(); iterOld++)
		{
			if ((*iterOld).GetGUID() == (*it).second.GetGUID())
			{
				break;
			}
		}

		if (iterOld == oldMarkers.end())
		{
			// we add the marker whose Guid is not in existing markers
			newMarkers.insert((*it).second);
		}
		else
		{
			ML::SDKErrors::SetSDKWarningString(
				dvacore::ZString(kMarkerHaveExistedMsg, 
				(*it).second.GetName(), 
				clipName));
		}
	}

	if (newMarkers.empty())
	{
		return;
	}

	newMarkers.insert(oldMarkers.begin(), oldMarkers.end());
#endif

	// 3. write markers to xmp
	Utilities::BuildXMPStringFromMarkers(xmpContent , newMarkers, PL::TrackTypes(), videoFrameRate);
	XMPText newXMP(new ASL::StdString(xmpContent));
	// save xmp, do as save logging clip workflow
	{
		AssetMediaInfoPtr oldAssetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(inPath);

		ASL::String fileCreateTime;
		ASL::String fileModifyTime;
		if (!MZ::Utilities::IsEAMediaPath(inPath))
		{
			fileCreateTime = PL::SRLibrarySupport::GetFileCreateTime(oldAssetMediaInfo->GetMediaPath());
			fileModifyTime = PL::SRLibrarySupport::GetFileModifyTime(oldAssetMediaInfo->GetMediaPath());
		}

		AssetMediaInfoPtr newAssetMediaInfo = AssetMediaInfo::Create(
			oldAssetMediaInfo->GetAssetMediaType(),
			oldAssetMediaInfo->GetAssetMediaInfoGUID(),
			inPath,
			newXMP,
			oldAssetMediaInfo->GetFileContent(),
			oldAssetMediaInfo->GetCustomMetadata(),
			fileCreateTime,
			fileModifyTime,
			oldAssetMediaInfo->GetRateTimeBase(),
			oldAssetMediaInfo->GetRateNtsc(),
			oldAssetMediaInfo->GetAliasName(),
			oldAssetMediaInfo->GetNeedSavePreCheck());

		SRProject::GetInstance()->RegisterAssetMediaInfo(inPath, newAssetMediaInfo, true);

		// Notify metadata updated to let cached xmp data refresh, such as metadata panel.		
		SRProject::GetInstance()->GetAssetMediaInfoWrapper(inPath)->RefreshMediaXMP(newAssetMediaInfo->GetXMPString());

		// Update content in BEMasterClip
		if (!MZ::Utilities::IsEAMediaPath(inPath))
		{
			BE::IMediaMetaDataRef  mediaMetaData = 
				BE::IMediaMetaDataRef(BE::MasterClipUtils::GetDeprecatedMedia(masterClip)->GetMediaInfo());
			ASL::SInt32 requestID;

			mediaMetaData->WriteMetaDatum(
				MF::BinaryData(),
				BE::kMetaDataType_XMPBinary,
				0, 
				MF::BinaryData(newAssetMediaInfo->GetXMPString()->c_str(), newAssetMediaInfo->GetXMPString()->length()),
				true,
				true,
				&requestID);
		}

		// send save msg.
		PL::AssetMediaInfoList assetMediaInfoList;
		assetMediaInfoList.push_back(newAssetMediaInfo);
		SRProject::GetInstance()->GetAssetLibraryNotifier()->SaveAssetMediaInfo(
			ASL::Guid::CreateUnique(),
			ASL::Guid::CreateUnique(),
			assetMediaInfoList,
			PL::AssetItemPtr());
	}
}

void ApplyMarkersAtStartTime(PL::UnassociatedMetadataList const& inUnassociatedMetadataList)
{
	ISRPrimaryClipPlaybackRef  loggingClip = ModulePicker::GetInstance()->GetLoggingClip();
	if (loggingClip == NULL)
		return;

	BE::ISequenceRef curSequence = loggingClip->GetBESequence();
	if (curSequence == NULL)
	{
		return;
	}

	ML::IPlayerRef player;
	ASL::Result result = MZ::GetSequencePlayer(curSequence, MZ::GetProject(), ML::kPlaybackPreference_AudioVideo, player);
	if(!ASL::ResultSucceeded(result) || player == NULL)
	{
		ASL_TRACE("HSL.SequencePlayer", 1, 
			"Create(): MZ::GetSequencePlayer() failed with result: "
			<< result);
		return;
	}
	// Current CTI position
	dvamediatypes::TickTime playerPosition = player->GetCurrentPosition();

	// Current Video FrameRate
	dvamediatypes::FrameRate videoFrameRate = curSequence->GetVideoFrameRate();

	// Obtain the MasterClip for the currently opened sequence so we can add our
	// new marker list to it
	PL::ClipDurationMap clipDurationMap;
	clipDurationMap.SetSequence(curSequence);
	dvamediatypes::TickTime clipStart;
	dvamediatypes::TickTime clipDuration;
	BE::IMasterClipRef masterClip = clipDurationMap.GetClipStartAndDurationFromSequenceTime(playerPosition, clipStart, clipDuration);
	dvamediatypes::TickTime clipEnd = clipStart + clipDuration;

	ISRPrimaryClipRef primaryClip(loggingClip);
	XMPText xmp;
	ASL::String clipName;
	if ( primaryClip != NULL )
	{
		xmp = primaryClip->GetAssetMediaInfo()->GetXMPString();
		clipName = MZ::BEUtilities::IsEAMasterClip(masterClip) ? 
					primaryClip->GetAssetItem()->GetAssetName() : 
					primaryClip->GetAssetItem()->GetMediaPath();
	}

	GuidToCWMarkerMap unassociatedMarkers;
	BuildAvailableUnassociatedMarkers(
		inUnassociatedMetadataList, xmp, masterClip, videoFrameRate, clipDuration, clipName, unassociatedMarkers);

	CottonwoodMarkerList newMarkerList;

	ISRPrimaryClipPlaybackRef primaryClipPlayback = ModulePicker::GetInstance()->GetCurrentModuleData();
	SRMarkersList markersList = primaryClipPlayback->GetMarkersList();
	DVA_ASSERT (markersList.size() == 1);
    PL::ISRMarkersRef masterClipMarkers = markersList[0];

	for (GuidToCWMarkerMap::const_iterator it = unassociatedMarkers.begin(); 
		it != unassociatedMarkers.end(); 
		it++)
	{
		if (masterClipMarkers->HasMarker((*it).second.GetGUID()))
		{
#if ALLOW_REAPPLYMARKER
			ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerUpdatedMsg, (*it).second.GetName()));
#else
			ML::SDKErrors::SetSDKWarningString(
				dvacore::ZString(kMarkerHaveExistedMsg, 
				(*it).second.GetName(), 
				clipName));
			continue;
#endif
		}
		newMarkerList.push_back((*it).second);
	}

	if (NULL != masterClip)
	{
		if (!newMarkerList.empty())
		{
			AssociateMarkers(newMarkerList, masterClip, true);
		}
	}
}

void ApplyMarkersAtCurrentPlayerPosition(PL::UnassociatedMetadataList const& inUnassociatedMetadataList)
{
	ISRPrimaryClipPlaybackRef  loggingClip = ModulePicker::GetInstance()->GetLoggingClip();
	if (loggingClip == NULL)
		return;

	BE::ISequenceRef curSequence = loggingClip->GetBESequence();
	if (curSequence == NULL)
	{
		return;
	}

	ML::IPlayerRef player;
	ASL::Result result = MZ::GetSequencePlayer(curSequence, MZ::GetProject(), ML::kPlaybackPreference_AudioVideo, player);
	if(!ASL::ResultSucceeded(result) || player == NULL)
	{
		ASL_TRACE("HSL.SequencePlayer", 1, 
			"Create(): MZ::GetSequencePlayer() failed with result: "
			<< result);
		return;
	}
	// Current CTI position
	dvamediatypes::TickTime playerPosition = player->GetCurrentPosition();

	// Current Video FrameRate
	dvamediatypes::FrameRate videoFrameRate = curSequence->GetVideoFrameRate();

	// Obtain the MasterClip for the currently opened sequence so we can add our
	// new marker list to it
	PL::ClipDurationMap clipDurationMap;
	clipDurationMap.SetSequence(curSequence);
	dvamediatypes::TickTime clipStart;
	dvamediatypes::TickTime clipDuration;
	BE::IMasterClipRef masterClip = clipDurationMap.GetClipStartAndDurationFromSequenceTime(playerPosition, clipStart, clipDuration);
	dvamediatypes::TickTime clipEnd = clipStart + clipDuration;

	ISRPrimaryClipPlaybackRef primaryClipPlayback = ModulePicker::GetInstance()->GetCurrentModuleData();
	SRMarkersList markersList = primaryClipPlayback->GetMarkersList();
	DVA_ASSERT (markersList.size() == 1);
	PL::ISRMarkersRef masterClipMarkers = markersList[0];

	ASL::String clipName;
	ISRPrimaryClipRef primaryClip(loggingClip);
	if ( primaryClip != NULL )
	{
		clipName = MZ::BEUtilities::IsEAMasterClip(masterClip) ?
					primaryClip->GetAssetItem()->GetAssetName() :
					primaryClip->GetAssetItem()->GetMediaPath();
	}

	MarkerTrack allMarkers;
	BOOST_FOREACH(const PL::UnassociatedMetadataPtr& unassociatedMetadata, inUnassociatedMetadataList)
	{
		// Collect marker list from unassociated metadata
		SRUnassociatedMetadataRef srUnassociatedMetadata = NULL;
		if (unassociatedMetadata->mXMP != NULL)
		{
			srUnassociatedMetadata = SRFactory::CreateSRUnassociatedMetadata(unassociatedMetadata->mMetadataId, unassociatedMetadata->mMetadataPath, unassociatedMetadata->mXMP);
		}
		else
		{
			srUnassociatedMetadata = SRProject::GetInstance()->FindSRUnassociatedMetadata(unassociatedMetadata->mMetadataId);		
		}
		if (srUnassociatedMetadata == NULL)
		{
			continue;
		}

		PL::ISRUnassociatedMarkersRef srMarkers = srUnassociatedMetadata->GetMarkers();
		if (srMarkers == NULL)
		{
			continue;
		}

		MarkerTrack markerTrack = srMarkers->GetMarkers();
		if (markerTrack.empty())
		{
			continue;
		}

		for (MarkerTrack::iterator it = markerTrack.begin(); it != markerTrack.end(); ++it)
		{
			CottonwoodMarker marker = (*it).second;

			if (!unassociatedMetadata->mIsApplyAll &&
				unassociatedMetadata->mIncludedMarkerIDs.find(marker.GetGUID()) == unassociatedMetadata->mIncludedMarkerIDs.end())
			{
				continue;
			}

			if (masterClipMarkers->HasMarker(marker.GetGUID()))
			{
#if ALLOW_REAPPLYMARKER
				ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerUpdatedMsg, marker.GetName()));
#else
				ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerHaveExistedMsg, marker.GetName(), clipName));
				continue;
#endif
			}

			allMarkers.insert(std::make_pair(marker.GetStartTime(), marker));
		}
	}

	if (allMarkers.empty())
	{
		return;
	}

	MarkerTrack::iterator firstTrackItr = allMarkers.begin();
	CottonwoodMarker firstMarker = (*firstTrackItr).second;
	dvamediatypes::TickTime firstMarkerOrigStartTime = firstMarker.GetStartTime();
	firstMarkerOrigStartTime.AlignToFrame(videoFrameRate);

	CottonwoodMarkerList newMarkerList;
	// Walk each marker and adjust start time value to be indexed from current CTI timecode
	// on open asset
	for (MarkerTrack::iterator trackItr = allMarkers.begin(); trackItr != allMarkers.end(); ++trackItr)
	{
		CottonwoodMarker marker = (*trackItr).second;

		dvamediatypes::TickTime markerStartTime = marker.GetStartTime();
		markerStartTime.AlignToFrame(videoFrameRate);
		dvamediatypes::TickTime markerEndTime = markerStartTime + marker.GetDuration();
		markerEndTime.AlignToFrame(videoFrameRate);

		if (markerStartTime == firstMarkerOrigStartTime)
		{
			// First marker gets assigned the current player position
			marker.SetStartTime(playerPosition);
			dvamediatypes::TickTime newMarkerDuration = marker.GetDuration();

			if (playerPosition + newMarkerDuration > clipEnd)
			{
				marker.SetDuration(clipEnd - playerPosition);
			}
			else
			{
				marker.SetDuration(newMarkerDuration);
			}
		}
		else
		{
			// Each subsequent marker is assigned a relative offset to the first marker
			// maintaining its distance from the first marker from the unassociated metadata.
			// Any markers that begin either before or after the clip's length are not added.
			dvamediatypes::TickTime newStartTimeOffset = dvamediatypes::kTime_Zero;
			newStartTimeOffset = markerStartTime - firstMarkerOrigStartTime;

			// If marker OUT Point is beyond range of masterclip being applied to, adjust
			// the marker OUT Point to equal the end of the masterclip.
			if ((playerPosition + newStartTimeOffset + marker.GetDuration()) > clipEnd)
			{
				// Adjust marker duration to equal the distance from the marker IN Point to the
				// End of the masterclip
				marker.SetDuration(clipEnd - playerPosition - newStartTimeOffset);
			}


			if ((playerPosition + newStartTimeOffset) < clipStart ||
				(playerPosition + newStartTimeOffset) > clipEnd)
			{
				// Start point of marker either starts before or after the
				// current clip range, don't add marker
				ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kMarkerFailToAddMsg, marker.GetName(), clipName));
				continue;
			}
			else
			{
				marker.SetStartTime(playerPosition + newStartTimeOffset);
			}
		}

		if (IsZeroDurationSubClipMarker(marker))
		{
			ML::SDKErrors::SetSDKWarningString(dvacore::ZString(kZeroDurationSubClipMarkerMsg, marker.GetName(), clipName));
			continue;
		}

		// Add marker with updated start time to our new MarkerList
		newMarkerList.push_back(marker);
	}

	if ( NULL != masterClip)
	{
		if (!newMarkerList.empty())
		{
			AssociateMarkers(newMarkerList, masterClip, true);
		}
	}
}

class AsyncCopyRequest;
typedef boost::shared_ptr<AsyncCopyRequest> AsyncCopyRequestPtr;

static std::map<ASL::Guid, AsyncCopyRequestPtr> sTransferRequests;

static const ASL::String& kTransferExecutorName = ASL_STR("transfer_executor");

static const char* kCreateFolderFailMsg = "$$$/Prelude/SRLibrarySupport/CopyError/CreateDirFailed=Failed to create the folder @0.";

static dvacore::threads::AsyncThreadedExecutorPtr GetTransferExecutor()
{
	dvacore::threads::AsyncThreadedExecutorPtr executor = threads::GetExecutor(kTransferExecutorName);
	// Must use only one thread to ensure order for several async calls.
	return (executor != NULL) ? executor : threads::CreateAndRegisterExecutor(kTransferExecutorName, 1, true);
}

typedef std::pair<ASL::String, ASL::UInt64> PathSizePair;
typedef std::vector<PathSizePair> PathSizeVector;
class CopyingItemInfo
{
public:
	ASL::UInt64			mSize;
	TransferItemPtr		mTransferItem;
	// Only used by folder copy. It includes all children path and size pairs.
	PathSizeVector		mCopyList;
	ASL::String			mErrMsg;
	ASL::String			mDestinationRoot;
	ASL::String			mNewName;
	enum CopyStatus{kCopyStatus_NotStart, kCopyStatus_Canceled, kCopyStatus_Success, kCopyStatus_Failed, };
	CopyStatus			mCopyStatus;

	CopyingItemInfo()
		: mSize(0)
		, mCopyStatus(kCopyStatus_NotStart)
	{
	}

	void AppendErrMsg(const ASL::String& inErrStr)
	{
		if (!mErrMsg.empty())
		{
			mErrMsg += ASL_STR("\n");
		}
		mErrMsg += inErrStr;
	}
};
typedef std::vector<CopyingItemInfo> CopyingItems;

class AsyncCopyRequest : public IngestTask::ThreadProcess
{
public:
	AsyncCopyRequest(
		const ASL::Guid& inBatchID,
		const TransferItemList& inItemList,
		TransferOptionType inOption)
		: mBatchID(inBatchID)
		, mItemList(inItemList)
		, mOption(inOption)
		, mDoneCount(0)
		, mTotalCount(0)
	{
	}

	void RequestFinished(bool inSuccess, const TransferResultList& inResultList)
	{
		// This function should be called in main thread
		DVA_ASSERT(ASL::ThreadManager::CurrentThreadIsMainThread());

		SRProject::GetInstance()->GetAssetLibraryNotifier()->TransferStatus(
			mBatchID,
			ASL::Guid::CreateUnique(),
			inSuccess ? ASL::kSuccess : (IsCanceled() ? ASL::eUserCanceled : ASL::ResultFlags::kResultTypeFailure),
			inResultList);

		sTransferRequests.erase(mBatchID);
		if (sTransferRequests.empty())
		{
			threads::UnregisterExecutor(kTransferExecutorName);
		}
	}

	void NotifyFinished(bool inSuccess, const CopyingItems& inCopyingItems)
	{
		TransferResultList resultList;
		bool isCanceled = IsCanceled();
		BOOST_FOREACH (const CopyingItemInfo& itemInfo, inCopyingItems)
		{
			switch (itemInfo.mCopyStatus)
			{
			case CopyingItemInfo::kCopyStatus_NotStart:
				DVA_ASSERT(isCanceled);
				// break; // Do NOT break for NotStart should have same behavior with Canceled
			case CopyingItemInfo::kCopyStatus_Canceled:
				resultList.push_back(TransferResultPtr(new TransferResult(
					itemInfo.mTransferItem->mSrcPath,
					GetFullDestPath(itemInfo.mTransferItem->mDstPath),
					ASL::eUserCanceled,
					dvacore::ZString("$$$/Prelude/SRLibrarySupport/CopyError/UserCanceled=Copying from @0 to @1 is canceled.", itemInfo.mTransferItem->mSrcPath, GetFullDestPath(itemInfo.mTransferItem->mDstPath)))));
				break;
			case CopyingItemInfo::kCopyStatus_Success:
				resultList.push_back(TransferResultPtr(new TransferResult(
					itemInfo.mTransferItem->mSrcPath,
					GetFullDestPath(itemInfo.mTransferItem->mDstPath),
					ASL::kSuccess)));
				break;
			case CopyingItemInfo::kCopyStatus_Failed:
				resultList.push_back(TransferResultPtr(new TransferResult(
					itemInfo.mTransferItem->mSrcPath,
					GetFullDestPath(itemInfo.mTransferItem->mDstPath),
					ASL::ResultFlags::kResultTypeFailure,
					itemInfo.mErrMsg)));
				break;
			default:
				break;
			}
		}
		ASL::AsyncCallFromMainThread(boost::bind(&AsyncCopyRequest::RequestFinished, this, inSuccess, resultList));
	}

	virtual bool EnsureSubDirectory(ASL::String const& inPath) = 0;

	virtual ASL::Result DoTransfer(
							ASL::String const& inSrcPath,
							ASL::String const& inDstPath,
							CopyAction inAction,
							ASL::UInt64 inFileSize) = 0;

	virtual ASL::String CombinePaths(ASL::String const & inDirPath, ASL::String const & inFilePath) = 0;

	// this is to provide server + remote folder in ftp case
	virtual ASL::String GetFullDestPath(ASL::String const& inPath) = 0;

	virtual void Process()
	{
		CopyingItems copyingItems;
		bool copySuccess = true;

		if (!CanContinue())
		{
			NotifyFinished(false, copyingItems);
			return;
		}

		const size_t itemCount = mItemList.size();
		copyingItems.resize(itemCount);

		mDoneCount = 0;
		mTotalCount = 0;

		// prepare copying info and count total size
		mTotalCount = PrepareCopyingInfo(copyingItems);

		CopyAction originalCopyAction = (mOption == kTransferOptionType_Overwrite) ? kCopyAction_Replaced : kCopyAction_Copied;
		BOOST_FOREACH (CopyingItemInfo& itemInfo, copyingItems)
		{
			// For failed item on prepare, we didn't count its size
			if (itemInfo.mCopyStatus != CopyingItemInfo::kCopyStatus_NotStart)
			{
				copySuccess = false;
				continue;
			}

			// If src is dir, should copy every files in it and create empty folders.
			if (ASL::PathUtils::IsDirectory(itemInfo.mTransferItem->mSrcPath))
			{
				ASL::String childRoot = CombinePaths(itemInfo.mDestinationRoot, itemInfo.mNewName);
				if (!EnsureSubDirectory(childRoot))
				{
					copySuccess = false;
					itemInfo.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
					itemInfo.AppendErrMsg(dvacore::ZString(kCreateFolderFailMsg, GetFullDestPath(childRoot)));
					continue;
				}

				BOOST_FOREACH (const PathSizePair& pathSizePair, itemInfo.mCopyList)
				{
					if (!CanContinue())
					{
						NotifyFinished(false, copyingItems);
						return;
					}

					const ASL::String& srcPath = pathSizePair.first;
					ASL::String relativePath = Utilities::GetRelativePath(itemInfo.mTransferItem->mSrcPath, srcPath);
					ASL::String dest = CombinePaths(childRoot, relativePath);
					ASL::String destParentFolder = ASL::PathUtils::GetFullDirectoryPart(dest);
					if (!EnsureSubDirectory(destParentFolder))
					{
						copySuccess = false;
						itemInfo.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
						itemInfo.AppendErrMsg(dvacore::ZString(kCreateFolderFailMsg, GetFullDestPath(destParentFolder)));
						continue;
					}

					CopyAction copyAction = originalCopyAction;
					ASL::Result result = DoTransfer(
						srcPath,
						dest,
						copyAction,
						pathSizePair.second);
					mDoneCount += pathSizePair.second;
					if (ASL::ResultFailed(result))
					{
						copySuccess = false;
						itemInfo.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
						itemInfo.AppendErrMsg(IngestUtils::ReportStringOfCopyResult(srcPath, GetFullDestPath(dest), result));
					}
				}
			}
			// If src is file, copy it.
			else
			{
				if (!CanContinue())
				{
					NotifyFinished(false, copyingItems);
					return;
				}

				ASL::String dest = CombinePaths(itemInfo.mDestinationRoot, itemInfo.mNewName);
				ASL::String destParentFolder = ASL::PathUtils::GetFullDirectoryPart(dest);
				if (!EnsureSubDirectory(destParentFolder))
				{
					copySuccess = false;
					itemInfo.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
					itemInfo.mErrMsg = dvacore::ZString(kCreateFolderFailMsg, GetFullDestPath(destParentFolder));
					continue;
				}
				CopyAction copyAction = originalCopyAction;
				ASL::Result result = DoTransfer(
					itemInfo.mTransferItem->mSrcPath,
					dest,
					copyAction,
					itemInfo.mSize);
				mDoneCount += itemInfo.mSize;
				if (ASL::ResultFailed(result))
				{
					copySuccess = false;
					itemInfo.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
					itemInfo.mErrMsg = IngestUtils::ReportStringOfCopyResult(itemInfo.mTransferItem->mSrcPath, GetFullDestPath(dest), result);
				}
			}
			if (itemInfo.mCopyStatus == CopyingItemInfo::kCopyStatus_NotStart)
			{
				itemInfo.mCopyStatus = IsCanceled() ?
									CopyingItemInfo::kCopyStatus_Canceled :
									CopyingItemInfo::kCopyStatus_Success;
			}
		}

		NotifyFinished(copySuccess, copyingItems);
	}

	ASL::UInt64 PrepareCopyingInfo(std::vector<CopyingItemInfo>& ioCopyingItems)
	{
		ASL::UInt64 outTotalCount = 0;
		const size_t itemCount = ioCopyingItems.size();
		for (size_t i = 0; i < itemCount; ++i)
		{
			CopyingItemInfo& info = ioCopyingItems[i];
			info.mTransferItem = mItemList[i];

			if (info.mTransferItem->mIsDstParent)
			{
				info.mDestinationRoot = ASL::PathUtils::AddTrailingSlash(info.mTransferItem->mDstPath);
				ASL::String srcPath = ASL::PathUtils::RemoveTrailingSlash(info.mTransferItem->mSrcPath);
				info.mNewName = ASL::PathUtils::GetFullFilePart(srcPath);
			}
			else
			{
				ASL::String dstPath = ASL::PathUtils::RemoveTrailingSlash(info.mTransferItem->mDstPath);
				info.mNewName = ASL::PathUtils::GetFullFilePart(dstPath);
				info.mDestinationRoot = ASL::PathUtils::GetFullDirectoryPart(dstPath);
			}

			ASL::String& srcPath = info.mTransferItem->mSrcPath;
			// If src is invalid or not exist, indicated as error.
			if (!ASL::PathUtils::IsValidPath(srcPath))
			{
				info.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
				info.mErrMsg = dvacore::ZString("$$$/Prelude/SRLibrarySupport/CopyError/InvalidPath=The source path @0 is invalid.", srcPath);
			}
			else if(!ASL::PathUtils::ExistsOnDisk(srcPath))
			{
				info.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
				info.mErrMsg = dvacore::ZString("$$$/Prelude/SRLibrarySupport/CopyError/NotExistPath=The source path @0 doesn't exist.", srcPath);
			}
			// If src is directory, should get its children and count total size.
			else if (ASL::PathUtils::IsDirectory(srcPath))
			{
				ASL::PathnameList  childrenPaths;
				Utilities::ExtendFolderPaths(srcPath, childrenPaths);
				BOOST_FOREACH (const ASL::String& childPath, childrenPaths)
				{
					if (!ASL::PathUtils::IsDirectory(childPath))
					{
						ASL::UInt64 outSize = 0;
						ASL::File::SizeOnDisk(childPath, outSize);
						info.mCopyList.push_back(PathSizePair(childPath, outSize));
						info.mSize += outSize;
					}
					else
					{
						// Only empty directory will be added into childrenPaths by MZ::Utilities::ExtendFolderPaths
						ASL::String const& childRoot = CombinePaths(info.mDestinationRoot, info.mNewName);
						ASL::String const& relativePath = Utilities::GetRelativePath(info.mTransferItem->mSrcPath, childPath);
						ASL::String const& dest = CombinePaths(childRoot, relativePath);
						if (!EnsureSubDirectory(dest))
						{
							info.mCopyStatus = CopyingItemInfo::kCopyStatus_Failed;
							info.AppendErrMsg(dvacore::ZString(kCreateFolderFailMsg, GetFullDestPath(dest)));
							continue;
						}
					}
				}
			}
			// If src is file, should only get its size.
			else
			{
				ASL::UInt64 outSize = 0;
				ASL::File::SizeOnDisk(srcPath, info.mSize);
			}
			outTotalCount += info.mSize;
		}
		return outTotalCount;
	}

protected:
	ASL::Guid			mBatchID;
	TransferItemList	mItemList;
	TransferOptionType	mOption;
	ASL::UInt64			mDoneCount;
	ASL::UInt64			mTotalCount;
};

class AsyncFTPTransferRequest : public AsyncCopyRequest
{
	struct FTPTransferProgresData 
	{
		FTPTransferProgresData()
		{
			Reset();
		}

		void Reset()
		{
			mTotalProgress = 100;
			mCurrentProgress = 0;
			mLastProgress = 0;
		}

		boost::function<bool (ASL::UInt64)>		mUpdateProgress;
		ASL::UInt64								mTotalProgress;
		ASL::UInt64								mCurrentProgress;
		ASL::UInt64								mLastProgress;
	};

public:
	AsyncFTPTransferRequest(
		const ASL::Guid& inBatchID,
		const TransferItemList& inItemList,
		TransferOptionType inOption,
		FTPSettingsInfo const& inFTPSettingsInfo)
		:
		AsyncCopyRequest(inBatchID, inItemList, inOption),
		mFTPSettings(inFTPSettingsInfo)
	{
	}

	bool IsValidPath(ASL::String const& inPath)
	{
		return (!inPath.empty() && inPath != ASL_STR("\\") && inPath != ASL_STR("/"));
	}
	
	virtual bool EnsureSubDirectory(ASL::String const& inPath)
	{
		// on mac, make dir will fail if the folder exists, so we check it on disk first.
		if (IsValidPath(inPath) && !FTPUtils::FTPExistOnDisk(mFTPConnection, inPath, true))
		{
			return FTPUtils::FTPMakeDir(mFTPConnection, inPath);
		}

		return true;
	}

	virtual ASL::String CombinePaths(ASL::String const & inDirPath, ASL::String const & inFilePath)
	{
		ASL::String const& dirAdjusted = MZ::Utilities::AdjustToForwardSlash(inDirPath);
		ASL::String const& filePathAdjusted = MZ::Utilities::AdjustToForwardSlash(inFilePath);

		return !IsValidPath(dirAdjusted) ?
				filePathAdjusted : 
				MZ::Utilities::AddTrailingForwardSlash(dirAdjusted) + filePathAdjusted;
	}

	virtual ASL::String GetFullDestPath(ASL::String const& inPath)
	{
		return MZ::Utilities::AddTrailingForwardSlash(mFTPSettings.mServerName) + inPath;
	}

	virtual ASL::Result DoTransfer(
		ASL::String const& inSrcPath,
		ASL::String const& inDstPath,
		CopyAction inAction,
		ASL::UInt64 inFileSize)
	{
		if (inAction != kCopyAction_Replaced && FTPUtils::FTPExistOnDisk(mFTPConnection, inDstPath, false))
		{
			return ASL::eFileAlreadyExists;
		}

		FTPTransferProgresData progressData;
		progressData.mUpdateProgress = boost::bind(
									&AsyncFTPTransferRequest::OnProgressUpdate, 
									this, 
									_1,
									inFileSize,
									&progressData);
		return FTPUtils::FTPUpload(mFTPConnection, inDstPath, inSrcPath, &progressData) ? 
							ASL::kSuccess : ASL::eAccessIsDenied;
	}

	virtual void Process()
	{
		if (!FTPUtils::CreateFTPConnection(
						mFTPConnection,
						mFTPSettings.mServerName,
						mFTPSettings.mPort,
						ASL_STR(""),
						mFTPSettings.mUserName,
						mFTPSettings.mPassword))
		{
			TransferResultList resultList;
			BOOST_FOREACH (const TransferItemPtr& item, mItemList)
			{
				resultList.push_back(TransferResultPtr(new TransferResult(
					item->mSrcPath,
					item->mDstPath,
					ASL::eAccessIsDenied,
					dvacore::ZString("$$$/Prelude/SRLibrarySupport/FTPTransfer/ConnectionFailed=Failed to connect to @0.", mFTPSettings.mServerName))));
			}
			ASL::AsyncCallFromMainThread(boost::bind(&AsyncCopyRequest::RequestFinished, this, false, resultList));

			return;
		}

		AsyncCopyRequest::Process();

		FTPUtils::CloseFTPConncection(mFTPConnection);
	}

private:
	bool OnProgressUpdate(ASL::UInt64 inSteps, ASL::UInt64 inFileSize, FTPTransferProgresData* pProgressData)
	{
		if (inSteps != 0 && pProgressData)
		{
			double percent = 
				(double)(mDoneCount + inFileSize*pProgressData->mCurrentProgress/pProgressData->mTotalProgress)/(double)mTotalCount;
			double last_percent = 
				(double)(mDoneCount + inFileSize*pProgressData->mLastProgress/pProgressData->mTotalProgress)/(double)mTotalCount;

			if ((ASL::SInt32)(last_percent*100) != (ASL::SInt32)(percent*100))
			{
				ASL::AsyncCallFromMainThread(boost::bind(
					&IAssetLibraryNotifier::TransferProgress,
					SRProject::GetInstance()->GetAssetLibraryNotifier(),
					mBatchID,
					ASL::Guid::CreateUnique(),
					(ASL::SInt32)(percent*100)));

				pProgressData->mLastProgress = pProgressData->mCurrentProgress;
			}
		}

		return !CanContinue();
	}

private:
	FTPUtils::FTPConnectionDataPtr		mFTPConnection;
	FTPSettingsInfo						mFTPSettings;
};

class AsyncLocalTransferRequest : public AsyncCopyRequest
{
public:
	AsyncLocalTransferRequest(
		const ASL::Guid& inBatchID,
		const TransferItemList& inItemList,
		TransferOptionType inOption)
		:
		AsyncCopyRequest(inBatchID, inItemList, inOption)
	{
	}

	virtual bool EnsureSubDirectory(ASL::String const& inPath)
	{
		return dvacore::utility::FileUtils::EnsureDirectoryExists(inPath);
	}

	virtual ASL::String CombinePaths(ASL::String const & inDirPath, ASL::String const & inFilePath)
	{
		return MZ::Utilities::CombinePaths(inDirPath, inFilePath);
	}

	virtual ASL::String GetFullDestPath(ASL::String const& inPath)
	{
		return inPath;
	}

	virtual ASL::Result DoTransfer(
		ASL::String const& inSrcPath,
		ASL::String const& inDstPath,
		CopyAction inAction,
		ASL::UInt64 inFileSize)
	{
		ASL::String dstPath = inDstPath;
		return IngestUtils::SmartCopyFileWithProgress(
			inSrcPath,
			dstPath,
			inAction,
			boost::bind(&AsyncLocalTransferRequest::OnProgressUpdate, this, inFileSize, _1, _2));
	}

private:
	bool OnProgressUpdate(ASL::UInt64 inFileSize, ASL::Float32 inLastPercentDone, ASL::Float32 inPercentDone)
	{
		double percent = (double)(mDoneCount + inPercentDone*inFileSize)/(double)mTotalCount;
		double last_percent = (double)(mDoneCount + inLastPercentDone*inFileSize)/(double)mTotalCount;

		if ((ASL::SInt32)(last_percent*100) != (ASL::SInt32)(percent*100))
		{
			ASL::AsyncCallFromMainThread(boost::bind(
				&IAssetLibraryNotifier::TransferProgress,
				SRProject::GetInstance()->GetAssetLibraryNotifier(),
				mBatchID,
				ASL::Guid::CreateUnique(),
				(ASL::SInt32)(percent*100)));
		}

		return CanContinue();
	}

};

}


namespace SRLibrarySupport
{

	/*
	**
	*/
	ASL::Result ImportXMPImpBackDoor(
		const ASL::String& inFilePath,
		MZ::ImportResultVector& outFilesFailure,
		CustomMetadata& outCustomMetadata,
		XMPText outXMPBuffer)
	{
		ASL::Result result = ASL::kSuccess;

		//	We import file first, so that we can get necessary info of this file
		MZ::ImportResultVector failureVector;

		// Ignore audio, should not ignore XMP
		BE::IMasterClipRef masterClip = BEUtilities::ImportFile(inFilePath, BE::IProjectRef(), failureVector, false, false); 

		BE::IMasterClipRef masterClipInProject;
		if (masterClip == NULL)
		{
			if (failureVector.empty())
			{
				result = MZ::eAppUnSupportFormat;
			}
			else
			{
				result = failureVector.front().first;
			}
		}
		else if (ASL::ResultFailed(IsSupportedMedia(masterClip)))
		{
			result = MZ::eAppUnSupportFormat;
		}
		else
		{
			BE::IProjectRef project = MZ::GetProject();
			if (project)
			{
				masterClipInProject = MZ::BEUtilities::AddMasterClipToProjectInMainThread(
															project,
															project->GetRootProjectItem(),
															masterClip,
															false);
				if (!masterClipInProject)
				{
					DVA_ASSERT_MSG(0, "Add masterClip failed!");
					masterClipInProject = masterClip;
				}
			}
			else
			{
				DVA_ASSERT_MSG(0, "No project!");
				result = ASL::eUnknown;
			}
		}

		if (ASL::ResultSucceeded(result))
		{
			// Get custom Metadata
			outCustomMetadata = Utilities::GetCustomMedadata(masterClipInProject);

			// Get XMP Data 
			ASL::String errorInfo;
			SRLibrarySupport::ReadXMPFromCache(inFilePath, outXMPBuffer, errorInfo);
		}

		if ( !ASL::ResultSucceeded(result))
		{
			outFilesFailure.push_back(std::make_pair(result, inFilePath));
		}

		return result;
	}


/*
**
*/
bool OpenLibraryItem(AssetItemPtr const& inAssetItem)
{
	bool result(false);
	if (inAssetItem)
	{
		AssetMediaType assetType = inAssetItem->GetAssetMediaType();
		switch(assetType)
		{
			case kAssetLibraryType_MasterClip:
			case kAssetLibraryType_SubClip:
				result = OpenLoggingClip(inAssetItem);
				break;
			case kAssetLibraryType_RoughCut:
				result = OpenRoughCut(inAssetItem);
				break;
			default:
				DVA_ASSERT_MSG(false, "Unknown format!");
				break; 
		}
	}
	return result;
}

/*
**
*/
bool SaveRoughCutToDisk(AssetMediaInfoPtr const& inAssetMediaInfo, AssetItemList const& inAssetItemList)
{
	ASL::String roughCutContent = PL::BuildRoughCutContent(inAssetMediaInfo, inAssetItemList.front());
	bool result = ASL::ResultSucceeded(SaveRoughCut(inAssetMediaInfo->GetMediaPath(), roughCutContent));
	return result;
}

/*
**
*/
bool OpenRoughCut(AssetItemPtr inRCAssetItem)
{
	bool result = false;
	if (inRCAssetItem == NULL || inRCAssetItem->GetAssetMediaType() != kAssetLibraryType_RoughCut)
	{
		DVA_ASSERT_MSG(0, "wrong parameter for open rough cut in SRLibrarySupport.");
		return false;
	}

	dvacore::UTF16String mediaPath = inRCAssetItem->GetMediaPath();
	AssetMediaInfoPtr assetMediainfo = SRProject::GetInstance()->GetAssetMediaInfo(mediaPath);
	if (ASL::PathUtils::ExistsOnDisk(mediaPath) || (assetMediainfo && !assetMediainfo->GetNeedSavePreCheck()))
	{
		result = ModulePicker::GetInstance()->ActiveModule(inRCAssetItem);
	}
	else
	{
		const dvacore::UTF16String& errorMsg = dvacore::ZString(
			"$$$/Prelude/SRLibrarySupport/SRRoughCutOffline=The Rough Cut you are attempting to work on is offline. Please locate your Rough Cut, bring it online and try it again.");
		SRUtilitiesPrivate::PromptForError(
			dvacore::ZString("$$$/Prelude/SRLibrarySupport/SRRoughCutOfflineCaption=Rough Cut Offline"),
			errorMsg);
	}

	return result;
}

/*
**
*/
ASL::Result OpenRoughCut(
	const PL::AssetItemPtr& inAssetItem,
	ASL::String outErrorInfo)
{
	{
		// Register to selection manager
		PL::AssetItemList itemList;
		itemList.push_back(inAssetItem);
		PL::SRProject::GetInstance()->GetAssetSelectionManager()->SetSelectedAssetItemList(itemList);
	}

	ASL::Result result = PL::SRLibrarySupport::OpenRoughCut(inAssetItem) ?
							ASL::kSuccess :
							ASL::ResultFlags::kResultTypeFailure;

	if (ASL::ResultFailed(result))
	{
		outErrorInfo =  
			dvacore::ZString("$$$/Prelude/PLCore/LibrarySupport/OpenRoughCutFail=Open rough cut failed");
	}

	return result;
}

void SyncRoughCutFileContent(AssetItemPtr& ioAssetItem)
{
	DVA_ASSERT(ioAssetItem->GetAssetMediaType() == kAssetLibraryType_RoughCut);

	// Get file content from file
	PL::AssetMediaInfoList assetMediaInfoList;
	PL::AssetItemList assetItemList;

	PL::AssetItemPtr assetItem;
	PL::AssetMediaInfoPtr assetMediaInfo;
	ASL::Result result = PL::LoadRoughCut(
								ioAssetItem->GetMediaPath(),
								ioAssetItem->GetAssetGUID(),
								assetMediaInfo,
								assetItem);
	assetItemList.push_back(assetItem);
	assetMediaInfoList.push_back(assetMediaInfo);

	if (ASL::ResultSucceeded(result))
	{
		ioAssetItem->SetSubAssetItemList(assetItemList.front()->GetSubAssetItemList());
	}
	else
	{
		ioAssetItem->SetSubAssetItemList(AssetItemList());
	}
}

/*
**
*/
bool OpenLoggingClip(AssetItemPtr const& inAssetItem)
{
	bool result(false);
	if (inAssetItem)
	{
		DVA_ASSERT(inAssetItem->GetAssetMediaType() == kAssetLibraryType_MasterClip || inAssetItem->GetAssetMediaType() == kAssetLibraryType_SubClip);
		
		if (inAssetItem->GetAssetMediaType() == kAssetLibraryType_MasterClip || inAssetItem->GetAssetMediaType() == kAssetLibraryType_SubClip)
		{
			dvacore::UTF16String mediaPath = inAssetItem->GetMediaPath();
			if (MZ::Utilities::IsEAMediaPath(mediaPath) || ASL::PathUtils::ExistsOnDisk(mediaPath))
			{
				result = ModulePicker::GetInstance()->ActiveModule(inAssetItem);	
			}
			else
			{
				const dvacore::UTF16String& errorMsg = dvacore::ZString(
					"$$$/Prelude/SRLibrarySupport/SRMediaOffline=The media you are attempting to work on is offline. Please locate your media, bring it online and try it again.");
				SRUtilitiesPrivate::PromptForError(
					dvacore::ZString("$$$/Prelude/SRLibrarySupport/SRMediaOfflineCaption=Media Offline"),
					errorMsg);
			}
		}
	}

	return result;
}

void OnWriteXMPToDiskFinished(
	ASL::String const& inMediaPath,
	ASL::Result inResult,
	ASL::SInt32 inRequestID)
{
	BE::RequestIDs requestIDs;
	requestIDs.insert(inRequestID);
	ASL::StationUtils::PostMessageToUIThread(
		MZ::kStation_PreludeProject,
		WriteXMPToDiskFinished(inResult, inMediaPath, requestIDs));
}

ASL::Result WriteXMPToDisk(
	ASL::String const& inMediaPath,
	XMPText inXmp,
	ImporterHost::WriteMetadataCompletionRoutine const& inWriteXMPToDiskCallbackFn,
	ASL::SInt32* outRequestID)
{
	XMPText newXMPText = inXmp;
	bool supportXMP = Utilities::PathSupportsXMP(inMediaPath);
	ASL::Result result(ASL::ResultFlags::kResultTypeFailure);

	bool isWritable = PL::Utilities::IsXMPWritable(inMediaPath);

	if (supportXMP && isWritable)
	{
		BE::IMediaMetaDataRef  mediaMetaData;
		BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), inMediaPath);
		if (projectItem != NULL)
		{
			BE::IMediaRef media = BE::MasterClipUtils::GetMedia(BE::ProjectItemUtils::GetMasterClip(projectItem));
			if (media != NULL)
			{
				mediaMetaData = BE::IMediaMetaDataRef(media->GetMediaInfo());
			}
		}

		if (mediaMetaData != NULL)
		{
			result = mediaMetaData->WriteMetaDatum(
				MF::BinaryData(),
				BE::kMetaDataType_XMPBinary,
				0, 
				MF::BinaryData(newXMPText->c_str(),newXMPText->length()),
				true,
				true,
				outRequestID);
		}
		else
		{
			result = ImporterHost::WriteMetadataToPath(
				ASL::PathUtils::ToNormalizedPath(inMediaPath), 
				MF::BinaryData(newXMPText->c_str(),newXMPText->length()),
				inWriteXMPToDiskCallbackFn,
				outRequestID);
		}
	}

	return result;
}

bool SaveXMPToDisk(AssetMediaInfoPtr const& inAssetMediaInfo, bool writeCacheFile)
{
	DVA_ASSERT(inAssetMediaInfo);

	ImporterHost::WriteMetadataCompletionRoutine writeXMPToDiskRoutine = 
		boost::bind(
		OnWriteXMPToDiskFinished,
		_1,
		_2,
		_3);

	ASL::SInt32 requestID(0);
	ASL::Result result = WriteXMPToDisk(inAssetMediaInfo->GetMediaPath(), inAssetMediaInfo->GetXMPString(), writeXMPToDiskRoutine, &requestID);

	if (ASL::ResultFailed(result) || requestID == 0)
	{
		return false;
	}
	else
	{
		WriteXMPToDiskCache::GetInstance()->RegisterWriting(
											inAssetMediaInfo->GetMediaPath(),
											inAssetMediaInfo->GetAssetMediaInfoGUID(), 
											*inAssetMediaInfo->GetXMPString(),
											requestID,
											writeCacheFile);
		return true;
	}
}

void InsertClipsToRoughCut(
	AssetItemList const & inAssetItemList,
	ISRPrimaryClipPlaybackRef inRCPlayBack,
	dvamediatypes::TickTime inInsertPos)
{
	ISRRoughCutRef rcClip(inRCPlayBack);
	ISRPrimaryClipRef rcPrimaryClip(inRCPlayBack);

	if (inRCPlayBack == NULL || rcClip == NULL || inAssetItemList.empty())
	{
		DVA_ASSERT_MSG(0, "InsertClipsToRoughCut with wrong parameter!");
		return;
	}

	AssetItemList offlineAssetList;
	AssetItemList appendAssetItemList;

	AssetItemList::const_iterator iter = inAssetItemList.begin();
	AssetItemList::const_iterator end = inAssetItemList.end();
	for (; iter != end; ++iter)
	{
		AssetItemPtr assetItem = *iter;
		AssetMediaType assetItemType = assetItem->GetAssetMediaType();
		//	Only online Logging Clip can be added to the Roughcut
		if (!PL::Utilities::IsAssetItemOffline(assetItem))
		{
			if (assetItemType == kAssetLibraryType_MasterClip || assetItemType == kAssetLibraryType_SubClip)
			{
				AssetItemPtr  rcAssetItem(new AssetItem(
					kAssetLibraryType_RCSubClip,				//	inType
					assetItem->GetMediaPath(),						//	inMediaPath
					ASL::Guid::CreateUnique(),						//	inGUID
					rcPrimaryClip->GetAssetItem()->GetAssetGUID(),	//	inParentGUID
					assetItem->GetAssetMediaInfoGUID(),				//	inAssetMediaInfoGUID
					assetItem->GetAssetName(),						//	inAssetName
					DVA_STR(""),									//	inAssetMetadata
					assetItem->GetInPoint(),						//	InPoint
					assetItem->GetDuration(),						//	inDuration 
					assetItem->GetCustomInPoint(),
					assetItem->GetCustomOutPoint(),
					ASL::Guid::CreateUnique()));					//	inClipItemGuid

				appendAssetItemList.push_back(rcAssetItem);
			}
		}
		else
		{
			offlineAssetList.push_back(assetItem);
		}
	}

	if (appendAssetItemList.size() > 0)
	{
		SRClipItems clipItems;
		Utilities::BuildClipItemFromLibraryInfo(appendAssetItemList, clipItems);

		dvamediatypes::TickTime changedDuration;
		rcClip->AddClipItems(clipItems, inInsertPos, &changedDuration);
	}

	if (offlineAssetList.size() > 0)
	{
		PromptAppendOfflineError(offlineAssetList);
	}

	if (offlineAssetList.size() < inAssetItemList.size())
	{
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			SilkRoadSubClipAppended(inRCPlayBack->GetBESequence()),
			true);
	}
}

/*
**
*/
ASL::Result InsertClipsToRoughCut(
	AssetItemList const & inAssetItemList,
	ISRPrimaryClipPlaybackRef inRCPlayBack,
	ActionResultList& outActionResultList,
	bool inInsertAhead)
{
	DVA_ASSERT(inRCPlayBack);
	DVA_ASSERT(inAssetItemList.size() > 0);

	ASL::Result result = ASL::ResultFlags::kResultTypeFailure;
	if (inRCPlayBack)
	{
		ISRRoughCutRef rcClip(inRCPlayBack);
		ISRPrimaryClipRef rcPrimaryClip(inRCPlayBack);

		// calculate insert position
		BE::ISequenceRef theSequence = inRCPlayBack->GetBESequence();

		ML::IPlayerRef player;
		ASL::Result result = MZ::GetSequencePlayer(
			theSequence, MZ::GetProject(), ML::kPlaybackPreference_AudioVideo, player);
		dvamediatypes::TickTime curPos(player->GetCurrentPosition());
		// edge cases, when we are at the end, adjust curPos to be in a track item
		if (curPos == theSequence->GetEnd() && curPos != dvamediatypes::kTime_Zero)
			curPos -= dvamediatypes::TickTime::TicksToTime(1);

		// which track item are you in?
		BE::ITrackGroupRef trackGroup = theSequence->GetTrackGroup(BE::kMediaType_Video);
		BE::IClipTrackRef clipTrack = trackGroup->GetClipTrack(0);

		BE::ITrackItemRef currentTrackItem = clipTrack->GetTrackItem(BE::kTrackItemType_Clip, curPos);

		dvamediatypes::TickTime insertPos = (inInsertAhead ? currentTrackItem->GetStart() : currentTrackItem->GetEnd());

		//	First, switch to rc module.
		ModulePicker::GetInstance()->ActiveModule(rcPrimaryClip->GetAssetItem());

		//	Second, insert Subclip to rc 
		result = ASL::kSuccess;
		AssetItemList::const_iterator iter = inAssetItemList.begin();
		AssetItemList::const_iterator end = inAssetItemList.end();
		AssetItemList offlineAssetList;
		AssetItemList appendAssetItemList;
		for (; iter != end; ++iter)
		{
			AssetItemPtr assetItem = *iter;
			AssetMediaType assetItemType = assetItem->GetAssetMediaType();
			//	Only online Logging Clip can be added to the Roughcut
			if (!PL::Utilities::IsAssetItemOffline(assetItem))
			{
				if(assetItemType == kAssetLibraryType_MasterClip || assetItemType == kAssetLibraryType_SubClip)
				{
					AssetItemPtr  rcAssetItem(new AssetItem(
						kAssetLibraryType_RCSubClip,				//	inType
						assetItem->GetMediaPath(),						//	inMediaPath
						ASL::Guid::CreateUnique(),						//	inGUID
						rcPrimaryClip->GetAssetItem()->GetAssetGUID(),	//	inParentGUID
						assetItem->GetAssetMediaInfoGUID(),				//	inAssetMediaInfoGUID
						assetItem->GetAssetName(),						//	inAssetName
						DVA_STR(""),									//	inAssetMetadata
						assetItem->GetInPoint(),						//	InPoint
						assetItem->GetDuration(),						//	inDuration 
						assetItem->GetCustomInPoint(),
						assetItem->GetCustomOutPoint(),
						ASL::Guid::CreateUnique()));					//	inClipItemGuid

					appendAssetItemList.push_back(rcAssetItem);
				}
			}
			else
			{
				offlineAssetList.push_back(assetItem);
			}
		}

		if (!appendAssetItemList.empty())
		{
			SRClipItems clipItems;
			Utilities::BuildClipItemFromLibraryInfo(appendAssetItemList, clipItems);

			dvamediatypes::TickTime changedDuration;
			if (rcClip->AddClipItems(clipItems, insertPos, &changedDuration))
			{
				result = ASL::kSuccess;
			}
		}

		if (!offlineAssetList.empty())
		{
			PromptAppendOfflineError(offlineAssetList);
		}

		if (offlineAssetList.size() < inAssetItemList.size())
		{
			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject,
				SilkRoadSubClipAppended(inRCPlayBack->GetBESequence()),
				true);
		}
	}

	return result;
}

/*
**
*/
ASL::Result AppendClipToRoughCut(
	AssetItemList const & inAssetItemList,
	ISRPrimaryClipPlaybackRef inRCPlayBack,
	ActionResultList& outActionResultList)
{
	DVA_ASSERT(inRCPlayBack);
	DVA_ASSERT(inAssetItemList.size() > 0);

	ASL::Result result = ASL::ResultFlags::kResultTypeFailure;
	if (inRCPlayBack)
	{
		ISRRoughCutRef rcClip(inRCPlayBack);
		ISRPrimaryClipRef rcPrimaryClip(inRCPlayBack);

		//	First, switch to rc module.
		ModulePicker::GetInstance()->ActiveModule(rcPrimaryClip->GetAssetItem());

		//	Second, Append Subclip to rc 
		result = ASL::ResultFlags::kResultTypeFailure;
		AssetItemList offlineAssetList;
		AssetItemList appendAssetItemList;
		AssetItemList::const_iterator iter = inAssetItemList.begin();
		AssetItemList::const_iterator end = inAssetItemList.end();
		for (; iter != end; ++iter)
		{
			AssetItemPtr assetItem = *iter;
			AssetMediaType assetItemType = assetItem->GetAssetMediaType();

			//	Only online Logging Clip can be added to the Roughcut
			if (!PL::Utilities::IsAssetItemOffline(assetItem))
			{
				if(assetItemType == kAssetLibraryType_MasterClip || assetItemType == kAssetLibraryType_SubClip)
				{

					AssetItemPtr  rcAssetItem(new AssetItem(
						kAssetLibraryType_RCSubClip,				//	inType
						assetItem->GetMediaPath(),						//	inMediaPath
						ASL::Guid::CreateUnique(),						//	inGUID
						rcPrimaryClip->GetAssetItem()->GetAssetGUID(),	//	inParentGUID
						assetItem->GetAssetMediaInfoGUID(),				//	inAssetMediaInfoGUID
						assetItem->GetAssetName(),						//	inAssetName
						DVA_STR(""),									//	inAssetMetadata
						assetItem->GetInPoint(),						//	InPoint
						assetItem->GetDuration(),						//	inDuration 
						assetItem->GetCustomInPoint(),					//  Custom InPoint
						assetItem->GetCustomOutPoint(),					//  Custom OutPoint
						ASL::Guid::CreateUnique()));					//	inClipItemGuid

					appendAssetItemList.push_back(rcAssetItem);
				}
			}
			else
			{
				offlineAssetList.push_back(assetItem);
			}
		}

		if (!appendAssetItemList.empty())
		{
			SRClipItems clipItems;
			Utilities::BuildClipItemFromLibraryInfo(appendAssetItemList, clipItems);
			if (rcClip->AppendClipItems(clipItems))
			{
				result = ASL::kSuccess;
			}
		}		

		if (offlineAssetList.size() < inAssetItemList.size())
		{
			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject,
				SilkRoadSubClipAppended(inRCPlayBack->GetBESequence()),
				true);
		}

		if (offlineAssetList.size() > 0)
		{
			PromptAppendOfflineError(offlineAssetList);
		}
	}

	return result;
}
/*
**
*/
bool IsValidDataForDrop(
	AssetItemList const& inAssetItemList,
	ISRPrimaryClipPlaybackRef inPrimaryClipPlayback)
{
	bool multipleItems = inAssetItemList.size() > 1 ? true : false;
	bool validItemList = IsValidAssetItemList(inAssetItemList);
	bool validData(true);
	//	We don't allow to drop mixed items.
	if (!validItemList)
	{
		validData = false;
	}

	if (validData)
	{
		ClipType clipType = Utilities::GetClipType(inPrimaryClipPlayback);
		AssetMediaType dropType = inAssetItemList.front()->GetAssetMediaType();

		switch(clipType)
		{
		case kClipType_Clip:
		case kClipType_Unknown:
			{
				if (multipleItems)
				{
					validData = false;
				}
				else
				{
					switch (dropType)
					{
					case kAssetLibraryType_MasterClip:
					case kAssetLibraryType_SubClip:
					case kAssetLibraryType_RoughCut:
						validData = true;
						break;
					default:
						validData = false;
						break;
					}
				}
			}
			break;
		case kClipType_RoughCut:
			{
				if (!multipleItems && (dropType == kAssetLibraryType_MasterClip || dropType == kAssetLibraryType_SubClip || dropType == kAssetLibraryType_RoughCut))
				{
					validData = true;
				}
				else
				{
					validData = true;
					AssetItemList::const_iterator itr = inAssetItemList.begin();
					AssetItemList::const_iterator itr_end = inAssetItemList.end();
					for ( ; itr!=itr_end; itr++)
					{
						if ((*itr)->GetAssetMediaType() != kAssetLibraryType_MasterClip && (*itr)->GetAssetMediaType() != kAssetLibraryType_SubClip)
						{
							validData = false;
							break;
						}
					}
				}
			}
			break;
		default:
			validData = false;
			break;

		}
	}

	return validData;
}

/*
**
*/
ASL::Result DeleteAssetItems(AssetItemList const & inAssetItemList, ActionResultList& outActionResultList)
{
	AssetItemList::const_iterator iter = inAssetItemList.begin();
	AssetItemList::const_iterator end = inAssetItemList.end();
	ASL::Result result = ASL::kSuccess;

	std::map<ASL::Guid, AssetItemList> subclipAssetItemListMap;
	for (; iter != end; ++iter)
	{
		AssetItemPtr assetItem = (*iter);
		if (assetItem->GetAssetMediaType() == kAssetLibraryType_SubClip && !MZ::Utilities::IsEAMediaPath(assetItem->GetMediaPath()))
		{
			// we need check the sub-clips which belong to the same clip.
			subclipAssetItemListMap[assetItem->GetAssetMediaInfoGUID()].push_back(assetItem);
		}
		else
		{
			ActionResultPtr actionResult(new ActionResult(assetItem->GetAssetGUID(), assetItem->GetMediaPath()));

			switch (assetItem->GetAssetMediaType())
			{
			case kAssetLibraryType_MasterClip:
				actionResult->mResult = DeleteMasterClip(assetItem);// DeleteMasterClip will always succeed.
				break;
			case kAssetLibraryType_SubClip:
				DVA_ASSERT(MZ::Utilities::IsEAMediaPath(assetItem->GetMediaPath()));
				actionResult->mResult = DeleteEASubClip(assetItem);// DeleteMasterClip will always succeed.
				break;
			case kAssetLibraryType_RoughCut:
				actionResult->mResult =DeleteRoughCut(assetItem);// DeleteRoughCut will always succeed.
				break;
			default:
				DVA_ASSERT_MSG(0, "Invalid Asset Type");
				break;
			}

			outActionResultList.push_back(actionResult);
		}
	}

	std::map<ASL::Guid, AssetItemList>::iterator it = subclipAssetItemListMap.begin();
	for (; it != subclipAssetItemListMap.end(); ++it)
	{
		ActionResultList subclipActionResultList;
		ASL::Result tempResult = DeleteSubClipsInSameClip(it->second, subclipActionResultList);
		if (tempResult != ASL::kSuccess)
		{
			result = ASL::ResultFlags::kResultTypeFailure;
		}
		outActionResultList.insert(outActionResultList.end(), subclipActionResultList.begin(), subclipActionResultList.end());
	}
	return result;
}

/*
**
*/
ASL::Result ImportXMPAndCustomMetadata(
				const ASL::String& inFilePath,
				MZ::ImportResultVector& outFilesFailure,
				CustomMetadata& outCustomMetadata,
				XMPText outXMPBuffer)
{
	return !SRAsyncCallAssembler::NeedImportXMPInMainThread(inFilePath) 
		?
		ImportXMPImpl(inFilePath, outFilesFailure, outCustomMetadata, outXMPBuffer)
		:
		SRAsyncCallAssembler::ImportXMPInMainThread(inFilePath, outFilesFailure, outCustomMetadata, outXMPBuffer, ImportXMPImpl);
}

/*
**
*/
ASL::Result ReadXMPFromCache(
			ASL::String const& inFilePath,
			XMPText outXMPBuffer,
			ASL::String& outErrorInfo)
{
	if (!WriteXMPToDiskCache::GetInstance()->GetCachedXMPData(inFilePath, *outXMPBuffer.get()))
	{
		return ReadXMPFromFile(inFilePath, outXMPBuffer, outErrorInfo);
	}

	return ASL::kSuccess;
}

/*
**
*/
ASL::Result ReadXMPFromFile(
			ASL::String const& inFilePath, 
			XMPText outXMPBuffer, 
			ASL::String& outErrorInfo)
{
	ASL::Result result(ASL::kSuccess);

	SXMPMeta xmpMeta;
	if (Utilities::PathSupportsXMP(inFilePath))
	{
		ISRMediaRef srMedia = SRProject::GetInstance()->GetSRMedia(inFilePath);
		MF::BinaryData metaDataBuffer;
		if (srMedia)
		{
			BE::IMediaMetaDataRef mediaMetaData = 
				BE::IMediaMetaDataRef(BE::MasterClipUtils::GetDeprecatedMedia(srMedia->GetMasterClip())->GetMediaInfo());

			if (mediaMetaData != NULL)
			{
				result = mediaMetaData->ReadMetaDatum(
					MF::BinaryData(),
					BE::kMetaDataType_XMPBinary,
					0,
					metaDataBuffer);
			}
			else
			{
				SXMPFiles	docMeta;
				SXMPMeta	xmpMeta;
				bool docFileOpen = docMeta.OpenFile(
					reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFilePath).c_str()), 
					kXMP_UnknownFile,
					kXMPFiles_OpenForRead | kXMPFiles_OpenUseSmartHandler | kXMPFiles_OpenLimitedScanning);

				if (docFileOpen)
				{
					docMeta.GetXMP(&xmpMeta);
					std::string buffer;
					xmpMeta.SerializeToBuffer(&buffer);
					metaDataBuffer = MF::BinaryData(buffer.c_str(), buffer.size());
				}

			}
		}
		else
		{
			bool canWriteMetadata;
			bool isDirty;
			result = ImporterHost::ReadMetadataFromPath(
				inFilePath, 
				false,
				metaDataBuffer,
				canWriteMetadata,
				isDirty);

		}

		try
		{
			xmpMeta.ParseFromBuffer(
				reinterpret_cast<XMP_StringPtr>(metaDataBuffer.GetData()),
				static_cast<XMP_StringLen>(metaDataBuffer.GetSize()));
		}
		catch (...)
		{
			result = ASL::ResultFlags::kResultTypeFailure;
		}
	}
	else
	{
		result = ASL::ResultFlags::kResultTypeFailure;
	}

	xmpMeta.SerializeToBuffer(outXMPBuffer.get(), kXMP_OmitPacketWrapper);

	return result;
}

/*
**
*/
ASL::Result ReadXMPFromMasterClip(
	const BE::IMasterClipRef& inMasterClip, 
	XMPText outXMPBuffer, 
	ASL::String& outErrorInfo)
{
	ASL::Result result(ASL::ResultFlags::kResultTypeFailure);

	SXMPMeta xmpMeta;
	if (inMasterClip)
	{
		BE::IMediaMetaDataRef mediaMetaData = BE::IMediaMetaDataRef(BE::MasterClipUtils::GetMediaInfo(inMasterClip));
		MF::BinaryData metaDataBuffer;
		if (mediaMetaData != NULL)
		{
			result = mediaMetaData->ReadMetaDatum(
				MF::BinaryData(),
				BE::kMetaDataType_XMPBinary,
				0,
				metaDataBuffer);
		}
		else
		{
			return result;
		}

		try
		{
			xmpMeta.ParseFromBuffer(
				reinterpret_cast<XMP_StringPtr>(metaDataBuffer.GetData()),
				static_cast<XMP_StringLen>(metaDataBuffer.GetSize()));
		}
		catch (...)
		{
			return result;
		}
	}

	xmpMeta.SerializeToBuffer(outXMPBuffer.get(), kXMP_OmitPacketWrapper);

	return result;
}

/*
**
*/
void RequestThumbnailAsyncImpl(
		ASL::String const& inImportFile,
		dvamediatypes::TickTime const& inClipTime,
		RequestThumbnailCallbackFn inRequestThumbnailCallbackFn)
{
	DVA_ASSERT(inRequestThumbnailCallbackFn != NULL);
	DVA_TRACE("RequestThumbnailAsyncImpl", 5, inImportFile);
	
	if (ASL::PathUtils::ExistsOnDisk(inImportFile))
	{
		BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), inImportFile);
		BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);

		if (!masterClip)
		{
			masterClip = PL::BEUtilities::ImportFile(inImportFile, 
				BE::IProjectRef(),
				true,	//  inNeedErrorMsg
				false,	//	inIgnoreAudio
				false);	//	inIgnoreMetadata
		}

		if (MZ::BEUtilities::IsAudioOnlyMasterClip(masterClip))
		{
			dvacore::UTF16String path =
				ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(ASL::kApplicationDirectory));
#if ASL_TARGET_OS_MAC
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("Contents"));
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("Frameworks"));
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("UIFramework.framework"));
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("Versions"));
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("A"));
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("Resources"));
#endif
			path = ASL::PathUtils::CombinePaths(path, ASL_STR("png"));
			path = ASL::PathUtils::CombinePaths(path, dvacore::utility::UTF8to16(UIF::kAudioWaveLargeDecal.c_str()));
			inRequestThumbnailCallbackFn(path, ASL::kSuccess);
			return;
		}
	}

	BE::MediaVector mediaVector;
	ASL::Result result = ASL::ResultFlags::kResultTypeFailure;
	if (ASL::PathUtils::ExistsOnDisk(inImportFile))
	{
		result = MZ::GetMediaVectorForThumbnail(inImportFile, mediaVector);
	}

	if (ASL::ResultSucceeded(result))
	{
		if (!mediaVector.empty())
		{
			MZ::SaveThumbnailToMediaCache(
				inImportFile,
				mediaVector,
				inRequestThumbnailCallbackFn,
				inClipTime);
				
			return;
		}
		else 
		{
			result = ASL::eUnknown;
		}
	}

	inRequestThumbnailCallbackFn(DVA_STR(""), result);
}

/*
**
*/
void Import(
		const ASL::Guid& inTaskID, 
		const ASL::Guid& inMsgID, 
		const ASL::String& inPath, 
		const ASL::Guid& inAssetItemID,
		const ASL::Guid& inBatchID,
		const ASL::Guid& inHostTaskID)
{
	if (TaskScheduler::IsImportTask(inHostTaskID))
	{ 
		ImportXMPElement element;
		element.mMessageID = inMsgID;
		element.mImportFile = inPath;
		element.mAssetItemID = inAssetItemID;
		element.mHostTaskID = inHostTaskID;
		element.mBatchID = inBatchID;

		ASL::StationUtils::PostMessageToUIThread(
			kStation_IngestMedia, 
			ImportXMPMessage(element), 
			true);
	}
	else
	{
		SRLibrarySupport::AsyncLoadingFileInWorkingThread(inTaskID, inMsgID, inPath, inAssetItemID, false);
	}
}

/*
**
*/
void RequestThumbnail(
				const ASL::Guid& inTaskID, 
				const ASL::Guid& inMsgID,
				ASL::String const& inFilePath,
				dvamediatypes::TickTime const& inClipTime,
				ASL::Guid const& inAssetItemID,
				ASL::Guid const& inBatchID,
				ASL::Guid const& inHostTaskID)
{
	if (TaskScheduler::IsImportTask(inHostTaskID))
	{ 
		RequestThumbnailElement element;
		element.mMessageID = inMsgID;
		element.mImportFile = inFilePath;
		element.mClipTime = inClipTime;
		element.mAssetItemID = inAssetItemID;
		element.mHostTaskID = inHostTaskID;
		element.mBatchID = inBatchID;
		ASL::StationUtils::PostMessageToUIThread(
			kStation_IngestMedia, 
			RequestThumbnailMessage(element), 
			true);
	}
	else
	{
		DVA_TRACE("AssetLibraryDispatcher::RequestThumbnail", 5, inFilePath);			
		SRLibrarySupport::RequestThumbnailAsync(
			inFilePath,
			inClipTime,
			boost::bind<void>(ThumbnailCompleteCallback, inTaskID, inMsgID, inFilePath, inClipTime, inAssetItemID, _1, _2));
	}
}

/*
**
*/
void RequestThumbnailAsync(
	ASL::String const& inImportFile,
	dvamediatypes::TickTime const& inClipTime,
	RequestThumbnailCallbackFn inRequestThumbnailCallbackFn
	)
{
	if (!SRAsyncCallAssembler::NeedRequestThumbnailInMainThread(inImportFile))
	{
		RequestThumbnailAsyncImpl(inImportFile, inClipTime, inRequestThumbnailCallbackFn);
	}
	else
	{
		SRAsyncCallAssembler::RequestThumbnailInMainThread(inImportFile, inClipTime, RequestThumbnailAsyncImpl, inRequestThumbnailCallbackFn);
	}
}

/*
**
*/
void FinishImportFile(
				ASL::Guid const& inTaskID,
				ASL::Guid const& inMsgID,
				ASL::String const& inMediaPath,
				ASL::Guid const& inBatchID,
				ASL::Guid const& inHostTaskID,
				ASL::Result const& inResult,
				ASL::String const& inErrorInfo)
{
	if (TaskScheduler::IsImportTask(inHostTaskID))
	{ 
		//	Fix bug #3288131
		//	If there is SRMedia related to newly imported file, we should make sure 
		//	it isn't offline.
		if (ASL::ResultSucceeded(inResult))
		{
			if (SRProject::GetInstance()->GetSRMedia(inMediaPath))
			{
				SRProject::GetInstance()->GetSRMedia(inMediaPath)->RefreshOfflineMedia();
			}
		}

		ASL::StationUtils::PostMessageToUIThread(
			kStation_IngestMedia, 
			FinishImportFileMessage(inMediaPath, inBatchID, inHostTaskID, inResult, inErrorInfo), 
			true);
	}	
	else
	{
		// If user cancel ingest and some import finished messages still in stack, then will enter here. It should be harmless.
		// In Library mode, when ingest a RC that contains some clips, it will arrive here.
		if (!UIF::IsEAMode() && PL::Utilities::NeedEnableMediaCollection())
		{
			DVA_ASSERT_MSG(0, "Should not come here in most cases.");
		}
	}
}

/*
**
*/
void NotifyCreateRoughCutCommand()
{
	SRProject::GetInstance()->GetAssetLibraryNotifier()->OnCreateRoughCutNotify(
		ASL::Guid::CreateUnique(), ASL::Guid::CreateUnique());
}

/*
**
*/
void NotifyAppendToRoughCutCommand()
{
	SRProject::GetInstance()->GetAssetLibraryNotifier()->OnAppendToRoughCutNotify(
		ASL::Guid::CreateUnique(), ASL::Guid::CreateUnique());
}

/*
**
*/
void NotifyInsertClipsToRoughCutCommand(bool inInsertAhead)
{
	SRProject::GetInstance()->GetAssetLibraryNotifier()->OnInsertClipsToRoughCutNotify(
		ASL::Guid::CreateUnique(), ASL::Guid::CreateUnique(), inInsertAhead);
}

/*
**
*/
bool RelinkFileInCore(AssetItemList const& inRelinkedAssetItemList)
{
	if (inRelinkedAssetItemList.empty())
	{
		return true;
	}

	if (inRelinkedAssetItemList.front()->GetAssetMediaType() == kAssetLibraryType_RCSubClip)
	{
		if (ModulePicker::GetInstance()->GetCurrentEditingModule() == SRRoughCutModule)
		{
			ISRRoughCutRef roughcut =  ISRRoughCutRef(ModulePicker::GetInstance()->GetCurrentModuleData());

			SRClipItems clipItems;
			AssetItemList assetItemList = inRelinkedAssetItemList;
			Utilities::BuildClipItemFromLibraryInfo(assetItemList, clipItems);
			roughcut->RelinkClipItem(clipItems);
		}
	}
	else
	{
		BOOST_FOREACH(AssetItemPtr const& relinkedAsseteItem, inRelinkedAssetItemList)
		{
			switch(relinkedAsseteItem->GetAssetMediaType())
			{
			case kAssetLibraryType_MasterClip:
				//	If this masterclip haven't been opened, we still need to relink its srMedia which created by RC.
				if (SRProject::GetInstance()->GetSRMedia(relinkedAsseteItem->GetMediaPath()))
				{
					SRProject::GetInstance()->GetSRMedia(relinkedAsseteItem->GetMediaPath())->RefreshOfflineMedia();
				}
				//	No break here.
				// break;
			case kAssetLibraryType_RoughCut:

				//	Mark media online 
				SRProject::GetInstance()->GetAssetMediaInfoWrapper(relinkedAsseteItem->GetMediaPath())->SetAssetMediaInfoOffline(false);

				//	Update selection in SRProject
				if(SRProject::GetInstance()->GetAssetSelectionManager()->IsAssetItemCurrentSelected(relinkedAsseteItem))
				{
					SRProject::GetInstance()->GetAssetSelectionManager()->UpdateSelectedAssetItem(relinkedAsseteItem);
				}

				//	Update PrimaryClip through ModulePicker
				//	[TRICKY] We store old media path in GetAssetMetadata(). It's very very very tricky and should be taken care of in CS7.
				//	The fundamental problem behind this tricky thing is We use path as ID for searching things and ID is not trustable at some cases.
				//	Please see comments in TaskEnine
				if (ModulePicker::GetInstance()->IsAssetItemCurrentOpened(relinkedAsseteItem->GetAssetMetadata(), relinkedAsseteItem))
				{
					ModulePicker::GetInstance()->UpdateOpenedAssetItem(relinkedAsseteItem->GetAssetMetadata(), relinkedAsseteItem);
				}
				break;
			default:
				DVA_ASSERT(false && "Not supported media type!");
			}
		}
	}

	//	update UI 
	SRProject::GetInstance()->GetAssetSelectionManager()->TriggerLibraryMetaDataSelection();
	SRProject::GetInstance()->RemoveUnreferenceResources();

	return true;
}

/*
**
*/
void AsyncLoadingFileInWorkingThread(
	const ASL::Guid& inTaskGuid, 
	const ASL::Guid& inMsgGuid, 
	const ASL::String& inPath, 
	const ASL::Guid& inAssetID,
	bool inIsMediaPropertyOnly)
{
	// [TODO] If import in main thread is OK, should remove this thread work queue and call LoadingFileOperation::Process directly.
	ASL::ThreadedWorkQueueRef threadedWorkQueue = ASL::SharedThreadedWorkQueue::Instance()->RetrieveOrCreate(ASL::ThreadPriorities::kBelowNormal);
	if (threadedWorkQueue == NULL)
	{
		DVA_ASSERT_MSG(0, "Retrieve ASL threaded work queue fail.");
		return;
	}

	LoadingFileOperationRef loadingOperation = LoadingFileOperation::CreateClassRef();
	loadingOperation->Initialize(inTaskGuid, inMsgGuid, inPath, inAssetID, inIsMediaPropertyOnly);
	threadedWorkQueue->QueueRequest(ASL::IThreadedQueueRequestRef(loadingOperation));
}

/*
**
*/
ASL::String GetFileCreateTime(ASL::String const& inFilePath)
{
	if (!ASL::PathUtils::IsValidPath(inFilePath) || !ASL::PathUtils::ExistsOnDisk(inFilePath))
	{
		return ASL::String();
	}

	ASL::String outCreateTime;

	try
	{
		dvacore::filesupport::File tmpFile(inFilePath);
		std::tm time = to_tm(tmpFile.Created());
		outCreateTime = IngestUtils::CreateDateString(time);
	}
	catch (...)
	{

	}
	
	return outCreateTime;
}

/*
**
*/
ASL::String GetFileModifyTime(ASL::String const& inFilePath)
{
	if (!ASL::PathUtils::IsValidPath(inFilePath) || !ASL::PathUtils::ExistsOnDisk(inFilePath))
	{
		return ASL::String();
	}
	ASL::String outLastModifyTime;

	try
	{
		if (IsRoughCutFileExtension(inFilePath))
		{
			dvacore::filesupport::File tmpFile(inFilePath);
			std::tm time = to_tm(tmpFile.Modified());
			outLastModifyTime = IngestUtils::CreateDateString(time);
		}
		else
		{
			XMP_FileFormat format = ML::MetadataManager::GetXMPFormat(inFilePath);

			XMP_DateTime xmpLastModifyTime;
			SXMPFiles::GetFileModDate(ASL::MakeStdString(inFilePath).c_str(), &xmpLastModifyTime, &format, kXMPFiles_ForceGivenHandler);

			std::tm time =
			{
				xmpLastModifyTime.second,
				xmpLastModifyTime.minute,
				xmpLastModifyTime.hour,
				xmpLastModifyTime.day,
				xmpLastModifyTime.month - 1,
				xmpLastModifyTime.year - 1900
			};
			outLastModifyTime = IngestUtils::CreateDateString(time);
		}
	}
	catch (...)
	{

	}

	return outLastModifyTime;
}

ASL::Result UpdateXMPWithMetadata(
	XMPText ioXMP,
	NamespaceMetadataList const& inMetadataList,
	bool inStoreEmpty)
{
	SXMPMeta xmpMetaData = SXMPMeta(ioXMP->c_str(), ioXMP->size());

	// namespace
	NamespaceMetadataList::const_iterator namespaceIt = inMetadataList.begin();
	NamespaceMetadataList::const_iterator namespaceEnd = inMetadataList.end();
	try
	{
		for (; namespaceIt != namespaceEnd; ++namespaceIt)
		{
			NamespaceMetadataPtr namespaceMetadata = *namespaceIt;

			if (namespaceMetadata->mNamespaceUri.empty() || namespaceMetadata->mNamespacePrefix.empty())
				continue;
			ASL::StdString namespaceURI = ASL::MakeStdString(namespaceMetadata->mNamespaceUri);
			ASL::StdString namespacePrefix = ASL::MakeStdString(namespaceMetadata->mNamespacePrefix);

			ASL::StdString registerPrefix;
			SXMPMeta::RegisterNamespace(namespaceURI.c_str(), namespacePrefix.c_str(), &registerPrefix);

			NamespaceMetadataValueMap::const_iterator mapIt = namespaceMetadata->mNamespaceMetadataValueMap.begin();
			NamespaceMetadataValueMap::const_iterator mapEnd = namespaceMetadata->mNamespaceMetadataValueMap.end();
			for (; mapIt != mapEnd; ++mapIt)
			{
				ASL::StdString propName = ASL::MakeStdString(mapIt->first);
				ASL::StdString propValue = ASL::MakeStdString(mapIt->second);
				if (inStoreEmpty || !propValue.empty())
					xmpMetaData.SetProperty(namespaceURI.c_str(), propName.c_str(), propValue.c_str());
			}
		}

		xmpMetaData.SerializeToBuffer(ioXMP.get(), kXMP_OmitPacketWrapper);
	}
	catch(...)
	{
		return ASL::ResultFlags::kResultTypeFailure;
	}

	return ASL::kSuccess;
}

ASL::Result UpdateXMPWithMinimumMetadata(
	XMPText ioXMP,
	const MinimumMetadata& inMiniMetadata)
{
	// namespace
	ASL::Result result = UpdateXMPWithMetadata(ioXMP,
		inMiniMetadata.mNamespaceMetadataList, 
		inMiniMetadata.mStoreEmptyInfo);
	if (ASL::ResultFailed(result))
		return result;

	// keywords
	SXMPMeta xmpMetaData = SXMPMeta(ioXMP->c_str(), ioXMP->size());

	KeywordSet::const_iterator keywordIt = inMiniMetadata.mKeywordSet.begin();
	KeywordSet::const_iterator keywordEnd = inMiniMetadata.mKeywordSet.end();
	for (; keywordIt != keywordEnd; ++keywordIt)
	{
		ASL::StdString keyword = ASL::MakeStdString(*keywordIt);
		try 
		{
			xmpMetaData.AppendArrayItem(kXMP_NS_DC, "subject", kXMP_PropArrayIsUnordered, keyword.c_str());
		}
		catch(XMP_Error& error)
		{
			dvacore::UTF16String errorString =
				dvacore::utility::UTF8to16(
				dvacore::utility::CharAsUTF8(
				error.GetErrMsg()));

			DVA_ASSERT_MSG(0, errorString);
			break;
		}
		catch(...)
		{
			DVA_ASSERT_MSG(0, "Add keyword failed with unknown error!");
			break;
		}
	}

	xmpMetaData.SerializeToBuffer(ioXMP.get(), kXMP_OmitPacketWrapper);
	return result;
}

ASL::Result UpdateXMPWithMinimumMetadata(AssetMediaInfoList const& inAssetMediaInfoList, 
							IngestCustomData const& inCustomData, 
							bool inUseLocalFile, 
							AssetMediaInfoList& outAssetMediaInfoList)
{
	ASL::Result result = ASL::ResultFlags::kResultTypeFailure;
	outAssetMediaInfoList.clear();

	AssetMediaInfoList::const_iterator inMediaInfoIt = inAssetMediaInfoList.begin();
	AssetMediaInfoList::const_iterator inMediaInfoEnd = inAssetMediaInfoList.end();
	for (; inMediaInfoIt != inMediaInfoEnd; ++inMediaInfoIt)
	{
		AssetMediaInfoPtr assetMediaInfo = *inMediaInfoIt;
		if (assetMediaInfo->GetAssetMediaType() != kAssetLibraryType_MasterClip)
		{
			outAssetMediaInfoList.push_back(assetMediaInfo);
			continue;
		}

		XMPText mergedXMP(new ASL::StdString(*assetMediaInfo->GetXMPString().get()));
		result = UpdateXMPWithMinimumMetadata(mergedXMP, inCustomData.mMinimumMetadata);
		if (ASL::ResultFailed(result))
			return result;

		AssetMediaInfoPtr newAssetMediaInfo = AssetMediaInfo::Create(
			assetMediaInfo->GetAssetMediaType(),
			assetMediaInfo->GetAssetMediaInfoGUID(),
			assetMediaInfo->GetMediaPath(),
			mergedXMP,
			assetMediaInfo->GetFileContent(),
			assetMediaInfo->GetCustomMetadata(),
			assetMediaInfo->GetCreateTime(),
			assetMediaInfo->GetLastModifyTime(),
			assetMediaInfo->GetRateTimeBase(),
			assetMediaInfo->GetRateNtsc(),
			assetMediaInfo->GetAliasName());
		outAssetMediaInfoList.push_back(newAssetMediaInfo);

		result = ASL::kSuccess;
	}
	return result;
}

const char* kXMPAliasNameProperty = "title";
const char* kXMPAliasNameNamespcae = kXMP_NS_DC;

ASL::Result UpdateXMPWithRenameData(
	XMPText ioXMP, 
	ASL::String const& inNewName)
{
	SXMPMeta xmpMetaData = SXMPMeta(ioXMP->c_str(), ioXMP->size());
	ASL::StdString name = ASL::MakeStdString(inNewName);
	try
	{
		xmpMetaData.SetLocalizedText(kXMPAliasNameNamespcae, kXMPAliasNameProperty, NULL, "x-default", name);

		xmpMetaData.SerializeToBuffer(ioXMP.get(), kXMP_OmitPacketWrapper);
	}
	catch(...)
	{
		return ASL::ResultFlags::kResultTypeFailure;
	}

	return ASL::kSuccess;
}

const char* const kXMPClipIDPropertyName = "identifier";

ASL::Result WriteClipIDToXMPIfNeeded(XMPText ioXMP, const ASL::Guid& inGuid)
{
	SXMPMeta xmpMetaData = SXMPMeta(ioXMP->c_str(), ioXMP->size());

	std::string clipIDString;
	XMP_StringLen clipIDStringLen;
	bool hasClipID = xmpMetaData.GetProperty(kXMP_NS_DC, kXMPClipIDPropertyName, &clipIDString, &clipIDStringLen);
	if (!hasClipID)
	{
		dvacore::UTF8String clipIDUTF8 = inGuid.AsUTF8String().c_str();
		dvacore::StdString clipIdentifier(clipIDUTF8.begin(), clipIDUTF8.end());
		try
		{
			xmpMetaData.SetProperty(kXMP_NS_DC, kXMPClipIDPropertyName, clipIdentifier.c_str());
			xmpMetaData.SerializeToBuffer(ioXMP.get(), kXMP_OmitPacketWrapper);
		}
		catch(...)
		{
			return ASL::ResultFlags::kResultTypeFailure;
		}
	}

	return ASL::kSuccess;
}

ASL::Result GetNameFromXMP(
					XMPText const& inXMP, 
					ASL::String& outName)
{
	SXMPMeta xmpMetaData = SXMPMeta(inXMP->c_str(), inXMP->size());
	dvacore::StdString stdString;
	try
	{
		dvacore::StdString actualLang;
		xmpMetaData.GetLocalizedText(kXMPAliasNameNamespcae, kXMPAliasNameProperty, NULL, "x-default", &actualLang, &stdString, 0);
	}
	catch(...)
	{
		return ASL::ResultFlags::kResultTypeFailure;
	}

	outName = ASL::MakeString(stdString);

	return ASL::kSuccess;
}

void XMPFileTimeChanged(ASL::Guid const& inAssetID, ASL::String const& inMediaPath, ASL::Result inResult)
{
	ActionResultList actionResultList;
	ActionResultPtr actionResult(new ActionResult(inAssetID, 
														inMediaPath,
														inResult,
														ASL::String(),
														GetFileCreateTime(inMediaPath),
														GetFileModifyTime(inMediaPath)));
	if (ASL::ResultFailed(inResult))
	{
		actionResult->mErrorMsg = dvacore::ZString(
			"$$$/Prelude/SRLibrarySupport/WriteXMPToDiskFinished=Write XMP to disk failed.");
	}
	actionResultList.push_back(actionResult);
	
	SRProject::SharedPtr srProject = SRProject::GetInstance();
	if (srProject)
	{
		IAssetLibraryNotifier* notifier = srProject->GetAssetLibraryNotifier();
		if (notifier) 
		{
			// This is notification that needs create two default ID from host side.
			notifier->XMPFileTimeChanged(
										 ASL::Guid::CreateUnique(),
										 ASL::Guid::CreateUnique(),
										 inResult,
										 actionResultList);
		}
	}
}

/*
**
*/
void SendUnassociatedMetadata(UnassociatedMetadataList const& inUnassociatedMetadataList)
{
	Utilities::RegisterUnassociatedMetadata(inUnassociatedMetadataList);
}

/*
**
*/
void ApplyUnassociatedMetadata(UnassociatedMetadataList const& inUnassociatedMetadataList,
							   ApplyUnassociatedMetadataPositionType inApplyPos)
{
	if (inApplyPos == kApplyUnassociatedMetadataPosition_Player)
	{
		// User wants to apply markers based on the current player position
		ApplyMarkersAtCurrentPlayerPosition(inUnassociatedMetadataList);
	}
	else if (inApplyPos == kApplyUnassociatedMetadataPosition_Marker)
	{
		// User wants to apply markers based on the marker's start time
		ApplyMarkersAtStartTime(inUnassociatedMetadataList);
	}
	else
	{
		DVA_ASSERT(0);
	}
}

/*
**
*/
void ApplyUnassociatedMetadataToMedias(
	const PL::UnassociatedMetadataList& inUnassociatedMetadataList,
	const ASL::PathnameList& inPaths)
{
	ASL::String openedFilePath;
	PL::ISRLoggingClipRef loggingClip(PL::ModulePicker::GetInstance()->GetLoggingClip());
	if (loggingClip)
	{
		openedFilePath = loggingClip->GetSRClipItem()->GetOriginalClipPath();
	}

	BOOST_FOREACH(const ASL::String& path, inPaths)
	{
		if (!openedFilePath.empty() && ASL::CaseInsensitive::StringEquals(openedFilePath, path))
		{
			// The clip is opened, we follow the workflow of applying to timeline
			ApplyUnassociatedMetadata(inUnassociatedMetadataList, PL::kApplyUnassociatedMetadataPosition_Marker);
		}
		else
		{
			ApplyUnassociatedMetadataToOneMedia(inUnassociatedMetadataList, path);
		}
	}
}

/*
**
*/
ASL::Result IsSupportedMedia(const BE::IMasterClipRef& inMasterClip)
{
	ASL::Result result = MZ::eAppUnSupportFormat;
	BE::IMediaRef media(BE::MasterClipUtils::GetDeprecatedMedia(inMasterClip));
	BE::IMediaInfoRef mediaInfo;
	if (media)
	{
		mediaInfo = media->GetMediaInfo();
		bool isSupported = false;

		size_t numVideoStreams = mediaInfo->GetNumStreams(BE::kMediaType_Video);
		for (size_t index = 0; index < numVideoStreams; index++)
		{
			MF::ISourceRef source = mediaInfo->GetStream(BE::kMediaType_Video, index);
			if (source)
			{
				isSupported = true;
				break;
			}
		}
		if (isSupported)
		{
			result = ASL::kSuccess;
		}

		size_t numAudioStreams = mediaInfo->GetNumStreams(BE::kMediaType_Audio);
		for (size_t index = 0; index < numAudioStreams; index++)
		{
			if (mediaInfo->GetStream(BE::kMediaType_Audio, index))
			{
				isSupported = true;
				break;
			}
		}
		if (isSupported)
		{
			result = ASL::kSuccess;
		}
	}

	return result;
}

void RequestAsyncCopy(
	const ASL::Guid& inBatchID,
	const TransferItemList& inItemList,
	TransferOptionType inOption,
	FTPSettingsInfo const& inFTPSettingsInfo,
	TransferType inTransferType)
{
	AsyncCopyRequestPtr newRequest;
	if (inTransferType == kTransferType_FTP)
	{
		newRequest = AsyncCopyRequestPtr(new AsyncFTPTransferRequest(inBatchID, inItemList, inOption, inFTPSettingsInfo));
	}
	else
	{
		newRequest = AsyncCopyRequestPtr(new AsyncLocalTransferRequest(inBatchID, inItemList, inOption));
	}
	sTransferRequests[inBatchID] = newRequest;
	GetTransferExecutor()->CallAsynchronously(boost::bind(&AsyncCopyRequest::Process, newRequest.get()));
}

void CancelAsyncCopy(const ASL::Guid& inBatchID)
{
	if (sTransferRequests.find(inBatchID) != sTransferRequests.end())
	{
		sTransferRequests[inBatchID]->Cancel();
	}
}

static void BatchRename(
	const ASL::Guid& inBatchID,
	const RenameItemList& inItemList)
{
	bool allSuccess = true;
	TransferResultList resultList;
	BOOST_FOREACH (const RenameItem& item, inItemList)
	{
		try
		{
			if (ASL::PathUtils::IsDirectory(item.first))
			{
				dvacore::filesupport::Dir(item.first).Rename(item.second);
			}
			else
			{
				dvacore::filesupport::File(item.first).Rename(item.second);
			}
			resultList.push_back(TransferResultPtr(new TransferResult(
				item.first,
				item.second,
				ASL::kSuccess)));
		}
		catch (...)
		{
			allSuccess = false;
			ASL::String errorMsg = dvacore::ZString("$$$/Prelude/SRLibrarySupport/RenameFail=Rename @0 with @1 failed.", item.first, item.second);
			resultList.push_back(TransferResultPtr(new TransferResult(
				item.first,
				item.second,
				ASL::ResultFlags::kResultTypeFailure,
				errorMsg)));
			ML::SDKErrors::SetSDKInfoString(errorMsg);
		}
	}

	ASL::AsyncCallFromMainThread(boost::bind(
		&IAssetLibraryNotifier::RenameStatus,
		SRProject::GetInstance()->GetAssetLibraryNotifier(),
		inBatchID,
		ASL::Guid::CreateUnique(),
		allSuccess ? ASL::kSuccess : ASL::ResultFlags::kResultTypeFailure,
		resultList));
}

void RequestAsyncRename(
	const ASL::Guid& inBatchID,
	const RenameItemList& inItemList)
{
	GetTransferExecutor()->CallAsynchronously(boost::bind(&BatchRename, inBatchID, inItemList));
}

/*
**
*/
ASL::Result CreateRoughCut(
	const ASL::String& inFilePath, 
	bool inShouldSaveToFile, 
	ASL::String& outContentText, 
	PL::AssetMediaInfoList& outAssetMediaInfoList, 
	ASL::String& outErrorMessage)
{
	ASL::Result result = ASL::kSuccess;
	ASL::String timebase;
	ASL::String ntsc;

	// Check if need to save to file, if yes, save it
	ASL::String destinationFile = inFilePath;
	if (inShouldSaveToFile)
	{
#if ASL_TARGET_OS_MAC
		destinationFile = MZ::Utilities::PrependBootVolumePath(destinationFile);
#endif			
		if (!destinationFile.empty())
		{
			// check if it's the rough cut currently opened.
			PL::ISRRoughCutRef roughCut(PL::ModulePicker::GetInstance()->GetRoughCutClip());
			if (roughCut != NULL 
				&& dvacore::utility::CaseInsensitive::StringEquals(
				ASL::PathUtils::ToNormalizedPath(roughCut->GetFilePath()),
				ASL::PathUtils::ToNormalizedPath(destinationFile)))
			{
				outErrorMessage = dvacore::ZString("$$$/Prelude/PLCore/OverrideOpeningRCOnCreate=@0 is open now, please close it first."),
					outErrorMessage = dvacore::utility::ReplaceInString(outErrorMessage, destinationFile);
				UIF::MessageBox(
					outErrorMessage,
					dvacore::ZString("$$$/Prelude/PLCore/CreateRCFailed=Warning"),
					UIF::MBFlag::kMBFlagOK,
					UIF::MBIcon::kMBIconWarning);
				result = ASL::eFileInUse;
			}
			else
			{
				outContentText = PL::CreateEmptyRoughCutContent(ASL::PathUtils::GetFilePart(destinationFile));
				PL::FCPRateToTimebaseAndNtsc(timebase, ntsc);
				// Write initial data for rough cut
				//result = PL::CreateEmptyRoughCutFile(destinationFile);
				result = PL::SaveRoughCut(destinationFile, outContentText);
			}
		}
		else
		{
			result = ASL::eUserCanceled;
			outErrorMessage = dvacore::ZString(
				"$$$/Prelude/PLCore/CreateRoughCutCanceled=User canceled rough cut creation.");
		}
	}
	else
	{
		// if don't save to file, inFilePath should mean the new rough cut name.
		ASL::String rcName = destinationFile;
#if ASL_TARGET_OS_MAC
		rcName = MZ::Utilities::PrependBootVolumePath(rcName);
#endif			
		if (rcName.empty())
		{
			result = ASL::eUnknown;
			outErrorMessage = dvacore::ZString(
				"$$$/Prelude/PLCore/ErrCreateRCWithEmptyName=Rough cut cannot have an empty name.");
		}
		else
		{
			outContentText = PL::CreateEmptyRoughCutContent(rcName);
			PL::FCPRateToTimebaseAndNtsc(timebase, ntsc);
		}
	}

	if (ASL::ResultSucceeded(result))
	{
		PL::AssetMediaInfoPtr assetMediaInfo = PL::AssetMediaInfo::CreateRoughCutMediaInfo(
			ASL::Guid::CreateUnique(),
			destinationFile,
			outContentText,
			PL::SRLibrarySupport::GetFileCreateTime(destinationFile),
			PL::SRLibrarySupport::GetFileModifyTime(destinationFile),
			timebase,
			ntsc);
		outAssetMediaInfoList.push_back(assetMediaInfo);
	}

	return result;
}

/*
**
*/
void RegisterAssetMediaInfoToSRCore(
	const PL::AssetMediaInfoList& inAssetMediaInfoList, 
	const PL::AssetItemList& inAssetItemList, 
	bool inMediaUseLocalFile, 
	bool inRoughCutUseLocalFile, 
	bool inNeedForceRegister, 
	bool inIsRelink)
{
	PL::AssetMediaInfoList mediaInfoListInCore;
	BOOST_FOREACH(PL::AssetMediaInfoPtr assetMediaInfo, inAssetMediaInfoList)
	{
		switch(assetMediaInfo->GetAssetMediaType())
		{
		case PL::kAssetLibraryType_MasterClip:
			{
				assetMediaInfo->SetNeedSavePreCheck(inMediaUseLocalFile);
				if ( inMediaUseLocalFile && assetMediaInfo->GetForceLoadFromLocalDisk() )
				{
					ASL::String xmpStr;
					ASL::String errorInfo;
					PL::XMPText xmpText(new ASL::StdString(ASL::MakeStdString(xmpStr)));
					PL::SRLibrarySupport::ReadXMPFromCache(assetMediaInfo->GetMediaPath(), xmpText, errorInfo);
					PL::AssetMediaInfoPtr newAssetMediaInfo = PL::AssetMediaInfo::Create(
						assetMediaInfo->GetAssetMediaType(),
						assetMediaInfo->GetAssetMediaInfoGUID(),
						assetMediaInfo->GetMediaPath(),
						xmpText,
						assetMediaInfo->GetFileContent(),
						assetMediaInfo->GetCustomMetadata(),
						assetMediaInfo->GetCreateTime(),
						assetMediaInfo->GetLastModifyTime(),
						assetMediaInfo->GetRateTimeBase(),
						assetMediaInfo->GetRateNtsc(),
						assetMediaInfo->GetAliasName());
					mediaInfoListInCore.push_back(newAssetMediaInfo);
				}
				else
				{
					mediaInfoListInCore.push_back(assetMediaInfo);
				}
				break;
			}
		case PL::kAssetLibraryType_RoughCut:
			assetMediaInfo->SetNeedSavePreCheck(inRoughCutUseLocalFile);
			mediaInfoListInCore.push_back(assetMediaInfo);
			break;
		default:
			DVA_ASSERT(0);
		}
	}

	PL::SRProject::GetInstance()->RegisterAssetMediaInfoList(
		inAssetItemList, mediaInfoListInCore, inNeedForceRegister, inIsRelink);
}


} // namespace SRLibrarySupport
} // namespace PL
