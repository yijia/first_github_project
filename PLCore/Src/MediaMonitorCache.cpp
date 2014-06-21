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
#include "Prefix.h"

#include "PLMediaMonitorCache.h"
#include "MZThumbnailSupport.h"
#include "PLConstants.h"
#include "PLMessage.h"
#include "PLUtilities.h"
#include "PLProject.h"
#include "MZUtilities.h"
#include "MLMetadataManager.h"
#include "PLWriteXMPToDiskCache.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLThreadedQueueRequest.h"
#include "ASLStationUtils.h"

// BE
#include "BE/IBackend.h"

//	std
#include "boost/bind.hpp"
#include "boost/foreach.hpp"

namespace PL
{

namespace
{
/*
** XMP_FileFormat can help us get XMP_DateTime with high speed.
** XMP_DateTime is the last ModTime of XMP file
*/
typedef std::pair<XMP_FileFormat, XMP_DateTime> MediaFormatModTimePair;
typedef std::map<ASL::String, MediaFormatModTimePair>  MediaXMPLastEditMap;
MediaXMPLastEditMap sMediaXMPModTimeMap;

ASL_DEFINE_CLASSREF(RefreshFileRequest, ASL::IThreadedQueueRequest);
ASL_DEFINE_IMPLID(RefreshFileRequest, 0xcef36650, 0xaa2c, 0x4be1, 0xb8, 0xd0, 0x42, 0x6a, 0xb7, 0xeb, 0x6f, 0xd9);


/*
**	Used for Monitoring File offline.
*/
SRMediaMonitor::MediaFileMonitorSet sMediaFileMonitorSet;

/*
**
*/
class RefreshFileRequest
	:
	public ASL::IThreadedQueueRequest
{
	ASL_BASIC_CLASS_SUPPORT(RefreshFileRequest);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_ENTRY(RefreshFileRequest)
	ASL_QUERY_MAP_END

public:

	/**
	**
	*/
	RefreshFileRequest()
			:
			mAbort(0)
			{};

	/**
	**
	*/
	void Initialize(MediaXMPLastEditMap const& inXMPLastEditMap)
	{
		mMediaXMPLastEditMap = inXMPLastEditMap;
	};

	/**
	**
	*/
	void Process();

	/**
	**
	*/
	bool Start();

	/**
	**
	*/
	void Abort(){ASL::AtomicCompareAndSet(mAbort, 0, 1);}

	/**
	**
	*/
	bool GetAbort(){return (ASL::AtomicRead(mAbort) == 1);}

private:
	volatile ASL::AtomicInt		mAbort;
	MediaXMPLastEditMap			mMediaXMPLastEditMap;
};
RefreshFileRequestRef sRefreshFileRequest;

/*
**
*/
bool RefreshFileRequest::Start()
{
	dvacore::threads::AsyncExecutorPtr threadedWorkQueue = MZ::GetThumbExtractExecutor();
	return (threadedWorkQueue != NULL) &&
		threadedWorkQueue->CallAsynchronously(boost::bind(&ASL::IThreadedQueueRequest::Process, ASL::IThreadedQueueRequestRef(this)));
}

/*
**
*/
void RefreshFileRequest::Process()
{
	ASL::PathnameList outSyncPathList;

	MediaXMPLastEditMap::iterator iter = mMediaXMPLastEditMap.begin();
	MediaXMPLastEditMap::iterator end = mMediaXMPLastEditMap.end();

	for (; iter != end; ++iter)
	{
		XMP_DateTime modDate;
		// Must have init value to avoid impact from other type of medias
		XMP_FileFormat format = ML::MetadataManager::GetXMPFormat(iter->first);

		if (GetAbort())
		{
			break;
		}
		try
		{
			if (ASL::PathUtils::ExistsOnDisk(iter->first) && Utilities::PathSupportsXMP(iter->first))
			{
				bool modDateResult = SXMPFiles::GetFileModDate(ASL::MakeStdString(iter->first).c_str(), &modDate, &format, kXMPFiles_ForceGivenHandler);

				if (!PL::Utilities::IsDateTimeEqual(iter->second.second, modDate))
				{
					outSyncPathList.push_back(iter->first);
					PL::SRMediaMonitor::GetInstance()->UpdateXMPModTime(iter->first);
				}
			}
		}
		catch(...)
		{

		}
	}

	if (!GetAbort() && !outSyncPathList.empty())
	{
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			PL::MediaMetaDataOutOfSync(outSyncPathList));
	}
}

}	//	namespace

