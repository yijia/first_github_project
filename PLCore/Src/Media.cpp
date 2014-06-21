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

// Local
#include "PLMessage.h"
#include "PLMedia.h"
#include "PLFactory.h"
#include "PLProject.h"
#include "IPLMarkers.h"
#include "PLMarkers.h"
#include "PLUtilities.h"
#include "PLModulePicker.h"
#include "PLAssetItem.h"
#include "PLBEUtilities.h"

//	MZ
#include "PLConstants.h"
#include "boost/bind.hpp"
#include "PLMessage.h"
#include "MZMedia.h"
#include "MZProject.h"
#include "MZAction.h"
#include "MZMediaParams.h"
#include "MZMasterClip.h"
#include "MZBEUtilities.h"
#include "MZProjectActions.h"
#include "MZUtilities.h"

//	DVA
#include "dvaui/ui/OS_UI.h"
#include "dvacore/config/Localizer.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLStationUtils.h"
#include "ASLStringCompare.h"
#include "ASLThreadManager.h"
#include "ASLAsyncCallFromMainThread.h"
#include "ASLStationUtils.h"

//	BE
#include "BE/Core/INotification.h"
#include "BE/Clip/MasterClipMessages.h"
#include "Source/ISource.h"
#include "BE/Media/MediaMessages.h"

// ML
#include "MLConformUtils.h"

// UIF
#include "UIFDocumentManager.h"

namespace PL
{

ASL_MESSAGE_MAP_DEFINE(SRMedia)
	ASL_MESSAGE_HANDLER(PL::MediaMetaDataMarkerNotInSync, OnMediaMetaDataMarkerNotInSync)
	ASL_MESSAGE_HANDLER(BE::MediaInfoChangedMessage, OnMediaInfoChanged)
	ASL_MESSAGE_HANDLER(BE::MasterClipClipChangedMessage, OnMasterClipClipChanged)
	ASL_MESSAGE_HANDLER(BE::MediaChangedMessage, OnMediaChanged)
ASL_MESSAGE_MAP_END

BE::IOrphanItemRef SRMedia::sOrphanItems;
    
/*
**
*/
SRMedia::SRMedia()
	:
	mStationID(ASL::StationRegistry::RegisterUniqueStation()),
	mIsDirty(false),
	mIsWritable(false)
{
}

/*
**
*/
SRMedia::~SRMedia()
{
	BE::IMasterClipRef masterClip = GetMasterClip();
	if (masterClip)
	{
		BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
		BE::INotificationRef notification = BE::INotificationRef(media);
		if (notification)
		{
			ASL::StationUtils::RemoveListener(notification->GetStationID(), this);
		}
	}

	if (masterClip && !MZ::BEUtilities::IsEAMasterClip(masterClip))
	{
		BEUtilities::RemoveMasterClipFromCurrentProject(masterClip);
	}

	if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(mClipFilePath))
	{
		ASL::StationUtils::RemoveListener(
			SRProject::GetInstance()->GetAssetMediaInfoWrapper(mClipFilePath)->GetStationID(),
			this);
	}
    
	if (mProjectItem && MZ::BEUtilities::IsEAProjectItem(mProjectItem))
	{
		SRMedia::sOrphanItems->RemoveProjectItem(mProjectItem);
	}

	ASL::StationRegistry::DisposeStation(mStationID);
}

/*
**
*/
void SRMedia::TryCacheProjectItem() const
{
	if (!mClipFilePath.empty() && mProjectItem == NULL)
	{
		BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), mClipFilePath);

		if (UIF::IsEAMode())
		{
			mProjectItem = projectItem->Clone();
			BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(mProjectItem);
			dvamediatypes::TickTime startTime;
			dvamediatypes::TickTime endTime;
			bool isHardBoundary = false;

			if (masterClip->GetHardOrSoftContentBoundaries(startTime, endTime, isHardBoundary) && !isHardBoundary)
			{
				MZ::ExecuteActionWithoutUndo(
					(BE::IExecutorRef)masterClip,
					masterClip->CreateSetContentBoundariesAction(
					dvamediatypes::kTime_Invalid, 
					dvamediatypes::kTime_Invalid,
					false), // isHardBoundaries
					true);// inDoNotification
			}

			ASL::String eaAssetID = MZ::BEUtilities::GetEAAssetIDInProjectItem(projectItem);
			MZ::BEUtilities::SetEAAssetIDInProjectItem(mProjectItem, eaAssetID);
			SRMedia::sOrphanItems->AddProjectItem(mProjectItem);
			BE::IMasterClipRef originalMasterClip = BE::ProjectItemUtils::GetMasterClip(projectItem);
			if (BE::MasterClipUtils::GetMediaInfo(originalMasterClip) == NULL)
			{
				BE::INotificationRef notification(originalMasterClip);
				ASL::StationUtils::AddListener(notification->GetStationID(), const_cast<PL::SRMedia*>(this));
			}
		}
		else
		{
			mProjectItem = projectItem;
		}
	}
}

