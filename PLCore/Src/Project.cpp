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

//	XMP
#include "IncludeXMP.h"

//	Local
#include "PLProject.h"
#include "PLBEUtilities.h"

//	SR
#include "PLUtilitiesPrivate.h"
#include "PLLoggingClip.h"
#include "PLFactory.h"
#include "IPLRoughCut.h"
#include "IPLAssetLibraryNotifier.h"
#include "PLMedia.h"

//	MZ
#include "MZProject.h"
#include "MZMasterClip.h"
#include "MZUndo.h"
#include "MZInfoStation.h"
#include "MZProjectActions.h"
#include "PLConstants.h"
#include "MZBEUtilities.h"
#include "PLUtilities.h"
#include "PLWriteXMPToDiskCache.h"
#include "PLModulePicker.h"
#include "PLLibrarySupport.h"
#include "PLMediaMonitorCache.h"
#include "MZUtilities.h"
#include "MZMedia.h"
#include "MZProject.h"
#include "PLXMPilotUtils.h"
#include "MLMetadataManager.h"

//	ASL
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLStringCompare.h"
#include "ASLPathUtils.h"
#include "ASLClassFactory.h"
#include "ASLAsyncCallFromMainThread.h"
#include "ASLStations.h"

//	BE
#include "BEExecutor.h"
#include "BEProperties.h"
#include "BEClip.h"
#include "BEMedia.h"
#include "BETrack.h"
#include "BEMasterClip.h"
#include "BEAction.h"
#include "BEActionImpl.h"
#include "BETransaction.h"
#include "BESequence.h"
#include "BESequenceEditor.h"
#include "BEUndoStack.h"
#include "BE/Project/ProjectItemFactory.h"
#include "BE/Project/IProject.h"
#include "BE/Core/INotification.h"

//	ML
#include "MLMedia.h"
#include "ImporterHost.h"

//	DVA
#include "dvacore/config/Localizer.h"

//	EAMedia
#include "IEAMediaInfo.h"

namespace PL
{

namespace
{

static const dvacore::StdString kXMPPartID_Tracks("/metadata/xmpDM/Tracks");
static const dvacore::StdString kXMPPartID_Metadata("/metadata");

/*
**
*/
void WriteMetadataCompletionRoutine(ASL::String inMediaPath, ASL::Result inResult, ASL::SInt32 inRequestID)
{
	BE::RequestIDs requestIDs;
	requestIDs.insert(inRequestID);

	ASL::StationUtils::PostMessageToUIThread(
		MZ::kStation_PreludeProject,
		WriteXMPToDiskFinished(inResult, inMediaPath, requestIDs));
}

/*
**
*/
AssetMediaInfoPtr CreateDummyMasterClipMediaInfo(ASL::String const& inMediaPath)
{
	return	AssetMediaInfo::CreateMasterClipMediaInfo(
								ASL::Guid::CreateUnique(),
								inMediaPath,
								ASL::PathUtils::GetFilePart(inMediaPath),
								XMPText(new ASL::StdString));
}

/*
**
*/
bool VerifyAssetMediaInfo(AssetMediaInfoPtr const& inAssetMediaInfo)
{
	
	bool result(true);
	// inAssetMediaInfo->GetMediaPath() can be empty in EA mode.
	if (inAssetMediaInfo->GetAssetMediaInfoGUID().AsString().empty())
	{
		result = false;
	}
	if (result)
	{
		switch(inAssetMediaInfo->GetAssetMediaType())
		{
		case kAssetLibraryType_MasterClip:
			if (inAssetMediaInfo->GetXMPString()->empty())
			{
				result = false;
			}
			break;
		case kAssetLibraryType_RoughCut:
			break;
		default:
			result = false;
			break;
		}
	}

	return result;
}

}

ASL::CriticalSection SRProject::sCriticalSection;

SRProject::SharedPtr SRProject::sProject;

/*
**
*/
ASL::CriticalSection& SRProject::GetLocker()
{
	return sCriticalSection;
}

/*
**
*/
SRProject::SharedPtr SRProject::GetInstance()
{
	return sProject;
}

/*
**
*/
SRProject::SRProject()
	:
	mAssetLibraryNotifier(NULL),
	mProjectResourceChanged(false)
{
	mUnreferencedResourceRemovalExecutor = dvacore::threads::CreateAsyncThreadedExecutor("SRProject Unreferenced Resource Removal", 1);
}

/*
**
*/
SRProject::~SRProject()
{
	// Reset removal executor
	dvacore::threads::AsyncThreadedExecutorPtr executor;
	{
		ASL::CriticalSectionLock lock(GetLocker());
		mUnreferencedResourceRemovalExecutor.swap(executor);
	}

	if (executor)
	{
		executor->Terminate();
		executor->Flush();
	}
}

/*
**
*/
void SRProject::Initialize()
{
	if ( !sProject )
	{
		sProject.reset(new SRProject());
	}

	ASL::StationRegistry::RegisterStation(kStation_SubclipInOut);
	ASL::StationRegistry::RegisterStation(MZ::kStation_PreludeProject);
	ASL::StationRegistry::RegisterStation(kStation_PreludeStoredMetadata);

	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, sProject.get());
	ASL::StationUtils::AddListener(MZ::CCMService::GetInstance()->GetStation(), sProject.get());

	ImporterHost::SetWriteMetadataCompletionRoutine(&WriteMetadataCompletionRoutine);
    SRMedia::sOrphanItems = BE::OrphanItemFactory::CreateOrphanItem();

	// Listen app suspend and resume message
	ASL::StationUtils::AddListener(ASL::kStation_AppSuspend, sProject.get());
	ASL::StationUtils::AddListener(ASL::kStation_AppResume, sProject.get());
}

/*
**
*/
void SRProject::OnAppSuspended()
{
}

/*
**
*/
void SRProject::OnAppResumed()
{
	//MZ::Cursor::SetWaitCursor();
	RefreshFiles();
	PL::SRMediaMonitor::GetInstance()->Refresh();
	//MZ::Cursor::SetArrowCursor();
}

