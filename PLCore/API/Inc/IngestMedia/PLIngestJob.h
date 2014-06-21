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

#pragma once

#ifndef PLINGESTJOB_H
#define PLINGESTJOB_H

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLINGESTVERIFYTYPES_H
#include "IngestMedia/PLIngestVerifyTypes.h"
#endif

#ifndef PLINGESTTYPES_H
#include "IngestMedia/PLIngestTypes.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

#ifndef IPLASSETLIBRARYNOTIFIER_H
#include "IPLAssetLibraryNotifier.h"
#endif

// BE
#ifndef BEPROJECT_H
#include "BEProject.h"
#endif

// ASL
#ifndef ASLDATE_H
#include "ASLDate.H"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLBATCHOPERATIONFWD_H
#include "ASLBatchOperationFwd.h"
#endif

#ifndef ASLSTRINGVECTOR_H
#include "ASLStringVector.h"
#endif

#ifndef DVACORE_THREADS_ATOMIC_H
#include "dvacore/threads/Atomic.h"
#endif

// AME
#ifndef DO_NOT_USE_AME

#ifndef AME_IPRESET_H
#include "IPreset.h"
#endif

#ifndef IEXPORTERMODULE_H
#include "Exporter/IExporterModule.h"
#endif 

#ifndef AME_IENCODERFACTORY_H
#include "IEncoderFactory.h"
#endif

#ifndef MZENCODERMANAGER_H
#include "MZEncoderManager.h"
#endif

#endif




namespace PL
{
	typedef std::set<ASL::Guid>				GuidSet;

	typedef dvacore::UTF16String			RequesterID;
	typedef dvacore::UTF16String			JobID;

	namespace IngestTask
	{
		struct ImportXMPElement
		{
			ASL::Guid		mMessageID;
			ASL::String		mImportFile;
			ASL::Guid		mAssetItemID;
			ASL::Guid		mBatchID;
			ASL::Guid		mHostTaskID;
		};

		typedef std::vector<ImportXMPElement> ImportXMPElements;

		struct RequestThumbnailElement
		{
			ASL::Guid		mMessageID;
			ASL::String		mImportFile;
			dvamediatypes::TickTime  mClipTime;
			ASL::Guid		mAssetItemID;
			ASL::Guid		mBatchID;
			ASL::Guid		mHostTaskID;
		};

		typedef std::vector<RequestThumbnailElement> RequestThumbnailElements;

		struct ClipSetting
		{
			ASL::String						mFileName;
			// only true when rename preset is enbabled and not empty.
			bool							mIsRenamePresetAvailable;
			ASL::String						mAliasName;

			dvamediatypes::TickTime			mInPoint;
			dvamediatypes::TickTime			mOutPoint;
			dvamediatypes::FrameRate		mFrameRate;			

			ClipSetting(
				const ASL::String& inFileName = DVA_STR(""),
				bool inIsRenamePresetAvailable = false,
				const ASL::String& inAliasName = DVA_STR(""),
				const dvamediatypes::TickTime& inInPoint = dvamediatypes::kTime_Invalid,
				const dvamediatypes::TickTime& inOutPoint = dvamediatypes::kTime_Invalid,
				const dvamediatypes::FrameRate& inFrameRate = dvamediatypes::kFrameRate_Zero)
				: 
				mFileName(inFileName),
				mIsRenamePresetAvailable(inIsRenamePresetAvailable),
				mInPoint(inInPoint),
				mOutPoint(inOutPoint),
				mAliasName(inAliasName),
				mFrameRate(inFrameRate)
			{
			}
		};
		typedef std::list<ClipSetting>		ClipSettingList;

		typedef boost::function<void (bool isFocus)> ConcatenateEditGainFocusCallBack;
		struct TranscodeSetting
		{
		public:
			TranscodeSetting()
				:
				mEnable(false),
				mNeedImport(false),
				mConcatenationEnabled(false),
				mSetConcatenateEditFocusFn(ConcatenateEditGainFocusCallBack()),
				mShouldAutoDeleteAfterIngested(false)
			{
			}

