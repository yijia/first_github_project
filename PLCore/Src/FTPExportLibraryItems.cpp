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

//	ASL
#include "ASLAtomic.h"
#include "ASLBatchOperation.h"
#include "ASLClass.h"
#include "ASLClassFactory.h"
#include "ASLPathUtils.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLStringUtils.h"
#include "ASLThreadedWorkQueue.h"
#include "ASLSleep.h"
#include "ASLFile.h"
#include "ASLListener.h"
#include "ASLMessageMap.h"
#include "ASLResults.h"
#include "ASLContract.h"

//	BE
#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProgress.h"
#include "BEProject.h"
#include "BEResults.h"

// local
#include "PLExportLibraryItems.h"
#include "MZProgressImpl.h"
#include "PLFTPSettingsData.h"
//#include "IngestMedia/PLIngestUtils.h"
#include "PLFTPUtils.h"
#include "ExportUtils.h"
#include "MZUtilities.h"

// dvacore
#include "dvacore/config/Localizer.h"
#include "dvacore/threads/SharedThreads.h"
#include "dvacore/utility/StringUtils.h"

// UIF
#include "UIFMessageBox.h"

namespace PL
{


namespace
{
	ASL::StdString NormalizeServerName(ASL::String const& inServer)
	{
		return (inServer.substr(0, 6) != ASL_STR("ftp://"))
			? ASL::MakeStdString(ASL_STR("ftp://") + inServer)
			: ASL::MakeStdString(inServer);
	}

	ASL::String BuildUnqiueDestinationPathForFTP(
		ASL::String const& inMediaPath,
		ASL::String const& inDestRoot,
		PL::ExportUtils::ExportItem const& inExportItem)
	{
		ASL::String destPath = PL::ExportUtils::BuildUnqiueDestinationPath(inMediaPath, inDestRoot, inExportItem);
		std::replace(destPath.begin(), destPath.end(), '\\', '/');
		return destPath;
	}

	ASL::String BuildProjectPath(ASL::String const& inDestinationFolder, ASL::String const& inProjectFile)
	{
		ASL::String	projectFile = inDestinationFolder + inProjectFile;
		std::replace(projectFile.begin(), projectFile.end(), '\\', '/');
		return projectFile;
	}

	ASL_DECLARE_MESSAGE_WITH_0_PARAM(FTPConvertToFinalProjectMessage);

	// This class implements the operation of doing export media to new location on local disk.
	ASL_DEFINE_CLASSREF(FTPExportLibraryItemsOperation, PL::IExportLibraryItemsOperation);

	class FTPExportLibraryItemsOperation :
		public IExportLibraryItemsOperation,
		public ASL::IBatchOperation,
		public ASL::IThreadedQueueRequest,
		public MZ::ProgressImpl,
		public ASL::Listener
	{
		ASL_BASIC_CLASS_SUPPORT(FTPExportLibraryItemsOperation);
		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(IExportLibraryItemsOperation)
			ASL_QUERY_ENTRY(ASL::IBatchOperation)
			ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
			ASL_QUERY_CHAIN(MZ::ProgressImpl)
			ASL_QUERY_MAP_END

			ASL_MESSAGE_MAP_BEGIN(FTPExportLibraryItemsOperation)
			ASL_MESSAGE_HANDLER(FTPConvertToFinalProjectMessage, OnConvertToFinalProject)
			ASL_MESSAGE_MAP_END
	public:

		static FTPExportLibraryItemsOperationRef Create(
			PL::FTPSettingsData::SharedPtr inFTPSettingsDataPtr,
			ML::IProjectConverterModuleRef inConverterModule,
			ASL::String const& inDestinationFolder,
			ASL::String const& inProjectFile)
		{
			FTPExportLibraryItemsOperationRef operation(CreateClassRef());
			operation->mFTPSettingsDataPtr = inFTPSettingsDataPtr;
			operation->mConverterModule = inConverterModule;
			operation->mDestinationFolder = inDestinationFolder;
			operation->mProjectFile = inProjectFile;
			return operation;
		}

		FTPExportLibraryItemsOperation() :
		mResult(BE::kSuccess),
			mStationID(ASL::StationRegistry::RegisterUniqueStation()),
			mThreadDone(0),
			mAbort(0),
			mInProcess(0),
			mConfirmedAbort(false)
		{
			ASL::StationUtils::AddListener(mStationID, this);
		}

		virtual ~FTPExportLibraryItemsOperation()
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

		virtual void SetResult(ASL::Result inResult)
		{
			mResult = inResult;
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
			mThreadedWorkQueue = dvacore::threads::CreateIOBoundExecutor();
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
				ASL::Sleep(0);
			}
		}

