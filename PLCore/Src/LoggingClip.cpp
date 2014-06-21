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
#include "MZMasterClip.h"
#include "MZBEUtilities.h"
#include "PLConstants.h"
#include "MZSequenceActions.h"
#include "MZProject.h"
#include "MZPlayer.h"
#include "MZUtilities.h"
#include "MZTimecodeOverlay.h"
#include "MZUndo.h"

//	SR
#include "PLLoggingClip.h"
#include "PLClipItem.h"
#include "PLProject.h"
#include "IPLMarkers.h"
#include "PLMarkers.h"
#include "PLUtilities.h"
#include "PLUtilitiesPrivate.h"
#include "PLLibrarySupport.h"
#include "IPLMarkerOwner.h"

//	DVA
#include "dvacore/config/Localizer.h"
#include "dvaui/dialogs/OS_FileSaveAsDialog.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/threads/SharedThreads.h"
#include "dvacore/utility/Utility.h"

//	MediaCore
#include "Player/IPlayer.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLStringCompare.h"

namespace PL
{

namespace
{

// Work around to avoid save function re-enter.
bool sIsSavingLoggingClip = false;

/*
**
*/
void SaveLoggingClipCallBackRoutine(
		PL::AssetItemPtr const& inAssetItem,
		PL::AssetMediaInfoPtr const& inAssetMediaInfo,
		ASL::Result result,
		ASL::String const& inErrorMsg)
{
	if (ASL::ResultFailed(result))
		return;

	PL::AssetMediaInfoList assetMediaInfoList;
	assetMediaInfoList.push_back(inAssetMediaInfo);
	SRProject::GetInstance()->GetAssetLibraryNotifier()->SaveAssetMediaInfo(
		ASL::Guid::CreateUnique(),
		ASL::Guid::CreateUnique(),
		assetMediaInfoList,
		inAssetItem);
}

ASL::String GetMasterClipNameFromAssetItem(PL::AssetItemPtr const& inAssetItem)
{
	ASL::String mediaPath = inAssetItem->GetMediaPath();

	// try media info alias name
	PL::AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mediaPath);
	if (assetMediaInfo != NULL && !assetMediaInfo->GetAliasName().empty())
		return assetMediaInfo->GetAliasName();

	// for master clip, asset item name is acceptable
	if (inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_MasterClip)
	{
		return inAssetItem->GetAssetName();
	}
	else
	{
		// try xmp title field first
		if (assetMediaInfo != NULL)
		{
			ASL::String newName;
			if (ASL::ResultSucceeded(SRLibrarySupport::GetNameFromXMP(assetMediaInfo->GetXMPString(),newName))
				&& !newName.empty())
			{
				return newName;
			}
		}

		// try BE project item name (same with master clip name)
		BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), mediaPath);
		if (projectItem != NULL)
		{
			return projectItem->GetName();
		}

		// Most of the time, should NOT get here. However, return full file part for any exceptions.
		return ASL::PathUtils::GetFullFilePart(mediaPath);
	}
}

}

ASL_MESSAGE_MAP_DEFINE(SRLoggingClip)
	ASL_MESSAGE_HANDLER(SRMediaChangedMessage, OnSRMediaChanged)
	ASL_MESSAGE_HANDLER(PL::SilkRoadMarkerChanged, OnSilkRoadMarkerRenamed)
	ASL_MESSAGE_HANDLER(MZ::ShowTimecodeStatusChanged, OnShowTimecodeStatusChanged)		
	ASL_MESSAGE_HANDLER(PL::MediaMetaDataChanged, OnMediaMetaDataChanged)	
	ASL_MESSAGE_HANDLER(PL::AssetItemNameChanged, OnAssetItemRename)	
	ASL_MESSAGE_HANDLER(MZ::UndoStackClearedMessage, OnUndoStackCleared)
ASL_MESSAGE_MAP_END


/*
**
*/
void SRLoggingClip::OnAssetItemRename(ASL::String const& inMediaFile, ASL::String const& inNewName)
{
	AssetItemPtr assetItem = GetAssetItem();

	if (MZ::Utilities::NormalizePathWithoutUNC(assetItem->GetMediaPath()) == MZ::Utilities::NormalizePathWithoutUNC(inMediaFile))
	{
		if (!inNewName.empty() && inNewName != assetItem->GetAssetName())
		{
			ASL::String oldName = assetItem->GetAssetName();
			assetItem->SetAssetName(inNewName);

			ASL::StationUtils::BroadcastMessage(
							MZ::kStation_PreludeProject,
							PL::PrimaryClipRenamedMessage(assetItem->GetMediaPath(), inNewName, oldName));
		}
	}
}