			TranscodeSetting(
				EncoderHost::IEncoderFactory::IExporterModuleRef inModule,
				EncoderHost::IPresetRef inPreset,
				bool inEnable
				)
				:
				mExporterModule(inModule),
				mPreset(inPreset),
				mEnable(inEnable),
				mNeedImport(false),
				mConcatenationEnabled(false),
				mSetConcatenateEditFocusFn(ConcatenateEditGainFocusCallBack()),
				mShouldAutoDeleteAfterIngested(false)
			{
			}

			TranscodeSetting(const TranscodeSetting& inSetting)
			{
				mExporterModule = inSetting.mExporterModule;
				mPreset			= inSetting.mPreset;
				mEnable			= inSetting.mEnable;
				mNeedImport		= inSetting.mNeedImport;
				mDestFolder		= inSetting.mDestFolder;
				mSrcRootPath	= inSetting.mSrcRootPath;
				mSrcFileSettings.assign(inSetting.mSrcFileSettings.begin(), inSetting.mSrcFileSettings.end());
				mExporterModuleName = inSetting.mExporterModuleName;
				mPresetName = inSetting.mPresetName;
				mConcatenationEnabled = inSetting.mConcatenationEnabled;
				mConcatenationName = inSetting.mConcatenationName;
				mSetConcatenateEditFocusFn = inSetting.mSetConcatenateEditFocusFn;
				mShouldAutoDeleteAfterIngested = inSetting.mShouldAutoDeleteAfterIngested;
			}


			~TranscodeSetting() {}

			void							SetPreset(EncoderHost::IPresetRef inPreset) { mPreset = inPreset; } 
			EncoderHost::IPresetRef			GetPreset() const { return mPreset; } 

			void							SetExporterModule(EncoderHost::IEncoderFactory::IExporterModuleRef inModule) { mExporterModule = inModule; }
			EncoderHost::IEncoderFactory::IExporterModuleRef GetExportModule() const { return mExporterModule; }

			void							SetEnable(bool inEnable) { mEnable = inEnable; }
			bool							IsEnable() const { return mEnable; }

			void							SetPresetName(const ASL::String& inName)	{mPresetName = inName;}
			ASL::String						GetPresetName()	{return mPresetName;}
			void							SetExporterModuleName(const ASL::String& inName)	{mExporterModuleName = inName;}
			ASL::String						GetExporterModuleName()	{return mExporterModuleName;}

			void							SetConcatenationEnabled(bool inEnabled) {mConcatenationEnabled = inEnabled;}
			bool							IsConcatenationEnabled() const {return mConcatenationEnabled;}
			void							SetConcatenationName(ASL::String const& inName) 
											{
												mConcatenationName = inName;
												dvacore::utility::Trim(mConcatenationName);
											}
			ASL::String						GetConcatenationName() const {return mConcatenationName;}
			ConcatenateEditGainFocusCallBack GetConcatenateEditGainFocusFn() {return mSetConcatenateEditFocusFn;}
			void							SetConcatenateEditGainFocusFn(ConcatenateEditGainFocusCallBack inSetConcatenateFocusFn) {mSetConcatenateEditFocusFn = inSetConcatenateFocusFn;}

		//private:
			EncoderHost::IPresetRef								mPreset;
			EncoderHost::IEncoderFactory::IExporterModuleRef	mExporterModule;
			ASL::String											mPresetName;
			ASL::String											mExporterModuleName;
			bool												mEnable;

			ClipSettingList										mSrcFileSettings;
			ASL::String											mSrcRootPath;
			ASL::String											mDestFolder;
			bool												mNeedImport;

			bool												mConcatenationEnabled;
			ASL::String											mConcatenationName;
			ConcatenateEditGainFocusCallBack					mSetConcatenateEditFocusFn;

