/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*												                   */
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

#include "PLModulePicker.h"

#include "PLMessage.h"
#include "MZActivation.h"
#include "PLUtilities.h"
#include "MZProject.h"
#include "MZUndo.h"
#include "MZUtilities.h"

//	SR
#include "PLFactory.h"
#include "PLProject.h"
#include "PLUtilitiesPrivate.h"
#include "IPLMarkerSelection.h"
#include "IPLLoggingClip.h"

// ML
#include "MLPlayer.h"

//	UIF
#include "UIFWorkSpace.h"
#include "UIFStations.h"
#include "UIFPlayerControl.h"
#include "UIFDocumentManager.h"

//	DVA
#include "dvaworkspace/workspace/WorkspacesManager.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/config/Localizer.h"
#include "dvaworkspace/workspace/TabPanel.h"
#include "dvaworkspace/workspace/Workspace.h"

//	BE
#include "BEBackend.h"
#include "BEProperties.h"

//	ASL
#include "ASLStringCompare.h"
#include "ASLStationUtils.h"
#include "ASLPathUtils.h"
#include "ASLFile.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"

namespace PL
{

	/*
	**
	*/
	ModulePicker::SharedPtr ModulePicker::sModulePicker;

	ASL_MESSAGE_MAP_DEFINE(ModulePicker)
		ASL_MESSAGE_HANDLER(MZ::UndoStackChangedMessage, OnUndoStackChanged);
		ASL_MESSAGE_HANDLER(MZ::UndoStackClearedMessage, OnUndoStackCleared);
		ASL_MESSAGE_HANDLER(PL::MediaMetaDataMarkerNotInSync, OnMediaMetaDataMarkerNotInSync);
	ASL_MESSAGE_MAP_END

	void ModulePicker::OnUndoStackCleared()
	{
		PL::ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = GetCurrentModuleData();
		if ( activePrimaryClipPlayback )
		{
			PL::ISRPrimaryClipRef primaryClip = PL::ISRPrimaryClipRef(activePrimaryClipPlayback);
			if (primaryClip)
				mIsModuleOriginalDirty = primaryClip->IsDirty();
		}
	}

	/*
	**
	*/
	void ModulePicker::OnUndoStackChanged()
	{
		if (MZ::UndoStackIsEmpty(MZ::GetProject()) && !IsAutoSaveEnabled())
		{
			PL::ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = GetCurrentModuleData();
			if ( activePrimaryClipPlayback )
			{
				PL::ISRPrimaryClipRef primaryClip = PL::ISRPrimaryClipRef(activePrimaryClipPlayback);
				if (primaryClip && primaryClip->IsDirty())
				{
					primaryClip->SetDirty(mIsModuleOriginalDirty);
				}
			}
		}
	}

	/*
	**
	*/
	void ModulePicker::OnMediaMetaDataMarkerNotInSync(ASL::String const& inMediaLocator)
	{
		ISRPrimaryClipRef currentPrimaryClip = GetPrimaryClip(mCurrentModuleID);
		if (currentPrimaryClip)
		{
			if (SRProject::GetInstance()->GetAssetMediaInfo(currentPrimaryClip->GetAssetItem()->GetMediaPath())->GetMediaPath() == inMediaLocator)
			{
				MZ::ClearUndoStack(MZ::GetProject());
			}
		}
	}

	/*
	**
	*/
	void ModulePicker::Initialize()
	{
		if ( !sModulePicker)
		{
			sModulePicker.reset(new ModulePicker());
		}

	}

	/*
	**
	*/
	void ModulePicker::Terminate()
	{
		if ( sModulePicker )
		{
			sModulePicker.reset();
		}
	}

	/*
	**
	*/
	ModulePicker::SharedPtr ModulePicker::GetInstance()
	{
		return sModulePicker;
	}

	/*
	**
	*/
	ModulePicker::ModulePicker() 
		:
	mCurrentModuleID(SRUnknownModule),
	mIsModuleOriginalDirty(false)
	{
		ASL::StationUtils::AddListener(MZ::kStation_Undo, this);
	}

	ModulePicker::~ModulePicker()
	{
		ASL::StationUtils::RemoveListener(MZ::kStation_Undo, this);
	}