/*
**
*/
BE::ISequenceRef SRLoggingClip::GetBESequence()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem);
}

/*
**
*/
SRLoggingClip::SRLoggingClip()
	: 
	mEditTime(dvamediatypes::kTime_Zero),
	mActiveSubclipStart(dvamediatypes::kTime_Min),
	mActiveSubclipEnd(dvamediatypes::kTime_Max),
	mUndoableActionCountAfterAutoSave(0),
	mUserHasCanceledSaveFailure(false)
{
	ASL::StationUtils::AddListener(MZ::kStation_Undo, this);
}

/*
**
*/
SRLoggingClip::~SRLoggingClip()
{
	ASL::StationUtils::RemoveListener(MZ::kStation_Undo, this);
}

/*
**
*/
void SRLoggingClip::OnMediaMetaDataChanged(ASL::String const& InMediaInfoID)
{
	if (!IsLoggingClipForEA())
	{
		AssetItemPtr assetItem = GetAssetItem();

		if (MZ::Utilities::NormalizePathWithoutUNC(assetItem->GetMediaPath()) == MZ::Utilities::NormalizePathWithoutUNC(InMediaInfoID))
		{
			ASL::String newName;
			SRLibrarySupport::GetNameFromXMP(SRProject::GetInstance()->GetAssetMediaInfo(assetItem->GetMediaPath())->GetXMPString(),newName);

			OnAssetItemRename(assetItem->GetMediaPath(), newName);
		}
	}
}

/*
**
*/
void SRLoggingClip::SetActiveSubClipBoundary(
	const dvamediatypes::TickTime& inStart,
	const dvamediatypes::TickTime& inEnd,
	const ASL::Guid& inID)
{
	ASL_ASSERT(inStart < inEnd);

	mActiveSubclipStart = inStart;
	mActiveSubclipEnd = inEnd;
	mActiveSubclipID = inID;
}

/*
**
*/
dvamediatypes::TickTime SRLoggingClip::GetStartTime() const
{
	return mActiveSubclipStart;
}

/*
**
*/
dvamediatypes::TickTime SRLoggingClip::GetDuration() const
{
	return mActiveSubclipEnd - mActiveSubclipStart;
}

/*
**
*/
void SRLoggingClip::UpdateBoundary(const dvamediatypes::TickTime& inStartTime, const dvamediatypes::TickTime& inDuration)
{
	mActiveSubclipStart = inStartTime;
	mActiveSubclipEnd = inDuration + inStartTime;
}

/*
**
*/
void SRLoggingClip::SetEditTime(const dvamediatypes::TickTime& inEditTime)
{
	mEditTime = inEditTime;
}

/*
**
*/
dvamediatypes::TickTime SRLoggingClip::GetEditTime() const
{
	return mEditTime;
}

/*
**
*/
bool SRLoggingClip::IsDirty() const
{
	ISRMediaRef srMedia = GetSRClipItem()->GetSRMedia();
	return srMedia != NULL ? srMedia->IsDirty() : false;
}

void SRLoggingClip::SetDirty(bool inIsDirty)
{
	ISRMediaRef srMedia = GetSRClipItem()->GetSRMedia();
	if (srMedia != NULL)
	{
		srMedia->SetDirty(inIsDirty);
	}
}

/*
**
*/
void SRLoggingClip::LoadCachedXMPData()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	ISRMediaRef  srMedia = mSRClipItem->GetSRMedia();
	PL::AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mAssetItem->GetMediaPath());
	ISRMarkerOwnerRef(srMedia)->GetMarkers()->BuildMarkersFromXMPString(assetMediaInfo->GetXMPString(), true);
}