/*
**
*/
void SRMedia::OnMasterClipClipChanged(BE::IMasterClipRef inMasterClip)
{
	if (BE::MasterClipUtils::GetMediaInfo(inMasterClip))
	{
		BE::IMasterClipRef cachedMasterClip = BE::ProjectItemUtils::GetMasterClip(mProjectItem);
		BE::IMediaRef media = BE::MasterClipUtils::GetMedia(cachedMasterClip);

		BE::IActionRef action;
		ASL::Result result = MZ::Media::ReattachMedia(
			MZ::GetProject(),
			cachedMasterClip, 
			media, 
			ASL::String(), 
			MZ::MediaParams::CreateDefault(), 
			false, 
			action,
			false,
			BE::MasterClipUtils::GetMediaInfo(inMasterClip));

		if ( ASL::ResultSucceeded(result) )
		{
			MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inMasterClip), action, true);
		}

		BE::INotificationRef notification(inMasterClip);
		ASL::StationUtils::RemoveListener(notification->GetStationID(), this);
	}
}

/*
**
*/
BE::IMasterClipRef SRMedia::GetMasterClip() const
{
	TryCacheProjectItem();
	// If project has been closed, we cannot get master clip
	return MZ::BEUtilities::GetMasteClipFromProjectitem(mProjectItem);
}

/*
**
*/
ISRMediaRef SRMedia::Create(ASL::String const& inClipFilePath)
{
	ASL_ASSERT(!inClipFilePath.empty());

	SRMediaRef srMedia = CreateClassRef();
	srMedia->Initialize(inClipFilePath);
	ASL_REQUIRE(srMedia);
	return ISRMediaRef(srMedia);
}

/*
**
*/
ASL::StationID SRMedia::GetStationID() const
{
	return mStationID;
}

/*
**
*/
void SRMedia::Initialize(ASL::String const& inClipFilePath)
{
	mClipFilePath = inClipFilePath;
	mIsWritable = PL::Utilities::IsXMPWritable(mClipFilePath);

	TryCacheProjectItem();
	BE::IMasterClipRef masterClip;
	if (!mProjectItem)
	{
		if (ASL::PathUtils::ExistsOnDisk(mClipFilePath))
		{
			masterClip = BEUtilities::ImportFile(
				mClipFilePath, 
				BE::IProjectRef(), 
				true,	//	inNeedErrorMsg
				false,	//	inIgnoreAudio
				false); //	inIgnoreMetadata
		}

		if ( NULL != masterClip )
		{
			if (ASL::ThreadManager::CurrentThreadIsMainThread())
			{
				MZ::ProjectActions::AddMasterClipToBin(
					MZ::GetProject(), 
					MZ::GetProject()->GetRootProjectItem(), 
					masterClip,
					false);
			}
			else
			{
				DVA_ASSERT_MSG(0, "Must import in main thread for core!");
			}
		}
		else  // Import failure or the file does not exists on disk
		{
			masterClip = MZ::BEUtilities::CreateOfflineMasterClip(mClipFilePath, false);
			MZ::Utilities::SyncCallFromMainThread(
				boost::bind(&MZ::ProjectActions::AddMasterClipToBin,
							MZ::GetProject(), 
							MZ::GetProject()->GetRootProjectItem(), 
							masterClip,
							false));
		}
	}
	else
	{
		masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(mProjectItem);
	}

	if (!masterClip)
	{
		DVA_ASSERT_MSG(0, "Should not get here.");
		return;
	}

    SRMarkerOwner::SetMarkers(PL::ISRMarkersRef(PL::SRMarkers::Create(ISRMediaRef(this))));
	BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
	BE::INotificationRef notification = BE::INotificationRef(media);
	if (notification)
	{
		ASL::StationUtils::AddListener(notification->GetStationID(), this);
	}
	if (!media->IsOffline())
	{
		PL::AssetMediaInfoPtr assetMediaInfo 
				= SRProject::GetInstance()->GetAssetMediaInfo(mClipFilePath);
        SRMarkerOwner::GetMarkers()->BuildMarkersFromXMPString(assetMediaInfo->GetXMPString(), false);
	}

	if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(mClipFilePath))
	{
		ASL::StationUtils::AddListener(
			SRProject::GetInstance()->GetAssetMediaInfoWrapper(mClipFilePath)->GetStationID(),
			this);
	}

	BE::IMediaInfoRef mediaInfo = BE::MasterClipUtils::GetDeprecatedMedia(masterClip)->GetMediaInfo();
	if (mediaInfo)
	{
		ASL::StationUtils::AddListener(mediaInfo->GetStationID(), this);
	}

	ML::ConformUtils::InitiateAsyncConform(masterClip);
}