	void ModulePicker::ActivateAsset(PL::AssetItemPtr const& inAssetItem)
	{
		ISRLoggingClipRef logginClip = (ISRLoggingClipRef)GetPrimaryClip(GetCurrentEditingModule());

		dvamediatypes::TickTime start = dvamediatypes::kTime_Invalid;
		dvamediatypes::TickTime duration = dvamediatypes::kTime_Invalid;

		ISRPrimaryClipPlaybackRef playbackClip = (ISRPrimaryClipPlaybackRef)logginClip;

		if ( inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_SubClip )
		{
			ISRMarkerSelectionRef markerSelectionRef(GetCurrentModuleData());
			if ( markerSelectionRef && logginClip && logginClip->GetSRClipItem() )
			{
				PL::CottonwoodMarker selectedMarker;
                selectedMarker.SetGUID(inAssetItem->GetAssetMarkerGuid());
				markerSelectionRef->ClearSelection();
				markerSelectionRef->AddMarkerToSelection(selectedMarker);
			}

			start = inAssetItem->GetInPoint();
			duration = inAssetItem->GetDuration();
		}
		else if ( logginClip && inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_MasterClip )
		{
			start = logginClip->GetStartTime();
			duration = logginClip->GetDuration();
		}
		else if ( inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_RCSubClip )
		{
			start = inAssetItem->GetInPoint();
			duration = inAssetItem->GetDuration();
		}

		// Only move the CTI position for RCSubclip and Subclip 
		// If a master clip is double clicked or the module switch button is clicked, CTI should be unchanged.
		if ( inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_RCSubClip || 
			 inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_SubClip)
		{
			ML::IPlayerRef player;
			MZ::GetSequencePlayer(
				playbackClip->GetBESequence(), 
				MZ::GetProject(), 
				ML::kPlaybackPreference_AudioVideo, 
				player);

			if (player)
			{
				player->SetPosition(start);
			}
		}

		ASL::StationUtils::BroadcastMessage(
			MZ::kStation_PreludeProject, 
			ZoomToActiveRange(start, duration));
	}

	/*
	**
	*/
	bool ModulePicker::IsAssetItemCurrentOpened(ASL::String const& inOldMediaPath, PL::AssetItemPtr const& inAssetItem)
	{
		bool result(false);
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
		ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();

		for (; iter != end; ++iter)
		{
			if (iter->second->GetMediaPath() == inOldMediaPath)
			{
				result = true;
				break;
			}
		}

		return result;
	}

	/*
	**
	*/
	bool ModulePicker::UpdateOpenedAssetItem(ASL::String const& inOldMediaPath, PL::AssetItemPtr const& inAssetItem)
	{
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
		ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();

		for (; iter != end; ++iter)
		{
			if (iter->second->GetMediaPath() == inOldMediaPath)
			{
				bool isCurrentlyAcive = (mCurrentModuleID == iter->first);
				ISRPrimaryClipRef primaryClip = GetPrimaryClip(iter->first);
				if (primaryClip)
				{
					if (isCurrentlyAcive)
					{
						DetachModuleData(primaryClip);
					}
					primaryClip->Relink(inAssetItem);
					iter->second = inAssetItem;
					if (isCurrentlyAcive)
					{
						MZ::ClearUndoStack(MZ::GetProject());
						primaryClip->CreateSequence();
						AttachModuleData(primaryClip);
						ASL::StationUtils::PostMessageToUIThread(
							MZ::kStation_PreludeProject, 
							PreludeEditModeChanged(),
							true);
					}
				}
				break;
			}
		}

		return true;
	}

	bool ModulePicker::ClearActiveAssetUndoStack(PL::AssetItemPtr const& inAssetItem)
	{
		bool result = false;

		if (IsSameClipPath(mCurrentModuleID, inAssetItem))
		{
			MZ::ClearUndoStack(MZ::GetProject());

			result = true;
		}

		return result;
	}