			bool												mShouldAutoDeleteAfterIngested;
		};

		// This setting applies to all copy tasks if scheduler is running
		// The added tasks have to follow the same rule
		struct CopyRunnerSetting
		{
			CopyExistOption		mExistFolderOption;
			CopyExistOption		mExistFileOption;

			PL_EXPORT
			CopyRunnerSetting();

			void Reset();
		};

		struct SrcToDestCopyData
		{
			// Full src file path
			ASL::String		mSrcFile;
			// Full des file path, its file name is not necessary same with mSrcFile.
			ASL::String		mDestFile;
			// Filled as input by UI. It will be converted to mCopyAction.
			CopyExistOption mExistOption;
			// Filled when copying according to mExistOption and real situation.
			CopyAction		mCopyAction;
			// Indicate that this file maybe not exist when try to copy and should NOT report error.
			// This case is caused by folder selection, when we start to copy, the src might have been deleted or renamed.
			bool			mIsOptionalSrc;
		};

		typedef std::list<SrcToDestCopyData> SrcToDestCopyDataList;
		// Maybe some day the value will become a struct, so name it with ImportPathInfoMap, rather than ImportPathNameMap.
		typedef std::map<ASL::String, PL::IngestItemPtr> ImportPathInfoMap;
		// This map should only include path to alias name.
		//	path should be original full file path. Its file name maybe same with alias name.
		typedef std::map<ASL::String, ASL::String> ClipAliasNameMap;

		//	first: file path
		//	second: clip ID
		typedef std::map<ASL::String, ASL::Guid> PathToClipIDMap;

		/**
		**	When all files includes in this set are copied completely, its import files can be imported immediately.
		*/
		struct CopyUnit : public dvacore::utility::RefCountedObject
		{
			DVA_UTIL_TYPEDEF_SHARED_REF_PTR(CopyUnit) SharedPtr;
			SrcToDestCopyDataList		mSrcToDestCopyData;
			//  this map is only used to create subsequent import task.
			// [TODO] we should remove these weird member variables later. They are not needed by this task itself.
			ImportPathInfoMap			mNeedImportFiles;
			//  this map is only used to create subsequent update metadata task.
			ClipAliasNameMap			mAliasNameMap;

			//	We will set the unique clip ID for all destinations as long as they are from the same source.
			PathToClipIDMap				mPathToClipIDMap;
		};

		typedef std::list<CopyUnit::SharedPtr> CopyUnitContainer;

		struct CopySetting
		{
			CopyUnitContainer			mCopyUnits;
			VerifyOption				mVerifyOption;

			// [TODO] This is used to determine whether import task should be created.
			//	Previously we always create import task if mNeedImportFiles is not empty.
			//	But update metadata task need to use mNeedImportFiles to determine which file's metadata
			//	should be updated, so mNeedImportFiles maybe not empty even for backup destination and we need another
			//	flag to indicate if need to create import task.
			//	This is a little tricky and should be refactored.
			bool mNeedCreateImportTask;

			PL_EXPORT
			CopySetting();
		};

		struct ImportSetting
		{
			ImportPathInfoMap mSrcFiles;
		};

		/**
		**	
		*/
		struct UpdateMetadataSetting
		{
			ClipAliasNameMap	mAliasNameMap;

			PathToClipIDMap		mPathToClipIDMap;

			// Indicate if need to rename src file in update metadata task.
			//	This is because maybe no copy task before update metadata.
			bool				mNeedDoFileRename;

			// this map is only used to create subsequent import task.
			ImportPathInfoMap	mNeedImportFiles;

			// [TODO] This is used to determine whether import task should be created.
			//	Previously we always create import task if mNeedImportFiles is not empty.
			//	But update metadata task need to use mNeedImportFiles to determine which file's metadata
			//	should be updated, so mNeedImportFiles maybe not empty even for backup destination and we need another
			//	flag to indicate if need to create import task.
			//	This is a little tricky and should be refactored.
			bool				mNeedCreateImportTask;
		};