/*
**
*/
void SRProject::OnRefreshFiles()
{
	RefreshOfflineClips();
}

/*
**
*/
void SRProject::RefreshFiles()
{
	// Refresh SRMedias
	if (!Utilities::NeedEnableMediaCollection())
	{
		SRProject::GetInstance()->RefreshOfflineClips();
	}
}

/*
**
*/
void SRProject::SetProject(BE::IProjectRef project)
{
	if (project)
	{
		ASL::StationUtils::AddListener(BE::INotificationRef(project->GetRootProjectItem())->GetStationID(), sProject.get());
	}
}

/*
**
*/
void SRProject::Terminate()
{
	ASL::CriticalSectionLock lock(SRProject::GetLocker());

	if ( sProject )
	{
		ASL::StationUtils::RemoveListener(ASL::kStation_AppSuspend, sProject.get());
		ASL::StationUtils::RemoveListener(ASL::kStation_AppResume, sProject.get());

		ImporterHost::SetWriteMetadataCompletionRoutine(NULL);

		sProject->RemoveAllSRMedias();
		sProject->UnRegisterAllAssetMediaInfo();
		sProject->mSRPrimaryClipList.clear();
		sProject->mSelectionWatcher.reset();
		sProject->mAssetSelectionManager.reset();

		ASL::StationUtils::RemoveListener(MZ::kStation_PreludeProject, sProject.get());
		ASL::StationUtils::RemoveListener(MZ::CCMService::GetInstance()->GetStation(), sProject.get());

		ASL::StationRegistry::DisposeStation(kStation_SubclipInOut);
		ASL::StationRegistry::DisposeStation(MZ::kStation_PreludeProject);
		ASL::StationRegistry::DisposeStation(kStation_PreludeStoredMetadata);

		sProject.reset();
        
        if (SRMedia::sOrphanItems)
        {
            SRMedia::sOrphanItems.Clear();
        }
	}
}

/*
**
*/
SRAssetSelectionManager::SharedPtr	SRProject::GetAssetSelectionManager()  
{
	ASL::CriticalSectionLock lock(GetLocker());
	if (mAssetSelectionManager == NULL)
	{
		mAssetSelectionManager.reset(new SRAssetSelectionManager);
	}
	return mAssetSelectionManager;
}

/*
**
*/
SRSelectionWatcherPtr SRProject::GetSelectionWatcher() 
{
	ASL::CriticalSectionLock lock(GetLocker());
	if (mSelectionWatcher == NULL)
	{
		mSelectionWatcher.reset(new SRSelectionWatcher());
	}
	return mSelectionWatcher;
}


void SRProject::RemoveSRMedia(SRMediaSet& ioSRMedias, ISRMediaRef inSRMedia)
{
	if (ioSRMedias.find(inSRMedia) != ioSRMedias.end())
	{
		ioSRMedias.erase(inSRMedia);
	}
}
	
/*
**
*/
void SRProject::OnCCMLogout()
{
	SRUnassociatedMetadataList removedList;
	SRUnassociatedMetadataList	tempList = mSRUnassociatedMetadatas;
	BOOST_FOREACH(SRUnassociatedMetadataRef data, tempList)
	{
		if (data->GetIsCreativeCloud())
		{
			RemoveSRUnassociatedMetadata(data);
			removedList.push_back(data);
		}
	}

	if (!removedList.empty())
	{
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			UnassociatedMetadataRemovedMessage(removedList),
			true);
	}
}

/*
**
*/
void SRProject::OnProjectStructureChanged(const BE::ProjectStructureChangedInfo& inChangedInfo)
{
	BOOST_FOREACH (const BE::ItemRemovedInfo& itemRemovedInfo, inChangedInfo.mRemovedItems)
	{
		BE::IMasterClipRef  masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(itemRemovedInfo.mItem);

		BE::IMediaInfoRef mediaInfo = BE::MasterClipUtils::GetMediaInfo(masterClip);
		if (mediaInfo)
		{
			ASL::StationUtils::RemoveListener(mediaInfo->GetStationID(), this);
		}
	}

	BOOST_FOREACH (const BE::ItemAddedInfo& ItemAddedInfo, inChangedInfo.mAddedItems)
	{
		BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(ItemAddedInfo.mItem);
		BE::IMediaInfoRef mediaInfo = BE::MasterClipUtils::GetMediaInfo(masterClip);
		if (mediaInfo)
		{
			ASL::StationUtils::AddListener(mediaInfo->GetStationID(), this);
		}
	}
}

/*
**
*/
bool SRProject::RemoveAllSRMedias()
{
	ASL::CriticalSectionLock lock(GetLocker());

	mProjectResourceChanged = true;

	SRMediaSet	tempSet = mSRMedias;
	SRMediaSet::iterator iter = tempSet.begin();
	SRMediaSet::iterator end = tempSet.end();

	for (; iter != end; ++iter)
	{
		RemoveSRMedia(mSRMedias, *iter);
	}

	return true;
}
	
/*
**
*/
ISRMediaRef SRProject::GetSRMedia(
	const ASL::String& inMasterClipPath) const
{
	return GetSRMedia(mSRMedias, inMasterClipPath);
}

ISRMediaRef SRProject::GetSRMedia(const SRMediaSet& inSRMedias, const ASL::String& inMasterClipPath) const
{
	ASL::CriticalSectionLock lock(GetLocker());

	ASL::String masterClipPath = MZ::Utilities::NormalizePathWithoutUNC(inMasterClipPath);
	for (SRMediaSet::const_iterator itr = inSRMedias.begin(); itr != inSRMedias.end(); ++itr)
	{
		ASL::String cachedPath = MZ::Utilities::NormalizePathWithoutUNC((*itr)->GetClipFilePath());
		if (ASL::CaseInsensitive::StringEquals(masterClipPath, cachedPath))
		{
			return *itr;
		}
	}
		
	return ISRMediaRef();
}