	/*
	**
	*/
	bool ModulePicker::ActiveModule(PL::AssetItemPtr const& inAssetItem, bool inNeedZoom)
	{
		bool activeSuccess = false;
		SREditingModule moduleID;
		moduleID = GetModuleTypeFromAssetItem(inAssetItem);
		if ( moduleID != SRUnknownModule )
		{
			// [COMMENT] If inAssetItem has same moduleID and same clipPath as old one, 
			// which means they are same roughcut or loggingclip,
			// Open is successful and do not need to ActiveModuleInternal
			if (IsSameModule(moduleID) && IsSameClipPath(moduleID, inAssetItem))
			{		
				activeSuccess = true;

				PL::ISRPrimaryClipPlaybackRef playback = PL::ModulePicker::GetInstance()->GetCurrentModuleData();
				if (playback)
				{
					ASL::StationUtils::BroadcastMessage(
						MZ::kStation_SequenceActivation, 
						MZ::SequenceOpenedMessage(playback->GetBESequence(), true));
				}
				//return true;
			}
			// [COMMENT] If inAssetItem has different clipPath as old one, 
			// which means they are different things,
			// when old assetItem is dirty which needs to be saved and user canceled the save,
			// inAssetItem will not open, means, open failed
			else if (!IsSameClipPath(moduleID, inAssetItem) && IsUserCanceledSave(moduleID))
			{
				activeSuccess = false;
				//return false;
			}
			// [COMMENT] Except the two occasions above, all the other situations need
			// actually open the inAssetItem and return open successful
			else
			{
				ActiveModuleInternal(moduleID, inAssetItem);
				activeSuccess = true;
			}
		}

		if ( activeSuccess && inNeedZoom )
		{
			ActivateAsset(inAssetItem);
		}

		return activeSuccess;
	}

	/*
	**
	*/
	SREditingModule ModulePicker::GetModuleTypeFromAssetItem(PL::AssetItemPtr const& inAssetItem)
	{
		SREditingModule editingModule(SRUnknownModule);
		if (inAssetItem)
		{
			PL::AssetMediaType  assetMediaType = inAssetItem->GetAssetMediaType();
			switch(assetMediaType)
			{
			case kAssetLibraryType_RoughCut:
				editingModule = SRRoughCutModule;
				break;
			case kAssetLibraryType_MasterClip:
			case kAssetLibraryType_SubClip:
				editingModule = SRLoggingModule;
				break;
			default:
				DVA_ASSERT_MSG(false, "inClipInfo can't be interpreted into correct Module");
			}
		}

		return editingModule;
	}