		typedef boost::shared_ptr<TranscodeSetting> TranscodeSettingPtr;
		typedef std::vector<TranscodeSettingPtr>	TranscodeSettingPtrs;



		//---------------------------------------------------------------
		//---------------------------------------------------------------
		//---------------------------------------------------------------
		//---------------------------------------------------------------
		class TaskBase;
		typedef boost::shared_ptr<TaskBase>				TaskBasePtr;

		class CopyTask;
		typedef boost::shared_ptr<CopyTask>				CopyTaskPtr;
		typedef std::list<CopyTaskPtr>					CopyTaskList;

		class UpdateMetadataTask;
		typedef boost::shared_ptr<UpdateMetadataTask>	UpdateMetadataTaskPtr;
		typedef std::list<UpdateMetadataTaskPtr>		UpdateMetadataTaskList;

		class ImportTask;
		typedef boost::shared_ptr<ImportTask>			ImportTaskPtr;
		typedef std::list<ImportTaskPtr>				ImportTaskList;

		class TranscodeTask;
		typedef boost::shared_ptr<TranscodeTask>		TranscodeTaskPtr;
		typedef std::list<TranscodeTaskPtr>				TranscodeTaskList;

		class ConcatenateTask;
		typedef boost::shared_ptr<ConcatenateTask>		ConcatenateTaskPtr;
		typedef std::list<ConcatenateTaskPtr>			ConcatenateTaskList;

		class VerifyTask;
		typedef boost::shared_ptr<VerifyTask>			VerifyTaskPtr;
		typedef std::list<VerifyTaskPtr>				VerifyTaskList;

		class TaskScheduler;
		typedef boost::shared_ptr<TaskScheduler>		TaskSchedulerPtr;

		//--------------------------------------------------------------------------------------
		// class ThreadProcess

		enum TaskState
		{
			kTaskState_Init,
			kTaskState_Running,
			kTaskState_Done,
			kTaskState_Paused,
			kTaskState_Failure,
			kTaskState_Aborted
		};

		class ThreadProcess
		{
		public:
			ThreadProcess();

			/**
			 ** [NOTE] Please set done status after operation completed.
			 */
			virtual void Process();

			// ----------- member functions used only for main thread -----------

			virtual bool Pause();

			virtual bool Resume();

			virtual bool Cancel();

			virtual bool IsDone() const;

			// ----------- member functions used for both main thread and work thread -----------

			virtual void Done();

			virtual bool IsPaused() const;

			virtual bool IsCanceled() const;

			// ----------- member functions used only for work thread -----------

			/**
			 ** inIntervalTime: If waiting, then check again every inIntervalTime in milliseconds.
			 **	If current status is paused, then wait by sleep until canceled or resume,
			 **	if current status is canceled, then return false immediately,
			 **	In other cases, return true immediately.
			 */
			virtual bool CanContinue(ASL::UInt32 inIntervalTime = 100) const;

		private:
			volatile dvacore::threads::AtomicInt32	mStatus;
		};

		//--------------------------------------------------------------------------------------
		// class BaseOperation

		class BaseOperation : public ThreadProcess
		{
		public:
			BaseOperation(const ASL::StationID& inStationID);

			virtual ~BaseOperation();

			virtual ASL::StationID GetStationID() const;

		private:
			const ASL::StationID					mStationID;
		};

		typedef boost::shared_ptr<BaseOperation> BaseOperationPtr;

		//--------------------------------------------------------------------------------------
		// class UpdateMetadataOperation

		class UpdateMetadataOperation : public BaseOperation
		{
		public:
			UpdateMetadataOperation(const ASL::StationID& inStationID, 
				UpdateMetadataTaskPtr inTask,
				TaskScheduler* inTaskScheduler);