/*
**
*/
bool SRLoggingClip::SendSaveNotification(
	bool& outDiscardChanges,
	bool isCloseOrExit,
	ISRPrimaryClip::SaveCallBackFn const& inSaveCallBackFn)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	ISRMediaRef  srMedia = mSRClipItem->GetSRMedia();
	BE::IMasterClipRef masterClip = srMedia->GetMasterClip();
	XMPText xmpStr = 
		Utilities::GetXMPTextWithAltTimecodeField(ISRMarkerOwnerRef(srMedia)->GetMarkers()->BuildXMPStringFromMarkers(), masterClip);

	ASL::String filePath = mAssetItem->GetMediaPath();
	AssetMediaInfoPtr oldAssetMediaInfo = 
					SRProject::GetInstance()->GetAssetMediaInfo(filePath);

	ASL::String mediaLocator = oldAssetMediaInfo->GetMediaPath();
	ASL::String fileCreateTime;
	ASL::String fileModifyTime;

	if (!IsLoggingClipForEA())
	{
		fileCreateTime = PL::SRLibrarySupport::GetFileCreateTime(oldAssetMediaInfo->GetMediaPath());
		fileModifyTime = PL::SRLibrarySupport::GetFileModifyTime(oldAssetMediaInfo->GetMediaPath());
	}

	AssetMediaInfoPtr newAssetMediaInfo = AssetMediaInfo::Create(
														oldAssetMediaInfo->GetAssetMediaType(),
														oldAssetMediaInfo->GetAssetMediaInfoGUID(),
														mediaLocator,
														xmpStr,
														oldAssetMediaInfo->GetFileContent(),
														oldAssetMediaInfo->GetCustomMetadata(),
														fileCreateTime,
														fileModifyTime,
														oldAssetMediaInfo->GetRateTimeBase(),
														oldAssetMediaInfo->GetRateNtsc(),
														oldAssetMediaInfo->GetAliasName(),
														oldAssetMediaInfo->GetNeedSavePreCheck());

	//	Do pre-check before send save message(read-only, disk space, xmp modtime, off-line)
	bool result = SRUtilitiesPrivate::PreCheckBeforeSendSaveMsg(
		filePath,
		newAssetMediaInfo,
		outDiscardChanges,
		!isCloseOrExit && mUserHasCanceledSaveFailure);


	if (result)
	{
		// remember current undo/redo history position so if need to rollback, we know where we should stop
		BE::IExecutorRef executor(MZ::GetProject());
		mUndoableActionCountAfterAutoSave = executor->GetUndoableActionCount();

		//	[Comment] We should update AssetMediaInfo cache when we save Logging Clip.
		//	or the cache may out of data compared with library.
		SRProject::GetInstance()->GetAssetMediaInfoWrapper(filePath)->RefreshMediaXMP(newAssetMediaInfo->GetXMPString());

		//	Shouldn't trigger follow code under EA case for EA's metadata saving should be only take cared by EAMediaInfo in MCM
		if (!newAssetMediaInfo->GetNeedSavePreCheck() && !IsLoggingClipForEA())
		{
			BE::IMediaMetaDataRef  mediaMetaData = 
					BE::IMediaMetaDataRef(BE::MasterClipUtils::GetDeprecatedMedia(srMedia->GetMasterClip())->GetMediaInfo());
			ASL::SInt32 requestID;

			mediaMetaData->WriteMetaDatum(
					MF::BinaryData(),
					BE::kMetaDataType_XMPBinary,
					0, 
					MF::BinaryData(newAssetMediaInfo->GetXMPString()->c_str(),newAssetMediaInfo->GetXMPString()->length()),
					true,
					true,
					&requestID);
		}
	}

	//	No matter success or not, we should always invoke callback function.
	if (inSaveCallBackFn)
	{
		if (result)
		{
			inSaveCallBackFn(mAssetItem, newAssetMediaInfo, ASL::kSuccess, ASL::String());
		}
		else
		{
			inSaveCallBackFn(mAssetItem, newAssetMediaInfo, ASL::ResultFlags::kResultTypeFailure, DVA_STR("Save check failed! The clip may be off-line or read-only."));
		}
	}
	return result;
}