/*
**
*/
ISRMediaRef SRProject::GetSRMediaByInstanceString(
	const ASL::String& inMediaInstanceString) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	for (SRMediaSet::const_iterator itr = mSRMedias.begin(); itr != mSRMedias.end(); ++itr)
	{
		ASL::String cachedString = BE::MasterClipUtils::GetMediaInstanceString((*itr)->GetMasterClip());
		if (ASL::CaseInsensitive::StringEquals(inMediaInstanceString, cachedString))
		{
			return *itr;
		}
	}

	return ISRMediaRef();
}
	
/*
**
*/
SRMediaSet SRProject::GetSRMedias() const
{
	ASL::CriticalSectionLock lock(GetLocker());
	return mSRMedias;
}

/*
**
*/
void SRProject::AddPrimaryClip(
				ISRPrimaryClipRef inPrimaryClip)
{
	ASL::CriticalSectionLock lock(GetLocker());
	if ( inPrimaryClip != NULL )
	{
		mSRPrimaryClipList.push_back(inPrimaryClip);
	}
}

/*
**
*/
ISRPrimaryClipRef SRProject::GetPrimaryClip(const dvacore::UTF16String& inMediaPath) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	if ( !inMediaPath.empty() )
	{
		SRPrimaryClipList::const_iterator iter = mSRPrimaryClipList.begin();
		SRPrimaryClipList::const_iterator end = mSRPrimaryClipList.end();

		for (; iter != end; ++iter)
		{
			// We should get media path from asset item, rather than media info. In EA case, they are not same.
			ASL::String mediaPath = (*iter)->GetAssetItem()->GetMediaPath();
			if (ASL::CaseInsensitive::StringEquals(mediaPath, inMediaPath))
			{
				return *iter;
			}
		}
	}
	return ISRPrimaryClipRef();
}

/*
**
*/
ISRPrimaryClipRef SRProject::RemovePrimaryClip(const dvacore::UTF16String& inMediaPath)
{
	ASL::CriticalSectionLock lock(GetLocker());
	ISRPrimaryClipRef removeClip;
	if (!inMediaPath.empty())
	{
		SRPrimaryClipList::iterator iter = mSRPrimaryClipList.begin();
		SRPrimaryClipList::iterator end = mSRPrimaryClipList.end();

		for (; iter != end; ++iter)
		{
			ASL::String mediaPath = (*iter)->GetAssetItem()->GetMediaPath();
			if (ASL::CaseInsensitive::StringEquals(mediaPath, inMediaPath))
			{
				removeClip = (*iter);
				break;
			}
		}

		if (removeClip)
		{
			mSRPrimaryClipList.remove(removeClip);
			RemoveUnreferenceResources();
		}
	}
	return removeClip;
}

/*
**
*/
void SRProject::AddSRMedia(ISRMediaRef inSRMedia)
{
	ASL::CriticalSectionLock lock(GetLocker());
	mProjectResourceChanged = true;

	mSRMedias.insert(inSRMedia);
}

/*
**
*/
void SRProject::AddUnassociatedMetadata(SRUnassociatedMetadataRef inSRUnassociatedMetadata)
{
	ASL::CriticalSectionLock lock(GetLocker());
	mSRUnassociatedMetadatas.push_back(inSRUnassociatedMetadata);
}


void SRProject::RemoveSRUnassociatedMetadata(SRUnassociatedMetadataRef inSRUnassociatedMetadata)
{
	ASL::CriticalSectionLock lock(GetLocker());
	SRUnassociatedMetadataList::iterator iter = std::find(
		mSRUnassociatedMetadatas.begin(), 
		mSRUnassociatedMetadatas.end(),
		inSRUnassociatedMetadata);
	if (iter != mSRUnassociatedMetadatas.end())
	{
		mSRUnassociatedMetadatas.erase(iter);
	}
}

/*
**
*/
bool SRProject::RemoveAllSRUnassociatedMetadatas()
{
	ASL::CriticalSectionLock lock(GetLocker());
	mSRUnassociatedMetadatas.clear();	
	return true;
}

/*
**	Look up reference by file path
*/
SRUnassociatedMetadataRef SRProject::FindSRUnassociatedMetadata(
	const ASL::String& inMetadataID) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	for (SRUnassociatedMetadataList::const_iterator itr = mSRUnassociatedMetadatas.begin(); itr != mSRUnassociatedMetadatas.end(); ++itr)
	{
		if (ASL::CaseInsensitive::StringEquals((*itr)->GetMetadataID(), inMetadataID))
		{
			return *itr;
		}
	}

	return SRUnassociatedMetadataRef();
}

/*
**
*/
const SRUnassociatedMetadataList& SRProject::GetSRUnassociatedMetadatas() const
{
	ASL::CriticalSectionLock lock(GetLocker());
	return mSRUnassociatedMetadatas;
}

/*
**
*/
void SRProject::RefreshOfflineClips()
{
	ASL::CriticalSectionLock lock(GetLocker());
	BE::IProjectRef project = MZ::GetProject();
	MZ::Project mzproject(project);

	for (SRMediaSet::const_iterator iter = mSRMedias.begin(); iter != mSRMedias.end(); ++iter)
	{
		BE::IMediaRef media;
		ASL::String const& clipPath = (*iter)->GetClipFilePath();

		BE::IMasterClipRef masterClip = (*iter)->GetMasterClip();
		if (masterClip == NULL 
			|| (media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip)) == NULL
			|| !media->IsOffline() )
		{
			continue;
		}

		BE::IActionRef action;

		if ( media && ASL::PathUtils::ExistsOnDisk(clipPath) )
		{
			ASL::Result result = MZ::Media::ReattachMedia(
				project,
				masterClip, 
				media, 
				clipPath, 
				MZ::MediaParams::CreateDefault(), 
				true, 
				action);

			if ( ASL::ResultSucceeded(result) )
			{
				MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(masterClip), action, true);
			}
		}
	}
}