/*
**
*/
void SRMedia::RefreshOfflineMedia()
{
	BE::IMediaRef media;
	BE::IProjectRef project = MZ::GetProject();
	TryCacheProjectItem();

	if (!mProjectItem)
	{
		DVA_ASSERT(0);
		return;
	}

	bool isWritable= PL::Utilities::IsXMPWritable(mClipFilePath);
	UpdateWritableState(isWritable);

	BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(mProjectItem);
	if (masterClip == NULL 
		|| (media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip)) == NULL
		|| !media->IsOffline() )
	{
		return;
	}

	BE::IActionRef action;
	if ( media && ASL::PathUtils::ExistsOnDisk(mClipFilePath))
	{
		ASL::Result result = MZ::Media::ReattachMedia(
			project,
			masterClip, 
			media, 
			mClipFilePath, 
			MZ::MediaParams::CreateDefault(), 
			true, 
			action);

		if ( ASL::ResultSucceeded(result) )
		{
			MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(masterClip), action, true);
		}
	}
}


/*
**
*/
void SRMedia::OnMediaMetaDataMarkerNotInSync(ASL::String const& inMediaLocator)
{
	if (SRProject::GetInstance()->GetAssetMediaInfo(mClipFilePath)->GetMediaPath() == inMediaLocator)
	{
		BE::IMasterClipRef masterClip = GetMasterClip();
		if (!masterClip)
		{
			return;
		}

		BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);

		bool needRefreshMarker(false);
		if (IsDirty())
		{
			ASL::String titleString = 
				dvacore::ZString("$$$/Prelude/Mezzanine/SRMedia/Warning=Warning");
			ASL::String messageString = 
				dvacore::ZString("$$$/Prelude/Mezzanine/SRMedia/DirtySRMediaIsChanged=Your markers might have been changed by another application. Do you want to reload them?");
			messageString = dvacore::utility::ReplaceInString(messageString, inMediaLocator);
			UIF::MBResult::Type ret = UIF::MessageBox(
				messageString,
				titleString,
				UIF::MBFlag::kMBFlagYesNo, 
				UIF::MBIcon::kMBIconWarning,
				UIF::MBResult::kMBResultYes);

			needRefreshMarker = (ret == UIF::MBResult::kMBResultYes);
		}
		else if (!UIF::IsEAMode())
		{
			ASL::String messageString = 
				dvacore::ZString("$$$/Prelude/Mezzanine/SRMedia/UndirtySRMediaIsChanged=Media: @0's XMP has been changed.");

			messageString = dvacore::utility::ReplaceInString(messageString, inMediaLocator);
			ML::SDKErrors::SetSDKWarningString(messageString);
			needRefreshMarker = true;
		}

		if(needRefreshMarker && media && !media->IsOffline())
		{
            SRMarkerOwner::GetMarkers()->BuildMarkersFromXMPString(
				SRProject::GetInstance()->GetAssetMediaInfo(mClipFilePath)->GetXMPString(),
				true);
			SetDirty(false);
		}
	}
}

void SRMedia::OnMediaChanged(
	const BE::IMediaRef& inMedia)
{
	DVA_TRACE("SRMedia", 6, "OnMediaChanged()");
	ASL::StationUtils::PostMessageToUIThread(
		mStationID, 
		SRMediaChangedMessage(ISRMediaRef(this)),
		true);
}

