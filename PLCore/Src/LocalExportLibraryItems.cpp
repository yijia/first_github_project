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
#include "ASLDiskUtils.h"

//	BE
#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProgress.h"
#include "BEProject.h"
#include "BEResults.h"

// local
#include "PLExportLibraryItems.h"
#include "MZProgressImpl.h"
#include "IngestMedia/PLIngestUtils.h"
#include "ExportUtils.h"
#include "MZUtilities.h"

// dvacore
#include "dvacore/config/Localizer.h"
#include "dvacore/threads/SharedThreads.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/utility/FileUtils.h"

// UIF
#include "UIFMessageBox.h"

namespace PL
{
namespace {

bool progress_callback(ASL::Float32 inPercentDone, void* inProgressData);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(ConvertToFinalProjectMessage);

// This class implements the operation of doing export media to new location on local disk.
ASL_DEFINE_CLASSREF(LocalExportLibraryItemsOperation, PL::IExportLibraryItemsOperation);

class LocalExportLibraryItemsOperation :
	public PL::IExportLibraryItemsOperation,
	public ASL::IBatchOperation,
	public ASL::IThreadedQueueRequest,
	public MZ::ProgressImpl,
	public ASL::Listener
{
	ASL_BASIC_CLASS_SUPPORT(LocalExportLibraryItemsOperation);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(PL::IExportLibraryItemsOperation)
		ASL_QUERY_ENTRY(ASL::IBatchOperation)
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_CHAIN(MZ::ProgressImpl)
	ASL_QUERY_MAP_END

	ASL_MESSAGE_MAP_BEGIN(LocalExportLibraryItemsOperation)
		ASL_MESSAGE_HANDLER(ConvertToFinalProjectMessage, OnConvertToFinalProject)
	ASL_MESSAGE_MAP_END
public:
	static LocalExportLibraryItemsOperationRef Create(
		ML::IProjectConverterModuleRef inConverterModule, 
		ASL::String const& inDestinationFolder,
		ASL::String const& inProjectFile)
	{
		LocalExportLibraryItemsOperationRef operation(CreateClassRef());
		operation->mConverterModule = inConverterModule;
		operation->mProjectFile = inProjectFile;
		operation->mDestinationFolder = inDestinationFolder;
		return operation;
	}

	LocalExportLibraryItemsOperation() :
		mResult(BE::kSuccess),
		mStationID(ASL::StationRegistry::RegisterUniqueStation()),
		mThreadDone(0),
		mAbort(0),
		mInProcess(0),
		mConfirmedAbort(false)
	{
		ASL::StationUtils::AddListener(mStationID, this);
	}

	virtual ~LocalExportLibraryItemsOperation()
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
			ASL::Sleep(1);
		}
	}

	virtual ASL::String GetExportErrorMessage() const
	{
		ASL::String errorMessage;
		for (std::vector<ASL::String>::const_iterator f = mExportErrors.begin(), l = mExportErrors.end(); f != l; ++f)
		{
			errorMessage += *f + ASL_STR("\n");
		}
		return errorMessage;
	}
	
	virtual ASL::String GetExportWarningMessage() const
	{
		ASL::UInt32 warningMsgCount = 0;
		ASL::String warnMessage;
		for (std::vector<ASL::String>::const_iterator f = mExportWarnings.begin(), l = mExportWarnings.end(); f != l; ++f)
		{
			if (++warningMsgCount > PL::ExportUtils::nMaxWarningMessageNum)
			{
				ASL::String tmpLen = ASL::CoerceAsString::Result(mExportWarnings.size());
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/GetExportWarningMessage=There are @0 export warnings in total, not all of them are displayed.\n");
				message = dvacore::utility::ReplaceInString(message, tmpLen);

				warnMessage += message;
				break;
			}
			warnMessage += *f + ASL_STR("\n");
		}
		return warnMessage;
	}

	virtual ASL::String GetFullProjectPath() const
	{
		return mProjectFile;
	}

private:
	ASL::Result BuildExportData()
	{
		mResult = PL::ExportUtils::BuildUniqueExportFilesMap(	mExportMediaFiles, 
																mMediaPathToExportItems,
																boost::bind(&LocalExportLibraryItemsOperation::UpdateProgress, this, _1));
		return ASL::ResultSucceeded(mResult) ? PL::ExportUtils::UniquneSameSourceNamePathsWithPrompt(mMediaPathToExportItems) : mResult;
	}