/*
** Only Save action should refresh our cache by force, Other API should always Register by OnlyAddIfNew policy.
**	In EA mode, multiple master clips and sub clips might share same media information. But these clips will have diffrent URL.
**	So we should register media info for every clip URL in EA mode and keep they register with same media info shared ptr.
**	It should be compatible with local mode. Because in local mode, all master clips and sub clips shared same media info will have same asset path.
**	So when we register into a set, they will auto fall into only one item in set.
*/
AssetMediaInfoPtr SRProject::RegisterAssetMediaInfo(
							const ASL::String& inAssetPath,
							const AssetMediaInfoPtr& inAssetMediaInfo,
							bool inForceRegister,
							bool inRelink)
{
	ASL::CriticalSectionLock lock(GetLocker());

	mProjectResourceChanged = true;

	if (inAssetMediaInfo != NULL && VerifyAssetMediaInfo(inAssetMediaInfo))
	{
		ASL::String clipPath = inAssetPath;
		ASL::Guid mediaInfoID = inAssetMediaInfo->GetAssetMediaInfoGUID();

		// We should keep path a unify form to make sure find same path in diffrent form can return corrent result
		clipPath = MZ::Utilities::NormalizePathWithoutUNC(clipPath);

		// Find if the media info has been existed in our cache. Should walk through all items because the same media info might be registered with another asset URL.
		AssetMediaInfoWrapperMap::const_iterator iter = mAssetMediaInfoWrapperMap.find(clipPath);
		AssetMediaInfoWrapperPtr existedMediaInfoWrapper;
		if (iter != mAssetMediaInfoWrapperMap.end())
		{
			existedMediaInfoWrapper = iter->second;
		}
		else
		{
			BOOST_FOREACH (AssetMediaInfoWrapperMap::value_type& pathInfoPair, mAssetMediaInfoWrapperMap)
			{
				AssetMediaInfoWrapperPtr& mediaInfoWrapper = pathInfoPair.second;
				if (mediaInfoWrapper->GetAssetMediaInfo()->GetAssetMediaInfoGUID() == mediaInfoID)
				{
					existedMediaInfoWrapper = mediaInfoWrapper;
					mAssetMediaInfoWrapperMap[clipPath] = existedMediaInfoWrapper;
					break;
				}
			}
			
		}

		if (existedMediaInfoWrapper == NULL)
		{
			existedMediaInfoWrapper = AssetMediaInfoWrapperPtr(new AssetMediaInfoWrapper(inAssetMediaInfo));
			mAssetMediaInfoWrapperMap[clipPath] = existedMediaInfoWrapper;
		}
		else
		{
			// Maybe need to update exist media info wrapper.
			switch(inAssetMediaInfo->GetAssetMediaType())
			{
			case kAssetLibraryType_MasterClip:
				//	We always keep new metadata in our cache, in case client may send us old metadata cause all API is async call
				if (inAssetMediaInfo != existedMediaInfoWrapper->GetAssetMediaInfo())
				{
					if (inForceRegister || Utilities::IsNewerMetaData(*inAssetMediaInfo->GetXMPString().get(), *(existedMediaInfoWrapper->GetAssetMediaInfo()->GetXMPString().get())))
					{
						existedMediaInfoWrapper->SetAssetMediaInfo(inAssetMediaInfo);
						if (inRelink)
						{
							existedMediaInfoWrapper->UpdateXMPLastModTime();
						}
					}
				}
				break;
			case kAssetLibraryType_RoughCut:
				// Right now, rough cut doesn't care media info, so never overwrite.
				break;
			default:
				DVA_ASSERT(0);
				break;
			}
		}
		
		return existedMediaInfoWrapper->GetAssetMediaInfo();
	}
	return AssetMediaInfoPtr();
}

void SRProject::RegisterAssetMediaInfoList(
	const AssetItemList& inAssetItems,
	const AssetMediaInfoList& inMediaInfoList,
	bool inForceRegister,
	bool inRelink)
{
	ASL::CriticalSectionLock lock(GetLocker());

	mProjectResourceChanged = true;

	// Build map first to optimize finding from media info ID to media info.
	typedef std::map<ASL::Guid, AssetMediaInfoPtr> AssetMediaInfoDictionary;
	std::set<ASL::String> registerredMediaPath;
	AssetMediaInfoDictionary mediaInfoDictionary;
	BOOST_FOREACH (const AssetMediaInfoList::value_type& mediaInfo, inMediaInfoList)
	{
		mediaInfoDictionary.insert(AssetMediaInfoDictionary::value_type(mediaInfo->GetAssetMediaInfoGUID(), mediaInfo));
	}

	BOOST_FOREACH (const AssetItemList::value_type& assetItem, inAssetItems)
	{
		if (registerredMediaPath.find(assetItem->GetMediaPath()) == registerredMediaPath.end())
		{
			DVA_ASSERT_MSG(mediaInfoDictionary[assetItem->GetAssetMediaInfoGUID()] != NULL, "Asset item without a matched media info?!");
			RegisterAssetMediaInfo(assetItem->GetMediaPath(), mediaInfoDictionary[assetItem->GetAssetMediaInfoGUID()], inForceRegister, inRelink);
			registerredMediaPath.insert(assetItem->GetMediaPath());

			BOOST_FOREACH(const AssetItemList::value_type& subAssetItem, assetItem->GetSubAssetItemList())
			{
				if (registerredMediaPath.find(subAssetItem->GetMediaPath()) == registerredMediaPath.end())
				{
					DVA_ASSERT_MSG(mediaInfoDictionary[assetItem->GetAssetMediaInfoGUID()] != NULL, "Asset item without a matched media info?!");
					RegisterAssetMediaInfo(subAssetItem->GetMediaPath(), mediaInfoDictionary[subAssetItem->GetAssetMediaInfoGUID()], inForceRegister, inRelink);
					registerredMediaPath.insert(subAssetItem->GetMediaPath());
				}
			}
		}
	}
}

