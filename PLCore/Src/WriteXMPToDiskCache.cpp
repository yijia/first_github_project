/***********************
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

//	Self
#include "PLWriteXMPToDiskCache.h"
#include "IncludeXMP.h"

//	MZ
#include "PLLibrarySupport.h"
#include "PLUtilities.h"
#include "MZThumbnailSupport.h"
#include "MZBEUtilities.h"
#include "PLProject.h"
#include "PLMediaMonitorCache.h"
#include "PLConstants.h"
#include "MZUtilities.h"

//	ASL
#include "ASLMixHashGuid.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLPathUtils.h"
#include "ASLFile.h"
#include "ASLDirectory.h"
#include "ASLThreadedQueueRequest.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLSleep.h"

//	MediaCore
#include "XMLUtils.h"

//	DVA	
#include "dvacore/xml/XMLUtils.h"
#include "dvamediatypes/FrameRate.h"
#include "dvametadata/Initialize.h"
#include "dvacore/config/Localizer.h"
#include "dvacore/threads/SharedThreads.h"

// XMP
#include "IncludeXMP.h"
#include "XMP.incl_cpp"

namespace PL
{

namespace
{

const dvacore::UTF16String kCacheXMPFileRoot		= DVA_STR("XMPCache");
const dvacore::UTF16String kMediaPath					= DVA_STR("path");
const dvacore::UTF16String kMediaXMP					= DVA_STR("xmp");
const dvacore::UTF16String kXMPCacheFileExt				= DVA_STR(".xmpt");
const dvacore::UTF16String kXMPCacheFileFolderName		= DVA_STR("XMPCache");

ASL_DEFINE_CLASSREF(SaveCacheXMPFileRequest, ASL::IThreadedQueueRequest);
ASL_DEFINE_IMPLID(SaveCacheXMPFileRequest, 0xcd98e854, 0x48b2, 0x4660, 0xbc, 0x80, 0xa8, 0x31, 0xfd, 0x50, 0x1e, 0xc5);

/*
**
*/
class SaveCacheXMPFileRequest
	:
	public ASL::IThreadedQueueRequest
{
	ASL_BASIC_CLASS_SUPPORT(SaveCacheXMPFileRequest);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_ENTRY(SaveCacheXMPFileRequest)
	ASL_QUERY_MAP_END

public:

	/**
	**
	*/
	SaveCacheXMPFileRequest()
			:
			mAbort(0),
			mProcesing(0)
			{};

	/**
	**
	*/
	void Initialize(
			ASL::String const& inMediaPath,
			ASL::Guid const& inAssetID,
			ASL::StdString	const& inXMPString,
			ASL::StationID	const& inStationID)
	{
		mMediaPath = inMediaPath;
		mXMPString = inXMPString;
		mStationID = inStationID;
		mAssetID = inAssetID;
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
	void Abort();

	/**
	**
	*/
	bool GetAbort(){return (ASL::AtomicRead(mAbort) == 1);}

	/**
	**
	*/
	ASL::StdString GetXMPString() const {return mXMPString;}

	ASL::Guid GetAssetID() const {return mAssetID;}

private:

	/**
	**
	*/
	bool SaveToXMPTempPath(
					const ASL::String& inXMPTempPath,
					const ASL::String& inMediaPath,
					const ASL::StdString& inXMPString);

private:
	volatile ASL::AtomicInt		mAbort;
	volatile ASL::AtomicInt		mProcesing;
	ASL::String					mMediaPath;
	ASL::StdString				mXMPString;
	ASL::StationID			    mStationID;
	ASL::Guid					mAssetID;
};


/*
**
*/
ASL::String GetSettingsFullPath(
	const ASL::String& inFileName)
{
	ASL::String fullPath;
	ASL::String premiereDocuments = ASL::MakeString(ASL::kUserDocumentsDirectory);
	fullPath = ASL::DirectoryRegistry::FindDirectory(premiereDocuments);
	if (!fullPath.empty())
	{
		fullPath = ASL::PathUtils::CombinePaths(fullPath, kXMPCacheFileFolderName);
		fullPath = ASL::PathUtils::AddTrailingSlash(fullPath);
	}

	if (!ASL::Directory::IsDirectory(fullPath))
	{
		ASL::Directory::CreateOnDisk(fullPath);
	}

	if (!inFileName.empty())
	{
		fullPath += inFileName;
	}
	return fullPath;
}

/*
**
*/
ASL::String GetCacheXMPFilePath(ASL::String const& inMediaPath)
{
	ASL::MixHashGuid hash("WriteXMPToDiskHash");
	hash.MixInValue(inMediaPath);
	return  GetSettingsFullPath(hash.ToGuid().AsString()+ kXMPCacheFileExt);
}

/*
**
*/
bool SaveCacheXMPFileRequest::SaveToXMPTempPath(
	const ASL::String& inXMPTempPath,
	const ASL::String& inMediaPath,
	const ASL::StdString& inXMPString)
{
	if (ASL::PathUtils::IsValidPath(inXMPTempPath) && inXMPTempPath.size() <= static_cast<ASL::String::size_type>(ASL::ASL_MAX_PATH))
	{
		ASL::File tempFile;
		tempFile.Create(
			inXMPTempPath,
			ASL::FileAccessFlags::kWrite,
			ASL::FileShareModeFlags::kNone,
			ASL::FileCreateDispositionFlags::kCreateAlways,
			ASL::FileAttributesFlags::kAttributeNormal);
	}
	else
	{
		return false;
	}

	if (!GetAbort())
	{
		XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
		XMLUtils::CreateDOM(&parser, inXMPTempPath.c_str(), &doc);
		if(doc != NULL)
		{
			DOMElement* rootElement = doc->createElement(kCacheXMPFileRoot.c_str());
			doc->appendChild(rootElement);

			rootElement->setAttribute(kMediaPath.c_str(), inMediaPath.c_str());

			ASL::String encodedXMPString;
			ASL::String const& xmpString = ASL::MakeString(inXMPString);
			dvacore::xml::utils::EncodeReservedCharacters(xmpString, encodedXMPString);

			XMLUtils::AddChild(rootElement, kMediaXMP.c_str(), encodedXMPString.c_str());

			return  GetAbort() ? false : XMLUtils::DOMToXML(&parser, doc, inXMPTempPath.c_str());
		}
	}

	return false;
}

/*
**
*/
bool SaveCacheXMPFileRequest::Start()
{
	if (ASL::AtomicRead(mProcesing))
	{
		Abort();
	}
	ASL::AtomicCompareAndSet(mAbort, 1, 0);
	dvacore::threads::AsyncExecutorPtr threadedWorkQueue = MZ::GetThumbExtractExecutor();
	return (threadedWorkQueue != NULL) &&
		threadedWorkQueue->CallAsynchronously(boost::bind(&ASL::IThreadedQueueRequest::Process, ASL::IThreadedQueueRequestRef(this)));

}

/*
**
*/
void SaveCacheXMPFileRequest::Abort()
{
	ASL::AtomicCompareAndSet(mAbort, 0, 1);
	while(ASL::AtomicRead(mProcesing))
	{
		ASL::Sleep(10);
	}

	//	If this request has been aborted, the cache file associated with it should be deleted also if exist.
	ASL::String XMPCacheFilePath = GetCacheXMPFilePath(mMediaPath);
	if(ASL::PathUtils::ExistsOnDisk(XMPCacheFilePath))
	{
		ASL::File::Delete(XMPCacheFilePath);
	} 
}

/*
**
*/
class ProcessingGuard
{
public:
	ProcessingGuard(volatile ASL::AtomicInt& inProcessing)
	{
		mProcesing = inProcessing;
		ASL::AtomicCompareAndSet(mProcesing, 0, 1);
	}
	~ProcessingGuard()
	{
		ASL::AtomicCompareAndSet(mProcesing, 1, 0);
	}
private:
	volatile ASL::AtomicInt		mProcesing;
};
void SaveCacheXMPFileRequest::Process()
{
	ProcessingGuard processingGuard(mProcesing);

	// Write XMP to Prelude Cache
	ASL::String XMPCacheFilePath = GetCacheXMPFilePath(mMediaPath);
	SaveToXMPTempPath(XMPCacheFilePath , mMediaPath, mXMPString);	
}

/*
**
*/
typedef std::vector<ASL::SInt32>   WriteIDs;
struct  RequestDatasForSaveCacheXMP
{
	//	contains all ID which haven't return yet.
	WriteIDs mWriteIDs;

	//  The last write temp xmp file request.
	SaveCacheXMPFileRequestRef mLastXMPTempFileRequest;
};
typedef std::map<ASL::String, RequestDatasForSaveCacheXMP> PathRequestDataCache;
static PathRequestDataCache sPathRequestDataCache;

// sPathRequestDataCache may be updated in threaded functions, we should make its read/write atomic
static ASL::CriticalSection sRequestCacheCriticalSection;

/*
**
*/
bool ReadXMPTempFile(
			ASL::String const& inTempXMPPath,
			ASL::String& outMediaPath, 
			ASL::StdString& outXMPString)
{
	if (!ASL::PathUtils::ExistsOnDisk(inTempXMPPath))
	{
		return false;
	}

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
	XMLUtils::CreateDOM(&parser, inTempXMPPath.c_str(), &doc);
	if (doc == NULL)
		return false;

	DOMElement* rootElement = doc->getDocumentElement();
	DVA_ASSERT(rootElement != NULL);
	if (rootElement == NULL)
		return false;

	XMLUtils::GetAttr(rootElement, kMediaPath.c_str(), outMediaPath);
	if (outMediaPath.empty())
		return false;

	ASL::String encodedXMPString;
	ASL::String xmpString;
	ASL::StdString xmpStdString;
	XMLUtils::GetChildData(rootElement, kMediaXMP.c_str(), encodedXMPString);
	dvacore::xml::utils::DecodeReservedCharacters(encodedXMPString, xmpString);
	xmpStdString = ASL::MakeStdString(xmpString);
	
	bool result(false);
	SXMPMeta xmpMeta;

	if (!xmpStdString.empty())
	{
		try
		{
			xmpMeta.ParseFromBuffer(&xmpStdString[0], static_cast<XMP_StringLen>(xmpStdString.length()));
			result = true;
		}
		catch (XMP_Error& )
		{
		}
	}

	if (result)
	{
		outXMPString.swap(xmpStdString);
	}

	return result;
}

/*
**
*/
bool CanProcessContinue(ASL::StdString const& inTempXMPString, ASL::StdString const& inMediaXMPString)
{
	bool result(true);
	SXMPMeta  tempXMPMeta;
	SXMPMeta  mediaXMPMeta;

	try
	{
		tempXMPMeta.ParseFromBuffer(inTempXMPString.c_str(), static_cast<XMP_StringLen>(inTempXMPString.length()));
		mediaXMPMeta.ParseFromBuffer(inMediaXMPString.c_str(), static_cast<XMP_StringLen>(inMediaXMPString.length()));

		std::string tempXMPDocumentIDString;
		std::string mediaXMPDocumentIDString;
		XMP_StringLen tempStringLen;
		XMP_StringLen mediaStringLen;
		bool tempDocumentIDExist = tempXMPMeta.GetProperty(kXMP_NS_XMP_MM, "DocumentID", &tempXMPDocumentIDString, &tempStringLen);
		bool mediaDocumentIDExist = mediaXMPMeta.GetProperty(kXMP_NS_XMP_MM, "DocumentID", &mediaXMPDocumentIDString, &mediaStringLen);

		if (mediaDocumentIDExist && tempXMPDocumentIDString != mediaXMPDocumentIDString)
		{
			result = false;
		}
	}
	catch (XMP_Error& p)
	{
		result = false;
		ASL_UNUSED(p);
	}

	return result;
}

/*
**
*/
bool CombineXMPContent(ASL::String const& inMediaPath, ASL::StdString & ioXMPString)
{
	PL::XMPText mediaXMP(new ASL::StdString());
	ASL::String errorInfo;
	if (ASL::ResultSucceeded(SRLibrarySupport::ReadXMPFromFile(inMediaPath, mediaXMP, errorInfo)))
	{
		//	If same documentID, we return false, so we can delete the temp xmp file
		if (!CanProcessContinue(ioXMPString, *mediaXMP.get()))
		{
			return false;
		}

		bool tempFileContainsLatestChange = Utilities::IsNewerMetaData(ioXMPString, *mediaXMP.get());

		//	If temp file contains latest changes, we replace all data from current media file.
		if (tempFileContainsLatestChange)
		{
			//	No more action ,just return true.
			return true;
		}
		//	If current media file contains latest changes, we append markers from temp file into current media file
		else
		{
			//	Do merge here
			Utilities::MergeTemporalMarkers(*mediaXMP.get(), ioXMPString, ioXMPString);
		}
	}
	return true;
}

/*
**
*/
void FlushPendingMetadataToDisk()
{
	ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

	PathRequestDataCache::iterator iter = sPathRequestDataCache.begin();
	PathRequestDataCache::iterator end = sPathRequestDataCache.end();

	for (; iter != end; ++iter)
	{
		if (iter->second.mLastXMPTempFileRequest)
		{
			ASL::StdString xmpString = iter->second.mLastXMPTempFileRequest->GetXMPString();

			SXMPMeta metadata(xmpString.c_str(), (XMP_StringLen)xmpString.size());
			SXMPDocOps xmpDocOps;
			xmpDocOps.OpenXMP(&metadata, "", reinterpret_cast<const char*>(dvacore::utility::UTF16to8(iter->first).c_str()));
			xmpDocOps.NoteChange("/metadata");
			xmpDocOps.PrepareForSave("");

			bool saveSuccess(false);
			try
			{
				SXMPFiles file(reinterpret_cast<const char*>(dvacore::utility::UTF16to8(iter->first).c_str()),
					kXMP_UnknownFile,
					kXMPFiles_OpenForUpdate | kXMPFiles_OpenUseSmartHandler | kXMPFiles_OpenLimitedScanning);

				file.PutXMP(metadata);
				file.CloseFile();
				saveSuccess = true;
			}
			catch (XMP_Error& inError)
			{
				ASL_UNUSED(inError);
			}
			catch(...)
			{
				//	Do nothing here
			}

			if (saveSuccess)
			{
				iter->second.mLastXMPTempFileRequest->Abort();
			}
		}
	}

	sPathRequestDataCache.clear();
}

} // namespace