/*
**
*/
UIF::MBResult::Type SRLoggingClip::Save(bool isSilent, bool isCloseOrExit)
{
	DVA_ASSERT_MSG(dvacore::threads::CurrentThreadIsMainThread(), "Save logging clip from work thread?!");
	// [TRICKY] To avoid re-enter logging clip save. It will happen if user close clip with shortcut when editing something in edit box.
	//	We should refactor save workflow to remove this trick.
	if (sIsSavingLoggingClip)
		return UIF::MBResult::kMBResultCancel;
	dvacore::utility::ScSetReset<bool> SavingGuard(&sIsSavingLoggingClip, true);
	return Save(isSilent, isCloseOrExit, boost::bind(SaveLoggingClipCallBackRoutine, _1, _2, _3, _4));
}

/*
**
*/
bool SRLoggingClip::IsLoggingClipForEA() const
{
	return MZ::Utilities::IsEAMediaPath(GetSRClipItem()->GetAssetItem()->GetMediaPath());
}

/*
**
*/
void SRLoggingClip::OnShowTimecodeStatusChanged()
{
	BE::ISequenceRef sequence = GetBESequence();
	if (sequence)
	{
		MZ::ConformSequenceToTimecodeOverlay(sequence);
	}
}

/*
**
*/
void SRLoggingClip::OnSilkRoadMarkerRenamed(PL::CottonwoodMarkerList const& inChangedMarkerList)
{
	if (inChangedMarkerList.empty())
	{
		return;
	}

	AssetItemPtr assetItem = GetAssetItem();

	BOOST_FOREACH(PL::CottonwoodMarker const& changedMarker, inChangedMarkerList)
	{
		if (changedMarker.GetType() != PL::MarkerType::kInOutMarkerType  || 
			changedMarker.GetGUID() != assetItem->GetAssetMarkerGuid() ||
			changedMarker.GetName() == assetItem->GetAssetName())
		{
			continue;
		}

		ASL::String oldName = assetItem->GetAssetName();
		assetItem->SetAssetName(changedMarker.GetName());

		ASL::StationUtils::BroadcastMessage(
			MZ::kStation_PreludeProject,
			PL::PrimaryClipRenamedMessage(assetItem->GetMediaPath(), assetItem->GetAssetName(), oldName));
	}
}

/*
**
*/
void SRLoggingClip::UpdateSelection()
{
	SRClipItemPtr clipItem = GetSRClipItem();

	ISRMediaRef  srMedia = clipItem->GetSRMedia();
	PL::AssetItemList newSelection;
	PL::AssetItemList selectedAssetItems = SRProject::GetInstance()->GetAssetSelectionManager()->GetSelectedAssetItemList();
	PL::ISRMarkersRef srMarkers = ISRMarkerOwnerRef(srMedia)->GetMarkers();
	if (srMarkers)
	{
		PL::MarkerTrack markerTrack = srMarkers->GetMarkers();
		BOOST_FOREACH(PL::AssetItemPtr const& eachOldAsset, selectedAssetItems)
		{
			if (eachOldAsset->GetAssetMediaType() == PL::kAssetLibraryType_SubClip && 
				eachOldAsset->GetMediaPath() == clipItem->GetOriginalClipPath())
			{
				for (PL::MarkerTrack::iterator trackItr = markerTrack.begin(); trackItr != markerTrack.end(); ++trackItr)
				{
					PL::CottonwoodMarker marker = (*trackItr).second;
					if (marker.GetGUID() == eachOldAsset->GetAssetMarkerGuid())
					{
						// update subclip marker...
						eachOldAsset->SetInPoint(marker.GetStartTime());
						eachOldAsset->SetDuration(marker.GetDuration());
						eachOldAsset->SetAssetName(marker.GetName());
						newSelection.push_back(eachOldAsset);
						break;
					}
				}
			}
			else
			{
				newSelection.push_back(eachOldAsset);
			}
		}

		SRProject::GetInstance()->GetAssetSelectionManager()->SetSelectedAssetItemList(newSelection, true);
	}
}

