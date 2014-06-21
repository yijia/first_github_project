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
#include "PLUtilities.h"
#include "PLRoughCut.h"
#include "PLRoughCutUtils.h"
#include "IPLMarkers.h"
#include "PLUtilitiesPrivate.h"
#include "PLMessage.h"
#include "PLLibrarySupport.h"
#include "PLModulePicker.h"
#include "PLMarkers.h"
#include "IPLMarkerOwner.h"

//	MZ
#include "PLConstants.h"
#include "MZActivation.h"
#include "MZBEUtilities.h"
#include "MZProject.h"
#include "MZSequence.h"
#include "MZUtilities.h"
#include "MZMessages.h"
#include "MZTimecodeOverlay.h"
#include "MZUndo.h"

// ML
#include "MLPlayer.h"

// SR
#include "PLProject.h"

// BE
#include "BEMasterClip.h"
#include "BESequenceEditor.h"
#include "BESequence.h"
#include "BEClip.h"
#include "BEMedia.h"
#include "BETrack.h"
#include "BEExecutor.h"
#include "BE/Sequence/IVideoTransitionTrackItem.h"

//	ASL
#include "ASLStationUtils.h"
#include "ASLPathUtils.h"
#include "ASLAsyncCallFromMainThread.h"

// DVA
#include "dvacore/config/Localizer.h"
#include "dvacore/filesupport/file/file.h"
#include "dvacore/threads/SharedThreads.h"
#include "dvacore/utility/StringCompare.h"

// UIF
#include "UIFDocumentManager.h"
#include "UIFHeadlights.h"

namespace PL
{

namespace
{

// Work around to avoid save function re-enter.
bool sIsSavingRoughcut = false;

/*
**
*/
void SaveRCCallBackRoutine(
	PL::AssetItemPtr const& inAssetItem, 
	PL::AssetMediaInfoPtr const& inAssetMediaInfo,
	ASL::Result result,
	ASL::String const& inErrorMsg)
{
	if (ASL::ResultFailed(result))
		return;
	//	We should save rough cut asset item since rc sub clips can only be put into it.
	SRProject::GetInstance()->GetAssetLibraryNotifier()->SaveRoughCut(
		ASL::Guid::CreateUnique(), ASL::Guid::CreateUnique(), inAssetItem, inAssetMediaInfo);
}

}

ASL_MESSAGE_MAP_DEFINE(SRRoughCut)
	ASL_MESSAGE_HANDLER(MZ::ShowTimecodeStatusChanged, OnShowTimecodeStatusChanged)						
	ASL_MESSAGE_HANDLER(MZ::UndoStackClearedMessage, OnUndoStackCleared)
	ASL_MESSAGE_HANDLER(PL::AssetItemNameChanged, OnAssetItemRename)
ASL_MESSAGE_MAP_END

/*
**
*/
BE::ISequenceRef SRRoughCut::GetBESequence()
{
	return MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem);
}

/*
**
*/
SRRoughCutRef SRRoughCut::Create(
								PL::AssetItemPtr const& inAssetItem)
{
	UIF::Headlights::LogEvent("New", "CreateRoughcut", "CreateRoughCut");

	SRRoughCutRef srRoughCut = CreateClassRef();
	srRoughCut->Init(inAssetItem);

	return srRoughCut;
}

/*
**
*/
void SRRoughCut::Init(
				PL::AssetItemPtr const& inAssetItem)
{
	mAssetItem = inAssetItem;
	ASL::String filePath = mAssetItem->GetMediaPath();
	mIsWritable = UIF::IsEAMode() ? true : (ASL::PathUtils::ExistsOnDisk(filePath) && !ASL::PathUtils::IsReadOnly(filePath));

	Load();

    SRMarkerSelection::SetPrimaryClip(PL::ISRPrimaryClipPlaybackRef(this));
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
void SRRoughCut::OnAssetItemRename(ASL::String const& inMediaLocator, ASL::String const& inNewName)
{
	AssetItemPtr assetItem = GetAssetItem();

	if (MZ::Utilities::NormalizePathWithoutUNC(assetItem->GetMediaPath()) == MZ::Utilities::NormalizePathWithoutUNC(inMediaLocator))
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
void SRRoughCut::Load()
{
	mTrashItems.clear();
	mSequenceItem = BE::IProjectItemRef();
	mClipItems.clear();

	const AssetItemList& subAssetItems = mAssetItem->GetSubAssetItemList();
	AssetItemList::const_iterator iter = subAssetItems.begin();
	AssetItemList::const_iterator end = subAssetItems.end();

	for (; iter != end; ++iter)
	{
		PL::SRClipItemPtr srClipItem(new PL::SRClipItem(*iter));

		// If not given from file: Create a new GUID
		if (((*iter)->GetAssetClipItemGuid()).AsString().empty())	
		{
			srClipItem->SetSubClipGUID(ASL::Guid::CreateUnique());
		}
		else
		{
			srClipItem->SetSubClipGUID(ASL::Guid((*iter)->GetAssetClipItemGuid().AsString()));
		}

		mClipItems.push_back(srClipItem);
	}
}

/*
**
*/
SRRoughCut::SRRoughCut()
				: 
				mEditTime(dvamediatypes::kTime_Zero),
				mIsAttached(false),
				mIsDirty(false),
				mUndoableActionCountAfterAutoSave(0),
				mUserHasCanceledSaveFailure(false),
				mIsWritable(false)
{
	ASL::StationUtils::AddListener(MZ::kStation_Undo, this);
}

/*
**
*/
SRRoughCut::~SRRoughCut()
{
	ASL::StationUtils::RemoveListener(MZ::kStation_Undo, this);
}

/*
**
*/
dvacore::UTF16String SRRoughCut::GetName() const
{
	ASL::String rcName = GetRoughCutName();
	return dvacore::ZString("$$$/Prelude/Mezzanine/CottonwoodMasterClip/RoughCut=Rough Cut") + 
		DVA_STR(" (") + 
		rcName +
		DVA_STR(")") ;
}

/*
**
*/
dvacore::UTF16String SRRoughCut::GetRoughCutName() const
{
	ASL::String rcName = GetAssetItem()->GetAssetName();
	if (rcName.empty())
	{
		rcName = ASL::PathUtils::GetFullFilePart(GetFilePath());
	}

	return rcName;
}

/*
**
*/
bool SRRoughCut::IsDirty() const
{
	return mIsDirty;
}

/*
**
*/
bool SRRoughCut::Relink(
	PL::AssetItemPtr const& inRelinkAssetItem)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	DVA_ASSERT(inRelinkAssetItem->GetAssetGUID() == mAssetItem->GetAssetGUID());
	ClearSelection();
	Init(inRelinkAssetItem);
	SetDirty(false);

	ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadRoughCutRelinked(),
			true);

	return true;
}