ASL_MESSAGE_MAP_DEFINE(WriteXMPToDiskCache)
	ASL_MESSAGE_HANDLER(WriteXMPToDiskFinished, OnWriteXMPToDiskFinished)
ASL_MESSAGE_MAP_END

WriteXMPToDiskCache::SharedPtr WriteXMPToDiskCache::sWriteXMPToDiskCache;

/*
**
*/
void WriteXMPToDiskCache::Initialize()
{
	dvametadata::Initialize();
}

/*
**
*/
void WriteXMPToDiskCache::Terminate()
{
	FlushPendingMetadataToDisk();

	if (sWriteXMPToDiskCache)
	{
		sWriteXMPToDiskCache.reset();
	}
	dvametadata::Terminate();
}

/*
**
*/
WriteXMPToDiskCache::SharedPtr WriteXMPToDiskCache::GetInstance()
{
	if ( !sWriteXMPToDiskCache )
	{
		sWriteXMPToDiskCache.reset(new WriteXMPToDiskCache());
		ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, sWriteXMPToDiskCache.get());
	}

	return sWriteXMPToDiskCache;
}

/*
**
*/
WriteXMPToDiskCache::WriteXMPToDiskCache()
{
	mStationID = ASL::StationRegistry::RegisterUniqueStation();
	ASL::StationUtils::AddListener(mStationID, this);
}