	void Process()
	{
		ASL::UInt32 exportProject = mProjectFile.empty() ? 0 : 1;

		StartProgress(1, exportProject + 2*mExportMediaFiles.size());
		
		do 
		{
			ASL::String descriptionMsg = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/LocalExportPreparingTitle=Preparing media info");
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

			descriptionMsg = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/LocalExportingTitle=Export");
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
					ASL::StationUtils::PostMessageToUIThread(mStationID, ConvertToFinalProjectMessage());

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

		EndProgress();

		//	All done.
		ASL::StationUtils::PostMessageToUIThread(mStationID, ASL::OperationCompleteMessage());

		ASL::AtomicCompareAndSet(mThreadDone, 0, 1);
	}

	virtual void SetExportInfo(
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

	void OnConvertToFinalProject()
	{
		if (ASL::PathUtils::ExistsOnDisk(mProjectFile))
		{
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/ProjCopyOverwrite=Overwrite \"@0\".");
			message = dvacore::utility::ReplaceInString(message, mProjectFile);
			mExportWarnings.push_back(message);
		}

		if (!ASL::PathUtils::ExistsOnDisk(mDestinationFolder))
		{
			ASL::Directory::CreateOnDisk(mDestinationFolder, true);
		}

		if (mConverterModule == NULL)
		{
#if ASL_TARGET_OS_MAC
			ASL::Result delResult = ASL::kSuccess;
			if (ASL::PathUtils::ExistsOnDisk(mProjectFile))
			{
				delResult = ASL::File::Delete(mProjectFile);
			}
			if (ASL::ResultSucceeded(delResult))
			{
				mResult = ASL::File::Copy(mSeedPremiereProjectFile, mProjectFile);
			}
			else
			{
				DVA_ASSERT_MSG(0, "Delete original project file failed.");
			}

			if (ASL::ResultFailed(delResult) || ASL::ResultFailed(mResult))
			{
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Failure/CanNotCopyProject=Cannot copy project file \"@0\" to \"@1\".");
				message = dvacore::utility::ReplaceInString(message, mSeedPremiereProjectFile, mProjectFile);
				mExportErrors.push_back(message);
			}
#else
			mResult = ASL::File::Copy(mSeedPremiereProjectFile, mProjectFile);
			if (!ASL::ResultSucceeded(mResult))
			{
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Failure/CanNotCopyProject=Cannot copy project file \"@0\" to \"@1\".");
				message = dvacore::utility::ReplaceInString(message, mSeedPremiereProjectFile, mProjectFile);
				mExportErrors.push_back(message);
			}
#endif
		}
		else
		{
			ASL::String pluginErrorMessage;
			mResult = MZ::ProjectConverters::SaveConvertProject(mConverterModule, mProjectFile, mSeedPremiereProjectFile, pluginErrorMessage);
			if (mResult != ASL::kSuccess)
				mExportErrors.push_back(pluginErrorMessage);

			// bug #3135228, if having no write permission to the dest folder, SaveConvertProject() will not return fails.
			if (!ASL::PathUtils::ExistsOnDisk(mProjectFile))
			{
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Failure/CanNotCreateProject=Cannot create project file \"@0\".");
				message = dvacore::utility::ReplaceInString(message, mProjectFile);
				mExportErrors.push_back(message);
				mResult = ASL::eAccessIsDenied;
			}
		}

		ASL::AtomicExchange(mInProcess, 0);
	}

	bool IsEnoughDiskSpaceForExport() const
	{
		ASL::UInt64 srcTotalSize = 0;
		ASL::UInt64 curFileSize = 0;
		for (PL::ExportUtils::MeidaPathToExportItemMap::const_iterator iter = mMediaPathToExportItems.begin();
			iter != mMediaPathToExportItems.end();
			++iter)
		{
			PL::ExportUtils::ExportItem const& exportItem = iter->second;
			ASL::PathnameList const& exportFilePaths = exportItem.exportFilePaths;

			ASL::PathnameList::const_iterator f = exportFilePaths.begin();
			ASL::PathnameList::const_iterator f_end = exportFilePaths.end();
			for ( ; f != f_end; f++)
			{
				// ASL::File::SizeOnDisk won't modify curFileSize if no file exists
				curFileSize = 0;
				ASL::File::SizeOnDisk(*f, curFileSize);
				srcTotalSize += curFileSize;
			}
		}
		ASL::File::SizeOnDisk(mSeedPremiereProjectFile, curFileSize);
		srcTotalSize += curFileSize;
		ASL::UInt64 destFreeSpace = ASL::DiskUtils::GetFreeSpaceFromPath(mDestinationFolder);

		if (destFreeSpace <= srcTotalSize)
		{
			MZ::Utilities::PromptUIFMessageBoxFromMainThread(
				dvacore::ZString("$$$/Prelude/Libraries/Mezzanine/ProjectConverterFailureDiskSpace=Unable to export the selected project items, there is not enough space available on the destination disk.\n"),
				dvacore::ZString("$$$/Prelude/Libraries/Mezzanine/ProjectConversionError=Export Error"),
				UIF::MBFlag::kMBFlagOK,
				UIF::MBIcon::kMBIconError);

			return false;
		}

		return true;
	}

	void CheckFilesOnDestPath(ASL::String& outExistFiles) const
	{
		// Check project file
		if (!mProjectFile.empty() && ASL::PathUtils::ExistsOnDisk(mProjectFile))
		{
			outExistFiles += ASL_STR("\n") + mProjectFile;
		}

		// Check media files
		ASL::String const& destFolder = ASL::PathUtils::AddTrailingSlash(mDestinationFolder);
		ASL::String const& destRoot = destFolder + ASL_STR("media");

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
				ASL::String const& itemPath = MZ::Utilities::NormalizePathWithoutUNC(ASL::PathUtils::RemoveTrailingSlash(*f));
				ASL_ASSERT(!itemPath.empty());
				if (itemPath.empty())
					continue;

				ASL::String destPath = PL::ExportUtils::BuildUnqiueDestinationPath(itemPath, destRoot, exportItem);
				if (!ASL::PathUtils::IsDirectory(itemPath))
				{
					if (ASL::PathUtils::ExistsOnDisk(destPath))
					{
						if (++warningMsgCount <= PL::ExportUtils::nMaxWarningMessageNum)
						{
							outExistFiles += ASL_STR("\n") + destPath;
						}
					}
				}
			}
		}
		if (warningMsgCount > PL::ExportUtils::nMaxWarningMessageNum)
		{
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/CheckFilesOnDestPath=\nThere are @0 duplicated media files, not all of them are displayed.\n");
			ASL::String tmpLen = ASL::CoerceAsString::Result(warningMsgCount);
			message = dvacore::utility::ReplaceInString(message, tmpLen);

			outExistFiles += message;
		}
	}
	
	virtual ASL::Result PrecheckExportFiles() const
	{
		if (!IsEnoughDiskSpaceForExport())
		{
			return ASL::eUserCanceled;
		}

		ASL::String existFiles;
		CheckFilesOnDestPath(existFiles);
		if (!existFiles.empty())
		{
			ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/DestFileExist=The following files already exist at the selected destination. To overwrite them select YES. To cancel the copy operation select NO.\n @0");
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

	ASL::Result ExportMediaFiles()
	{
		PL::ExportUtils::PathPairVector pathPairs;

		mDestinationFolder = ASL::PathUtils::AddTrailingSlash(mDestinationFolder);
		ASL::String destRoot = mDestinationFolder + ASL_STR("media");

		for (PL::ExportUtils::MeidaPathToExportItemMap::iterator iter = mMediaPathToExportItems.begin();
			iter != mMediaPathToExportItems.end();
			++iter)
		{	
			PL::ExportUtils::ExportMediaPath const& mediaPath = iter->first; 
			PL::ExportUtils::ExportItem& exportItem = iter->second;

			ASL::String const& normalizedMediaPath = MZ::Utilities::NormalizePathWithoutUNC(mediaPath);

			if (!ASL::PathUtils::ExistsOnDisk(normalizedMediaPath))
			{
				ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/MediaNoExist=Cannot find \"@0\".");
				message = dvacore::utility::ReplaceInString(message, normalizedMediaPath);
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
				ASL::String const& normalizedItemPath = MZ::Utilities::NormalizePathWithoutUNC(ASL::PathUtils::RemoveTrailingSlash(*bf));
				ASL_ASSERT(!normalizedItemPath.empty());
				if (normalizedItemPath.empty())
					continue;

				// For every jobItem, assume the progress has 100 steps to do.
				PL::ExportUtils::ExportProgressData progressData;
				progressData.UpdateProgress = boost::bind(&LocalExportLibraryItemsOperation::UpdateProgress, this, _1);
				progressData.totalProgress = 100;
				progressData.currentProgress = 0;
				StartProgress(1, progressData.totalProgress);
				
				ASL::String destPath = PL::ExportUtils::BuildUnqiueDestinationPath(normalizedItemPath, destRoot, exportItem);

				ASL::Result copyResult = ASL::kSuccess;
				if (ASL::PathUtils::IsDirectory(normalizedItemPath))
				{
					if (!ASL::PathUtils::ExistsOnDisk(destPath))
					{
						dvacore::utility::FileUtils::EnsureDirectoryExists(ASL::PathUtils::GetFullDirectoryPart(destPath));
						copyResult = ASL::Directory::CreateOnDisk(destPath, true);
						if (copyResult != ASL::kSuccess)
						{
							ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Failure/CanNotMakeDir=Cannot make directory \"@0\".");
							message = dvacore::utility::ReplaceInString(message, destPath);
							mExportErrors.push_back(message);
						}
					}
				}
				else
				{
					// To fix bug #3068226, some providers don't provide normalized media path, but the dest media path should be put into pathPairs,
					// in this case, we should compare the media path with the normalized item path from provider
					//if (itemPath == *bf)
					//{
					//	pathPairs.push_back(PL::ExportUtils::PathPair(*bf, destPath));
					//}
					if (normalizedItemPath == normalizedMediaPath)
					{
						pathPairs.push_back(PL::ExportUtils::PathPair(normalizedItemPath, destPath));
					}

					if (ASL::PathUtils::ExistsOnDisk(destPath))
					{
						// Add warning for overriding
						ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Warning/CopyOverwrite=Overwrite \"@0\" from \"@1\".");
						message = dvacore::utility::ReplaceInString(message, destPath, normalizedItemPath);
						mExportWarnings.push_back(message);
					}
					else
					{
						dvacore::utility::FileUtils::EnsureDirectoryExists(ASL::PathUtils::GetFullDirectoryPart(destPath));
					}
					
#if ASL_TARGET_OS_MAC
					copyResult = PL::ExportUtils::CopyWithProgress(normalizedItemPath, destPath, progress_callback, &progressData);
#else
					copyResult = ASL::File::CopyWithProgress(normalizedItemPath, destPath, progress_callback, &progressData);
#endif
					
					if (copyResult != ASL::kSuccess)
					{
						ASL::String message = dvacore::ZString("$$$/Prelude/Mezzanine/ExportLocal/Failure/CanNotCopyFile=Cannot copy file \"@0\" to \"@1\".");
						message = dvacore::utility::ReplaceInString(message, normalizedItemPath, destPath);
						mExportErrors.push_back(message);
					} 
				} 

				// This job item finished.
				EndProgress();

				// Check the copy Result for one clip.
				if (ASL::ResultFailed(copyResult))
				{
					return copyResult;
				}
				if (UpdateProgress(0))
				{
					return ASL::eUserCanceled;
				}
			}

			// The clip is exported.
			EndProgress();

			// Todo: not affect to Roughcut for mediaPath of Roughcut is changed to its media path.
			for (ActionResultList::iterator resultit=mActionResultList.begin(); resultit!=mActionResultList.end(); ++resultit)
			{
				if ((*resultit)->mFilePath == normalizedMediaPath)
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

private:
	ASL::String							mTitleString;
	ASL::Result							mResult;
	ASL::StationID						mStationID;
	dvacore::threads::AsyncExecutorPtr	mThreadedWorkQueue;
	volatile ASL::AtomicInt				mThreadDone;
	volatile ASL::AtomicInt				mAbort;
	volatile ASL::AtomicInt				mInProcess;
	bool								mConfirmedAbort;

	ASL::String							mProjectFile;
	ASL::String							mDestinationFolder;
	ASL::String							mSeedPremiereProjectFile;
	ML::IProjectConverterModuleRef		mConverterModule;
	std::set<ASL::String>				mExportMediaFiles;
	std::vector<ASL::String>			mExportErrors;
	std::vector<ASL::String>			mExportWarnings;

	ActionResultList					mActionResultList;
	PL::ExportUtils::MeidaPathToExportItemMap mMediaPathToExportItems;
};

// callback for CopyWithProgress: return true to continue copy, false to stop
bool progress_callback(ASL::Float32 inPercentDone, void* inProgressData)
{
	ASL_ASSERT(inPercentDone <= 1);

	PL::ExportUtils::ExportProgressData *p = static_cast<PL::ExportUtils::ExportProgressData*>(inProgressData);
	ASL::UInt64 lastProgress = p->currentProgress;
	
	// In Mac OS, currentProgress may be larger than inPercentDone*p->totalProgress, that would cause crash.
	if (p->currentProgress <= static_cast<ASL::UInt64>(inPercentDone * p->totalProgress))
	{
		p->currentProgress = static_cast<ASL::UInt64>(inPercentDone * p->totalProgress);
	}
	
	// If user cancels the progress, updateProgress() return true
	return !p->UpdateProgress(p->currentProgress - lastProgress);
}

} // private namespace


IExportLibraryItemsOperationRef CreateLocalExportLibraryItemsOperation(
		ML::IProjectConverterModuleRef inConverterModule, 
		ASL::String const& inDestinationFolder,
		ASL::String const& inProjectFile)
{
	return IExportLibraryItemsOperationRef(LocalExportLibraryItemsOperation::Create(inConverterModule, inDestinationFolder, inProjectFile));
}

} // namespace PL