/*
**
*/
UIF::MBResult::Type SRLoggingClip::Save(
	bool isSilent,
	bool isCloseOrExit,
	ISRPrimaryClip::SaveCallBackFn const& inSaveCallBackFn)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	UIF::MBResult::Type returnValue = UIF::MBResult::kMBResultOK;

	// Save() may be called async after the clip is closed. But at that time, the clip is saved already.
	// So we check IsDirty() firstly.
	if (IsDirty())
	{
		UIF::MBResult::Type ret;
		ISRMediaRef  srMedia = mSRClipItem->GetSRMedia();
		DVA_ASSERT(srMedia);


		if (!isSilent)
		{
			dvacore::UTF16String saveMarkerChanges = dvacore::ZString(
				"$$$/Prelude/Mezzanine/SRLoggingClip/SaveMarkerChangesPrompt=Do you want to save changes to @0?",
				GetName());
			ret = SRUtilitiesPrivate::PromptForSave(
					dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/SaveChanges=Save Changes"),
					saveMarkerChanges);
		}
		else
		{
			ret = UIF::MBResult::kMBResultYes;
		}

		if (ret == UIF::MBResult::kMBResultYes)
		{
			bool discardChanges = false;
			if (SendSaveNotification(discardChanges, isCloseOrExit, inSaveCallBackFn))
			{
				// if save succeeded, we clear mUserHasCanceledSaveFailure so that next time save failure will bring up dialog.
				mUserHasCanceledSaveFailure = false;
				srMedia->SetDirty(false);
			}
			else
			{
				// save failed, we should refresh to read-only state
				srMedia->UpdateWritableState(false);

				// if discard, we should revert all unsaved changes
				if (discardChanges)
				{
					// revert unsaved changes by undo these actions
					BE::IExecutorRef executor(MZ::GetProject());
					ASL::UInt32 currentUndoableActionCount = executor->GetUndoableActionCount();
					if (currentUndoableActionCount < mUndoableActionCountAfterAutoSave)
					{
						executor->Redo(mUndoableActionCountAfterAutoSave - currentUndoableActionCount);
					}
					else if (currentUndoableActionCount > mUndoableActionCountAfterAutoSave)
					{
						executor->Undo(currentUndoableActionCount - mUndoableActionCountAfterAutoSave);
					}
					// The Undo stack doesn't broadcast changes, so do it myself...
					ASL::StationUtils::BroadcastMessage(MZ::kStation_Undo, MZ::UndoStackChangedMessage());

					// if discard save, we clear mUserHasCanceledSaveFailure so that next time save failure will bring up dialog.
					mUserHasCanceledSaveFailure = false;
					srMedia->SetDirty(false);
					returnValue = UIF::MBResult::kMBResultNo;
				}
				else
				{
					// user select Cancel, we should remember it and don't pop up dialog again until close or exit.
					mUserHasCanceledSaveFailure = true;
					returnValue = UIF::MBResult::kMBResultCancel;
				}
			}
		}
		else if (ret == UIF::MBResult::kMBResultNo)
		{
			LoadCachedXMPData();
			srMedia->SetDirty(false);
			returnValue = UIF::MBResult::kMBResultNo;
		}
		else
		{
			returnValue = UIF::MBResult::kMBResultCancel;
		}
		
		UpdateSelection();
	}

	return returnValue;
}

/*
**
*/
BE::IMasterClipRef SRLoggingClip::GetMasterClip() const
{
	ISRMediaRef srMedia = GetSRClipItem()->GetSRMedia();
	DVA_ASSERT(srMedia != NULL);
	return srMedia != NULL ? srMedia->GetMasterClip() : BE::IMasterClipRef();
}

/*
**
*/
PL::AssetItemPtr SRLoggingClip::GetAssetItem() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return mAssetItem;
}

/*
**
*/
PL::AssetMediaInfoPtr SRLoggingClip::GetAssetMediaInfo() const
{
	PL::AssetMediaInfoPtr assetMediaInfo = PL::SRProject::GetInstance()->GetAssetMediaInfo(GetSRClipItem()->GetOriginalClipPath());
	DVA_ASSERT(assetMediaInfo != NULL);
	return assetMediaInfo; 
}

/*
**
*/
SRClipItemPtr SRLoggingClip::GetSRClipItem() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return mSRClipItem;
}

/*
**
*/
bool SRLoggingClip::IsOffline() const
{
	if (IsLoggingClipForEA())
	{
		return false;
	}

	SRClipItemPtr clipItemPtr = GetSRClipItem();
	if(clipItemPtr)
	{
		return !ASL::PathUtils::ExistsOnDisk(clipItemPtr->GetOriginalClipPath());
	}
	return true;
}