/*
**
*/
void SRRoughCut::OnShowTimecodeStatusChanged()
{
	if (mSequenceItem)
	{
		MZ::ConformSequenceToTimecodeOverlay(MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem));
	}
}

/*
**
*/
void SRRoughCut::SetDirty(bool inIsDirty)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if (mIsDirty != inIsDirty)
	{
		mIsDirty = inIsDirty;

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadRoughCutDirtyChanged(),
			true);

		SRProject::GetInstance()->GetAssetLibraryNotifier()->AssetDirtyStateChanged(
			ASL::Guid::CreateUnique(),
			ASL::Guid::CreateUnique(),
			mAssetItem,
			mIsDirty);
	}

	if (ModulePicker::IsAutoSaveEnabled())
	{
		if (mIsDirty)
		{
			ASL::AsyncCallFromMainThread(
					boost::bind<UIF::MBResult::Type>(&SRRoughCut::Save, this, true, false));
		}
	}
}

/*
**
*/
BE::IMasterClipRef SRRoughCut::GetSequenceMasterClip() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return (mSequenceItem != NULL) ? MZ::BEUtilities::GetMasteClipFromProjectitem(mSequenceItem) : BE::IMasterClipRef();
}

/*
**
*/
PL::AssetMediaInfoPtr SRRoughCut::GetAssetMediaInfo() const
{
	PL::AssetMediaInfoPtr assetMediaInfo
						= PL::SRProject::GetInstance()->GetAssetMediaInfo(GetFilePath());
	DVA_ASSERT(assetMediaInfo != NULL);
	return assetMediaInfo;
}

/*
**
*/
PL::AssetItemPtr SRRoughCut::GetAssetItem() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return mAssetItem;
}

/*
**
*/
SRClipItems SRRoughCut::GetClipItems() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	return mClipItems;
}

/*
**
*/
void SRRoughCut::ResetRoughtClipItemFromSequence()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	BE::ISequenceRef sequence = GetBESequence();
	if (sequence == NULL)
	{
		return;
	}

	BE::TrackItemVector trackItems = MZ::BEUtilities::GetTrackItemsFromTheFirstAvailableTrack(sequence);

	// Get clipItem from sequence
	SRClipItems totalClipItems = mClipItems;
	totalClipItems.splice(totalClipItems.end(), mTrashItems);

	// Clear the current clipItem and trashItem
	mClipItems.clear();
	mTrashItems.clear();

	BE::TrackItemVector::iterator trackIter = trackItems.begin();
	BE::TrackItemVector::iterator trackEnd = trackItems.end();

	for (; trackIter != trackEnd; ++trackIter)
	{
		BE::ITrackItemRef aTrackItem(*trackIter);
		if (aTrackItem == NULL || aTrackItem->GetType() == BE::kTrackItemType_Empty)
		{
			continue;
		}

		SRClipItems::iterator clipIter = totalClipItems.begin();
		SRClipItems::iterator clipEnd  = totalClipItems.end();

		for (; clipIter != clipEnd; ++clipIter)
		{
			if ((*clipIter)->GetTrackItem() == aTrackItem)
			{
				mClipItems.push_back(*clipIter);

				// sync between AssetItem and TrackItem
				PL::AssetItemPtr assetItem = (*clipIter)->GetAssetItem();

				BE::IClipTrackItemRef clipTrack = BE::IClipTrackItemRef(aTrackItem);
				BE::IChildClipRef childClip = clipTrack->GetChildClip();
				BE::IClipRef clipRef = childClip->GetClip();

				assetItem->SetInPoint(clipRef->GetInPoint());				
				assetItem->SetDuration(aTrackItem->GetDuration());

				break;
			}
		}

		DVA_ASSERT(clipIter != clipEnd);
		if (clipIter != clipEnd)
		{
			totalClipItems.erase(clipIter);
		}
	}

	// For the left clips, we moved them to the trash.
	mTrashItems = totalClipItems;
}

/*
**
*/
dvacore::UTF16String SRRoughCut::GetFilePath()const
{
	return GetAssetItem()->GetMediaPath();
}