		virtual ASL::String GetFullProjectPath() const
		{
			ASL::String const& remoteDir = mFTPSettingsDataPtr->GetUserName() + DVA_STR("@") +
					ASL::PathUtils::RemoveTrailingSlash(mFTPSettingsDataPtr->GetServerName()) + 
					MZ::Utilities::AddHeadingSlash(mFTPSettingsDataPtr->GetRemoteDirectory());
			return ASL::PathUtils::AddTrailingSlash(remoteDir) + mProjectFile;
		}

		virtual ASL::String GetExportErrorMessage() const;
		virtual ASL::String GetExportWarningMessage() const;

	private:
		ASL::Result BuildExportData()
		{
			mResult = PL::ExportUtils::BuildUniqueExportFilesMap(mExportMediaFiles, mMediaPathToExportItems,
																boost::bind(&FTPExportLibraryItemsOperation::UpdateProgress, this, _1));
			return ASL::ResultSucceeded(mResult) ? PL::ExportUtils::UniquneSameSourceNamePathsWithPrompt(mMediaPathToExportItems) : mResult;
		}

		void Process();

		void SetExportInfo(
			ASL::String const& inSeedPremiereProjectFile,
			std::set<ASL::String> const& inExportMediaFiles)
		{
			mSeedPremiereProjectFile = inSeedPremiereProjectFile;
			mExportMediaFiles = inExportMediaFiles;
		}

		virtual void SetResultList(ActionResultList const& inActionResultList)
		{
			mActionResultList = inActionResultList;
		}

		void CheckFilesOnDestPath(ASL::String& outExistFiles) const;

		virtual ASL::Result PrecheckExportFiles() const
		{
			ASL::String existFiles;
			CheckFilesOnDestPath(existFiles);

			if (!existFiles.empty())
			{
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/DestFileExist=The following files already exist at the selected destination. To overwrite them select YES. To cancel the copy operation select NO.\n @0");
				message = dvacore::utility::ReplaceInString(message, existFiles);
				if (UIF::MBResult::kMBResultYes != MZ::Utilities::PromptUIFMessageBoxFromMainThread(
					message, 
					dvacore::ZString("$$$/Prelude/Libraries/Mezzanine/ProjectConversionWarn=Export Warning"), 
					UIF::MBFlag::kMBFlagYesNo, 
					UIF::MBIcon::kMBIconWarning))
				{
					return ASL::eUserCanceled;
				}
			}
			return ASL::kSuccess;
		}

		void OnConvertToFinalProject();

		ASL::Result ExportMediaFiles();

	private:
		ASL::String							mTitleString;
		ASL::Result							mResult;
		ASL::StationID						mStationID;
		dvacore::threads::AsyncExecutorPtr	mThreadedWorkQueue;
		volatile ASL::AtomicInt				mThreadDone;
		volatile ASL::AtomicInt				mAbort;
		volatile ASL::AtomicInt				mInProcess;
		bool								mConfirmedAbort;

		ASL::String							mSeedPremiereProjectFile;
		ML::IProjectConverterModuleRef		mConverterModule;

		ASL::String							mProjectFile;
		ASL::String							mDestinationFolder;

		PL::FTPSettingsData::SharedPtr		mFTPSettingsDataPtr;
		std::set<ASL::String>				mExportMediaFiles;
		std::vector<ASL::String>			mExportErrors;
		std::vector<ASL::String>			mExportWarnings;

		ActionResultList					mActionResultList;
		PL::ExportUtils::MeidaPathToExportItemMap mMediaPathToExportItems;

		FTPUtils::FTPConnectionDataPtr		mFTPConnection;
	};

}

namespace FTPInternalUtils
{
bool CreateFTPConnection(FTPUtils::FTPConnectionDataPtr& outFTPConnection, PL::FTPSettingsData::SharedPtr const& inSettingsData)
{
	return FTPUtils::CreateFTPConnection(
						outFTPConnection,
						inSettingsData->GetServerName(),
						inSettingsData->GetServerPort(),
						inSettingsData->GetRemoteDirectory(),
						inSettingsData->GetUserName(),
						inSettingsData->GetPassword());
}
} // namespace FTPInternalUtils