SRMediaMonitor::SharedPtr SRMediaMonitor::sSRMediaMonitor;
ASL::CriticalSection sMediaMonitorLock;

ASL_MESSAGE_MAP_DEFINE(SRMediaMonitor)
	ASL_MESSAGE_HANDLER(PL::WriteXMPToDiskFinished, OnWriteXMPToDiskFinished)
ASL_MESSAGE_MAP_END


/*
**
*/
void SRMediaMonitor::Initialize()
{
}

/*
**
*/
void SRMediaMonitor::Terminate()
{
	if (sRefreshFileRequest)
	{
		sRefreshFileRequest->Abort();
		sRefreshFileRequest = RefreshFileRequestRef();
	}
	
	if (sSRMediaMonitor)
	{
		sSRMediaMonitor.reset();
	}
}

/*
**
*/
SRMediaMonitor::SRMediaMonitor()
{
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
SRMediaMonitor::~SRMediaMonitor()
{
	ASL::StationUtils::RemoveListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
SRMediaMonitor::SharedPtr SRMediaMonitor::GetInstance()
{
	if (NULL == sSRMediaMonitor)
	{
		sSRMediaMonitor.reset(new SRMediaMonitor());
	}

	return sSRMediaMonitor;
}

/*
**
*/
bool SRMediaMonitor::UpdateXMPModTime(ASL::String const& inMediaPath)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);

	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	bool result(false);
	if (sMediaXMPModTimeMap.find(mediaPath) != sMediaXMPModTimeMap.end())
	{
		try
		{
			MediaFormatModTimePair tempPair;
			tempPair.first = ML::MetadataManager::GetXMPFormat(inMediaPath);
			bool modDateResult = SXMPFiles::GetFileModDate(ASL::MakeStdString(mediaPath).c_str(), &tempPair.second, &tempPair.first, kXMPFiles_ForceGivenHandler);
			MediaFormatModTimePair& savedValue = sMediaXMPModTimeMap[mediaPath];
			if (savedValue.first != tempPair.first || !PL::Utilities::IsDateTimeEqual(savedValue.second, tempPair.second))
			{
				savedValue = tempPair;
				result = true;
			}
		}
		catch (...)
		{
		}
	}

	return result;
}

/*
**
*/
bool SRMediaMonitor::GetXMPModTime(ASL::String const& inMediaPath, XMP_DateTime& outXMPDate)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	MediaXMPLastEditMap::iterator  iter = sMediaXMPModTimeMap.find(inMediaPath);
	if (iter != sMediaXMPModTimeMap.end())
	{
		outXMPDate = iter->second.second;
	}

	return ( iter != sMediaXMPModTimeMap.end() );
}

/*
**
*/
bool SRMediaMonitor::IsFileMonitored(ASL::String const& inMediaPath) const
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	return (sMediaXMPModTimeMap.find(inMediaPath) != sMediaXMPModTimeMap.end());
}

/*
**
*/
bool SRMediaMonitor::RefreshXMPIfChanged(ASL::String const& inMediaPath)
{
	if (UpdateXMPModTime(inMediaPath))
	{
		ASL::PathnameList pathList;
		pathList.push_back(inMediaPath);
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			PL::MediaMetaDataOutOfSync(pathList));
		return true;
	}
	return false;
}

/*
**
*/
bool SRMediaMonitor::RegisterMonitorMediaXMP(ASL::String const& inMediaPath)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	if (sMediaXMPModTimeMap.find(mediaPath) == sMediaXMPModTimeMap.end())
	{
		MediaFormatModTimePair tempPair;
		try
		{
			tempPair.first = ML::MetadataManager::GetXMPFormat(inMediaPath);
			bool modDateResult =  SXMPFiles::GetFileModDate(ASL::MakeStdString(mediaPath).c_str(), &tempPair.second, &tempPair.first, kXMPFiles_ForceGivenHandler);
		}
		catch (...)
		{
		}

		//	No matter success or not we should always register. 
		//	cause we may get false when a side-car format like MPEG's side car file missing.
		sMediaXMPModTimeMap[mediaPath] = tempPair;

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			PL::RegisterMediaMonitor(mediaPath),
			true);
	}

	return (sMediaXMPModTimeMap.find(inMediaPath) != sMediaXMPModTimeMap.end());
}

/*
**
*/
void SRMediaMonitor::RegisterMonitorFilePath(ASL::String const& inMediaPath)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	if (!inMediaPath.empty())
	{
		ASL::String mediaPath = inMediaPath;
		mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

		sMediaFileMonitorSet.insert(inMediaPath);
	}
}