/*
**
*/
UIF::MBResult::Type SRRoughCut::Save(bool isSilent, bool isCloseOrExit)
{
	DVA_ASSERT_MSG(dvacore::threads::CurrentThreadIsMainThread(), "Save rough cut from work thread?!");
	// [TRICKY] To avoid re-enter rough cut save. It will happen if user try to change a read-only RC.
	//	We should refactor save workflow to remove this trick.
	if (sIsSavingRoughcut)
		return UIF::MBResult::kMBResultCancel;
	dvacore::utility::ScSetReset<bool> SavingGuard(&sIsSavingRoughcut, true);
	return Save(isSilent, isCloseOrExit, boost::bind(SaveRCCallBackRoutine, _1, _2, _3, _4));
}

/*
**
*/
PL::AssetItemPtr SRRoughCut::BuildRCAssetItem()
{
	AssetItemPtr assetItem = GetAssetItem();

	PL::AssetItemPtr  roughCutAssetItem(new PL::AssetItem(
		kAssetLibraryType_RoughCut,
		assetItem->GetMediaPath(),
		assetItem->GetAssetGUID(),
		assetItem->GetParentGUID(),
		assetItem->GetAssetMediaInfoGUID(),
		assetItem->GetAssetName()));

	// create sub clip asset item
	{
		PL::AssetItemList subClipList;
		SRClipItems clipItems = GetClipItems();

		SRClipItems::const_iterator iter = clipItems.begin();
		SRClipItems::const_iterator iterEnd = clipItems.end();
		for (; iter != iterEnd; ++iter)
		{
			SRClipItemPtr clipItem = *iter;
			PL::AssetItemPtr  subClipAssetItem(new PL::AssetItem(
				PL::kAssetLibraryType_RCSubClip,
				clipItem->GetAssetItem()->GetMediaPath(),
				clipItem->GetAssetItem()->GetAssetGUID(),
				clipItem->GetAssetItem()->GetParentGUID(),
				clipItem->GetAssetItem()->GetAssetMediaInfoGUID(),
				clipItem->GetAssetItem()->GetAssetName(),
				DVA_STR(""),
				clipItem->GetStartTime(),
				clipItem->GetDuration(),
				dvamediatypes::kTime_Invalid,
				dvamediatypes::kTime_Invalid,
				clipItem->GetAssetItem()->GetAssetClipItemGuid()));
			subClipList.push_back(subClipAssetItem);
		}
		roughCutAssetItem->SetSubAssetItemList(subClipList);
	}

	// build transitions
	PL::TrackTransitionMap videoTransitions = 
		PL::CloneTrackTransitionMap(assetItem->GetTrackTransitions(BE::kMediaType_Video));
	PL::TrackTransitionMap audioTransitions = 
		PL::CloneTrackTransitionMap(assetItem->GetTrackTransitions(BE::kMediaType_Audio));

	roughCutAssetItem->SetTrackTransitions(videoTransitions, BE::kMediaType_Video);
	roughCutAssetItem->SetTrackTransitions(audioTransitions, BE::kMediaType_Audio);

	return roughCutAssetItem;
}

/*
**
*/
bool SRRoughCut::IsRoughCutForEA() const
{
	return MZ::Utilities::IsEAMediaPath(GetAssetItem()->GetMediaPath());
}

/*
**
*/
void SRRoughCut::UpdateAssetItem()
{
	BE::ISequenceRef sequence = GetBESequence();
	if (sequence == NULL)
	{
		return;
	}

	PL::AssetItemList subAssetItemList;
	for (SRClipItems::iterator clipIter = mClipItems.begin();
		clipIter != mClipItems.end();
		++clipIter)
	{
		PL::AssetItemPtr subAssetItem = (*clipIter)->GetAssetItem();
		subAssetItemList.push_back(subAssetItem);
	}

	// build transitions
	PL::TrackTransitionMap videoTransitions =
		PL::Utilities::BuildTransitionItems(sequence, BE::kMediaType_Video);
	PL::TrackTransitionMap audioTransitions =
		PL::Utilities::BuildTransitionItems(sequence, BE::kMediaType_Audio);

	{
		ASL::CriticalSectionLock lock(mCriticalSection);
		mAssetItem->SetSubAssetItemList(subAssetItemList);

		mAssetItem->SetTrackTransitions(videoTransitions, BE::kMediaType_Video);
		mAssetItem->SetTrackTransitions(audioTransitions, BE::kMediaType_Audio);
	}
}