/*
**
*/
WriteXMPToDiskCache::~WriteXMPToDiskCache()
{
	ASL::StationUtils::RemoveListener(mStationID, this);
	ASL::StationRegistry::DisposeStation(mStationID);
}

/*
**
*/
bool WriteXMPToDiskCache::ContainsRegisterRequestID(dvacore::UTF16String inMediaPath, const BE::RequestIDs& requestIDs)
{
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);
	bool result(false);

	{
		ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

		PathRequestDataCache::iterator iter =  sPathRequestDataCache.find(mediaPath);
		if (iter != sPathRequestDataCache.end())
		{
			BOOST_FOREACH(ASL::SInt32 requestID, requestIDs)
			{
				result = 
					(std::find(iter->second.mWriteIDs.begin(), iter->second.mWriteIDs.end(), requestID) != iter->second.mWriteIDs.end());

				if (result)
				{
					break;
				}
			}
		}
	}

	return result;
}

/*
**
*/
void WriteXMPToDiskCache::OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String inMediaPath, const BE::RequestIDs& requestIDs)
{
	//	BUG 3529836, If the error directly come from MediaCore, we ignore its error report.
	if (ASL::ResultFailed(inResult) && ContainsRegisterRequestID(inMediaPath, requestIDs))
	{
		ASL::String errStr = dvacore::ZString("$$$/Prelude/Mezzanine/WriteXMPToDiskCache/WriteXMPToDiskFailed=Write XMP to media file: @0 failed");
		errStr = dvacore::utility::ReplaceInString(errStr, MZ::Utilities::NormalizePathWithoutUNC(inMediaPath));
		ML::SDKErrors::SetSDKErrorString(errStr);
	}

	BOOST_FOREACH(ASL::SInt32 requestID, requestIDs)
	{
		WriteXMPToDiskCache::UnregisterWriteRequest(inMediaPath, requestID, inResult);
	}
}