			virtual ~UpdateMetadataOperation();
			virtual void Process();

		private:
			UpdateMetadataTaskPtr	mTask;

			//Members which used as temporary data when processing
			size_t					mDoneCount;
			ASL::Result				mTotalResult;
			size_t					mTotalCount;
			ResultReportVector		mResults;

			TaskScheduler*			mTaskScheduler;
		};

		typedef boost::shared_ptr<UpdateMetadataOperation> UpdateMetadataOperationPtr;

		//--------------------------------------------------------------------------------------
		// class CopyOperation

		typedef boost::function<CopyAction (
			const CopyExistOption& inTaskOption, 
			const ASL::String& inSourcePath,
			const ASL::String& inDestPath)> GenerateCopyActionFn;

		class CopyOperation : public BaseOperation
		{
		public:
			CopyOperation(const ASL::StationID& inStationID, 
						  CopyTaskPtr			inTask,
						  CopyRunnerSetting&	ioRunningSetting,
						  TaskScheduler*		inTaskScheduler);

			virtual ~CopyOperation();
			virtual void Process();

		private:
			bool OnProgressUpdate(size_t doneCount, size_t totalCount, ASL::Float32 inLastPercentDone, ASL::Float32 inPercentDone);
			void CalcTotalfilesCount();
            
			ASL::Result CopyOneImportableSet(
				PL::IngestTask::CopyUnit::SharedPtr inSet,
				PL::VerifyOption inVerifyOption);

		private:
			CopyTaskPtr				mTask;
			CopyRunnerSetting&		mRunningSetting;

			//Members which used as temporary data when processing
			size_t					mDoneCount;
			ASL::Result				mTotalResult;
			size_t					mTotalCount;
			CopyResultVector		mCopyResults;

			TaskScheduler*			mTaskScheduler;
		};

		typedef boost::shared_ptr<CopyOperation> CopyOperationPtr;

		class TaskBase
		{
			friend class TaskScheduler;

		public:
			TaskBase(const ASL::Guid& inBatchID, const IngestCustomData& inCustomData, TaskState inState = kTaskState_Init);
			virtual ~TaskBase();

			TaskState		GetTaskState() const;
			void			SetTaskState(TaskState inState);

			ASL::Guid		GetBatchID() const;
			void			SetBatchID(const ASL::Guid& inID);

			ASL::Guid		GetTaskID() const;

			/**
			**	Task unit count means if this type task is added into queue, we should count how many total tasks for progress.
			**	For example, copy task maybe trigger an update metadata task which will trigger an import task. So one copy task
			**	is pushed means maybe at most three tasks will be updated. So it should return 3 to make total count is not less
			**	than updating tasks.
			*/
			virtual std::size_t		GetTaskUnitCount() const;

			IngestCustomData	GetCustomData() const;

			IngestCustomData& GetCustomData();

		protected:
			TaskState				mTaskState;
			ASL::Guid				mTaskID;
			ASL::Guid				mBatchID;
			IngestCustomData		mCustomData;
		};


		class CopyTask : public TaskBase
		{
			friend class TaskScheduler;
			friend class CopyOperation;
		public:
			CopyTask(const CopySetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);

			virtual ~CopyTask();

			const CopySetting&		GetCopySetting() const;
			virtual std::size_t		GetTaskUnitCount() const;
			void RemoveDuplicatedCopy();
			CopySetting& GetCopySetting();

		private:
			CopySetting				mCopySetting;
		};

		class UpdateMetadataTask : public TaskBase
		{
			friend class TaskScheduler;
			friend class UpdateMetadataOperation;
		public:
			UpdateMetadataTask(const UpdateMetadataSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);

			virtual ~UpdateMetadataTask();
			virtual std::size_t GetTaskUnitCount() const;

			UpdateMetadataSetting& GetSetting();

		private:
			UpdateMetadataSetting mSetting;
		};