/*
**
*/
UIF::MBResult::Type SRRoughCut::Save(
							bool isSilent,
							bool isCloseOrExit,
							ISRPrimaryClip::SaveCallBackFn const& inSaveCallBackFn)
{
	// Step 1: Check if need to do save
	if (!IsDirty())
	{
		return UIF::MBResult::kMBResultOK;
	}

	// Step 2: If not silent, prompt for save. If user select NO or CANCEL, will not do next step to save.
	// Maybe this step should be extracted out and put into the caller.
	if (!isSilent)
	{
		dvacore::UTF16String saveRoughCutChanges = 
			dvacore::ZString("$$$/Prelude/Mezzanine/CottonwoodMasterClip/SaveRoughCutChangesPrompt=Do you want to save changes to Rough Cut '@0'?", GetRoughCutName());

		UIF::MBResult::Type ret = SRUtilitiesPrivate::PromptForSave(
				dvacore::ZString("$$$/Prelude/Mezzanine/CottonwoodMasterClip/SaveChanges=Save Changes"),
				saveRoughCutChanges
			);
		if (ret == UIF::MBResult::kMBResultNo)
		{
			SetDirty(false);
			return ret;
		}
		else if (ret == UIF::MBResult::kMBResultCancel)
		{
			return ret;
		}
	}

	bool needMessageBoxForSaveFailed = isCloseOrExit || !mUserHasCanceledSaveFailure;
	// Step 3: Need to do save
	UpdateAssetItem();

	ASL::String fileContent = PL::BuildRoughCutContent(
		GetBESequence(),
		GetClipItems(),
		GetName());

	ASL::String fileCreateTime;
	ASL::String fileModifyTime;

	AssetItemPtr assetItem = GetAssetItem();
	ASL::String filePath = assetItem->GetMediaPath();
	if (!IsRoughCutForEA())
	{
		fileCreateTime = PL::SRLibrarySupport::GetFileCreateTime(filePath);
		fileModifyTime = PL::SRLibrarySupport::GetFileModifyTime(filePath);
	}

	ASL::String rateTimeBase, rateNTSC;
	dvamediatypes::FrameRate sequenceFrameRate = GetBESequence()->GetVideoFrameRate();
	FCPRateToTimebaseAndNtsc(rateTimeBase, rateNTSC, sequenceFrameRate);
	PL::AssetMediaInfoPtr roughCutMediaInfo = PL::AssetMediaInfo::CreateRoughCutMediaInfo(
		assetItem->GetAssetGUID(),
		filePath,
		fileContent,
		fileCreateTime,
		fileModifyTime,
		rateTimeBase,
		rateNTSC);

	bool discardChanges = false;
	if (SRProject::GetInstance()->GetAssetMediaInfo(filePath)->GetNeedSavePreCheck() && !IsRoughCutForEA())
	{
		const dvacore::UTF16String tipsForNextOptions = dvacore::ZString("$$$/Prelude/Mezzanine/SRRoughCut/SaveFailedNextOptionsTips=All new changes will be discarded.");
		// Check off-line
		if (!ASL::PathUtils::ExistsOnDisk(filePath))
		{
			ASL::Result result = CreateEmptyFile(filePath);
			// Ensure the folder can be written
			if (!ASL::ResultSucceeded(result))
			{
				const ASL::String& warningCaption = 
					dvacore::ZString("$$$/Prelude/Mezzanine/SRRoughCut/WarnAssetOfflineCaption=Rough cut Off-line");
				dvacore::UTF16String errorMsg = dvacore::ZString(
					"$$$/Prelude/Mezzanine/SRRoughCut/WarnAssetOfflinel=The off-line rough cut \"@0\" cannot be saved.",
					MZ::Utilities::NormalizePathWithoutUNC(filePath));
				if (needMessageBoxForSaveFailed)
				{
					UIF::MessageBox(
						errorMsg + DVA_STR("\n") + tipsForNextOptions,
						warningCaption,
						UIF::MBIcon::kMBIconError);
					discardChanges = true;
				}

				if (!discardChanges)
				{
					ML::SDKErrors::SetSDKErrorString(errorMsg);
				}
				else
				{
					DiscardUnsavedChanges();
				}

				UpdateWritableState(false);

				if (inSaveCallBackFn)
				{
					inSaveCallBackFn(PL::AssetItemPtr(), roughCutMediaInfo, ASL::ResultFlags::kResultTypeFailure, DVA_STR("The rough cut file is off-line."));
				}
				return discardChanges ? UIF::MBResult::kMBResultNo : UIF::MBResult::kMBResultCancel;
			}
		}

		// Check read only
		if (ASL::PathUtils::IsReadOnly(filePath))
		{
			const ASL::String caption = dvacore::ZString("$$$/Prelude/Mezzanine/SRRoughCut/SaveReadOnly=Save File Failed");
			const dvacore::UTF16String content = dvacore::utility::ReplaceInString(dvacore::ZString("$$$/Prelude/Mezzanine/SRRoughCut/AccessIsDenied=The file @0 cannot be saved, because the file is READ ONLY."), ASL::PathUtils::GetFullFilePart(filePath.c_str()));
			if (needMessageBoxForSaveFailed)
			{
				UIF::MessageBox(
					content + DVA_STR("\n") + tipsForNextOptions,
					caption,
					UIF::MBIcon::kMBIconError);
				discardChanges = true;
			}

			if (!discardChanges)
			{
				mUserHasCanceledSaveFailure = true;
				ML::SDKErrors::SetSDKErrorString(content);
			}
			else
			{
				DiscardUnsavedChanges();
				mUserHasCanceledSaveFailure = false;
			}

			UpdateWritableState(false);

			if (inSaveCallBackFn)
			{
				inSaveCallBackFn(PL::AssetItemPtr(), roughCutMediaInfo, ASL::ResultFlags::kResultTypeFailure, DVA_STR("The rough cut file is write-protected."));
			}
			return discardChanges ? UIF::MBResult::kMBResultNo : UIF::MBResult::kMBResultCancel;
		}
	}

	//	Before Save, we should register assetMediaInfo in the cache.
	SRProject::GetInstance()->RegisterAssetMediaInfo(assetItem->GetMediaPath(), roughCutMediaInfo, true);

	//	Build AssetItem
	PL::AssetItemPtr  roughCutAssetItem = BuildRCAssetItem();

	// Currently a tricky solution, to update selected asset item list everytime saving a dirty rough cut
	SRProject::GetInstance()->GetAssetSelectionManager()->UpdateSelectedRCSubAssetItemList(roughCutAssetItem);

	if (inSaveCallBackFn)
	{
		inSaveCallBackFn(roughCutAssetItem, roughCutMediaInfo, ASL::kSuccess, ASL::String());
	}

	mUserHasCanceledSaveFailure = false;
	BE::IExecutorRef executor(MZ::GetProject());
	mUndoableActionCountAfterAutoSave = executor->GetUndoableActionCount();
	SetDirty(false);
	return UIF::MBResult::kMBResultOK;
}