namespace {

ASL::String FTPExportLibraryItemsOperation::GetExportErrorMessage() const
{
	ASL::String errorMessage;
	for (std::vector<ASL::String>::const_iterator f = mExportErrors.begin(), l = mExportErrors.end(); f != l; ++f)
	{
		errorMessage += *f + ASL_STR("\n");
	}
	return errorMessage;
}

ASL::String FTPExportLibraryItemsOperation::GetExportWarningMessage() const
{
	ASL::UInt32 warningMsgCount = 0;
	ASL::String warnMessage;
	for (std::vector<ASL::String>::const_iterator f = mExportWarnings.begin(), l = mExportWarnings.end(); f != l; ++f)
	{
		if (++warningMsgCount > PL::ExportUtils::nMaxWarningMessageNum)
		{
			ASL::String tmpLen = ASL::CoerceAsString::Result(mExportWarnings.size());
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/GetExportWarningMessage=There are @0 export warnings in total, not all of them are displayed.\n");
			message = dvacore::utility::ReplaceInString(message, tmpLen);

			warnMessage += message;
			break;
		}
		warnMessage += *f + ASL_STR("\n");
	}
	return warnMessage;
}

void FTPExportLibraryItemsOperation::Process()
{
	ASL::UInt32 exportProject = mProjectFile.empty() ? 0 : 1;

	StartProgress(1, exportProject + 2*mExportMediaFiles.size());

	do 
	{
		if (!FTPInternalUtils::CreateFTPConnection(mFTPConnection, mFTPSettingsDataPtr))
		{
			mResult = ASL::eAccessIsDenied;
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Failure/CanNotOpenFTPConncetion=Cannot connect to FTP server.");
			mExportErrors.push_back(message);

			break;
		}

		ASL::String descriptionMsg = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/FTPExportPreparingTitle=Preparing media info");
		ASL::StationUtils::PostMessageToUIThread(mStationID, ASL::UpdateOperationDescriptionMessage(descriptionMsg), true);
		mResult = BuildExportData();
		if (!ASL::ResultSucceeded(mResult))
		{
			break;
		}

		mResult = PrecheckExportFiles();
		if (!ASL::ResultSucceeded(mResult))
		{
			break;
		}

		descriptionMsg = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/FTPExportingTitle=Export");
		ASL::StationUtils::PostMessageToUIThread(mStationID, ASL::UpdateOperationDescriptionMessage(descriptionMsg), true);
		mResult = ExportMediaFiles();

		if (GetAbort())
		{
			mResult = ASL::eUserCanceled;
		}
		else if (mResult == ASL::kSuccess)
		{
			if (!mProjectFile.empty())
			{
				ASL::AtomicExchange(mInProcess, 1);
				ASL::StationUtils::PostMessageToUIThread(mStationID, FTPConvertToFinalProjectMessage());

				while (ASL::AtomicRead(mInProcess) != 0)
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
	} while (0);

	//	All done.
	ASL::StationUtils::PostMessageToUIThread(mStationID, ASL::OperationCompleteMessage());

	ASL::AtomicCompareAndSet(mThreadDone, 0, 1);

	EndProgress();

	FTPUtils::CloseFTPConncection(mFTPConnection);
}

void FTPExportLibraryItemsOperation::CheckFilesOnDestPath(ASL::String& outExistFiles) const
{
	outExistFiles.clear();

	// Check project file
	ASL::String	tmpProjFile = BuildProjectPath(mDestinationFolder, mProjectFile);
	if (!mProjectFile.empty() && FTPUtils::FTPExistOnDisk(mFTPConnection, tmpProjFile, false))
	{
		outExistFiles += ASL_STR("\n") + mFTPSettingsDataPtr->GetServerName() + ASL_STR("/") + tmpProjFile;
	}

	// Check media
	ASL::String const& destMediaDir =  mDestinationFolder +  ASL_STR("media");
	ASL::UInt32 warningMsgCount = 0;
	for (PL::ExportUtils::MeidaPathToExportItemMap::const_iterator iter = mMediaPathToExportItems.begin();
		iter != mMediaPathToExportItems.end();
		++iter)
	{
		PL::ExportUtils::ExportMediaPath const& mediaPath = iter->first; 
		PL::ExportUtils::ExportItem const& exportItem = iter->second;

		ASL::PathnameList const& exportFilePaths = exportItem.exportFilePaths;
		ASL::PathnameList::const_iterator f = exportFilePaths.begin();
		ASL::PathnameList::const_iterator f_end = exportFilePaths.end();
		for ( ; f != f_end; f++)
		{
			ASL::String itemDest;
#if ASL_TARGET_OS_WIN
			ASL::String const& itemPath = ASL::PathUtils::Win::StripUNC(ASL::PathUtils::ToNormalizedPath( ASL::PathUtils::RemoveTrailingSlash(*f) ));
#else
			ASL::String const& itemPath = ASL::PathUtils::RemoveTrailingSlash(*f);
#endif
			ASL_ASSERT(!itemPath.empty());
			if (itemPath.empty())
				continue;

			ASL::String destPath = BuildUnqiueDestinationPathForFTP(itemPath, destMediaDir, exportItem);
		
			if (!ASL::PathUtils::IsDirectory(itemPath))
			{
				if (FTPUtils::FTPExistOnDisk(mFTPConnection, destPath, false))
				{
					if (++warningMsgCount <= PL::ExportUtils::nMaxWarningMessageNum)
					{
						outExistFiles += ASL_STR("\n") + mFTPSettingsDataPtr->GetServerName() + ASL_STR("/") + destPath;
					}
				}
			}
		}
	}
	if (warningMsgCount > PL::ExportUtils::nMaxWarningMessageNum)
	{
		ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/CheckFilesOnDestPath=\nThere are @0 duplicated media files, not all of them are displayed.\n");
		ASL::String tmpLen = ASL::CoerceAsString::Result(warningMsgCount);
		message = dvacore::utility::ReplaceInString(message, tmpLen);

		outExistFiles += message;
	}
}

ASL::Result FTPExportLibraryItemsOperation::ExportMediaFiles()
{
	PL::ExportUtils::PathPairVector pathPairs;

	ASL::String const& remoteDir =  mDestinationFolder +  ASL_STR("media");
	ASL::String const& remoteRoot = mFTPSettingsDataPtr->GetServerName() + ASL_STR("/");
	
	// Export every media
	for (PL::ExportUtils::MeidaPathToExportItemMap::iterator iter = mMediaPathToExportItems.begin();
		iter != mMediaPathToExportItems.end();
		++iter)
	{	
		PL::ExportUtils::ExportMediaPath const& mediaPath = iter->first; 
		PL::ExportUtils::ExportItem& exportItem = iter->second;

		if (!ASL::PathUtils::ExistsOnDisk(mediaPath))
		{
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/MediaNoExist=Cannot find \"@0\".");
			message = dvacore::utility::ReplaceInString(message, mediaPath);
			mExportWarnings.push_back(message);

			UpdateProgress(1);
			continue;
		}

		ASL::PathnameList const& exportFilePaths = exportItem.exportFilePaths;	
		if (exportFilePaths.empty())
		{
			DVA_ASSERT_MSG(0, "A deleted file is recovered during exporting!");
			UpdateProgress(1);
			continue;
		}

		StartProgress(1, exportFilePaths.size());

		for (ASL::PathnameList::const_iterator bf = exportFilePaths.begin(), bl = exportFilePaths.end(); bf != bl; ++bf)
		{
#if ASL_TARGET_OS_WIN
			ASL::String const& itemPath = ASL::PathUtils::Win::StripUNC(ASL::PathUtils::ToNormalizedPath( ASL::PathUtils::RemoveTrailingSlash(*bf) ));
#else
			ASL::String const& itemPath = ASL::PathUtils::RemoveTrailingSlash(*bf);
#endif
			ASL_ASSERT(!itemPath.empty());
			if (itemPath.empty())
				continue;

			PL::ExportUtils::ExportProgressData progressData;
			progressData.UpdateProgress = boost::bind(&FTPExportLibraryItemsOperation::UpdateProgress, this, _1);
			progressData.totalProgress = 100;
			progressData.currentProgress = 0;
			StartProgress(1, progressData.totalProgress);

			ASL::String destPath = BuildUnqiueDestinationPathForFTP(itemPath, remoteDir, exportItem);
			
			bool success = true;
			if (ASL::PathUtils::IsDirectory(itemPath))
			{
				ASL::String const& remoteDestDir = destPath;
				bool exist = FTPUtils::FTPExistOnDisk(mFTPConnection, remoteDestDir, true);
				if (!exist)
				{
					success = FTPUtils::FTPMakeDir(mFTPConnection, remoteDestDir);
					if (!success)
					{
						ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Failure/CanNotMakeDir=Cannot make remote directory \"@0\".");
						message = dvacore::utility::ReplaceInString(message, remoteDestDir);
						mExportErrors.push_back(message);
					}
				}
			}
			else
			{
				if (itemPath == *bf)
				{
					pathPairs.push_back(PL::ExportUtils::PathPair(*bf, destPath));
				}

				ASL::String remoteDestFile = mFTPSettingsDataPtr->GetServerName() + ASL_STR("/") + destPath;

				if (FTPUtils::FTPExistOnDisk(mFTPConnection, destPath, false))
				{								
					ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/CopyOverwrite=Overwrite \"@0\" from \"@1\".");
					message = dvacore::utility::ReplaceInString(message, remoteDestFile, itemPath);
					mExportWarnings.push_back(message);				
				}

				success = FTPUtils::FTPUpload(mFTPConnection, destPath, itemPath, &progressData);
				if (!success)
				{
					ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Failure/CanNotUploadFile=Cannot upload file \"@0\" to \"@1\".");
					message = dvacore::utility::ReplaceInString(message, itemPath, remoteDestFile);
					mExportErrors.push_back(message);
				}
			}

			EndProgress();

			if (UpdateProgress(0))
			{
				return ASL::eUserCanceled;
			}

			if (!success)
			{
				return ASL::eAccessIsDenied;
			}
		}