		class ImportTask : public TaskBase
		{
			friend class TaskScheduler;
		public:
			ImportTask(const ImportSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);
			virtual ~ImportTask();

			ImportSetting			GetImportSetting() const;
		private:
			ImportSetting			mImportSetting;
			ASL::PathnameList		mFinishedFiles;
		};

		//	TaskID is used to track each AME request, and will be used as RequestID for 
		//	Transcode and Concatenate
		class TranscodeTask : public TaskBase
		{
			friend class TaskScheduler;
		public:
			TranscodeTask(const TranscodeSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);
			virtual ~TranscodeTask();

			virtual std::size_t		GetTaskUnitCount() const;

		protected:
			TranscodeSetting		mTranscodeSetting;
			MZ::JobID				mTranscodeJobID;
			dvacore::UTF16String	mEncoderID;
		};

		typedef std::pair<ASL::String, ASL::String> PathToErrorInfoPair;
		typedef std::vector<PathToErrorInfoPair> PathToErrorInfoVec;
		class ConcatenateTask : public TranscodeTask
		{
			friend class TaskScheduler;

		public:
			ConcatenateTask(const TranscodeSetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);
			virtual ~ConcatenateTask();

			virtual std::size_t		GetTaskUnitCount() const;

			void SetProject(BE::IProjectRef const& inProject);
			void RemoveTempProjectFile();

		private:
			BE::IProjectRef			mProject;
		};

		class TaskFactory
		{
		public:
			TaskFactory();
			~TaskFactory();