/*
**
*/
 bool WriteXMPToDiskCache::GetCachedXMPData(ASL::String const& inMediaPath, ASL::StdString& outXMPString)
{
	ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

	PathRequestDataCache::iterator iter =  sPathRequestDataCache.find(inMediaPath);
	if (iter != sPathRequestDataCache.end())
	{
		//	Means we haven't finished write temp XMP file yet.
		if(iter->second.mLastXMPTempFileRequest != NULL)
		{
			outXMPString = iter->second.mLastXMPTempFileRequest->GetXMPString();
			return true;
		}
		//	Means we have a temp file here.
		else
		{
			ASL::String mediaPath = inMediaPath;
			mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);
			ASL::String cachedXMPPath = GetCacheXMPFilePath(mediaPath);
			ASL::String storedMediaPath;
			return ReadXMPTempFile(cachedXMPPath, storedMediaPath, outXMPString);
		}
	}

	return false;
}

/*
** Rules for restore cached XMP
** 1. File is offline  -> delete temp file
** 2. File's content ID is different from temp file's content ID -> delete temp file
** 3. Temp file contains latest metadata -> replace metadata in media file.
** 4. Media file contains latest metadata ->Keep Media file's metadata as base, and append markers from temp file into media file.
**										    (if marker ID is same, keep the one in media file)
** 5. Read temp file failed, -> delete temp file
*/
void WriteXMPToDiskCache::RestoreCachedXMP()
{
	ASL::PathnameList tempXMPPathList;
	ASL::Directory clipDirectory = ASL::Directory::ConstructDirectory(GetSettingsFullPath(ASL::String()));
	clipDirectory.GetContainedFilePaths(kXMPCacheFileExt, tempXMPPathList, false);

	ASL::PathnameList::iterator iter = tempXMPPathList.begin();
	ASL::PathnameList::iterator end = tempXMPPathList.end();

	for (; iter != end; ++iter)
	{
		ASL::String mediaPath;
		ASL::StdString xmpContent;
		if (ReadXMPTempFile(*iter, mediaPath, xmpContent) && CombineXMPContent(mediaPath, xmpContent))
		{
			AssetMediaInfoPtr assetMediaInfo = AssetMediaInfo::CreateMasterClipMediaInfo(
														ASL::Guid::CreateUnique(),
														mediaPath,
														ASL::PathUtils::GetFilePart(mediaPath),
														PL::XMPText(new ASL::StdString(xmpContent)));

			if (!SRLibrarySupport::SaveXMPToDisk(assetMediaInfo, false))
			{
				ASL::File::Delete(*iter);
			}
		}
		else
		{
			ASL::File::Delete(*iter);
		}
	}
}