/*
**
*/
PL::ISRMarkersRef SRLoggingClip::GetMarkers(BE::IMasterClipRef inMasterClip) const
{
	ISRMediaRef srMedia = GetSRClipItem()->GetSRMedia();
	DVA_ASSERT(srMedia != NULL);

	// Compare the media-based MasterClip, maybe it's not the input one
	if (srMedia)
	{
		if (srMedia->GetMasterClip() == inMasterClip)
		{
			return ISRMarkerOwnerRef(srMedia)->GetMarkers();
		}
	}

	return PL::ISRMarkersRef();
}

/*
**
*/
SRMarkersList SRLoggingClip::GetMarkersList() const
{
	SRMarkersList markersList;
	ISRMediaRef srMedia = GetSRClipItem()->GetSRMedia();
	DVA_ASSERT(srMedia != NULL);
	markersList.push_back(ISRMarkerOwnerRef(srMedia)->GetMarkers());
	return markersList;
}

/*
**
*/
ASL::PathnameList SRLoggingClip::GetReferencedMediaPath()
{
	ASL::PathnameList mediaPathList;
	mediaPathList.push_back(GetSRClipItem()->GetOriginalClipPath());
	return mediaPathList;
}

/*
**
*/
SRLoggingClipRef SRLoggingClip::Create(PL::AssetItemPtr const& inAssetItem)
{
	SRLoggingClipRef srLoggingClip(CreateClassRef());
	srLoggingClip->Init(inAssetItem);
	return srLoggingClip;
}

/*
**
*/
bool SRLoggingClip::Relink(
	PL::AssetItemPtr const& inRelinkAssetItem)
{
	SetDirty(false);
	ClearSelection();
	Init(inRelinkAssetItem);

	ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadLoggingClipRelinked(),
			true);

	return true;
}


/*
**
*/
void SRLoggingClip::Init(PL::AssetItemPtr const& inAssetItem)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	DVA_ASSERT ( inAssetItem != NULL );
 
	SRClipItems	clipItems;
	PL::AssetItemList assetitemList;

	//	Means it is EA asset
	if (MZ::Utilities::IsEAMediaPath(inAssetItem->GetMediaPath()))
	{
		mAssetItem = inAssetItem;

	}
	else
	{
		mAssetItem = PL::AssetItemPtr(new AssetItem(
			PL::kAssetLibraryType_MasterClip,
			inAssetItem->GetMediaPath(),
			inAssetItem->GetAssetGUID(),
			inAssetItem->GetParentGUID(),
			inAssetItem->GetAssetMediaInfoGUID(),
			GetMasterClipNameFromAssetItem(inAssetItem)));
	}

	PL::AssetItemPtr appendAssetItem = PL::AssetItemPtr(new AssetItem(
											inAssetItem->GetAssetMediaType(),
											inAssetItem->GetMediaPath(),
											inAssetItem->GetAssetGUID(),
											inAssetItem->GetParentGUID(),
											inAssetItem->GetAssetMediaInfoGUID(),
											ASL::String()));

	assetitemList.push_back(appendAssetItem);
	Utilities::BuildClipItemFromLibraryInfo(assetitemList, clipItems);
	DVA_ASSERT(clipItems.size() >= 1);
	mSRClipItem = clipItems.front();
	mSequenceItem = BE::IProjectItemRef();
    
    SRMarkerSelection::SetPrimaryClip(PL::ISRPrimaryClipPlaybackRef(this));
	ASL::StationUtils::AddListener(mSRClipItem->GetSRMedia()->GetStationID(), this);
	ASL::StationUtils::AddListener(SRProject::GetInstance()->GetAssetMediaInfoWrapper(mAssetItem->GetMediaPath())->GetStationID(), this);
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
bool SRLoggingClip::IsSupportMarkerEditing() const
{
    ISRMediaRef srMedia =  GetSRClipItem()->GetSRMedia();
    if ( srMedia != NULL && srMedia->GetMasterClip() != NULL)
    {
        return ( srMedia->IsWritable() && !MZ::BEUtilities::IsMasterClipStill(srMedia->GetMasterClip()));
    }
	return true;
}

/*
**
*/
bool SRLoggingClip::IsSupportTimeLineEditing() const
{
	return false;
}