/*
**
*/
void SRProject::UnRegisterAssetMediaInfo(AssetMediaInfoWrapperMap& ioAssetMediaInfoWrapperMap, const ASL::String& inMediaPath)
{
	ASL::String clipPath = MZ::Utilities::NormalizePathWithoutUNC(inMediaPath);
	AssetMediaInfoWrapperMap::iterator iter = ioAssetMediaInfoWrapperMap.find(clipPath);
	if (iter != ioAssetMediaInfoWrapperMap.end())
	{
		ioAssetMediaInfoWrapperMap.erase(iter);
	}
}

/*
**
*/
void SRProject::UnRegisterAllAssetMediaInfo()
{
	ASL::CriticalSectionLock lock(GetLocker());

	mProjectResourceChanged = true;

	std::set<ASL::String> removedPaths;
	BOOST_FOREACH (AssetMediaInfoWrapperMap::value_type& pathInfoPair, mAssetMediaInfoWrapperMap)
	{
		removedPaths.insert(pathInfoPair.first);
	}

	BOOST_FOREACH (const ASL::String& clipPath, removedPaths)
	{
		UnRegisterAssetMediaInfo(mAssetMediaInfoWrapperMap, clipPath);
	}
}

/*
**
*/
void SRProject::RemoveUnreferenceSRMedia(SRMediaSet& ioSRMedias, ASL::PathnameList const& inReservedMediaPathList)
{
	SRMediaSet reservedMediaSet;

	ASL::PathnameList::const_iterator iter = inReservedMediaPathList.begin();
	ASL::PathnameList::const_iterator end = inReservedMediaPathList.end();

	for (; iter != end; ++iter)
	{
		ISRMediaRef srMedia = GetSRMedia(ioSRMedias, *iter);
		if (srMedia)
		{
			reservedMediaSet.insert(srMedia);
		}
	}

	SRMediaSet tempSet = ioSRMedias;
	SRMediaSet::iterator mediaIter = tempSet.begin();
	SRMediaSet::iterator mediaEnd = tempSet.end();

	for (; mediaIter != mediaEnd; ++mediaIter)
	{
		if (reservedMediaSet.end() == reservedMediaSet.find(*mediaIter))
		{
			RemoveSRMedia(ioSRMedias, *mediaIter);
		}
	}
}

/*
**
*/
void SRProject::RemoveUnreferenceAssetMediaInfo(AssetMediaInfoWrapperMap& ioAssetMediaInfoMap, ASL::PathnameList const& inReservedMediaPathList)
{
	std::set<ASL::String> removedPaths;
	BOOST_FOREACH (AssetMediaInfoWrapperMap::value_type& pathInfoPair, ioAssetMediaInfoMap)
	{
		removedPaths.insert(pathInfoPair.first);
	}

	BOOST_FOREACH (const ASL::String& clipPath, inReservedMediaPathList)
	{
		std::set<ASL::String>::iterator iter = removedPaths.find(MZ::Utilities::NormalizePathWithoutUNC(clipPath));
		if (iter != removedPaths.end())
		{
			removedPaths.erase(iter);
		}
	}

	BOOST_FOREACH (const ASL::String& clipPath, removedPaths)
	{
		UnRegisterAssetMediaInfo(ioAssetMediaInfoMap, clipPath);
	}
}

/*
**
*/
void SRProject::OnMediaMetaDataWriteComplete(
				ASL::Result inResult, 
				const BE::IMediaMetaDataRef& inMediaMetaData, 
				const BE::RequestIDs& inRequestIDs)
{
	// For EA mode, we don't need to broadcast message.
	EAMedia::IEAMediaInfoRef eaMediaInfo(inMediaMetaData);
	if (eaMediaInfo)
	{
		return;
	}

	BE::IMediaFileRef mediaFileRef(inMediaMetaData);
	if (mediaFileRef)
	{
		ASL::String path = mediaFileRef->GetFilePath(); 

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			WriteXMPToDiskFinished(inResult, path, inRequestIDs));
	}
}

/*
**
*/
void SRProject::OnMediaMetaDataNotInSync(ASL::PathnameList const& inPathList)
{
	ASL::PathnameList::const_iterator iter = inPathList.begin();
	ASL::PathnameList::const_iterator end = inPathList.end();
	for (; iter != end; ++iter)
	{
		ASL::String clipPath = (*iter); 
		clipPath = MZ::Utilities::NormalizePathWithoutUNC(clipPath);

		XMPText xmpTest(new ASL::StdString());
		ASL::String outErrorInfo;
		if (ASL::ResultSucceeded(SRLibrarySupport::ReadXMPFromFile(clipPath, xmpTest, outErrorInfo)))
		{
			BE::IMasterClipRef masterClip;
			BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), clipPath);
			DVA_ASSERT(projectItem);
			if (projectItem)
			{
				masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);
			}
			else
			{
				MZ::ImportResultVector failureVector;
				// Not ignore audio, not ignore XMP
				masterClip = BEUtilities::ImportFile(clipPath, BE::IProjectRef(), failureVector, false, false); 
			}
			DVA_ASSERT(masterClip);
			CustomMetadata customMetadata = Utilities::GetCustomMedadata(masterClip);

			XMPilotEssenseMarkerList essenseMarkerList;
			NamespaceMetadataList planningMetadata;
			if (ParseXMPilotXmlFile(clipPath, essenseMarkerList, planningMetadata))
			{
				bool isMarkerChanged = ApplyXMPilotEssenseMarker(*xmpTest.get(), essenseMarkerList, customMetadata.mVideoFrameRate);
				bool isPlanningMetadataChanged = MergeXMPWithPlanningMetadata(xmpTest, planningMetadata);
				if (isMarkerChanged || isPlanningMetadataChanged)
				{
					AssetMediaInfoPtr assetMediaInfo = AssetMediaInfo::CreateMasterClipMediaInfo(
						ASL::Guid::CreateUnique(),
						clipPath,
						ASL::PathUtils::GetFilePart(clipPath),
						xmpTest);
					if (!SRLibrarySupport::SaveXMPToDisk(assetMediaInfo))
					{
						const dvacore::UTF16String errorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/SRLibrarySupport/SaveFailed=The file @0 cannot be saved", ASL::PathUtils::GetFullFilePart(clipPath.c_str()));
						ML::SDKErrors::SetSDKErrorString(errorMsg);
					}
				}
			}

			AssetMediaInfoPtr assetMediaInfo;

			AssetMediaInfoWrapperMap::iterator mediaInfoIter = mAssetMediaInfoWrapperMap.find(clipPath);
			if(mediaInfoIter != mAssetMediaInfoWrapperMap.end())
			{
				if (!Utilities::IsXMPStemEqual(xmpTest, mediaInfoIter->second->GetAssetMediaInfo()->GetXMPString()))
				{
					mediaInfoIter->second->RefreshMediaXMP(xmpTest, false, true);
					assetMediaInfo = mediaInfoIter->second->GetAssetMediaInfo();
				}
			}
			else
			{
				assetMediaInfo = AssetMediaInfo::CreateMasterClipMediaInfo(
					ASL::Guid::CreateUnique(),
					*iter,
					ASL::PathUtils::GetFilePart(*iter),
					xmpTest);
			}

			if (assetMediaInfo)
			{
				AssetMediaInfoList assetMediaInfoList;
				assetMediaInfoList.push_back(assetMediaInfo);
				GetAssetLibraryNotifier()->NotifyXMPChanged(
					ASL::Guid::CreateUnique(), 
					ASL::Guid::CreateUnique(), 
					assetMediaInfoList);
			}
		}
	}
}