/*
**
*/
bool SRRoughCut::AttachToTimeLine()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if (!mIsAttached)
	{
		if(NULL == mSequenceItem)
		{
			CreateSequence();
		}
		SilkRoadPrivateCreator::AttachSequenceToTimeLineEditor(mSequenceItem);
		mIsAttached = !mIsAttached;
	}
	
	return true;
}

/*
**
*/
bool SRRoughCut::TestIfRoughCutChanged()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	// compare the sequence structure with mClipItems to see if it had been changed.
	BE::ISequenceRef sequence = GetBESequence();
	if (sequence == NULL)
	{
		return false;
	}

	BE::TrackItemVector trackItems = MZ::BEUtilities::GetTrackItemsFromTheFirstAvailableTrack(sequence);
	BE::TrackItemVector nonEmptyClipItems;
	BOOST_FOREACH(BE::ITrackItemRef trackItem, trackItems)
	{
		if (trackItem->GetType() != BE::kTrackItemType_Empty)
		{
			nonEmptyClipItems.push_back(trackItem);
		}
	}

	//	size is different
	if (nonEmptyClipItems.size() != mClipItems.size())
	{
		return true;
	}

	SRClipItems::iterator clipItemIter = mClipItems.begin();
	SRClipItems::iterator clipItemEnd = mClipItems.end();

	BE::TrackItemVector::iterator trackIter = nonEmptyClipItems.begin();
	BE::TrackItemVector::iterator trackEnd = nonEmptyClipItems.end();
	for (; trackIter != trackEnd; ++trackIter, ++clipItemIter)
	{
		BE::ITrackItemRef aTrackItem(*trackIter);
		if (aTrackItem == NULL || aTrackItem->GetType() == BE::kTrackItemType_Empty)
		{
			continue;
		}
		bool isExactlyMatch = false;
		if ((*clipItemIter)->GetTrackItem() == aTrackItem)
		{
			PL::AssetItemPtr assetItem = (*clipItemIter)->GetAssetItem();
			BE::IClipTrackItemRef clipTrack = BE::IClipTrackItemRef(aTrackItem);
			BE::IChildClipRef childClip = clipTrack->GetChildClip();
			BE::IClipRef clipRef = childClip->GetClip();

			if (assetItem->GetInPoint() == clipRef->GetInPoint() &&
				assetItem->GetDuration() == aTrackItem->GetDuration())
			{
				isExactlyMatch = true;
			}
		}

		if (!isExactlyMatch)
		{
			break;
		}
	} 

	if (clipItemIter != clipItemEnd)
	{
		return true;
	}

	// check transitions
	PL::TrackTransitionMap videoTransitions = mAssetItem->GetTrackTransitions(BE::kMediaType_Video);
	PL::TrackTransitionMap latestVideoTransitions = PL::Utilities::BuildTransitionItems(sequence, BE::kMediaType_Video);
	if (!PL::TrackTransitionsAreMatch(videoTransitions, latestVideoTransitions))
	{
		return true;
	}

	PL::TrackTransitionMap audioTransitions = mAssetItem->GetTrackTransitions(BE::kMediaType_Audio);
	PL::TrackTransitionMap latestAudioTransitions = PL::Utilities::BuildTransitionItems(sequence, BE::kMediaType_Audio);
	if (!PL::TrackTransitionsAreMatch(audioTransitions, latestAudioTransitions))
	{
		return true;
	}

	return false;
}

/*
**
*/
bool SRRoughCut::DetachFromTimeLine()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if (mIsAttached)
	{
		SilkRoadPrivateCreator::DetachSequenceFromTimeLineEditor(mSequenceItem);
		mIsAttached = !mIsAttached;
	}
	
	return true;
}

/*
**
*/
bool SRRoughCut::CreateSequence()
{
	if (!mSequenceItem)
	{
		BE::IMasterClipRef sequenceTemplate;
		if (UIF::IsEAMode())
		{
			BE::IProjectItemRef rcProjectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), mAssetItem->GetAssetGUID().AsString());
			sequenceTemplate = MZ::BEUtilities::GetMasteClipFromProjectitem(rcProjectItem);
		}

		mSequenceItem = SilkRoadPrivateCreator::BuildSequence(
			mClipItems,
			mAssetItem->GetTrackTransitions(BE::kMediaType_Video),
			mAssetItem->GetTrackTransitions(BE::kMediaType_Audio), 
			sequenceTemplate, 
			MZ::kSequenceAudioTrackRule_RoughCut);
	}
	return true;
}

/*
**
*/
void SRRoughCut::SetEditTime(const dvamediatypes::TickTime& inEditTime)
{
	mEditTime = inEditTime;
}

