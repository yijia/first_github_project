/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2008 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

// Prefix
#include "Prefix.h"

// Self

// Local
#include "IngestMedia/IngestScheduler.h"
#include "IngestMedia/ImporterTaskExecutor.h"

// MZ
#include "IngestMedia/PLIngestUtils.h"
#include "IngestMedia/PLIngestMessage.h"
#include "PLProject.h"
#include "PLUtilitiesPrivate.h"
#include "MZUtilities.h"
#include "PLClipFinderUtils.h"
#include "PLLibrarySupport.h"
#include "PLUtilities.h"
#include "PLMediaMonitorCache.h"
#include "PLThreadUtils.h"

// ASL
#include "ASLStationRegistry.h"
#include "ASLSleep.h"
#include "ASLPathUtils.h"
#include "ASLStationUtils.h"
#include "ASLCoercion.h"
#include "ASLStringCompare.h"
#include "ASLAsyncCallFromMainThread.h"

// DVA
#include "dvacore/debug/Debug.h"
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/FileUtils.h"
#include "FIleManager/IFileManager.h"

// UIF
#include "UIFDocumentManager.h"

namespace PL
{
namespace IngestTask
{
	namespace
	{
		ASL::Guid sJobID("A289E6A8-D0E8-45A5-A545-5C6C2F1F1855");
		const ASL::String kCopyExecutorName = ASL_STR("IngestCopyExecutor");
		const ASL::String kUpdateMetadataExecutorName = ASL_STR("IngestUpdateMetadataExecutor");
		const ASL::String kSingleFileFormat = ASL_STR("Single");

		dvacore::threads::AsyncThreadedExecutorPtr CreateOrGetCopyExecutor()
		{
			dvacore::threads::AsyncThreadedExecutorPtr existInstance = PL::threads::GetExecutor(kCopyExecutorName);
			return existInstance != NULL ? existInstance : PL::threads::CreateAndRegisterExecutor(kCopyExecutorName);
		}

		void ReleaseCopyExecutor()
		{
			PL::threads::UnregisterExecutor(kCopyExecutorName);
		}

		dvacore::threads::AsyncThreadedExecutorPtr CreateOrGetUpdateMetadataExecutor()
		{
			dvacore::threads::AsyncThreadedExecutorPtr existInstance = PL::threads::GetExecutor(kUpdateMetadataExecutorName);
			return existInstance != NULL ? existInstance : PL::threads::CreateAndRegisterExecutor(kUpdateMetadataExecutorName);
		}

		void ReleaseUpdateMetadataExecutor()
		{
			PL::threads::UnregisterExecutor(kUpdateMetadataExecutorName);
		}

		template <typename T>
		bool IncludeRunningTask(T& t)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskState() == kTaskState_Running )
				{
					return true;
				}
			}