/*
**
*/
void SRProject::ValidProjectData()
{
	std::size_t primaryClipSize = mSRPrimaryClipList.size();
	std::size_t srMediaSize = mSRMedias.size();
	std::size_t assetMediaInfoSize = mAssetMediaInfoWrapperMap.size();
	std::size_t selectionSize = mAssetSelectionManager->GetSelectedAssetItemList().size();

	DVA_TRACE("SRProject content:", 5, 
		"primaryClip -- Size: "<<primaryClipSize<<
		"SRMedia -- Size: "<<srMediaSize<<
		"assetMediaInfo -- Size "<<assetMediaInfoSize<<
		"AssetItemselection -- Size"<<selectionSize);
}

/*
**
*/
void SRProject::RemoveUnreferenceResources()
{
	mUnreferencedResourceRemovalExecutor->CallAsynchronously(boost::bind(&SRProject::RemoveUnreferenceResourcesInternal, this));
}

/*
**
*/
void SRProject::RemoveUnreferenceResourcesInternal()
{
	if (GetInstance() == NULL)
		return;

	SRPrimaryClipList			backupPrimaryList;
	AssetMediaInfoWrapperMap	backupAssetMediaInfoWrapperMap;
	SRMediaSet					backupSRMediaSet;
	{
		ASL::CriticalSectionLock lock(GetLocker());

		if (!mProjectResourceChanged)
			return;

		// backup data
		backupPrimaryList = mSRPrimaryClipList;
		backupAssetMediaInfoWrapperMap = mAssetMediaInfoWrapperMap;
		backupSRMediaSet = mSRMedias;

		mProjectResourceChanged = false;
	}

	ASL::PathnameList reservedMediaPathList;
	SRPrimaryClipList::const_iterator iter = backupPrimaryList.begin();
	SRPrimaryClipList::const_iterator end = backupPrimaryList.end();

	//	Get referenced media path list from primaryClip
	for (; iter != end ; ++iter)
	{
		ASL::PathnameList tempList = (*iter)->GetReferencedMediaPath();
		BOOST_FOREACH(const ASL::String& mediaPath, tempList)
		{
			reservedMediaPathList.push_back(mediaPath);
		}
	}

	//	Get referenced media path list for unsaved media
	ASL::PathnameList cachedMediaPathList= WriteXMPToDiskCache::GetInstance()->GetCachedXMPMediaPathList();
	BOOST_FOREACH(const ASL::String& mediaPath, cachedMediaPathList)
	{
		reservedMediaPathList.push_back(mediaPath);
	}

	//	Remove unreferenced SRMedia
	RemoveUnreferenceSRMedia(backupSRMediaSet, reservedMediaPathList);

	//	Get referenced assetMediaInfo from current library selection
	AssetItemList assetItemlist = GetAssetSelectionManager()->GetSelectedAssetItemList();
	BOOST_FOREACH(const AssetItemPtr& assetItem, assetItemlist)
	{
		reservedMediaPathList.push_back(assetItem->GetMediaPath());
		if (!assetItem->GetSubAssetItemList().empty())
		{
			BOOST_FOREACH(const AssetItemPtr& subAssetItem, assetItem->GetSubAssetItemList())
			{
				reservedMediaPathList.push_back(subAssetItem->GetMediaPath());
			}
		}
	}

	//	Remove unreferenced assetMediaInfo
	RemoveUnreferenceAssetMediaInfo(backupAssetMediaInfoWrapperMap, reservedMediaPathList);

	// restore data
	{
		ASL::CriticalSectionLock lock(GetLocker());

		if (!mProjectResourceChanged)
		{
			mSRPrimaryClipList = backupPrimaryList;
			mAssetMediaInfoWrapperMap = backupAssetMediaInfoWrapperMap;
			mSRMedias = backupSRMediaSet;
		}
	}
}

/*
**
*/
AssetMediaInfoPtr SRProject::GetAssetMediaInfo(const ASL::String& inMediaPath, bool inCreateDummy) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	AssetMediaInfoWrapperPtr mediaInfoWrapper = GetAssetMediaInfoWrapper(inMediaPath);
	if (mediaInfoWrapper != NULL)
	{
		return mediaInfoWrapper->GetAssetMediaInfo();
	}
	else if (inCreateDummy)
	{
		return CreateDummyMasterClipMediaInfo(inMediaPath);
	}
	return AssetMediaInfoPtr();
}