	/*
	**
	*/
	void ModulePicker::ActiveModuleInternal(
		SREditingModule inModuleID, 
		PL::AssetItemPtr const& inAssetItem)
	{
		SREditingModule oldModuleID = mCurrentModuleID;

		//  1. Create new primaryClip by inAssetItem or get it from map
		ISRPrimaryClipRef primaryClip = TakePrimaryClip(inAssetItem);

		if (primaryClip == NULL)
		{
			return;
		}
		primaryClip->CreateSequence();

		//	2. we detach current module data from time line editor.
		DetachModuleData(GetPrimaryClip(oldModuleID));	

		//  3. Register inAssetItem into the Map, and set mCurrentModuleID
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.find(inModuleID);
		if (iter != mModulePickerEntrys.end() && 
			!PL::AssetItem::IsAssetItemSamePath(primaryClip->GetAssetItem(), iter->second))
		{
			SRProject::GetInstance()->RemovePrimaryClip(iter->second->GetMediaPath());
			SendCloseNotification(iter->second);
		}

		// It has to store the asset item in the primary clip because the asset item is 
		// changed when the primary clip was created.
		mModulePickerEntrys[inModuleID] = primaryClip->GetAssetItem();
		mCurrentModuleID = inModuleID;

		//	4. Attach new primaryClip to time line editor.
		AttachModuleData(primaryClip);

		SendOpenNotification(primaryClip->GetAssetItem());

		MZ::ClearUndoStack(MZ::GetProject());

		//	5. Tell the world, we change module data.
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			PreludeEditModeChanged(),
			true);
	}
	
	/*
	**
	*/
	bool ModulePicker::IsSameModule(SREditingModule inModuleID)
	{
		DVA_ASSERT(inModuleID != SRUnknownModule);
		return inModuleID == mCurrentModuleID;
	}


	/*
	**
	*/
	bool ModulePicker::IsSameClipPath(
						SREditingModule inModuleID, 
						PL::AssetItemPtr const& inAssetItem)
	{
		DVA_ASSERT(inModuleID != SRUnknownModule);
		DVA_ASSERT(inAssetItem);

		PL::AssetItemPtr oldAssetItem;
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.find(inModuleID);;
		if (iter != mModulePickerEntrys.end())
		{
			oldAssetItem = iter->second;
		}
		PL::AssetItemPtr newAssetItem = inAssetItem;								

		bool isSameClipPath = AssetItem::IsAssetItemSamePath(newAssetItem, oldAssetItem);

		return isSameClipPath;
	}

	/*
	**
	*/
	bool ModulePicker::IsUserCanceledSave(SREditingModule inModuleID)
	{
		DVA_ASSERT(inModuleID != SRUnknownModule);
		ISRPrimaryClipRef oldModuleData = GetPrimaryClip(inModuleID);

		if (oldModuleData)
		{
			// If different clip, ask user to save it
			UIF::MBResult::Type saveResult = oldModuleData->Save(IsAutoSaveEnabled(), true);

			return saveResult == UIF::MBResult::kMBResultCancel;
		}
		
		return false;
	}

	/*
	**	
	*/
	ISRPrimaryClipRef ModulePicker::TakePrimaryClip(
										PL::AssetItemPtr const& inAssetItem)
	{
		ISRPrimaryClipRef primaryClip;
		ASL::String mediaPath = inAssetItem->GetMediaPath();

		primaryClip = SRProject::GetInstance()->GetPrimaryClip(mediaPath);
		if (NULL == primaryClip)
		{
			primaryClip = SRFactory::CreatePrimaryClip(inAssetItem);
			SRProject::GetInstance()->AddPrimaryClip(primaryClip);
		}
		
		return primaryClip;
	}

	/*
	**
	*/
	ISRPrimaryClipRef ModulePicker::GetPrimaryClip(SREditingModule inModuleID)
	{
		ISRPrimaryClipRef primaryClip;
		ASL::String mediaPath;

		ModulePickerEntryMap::iterator it = mModulePickerEntrys.find(inModuleID);
		if (it != mModulePickerEntrys.end())
		{
			mediaPath = it->second->GetMediaPath();
		}

		return SRProject::GetInstance()->GetPrimaryClip(mediaPath);
	}

	/*
	**
	*/
	void ModulePicker::AttachModuleData(ISRPrimaryClipRef inPrimaryClip)
	{
		if (inPrimaryClip)
		{
			inPrimaryClip->AttachToTimeLine();
		}
	}

	/*
	**
	*/
	void ModulePicker::SendCloseNotification(AssetItemPtr const& inCloseAssetItem)
	{
		if (inCloseAssetItem)
		{
			//	Send Close notification to client
			PL::AssetItemList assetItemList;

			PL::AssetItemPtr masterItem(new AssetItem(
								inCloseAssetItem->GetAssetMediaType(),
								inCloseAssetItem->GetMediaPath(),
								inCloseAssetItem->GetAssetGUID(),
								inCloseAssetItem->GetParentGUID(),
								inCloseAssetItem->GetAssetMediaInfoGUID(),
								inCloseAssetItem->GetAssetName()));

			assetItemList.push_back(masterItem);
			SRProject::GetInstance()->GetAssetLibraryNotifier()->AssetItemClosed(
				ASL::Guid::CreateUnique(),
				ASL::Guid::CreateUnique(),
				assetItemList);


			//	Remove listener from its AssetMediaInfo
			if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(inCloseAssetItem->GetMediaPath()))
			{
				ASL::StationUtils::RemoveListener(
					SRProject::GetInstance()->GetAssetMediaInfoWrapper(inCloseAssetItem->GetMediaPath())->GetStationID(), 
					this);
			}
		}
	}

	void ModulePicker::SendOpenNotification(AssetItemPtr const& inOpenAssetItem)
	{
		if (inOpenAssetItem)
		{
			//	 Send Open Notification to client
			PL::AssetItemList assetItemList;
			PL::AssetItemPtr masterItem(new AssetItem(
				inOpenAssetItem->GetAssetMediaType(),
				inOpenAssetItem->GetMediaPath(),
				inOpenAssetItem->GetAssetGUID(),
				inOpenAssetItem->GetParentGUID(),
				inOpenAssetItem->GetAssetMediaInfoGUID(),
				inOpenAssetItem->GetAssetName()));

			assetItemList.push_back(masterItem);
			SRProject::GetInstance()->GetAssetLibraryNotifier()->AssetItemOpened(
				ASL::Guid::CreateUnique(),
				ASL::Guid::CreateUnique(),
				assetItemList);

			//	 Add Listener to its AssetMediainfo
			if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(inOpenAssetItem->GetMediaPath()))
			{
				ASL::StationUtils::AddListener(
					SRProject::GetInstance()->GetAssetMediaInfoWrapper(inOpenAssetItem->GetMediaPath())->GetStationID(), 
					this);
			}
		}
	}


	/*
	**
	*/
	void ModulePicker::DetachModuleData(ISRPrimaryClipRef inPrimaryClip)
	{
		UIF::PlayerControl::StopAllPlayers();

		if (inPrimaryClip)
		{
			inPrimaryClip->DetachFromTimeLine();
		}
	}

	/*
	**	Get edited data from workspace ID
	*/
	ISRPrimaryClipPlaybackRef ModulePicker::GetModuleData(SREditingModule inModuleID)
	{
		return ISRPrimaryClipPlaybackRef(GetPrimaryClip(inModuleID));
	}

	/*
	** Return Logging Clip
	*/
	ISRPrimaryClipPlaybackRef ModulePicker::GetLoggingClip()
	{
		return GetModuleData(SRLoggingModule);
	}

	/*
	** Return Rough Cut Clip
	*/
	ISRPrimaryClipPlaybackRef ModulePicker::GetRoughCutClip()
	{
		return GetModuleData(SRRoughCutModule);
	}

	/*
	**	Get current edited data
	*/
	ISRPrimaryClipPlaybackRef ModulePicker::GetCurrentModuleData()
	{
		return GetModuleData(mCurrentModuleID);
	}

	/*
	**	Save Modules
	*/
	bool ModulePicker::CloseActiveModule()
	{
		PL::ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = GetCurrentModuleData();

		if ( activePrimaryClipPlayback )
		{
			PL::ISRPrimaryClipRef primaryClip = PL::ISRPrimaryClipRef(activePrimaryClipPlayback);
			if (primaryClip)
			{
				if ( primaryClip->IsDirty() )
				{
					UIF::MBResult::Type saveResult = primaryClip->Save(IsAutoSaveEnabled(), true);
					if (saveResult == UIF::MBResult::kMBResultCancel)
					{
						return false;
					}
				}

				RemovePrimaryClipInternal(primaryClip);

				//	Notify client close action
				SendCloseNotification(primaryClip->GetAssetItem());

				// After close module, should clear undo stack.
				MZ::ClearUndoStack(MZ::GetProject());

				//	Send ModeChange Msg
				ASL::StationUtils::PostMessageToUIThread(
					MZ::kStation_PreludeProject, 
					PreludeEditModeChanged(),
					true);
			}
		}

		return true;
	}

	/*
	** Save all Modules
	*/
	UIF::MBResult::Type ModulePicker::SaveAllModuleData(bool inIsSilent)
	{
		UIF::MBResult::Type saveResult = UIF::MBResult::kMBResultYes;
		if (!ContainsDirtyData())
			return saveResult;

		bool isClearUndoStack(true);
		if (!inIsSilent)
		{
			//	Ask if current editing dirty clip is ok for quit.
			ASL::String warnUserCaption = dvacore::ZString("$$$/Prelude/Mezzanine/SaveDirtyDataCaption=Adobe Prelude");
			ASL::String warnUserMessage = dvacore::ZString("$$$/Prelude/Mezzanine/SaveDirtyDataMessage=Save all changes?");
			saveResult = UIF::MessageBox(
				warnUserMessage,
				warnUserCaption,
				UIF::MBFlag::kMBFlagYesNoCancel,
				UIF::MBIcon::kMBIconWarning);
		}

		if (saveResult == UIF::MBResult::kMBResultYes)
		{
			ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
			ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();

			ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = GetCurrentModuleData();
			ISRPrimaryClipRef activePrimaryClip = ISRPrimaryClipRef(activePrimaryClipPlayback);
			for (; iter != end; ++iter)
			{
				ASL::String mediaPath = iter->second->GetMediaPath();

				PL::ISRPrimaryClipRef primaryClip  = 
					SRProject::GetInstance()->GetPrimaryClip(iter->second->GetMediaPath());

				if ( primaryClip && primaryClip->IsDirty() )
				{
					saveResult = primaryClip->Save(true, true);
					if ((activePrimaryClip == primaryClip) && (saveResult == UIF::MBResult::kMBResultCancel))
					{
						isClearUndoStack = false;
					}
				}
			}
		}
		else if (saveResult == UIF::MBResult::kMBResultNo) // fix bug #3283372, if user choose Not to save, we should still set dirty status to false
		{
			ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
			ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();

			for (; iter != end; ++iter)
			{
				ASL::String mediaPath = iter->second->GetMediaPath();

				PL::ISRPrimaryClipRef primaryClip  = 
					SRProject::GetInstance()->GetPrimaryClip(iter->second->GetMediaPath());
				if ( primaryClip && primaryClip->IsDirty() )
				{
					primaryClip->SetDirty(false);
				}
			}
		}

		// if Save operation of PrimaryClip failed, the result will also be kMBResultCancel
		if (isClearUndoStack)
		{
			// After close module, should clear undo stack.
			MZ::ClearUndoStack(MZ::GetProject());
		}

		return saveResult;
	}

	/*
	**	
	*/
	UIF::MBResult::Type ModulePicker::SaveModuleXMPDataAs()
	{
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.find(SRLoggingModule);
		if (iter == mModulePickerEntrys.end())
			return UIF::MBResult::kMBResultCancel;

		XMPText xmpStr;
		PL::ISRPrimaryClipPlaybackRef primaryClipPlayBack  = GetModuleData(SRLoggingModule);
		if (primaryClipPlayBack)
			xmpStr = primaryClipPlayBack->GetMarkersList().front()->BuildXMPStringFromMarkers();

		PL::ISRLoggingClipRef loggingClip(primaryClipPlayBack);
		if (loggingClip)
		{
			xmpStr = Utilities::GetXMPTextWithAltTimecodeField(xmpStr, loggingClip->GetSRClipItem()->GetMasterClip());
		}

		const ASL::String& extension = DVA_STR(".xmp");
		ASL::String fileFilter = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/SaveXMPAsFilterStr=XMP File (*.xmp)");
		fileFilter.push_back('\0');
		fileFilter.append(DVA_STR("*.xmp"));
		fileFilter.push_back('\0');

		ASL::String const& caption = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/SaveXMPAs=Save File");
		ASL::String filePath = iter->second->GetMediaPath();
		if (MZ::Utilities::IsEAMediaPath(filePath))
		{
			ASL::String preludeDocuments = ASL::MakeString(ASL::kUserDocumentsDirectory);
			filePath = ASL::DirectoryRegistry::FindDirectory(preludeDocuments);

			if (!ASL::Directory::IsDirectory(filePath))
			{
				ASL::Directory::CreateOnDisk(filePath);
			}

		}
		else
		{
			filePath = ASL::PathUtils::GetDrivePart(filePath) + ASL::PathUtils::GetDirectoryPart(filePath) + ASL::PathUtils::GetFilePart(filePath) + DVA_STR("_Separate") + extension;
		}

		if (!SRUtilitiesPrivate::AskSaveFilePath(caption, filePath, extension, fileFilter))
			return UIF::MBResult::kMBResultCancel;

		dvacore::UTF16String errorCaption = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/WriteXMPFileErrorCaption=Save Failed");
		if (ASL::PathUtils::GetFreeDriveSpace(filePath) < xmpStr->length())
		{
			dvacore::UTF16String errorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/DiskSpaceNotEnough=Not enough available disk space.");
			SRUtilitiesPrivate::PromptForError(errorCaption, errorMsg);
			return UIF::MBResult::kMBResultAbort;
		}

		bool result(false);
		ASL::File xmpFile;
		if(ASL::ResultSucceeded(xmpFile.Create(
				filePath,
				ASL::FileAccessFlags::kWrite,
				ASL::FileShareModeFlags::kNone,
				ASL::FileCreateDispositionFlags::kCreateAlways,
				ASL::FileAttributesFlags::kAttributeNormal)))
		{
			ASL::UInt32 numberOfBytesWritten;
			result = ASL::ResultSucceeded(xmpFile.Write(xmpStr->c_str(), static_cast<ASL::UInt32>(xmpStr->size()), numberOfBytesWritten));
		}

		if (!result)
		{
			dvacore::UTF16String errorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/WriteXMPFileError=Save XMP file \"@0\" error!");
			errorMsg = dvacore::utility::ReplaceInString(errorMsg, filePath);
			SRUtilitiesPrivate::PromptForError(errorCaption, errorMsg);
		}
		else
		{
			//	[COMMENT] Save as shouldn't clear primaryClip's dirty state.
			//	That is also to say user have to choose 'no' when they do actions like "close" "quit" "switch primaryClip"
			//  And off-line dirty clip can never be set as unDirty.

			//GetPrimaryClip(SRLoggingModule)->SetDirty(false);
			//if (mCurrentModuleID == SRLoggingModule)
			//{
			//	ClearUndoStack(MZ::GetProject());
			//}
		}

		return result ? UIF::MBResult::kMBResultOK : UIF::MBResult::kMBResultCancel;
	}

	/*
	**
	*/
	SREditingModule ModulePicker::GetCurrentEditingModule()
	{
		return mCurrentModuleID;
	}

	/*
	**	
	*/
	UIF::MBResult::Type ModulePicker::SaveModuleRoughCutDataAs()
	{
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.find(SRRoughCutModule);
		if (iter == mModulePickerEntrys.end())
			return UIF::MBResult::kMBResultCancel;

		PL::ISRPrimaryClipPlaybackRef primaryClipPlayBack  = GetModuleData(SRRoughCutModule);
		PL::ISRRoughCutRef roughCut = ISRRoughCutRef(primaryClipPlayBack);
		if (roughCut == NULL)
			return UIF::MBResult::kMBResultCancel;

		ASL::String filePath = roughCut->GetFilePath();
		const ASL::String& extension = DVA_STR(".arcut");
		ASL::String fileFilter = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/SaveRoughCutAsFilterStr=Rough Cut File (*.arcut)");
		fileFilter.push_back('\0');
		fileFilter.append(DVA_STR("*.arcut"));
		fileFilter.push_back('\0');

		ASL::String const& caption = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/SaveRoughCutAs=Save File");
		if (!SRUtilitiesPrivate::AskSaveFilePath(caption, filePath, extension, fileFilter))
			return UIF::MBResult::kMBResultCancel;

		ASL::String fileContent = PL::BuildRoughCutContent(
			primaryClipPlayBack->GetBESequence(),
			roughCut->GetClipItems(),
			ASL::PathUtils::GetFilePart(filePath));

		bool result = ASL::ResultSucceeded(PL::SaveRoughCut(filePath, fileContent));
		if (!result) 
		{
			ASL::String errStr = dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/SaveRoughCutAsErrorMsg=Project save rough cut as @0 to @1 failed",
												  ASL::PathUtils::GetFullFilePart(roughCut->GetFilePath()),
												  filePath);
			ML::SDKErrors::SetSDKErrorString(errStr);
		}
		return result ? UIF::MBResult::kMBResultOK : UIF::MBResult::kMBResultCancel;

		//	[COMMENT] Save as shouldn't clear primaryClip's dirty state.
		//	That is also to say user have to choose 'no' when they do actions like "close" "quit" "switch primaryClip"
		//  And off-line dirty clip can never be set as unDirty.

		//if (result)
		//{
		//	GetPrimaryClip(SRRoughCutModule)->SetDirty(false);
		//	if (mCurrentModuleID == SRRoughCutModule)
		//	{
		//		ClearUndoStack(MZ::GetProject());
		//	}
		//	return UIF::MBResult::kMBResultOK;
		//}
        //
		//return UIF::MBResult::kMBResultCancel;
	}

	/*
	**	Any data is dirty?
	*/
	bool ModulePicker::ContainsDirtyData() const
	{
		ModulePickerEntryMap::const_iterator iter = mModulePickerEntrys.begin();
		ModulePickerEntryMap::const_iterator end = mModulePickerEntrys.end();

		bool dirty = false;
		for (; iter != end; ++iter)
		{
			PL::ISRPrimaryClipRef primaryClip  = 
				SRProject::GetInstance()->GetPrimaryClip(iter->second->GetMediaPath());
			if ( primaryClip  && primaryClip->IsDirty() )
			{
				dirty = true;
				break;
			}
		}
		return dirty;
	}

	/*
	**	Is auto save enabled?
	*/
	bool ModulePicker::IsAutoSaveEnabled()
	{
		// right now PL support auto-save for both local mode and EA mode
		return true;
	}

	/*
	**	Close all Modules
	*/
	void ModulePicker::CloseAllModuleData(bool inSuppressModeChangedMsg)
	{
		// if no current module, it means no modules, and we don't need do anything.
		if (GetCurrentModuleData() == NULL)
		{
			return;
		}

		// We have to lose current focus to save data before closing all module data.
		dvaworkspace::workspace::Workspace* dvaWorkspace = UIF::Workspace::Instance()->GetPtr();
		dvaworkspace::workspace::TabPanel* focusedPanel = NULL;

		if (dvaWorkspace)
		{
			focusedPanel = dvaWorkspace->GetFocusedTabPanel();
			if (focusedPanel)
			{
				focusedPanel->UI_LoseKeyboardFocus();
			}
		}

		// Remove BE Sequence
		DetachModuleData( GetPrimaryClip(mCurrentModuleID) );

		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
		ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();
		for (; iter != end; ++iter)
		{
			PL::ISRPrimaryClipRef primaryClip = 
				SRProject::GetInstance()->GetPrimaryClip(iter->second->GetMediaPath()); 
			if ( primaryClip )
			{
				SRProject::GetInstance()->RemovePrimaryClip(primaryClip->GetAssetItem()->GetMediaPath());
				SendCloseNotification(primaryClip->GetAssetItem());
			}
		}

		mModulePickerEntrys.clear();
		// After close module, should clear undo stack.
		MZ::ClearUndoStack(MZ::GetProject());

		if ( !inSuppressModeChangedMsg )
		{
			// BroadCast message
			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject, 
				PreludeEditModeChanged(),
				true);
		}

		if (dvaWorkspace && focusedPanel)
		{
			dvaWorkspace->SetFocusedTabPanel(focusedPanel);
		}
	}

	/*
	** Remove primary Clip from ModulePicker by path
	*/
	ISRPrimaryClipRef ModulePicker::CloseModule(const dvacore::UTF16String& inMediaPath)
	{
		ISRPrimaryClipPlaybackRef activePrimaryClipPlayback = GetCurrentModuleData();
		ISRPrimaryClipRef activePrimaryClip = ISRPrimaryClipRef(activePrimaryClipPlayback);

		ISRPrimaryClipRef primaryClip = SRProject::GetInstance()->GetPrimaryClip(inMediaPath);
		if (primaryClip)
		{
			RemovePrimaryClipInternal(primaryClip);
			//	Notify client close action
			SendCloseNotification(primaryClip->GetAssetItem());

			// If close active module, should clear undo stack.
			if (primaryClip == activePrimaryClip)
				MZ::ClearUndoStack(MZ::GetProject());

			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject, 
				PreludeEditModeChanged(),
				true);
		}
		return primaryClip;
	}

	/*
	**
	*/
	void ModulePicker::RemovePrimaryClipInternal(ISRPrimaryClipRef inPrimaryClip)
	{
		if (NULL == inPrimaryClip)
		{
			DVA_ASSERT(false);
			return;
		}

		bool removeActiveClip = (inPrimaryClip == GetPrimaryClip(mCurrentModuleID));
	
		// First, if the clip which we should remove is current active clip, we need detach it.
		if (removeActiveClip)
		{
			DetachModuleData(inPrimaryClip);
		}

		//  Second, we remove this primaryclip from map if it exist.
		ModulePickerEntryMap::iterator iter = mModulePickerEntrys.begin();
		ModulePickerEntryMap::iterator end = mModulePickerEntrys.end();

		for (; iter != end; ++iter)
		{
			PL::ISRPrimaryClipRef primaryClip = 
				SRProject::GetInstance()->GetPrimaryClip(iter->second->GetMediaPath()); 

			if (primaryClip == inPrimaryClip)
			{
				mModulePickerEntrys.erase(iter);
				SRProject::GetInstance()->RemovePrimaryClip(inPrimaryClip->GetAssetItem()->GetMediaPath());
				break;
			}
		}

		//	Last, if we already remove current active clip, and there are primary clip background,
		//	we need attach the first one.
		if (removeActiveClip)
		{
			if (!mModulePickerEntrys.empty())
			{
				mCurrentModuleID = mModulePickerEntrys.begin()->first;
				ISRPrimaryClipRef overlaidPrimaryClip = 
							SRProject::GetInstance()->GetPrimaryClip(mModulePickerEntrys.begin()->second->GetMediaPath());
				AttachModuleData(overlaidPrimaryClip);
			}
			else
			{
				mCurrentModuleID = SRUnknownModule;
			}
		}
	}

	bool CurrentMarkersAreEditable()
	{
		PL::ISRPrimaryClipPlaybackRef primaryClipPlayback = 
			PL::ModulePicker::GetInstance()->GetCurrentModuleData();
		PL::ISRPrimaryClipRef primaryClip(primaryClipPlayback);
		if (primaryClipPlayback != NULL && primaryClip->IsWritable())
		{
			return primaryClipPlayback->IsSupportMarkerEditing();
		}
		return false;
	}

} // namespace PL