/*
**
*/
void SRMediaMonitor::UnRegisterMonitorFilePath(ASL::String const& inMediaPath)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	if (!inMediaPath.empty())
	{
		ASL::String mediaPath = inMediaPath;
		mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

		sMediaFileMonitorSet.erase(inMediaPath);
	}
}

/*
**
*/
bool SRMediaMonitor::UnRegisterMonitorMediaXMP(ASL::String const& inMediaPath)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	MediaXMPLastEditMap::iterator iter = sMediaXMPModTimeMap.find(mediaPath);
	if (iter  != sMediaXMPModTimeMap.end())
	{
		sMediaXMPModTimeMap.erase(iter);

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject,
			PL::UnregisterMediaMonitor(mediaPath),
			true);
		return true;
	}

	return false;
}

/*
**
*/
void SRMediaMonitor::RefreshFiles(ASL::PathnameList& outOfflinePathList)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	BOOST_FOREACH(ASL::String const& mediaPath, sMediaFileMonitorSet)
	{
		if (!ASL::PathUtils::ExistsOnDisk(mediaPath))
		{
			outOfflinePathList.push_back(mediaPath);
		}
	}
}

/*
**
*/
void SRMediaMonitor::RefreshMediaXMP(ASL::PathnameList const& inExclusivePathList)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	MediaXMPLastEditMap tempXMPLastEditMap;


	BOOST_FOREACH(MediaXMPLastEditMap::value_type pair, sMediaXMPModTimeMap)
	{
		//	If there is pending xmp write, we don't need to check xmp mod time here
		//	Performance optimization.
		if (std::find(inExclusivePathList.begin(), inExclusivePathList.end(), pair.first) == inExclusivePathList.end() && 
			!PL::WriteXMPToDiskCache::GetInstance()->ExistCache(pair.first))
		{
			tempXMPLastEditMap.insert(pair);
		}
	}

	if (sRefreshFileRequest)
	{
		sRefreshFileRequest->Abort();
	}

	if (tempXMPLastEditMap.empty())
	{
		return;
	}

	sRefreshFileRequest = RefreshFileRequest::CreateClassRef();
	sRefreshFileRequest->Initialize(tempXMPLastEditMap);
	sRefreshFileRequest->Start();
}

/*
**
*/
void SRMediaMonitor::Refresh(ASL::PathnameList const&)
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	//	Get offline files
	ASL::PathnameList offlineFiles;
	RefreshFiles(offlineFiles);

	if (!offlineFiles.empty())
	{
		//	if file is offline, we need send event to client, and remove all registration from the SRMediaMonitor
		BOOST_FOREACH(ASL::String const& offlineFile, offlineFiles)
		{
			UnRegisterMonitorFilePath(offlineFile);
			UnRegisterMonitorMediaXMP(offlineFile);
			PL::AssetMediaInfoWrapperPtr assetMediaInfoWrapper = PL::SRProject::GetInstance()->GetAssetMediaInfoWrapper(offlineFile);
			if (assetMediaInfoWrapper)
			{
				assetMediaInfoWrapper->SetAssetMediaInfoOffline(true);
			}
		}

		//	Send off-line event
		PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->NotifyOffLineFiles(
			ASL::Guid::CreateUnique(),
			ASL::Guid::CreateUnique(),
			offlineFiles);
	}

	//	Refresh media XMP which is still on-line.
	RefreshMediaXMP();

	//	We can pop-up relink dialog to let user select relink file here
	// [TODO] Nice to have

}

/*
**
*/
void SRMediaMonitor::OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String const& inMediaParh, BE::RequestIDs)
{
	if (ASL::ResultSucceeded(inResult))
	{
		UpdateXMPModTime(inMediaParh);
	}
}

/*
**
*/
SRMediaMonitor::MediaFileMonitorSet SRMediaMonitor::GetMediaFileMonitorSet() const
{
	ASL::CriticalSectionLock lock(sMediaMonitorLock);
	return sMediaFileMonitorSet;
}

void RefreshBackEndFileCache(const ASL::String& inPath)
{
	// refresh media file cache
	BE::IBackendRef backend(BE::GetBackend());
	if (backend != NULL)
	{
		BE::IFileCacheRef fileCache(backend->GetFileCache());
		if (fileCache != NULL)
		{
			bool isMissing = false;
			fileCache->RefreshFile(inPath, isMissing);
		}
	}
}

}	//	MZ