/*
**
*/
dvamediatypes::TickTime SRRoughCut::GetEditTime() const
{
	return mEditTime;
}

/*
**
*/
ISRMediaRef SRRoughCut::GetSRMedia(BE::IMasterClipRef inMasterClip) const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
	{
		ISRMediaRef srMedia = (*f)->GetSRMedia();
		if  (srMedia != NULL && inMasterClip == srMedia->GetMasterClip())
		{
			return srMedia;
		}
	}

	// Try to get the true master clip for audio only clip since it was nested by a sequence
	for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
	{
		ISRMediaRef srMedia = (*f)->GetSRMedia();
		if  (srMedia != NULL && inMasterClip == srMedia->GetMasterClip())
		{
			return srMedia;
		}
	}
	return ISRMediaRef();
}

/*
**
*/
PL::ISRMarkersRef SRRoughCut::GetMarkers(BE::IMasterClipRef inMasterClip) const
{	
	ISRMediaRef srMedia = GetSRMedia(inMasterClip);
	//ASL_ASSERT(srMedia != NULL);
	if (srMedia != NULL && ISRMarkerOwnerRef(srMedia))
	{
		return ISRMarkerOwnerRef(srMedia)->GetMarkers();
	}
	return PL::ISRMarkersRef();
}
						   
/*
**
*/
SRMarkersList SRRoughCut::GetMarkersList() const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	SRMarkersList markersList;

	for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
	{
		ISRMediaRef srMedia = (*f)->GetSRMedia();
		
		if (srMedia != NULL)
		{
			markersList.push_back(ISRMarkerOwnerRef(srMedia)->GetMarkers());
		}
	}
	return markersList;

}

/*
**
*/
bool SRRoughCut::IsSupportMarkerEditing() const
{
	return false;
}

/*
**
*/
bool SRRoughCut::IsSupportTimeLineEditing() const
{
	return true;
}

/*
**
*/
bool SRRoughCut::IsDirtyChild(BE::IMasterClipRef inMasterClip) const
{
	PL::SRClipItems clipItems = GetClipItems();
	PL::SRClipItems::iterator iter = clipItems.begin();
	PL::SRClipItems::iterator end = clipItems.end();
	for (; iter != end; ++iter)
	{
		if ((*iter)->GetMasterClip() == inMasterClip)
		{
			return (*iter)->GetSRMedia()->IsDirty();
		}
	}
	return false;
}

/*
**
*/
bool SRRoughCut::AppendClipItems(SRClipItems& ioClipItems)
{
	dvamediatypes::TickTime changedDuration;
	return AddClipItems(ioClipItems, dvamediatypes::kTime_Invalid, &changedDuration);
}

/*
**
*/
void SRRoughCut::RefreshRoughCut(PL::AssetItemPtr const& inRCAssetItem)
{
	bool isAttached = mIsAttached;
	if (mIsAttached)
	{
		DetachFromTimeLine();
	}

	Init(inRCAssetItem);

	if (isAttached)
	{
		AttachToTimeLine();
		// should only clear when RC is front
		MZ::ClearUndoStack(MZ::GetProject());
	}
}

void SRRoughCut::ReverseInsertClipItem(SRClipItems::iterator& ioPosition, const SRClipItems::value_type& inClipItem)
{
	// private, no lock is needed.
	ioPosition = mClipItems.insert(ioPosition, inClipItem);
}

/*
**
*/
bool SRRoughCut::AddClipItems(SRClipItems& ioClipItems, dvamediatypes::TickTime inInsertTime, dvamediatypes::TickTime* outChangedDuration)
{
	if (ioClipItems.empty())
	{
		return false;
	}
	bool isSuccess = true;
	
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	// when both mClipItems and mTrashItems are empty, only empty sequence
	// possible to be created already, when ResolveSequence already executed,
	// the real sequence have been created, thus mClipItems and mTrashItems 
	// is not possible both empty.
	if (mClipItems.size() == 0 && mTrashItems.size() == 0)
	{
		SilkRoadPrivateCreator::ResolveSequence(mSequenceItem, ioClipItems.front()->GetSRMedia()->GetMasterClip(), MZ::kSequenceAudioTrackRule_RoughCut);
	}

	// Try to stop the sequence player when clips are dragged and dropped or appended to a RC.
	StopPlayer();

	// fix bug 3752602
	// Attention: we will insert into sequence in reverse order, so we can insert all new items into the same pos
	// Refer to PL::SequenceActions::AddClipItemsToSequence
	SRClipItems::iterator pos;
	if (inInsertTime == dvamediatypes::kTime_Invalid)
		pos = mClipItems.end();
	else
	{
		// find the position to insert
		dvamediatypes::TickTime offset = dvamediatypes::kTime_Zero;
		SRClipItems::iterator it = mClipItems.begin();
		for (; it != mClipItems.end(); it++)
		{
			offset += (*it)->GetDuration();
			if (offset > inInsertTime)
			{
				break;
			}
		}
		pos = it;
	}
	
	boost::function<void(SRClipItemPtr)> AddToRCClipItemsFxn(boost::bind(&SRRoughCut::ReverseInsertClipItem, this, pos, _1));
	isSuccess = SilkRoadPrivateCreator::AddClipItemsToSequence(
					mSequenceItem,
					ioClipItems,
					inInsertTime,
					AddToRCClipItemsFxn,
					MZ::ActionUndoableFlags::kUndoable,
					dvacore::ZString("$$$/Prelude/MZ/AddRCSubClipAction=Add clip(s) to Rough Cut"),
					outChangedDuration,
                    false);

	// [TODO] Need calculate the index
	if (!ioClipItems.empty())
	{
		SetDirty(true);
		SilkRoadPrivateCreator::Refresh(mSequenceItem);

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadRoughCutStructureChanged(MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem)),
			true);
	}
	return isSuccess;
}