AssetMediaInfoWrapperPtr SRProject::FindAssetMediaInfoWrapper(const ASL::Guid& inMediaInfoID) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	BOOST_FOREACH(AssetMediaInfoWrapperMap::value_type pair, mAssetMediaInfoWrapperMap)
	{
		if (pair.second->GetAssetMediaInfo()->GetAssetMediaInfoGUID() == inMediaInfoID)
		{
			return pair.second;
		}
	}
	return AssetMediaInfoWrapperPtr();
}

AssetMediaInfoPtr SRProject::FindAssetMediaInfo(const ASL::Guid& inMediaInfoID) const
{
	AssetMediaInfoWrapperPtr mediaInfoWrapper = FindAssetMediaInfoWrapper(inMediaInfoID);

	return mediaInfoWrapper != NULL ? mediaInfoWrapper->GetAssetMediaInfo() : AssetMediaInfoPtr();
}

/*
**
*/
AssetMediaInfoWrapperPtr SRProject::GetAssetMediaInfoWrapper(const ASL::String& inMediaPath) const
{
	ASL::CriticalSectionLock lock(GetLocker());
	ASL::String clipPath = inMediaPath;
	if (ASL::PathUtils::IsValidPath(clipPath))
	{
		clipPath = MZ::Utilities::NormalizePathWithoutUNC(clipPath);
	}

	AssetMediaInfoWrapperMap::const_iterator iter = mAssetMediaInfoWrapperMap.find(clipPath);
	return (iter != mAssetMediaInfoWrapperMap.end()) ? iter->second: AssetMediaInfoWrapperPtr();
}

/*
**
*/
bool SRProject::RefreshAssetMediaInfoByPath(const ASL::String& inFilePath, XMPText const& inXMPTest)
{
	ASL::CriticalSectionLock lock(GetLocker());
	AssetMediaInfoWrapperPtr mediaInfoWrapper = GetAssetMediaInfoWrapper(inFilePath);

	if (mediaInfoWrapper != NULL)
	{
		mediaInfoWrapper->RefreshMediaXMP(inXMPTest);
		return true;
	}
	else
	{
		return false;
	}
}

bool SRProject::RefreshAssetMediaInfoByID(const ASL::Guid& inMediaInfoID, XMPText const& inXMPTest)
{
	ASL::CriticalSectionLock lock(GetLocker());
	AssetMediaInfoWrapperPtr mediaInfoWrapper = FindAssetMediaInfoWrapper(inMediaInfoID);

	if (mediaInfoWrapper != NULL)
	{
		mediaInfoWrapper->RefreshMediaXMP(inXMPTest);
		return true;
	}
	else
	{
		return false;
	}
}
	
/*
**
*/
void SRProject::SetAssetLibraryNotifier(IAssetLibraryNotifier* inAssetLibraryNotifier)
{
	mAssetLibraryNotifier = inAssetLibraryNotifier;
}
	
/*
**
*/
IAssetLibraryNotifier* SRProject::GetAssetLibraryNotifier()
{
	return mAssetLibraryNotifier;
}	

//////////////////////////////////////////AssetMediaInfoWrapper////////////////////////////////////////////////////

/*
**
*/
AssetMediaInfoWrapper::AssetMediaInfoWrapper(AssetMediaInfoPtr const& inAssetMediaInfo)
	:
mAssetMediaInfo(inAssetMediaInfo),
mOffline(false),
mRedirectToXMPMonitor(false),
mStationID(ASL::StationRegistry::RegisterUniqueStation())
{
	if (mAssetMediaInfo && mAssetMediaInfo->GetNeedSavePreCheck())
	{
		mRedirectToXMPMonitor = SRMediaMonitor::GetInstance()->IsFileMonitored(mAssetMediaInfo->GetMediaPath());
	}

	//	Only work in file mode
	UpdateXMPLastModTime();

	//	Update Marker state
	UpdateMarkerState();

	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
void AssetMediaInfoWrapper::OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String inMediaPath, const BE::RequestIDs& inRequestIDs)
{
	if (ASL::ResultFailed(inResult))
	{
		return;
	}

	// In EA mode, media path should be empty.
	if (mAssetMediaInfo->GetMediaPath().empty() || ASL::CaseInsensitive::StringEquals(
		ASL::PathUtils::ToNormalizedPath(inMediaPath), 
		ASL::PathUtils::ToNormalizedPath(mAssetMediaInfo->GetMediaPath())))
	{
		//	Only work in file mode
		UpdateXMPLastModTime();
	}
}

/*
**
*/
void AssetMediaInfoWrapper::UpdateXMPLastModTime()
{
	if (!mRedirectToXMPMonitor && mAssetMediaInfo && mAssetMediaInfo->GetNeedSavePreCheck())
	{
		XMP_FileFormat format = ML::MetadataManager::GetXMPFormat(mAssetMediaInfo->GetMediaPath());

		bool getFileModTimeSuccess = false;
		try
		{
			getFileModTimeSuccess = SXMPFiles::GetFileModDate(ASL::MakeStdString(mAssetMediaInfo->GetMediaPath()).c_str(), &mXMPLastModifyTime, &format, kXMPFiles_ForceGivenHandler);
		}
		catch(...)
		{
			getFileModTimeSuccess = false;
		}

		if (!getFileModTimeSuccess)
		{
			mXMPLastModifyTime.hasDate = false;
			mXMPLastModifyTime.hasTime= false;
		}
	}
}

/*
**
*/
void AssetMediaInfoWrapper::UpdateMarkerState()
{
	GetLatestMetadataState(kXMPPartID_Tracks, mMarkerState);
}