/*
**
*/
ASL::Result WriteXMPToDiskCache::RegisterWriting(
							ASL::String const& inMediaPath, 
							ASL::Guid const& inAssetID,
							ASL::StdString const& inXMPString,
							ASL::SInt32 inRequestID,
							bool writeCacheFile)
{
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	{
		ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

		if (sPathRequestDataCache[mediaPath].mLastXMPTempFileRequest)
		{
			sPathRequestDataCache[mediaPath].mLastXMPTempFileRequest->Abort();
		}
		sPathRequestDataCache[mediaPath].mWriteIDs.push_back(inRequestID);
		sPathRequestDataCache[mediaPath].mLastXMPTempFileRequest = SaveCacheXMPFileRequest::CreateClassRef();
		sPathRequestDataCache[mediaPath].mLastXMPTempFileRequest->Initialize(mediaPath, inAssetID, inXMPString, mStationID);
		if (writeCacheFile)
		{
			sPathRequestDataCache[mediaPath].mLastXMPTempFileRequest->Start();
		}
	}

	return ASL::kSuccess;
}

/*
**
*/
ASL::PathnameList WriteXMPToDiskCache::GetCachedXMPMediaPathList()
{
	ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

	PathRequestDataCache::const_iterator iter = sPathRequestDataCache.begin();
	PathRequestDataCache::const_iterator end = sPathRequestDataCache.end();

	ASL::PathnameList cachedMediaPathList;
	for (; iter != end; ++iter)
	{
		cachedMediaPathList.push_back(iter->first);
	}

	return cachedMediaPathList;
}