			return false;
		}

		template <typename T>
		void PauseTask(T& t)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskState() == kTaskState_Init )
				{
					(*itr)->SetTaskState(kTaskState_Paused);
				}
			}
		}

		template <typename T>
		void ResumeTask(T& t)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskState() == kTaskState_Paused )
				{
					(*itr)->SetTaskState(kTaskState_Init);
				}
			}
		}

		template <typename T>
		void CancelTask(T& t)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskState() != kTaskState_Running )
				{
					(*itr)->SetTaskState(kTaskState_Aborted);
				}
			}
		}

		template <typename T>
		void ClearTask(T& t, const TaskState& inState)
		{
			T tempTaskList;
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskState() != inState )
				{
					tempTaskList.push_back(*itr);
				}
			}
			std::swap(t, tempTaskList);
		}

		template <typename T, typename F>
		F FindTask(T& t, const ASL::Guid& inID)
		{
			F val;
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetTaskID() == inID )
				{
					val = *itr;
				}
			}
			return val;
		}

		template <typename T, typename F>
		F FindBatch(T& t, const ASL::Guid& inID)
		{
			F val;
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr)->GetBatchID() == inID )
				{
					val = *itr;
					break;
				}
			}
			return val;
		}

		template <typename T>
		bool FindTask(T& t, TaskBase* task)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr).get() == task )
				{
					return true;
				}
			}
			return false;
		}

		template<typename T, typename P>
		void AddTask(T& t, P& val)
		{
			if ( !FindTask<T>(t, val.get()) )
			{
				t.push_back(val);
			}
		}

		template<typename T>
		void RemoveTask(T& t, TaskBase* task)
		{
			typename T::iterator itr = t.begin();
			typename T::iterator end = t.end();
			for (; itr != end; ++itr)
			{
				if ( (*itr).get() == task )
				{
					t.remove(*itr);
					break;
				}
			}
		}

		// [TODO] This is only a work around to keep CopyOperationPtr when async call its process function.
		// Should refactor to keep the shared ptr by a better way.
		void DoCopyOperation(CopyOperationPtr inOperation)
		{
			inOperation->Process();
		}

		void DoUpdateMetadataOperation(UpdateMetadataOperationPtr inOperation)
		{
			inOperation->Process();
		}

		ASL::String RemoveRootFromPath(const ASL::String& inFullPath, const ASL::String& inRoot)
		{
			if ( inFullPath.length() <= inRoot.length() )
			{
				return DVA_STR("");
			}

			const ASL::String& root = inFullPath.substr(0, inRoot.length());
			if (root != inRoot)
			{
				return inFullPath;
			}

			return inFullPath.substr(inRoot.length(), inFullPath.length() - inRoot.length());
		}

		/*
		**	Delete the file first. If after that, its parent folder is empty, then delete the parent folder.
		*/
		void ClearTempTranscodeFile(const ASL::String& inPath)
		{
			ASL::String parentDir = ASL::PathUtils::GetFullDirectoryPart(inPath);
			ASL::File::Delete(inPath);
			if (ASL::PathUtils::IsDirectoryEmpty(parentDir))
			{
				ASL::Directory::Delete(parentDir);
			}
		}

		bool FileIsEditingByPrelude(const ASL::String& inFilePath)
		{
			if (ASL::PathUtils::ExistsOnDisk(inFilePath))
			{
				PL::ISRMediaRef srMedia = PL::SRProject::GetInstance()->GetSRMedia(inFilePath);
				return ((srMedia != NULL) && srMedia->IsDirty());
			}
			return false;
		}

		void NotifyBackupItemsAreReady(
			const ASL::Guid& inJobID,
			const ASL::Guid& inBatchID,
			const PL::IngestItemList& inIngestItemList,
			const IngestCustomData& inCustomData)
		{
			PL::IngestItemList backupItemList = inIngestItemList;
			BOOST_FOREACH(IngestItemPtr& backupItem, backupItemList)
			{
				backupItem->mItemFormat = PL::Utilities::ConvertProviderIDToFormat(backupItem->mItemFormat);
			}
			SRProject::GetInstance()->GetAssetLibraryNotifier()->BackupItemsAreReady(
				inJobID, 
				inBatchID,
				backupItemList,
				inCustomData);
		}
	} // End of anonymous workspace

	//------------------------------------------------------------------------------
	// struct CopySetting

	CopySetting::CopySetting()
		:
		//mCopyAction(kCopyAction_Copied),
		mVerifyOption(kVerify_None)
	{
	}

	//------------------------------------------------------------------------------
	// struct CopyRunnerSetting
	CopyRunnerSetting::CopyRunnerSetting()
		:
		mExistFolderOption(kExist_WarnUser),
		mExistFileOption(kExist_WarnUser)
	{
	}

	void CopyRunnerSetting::Reset()
	{
		mExistFolderOption = kExist_WarnUser;
		mExistFileOption = kExist_WarnUser;
	}

	//------------------------------------------------------------------------------
	// class ThreadProcess

	static const dvacore::threads::AtomicInt32 kAsyncOperationStatus_Normal = 0;
	static const dvacore::threads::AtomicInt32 kAsyncOperationStatus_Paused = 1;
	static const dvacore::threads::AtomicInt32 kAsyncOperationStatus_Canceled = 2;
	static const dvacore::threads::AtomicInt32 kAsyncOperationStatus_Done = 3;

	ThreadProcess::ThreadProcess()
		: mStatus(kAsyncOperationStatus_Normal)
	{
	}

	void ThreadProcess::Process()
	{
		dvacore::threads::AtomicWrite(mStatus, kAsyncOperationStatus_Normal);
	}

	bool ThreadProcess::Pause()
	{
		return dvacore::threads::AtomicCompareAndSet(mStatus, kAsyncOperationStatus_Normal, kAsyncOperationStatus_Paused);
	}

	bool ThreadProcess::Resume()
	{
		return dvacore::threads::AtomicCompareAndSet(mStatus, kAsyncOperationStatus_Paused, kAsyncOperationStatus_Normal);
	}

	bool ThreadProcess::Cancel()
	{
		if (!IsDone())
		{
			dvacore::threads::AtomicWrite(mStatus, kAsyncOperationStatus_Canceled);
			return true;
		}
		return false;
	}

	bool ThreadProcess::IsPaused() const
	{
		return dvacore::threads::AtomicRead(mStatus) == kAsyncOperationStatus_Paused;
	}

	bool ThreadProcess::IsCanceled() const
	{
		return dvacore::threads::AtomicRead(mStatus) == kAsyncOperationStatus_Canceled;
	}

	bool ThreadProcess::IsDone() const
	{
		return dvacore::threads::AtomicRead(mStatus) == kAsyncOperationStatus_Done;
	}

	void ThreadProcess::Done()
	{
		if (!IsCanceled())
		{
			dvacore::threads::AtomicWrite(mStatus, kAsyncOperationStatus_Done);
		}
	}

	bool ThreadProcess::CanContinue(ASL::UInt32 inIntervalTime) const
	{
		while (IsPaused())
		{
			ASL::Sleep(inIntervalTime);
		}
		return !IsCanceled();
	}

	//------------------------------------------------------------------------------
	// class BaseOperation

	BaseOperation::BaseOperation(const ASL::StationID& inStationID)
		: mStationID(inStationID)
	{
	}

	BaseOperation::~BaseOperation()
	{
	}

	ASL::StationID BaseOperation::GetStationID() const
	{
		return mStationID;
	}

	//------------------------------------------------------------------------------
	// class CopyOperation
	CopyOperation::CopyOperation(
		const ASL::StationID& inStationID,
		CopyTaskPtr inTask,
		CopyRunnerSetting&	ioRunningSetting,
		TaskScheduler* inTaskScheduler)
		:
		BaseOperation(inStationID),
		mTask(inTask),
		mRunningSetting(ioRunningSetting),
		mDoneCount(0),
		mTotalResult(ASL::kSuccess),
		mTaskScheduler(inTaskScheduler)
	{
	}

	CopyOperation::~CopyOperation()
	{
		mTask = CopyTaskPtr();
	}

	bool CopyOperation::OnProgressUpdate(size_t doneCount, size_t totalCount, ASL::Float32 inLastPercentDone, ASL::Float32 inPercentDone)
	{
		// [TODO] post message to update progress
		double percent = (double)(doneCount + inPercentDone)/(double)totalCount;
		ASL::StationUtils::PostMessageToUIThread(
			kStation_IngestMedia,
			CopyProgressMessage(percent));

		return CanContinue();
	}

	ASL::Result CopyOperation::CopyOneImportableSet(
		PL::IngestTask::CopyUnit::SharedPtr inSet,
		PL::VerifyOption inVerifyOption)
	{
		ASL::Result copySetResult = ASL::kSuccess;

		// if any destination file is opened in Prelude, we don't allow copy
		BOOST_FOREACH(SrcToDestCopyDataList::value_type& srcToDstData, inSet->mSrcToDestCopyData)
		{
			if (!CanContinue())
			{
				return copySetResult;
			}

			if (FileIsEditingByPrelude(srcToDstData.mDestFile))
			{
				mTotalResult = ASL::eUnknown;
				copySetResult = ASL::eUnknown;
				ASL::Result copyResult = ASL::eFileInUse;
				mDoneCount += inSet->mSrcToDestCopyData.size();
				OnProgressUpdate(mDoneCount - 1, mTotalCount, 0.0f, 1.0f);
				mCopyResults.push_back(CopyResult(copyResult, IngestUtils::ReportStringOfCopyResult(srcToDstData.mSrcFile, srcToDstData.mDestFile, copyResult)));
				return copySetResult;
			}
		}

		ASL::PathnameList filesNeedRefreshMedia;
		BOOST_FOREACH(SrcToDestCopyDataList::value_type& srcToDstData, inSet->mSrcToDestCopyData)
		{
			if (!CanContinue())
			{
				return copySetResult;
			}

			const ASL::String& source = srcToDstData.mSrcFile;
			const ASL::String& destination = srcToDstData.mDestFile;
			MF::FileManager::TryToCloseFileWithoutBlocking(destination);

			ASL::Result result = ASL::eUnknown;
			VerifyFileResult vResult = kVerifyFileResult_Equal;

			if (srcToDstData.mIsOptionalSrc && !ASL::PathUtils::ExistsOnDisk(source))
			{
				result = ASL::kSuccess;
				OnProgressUpdate(mDoneCount, mTotalCount, 0.0f, 1.0f);
			}
			else if (srcToDstData.mCopyAction == kCopyAction_Ignored)
			{
				result = ASL::kSuccess;
				OnProgressUpdate(mDoneCount, mTotalCount, 0.0f, 1.0f);
			}
			// For folder which don't exist, ASL::PathUtils::IsDirectory will return false, so handle it here.
			else if (!ASL::PathUtils::ExistsOnDisk(source) && ASL::PathUtils::HasTrailingSlash(source))
			{
				result = ASL::ePathNotFound;
				OnProgressUpdate(mDoneCount, mTotalCount, 0.0f, 1.0f);
				mCopyResults.push_back(CopyResult(result, IngestUtils::ReportStringOfCopyResult(source, destination, result)));
			}
			else if ( ASL::PathUtils::IsDirectory(source) )
			{
				if ( !ASL::PathUtils::ExistsOnDisk(destination) )
				{
					result = ASL::Directory::CreateOnDisk(destination, true);
				}
				else
				{
					result = ASL::kSuccess;
				}
				OnProgressUpdate(mDoneCount, mTotalCount, 0.0f, 1.0f);
				mCopyResults.push_back(CopyResult(result, IngestUtils::ReportStringOfCopyResult(source, destination, result)));
			}
			else
			{
				dvacore::utility::FileUtils::EnsureDirectoryExists(ASL::PathUtils::GetFullDirectoryPart(destination));

				if (srcToDstData.mCopyAction == kCopyAction_Copied && MZ::Utilities::ExistOnDisk(destination))
				{
					bool canContinue = PL::IngestUtils::GenerateFileCopyAction(
						srcToDstData.mExistOption, 
						mRunningSetting.mExistFileOption,
						source,
						destination,
						srcToDstData.mCopyAction);
					if (!canContinue)
					{
						ASL::StationUtils::PostMessageToUIThread(PL::kStation_IngestMedia, PL::CancelIngestMessage());
						// Wait until it's really canceled.
						while (CanContinue())
						{
							ASL::Sleep(10);
						}
						return copySetResult;
					}
				}

				bool willOverwrite = (srcToDstData.mCopyAction == kCopyAction_Replaced);
				ASL::String realDestination(destination);
				CopyAction copyAction = srcToDstData.mCopyAction;

				// if imported file will be overwrite, we explicitely refresh it before so that its content ID will be refreshed.
				//	Or maybe xmp will not be 
				if (willOverwrite && SRMediaMonitor::GetInstance()->IsFileMonitored(realDestination))
				{
					// refresh media file cache
					RefreshBackEndFileCache(realDestination);
				}

				result = PL::IngestUtils::SmartCopyFileWithProgress(
					source,
					realDestination,
					copyAction,
					boost::bind(&CopyOperation::OnProgressUpdate, this, mDoneCount, mTotalCount, _1, _2));

				if (willOverwrite && ASL::ResultSucceeded(result))
				{
					filesNeedRefreshMedia.push_back(realDestination);
				}

				// Report copy result regardless failure or success
				mCopyResults.push_back(CopyResult(result, IngestUtils::ReportStringOfCopyResult(source, realDestination, result)));
				
				if ( copyAction != kCopyAction_NoFurtherAction )
				{
					const ASL::String& copyWarning = 
						IngestUtils::ReportStringOfCopyAction(source, destination, realDestination, copyAction);
					if (!copyWarning.empty())
					{
						mCopyResults.push_back(CopyResult(result, copyWarning));
					}
				}

				// Verify the copy result
				if ( ASL::ResultSucceeded(result) && 
					 inVerifyOption > kVerify_None && 
					 inVerifyOption < kVerify_End)
				{
					ASL::String verifyResultStr;
					vResult = PL::IngestUtils::VerifyFile(
						inVerifyOption, 
						source,
						destination,
						verifyResultStr);

					// Report verification result regardless failure or success
					ASL::Result copyResult = 
						(vResult != kVerifyFileResult_Equal) ? 
						ASL::eUnknown :
						ASL::kSuccess;
					mCopyResults.push_back(CopyResult(copyResult, verifyResultStr));
				}
			}

			if ( ASL::ResultFailed(result) || vResult != kVerifyFileResult_Equal )
			{
				mTotalResult = ASL::eUnknown;
				copySetResult = ASL::eUnknown;
			}
			++mDoneCount;
		}

		// If overwrite clips which have been in project panel, we should refresh our memory data
		BOOST_FOREACH (const ASL::String& refreshPath, filesNeedRefreshMedia)
		{
			// should refresh backend cache first before refresh xmp
			RefreshBackEndFileCache(refreshPath);
			SRMediaMonitor::GetInstance()->RefreshXMPIfChanged(refreshPath);
		}

		return copySetResult;
	}

	void CopyOperation::CalcTotalfilesCount()
	{
		mTotalCount = 0;
		CopySetting& setting = mTask->GetCopySetting();
		BOOST_FOREACH (const CopyUnit::SharedPtr& filesSet, setting.mCopyUnits)
		{
			mTotalCount += filesSet->mSrcToDestCopyData.size();
		}
	}

	void CopyOperation::Process()
	{
		if (!CanContinue())
		{
			return;
		}

		CalcTotalfilesCount();

		CopySetting& setting = mTask->GetCopySetting();

		std::size_t successCount = 0;
		BOOST_FOREACH (CopyUnit::SharedPtr& filesSet, setting.mCopyUnits)
		{
			ASL::Result copySetResult = CopyOneImportableSet(filesSet, setting.mVerifyOption);
			if (!CanContinue())
			{
				return;
			}
			if (ASL::ResultSucceeded(copySetResult))
			{
				++successCount;
			}
			ASL::AsyncCallFromMainThread(boost::bind(
				&TaskScheduler::CreateSubsequentTaskAfterCopy,
				mTaskScheduler,
				copySetResult,
				mTask,
				filesSet));
		}

		Done();

		ASL::StationUtils::PostMessageToUIThread(
			GetStationID(),
			CopyFinishedMessage(mTask->GetTaskID(), mTotalResult, mCopyResults, successCount, setting.mCopyUnits.size() - successCount),
			true);
	}

	//------------------------------------------------------------------------------
	// class UpdateMetadataOperation

	UpdateMetadataOperation::UpdateMetadataOperation(
		const ASL::StationID& inStationID,
		UpdateMetadataTaskPtr inTask,
		TaskScheduler* inTaskScheduler)
		:
	BaseOperation(inStationID),
		mTask(inTask),
		mDoneCount(0),
		mTotalResult(ASL::kSuccess),
		mTaskScheduler(inTaskScheduler)
	{
	}

	UpdateMetadataOperation::~UpdateMetadataOperation()
	{
		mTask.reset();
	}

	void UpdateMetadataOperation::Process()
	{
		if (!CanContinue())
		{
			return;
		}

		// Get information from mTask setting
		ClipAliasNameMap aliasNameMap;
		ClipAliasNameMap& aliasNameMapInSetting = mTask->GetSetting().mAliasNameMap;
		IngestCustomData customData = mTask->GetCustomData();
		PathToClipIDMap pathToClipIDMap = mTask->GetSetting().mPathToClipIDMap;

		// rename physical files if needed
		if (mTask->GetSetting().mNeedDoFileRename && !aliasNameMapInSetting.empty())
		{
			BOOST_FOREACH (const ClipAliasNameMap::value_type& pathNamePair, aliasNameMapInSetting)
			{
				try
				{
					ASL::String targetFullPath = Utilities::ReplaceFilePartOfPath(pathNamePair.first, pathNamePair.second);

					dvacore::filesupport::File(pathNamePair.first).Rename(ASL::PathUtils::GetFullFilePart(targetFullPath));

					// add new alias name item into aliasNameMap with new file path as key.
					aliasNameMap[targetFullPath] = pathNamePair.second;
				}
				catch (...)
				{
					mTotalResult = ASL::eAccessIsDenied;
					mResults.push_back(ResultReport(
						ASL::ResultFlags::kResultTypeFailure,
						dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/RenameFailure=Rename @0 with @1 failed.", pathNamePair.first, pathNamePair.second)));
					break;
				}
				if (!CanContinue())
				{
					return;
				}
			}
		}
		else
		{
			aliasNameMap = aliasNameMapInSetting;
		}

		// update logical alias name and metadata
		if ( ASL::ResultSucceeded(mTotalResult) && 
			(!aliasNameMap.empty() || !customData.mMetadata.empty() || !pathToClipIDMap.empty()))
		{
			ImportPathInfoMap importPathInfoMap = mTask->GetSetting().mNeedImportFiles;
			ImportPathInfoMap::iterator it = importPathInfoMap.begin();
			ImportPathInfoMap::iterator end = importPathInfoMap.end();
			for (; it != end; ++it)
			{
				ASL::String filePath = it->first;
				if (!Utilities::PathSupportsXMP(filePath))
				{
					// we ignore files not support XMP, such as rough cut file, and keep continue.
					if (!customData.mMetadata.empty() || aliasNameMap.find(filePath) != aliasNameMap.end())
					{
						// report error only when failed to write custom metadata or alias name. not report error when failed to write clipID.
						mResults.push_back(ResultReport(
							ASL::ResultFlags::kResultTypeFailure, 
							dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/NotSupportXMP=File @0 does not support metadata.", filePath)));
					}
					continue;
				}

				PL::XMPText xmpBuffer(new ASL::StdString());
				ASL::String errorInfo;
				ASL::Result result = PL::SRLibrarySupport::ReadXMPFromFile(filePath, xmpBuffer, errorInfo);
				if (ASL::ResultFailed(result) || xmpBuffer == NULL)
				{
					mTotalResult = ASL::eUnknown;
					mResults.push_back(ResultReport(
						ASL::ResultFlags::kResultTypeFailure, 
						dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/ReadXMPFailed=Cannot read metadata from file @0.", filePath)));
					break;
				}

				ClipAliasNameMap::iterator aliasIt = aliasNameMap.find(filePath);
				if (aliasIt != aliasNameMap.end())
				{
					result = PL::SRLibrarySupport::UpdateXMPWithRenameData(xmpBuffer, aliasIt->second);
					if (ASL::ResultFailed(result))
					{
						mTotalResult = ASL::eUnknown;
						mResults.push_back(ResultReport(
							ASL::ResultFlags::kResultTypeFailure, 
							dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/UpdateRenameFailed=Cannot update rename data for file @0.", filePath)));
						break;
					}
				}
				if (!customData.mMetadata.empty())
				{
					result = PL::SRLibrarySupport::UpdateXMPWithMetadata(xmpBuffer, customData.mMetadata, false);
					if (ASL::ResultFailed(result))
					{
						mTotalResult = ASL::eUnknown;
						mResults.push_back(ResultReport(
							ASL::ResultFlags::kResultTypeFailure, 
							dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/UpdateMetadataFailed=Cannot update metadata for file @0.", filePath)));
						break;
					}
				}

				PathToClipIDMap::iterator clipIDIt = pathToClipIDMap.find(filePath);
				if (clipIDIt != pathToClipIDMap.end())
				{
					if ((*clipIDIt).second != ASL::Guid())
					{
						PL::SRLibrarySupport::WriteClipIDToXMPIfNeeded(xmpBuffer, (*clipIDIt).second);
					}
				}

				PL::AssetMediaInfoPtr assetMediaInfo = PL::AssetMediaInfo::CreateMasterClipMediaInfo(
					ASL::Guid::CreateUnique(), 
					filePath, 
					DVA_STR(""), 
					xmpBuffer);
				DVA_ASSERT_MSG(assetMediaInfo, "Create MediaInfo failed!");
				if (!assetMediaInfo)
				{
					continue;
				}

				bool saveResult = PL::SRLibrarySupport::SaveXMPToDisk(assetMediaInfo);
				if (!saveResult && !UIF::IsEAMode())
				{
					mTotalResult = ASL::eUnknown;
					mResults.push_back(ResultReport(
						ASL::ResultFlags::kResultTypeFailure, 
						dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/WriteXMPFailed=Cannot write metadata to file @0.", filePath)));
					break;
				}
				mResults.push_back(ResultReport(ASL::kSuccess, filePath));

				if (!CanContinue())
				{
					return;
				}
			}
		}

		// Create import task
		ASL::AsyncCallFromMainThread(boost::bind(
			&TaskScheduler::CreateSubsequentTaskAfterUpdateMetadata,
			mTaskScheduler,
			mTotalResult,
			mTask));

		Done();

		ASL::StationUtils::PostMessageToUIThread(
			GetStationID(),
			UpdateMetadataFinishedMessage(mTask->GetTaskID(), mTotalResult, mResults),
			true);
	}

	//------------------------------------------------------------------------------
	// class TaskBase

	TaskBase::TaskBase(const ASL::Guid& inBatchID, const IngestCustomData& inCustomData, TaskState inState) 
		: 
	    mBatchID(inBatchID),
		mTaskState(inState),
		mTaskID(ASL::Guid::CreateUnique()),
		mCustomData(inCustomData)
	{
	}

	TaskBase::~TaskBase()	
	{

	}

	TaskState TaskBase::GetTaskState() const
	{
		return mTaskState;
	}

	void TaskBase::SetTaskState(TaskState inState)
	{
		mTaskState = inState;
	}

	ASL::Guid TaskBase::GetBatchID() const
	{
		return mBatchID;
	}
	void TaskBase::SetBatchID(const ASL::Guid& inID)
	{
		mBatchID = inID;
	}

	ASL::Guid TaskBase::GetTaskID() const
	{
		return mTaskID;
	}

	std::size_t	TaskBase::GetTaskUnitCount() const
	{
		return 1;
	}

	IngestCustomData TaskBase::GetCustomData() const
	{
		return mCustomData;
	}

	IngestCustomData& TaskBase::GetCustomData()
	{
		return mCustomData;
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	CopyTask::CopyTask(const CopySetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData) 
		: 
		TaskBase(inBatchID, inCustomData),
		mCopySetting(inSetting)
	{
	}

	CopyTask::~CopyTask()
	{
	}

	const CopySetting& CopyTask::GetCopySetting() const
	{
		return mCopySetting;
	}

	CopySetting& CopyTask::GetCopySetting()
	{
		return mCopySetting;
	}

	std::size_t	CopyTask::GetTaskUnitCount() const
	{
		// default 1 for copy task itself
		std::size_t unitCount = 1;

		const bool hasCustomMetadata = !mCustomData.mMetadata.empty();

		// count all import tasks and update metadata tasks
		BOOST_FOREACH (const CopyUnit::SharedPtr& filesSet, mCopySetting.mCopyUnits)
		{
			if (!filesSet->mNeedImportFiles.empty() && mCopySetting.mNeedCreateImportTask)
			{
				++unitCount;
			}

			if (hasCustomMetadata || !filesSet->mAliasNameMap.empty() || !filesSet->mPathToClipIDMap.empty())
			{
				++unitCount;
			}
		}
		return unitCount;
	}

	CopyExistOption MerdgeCopyExistOption(const CopyExistOption& inFirst, const CopyExistOption& inSecond)
	{
		return inFirst > inSecond ? inFirst : inSecond;
	}

	void CopyTask::RemoveDuplicatedCopy()
	{
		// Keep copy item order (Huasheng & Yining required) and remove duplicated ones
		typedef std::map<ASL::String, SrcToDestCopyDataList::iterator> DstDataMap;
		std::map<ASL::String, DstDataMap> uniqueCopyData;
		BOOST_FOREACH (CopyUnit::SharedPtr& filesSet, mCopySetting.mCopyUnits)
		{
			SrcToDestCopyDataList& copyList = filesSet->mSrcToDestCopyData;
			SrcToDestCopyDataList::iterator iter = copyList.begin();
			SrcToDestCopyDataList::iterator iterEnd = copyList.end();
			while (iter != iterEnd)
			{
				DVA_ASSERT_MSG(!iter->mDestFile.empty(), "Empty destination in copy task.");
				DstDataMap& dstData = uniqueCopyData[iter->mSrcFile];
				if (dstData.find(iter->mDestFile) != dstData.end())
				{
					CopyExistOption& copyExistOption = dstData[iter->mDestFile]->mExistOption;
					copyExistOption = MerdgeCopyExistOption(copyExistOption, iter->mExistOption);
					iter = copyList.erase(iter);
				}
				else
				{
					dstData[iter->mDestFile] = iter;
					++iter;
				}
			}
		}
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	UpdateMetadataTask::UpdateMetadataTask(const UpdateMetadataSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData) 
		: TaskBase(inBatchID, inCustomData),
		mSetting(inSetting)
	{
	}

	UpdateMetadataTask::~UpdateMetadataTask()
	{
	}

	std::size_t UpdateMetadataTask::GetTaskUnitCount() const
	{
		// we think an import task will always be counted. If it's not really created when process, the operation should update it to complete directly.
		return 2;
	}

	UpdateMetadataSetting& UpdateMetadataTask::GetSetting()
	{
		return mSetting;
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	ImportTask::ImportTask(const ImportSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData) 
		: TaskBase(inBatchID, inCustomData),
		  mImportSetting(inSetting)
	{

	}

	ImportTask::~ImportTask()
	{

	}

	ImportSetting  ImportTask::GetImportSetting() const
	{
		return mImportSetting;
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	TranscodeTask::TranscodeTask(const TranscodeSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData) 
		: 
		TaskBase(inBatchID, inCustomData),
		mTranscodeSetting(inSetting),
		mTranscodeJobID(mTaskID.AsString())
	{
	}

	TranscodeTask::~TranscodeTask()
	{

	}

	std::size_t	TranscodeTask::GetTaskUnitCount() const
	{
		std::size_t count = 1;
		if (!mTranscodeSetting.mSrcFileSettings.front().mAliasName.empty() || !GetCustomData().mMetadata.empty())
			++count;
		
		if (mTranscodeSetting.mNeedImport)
			++count;

		return count;
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	ConcatenateTask::ConcatenateTask(const TranscodeSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData)
		: 
		TranscodeTask(inSetting, inBatchID, inCustomData)
	{

	}
	ConcatenateTask::~ConcatenateTask()
	{
		RemoveTempProjectFile();
	}

	std::size_t	ConcatenateTask::GetTaskUnitCount() const
	{
		// should always create update metadata task, for we always need rename.
		std::size_t count = 2;

		if (mTranscodeSetting.mNeedImport)
			++count;

		return count;
	}

	void ConcatenateTask::SetProject(BE::IProjectRef const& inProject) 
	{
		mProject = inProject;
	}
	void ConcatenateTask::RemoveTempProjectFile()
	{
		if (!mProject)
		{
			return;
		}

		ASL::String const& projectPath = mProject->GetFullPath();
		if (ASL::PathUtils::IsValidPath(projectPath) && ASL::PathUtils::ExistsOnDisk(projectPath))
		{
			ASL::File::Delete(projectPath);
		}
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	TaskFactory::TaskFactory()
	{
	}

	TaskFactory::~TaskFactory()
	{
	}

	CopyTaskPtr TaskFactory::CreateCopyTask(const CopySetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData)
	{
		CopyTaskPtr copyTask(new PL::IngestTask::CopyTask(inSetting, inBatchID, inCustomData));
		copyTask->RemoveDuplicatedCopy();
		return copyTask;
	}

	void TaskFactory::SubmitCopyTask(const CopySetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData)
	{
		TaskScheduler::AddCopyTask(CreateCopyTask(inSetting, inBatchID, inCustomData));
	}

	void TaskFactory::CreateTranscodeTask(
		const TranscodeSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData,
		TranscodeTaskList& outTasks)
	{
		ClipSettingList::const_iterator it = inSetting.mSrcFileSettings.begin();
		ClipSettingList::const_iterator end = inSetting.mSrcFileSettings.end();
		for (; it != end; ++it)
		{
			// Create one task for each source file.
			TranscodeSetting setting; 
			setting.mSrcFileSettings.push_back(*it);
			setting.mDestFolder		= inSetting.mDestFolder;
			setting.mEnable			= inSetting.mEnable;
			setting.mExporterModule = inSetting.mExporterModule;
			setting.mNeedImport     = inSetting.mNeedImport;
			setting.mPreset         = inSetting.mPreset;
			setting.mSrcRootPath	= inSetting.mSrcRootPath;
			PL::IngestTask::TranscodeTaskPtr transcodeTask(new PL::IngestTask::TranscodeTask(setting, inBatchID, inCustomData));
			outTasks.push_back(transcodeTask);
		}
	}

	void TaskFactory::SubmitTranscodeTask(
		const TranscodeSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		TranscodeTaskList tasks;
		CreateTranscodeTask(inSetting, inBatchID, inCustomData, tasks);
		BOOST_FOREACH (TranscodeTaskPtr& task, tasks)
		{
			TaskScheduler::AddTranscodeTask(task);
		}
	}

	ConcatenateTaskPtr TaskFactory::CreateConcatenateTask(
		const TranscodeSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		PL::IngestTask::ConcatenateTaskPtr concatenateTask(new PL::IngestTask::ConcatenateTask(inSetting, inBatchID, inCustomData));
		return concatenateTask;
	}

	void TaskFactory::SubmitConcatenateTask(
		const TranscodeSetting& inSetting, 
		const ASL::Guid& inBatchID, 
		const IngestCustomData& inCustomData)
	{
		TaskScheduler::AddConcatenateTask(CreateConcatenateTask(inSetting, inBatchID, inCustomData));
	}

	ImportTaskPtr TaskFactory::CreateImportTask(
		const ImportSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		PL::IngestTask::ImportTaskPtr importTask(new PL::IngestTask::ImportTask(inSetting, inBatchID, inCustomData));
		return importTask;
	}

	void TaskFactory::SubmitImportTask(
		const ImportSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		TaskScheduler::AddImportTask(CreateImportTask(inSetting, inBatchID, inCustomData));
	}

	UpdateMetadataTaskPtr TaskFactory::CreateUpdateMetadataTask(
		const UpdateMetadataSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		PL::IngestTask::UpdateMetadataTaskPtr newTask(new PL::IngestTask::UpdateMetadataTask(inSetting, inBatchID, inCustomData));
		return newTask;
	}

	void TaskFactory::SubmitUpdateMetadataTask(
		const UpdateMetadataSetting& inSetting,
		const ASL::Guid& inBatchID,
		const IngestCustomData& inCustomData)
	{
		TaskScheduler::AddUpdateMetadataTask(CreateUpdateMetadataTask(inSetting, inBatchID, inCustomData));
	}

	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	TaskSchedulerPtr TaskScheduler::sTaskScheduler;

	ASL_MESSAGE_MAP_DEFINE(TaskScheduler)
		
		ASL_MESSAGE_HANDLER(CopyProgressMessage,			OnCopyTaskProgress)
		ASL_MESSAGE_HANDLER(CopyFinishedMessage,			OnCopyTaskFinished)

		ASL_MESSAGE_HANDLER(UpdateMetadataFinishedMessage,	OnUpdateMetadataTaskFinished)

		ASL_MESSAGE_HANDLER(MZ::RenderProgressMessage,			OnTranscodeTaskProgress)
		ASL_MESSAGE_HANDLER(MZ::RenderStatusMessage,			OnTranscodeTaskStatus)
		ASL_MESSAGE_HANDLER(MZ::RenderCompleteMessage,			OnTranscodeTaskFinished)
		ASL_MESSAGE_HANDLER(MZ::RenderErrorMessage,				OnTranscodeTaskError)
		ASL_MESSAGE_HANDLER(MZ::RenderErrorMessage,				OnTranscodeTaskReady)
		ASL_MESSAGE_HANDLER(MZ::AMEServerOfflineMessage,		OnAMEServerOffline)

		ASL_MESSAGE_HANDLER(StartIngestMessage,				OnStartIngest)
		ASL_MESSAGE_HANDLER(PauseIngestMessage,				OnPauseIngest)
		ASL_MESSAGE_HANDLER(ResumeIngestMessage,			OnResumeIngest)
		ASL_MESSAGE_HANDLER(CancelIngestMessage,			OnCancelIngest)

		ASL_MESSAGE_HANDLER(ImportXMPMessage,				OnImportXMP)
		ASL_MESSAGE_HANDLER(RequestThumbnailMessage,		OnRequestThumbnail)
		ASL_MESSAGE_HANDLER(FinishImportFileMessage,		OnFinishImportFile)
		ASL_MESSAGE_HANDLER(ImportFinishedMessage,			OnImportTaskFinished)
	ASL_MESSAGE_MAP_END


	TaskScheduler::TaskScheduler()
		:
		mStationID(ASL::StationRegistry::RegisterUniqueStation()),
		mSchedulerState(kSchedulerState_Init),
		mDoneTaskCount(0),
		mTotalTaskCount(0),
		mCopyProgress(0.0f),
		mTranscodeProgress(0.0f),
		mConcatenateProgress(0.0f),
		mImportProgress(0.0f),
		mUpdateMetadataProgress(0.0f),
		mSucceededCopyFileCount(0),
		mSucceededTranscodeFileCount(0),
		mSucceededConcatenateFileCount(0),
		mSucceededImportFileCount(0),
		mSucceededUpdateMetadataCount(0),
		mFailedCopyFileCount(0),
		mFailedTranscodeFileCount(0),
		mFailedConcatenateFileCount(0),
		mFailedImportFileCount(0),
		mFailedUpdateMetadataCount(0)
	{
		ASL::StationUtils::AddListener(mStationID, this);
		ASL::StationUtils::AddListener(kStation_IngestMedia, this);
	}

	TaskScheduler::~TaskScheduler()
	{
		ReleaseCopyExecutor();
		ReleaseUpdateMetadataExecutor();
		ASL::StationUtils::RemoveListener(mStationID, this);
		ASL::StationUtils::RemoveListener(kStation_IngestMedia, this);
		ASL::StationRegistry::DisposeStation(mStationID);

		// Maybe some tasks are canceled, the rest temp files should be deleted.
		for (size_t i = 0, count = mTranscodeTempFiles.size(); i < count; ++i)
		{
			ClearTempTranscodeFile(mTranscodeTempFiles[i]);
		}
		mTranscodeTempFiles.clear();
	}

	TaskSchedulerPtr TaskScheduler::GetInstance()
	{
		if ( sTaskScheduler == NULL )
		{
			sTaskScheduler.reset(new TaskScheduler());
		}
		return sTaskScheduler;
	}

	void TaskScheduler::AddCopyTask(CopyTaskPtr inTask)
	{
		GetInstance()->Add(inTask);
	}

	void TaskScheduler::AddTranscodeTask(TranscodeTaskPtr inTask)
	{
		GetInstance()->Add(inTask);
	}

	void TaskScheduler::AddConcatenateTask(ConcatenateTaskPtr inTask)
	{
		GetInstance()->Add(inTask);
	}

	void TaskScheduler::AddImportTask(ImportTaskPtr inTask)
	{
		GetInstance()->Add(inTask);
	}

	void TaskScheduler::AddUpdateMetadataTask(UpdateMetadataTaskPtr inTask)
	{
		GetInstance()->Add(inTask);
	}

	bool TaskScheduler::IsImportTask(const ASL::Guid& inImportTaskID)
	{
		if ( sTaskScheduler != NULL )
		{
			ImportTaskPtr task = FindTask<ImportTaskList, ImportTaskPtr>(
				sTaskScheduler->mImportTaskQueue, 
				inImportTaskID);
			return (task != NULL);
		}

		return false;
	}

	void TaskScheduler::ReleaseInstance()
	{
		if (sTaskScheduler != NULL)
		{
			sTaskScheduler.reset();
		}
	}

	bool TaskScheduler::Shutdown(bool isForce)
	{
		if ( sTaskScheduler != NULL )
		{
			if ( sTaskScheduler->IsDone() )
			{
				return true;
			}

			if ( isForce )
			{
				sTaskScheduler->Cancel();
				return true;
			}

			bool needResume = sTaskScheduler->GetState() == kSchedulerState_Running;
			if ( needResume )
			{
				sTaskScheduler->Pause();
			}

			ASL::String warnUserCaption, warnUserMessage;
			warnUserCaption = dvacore::ZString(
				"$$$/Prelude/FrontEnd/JobBusyCaption=Ingest in progress");
			warnUserMessage = dvacore::ZString(
				"$$$/Prelude/FrontEnd/JobBusyMessage=Ingest in progress, do you still want to quit Prelude?");
			UIF::MBResult::Type result = UIF::MessageBox(
				warnUserMessage,
				warnUserCaption,
				UIF::MBFlag::kMBFlagYesNo,
				UIF::MBIcon::kMBIconWarning);

			if (result != UIF::MBResult::kMBResultYes)
			{
				if (needResume)
				{
					sTaskScheduler->Resume();
				}
				return false;
			}
			else
			{
				sTaskScheduler->Cancel();
			}
		}

		return true;
	}

	bool TaskScheduler::HasTaskRunning()
	{
		if ( sTaskScheduler != NULL && !sTaskScheduler->IsDone() )
		{
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------------------
	//				Scheduler State Machine by external command
	// ------------------------------------------------------------------------
	// CurrentState					Action					NextState
	// ------------------------------------------------------------------------
	// Init							Start					Running
	// Init							Pause					Paused
	// Init							Cancel					Init
	// Running						Pause					Paused
	// Running						Cancel					Init
	// Running						Start					Running
	// Paused						Resume					Running
	// Paused						Start					Paused
	// Paused						Cancel					Init
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------

	bool TaskScheduler::Start(ASL::Guid const& inBatchID, ASL::String const& inBinID)
	{
		if ( IsDone() )
		{
			return false;
		}

		if(mSchedulerState == kSchedulerState_Init)
		{
			mSchedulerState = kSchedulerState_Running;
			ASL::StationUtils::AddListener(MZ::EncoderManager::Instance()->GetStationID(), this);

			ASL::StationUtils::BroadcastMessage(
				kStation_IngestMedia, 
				IngestStartMessage());

			SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestTaskStarted(sJobID);
		}

		SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestBatchStarted(sJobID, inBatchID, inBinID);

		switch (mSchedulerState)
		{
		case kSchedulerState_Running:

			StartCopyTask();
			StartUpdateMetadataTask();
			StartImportTask();
			StartTranscodeTask();
			StartConcatenateTask();
			break;

		case kSchedulerState_Paused:
			break;

		default:
			DVA_ASSERT_MSG(0, DVA_STR("Should never come here!"));
			break;
		}
		return true;
	}

	bool TaskScheduler::Cancel()
	{
		ASL::StationUtils::RemoveListener(MZ::EncoderManager::Instance()->GetStationID(), this);

		// Cancel the Import operation and operation will send XMP request failure to client
		CancelCurrentOperation();

		if ( mTranscodeTaskQueue.size() > 0 )
		{
			BOOST_FOREACH(TranscodeTaskPtr task, mTranscodeTaskQueue)
			{
				MZ::EncoderManager::Instance()->CancelJob(task->mTaskID.AsString(), task->mTranscodeJobID);
			}
		}
		if ( mConcatenateTaskQueue.size() > 0 )
		{
			BOOST_FOREACH(ConcatenateTaskPtr task, mConcatenateTaskQueue)
			{
				MZ::EncoderManager::Instance()->CancelJob(task->mTaskID.AsString(), task->mTranscodeJobID);
			}
		}

		switch (mSchedulerState)
		{
		case kSchedulerState_Init:
		case kSchedulerState_Running:
		case kSchedulerState_Paused:
			CancelTask<CopyTaskList>(mCopyTaskQueue);
			CancelTask<UpdateMetadataTaskList>(mUpdateMetadataTaskQueue);
			CancelTask<ImportTaskList>(mImportTaskQueue);
			CancelTask<TranscodeTaskList>(mTranscodeTaskQueue);
			CancelTask<ConcatenateTaskList>(mConcatenateTaskQueue);

			mCopyTaskQueue.clear();
			mUpdateMetadataTaskQueue.clear();
			mImportTaskQueue.clear();
			mTranscodeTaskQueue.clear();
			mConcatenateTaskQueue.clear();
			ReleaseCopyExecutor();
			ReleaseUpdateMetadataExecutor();
			break;

		default:
			DVA_ASSERT_MSG(0, DVA_STR("Should never come here!"));
			break;
		}

		Reset();
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestFinishMessage());

		BOOST_FOREACH(const ASL::Guid& batchID, mBatchGuidSet)
		{
			SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestBatchFinished(sJobID, batchID);
		}
		mBatchGuidSet.clear();

		SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestTaskCancelled(sJobID);

		return true;
	}

	bool TaskScheduler::Pause()
	{
		PauseCurrentOperation();

		if ( mTranscodeTaskQueue.size() > 0 || mConcatenateTaskQueue.size() > 0)
		{
			MZ::EncoderManager::Instance()->PauseAMEServer(true);
		}

		switch (mSchedulerState)
		{
		case kSchedulerState_Init:
		case kSchedulerState_Running:
			PauseTask<CopyTaskList>(mCopyTaskQueue);
			PauseTask<UpdateMetadataTaskList>(mUpdateMetadataTaskQueue);
			PauseTask<ImportTaskList>(mImportTaskQueue);
			PauseTask<TranscodeTaskList>(mTranscodeTaskQueue);
			PauseTask<ConcatenateTaskList>(mConcatenateTaskQueue);

			break;

		default:
			DVA_ASSERT_MSG(0, DVA_STR("Should never come here!"));
			break;
		}

		mSchedulerState = kSchedulerState_Paused;

		PL::TaskIDList sTaskIDList;
		sTaskIDList.push_back(sJobID);
		SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestTaskPaused(sTaskIDList);

		return true;
	}

	void TaskScheduler::Done()
	{
		// We don't have finish state here, so reuse init.
		if (mSchedulerState != kSchedulerState_Init)
		{
			CompleteCurrentOperation();

			BOOST_FOREACH(const ASL::Guid& batchID, mBatchGuidSet)
			{
				SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestBatchFinished(sJobID, batchID);
			}
			mBatchGuidSet.clear();

			ASL::StationUtils::RemoveListener(MZ::EncoderManager::Instance()->GetStationID(), this);
			SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestTaskFinished(sJobID);
			ASL::StationUtils::BroadcastMessage(kStation_IngestMedia, IngestFinishMessage());

			ASL::String msg;
			if (mFailedCopyFileCount != 0)
			{
				msg = dvacore::ZString(
					"$$$/Prelude/Mezzanine/IngestScheduler/ReportCopyFail=Copy file(s), total file count: @0, failed file count: @1.\n",
					dvacore::utility::CoerceAsString::Result(mFailedCopyFileCount + mSucceededCopyFileCount),
					dvacore::utility::CoerceAsString::Result(mFailedCopyFileCount));
			}
			if (mFailedUpdateMetadataCount != 0)
			{
				msg = dvacore::ZString(
					"$$$/Prelude/Mezzanine/IngestScheduler/ReportUpdateMetadataFail=Update metadata(s), total file count: @0, failed file count: @1.\n",
					dvacore::utility::CoerceAsString::Result(mFailedUpdateMetadataCount + mSucceededUpdateMetadataCount),
					dvacore::utility::CoerceAsString::Result(mFailedUpdateMetadataCount));
			}
			if (mFailedTranscodeFileCount != 0)
			{
				msg += dvacore::ZString(
					"$$$/Prelude/Mezzanine/IngestScheduler/ReportTranscodeFail=Transcode file(s), total file count: @0, failed file count: @1.\n",
					dvacore::utility::CoerceAsString::Result(mFailedTranscodeFileCount + mSucceededTranscodeFileCount),
					dvacore::utility::CoerceAsString::Result(mFailedTranscodeFileCount));
			}
			if (mFailedConcatenateFileCount != 0)
			{
				msg += dvacore::ZString(
					"$$$/Prelude/Mezzanine/IngestScheduler/ReportConcatenateFail=Concatenate file(s), total file count: @0, failed file count: @1.\n",
					dvacore::utility::CoerceAsString::Result(mFailedConcatenateFileCount + mSucceededConcatenateFileCount),
					dvacore::utility::CoerceAsString::Result(mFailedConcatenateFileCount));
			}
			if (mFailedImportFileCount != 0)
			{
				msg += dvacore::ZString(
					"$$$/Prelude/Mezzanine/IngestScheduler/ReportImportFail=Import file(s), total file count: @0, failed file count: @1.\n",
					dvacore::utility::CoerceAsString::Result(mFailedImportFileCount + mSucceededImportFileCount),
					dvacore::utility::CoerceAsString::Result(mFailedImportFileCount));
			}
			if (!msg.empty())
			{
				msg = dvacore::ZString("$$$/Prelude/Mezzanine/ReportIngestFailMessage=One or more errors occurred during ingest. Please check the error event(s) in Events Panel.\n") + msg;
				PL::SRUtilitiesPrivate::PromptForError(
					dvacore::ZString("$$$/Prelude/Mezzanine/ReportFailCaption=Ingest Failure Warning"),
					msg);
			}

			Reset();
		}
	}

	bool TaskScheduler::Resume()
	{
		ResumeCurrentOperation();
		if ( mTranscodeTaskQueue.size() > 0 || mConcatenateTaskQueue.size() > 0 )
		{
			MZ::EncoderManager::Instance()->PauseAMEServer(false);
		}

		switch (mSchedulerState)
		{
		case kSchedulerState_Paused:
			ResumeTask<CopyTaskList>(mCopyTaskQueue);
			ResumeTask<UpdateMetadataTaskList>(mUpdateMetadataTaskQueue);
			ResumeTask<ImportTaskList>(mImportTaskQueue);
			ResumeTask<TranscodeTaskList>(mTranscodeTaskQueue);
			ResumeTask<ConcatenateTaskList>(mConcatenateTaskQueue);
			break;

		default:
			DVA_ASSERT_MSG(0, DVA_STR("Should never come here!"));
			break;
		}

		mSchedulerState = kSchedulerState_Running;

		StartCopyTask();
		StartImportTask();
		StartTranscodeTask();
		StartConcatenateTask();

		PL::TaskIDList sTaskIDList;
		sTaskIDList.push_back(sJobID);
		SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestTaskResumed(sTaskIDList);

		return true;
	}

	bool TaskScheduler::IsDone() const
	{
		return mCopyTaskQueue.empty()
			&& mUpdateMetadataTaskQueue.empty()
			&& mImportTaskQueue.empty()
			&& mTranscodeTaskQueue.empty()
			&& mConcatenateTaskQueue.empty();
	}

	SchedulerState TaskScheduler::GetState() const
	{
		return mSchedulerState;
	}

	void TaskScheduler::StartCopyTask()
	{
		if ( mCopyTaskQueue.empty() || IncludeRunningTask(mCopyTaskQueue) )
		{
			return;
		}

		bool canceled = false;
		// Start one Copy task
		CopyTaskList::iterator copyItr = mCopyTaskQueue.begin();
		CopyTaskList::iterator copyEnd = mCopyTaskQueue.end();
		CopyTaskPtr task;
		for (; copyItr != copyEnd; ++copyItr)
		{
			task = *copyItr;
			DVA_ASSERT(task != NULL);
			if ( task->mTaskState == kTaskState_Init && !task->mCopySetting.mCopyUnits.empty() )
			{
				task->mTaskState = kTaskState_Running;

				// Push this task to thread queue for execution

				// Precheck if the dest folder or file is on disk in main thread.
				// [TODO] need to walk all files rather than only the front.
				if (!GenerateCopyAction(task))
				{
					canceled = true;
					break;
				}

				//[ToDo] This is risky because the process in another thread may still use this 
				// when it is destructed from task scheduler.
				// Need use com-like interface to start the request (refer to MBC and MediaBrowser)
				mCopyOperation = CopyOperationPtr(new CopyOperation(
									mStationID, 
									*copyItr,
									mCopyRunnerSetting,
									this));
				CreateOrGetCopyExecutor()->CallAsynchronously(
					boost::bind(&DoCopyOperation, mCopyOperation));

				// Copy task has to run sequentially
				break;
			}
		}

		if (canceled)
		{
			Cancel();
		}
	}

	void TaskScheduler::StartUpdateMetadataTask()
	{
		if ( mUpdateMetadataTaskQueue.empty() )
		{
			return;
		}

		// Start one update metadata task
		BOOST_FOREACH (UpdateMetadataTaskPtr task, mUpdateMetadataTaskQueue)
		{
			DVA_ASSERT(task != NULL);
			// allow UpdateMetadataTask execute with nothing to do if no necessary rename or custom metadata.
			// [TODO] we can optimize to avoid such execution and refine performance a little in future.
			if ( task->mTaskState == kTaskState_Init )
			{
				task->mTaskState = kTaskState_Running;

				// Push this task to thread queue for execution
				mUpdateMetadataOperation = UpdateMetadataOperationPtr(new UpdateMetadataOperation(
					mStationID, 
					task,
					this));
				CreateOrGetUpdateMetadataExecutor()->CallAsynchronously(
					boost::bind(&DoUpdateMetadataOperation, mUpdateMetadataOperation));
			}
		}
	}

	void TaskScheduler::StartImportTask()
	{
		if ( mImportTaskQueue.empty())
		{
			return;
		}

		if (NULL == mImportOperation)
		{
			mImportOperation = BaseOperationPtr(new ImporterTaskExecutor(sJobID, mStationID));
		}

		mImportOperation->Process();

		// Start one Import task
		ImportTaskList::iterator importItr = mImportTaskQueue.begin();
		ImportTaskList::iterator importEnd = mImportTaskQueue.end();
		for (; importItr != importEnd; ++importItr)
		{
			if ( (*importItr)->mTaskState == kTaskState_Init )
			{
				(*importItr)->mTaskState = kTaskState_Running;

				PL::IngestItemList ingestItems;
				const ImportPathInfoMap& srcFiles = (*importItr)->GetImportSetting().mSrcFiles;
				BOOST_FOREACH (const ImportPathInfoMap::value_type& pairValue, srcFiles)
				{
					pairValue.second->mItemFormat = PL::Utilities::ConvertProviderIDToFormat(pairValue.second->mItemFormat);
					ingestItems.push_back(pairValue.second);
				}
				SRProject::GetInstance()->GetAssetLibraryNotifier()->ImportItemsAreReady(
															sJobID, 
															(*importItr)->GetBatchID(),
															(*importItr)->GetTaskID(), 
															ingestItems,
															(*importItr)->GetCustomData());
			}
		}
	}

	void TaskScheduler::ResumeCurrentOperation()
	{
		if (NULL != mImportOperation)
		{
			mImportOperation->Resume();
		}
		if (NULL != mCopyOperation)
		{
			mCopyOperation->Resume();
		}
		if (NULL != mUpdateMetadataOperation)
		{
			mUpdateMetadataOperation->Resume();
		}
	}

	void TaskScheduler::PauseCurrentOperation()
	{
		if (NULL != mImportOperation)
		{
			mImportOperation->Pause();
		}

		if (NULL != mCopyOperation)
		{
			mCopyOperation->Pause();
		}

		if (NULL != mUpdateMetadataOperation)
		{
			mUpdateMetadataOperation->Pause();
		}
	}

	void TaskScheduler::CancelCurrentOperation()
	{
		if (NULL != mImportOperation)
		{
			mImportOperation->Cancel();
		}
		if (NULL != mCopyOperation)
		{
			mCopyOperation->Cancel();
		}
		if (NULL != mUpdateMetadataOperation)
		{
			mUpdateMetadataOperation->Cancel();
		}
	}

	void TaskScheduler::CompleteCurrentOperation()
	{
		if (NULL != mImportOperation)
		{
			mImportOperation->Done();
		}
		if (NULL != mCopyOperation)
		{
			mCopyOperation->Done();
		}
		if (NULL != mUpdateMetadataOperation)
		{
			mUpdateMetadataOperation->Done();
		}
	}

	void TaskScheduler::StartTranscodeTask()
	{
		// Add all transcode tasks to AME queue without any blocking in Prelude side.
		if ( mTranscodeTaskQueue.empty() /* || IncludeRunningTask(mTranscodeTaskQueue)*/ )
		{
			return;
		}

		// Don't need test if any transcode task is running because they can managed by AME queue.

		// Start all Transcode Task because AME has a queue in another process
		TranscodeTaskList::iterator transcodeItr = mTranscodeTaskQueue.begin();
		TranscodeTaskList::iterator transcodeEnd = mTranscodeTaskQueue.end();
		TranscodeTaskList errorTasks;

		TranscodeTaskPtr task;
		for (; transcodeItr != transcodeEnd; ++transcodeItr)
		{
			task = *transcodeItr;
			if ( task != NULL && task->mTaskState == kTaskState_Init )
			{
				task->mTaskState = kTaskState_Running;

				// It should be only one file here
				ClipSetting srcClipSetting(task->mTranscodeSetting.mSrcFileSettings.front()); 
				const ASL::String& srcFile = srcClipSetting.mFileName;
				dvacore::UTF16String errorInfo;

				if ( ASL::Directory::IsDirectory(srcFile) )
				{
					// Cannot transcode an folder
					errorInfo = dvacore::ZString(
						"$$$/Prelude/Mezzanine/IngestScheduler/TranscodeTaskFolderFail=Cannot transcode a folder!");
					errorTasks.push_back(task);
					HandleTranscodeTaskError(task, errorInfo);
					continue;
				}

				ASL::String relativePath(RemoveRootFromPath(srcFile, task->mTranscodeSetting.mSrcRootPath));
				relativePath = MZ::Utilities::StripHeadingSlash(relativePath);
				DVA_ASSERT(!relativePath.empty());
				if ( relativePath.empty() )
				{
					// Cannot get a relative path
					errorInfo = dvacore::ZString(
						"$$$/Prelude/Mezzanine/IngestScheduler/TranscodeTaskRelativePathFail=Cannot get the right source path!");
					errorTasks.push_back(task);
					HandleTranscodeTaskError(task, errorInfo);
					continue;
				}

				ASL::String destPath(ASL::PathUtils::CombinePaths(MZ::Utilities::NormalizePathWithoutUNC(
					task->mTranscodeSetting.mDestFolder), 
					relativePath));

				destPath = ASL::PathUtils::GetFullDirectoryPart(destPath);

				if (task->mTranscodeSetting.GetPreset() == NULL)
				{
					task->mTranscodeSetting.SetPreset(PL::Utilities::CreateCustomPresetForMedia(srcFile, task->mTranscodeSetting.GetExportModule()));
				}
				ASL::String presetFile(PL::Utilities::SavePresetToFile(task->mTranscodeSetting.GetPreset()));
				
				if ( !PL::IngestUtils::TranscodeMediaFile(
					BE::kCompileSettingsType_Movie,
					srcFile, 
					destPath,
					ASL::String(),
					presetFile,
					srcClipSetting.mInPoint,
					srcClipSetting.mOutPoint,
					task->mTaskID.AsString(),
					task->mTranscodeJobID,
					errorInfo) )
				{
					errorTasks.push_back(task);
					HandleTranscodeTaskError(task, errorInfo);
				}
				else
				{
					if (task->mTranscodeSetting.mShouldAutoDeleteAfterIngested)
					{
						// In EA mode, should add the destination file into mTranscodeTempFiles so that these temp files can be deleted in time.
						mTranscodeTempFiles.push_back(destPath);
					}

					//break;
				}
			}
		}

		bool needRestartTasks = (errorTasks.size() == mTranscodeTaskQueue.size());
		transcodeItr = errorTasks.begin();
		transcodeEnd = errorTasks.end();
		for (; transcodeItr != transcodeEnd; ++ transcodeItr)
		{
			Remove(*transcodeItr);
		}

		// RunTasks should be triggered by AME message. However, if all tasks fail to be started, 
		// We should trigger this ourselves. 
		if ( needRestartTasks )
		{
			RunTasks();
		}
	}

	void TaskScheduler::StartConcatenateTask()
	{
		// Add all concatenate tasks to AME queue without any blocking in Prelude side.
		if ( mConcatenateTaskQueue.empty() )
		{
			return;
		}

		// Don't need test if any concatenate task is running because they can managed by AME queue.

		// Start all Concatenate Task because AME has a queue in another process
		ConcatenateTaskList::iterator concatenateItr = mConcatenateTaskQueue.begin();
		ConcatenateTaskList::iterator concatenateEnd = mConcatenateTaskQueue.end();
		ConcatenateTaskList errorTasks;

		ConcatenateTaskPtr task;
		for (; concatenateItr != concatenateEnd; ++concatenateItr)
		{
			task = *concatenateItr;
			if ( task != NULL && task->mTaskState == kTaskState_Init )
			{
				task->mTaskState = kTaskState_Running;

				// It's OK to only get front here:
				//   For concatenation, the dest path will be decided based on source file path and source root path,
				//   and the source root path was gotten from the 1st clip, so we should also get source file path of the 1st file.
				ClipSetting srcClipSetting(task->mTranscodeSetting.mSrcFileSettings.front()); 
				const ASL::String& srcFile = srcClipSetting.mFileName;

				ASL::String relativePath(RemoveRootFromPath(srcFile, task->mTranscodeSetting.mSrcRootPath));
				relativePath = MZ::Utilities::StripHeadingSlash(relativePath);
				DVA_ASSERT(!relativePath.empty());
				if ( relativePath.empty() )
				{
					// Cannot get a relative path
					ASL::String const& errorInfo = dvacore::ZString(
						"$$$/Prelude/Mezzanine/IngestScheduler/ConcatenateTaskRelativePathFail=Cannot get the right source path!");
					errorTasks.push_back(task);
					HandleConcatenateTaskError(task, errorInfo);
					continue;
				}

				ASL::String destPath(ASL::PathUtils::CombinePaths(MZ::Utilities::NormalizePathWithoutUNC(
					task->mTranscodeSetting.mDestFolder), 
					relativePath));

				destPath = ASL::PathUtils::GetFullDirectoryPart(destPath);

				PathToErrorInfoVec errorInfos;
				BE::IProjectRef theProject;
				BE::ISequenceRef theSequence;
				if (!IngestUtils::CreateSequenceForConcatenateMedia(task->mTranscodeSetting.mSrcFileSettings, theProject, theSequence, task->mTranscodeSetting.GetConcatenationName(), errorInfos))
				{
					errorTasks.push_back(task);
					HandleConcatenateTaskError(task, errorInfos);
					continue;
				}
				if (task->mTranscodeSetting.GetPreset() == NULL)
				{
					task->mTranscodeSetting.SetPreset(PL::Utilities::CreateCustomPresetForSequence(theSequence, task->mTranscodeSetting.GetExportModule()));
				}

				task->SetProject(theProject);

				if ( !PL::IngestUtils::TranscodeConcatenateFile(
					task->mTaskID.AsString(),
					task->mTranscodeJobID,
					task->mTranscodeSetting.GetPreset(),
					theProject,
					theSequence,
					destPath,
					task->mTranscodeSetting.GetConcatenationName(),
					errorInfos) )
				{
					errorTasks.push_back(task);
					HandleConcatenateTaskError(task, errorInfos);
				}

				if (task->mTranscodeSetting.mShouldAutoDeleteAfterIngested)
				{
					// In EA mode, should add the destination file into mTranscodeTempFiles so that these temp files can be deleted in time.
					mTranscodeTempFiles.push_back(destPath);
				}
			}
		}

		bool needRestartTasks = errorTasks.size() == mConcatenateTaskQueue.size();
		concatenateItr = errorTasks.begin();
		concatenateEnd = errorTasks.end();
		for (; concatenateItr != concatenateEnd; ++ concatenateItr)
		{
			Remove(*concatenateItr);
		}

		// RunTasks should be triggered by AME message. However, if all tasks fail to be started, 
		// We should trigger this ourselves. 
		if ( needRestartTasks )
		{
			RunTasks();
		}
	}

	void TaskScheduler::OnCopyTaskProgress(double inPercent)
	{
		mCopyProgress = inPercent;

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::OnCopyTaskFinished(
				const ASL::Guid& inTaskID, 
				ASL::Result inResult, 
				const CopyResultVector& inCopyResults,
				std::size_t inSucessCount,
				std::size_t inFailedCount)
	{
		CopyTaskPtr task = FindTask<CopyTaskList, CopyTaskPtr>(mCopyTaskQueue, inTaskID);
		if (task != NULL)
		{
			++mDoneTaskCount;
			task->mTaskState = kTaskState_Done;

			mSucceededCopyFileCount += inSucessCount;
			mFailedCopyFileCount += inFailedCount;

			BOOST_FOREACH(const CopyResultVector::value_type& copyResult, inCopyResults)
			{
				// report task finish
				TaskState state = ASL::ResultSucceeded(copyResult.first) ? kTaskState_Done : kTaskState_Failure;
				ASL::String msg(dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/CopyTask=Copy task - ") + copyResult.second);
				ASL::StationUtils::BroadcastMessage(
					kStation_IngestMedia, 
					IngestTaskStatusMessage(task->GetTaskID(), state, msg));
			}
		}

		mCopyProgress = 0.0f;

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));

		Remove(task);

		if (mCopyTaskQueue.empty())
		{
			ReleaseCopyExecutor();
		}

		RunTasks();
	}

	void TaskScheduler::OnUpdateMetadataTaskFinished(
		const ASL::Guid& inTaskID, 
		ASL::Result inResult, 
		const ResultReportVector& inResults)
	{
		UpdateMetadataTaskPtr task = FindTask<UpdateMetadataTaskList, UpdateMetadataTaskPtr>(mUpdateMetadataTaskQueue, inTaskID);
		if (task != NULL)
		{
			++mDoneTaskCount;
			task->mTaskState = kTaskState_Done;

			BOOST_FOREACH(const ResultReportVector::value_type& resultReport, inResults)
			{
				if (ASL::ResultSucceeded(resultReport.first))
				{
					++mSucceededUpdateMetadataCount;
				}
				else
				{
					++mFailedUpdateMetadataCount;
				}

				// report task finish
				TaskState state = ASL::ResultSucceeded(resultReport.first) ? kTaskState_Done : kTaskState_Failure;
				ASL::String msg(dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/UpdateMetadataTask=Update Metadata task - ") + resultReport.second);
				ASL::StationUtils::BroadcastMessage(
					kStation_IngestMedia, 
					IngestTaskStatusMessage(task->GetTaskID(), state, msg));
			}
		}

		mUpdateMetadataProgress = 0.0f;

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));

		Remove(task);

		if (mUpdateMetadataTaskQueue.empty())
		{
			ReleaseUpdateMetadataExecutor();
		}

		RunTasks();
	}

	void TaskScheduler::OnImportTaskFinished(const ASL::Guid& inTaskID)
	{
		ImportTaskPtr task = FindTask<ImportTaskList, ImportTaskPtr>(mImportTaskQueue, inTaskID);
		if (task != NULL)
		{
			++mDoneTaskCount;
			task->mTaskState = kTaskState_Done;

			// report task finish (Exception here)
			// Because one import task can deal with multiple files and each file needs report its status
			// No need to report the whole task again to avoid duplicate message in event panel.

			//ASL::StationUtils::BroadcastMessage(
			//	kStation_IngestMedia, 
			//	IngestTaskStatusMessage(
			//		task->GetTaskID(), 
			//		kTaskState_Done,
			//		DVA_STR("Import task ") + task->mTaskID.AsString()+ DVA_STR(" finished!")));
		}

		mImportProgress = 0.0f;
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));

		Remove(task);
		RunTasks();
	}


	void TaskScheduler::HandleTranscodeTaskError(TranscodeTaskPtr inTask, const dvacore::UTF16String& inErrorInfo)
	{
		if (inTask != NULL)
		{
			mDoneTaskCount += inTask->GetTaskUnitCount();

			++mFailedTranscodeFileCount;
			inTask->mTaskState = kTaskState_Failure;

			// report task failure
			const ClipSettingList& settings = inTask->mTranscodeSetting.mSrcFileSettings;
			DVA_ASSERT(!settings.empty());
			dvacore::UTF16String msg = dvacore::ZString(
				"$$$/Prelude/Mezzanine/IngestScheduler/TranscodeTaskFail=Transcode task - transcode \"@0\" failed. Reason: @1");
			msg = dvacore::utility::ReplaceInString(
				msg, 
				MZ::Utilities::NormalizePathWithoutUNC(settings.front().mFileName),
				inErrorInfo);

			ASL::StationUtils::BroadcastMessage(
				kStation_IngestMedia, 
				IngestTaskStatusMessage(inTask->GetTaskID(), kTaskState_Failure, msg));
		}

		mTranscodeProgress = 0.0f;

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::HandleConcatenateTaskError(ConcatenateTaskPtr inTask, const PathToErrorInfoVec& inErrorInfos)
	{
		if (inTask != NULL)
		{
			ASL_ASSERT(!inErrorInfos.empty());

			mDoneTaskCount += inTask->GetTaskUnitCount();

			inTask->mTaskState = kTaskState_Failure;

			dvacore::UTF16String msg = dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/ConcatenateTaskFailedTitle=Concatenate task -");

			msg += FormatConcatenateError(inErrorInfos);
			mFailedConcatenateFileCount += inErrorInfos.size();

			// [TRICKY] Not the actual succeeded concatenate file number, just to get the value of error msg in Event Panel.
			// Because once a file is not concatenated successfully, there won't be any successful concatenated files.
			mSucceededConcatenateFileCount = inTask->mTranscodeSetting.mSrcFileSettings.size() - mFailedConcatenateFileCount;
			ASL::StationUtils::BroadcastMessage(
				kStation_IngestMedia, 
				IngestTaskStatusMessage(inTask->GetTaskID(), kTaskState_Failure, msg));
		}

		mConcatenateProgress = 0.0f;

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::HandleConcatenateTaskError(ConcatenateTaskPtr inTask, const dvacore::UTF16String& inErrorInfo)
	{
		if (inTask != NULL)
		{
			PathToErrorInfoVec errorInfos;
			errorInfos.push_back(make_pair(inTask->mTranscodeSetting.GetConcatenationName(), inErrorInfo));
			HandleConcatenateTaskError(inTask, errorInfos);
		}
	}

	void TaskScheduler::OnAMEServerOffline()
	{
		ASL::String const& AMEOfflineError = dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/AMEOffline=AME is offline!");
		BOOST_FOREACH(TranscodeTaskPtr task, mTranscodeTaskQueue)
		{
			HandleTranscodeTaskError(
				task, 
				AMEOfflineError);
		}
		mTranscodeTaskQueue.clear();

		BOOST_FOREACH(ConcatenateTaskPtr task, mConcatenateTaskQueue)
		{
			HandleConcatenateTaskError(task, AMEOfflineError);
		}
		mConcatenateTaskQueue.clear();

		RunTasks();
	}

	void TaskScheduler::OnTranscodeTaskError(
							RequesterID					inRequestID,
							JobID						inJobID,
							dvacore::UTF16String		inEncoderID,
							const dvacore::UTF16String&	inErrorInfo)
	{
		DVA_TRACE( "TaskScheduler::OnTranscodeError", 5, 
			" TaskID: " << inJobID );

		TranscodeTaskPtr transcodeTask = FindTask<TranscodeTaskList, TranscodeTaskPtr>(mTranscodeTaskQueue, ASL::Guid(inRequestID));
		HandleTranscodeTaskError(transcodeTask, inErrorInfo);
		Remove(transcodeTask);

		ConcatenateTaskPtr concatenateTask = FindTask<ConcatenateTaskList, ConcatenateTaskPtr>(mConcatenateTaskQueue, ASL::Guid(inRequestID));
		HandleConcatenateTaskError(concatenateTask, inErrorInfo);
		Remove(concatenateTask);

		RunTasks();
	}

	void TaskScheduler::OnTranscodeTaskStatus(
							RequesterID				inRequestID,
							JobID					inJobID,
							dvacore::UTF16String	inEncoderID, /* encoderID */
							dvametadatadb::StatusType inStatus)
	{
		TranscodeResult failure;
		IngestUtils::TranscodeStatusToString(inStatus, failure);
		DVA_TRACE( "TaskScheduler::OnTranscodeStatus", 5, 
			" TaskID: " << inRequestID << 
			", inStatus: " << failure.second );
	}

	void TaskScheduler::OnTranscodeTaskProgress(
							RequesterID				inRequestID,
							JobID					inJobID,
							double					inPercent,
							dvacore::UTF16String	inEncoderID)
	{
		//DVA_TRACE( "TaskScheduler::OnTranscodeProgress", 5, 
		//	" inEncoderID: " << inEncoderID << 
		//	", Progress: " << ASL::CoerceAsString::Result<double>(inProgressValue) );

		ASL::Guid taskID(inRequestID);
		{
			TranscodeTaskPtr task = FindTask<TranscodeTaskList, TranscodeTaskPtr>(mTranscodeTaskQueue, taskID);
			if (task != NULL)
			{
				mTranscodeProgress = inPercent;
			}
		}

		{
			ConcatenateTaskPtr task = FindTask<ConcatenateTaskList, ConcatenateTaskPtr>(mConcatenateTaskQueue, taskID);
			if (task != NULL)
			{
				mConcatenateProgress = inPercent;
			}
		}

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::CreateSubsequentTaskAfterTranscode(
		TranscodeTaskPtr			inTask,
		dvacore::UTF16String const&	inOutputFilePath)
	{
		PL::IngestItemPtr ingestItem(new PL::IngestItem(inOutputFilePath));
		ASL::String folderPath;
		ASL::PathnameList extraPathList;
		PL::GetLogicalClipFromMediaPath(inOutputFilePath, folderPath, extraPathList, ingestItem->mAliasName, ingestItem->mItemFormat);
		BOOST_FOREACH(const ASL::String& path, extraPathList)
		{
			ASL::String pathTemp = MZ::Utilities::NormalizePathWithoutUNC(path);
			if (pathTemp != inOutputFilePath)
				ingestItem->mExtraPathList.push_back(pathTemp);
		}
		bool isSingleClip = PL::Utilities::ConvertProviderIDToFormat(ingestItem->mItemFormat) == kSingleFileFormat;

		// First check if need update metadata, if yes, fist create update metadata task
		UpdateMetadataSetting setting;
		setting.mNeedDoFileRename = false;

		// right now transcode task should has and only has one clip setting.
		ClipSetting& clipSetting = inTask->mTranscodeSetting.mSrcFileSettings.front();
		if (!clipSetting.mAliasName.empty() && clipSetting.mIsRenamePresetAvailable)
		{
			if (isSingleClip)
			{
				setting.mNeedDoFileRename = true;

				int index(1);
				ASL::String importPath = Utilities::ReplaceFilePartOfPath(inOutputFilePath, clipSetting.mAliasName);
				ASL::String uniqueTargetFullPath = importPath;
				while (ASL::PathUtils::ExistsOnDisk(uniqueTargetFullPath))
				{
					ASL::String drive, directory, file, extension;
					ASL::PathUtils::SplitPath(importPath, &drive, &directory, &file, &extension);
					
					file += DVA_STR("_");
					file += dvacore::utility::CoerceAsString::Result<std::uint64_t>(index++);
					uniqueTargetFullPath = ASL::PathUtils::MakePath(drive, directory, file, extension);
				}
				clipSetting.mAliasName = ASL::PathUtils::GetFilePart(uniqueTargetFullPath);
				ingestItem->mFilePath = uniqueTargetFullPath;
				ASL::String xmpPath = Utilities::GetXMPPath(inOutputFilePath);
				if (!xmpPath.empty() && xmpPath != inOutputFilePath)
				{
					ASL::String newFilePath = Utilities::RenameSideCarFile(
						xmpPath,
						ASL::PathUtils::GetFilePart(inOutputFilePath),
						clipSetting.mAliasName);
					if (newFilePath != xmpPath)
					{
						setting.mAliasNameMap[xmpPath] = ASL::PathUtils::GetFilePart(newFilePath);
					}
				}
			}
			setting.mAliasNameMap[inOutputFilePath] = clipSetting.mAliasName;
			ingestItem->mAliasName = clipSetting.mAliasName;
		}

		if (!setting.mAliasNameMap.empty() || !inTask->GetCustomData().mMetadata.empty())
		{
			setting.mNeedImportFiles[ingestItem->mFilePath] = ingestItem;
			setting.mNeedCreateImportTask = inTask->mTranscodeSetting.mNeedImport;
			TaskFactory::SubmitUpdateMetadataTask(
				setting,
				inTask->GetBatchID(),
				inTask->GetCustomData());
		}
		else
		{
			if (inTask->mTranscodeSetting.mNeedImport)
			{
				// If no update metadata, create import task directly
				ImportSetting setting;
				setting.mSrcFiles[inOutputFilePath] = ingestItem;
				TaskFactory::SubmitImportTask(setting, inTask->GetBatchID(), inTask->GetCustomData());
			}
			else
			{
				PL::IngestItemList backupItemList;
				backupItemList.push_back(ingestItem);
				NotifyBackupItemsAreReady(sJobID, inTask->GetBatchID(), backupItemList, inTask->GetCustomData());
			}
		}
	
		// Need decrease the placeholder import task created before the copy task starts
		// to avoid adding duplicated import task
		--mTotalTaskCount;
	}

	void TaskScheduler::CreateSubsequentTaskAfterConcatenate(
		ConcatenateTaskPtr			inTask,
		dvacore::UTF16String const&	inOutputFilePath)
	{
		PL::IngestItemPtr ingestItem(new PL::IngestItem(inOutputFilePath));
		ASL::String folderPath;
		ASL::PathnameList extraPathList;
		PL::GetLogicalClipFromMediaPath(inOutputFilePath, folderPath, extraPathList, ingestItem->mAliasName, ingestItem->mItemFormat);
		BOOST_FOREACH(const ASL::String& path, extraPathList)
		{
			ASL::String pathTemp = MZ::Utilities::NormalizePathWithoutUNC(path);
			if (pathTemp != inOutputFilePath)
				ingestItem->mExtraPathList.push_back(pathTemp);
		}

		// First check if need update metadata, if yes, fist create update metadata task
		// If rename preset is available, should rename to alias name which has been calculated before concatenate.
		ClipSetting& clipSetting = inTask->mTranscodeSetting.mSrcFileSettings.front();
		bool isRenamePresetAvailable = clipSetting.mIsRenamePresetAvailable;
		if (isRenamePresetAvailable || !inTask->GetCustomData().mMetadata.empty())
		{
			UpdateMetadataSetting setting;
			// No physical file rename, because concatinate in AME has used final name applied with rename preset.
			setting.mNeedDoFileRename = false;
			if (isRenamePresetAvailable)
			{
				if (clipSetting.mAliasName.empty())
					ingestItem->mAliasName = inTask->mTranscodeSetting.GetConcatenationName();
				else
					ingestItem->mAliasName = clipSetting.mAliasName;
				setting.mAliasNameMap[inOutputFilePath] = ingestItem->mAliasName;
			}
			setting.mNeedImportFiles[inOutputFilePath] = ingestItem;
			setting.mNeedCreateImportTask = inTask->mTranscodeSetting.mNeedImport;
			TaskFactory::SubmitUpdateMetadataTask(
				setting,
				inTask->GetBatchID(),
				inTask->GetCustomData());
		}
		else
		{
			// If no update metadata, create import task directly
			ImportSetting setting;
			setting.mSrcFiles[inOutputFilePath] = ingestItem;
			TaskFactory::SubmitImportTask(setting, inTask->GetBatchID(), inTask->GetCustomData());
		}

		// Need decrease the placeholder import task created before the copy task starts
		// to avoid adding duplicated import task
		--mTotalTaskCount;
	}

	void TaskScheduler::TranscodeTaskFinishedImpl(
		TranscodeTaskPtr		inTask,
		JobID					inJobID,
		dvacore::UTF16String	inOutputFilePath,
		dvacore::UTF16String	inEncoderID)
	{
		++mDoneTaskCount;
		++mSucceededTranscodeFileCount;
		inTask->mTaskState = kTaskState_Done;

		// report task finish
		const ClipSettingList& settings = inTask->mTranscodeSetting.mSrcFileSettings;
		DVA_ASSERT(!settings.empty());
		dvacore::UTF16String msg = dvacore::ZString(
			"$$$/Prelude/Mezzanine/IngestScheduler/TranscodeTaskFinish=Transcode task - transcode from \"@0\" to \"@1\" finished!");
		msg = dvacore::utility::ReplaceInString(
			msg, 
			MZ::Utilities::NormalizePathWithoutUNC(settings.front().mFileName),
			MZ::Utilities::NormalizePathWithoutUNC(inOutputFilePath));

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskStatusMessage(inTask->GetTaskID(), kTaskState_Done,	msg));

		CreateSubsequentTaskAfterTranscode(inTask, inOutputFilePath);

		mTranscodeProgress = 0.0f;
		MZ::EncoderManager::Instance()->RemoveRequest(inTask->mTaskID.AsString(), inTask->mTranscodeJobID);
		Remove(inTask);
	}

	void TaskScheduler::ConcatenateTaskFinishedImpl(
		ConcatenateTaskPtr		inTask,
		JobID					inJobID,
		dvacore::UTF16String	inOutputFilePath,
		dvacore::UTF16String	inEncoderID)
	{
		++mDoneTaskCount;
		++mSucceededConcatenateFileCount;
		inTask->mTaskState = kTaskState_Done;

		// report task finish
		const ClipSettingList& settings = inTask->mTranscodeSetting.mSrcFileSettings;
		DVA_ASSERT(!settings.empty());
		dvacore::UTF16String msg = dvacore::ZString(
			"$$$/Prelude/Mezzanine/IngestScheduler/ConcatenateTaskFinish=Concatenate task - concatenate to \"@0\" finished!");
		msg = dvacore::utility::ReplaceInString(
			msg, 
			MZ::Utilities::NormalizePathWithoutUNC(inOutputFilePath));

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskStatusMessage(inTask->GetTaskID(), kTaskState_Done,	msg));

		if (inTask->mTranscodeSetting.mNeedImport)
		{
			CreateSubsequentTaskAfterConcatenate(inTask, inOutputFilePath);
		}

		mConcatenateProgress = 0.0f;
		MZ::EncoderManager::Instance()->RemoveRequest(inTask->mTaskID.AsString(), inTask->mTranscodeJobID);
		Remove(inTask);
	}

	void TaskScheduler::OnTranscodeTaskReady(
							RequesterID					inRequestID,
							JobID						inJobID,
							dvacore::UTF16String		inOutputFilePath/* output filepath */,
							dvacore::UTF16String		inEncoderID/* encoderID */)
	{
		DVA_TRACE( "TaskScheduler::OnTranscodeTaskReady", 5, 
			" inEncoderID: " << inEncoderID << 
			", inOutputFilePath: " << inOutputFilePath );

		StartTranscodeTask();
	}


	void TaskScheduler::OnTranscodeTaskFinished(
							RequesterID				inRequestID,
							JobID					inJobID,
							dvacore::UTF16String	inOutputFilePath/* output filepath */,
							dvacore::UTF16String	inEncoderID/* encoderID */)
	{
		DVA_TRACE( "TaskScheduler::OnTranscodeFinished", 5, 
			" inEncoderID: " << inEncoderID << 
			", inOutputFilePath: " << inOutputFilePath );

		DVA_ASSERT(!inOutputFilePath.empty());

		ASL::Guid taskID(inRequestID);

		TranscodeTaskPtr task = FindTask<TranscodeTaskList, TranscodeTaskPtr>(mTranscodeTaskQueue, taskID);
		if (task != NULL)
		{
			TranscodeTaskFinishedImpl(task, inJobID, inOutputFilePath, inEncoderID);
		}
		else
		{
			ConcatenateTaskPtr task = FindTask<ConcatenateTaskList, ConcatenateTaskPtr>(mConcatenateTaskQueue, taskID);
			if (task != NULL)
			{
				ConcatenateTaskFinishedImpl(task, inJobID, inOutputFilePath, inEncoderID);
			}
		}

		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));

		RunTasks();
	}

	// Internal scheduler by tasks finish notification
	void TaskScheduler::RunTasks()
	{
		if ( IsDone() )
		{
			Done();
			return;
		}

		switch (mSchedulerState)
		{
		case kSchedulerState_Paused:
			break;

		default:
			StartCopyTask();
			StartUpdateMetadataTask();
			StartImportTask();
			StartTranscodeTask();
			StartConcatenateTask();
			break;
		}
	}

	double TaskScheduler::CalculateProgress() const
	{
		double ongoingProgress = mCopyProgress + mUpdateMetadataProgress + mImportProgress + mTranscodeProgress + mConcatenateProgress;
		double doneProgress = ((double)(mDoneTaskCount) + ongoingProgress)/ (double)mTotalTaskCount;
		return std::min(doneProgress, 1.0);
	}

	void TaskScheduler::Reset()
	{
		mDoneTaskCount			= 0;
		mTotalTaskCount			= 0;  // For single run
		mTranscodeProgress		= 0.0f;
		mConcatenateProgress	= 0.0f;
		mCopyProgress			= 0.0f;
		mImportProgress			= 0.0f;
		mCopyRunnerSetting.Reset();

		mFailedCopyFileCount		= 0;
		mFailedImportFileCount		= 0;
		mFailedUpdateMetadataCount  = 0;
		mFailedTranscodeFileCount	= 0;
		mFailedConcatenateFileCount	= 0;
		mSucceededCopyFileCount		= 0;
		mSucceededImportFileCount	= 0;
		mSucceededUpdateMetadataCount = 0;
		mSucceededTranscodeFileCount = 0;
		mSucceededConcatenateFileCount = 0;

		mSchedulerState = kSchedulerState_Init;
	}

	void TaskScheduler::AddBatchID(const ASL::Guid& inBatchID)
	{
		mBatchGuidSet.insert(inBatchID);
	}

	void TaskScheduler::RemoveBatchID(const ASL::Guid& inBatchID)
	{
		mBatchGuidSet.erase(inBatchID);
	}

	void TaskScheduler::CreateSubsequentTaskAfterCopy(
		ASL::Result inCopyResult,
		CopyTaskPtr inCopyTask,
		CopyUnit::SharedPtr inFilesSet)
	{
		if (inFilesSet == NULL || inCopyTask == NULL)
		{
			return;
		}

		// If need rename or has custom metadata, should create UpdateMetadataTask, else ImportTask
		bool needUpdateMetadata = !inCopyTask->GetCustomData().mMetadata.empty() || 
									!inFilesSet->mAliasNameMap.empty() ||
									!inFilesSet->mPathToClipIDMap.empty();

		bool needCreateImportTask = (!inFilesSet->mNeedImportFiles.empty() && inCopyTask->GetCopySetting().mNeedCreateImportTask);
		if (ASL::ResultFailed(inCopyResult))
		{
			if (needUpdateMetadata)
			{
				// Add 1 to skip the update metadata place holder task
				++mDoneTaskCount;
			}
			
			if (needCreateImportTask)
			{
				// Add 1 to skip the import place holder task
				++mDoneTaskCount;
			}
			return;
		}

		bool createSuccess = false;
		if (needUpdateMetadata)
		{
			createSuccess = true;
			UpdateMetadataSetting setting;
			setting.mNeedCreateImportTask = needCreateImportTask;
			setting.mNeedImportFiles = inFilesSet->mNeedImportFiles;
			setting.mAliasNameMap = inFilesSet->mAliasNameMap;
			setting.mNeedDoFileRename = false;
			setting.mPathToClipIDMap = inFilesSet->mPathToClipIDMap;
			TaskFactory::SubmitUpdateMetadataTask(
				setting,
				inCopyTask->GetBatchID(),
				inCopyTask->GetCustomData());
		}
		// Create Import Task if necessary
		else 
		{
			if (needCreateImportTask)
			{
				createSuccess = true;
				ImportSetting setting;
				setting.mSrcFiles = inFilesSet->mNeedImportFiles;
				TaskFactory::SubmitImportTask(
					setting,
					inCopyTask->GetBatchID(),
					inCopyTask->GetCustomData());
			}
			else
			{
				PL::IngestItemList backupItemList;
				for (ImportPathInfoMap::iterator it = inFilesSet->mNeedImportFiles.begin();
					it != inFilesSet->mNeedImportFiles.end();
					it++)
				{
					backupItemList.push_back((*it).second);
				}
				NotifyBackupItemsAreReady(sJobID, inCopyTask->GetBatchID(), backupItemList, inCopyTask->GetCustomData());
			}
		}

		if (createSuccess)
		{
			// Need decrease the placeholder task count created before the copy task starts
			// to avoid adding duplicate. 
			--mTotalTaskCount;
			RunTasks();
		}
	}

	void TaskScheduler::CreateSubsequentTaskAfterUpdateMetadata(
		ASL::Result inResult,
		UpdateMetadataTaskPtr inTask)
	{
		if (inTask == NULL)
		{
			return;
		}

		UpdateMetadataSetting& updateMetadataSetting = inTask->GetSetting();
		if (ASL::ResultFailed(inResult))
		{
			if (!updateMetadataSetting.mNeedImportFiles.empty() && updateMetadataSetting.mNeedCreateImportTask)
			{
				// Add 1 to skip the import place holder task
				++mDoneTaskCount;
			}
			return;
		}

		// Create Import Task if necessary
		if ( !updateMetadataSetting.mNeedImportFiles.empty() && updateMetadataSetting.mNeedCreateImportTask)
		{
			ImportSetting importSetting;
			importSetting.mSrcFiles = updateMetadataSetting.mNeedImportFiles;
			TaskFactory::SubmitImportTask(
				importSetting,
				inTask->GetBatchID(),
				inTask->GetCustomData());

			// Need decrease the placeholder task count created before the copy task starts
			// to avoid adding duplicate. 
			--mTotalTaskCount;
			RunTasks();
		}
		else
		{
			PL::IngestItemList backupItemList;
			for (ImportPathInfoMap::iterator it = updateMetadataSetting.mNeedImportFiles.begin();
				it != updateMetadataSetting.mNeedImportFiles.end();
				it++)
			{
				backupItemList.push_back((*it).second);
			}
			NotifyBackupItemsAreReady(sJobID, inTask->GetBatchID(), backupItemList, inTask->GetCustomData());
		}
	}

	void TaskScheduler::Add(CopyTaskPtr inTask)
	{
		// If the scheduler is in paused state, pause the new added task as well
		// this will avoid start import tasks created by copy/transcode finished message.
		if ( mSchedulerState == kSchedulerState_Paused )
		{
			inTask->SetTaskState(IngestTask::kTaskState_Paused);
		}
		AddTask<CopyTaskList, CopyTaskPtr>(mCopyTaskQueue, inTask);
		mTotalTaskCount += inTask->GetTaskUnitCount();

		AddBatchID(inTask->GetBatchID());

		// Need refresh the progress bar if it is visible 
		// because all tasks can be paused and no any indicate that the new tasks are added or not.
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::Add(TranscodeTaskPtr inTask)
	{
		// If the scheduler is in paused state, pause the new added task as well
		// this will avoid start import tasks created by copy/transcode finished message.
		if ( mSchedulerState == kSchedulerState_Paused )
		{
			inTask->SetTaskState(IngestTask::kTaskState_Paused);
		}
		AddTask<TranscodeTaskList, TranscodeTaskPtr>(mTranscodeTaskQueue, inTask);
		mTotalTaskCount += inTask->GetTaskUnitCount();

		AddBatchID(inTask->GetBatchID());

		// Need refresh the progress bar if it is visible 
		// because all tasks can be paused and no any indicate that the new tasks are added or not.
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::Add(ConcatenateTaskPtr inTask)
	{
		// If the scheduler is in paused state, pause the new added task as well
		// this will avoid start import tasks created by copy/transcode finished message.
		if ( mSchedulerState == kSchedulerState_Paused )
		{
			inTask->SetTaskState(IngestTask::kTaskState_Paused);
		}
		AddTask<ConcatenateTaskList, ConcatenateTaskPtr>(mConcatenateTaskQueue, inTask);
		mTotalTaskCount += inTask->GetTaskUnitCount();

		AddBatchID(inTask->GetBatchID());

		// Need refresh the progress bar if it is visible 
		// because all tasks can be paused and no any indicate that the new tasks are added or not.
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::Add(ImportTaskPtr inTask)
	{
		// If the scheduler is in paused state, pause the new added task as well
		// this will avoid start import tasks created by copy/transcode finished message.
		if ( mSchedulerState == kSchedulerState_Paused )
		{
			inTask->SetTaskState(IngestTask::kTaskState_Paused);
		}
		AddTask<ImportTaskList, ImportTaskPtr>(mImportTaskQueue, inTask);
		mTotalTaskCount += inTask->GetTaskUnitCount();

		AddBatchID(inTask->GetBatchID());

		// Need refresh the progress bar if it is visible 
		// because all tasks can be paused and no any indicate that the new tasks are added or not.
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::Add(UpdateMetadataTaskPtr inTask)
	{
		// If the scheduler is in paused state, pause the new added task as well
		// this will avoid start import tasks created by copy/transcode finished message.
		if ( mSchedulerState == kSchedulerState_Paused )
		{
			inTask->SetTaskState(IngestTask::kTaskState_Paused);
		}
		AddTask<UpdateMetadataTaskList, UpdateMetadataTaskPtr>(mUpdateMetadataTaskQueue, inTask);
		mTotalTaskCount += inTask->GetTaskUnitCount();

		AddBatchID(inTask->GetBatchID());

		// Need refresh the progress bar if it is visible 
		// because all tasks can be paused and no any indicate that the new tasks are added or not.
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));
	}

	void TaskScheduler::Remove(CopyTaskPtr inTask)
	{
		RemoveTask<CopyTaskList>(mCopyTaskQueue, inTask.get());
	}

	void TaskScheduler::Remove(UpdateMetadataTaskPtr inTask)
	{
		RemoveTask<UpdateMetadataTaskList>(mUpdateMetadataTaskQueue, inTask.get());
	}

	void TaskScheduler::Remove(TranscodeTaskPtr inTask)
	{
		RemoveTask<TranscodeTaskList>(mTranscodeTaskQueue, inTask.get());
	}

	void TaskScheduler::Remove(ConcatenateTaskPtr inTask)
	{
		RemoveTask<ConcatenateTaskList>(mConcatenateTaskQueue, inTask.get());
	}

	void TaskScheduler::Remove(ImportTaskPtr inTask)
	{
		RemoveTask<ImportTaskList>(mImportTaskQueue, inTask.get());
	}

	void TaskScheduler::OnStartIngest(ASL::Guid const& inBatchID, ASL::String const& inBinID)
	{
		Start(inBatchID, inBinID);
	}

	void TaskScheduler::OnPauseIngest()
	{
		Pause();
	}

	void TaskScheduler::OnResumeIngest()
	{
		Resume();
	}

	void TaskScheduler::OnCancelIngest()
	{
		Cancel();
	}

	void TaskScheduler::OnImportXMP(ImportXMPElement const& inImportElement)
	{
		ImporterTaskExecutorPtr importTaskExecutor = 
			boost::dynamic_pointer_cast<ImporterTaskExecutor, BaseOperation>(mImportOperation);

		if (importTaskExecutor)
		{
			ImportXMPElements xmpElements;
			xmpElements.push_back(inImportElement);
			importTaskExecutor->ImportFilesXMP(xmpElements);
		}
	}

	void TaskScheduler::OnRequestThumbnail(RequestThumbnailElement const& inReqThumbnailElement)
	{
		ImporterTaskExecutorPtr importTaskExecutor = 
			boost::dynamic_pointer_cast<ImporterTaskExecutor, BaseOperation>(mImportOperation);

		if (importTaskExecutor)
		{
			RequestThumbnailElements thumbnailElements;
			thumbnailElements.push_back(inReqThumbnailElement);
			importTaskExecutor->RequestThumbnail(thumbnailElements);
		}

	}

	void TaskScheduler::FindAndRemoveTranscodeTempFile(const ASL::String& inPath)
	{
		for (size_t i = 0; i < mTranscodeTempFiles.size(); ++i)
		{
			if (dvacore::utility::FileUtils::PathsEqual(mTranscodeTempFiles[i], inPath))
			{
				mTranscodeTempFiles.erase(mTranscodeTempFiles.begin() + i);
				ClearTempTranscodeFile(inPath);
				return;
			}
		}
	}

	void TaskScheduler::OnFinishImportFile(
							ASL::String const& inMediaPath, 
							ASL::Guid const& inBatchID,
							ASL::Guid const& inHostTaskID,
							ASL::Result const& inResult,
							ASL::String const& inErrorInfo)
	{
		ImportTaskPtr task = FindTask<ImportTaskList, ImportTaskPtr>(mImportTaskQueue, inHostTaskID);
		// It's possible that all tasks have been canceled and then asynchronous message arrive.
		if (task == NULL)
			return;

		ASL::Guid batchID = inBatchID == ASL::Guid() ? task->GetBatchID() : inBatchID;

		ASL::String mediaPath = ASL::PathUtils::ToNormalizedPath(inMediaPath);
		BOOST_FOREACH (ImportPathInfoMap::value_type& pairValue, task->mImportSetting.mSrcFiles)
		{
			if (mediaPath == ASL::PathUtils::ToNormalizedPath(pairValue.first))
			{
				task->mFinishedFiles.push_back(mediaPath);

				// report one file import finished
				TaskState state = kTaskState_Done;
				ASL::String displayPath;

				ASL::String msg;
				if (ASL::ResultFailed(inResult))
				{
					++mFailedImportFileCount;
					msg = dvacore::ZString(
						"$$$/Prelude/Mezzanine/IngestScheduler/IngestFileFail=Ingest task - ingest file: \"@0\" failed. Reason: @1");
					msg = dvacore::utility::ReplaceInString(
						msg, 
						MZ::Utilities::NormalizePathWithoutUNC(pairValue.first), 
						inErrorInfo);
					state = kTaskState_Failure;
				}
				else
				{
					++mSucceededImportFileCount;
					msg = dvacore::ZString("$$$/Prelude/Mezzanine/IngestScheduler/IngestFileFinish=Ingest task - ingest file: \"@0\" finished!");
					msg = dvacore::utility::ReplaceInString(
						msg, 
						MZ::Utilities::NormalizePathWithoutUNC(pairValue.first));
				}

				ASL::StationUtils::BroadcastMessage(
					kStation_IngestMedia, 
					IngestTaskStatusMessage(
									task->GetTaskID(), 
									state, 
									msg));

				break;
			}
		}

		FindAndRemoveTranscodeTempFile(inMediaPath);

		mImportProgress = (double)task->mFinishedFiles.size() / (double)task->mImportSetting.mSrcFiles.size();

		// Report progress
		ASL::StationUtils::BroadcastMessage(
			kStation_IngestMedia, 
			IngestTaskProgressMessage(CalculateProgress()));

		if (task->mImportSetting.mSrcFiles.size() == task->mFinishedFiles.size())
		{
			ASL::StationUtils::BroadcastMessage(
				mStationID, 
				ImportFinishedMessage(task->GetTaskID()));

			// Only taskItemID is finished, then to check batchID finish
			if (mBatchGuidSet.find(batchID) != mBatchGuidSet.end())
			{
				ImportTaskPtr importTask = FindBatch<ImportTaskList, ImportTaskPtr>(mImportTaskQueue, batchID);
				CopyTaskPtr copyTask = FindBatch<CopyTaskList, CopyTaskPtr>(mCopyTaskQueue, batchID);
				UpdateMetadataTaskPtr updateMetadataTask = FindBatch<UpdateMetadataTaskList, UpdateMetadataTaskPtr>(mUpdateMetadataTaskQueue, batchID);
				TranscodeTaskPtr transcodeTask = FindBatch<TranscodeTaskList, TranscodeTaskPtr>(mTranscodeTaskQueue, batchID);
				ConcatenateTaskPtr concatenateTask = FindBatch<ConcatenateTaskList, ConcatenateTaskPtr>(mConcatenateTaskQueue, batchID);
				if (importTask == NULL
					&& updateMetadataTask == NULL
					&& transcodeTask == NULL
					&& concatenateTask == NULL
					&& copyTask == NULL)
				{
					RemoveBatchID(batchID);
					SRProject::GetInstance()->GetAssetLibraryNotifier()->IngestBatchFinished(sJobID, batchID);
				}
			}
		}
	}

	bool TaskScheduler::GenerateCopyAction(CopyTaskPtr ioTask)
	{
		bool canContinue = false;
		BOOST_FOREACH (CopyUnit::SharedPtr& filesSet, ioTask->mCopySetting.mCopyUnits)
		{
			BOOST_FOREACH(SrcToDestCopyDataList::value_type& srcToDstData, filesSet->mSrcToDestCopyData)
			{
				const ASL::String& srcFile = srcToDstData.mSrcFile;
				const ASL::String& destFile = srcToDstData.mDestFile;
				if ( ASL::Directory::IsDirectory(srcFile) )
				{
					canContinue = PL::IngestUtils::GenerateFolderCopyAction(
						srcToDstData.mExistOption, 
						mCopyRunnerSetting.mExistFileOption,
						srcFile,
						destFile,
						srcToDstData.mCopyAction);
				}
				else
				{
					canContinue = PL::IngestUtils::GenerateFileCopyAction(
						srcToDstData.mExistOption, 
						mCopyRunnerSetting.mExistFileOption,
						srcFile,
						destFile,
						srcToDstData.mCopyAction);
				}
				if (!canContinue)
				{
					break;
				}
			}
		}
		return canContinue;
	}

	bool TaskScheduler::IsPathInCopyList(const ASL::String& inPath)
	{
		TaskSchedulerPtr scheduler = GetInstance();
		BOOST_FOREACH (CopyTaskList::value_type& copyTaskPtr, scheduler->mCopyTaskQueue)
		{
			const CopySetting& copySetting = copyTaskPtr->GetCopySetting();
			BOOST_FOREACH (const CopyUnit::SharedPtr& filesSet, copySetting.mCopyUnits)
			{
				BOOST_FOREACH (const SrcToDestCopyDataList::value_type& srcToDestCopyData, filesSet->mSrcToDestCopyData)
				{
					if (ASL::CaseInsensitive::StringEquals(srcToDestCopyData.mDestFile, inPath))
					{
						return true;
					}
				}
			}
		}
		return false;
	}
}


} // namespace PL