/*
**
*/
XMP_DateTime AssetMediaInfoWrapper::GetXMPLastModTime()const
{
	if (mRedirectToXMPMonitor)
	{
		XMP_DateTime lastModTime;
		bool result = SRMediaMonitor::GetInstance()->GetXMPModTime(mAssetMediaInfo->GetMediaPath(), lastModTime);
		DVA_ASSERT(result);
		return lastModTime;
	}
	else
	{
		return mXMPLastModifyTime;
	}
}

/*
**
*/
void AssetMediaInfoWrapper::OnUnregisterSRMediaMonitor(ASL::String const& inMediaPath)
{
	if (mAssetMediaInfo && mAssetMediaInfo->GetNeedSavePreCheck())
	{
		if (ASL::CaseInsensitive::StringEquals(
							ASL::PathUtils::ToNormalizedPath(inMediaPath), 
							ASL::PathUtils::ToNormalizedPath(mAssetMediaInfo->GetMediaPath())))
		{
			mRedirectToXMPMonitor = false;
			UpdateXMPLastModTime();
		}
	}
}

/*
**
*/
void AssetMediaInfoWrapper::OnRegisterSRMediaMonitor(ASL::String const& inMediaPath)
{
	if (mAssetMediaInfo && mAssetMediaInfo->GetNeedSavePreCheck())
	{
		if (ASL::CaseInsensitive::StringEquals(
							ASL::PathUtils::ToNormalizedPath(inMediaPath), 
							ASL::PathUtils::ToNormalizedPath(mAssetMediaInfo->GetMediaPath())))
		{
			mRedirectToXMPMonitor = true;
			UpdateXMPLastModTime();
		}
	}
}

/*
**
*/
void AssetMediaInfoWrapper::BackToCachedXMPData()
{
	ASL::String mediaInfoID;
	if (UIF::IsEAMode())
	{
		// In EA mode, AssetMediaInfo::mMediaPath is empty. So we need use MediaInfoGuid.
		mediaInfoID = mAssetMediaInfo->GetAssetMediaInfoGUID().AsString();
	}
	else
	{
		mediaInfoID = mAssetMediaInfo->GetMediaPath();
	}

	ASL::StationUtils::PostMessageToUIThread(
			mStationID,
			MediaMetaDataChanged(mediaInfoID),
			true);
}

/*
**
*/
void AssetMediaInfoWrapper::RefreshMediaXMP(XMPText const& inXMPTest, bool isSync, bool checkDirty)
{
	if (inXMPTest)
	{
		(*mAssetMediaInfo->GetXMPString()) = *inXMPTest;

		ASL::String mediaInfoID;
		if (UIF::IsEAMode())
		{
			// In EA mode, AssetMediaInfo::mMediaPath is empty. So we need use MediaInfoGuid.
			mediaInfoID = mAssetMediaInfo->GetAssetMediaInfoGUID().AsString();
		}
		else
		{
			mediaInfoID = mAssetMediaInfo->GetMediaPath();
		}

		bool sendMarkChangedMsg(false);
		dvacore::StdString	oldMarkerState = mMarkerState;
		UpdateMarkerState();
		sendMarkChangedMsg = (oldMarkerState != mMarkerState);

		//	[TRICKY] If Prelude detect xmp mod time not consistent, we send marker changed msg anyway.
		//	Cause there is no perfect solution to detect if outside changes affect marker or not.
		//	One example: Prelude change marker itself, Prelude have to listen to writeXMPToDisk finished 
		//	msg and update marker state in that msg routine, or else the cached one will not consistent with
		//	marker state in file. If we do that, it means some I/O and may not acceptable.
		if (checkDirty)
		{
			ASL::StationUtils::PostMessageToUIThread(
				mStationID,
				MediaMetaDataMarkerNotInSync(mediaInfoID),
				true);
		}
		else if (sendMarkChangedMsg)
		{
			ASL::StationUtils::PostMessageToUIThread(
				mStationID,
				MediaMetaDataMarkerChanged(mediaInfoID),
				true);
		}

		ASL::StationUtils::PostMessageToUIThread(
			mStationID,
			MediaMetaDataChanged(mediaInfoID),
			!isSync);
	}
}

/*
**
*/
ASL::Result AssetMediaInfoWrapper::GetLatestMetadataState(const dvacore::StdString& inPartID, dvacore::StdString& outMetadataState)
{
	ASL::Result result(-1);
	dvacore::StdString metadataState;

	// If no partID was specified, let's use /metadata to get everything metadata related
	dvacore::StdString partID = inPartID;
	if (partID.empty())
	{
		partID = std::string(kXMPPartID_Metadata);
	}

	try	
	{
		SXMPDocOps xmpDocOps;
		SXMPMeta meta(mAssetMediaInfo->GetXMPString()->c_str(), static_cast<XMP_StringLen>(mAssetMediaInfo->GetXMPString()->length()));
		xmpDocOps.OpenXMP(reinterpret_cast<SXMPMeta*>(&meta), "");

		//	We ask the doc ops if there is a part ID in this file.
		xmpDocOps.GetPartChangeID(inPartID.c_str(), &metadataState);

		if (!metadataState.empty())
		{
			outMetadataState = metadataState;
			result = ASL::kSuccess;
		}
	}
	catch(...)
	{
	}

	return result;
}

/*
**
*/
void AssetMediaInfoWrapper::OnMediaMetaDataOutOfSync(ASL::PathnameList const& inPathList)
{
	//	Only work in file mode
	if (mRedirectToXMPMonitor && mAssetMediaInfo && mAssetMediaInfo->GetNeedSavePreCheck())
	{
		BOOST_FOREACH(ASL::String const& mediaPath, inPathList)
		{
			if (ASL::CaseInsensitive::StringEquals(
				ASL::PathUtils::ToNormalizedPath(mediaPath), 
				ASL::PathUtils::ToNormalizedPath(mAssetMediaInfo->GetMediaPath())))
			{
				UpdateXMPLastModTime();
				break;
			}
		}
	}
}

}