/*
**
*/
void SRMedia::OnMediaInfoChanged()
{
	if (!UIF::IsEAMode())
	{
		BE::IMasterClipRef masterClip = GetMasterClip();
		if (masterClip)
		{
			AssetMediaInfoPtr oldAssetMediaInfo = 
				SRProject::GetInstance()->GetAssetMediaInfo(mClipFilePath);

			PL::AssetMediaInfoList assetMediaInfoList;

			CustomMetadata customMetadata = Utilities::GetCustomMedadata(masterClip);

			//	We should get duration directly from MediaInfo instead of MasterClip cause 
			//	the duration in MasterClip may not be updated in time.
			BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
			BE::IMediaInfoRef mediaInfo = media->GetMediaInfo();

			bool hasVideo = masterClip->ContainsMedia(BE::kMediaType_Video);
			MF::ISourceRef mediaSource;
			if (hasVideo)
			{
				mediaSource = mediaInfo->GetStream(BE::kMediaType_Video, 0);
			}
			else
			{
				mediaSource = mediaInfo->GetStream(BE::kMediaType_Audio, 0);
			}

			if (mediaSource)
			{
				customMetadata.mDuration = mediaSource->GetDuration();
			}

			ASL::String customMetadataStr = Utilities::CreateCustomMetadataStr(customMetadata);

			PL::AssetMediaInfoPtr assetMediaInfo = PL::AssetMediaInfo::CreateMasterClipMediaInfo(
				oldAssetMediaInfo->GetAssetMediaInfoGUID(),
				oldAssetMediaInfo->GetMediaPath(),
				oldAssetMediaInfo->GetAliasName(),
				oldAssetMediaInfo->GetXMPString(),
				customMetadataStr);
			assetMediaInfoList.push_back(assetMediaInfo);

			SRProject::GetInstance()->GetAssetLibraryNotifier()->MediaInfoChanged(
				ASL::Guid::CreateUnique(),
				ASL::Guid::CreateUnique(),
				assetMediaInfoList);
		}
	}
}

/*
**
*/
ASL::String SRMedia::GetClipFilePath() const
{
	return mClipFilePath;
}

/*
**
*/
void SRMedia::OnMetadataSave()
{
	SetDirty(false);
}

/*
**
*/
bool SRMedia::IsDirty() const
{
	return mIsDirty;
}

bool SRMedia::IsWritable() const
{
	return mIsWritable;
}

void SRMedia::UpdateWritableState(bool inIsWritable)
{
	if (mIsWritable != inIsWritable)
	{
		mIsWritable = inIsWritable;
		ASL::StationUtils::PostMessageToUIThread(MZ::kStation_PreludeProject, PL::CurrentModuleDataSavableStateChanged(mIsWritable));
	}
}

/*
**
*/
bool SRMedia::IsOffLine() const
{
	BE::IMasterClipRef masterClip = GetMasterClip();
	if (masterClip)
	{
		BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
		if (media)
		{
			return media->IsOffline();
		}
	}

	return true;
}

/*
**
*/
void SRMedia::SetDirty(bool inIsDirty)
{
	if(mIsDirty != inIsDirty)
	{
		mIsDirty = inIsDirty;
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMediaDirtyChanged(),
			true);

		PL::AssetItemPtr  assetItem;
		if (UIF::IsEAMode())
		{
			ISRPrimaryClipRef primaryClip = SRProject::GetInstance()->GetPrimaryClip(mClipFilePath);
			if (primaryClip)
			{
				assetItem = primaryClip->GetAssetItem();
			}
		}
		else
		{
			PL::AssetMediaInfoPtr assetMediaInfo = 
				SRProject::GetInstance()->GetAssetMediaInfo(mClipFilePath);

			assetItem = PL::AssetItemPtr(new PL::AssetItem(
				assetMediaInfo->GetAssetMediaType(),
				mClipFilePath,
				assetMediaInfo->GetAssetMediaInfoGUID(),
				assetMediaInfo->GetAssetMediaInfoGUID(),
				assetMediaInfo->GetAssetMediaInfoGUID(),
				DVA_STR("Temp AssetItem")));
		}

		if (assetItem)
		{
			SRProject::GetInstance()->GetAssetLibraryNotifier()->AssetDirtyStateChanged(
				ASL::Guid::CreateUnique(),
				ASL::Guid::CreateUnique(),
				assetItem,
				mIsDirty);
		}
	}
	
	if (ModulePicker::IsAutoSaveEnabled())
	{
		if (inIsDirty)
		{
			ISRPrimaryClipRef clipPrimaryClip = (ISRPrimaryClipRef)ModulePicker::GetInstance()->GetLoggingClip();
			if (clipPrimaryClip)
			{
				ASL::AsyncCallFromMainThread(
					boost::bind<UIF::MBResult::Type>(&ISRPrimaryClip::Save, clipPrimaryClip, true, false));
			}
		}
	}
}
	
}