		// The clip is exported.
		EndProgress();

		// Todo: not affect to Roughcut for mediaPath of Roughcut is changed to its media path.
		for (ActionResultList::iterator resultit=mActionResultList.begin(); resultit!=mActionResultList.end(); ++resultit)
		{
			if ((*resultit)->mFilePath == mediaPath)
				(*resultit)->mResult = ASL::kSuccess;
		}
	}

	//Modify the paths of media in project file
	if (!mProjectFile.empty() && pathPairs.size() > 0)
	{
		PL::ExportUtils::ReplaceMeidaPath(mSeedPremiereProjectFile, mDestinationFolder, pathPairs);
	}

	// Todo: now just change to success.
	for (ActionResultList::iterator resultit=mActionResultList.begin(); resultit!=mActionResultList.end(); ++resultit)
	{
		(*resultit)->mResult = ASL::kSuccess;
	}
	return ASL::kSuccess;
}

void FTPExportLibraryItemsOperation::OnConvertToFinalProject()
{
	ASL::String localProjectFile;
	ASL::String drive, directory, file, ext;
	ASL::PathUtils::SplitPath(mProjectFile, &drive, &directory, &file, &ext);

	if (mConverterModule == NULL)
	{
		localProjectFile = mSeedPremiereProjectFile;
	}
	else
	{
		ASL::String const& tmpName = ASL::PathUtils::AddTrailingSlash(ASL::PathUtils::GetTempDirectory()) + file;
		localProjectFile = ASL::PathUtils::MakeUniqueFilename(tmpName) + ext;

		ASL::String pluginErrorMessage;
		mResult = MZ::ProjectConverters::SaveConvertProject(mConverterModule, localProjectFile, mSeedPremiereProjectFile, pluginErrorMessage);
		if (mResult != ASL::kSuccess)
			mExportErrors.push_back(pluginErrorMessage);
	}

	ASL::String	tmpProjFile = BuildProjectPath(mDestinationFolder, mProjectFile);
	ASL::String const& remoteFile = mFTPSettingsDataPtr->GetServerName() + 
									ASL_STR("/") + 
									tmpProjFile;

	if (FTPUtils::FTPExistOnDisk(mFTPConnection, tmpProjFile, false))
	{								
		ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Warning/ProjCopyOverwrite=Overwrite \"@0\".");
		message = dvacore::utility::ReplaceInString(message, remoteFile);
		mExportWarnings.push_back(message);				
	}

	if (!FTPUtils::FTPUpload(mFTPConnection, tmpProjFile, localProjectFile, NULL))
	{
		ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportFTP/Failure/CanNotUploadProject=Cannot upload project file \"@0\" to \"@1\".");
		message = dvacore::utility::ReplaceInString(message, localProjectFile, remoteFile);
		mExportErrors.push_back(message);
		mResult = ASL::eAccessIsDenied;
	}
	
	if (localProjectFile != mSeedPremiereProjectFile)
	{
		ASL::File::Delete(localProjectFile);
	}

	ASL::AtomicExchange(mInProcess, 0);
}

} // private namespace


IExportLibraryItemsOperationRef CreateFTPExportLibraryItemsOperation(
	ML::IProjectConverterModuleRef inConverterModule,
	ASL::String const& inDestinationFolder,
	ASL::String const& inProjectFile,
	PL::FTPSettingsData::SharedPtr inFTPSettingsDataPtr)
{
	return IExportLibraryItemsOperationRef(FTPExportLibraryItemsOperation::Create(inFTPSettingsDataPtr, 
																					inConverterModule,
																					inDestinationFolder,
																					inProjectFile));
}

} // namespace PL