/*
**
*/
ASL::PathnameList SRRoughCut::GetReferencedMediaPath()
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	ASL::PathnameList mediaPathList;
	std::transform(
		mClipItems.begin(),
		mClipItems.end(),
		std::back_inserter(mediaPathList),
		boost::bind(
				&SRClipItem::GetOriginalClipPath,
				_1));

	std::transform(
		mTrashItems.begin(),
		mTrashItems.end(),
		std::back_inserter(mediaPathList),
		boost::bind(
		&SRClipItem::GetOriginalClipPath,
		_1));

	mediaPathList.push_back(GetFilePath());

	return mediaPathList;
}

/*
**
*/
bool SRRoughCut::RemoveSelectedTrackItem(bool inDoRipple, bool inAlignToVideo)
{
	StopPlayer();

	BE::ISequenceRef sequenceRef = MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem);
	DVA_ASSERT(sequenceRef);
	if ( sequenceRef == NULL )
	{
		return false;
	}

	BE::ITrackItemSelectionRef selection(sequenceRef->GetCurrentSelection());

	// [ToDo] Update Clip Items;
	MarkerSet deletingMarkers;
	if ( selection != NULL )
	{
		BE::TrackItemVector tracktems;
		BE::ITrackItemGroupRef trackGroup(selection);
		ASL_ASSERT(trackGroup != NULL);
		trackGroup->GetItems(BE::kTrackItemType_Clip, tracktems);

		if (tracktems.size() > 0)
		{
			BE::TrackItemVector::iterator itemEnd = tracktems.end();
			BE::TrackItemVector::iterator it = tracktems.begin();
			for(; it != itemEnd; ++it)
			{
				GetDeletingMarkers(MZ::BEUtilities::GetMasterClipFromTrackItem(*it), deletingMarkers);
				RemoveClipItem(*it);
			}
		}
		else
		{
			trackGroup->GetItems(BE::kTrackItemType_Transition, tracktems);
			if (tracktems.size() > 0)
			{
				inDoRipple = false;//now use false to delete transition trackItem.
			}
		}
	}

	MZ::Sequence sequence(sequenceRef);
	sequence.ClearItems(inDoRipple, inAlignToVideo);

	MarkerSet removedMarkers;
	if ( !deletingMarkers.empty() )
	{
		// Scan the RoughCut to see if the deleting markers still exist in other track items
		MarkerSet::const_iterator end = deletingMarkers.end();
		MarkerSet::const_iterator it = deletingMarkers.begin();
		for (; it != end; ++it)
		{
			if ( !FindMarker(*it) )
			{
				removedMarkers.insert(*it);
			}
		}

		RemoveMarkersFromSelection(removedMarkers);
	}
	
	ASL::StationUtils::PostMessageToUIThread(
		MZ::kStation_PreludeProject, 
		SilkRoadRoughCutStructureChanged(MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem)),
		true);

	return true;
}

bool SRRoughCut::FindMarker(const CottonwoodMarker& inMarker) const
{
	const SRMarkersList& markerList = GetMarkersList();
	SRMarkersList::const_iterator markerEnd = markerList.end();
	SRMarkersList::const_iterator markerItr = markerList.begin();
	for (; markerItr != markerEnd; ++markerItr)
	{
		if ( (*markerItr)->HasMarker(inMarker.GetGUID()) )
		{
			return true;
		}
	}
	return false;
}

void SRRoughCut::GetDeletingMarkers(const BE::IMasterClipRef inMasterClip, MarkerSet& outMarkers) const
{
	if ( inMasterClip == NULL )
	{
		return;
	}

    PL::MarkerSet markerSelection = SRMarkerSelection::GetMarkerSelection();
	MarkerSet::const_iterator end = markerSelection.end();
	MarkerSet::const_iterator it = markerSelection.begin();

	for (; it != end; ++it)
	{
		const CottonwoodMarker& marker = *it;
		if ( marker.GetMarkerOwner()->GetMarkers()->GetMediaMasterClip() == inMasterClip )
		{
			outMarkers.insert(*it);
		}
	}
}

void SRRoughCut::StopPlayer()
{
	// Stop the sequence player when clips are dragged and dropped or appended to a RC.
	// and delete items from sequence needs stop player as well.
	ML::IPlayerRef player;
	BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(mSequenceItem);
	MZ::GetSequencePlayer(sequence, MZ::GetProject(), ML::kPlaybackPreference_AudioVideo, player);
	if ( NULL != player )
	{
		player->StopPlayback();
	}
}

/*
**
*/
ASL::Color SRRoughCut::GetTrackItemLabelColor() const
{
	return ASL::Color(176, 176, 176);
}

/*
**
*/
bool SRRoughCut::RemoveClipItem(const BE::ITrackItemRef inTrackItem)
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	SRClipItems::iterator iter = mClipItems.begin();
	SRClipItems::iterator end = mClipItems.end();

	for(; iter != end; ++iter)
	{
		if ((*iter)->GetTrackItem() == inTrackItem)
		{
			break;
		}
	}
	
	if (iter != end)
	{
		SetDirty(true);
		mTrashItems.push_back(*iter);
		mClipItems.erase(iter);
		return true;
	}

	return false;
}