/*
**
*/
bool WriteXMPToDiskCache::ExistCache(ASL::String const& inMediaPath)
{
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	{
		ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

		// should NOT deternime according to whether physical file exist because physical file maybe exist for a while after deleted on Win.
		return sPathRequestDataCache.find(mediaPath) != sPathRequestDataCache.end();
	}
}

/*
**
*/
ASL::Result WriteXMPToDiskCache::UnregisterWriteRequest(
							ASL::String const& inMediaPath, 
							ASL::SInt32 inRequestID,
							ASL::Result inResult)
{
	ASL::Guid assetID;
	ASL::String mediaPath = inMediaPath;
	mediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

	{
		ASL::CriticalSectionLock lock(sRequestCacheCriticalSection);

		PathRequestDataCache::iterator iter =  sPathRequestDataCache.find(mediaPath);
		if (iter != sPathRequestDataCache.end())
		{
			WriteIDs::iterator IDIter = iter->second.mWriteIDs.begin();
			WriteIDs::iterator IDEnd = iter->second.mWriteIDs.end();
			bool exist(false);
			for (; IDIter != IDEnd; ++IDIter)
			{
				if (inRequestID == *IDIter)
				{
					++IDIter;
					exist = true;
					break;
				}
			}

			if (exist)
			{
				assetID = iter->second.mLastXMPTempFileRequest->GetAssetID();
				if (IDIter == IDEnd)
				{
					//	Abort the request anyway
					if (iter->second.mLastXMPTempFileRequest)
					{
						iter->second.mLastXMPTempFileRequest->Abort();
					}

					sPathRequestDataCache.erase(iter);

					if (SRProject::GetInstance())
					{
						SRProject::GetInstance()->RemoveUnreferenceResources();
					}
				}
				else
				{
					iter->second.mWriteIDs.erase(iter->second.mWriteIDs.begin(), IDIter);
				}
			}

		}
	}

	SRLibrarySupport::XMPFileTimeChanged(assetID, mediaPath, ASL::ResultSucceeded(inResult) ? ASL::kSuccess : ASL::ResultFlags::kResultTypeFailure);
	
	return ASL::kSuccess;
}

}