/*
**
*/
bool SRLoggingClip::AttachToTimeLine()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if (NULL == mSequenceItem)
	{
		CreateSequence();
	}

	SilkRoadPrivateCreator::AttachSequenceToTimeLineEditor(mSequenceItem);
	return true;
}

/*
**
*/
bool SRLoggingClip::DetachFromTimeLine()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	SilkRoadPrivateCreator::DetachSequenceFromTimeLineEditor(mSequenceItem);
	return true;
}

/*
**
*/
bool SRLoggingClip::CreateSequence()
{
	if (!mSequenceItem)
	{
		SRClipItems	clipItems;
		if ( mSRClipItem != NULL )
		{
			clipItems.push_back(mSRClipItem);
		}
		mSequenceItem = SilkRoadPrivateCreator::BuildSequence(clipItems);

		//	For logging clip, we always mark trackItems to selected state.
		SelectClipItem();
	}
	return true;
}

/*
**
*/
void SRLoggingClip::SelectClipItem()
{
	BE::ISequenceRef sequence = GetBESequence();
	DVA_ASSERT(sequence);
	BE::ITrackItemSelectionRef trackItemSelection = sequence->GetCurrentSelection();

	BE::ITrackItemRef trackItem = GetSRClipItem()->GetTrackItem();
	DVA_ASSERT(trackItem);
	if (trackItem)
	{
		BE::ITrackItemGroupRef group(sequence->FindLink(trackItem));
		if (group)
		{
			trackItemSelection->AddLink(group);
		}
		else
		{
			trackItemSelection->AddItem(trackItem);
		}					
	}
}

/*
**
*/
BE::IMasterClipRef SRLoggingClip::GetSequenceMasterClip() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return (mSequenceItem != NULL) ? MZ::BEUtilities::GetMasteClipFromProjectitem(mSequenceItem) : BE::IMasterClipRef();
}

/*
**
*/
dvacore::UTF16String SRLoggingClip::GetName() const
{
	AssetItemPtr assetItem = GetAssetItem();

	DVA_ASSERT (assetItem != NULL);
	
	if (assetItem)
	{
		return assetItem->GetAssetName();
	}

	return  dvacore::UTF16String();
}

/*
**
*/
ASL::Color SRLoggingClip::GetTrackItemLabelColor() const
{
	return ASL::Color(192, 190, 161);
}

/*
**
*/
void SRLoggingClip::OnSRMediaChanged(ISRMediaRef const& inSRMedia)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if (inSRMedia == mSRClipItem->GetSRMedia())
	{
		//	If sequence is opened, 
		if (mSequenceItem)
		{
			BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem);

			//	Get selection from current sequence 
			BE::ITrackItemSelectionRef aSelection = sequence->GetCurrentSelection();

			//	Do trim to make sequence show the maximum length of current media
			MZ::SequenceActions::TrimItems(
						sequence, 
						aSelection, 
						BE::kSequenceTrimType_Tail, 
						inSRMedia->GetMasterClip()->GetMaxCurrentUntrimmedDuration(BE::kMediaType_Any),
						true,			// inAlignToVideo
						true,			// inIsClipRelativeTime
						false);			// inIsUndoable
		}
	}
}

/*
**
*/
ASL::String SRLoggingClip::GetSubClipName(const BE::ITrackItemRef inTrackItem) const
{
	return GetSRClipItem()->GetTrackItem() == inTrackItem 
		? GetName()
		: ASL::String(DVA_STR(""));
}

/*
** 
*/
ASL::Guid SRLoggingClip::GetSubClipGUID(const BE::ITrackItemRef inTrackItem) const
{
	SRClipItemPtr clipItem = GetSRClipItem();

	return clipItem->GetTrackItem() == inTrackItem 
		? clipItem->GetSubClipGUID()
		: ASL::Guid();
}

bool SRLoggingClip::IsWritable() const
{
	SRClipItemPtr clipItem = GetSRClipItem();
	if(clipItem)
	{
		ISRMediaRef srMedia = clipItem->GetSRMedia();
		return srMedia != NULL ? srMedia->IsWritable() : false;
	}
	return false;
}

void SRLoggingClip::OnUndoStackCleared()
{
	BE::IExecutorRef executor(MZ::GetProject());
	mUndoableActionCountAfterAutoSave = executor->GetUndoableActionCount();
}

} // namespace PL