/*
**
*/
bool SRRoughCut::RelinkClipItem(SRClipItems& ioRelinkedClipItems)
{
	bool isSuccess = true;
	// Try to stop the sequence player when clips are dragged and dropped or appended to a RC.
	StopPlayer();

	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	SilkRoadPrivateCreator::RelinkClipItemPairVec relinkClipItemsPairVec;
	BOOST_FOREACH(SRClipItemPtr const& relinkedClipItem, ioRelinkedClipItems)
	{
		relinkedClipItem->GetSRMedia()->RefreshOfflineMedia();
		SilkRoadPrivateCreator::RelinkClipItemPair relinkClipItemPair;
		relinkClipItemPair.second = relinkedClipItem;
		BOOST_FOREACH(SRClipItemPtr const& clipItem, mClipItems)
		{
			if (clipItem->GetAssetItem()->GetAssetGUID() == relinkedClipItem->GetAssetItem()->GetAssetGUID())
			{
				relinkClipItemPair.first = clipItem; 
			}
		}
		if (relinkClipItemPair.first)
		{
			relinkClipItemsPairVec.push_back(relinkClipItemPair);
		}
	}

	void (SRClipItems::*ptr)(const SRClipItems::value_type&) = &SRClipItems::push_back;
	boost::function<void(SRClipItemPtr)> AddToRCClipItemsFxn(boost::bind(ptr, &mClipItems, _1));
	SilkRoadPrivateCreator::RelinkClipItems(mSequenceItem, relinkClipItemsPairVec, AddToRCClipItemsFxn);

	return isSuccess;
}

/*
**
*/
ASL::String SRRoughCut::GetSubClipName(const BE::ITrackItemRef inTrackItem) const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
	{
		if ((*f)->GetTrackItem() == inTrackItem)
		{
			return (*f)->GetSubClipName();
		}
	}
	return ASL::String(DVA_STR(""));
}

/*
**
*/
SRClipItemPtr SRRoughCut::GetSRClipItem(const BE::ITrackItemRef inTrackItem) const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	if ( NULL != inTrackItem )
	{
		for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
		{
			if ((*f)->GetTrackItem() == inTrackItem)
			{
				return (*f);
			}
		}
	}

	return SRClipItemPtr();
}

/*
**
*/
ASL::Guid SRRoughCut::GetSubClipGUID(const BE::ITrackItemRef inTrackItem) const
{
	ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

	for (SRClipItems::const_iterator f = mClipItems.begin(), l = mClipItems.end(); f != l; ++f)
	{
		if ((*f)->GetTrackItem() == inTrackItem)
		{
			return (*f)->GetSubClipGUID();
		}
	}
	return ASL::Guid();
}

/*
**
*/
bool SRRoughCut::Publish()
{
	PL::AssetMediaInfoPtr rcMediaInfo = GetAssetMediaInfo();
	BE::ISequenceRef sequence = GetBESequence();
	DVA_ASSERT(rcMediaInfo != NULL && sequence != NULL);
	if (rcMediaInfo == NULL || sequence == NULL)
	{
		return false;
	}

	ASL::String roughcutContent = BuildRoughCutContent(
		sequence,
		GetClipItems(),
		GetName());

	ASL::String filePath = rcMediaInfo->GetMediaPath();

	// Publish rough cut silent
	return ASL::ResultSucceeded(SaveRoughCut(filePath, roughcutContent));
}

void SRRoughCut::DiscardUnsavedChanges()
{
 	// revert unsaved changes by undo these actions
	BE::IExecutorRef executor(MZ::GetProject());
	ASL::UInt32 currentUndoableActionCount = executor->GetUndoableActionCount();
	if (currentUndoableActionCount < mUndoableActionCountAfterAutoSave)
	{
		executor->Redo(mUndoableActionCountAfterAutoSave - currentUndoableActionCount);
	}
	else
	{
		executor->Undo(currentUndoableActionCount - mUndoableActionCountAfterAutoSave);
	}
	// if discard save, we clear mUserHasCanceledSaveFailure so that next time save failure will bring up dialog.
	mUserHasCanceledSaveFailure = false;
	SetDirty(false);
}

bool SRRoughCut::IsWritable() const
{
	return mIsWritable;
}

bool SRRoughCut::IsOffline() const
{
	AssetItemPtr assetItem = GetAssetItem();

	if (assetItem)
	{
		return !ASL::PathUtils::ExistsOnDisk(assetItem->GetMediaPath());
	}
	return true;
}

void SRRoughCut::UpdateWritableState(bool inIsWritable)
{
	// Right now only support change from writable to read-only
	DVA_ASSERT_MSG(mIsWritable == inIsWritable || !inIsWritable, "Change writable state from read-only to writable?");
	if (mIsWritable != inIsWritable)
	{
		mIsWritable = inIsWritable;
		ASL::StationUtils::PostMessageToUIThread(MZ::kStation_PreludeProject, PL::CurrentModuleDataSavableStateChanged(mIsWritable));
	}
}

void SRRoughCut::OnUndoStackCleared()
{
	BE::IExecutorRef executor(MZ::GetProject());
	mUndoableActionCountAfterAutoSave = executor->GetUndoableActionCount();
}

} // namespace PL