		public:
			static CopyTaskPtr CreateCopyTask(
				const CopySetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			static void CreateTranscodeTask(
				const TranscodeSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData,
				TranscodeTaskList& outTasks);

			static ConcatenateTaskPtr CreateConcatenateTask(
				const TranscodeSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			static ImportTaskPtr CreateImportTask(
				const ImportSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			static UpdateMetadataTaskPtr CreateUpdateMetadataTask(
				const UpdateMetadataSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			PL_EXPORT
			static void SubmitCopyTask(const CopySetting& inSetting, const ASL::Guid& inBatchID, const IngestCustomData& inCustomData);

			PL_EXPORT
			static void SubmitTranscodeTask(
				const TranscodeSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			PL_EXPORT
			static void SubmitConcatenateTask(
				const TranscodeSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			PL_EXPORT
			static void SubmitImportTask(
				const ImportSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);

			PL_EXPORT
			static void SubmitUpdateMetadataTask(
				const UpdateMetadataSetting& inSetting,
				const ASL::Guid& inBatchID,
				const IngestCustomData& inCustomData);
		};

		enum SchedulerState
		{
			kSchedulerState_Init,
			kSchedulerState_Running,
			kSchedulerState_Paused
		};

		class TaskScheduler : public ASL::Listener
		{
			ASL_MESSAGE_MAP_DECLARE();

		public:
			~TaskScheduler();

			PL_EXPORT
			static bool HasTaskRunning();

			PL_EXPORT
			static bool Shutdown(bool isForce = false);

			static void ReleaseInstance();

			static bool IsImportTask(const ASL::Guid& inImportTaskID);

			PL_EXPORT
			static void AddCopyTask(CopyTaskPtr inTask);

			PL_EXPORT
			static void AddTranscodeTask(TranscodeTaskPtr inTask);

			PL_EXPORT
			static void AddConcatenateTask(ConcatenateTaskPtr inTask);

			PL_EXPORT
			static void AddImportTask(ImportTaskPtr inTask);

			PL_EXPORT
			static void AddUpdateMetadataTask(UpdateMetadataTaskPtr inTask);

			PL_EXPORT
			static bool IsPathInCopyList(const ASL::String& inPath);

			bool				Start(ASL::Guid const& inBatchID, ASL::String const& inBinID);
			bool				Cancel();
			bool				Pause();
			bool				Resume();

			bool				IsDone() const;
			
			SchedulerState		GetState() const;

			void				Add(CopyTaskPtr inTask);
			void				Add(TranscodeTaskPtr inTask);
			void				Add(ConcatenateTaskPtr inTask);
			void				Add(ImportTaskPtr inTask);
			void				Add(UpdateMetadataTaskPtr inTask);
			void				Remove(CopyTaskPtr inTask);
			void				Remove(UpdateMetadataTaskPtr inTask);
			void				Remove(TranscodeTaskPtr inTask);
			void				Remove(ConcatenateTaskPtr inTask);
			void				Remove(ImportTaskPtr inTask);

			void				AddBatchID(const ASL::Guid& inBatchID);
			void				RemoveBatchID(const ASL::Guid& inBatchID);

			void CreateSubsequentTaskAfterCopy(
				ASL::Result inCopyResult,
				CopyTaskPtr inCopyTask,
				CopyUnit::SharedPtr inFilesSet);

			void CreateSubsequentTaskAfterUpdateMetadata(
				ASL::Result inResult,
				UpdateMetadataTaskPtr inTask);

		protected:
			TaskScheduler();

			// Response to outside control of whole ingest job
			void				OnStartIngest(ASL::Guid const& inBatchID, ASL::String const& inBinID);
			void				OnPauseIngest();
			void				OnResumeIngest();
			void				OnCancelIngest();

			void				Done();
			void				RunTasks();

			// Start tasks
			void				StartCopyTask();
			void				StartUpdateMetadataTask();
			void				StartImportTask();
			void				StartTranscodeTask();
			void				StartConcatenateTask();

			void				ResumeCurrentOperation();
			void				PauseCurrentOperation();
			void				CancelCurrentOperation();
			void				CompleteCurrentOperation();

			double				CalculateProgress() const;
			void				Reset();

			// Listen to copy task status and progress
			void				OnCopyTaskFinished(
										const ASL::Guid& inTaskID, 
										ASL::Result inResult, 
										const CopyResultVector& inCopyResults,
										std::size_t inSucessCount,
										std::size_t inFailedCount);
			void				OnCopyTaskProgress(double inPercent);

			// Listen to copy task status and progress
			void				OnUpdateMetadataTaskFinished(
				const ASL::Guid& inTaskID, 
				ASL::Result inResult, 
				const ResultReportVector& inResults);

			// Listen to import task status and progress
			void				OnImportTaskFinished(const ASL::Guid& inTaskID);
			void				OnImportXMP(ImportXMPElement const& inImportElement);
			void				OnRequestThumbnail(RequestThumbnailElement const& inReqThumbnailElement);
			void				OnFinishImportFile(
											ASL::String const& inMediaPath, 
											ASL::Guid const& inBatchID,
											ASL::Guid const& inHostTaskID,
											ASL::Result const& inResult,
											ASL::String const& inErrorInfo);

			// Listen to transcode task status and progress
			void				OnTranscodeTaskReady(
										RequesterID					inRequestID,
										JobID						inJobID,
										dvacore::UTF16String		inOutputFilePath/* output filepath */,
										dvacore::UTF16String		inEncoderID/* encoderID */);

			void				OnTranscodeTaskFinished(
										RequesterID					inRequestID,
										JobID						inJobID,
										dvacore::UTF16String		inOutputFilePath/* output filepath */,
										dvacore::UTF16String		inEncoderID/* encoderID */);
			void				OnTranscodeTaskStatus(
										RequesterID					inRequestID,
										JobID						inJobID,
										dvacore::UTF16String		inEncoderID, /* encoderID */
										dvametadatadb::StatusType	inStatus);
			void				OnTranscodeTaskProgress(
										RequesterID					inRequestID,
										JobID						inJobID,
										double						inPercent,
										dvacore::UTF16String		inEncoderID);
			void				OnTranscodeTaskError(
										RequesterID					inRequestID,
										JobID						inJobID,
										dvacore::UTF16String		inEncoderID,
										const dvacore::UTF16String&	inErrorInfo);

			void				TranscodeTaskFinishedImpl(
										TranscodeTaskPtr			inTask,
										JobID						inJobID,
										dvacore::UTF16String		inOutputFilePath,
										dvacore::UTF16String		inEncoderID);
			void				CreateSubsequentTaskAfterTranscode(
										TranscodeTaskPtr			inTask,
										dvacore::UTF16String const&	inOutputFilePath);

			void				ConcatenateTaskFinishedImpl(
										ConcatenateTaskPtr			inTask,
										JobID						inJobID,
										dvacore::UTF16String		inOutputFilePath,
										dvacore::UTF16String		inEncoderID);
			void				CreateSubsequentTaskAfterConcatenate(
										ConcatenateTaskPtr			inTask,
										dvacore::UTF16String const&	inOutputFilePath);

			void				OnAMEServerOffline();

			void				HandleTranscodeTaskError(TranscodeTaskPtr inTask, const dvacore::UTF16String& inErrorInfo);

			void				HandleConcatenateTaskError(ConcatenateTaskPtr inTask, const dvacore::UTF16String& inErrorInfo);
			void				HandleConcatenateTaskError(ConcatenateTaskPtr inTask, const PathToErrorInfoVec& inErrorInfos);

			// Figure out copy action.
			bool				GenerateCopyAction(CopyTaskPtr ioTask);

			static TaskSchedulerPtr			GetInstance();

			void FindAndRemoveTranscodeTempFile(const ASL::String& inPath);

		private:
			CopyTaskList					mCopyTaskQueue;
			ImportTaskList					mImportTaskQueue;
			TranscodeTaskList				mTranscodeTaskQueue;
			ConcatenateTaskList				mConcatenateTaskQueue;
			UpdateMetadataTaskList			mUpdateMetadataTaskQueue;

			CopyOperationPtr				mCopyOperation;
			BaseOperationPtr				mImportOperation;
			UpdateMetadataOperationPtr		mUpdateMetadataOperation;

			ASL::StationID					mStationID;
			SchedulerState					mSchedulerState;

			// This might be changed because Prelude may allow multiple copy tasks to run
			// Not the limitation is Prelude only allows one task from each queue to run.

			// Finished # of tasks
			std::size_t						mDoneTaskCount;
			// Total # of tasks added to queue
			std::size_t						mTotalTaskCount;  // For single run

			double							mTranscodeProgress;
			double							mConcatenateProgress;
			double							mCopyProgress;
			double							mImportProgress;
			double							mUpdateMetadataProgress;

			CopyRunnerSetting				mCopyRunnerSetting;

			// Succeeded file count for single run
			std::size_t						mSucceededCopyFileCount;
			std::size_t						mSucceededTranscodeFileCount;
			std::size_t						mSucceededConcatenateFileCount;
			std::size_t						mSucceededImportFileCount;
			std::size_t						mSucceededUpdateMetadataCount;

			// Failed file count for single run
			std::size_t						mFailedCopyFileCount;
			std::size_t						mFailedTranscodeFileCount;
			std::size_t						mFailedConcatenateFileCount;
			std::size_t						mFailedImportFileCount;
			std::size_t						mFailedUpdateMetadataCount;

			GuidSet							mBatchGuidSet;

			static TaskSchedulerPtr			sTaskScheduler;

			ASL::StringVector				mTranscodeTempFiles;

			friend class TaskFactory;
		};
	}

//----------------------------------------------------------------
//		Initializer and Shutdown
//----------------------------------------------------------------
void InitializeIngestMedia();
void ShutdownIngestMedia();


PL_EXPORT
ASL::String FormatConcatenateError(const IngestTask::PathToErrorInfoVec& inErrorInfos);

} // namespace PL

#endif
