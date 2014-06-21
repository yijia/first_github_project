/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2012 Adobe Systems Incorporated                       */
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

#include "PLIngestLibrarySupport.h"

//	ASL
#include "ASLAsyncCallFromMainThread.h"
#include "ASLThreadManager.h"
#include "ASLFile.h"
#include "ASLPathUtils.h"

//	dvacore
#include "dvacore/threads/AsyncThreadedExecutor.h"
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/FileUtils.h"

//	MZ
#include "IngestMedia/PLIngestJob.h"
#include "IngestMedia/PLIngestUtils.h"
#include "PLProject.h"
#include "MZUtilities.h"
#include "MZBEUtilities.h"

#include "PLBEUtilities.h"
#include "PLUtilities.h"

namespace PL
{
namespace
{

	PL::IngestTask::ClipSettingList ConvertToClipSettingList(const PL::ConcatenationItemList& inConcatenationItemList)
	{
		PL::IngestTask::ClipSettingList clipSettingList;
		BOOST_FOREACH (ConcatenationItemPtr const& concatenationItem, inConcatenationItemList)
		{
			PL::IngestTask::ClipSetting clipSetting;
			clipSetting.mFileName = concatenationItem->mFilePath;
			if (concatenationItem->mStartTime == dvamediatypes::kTime_Invalid && concatenationItem->mDuration == dvamediatypes::kTime_Invalid )
			{
				clipSetting.mInPoint = dvamediatypes::kTime_Invalid;
				clipSetting.mOutPoint = dvamediatypes::kTime_Invalid;
			}
			else if (concatenationItem->mStartTime != dvamediatypes::kTime_Invalid && concatenationItem->mDuration != dvamediatypes::kTime_Invalid )
			{
				clipSetting.mInPoint = concatenationItem->mStartTime;
				clipSetting.mOutPoint = concatenationItem->mStartTime + concatenationItem->mDuration;
			}
			else
			{
				DVA_ASSERT(false);
			}
			clipSetting.mFrameRate = concatenationItem->mFrameRate;
			clipSettingList.push_back(clipSetting);
		}

		return clipSettingList;
	}

	void RefineConcatenationItem(PL::ConcatenationItem& ioConcatenationItem)
	{
		BE::IMasterClipRef masterClip = BEUtilities::ImportFile(
														ioConcatenationItem.mFilePath, 
														BE::IProjectRef(), 
														true,	//	inNeedErrorMsg
														false,	//	inIgnoreAudio
														true); //	inIgnoreMetadata
		dvamediatypes::TickTime clipDuration = masterClip->GetMaxCurrentUntrimmedDuration(BE::kMediaType_Any);

		if (ioConcatenationItem.mStartTime == dvamediatypes::kTime_Invalid
		 || ioConcatenationItem.mStartTime < dvamediatypes::kTime_Zero)
		{
			ioConcatenationItem.mStartTime = dvamediatypes::kTime_Zero;
		}
		else if (ioConcatenationItem.mStartTime > clipDuration)
		{
			ioConcatenationItem.mStartTime = clipDuration;
		}

		if (ioConcatenationItem.mDuration == dvamediatypes::kTime_Invalid
		 || ioConcatenationItem.mDuration + ioConcatenationItem.mStartTime > clipDuration)
		{
			ioConcatenationItem.mDuration  = clipDuration - ioConcatenationItem.mStartTime;
		}
		else if (ioConcatenationItem.mDuration < dvamediatypes::kTime_Zero)
		{
			ioConcatenationItem.mDuration = dvamediatypes::kTime_Zero;
		}
	}

	void RefineConcatenationItemList(PL::ConcatenationItemList& ioConcatenationItemList)
	{
		BOOST_FOREACH(PL::ConcatenationItemPtr concatenationItem, ioConcatenationItemList)
		{
			RefineConcatenationItem(*concatenationItem);
		}
	}

	void RefineTranscodeItemList(PL::TranscodeItemList& ioTranscodeItemList)
	{
		BOOST_FOREACH(PL::TranscodeItemPtr transcodeItem, ioTranscodeItemList)
		{
			RefineConcatenationItem(*transcodeItem);
		}
	}

	class EncoderManagerMessageListener : public ASL::Listener
	{
		ASL_MESSAGE_MAP_BEGIN(EncoderManagerMessageListener)
			ASL_MESSAGE_HANDLER(MZ::RenderProgressMessage,		OnTranscodeTaskProgress)
			ASL_MESSAGE_HANDLER(MZ::RenderStatusMessage,		OnTranscodeTaskStatus)
			ASL_MESSAGE_HANDLER(MZ::RenderCompleteMessage,		OnTranscodeTaskFinished)
			ASL_MESSAGE_HANDLER(MZ::RenderErrorMessage,			OnTranscodeTaskError)
			ASL_MESSAGE_HANDLER(MZ::AMEServerOfflineMessage,	OnAMEServerOffline)
		ASL_MESSAGE_MAP_END
	public:

		EncoderManagerMessageListener()
		{
			ASL::StationUtils::AddListener(MZ::EncoderManager::Instance()->GetStationID(), this);
		}

		~EncoderManagerMessageListener()
		{
			ASL::StationUtils::RemoveListener(MZ::EncoderManager::Instance()->GetStationID(), this);
		}

		class EncodeTask
		{
		public:
			EncodeTask():mProgress(0){}
			ASL::Guid mEncodeTaskID;
			ASL::UInt32 mProgress;
			bool		mIsConcatenation;

		};
		struct FindIf_TaskIDEqual {
			FindIf_TaskIDEqual(const ASL::Guid& inTaskID) : mTaskID(inTaskID) {}
			bool operator()(const EncodeTask &inEncoderTask) { return mTaskID == inEncoderTask.mEncodeTaskID; }
			ASL::Guid mTaskID;
		};

		typedef std::vector<EncodeTask> TranscodeTaskIDs;

		/*
		**
		*/
		void AddTranscodeTaskID(ASL::Guid const& inTaskID)
		{
			EncodeTask encodeTask;
			encodeTask.mEncodeTaskID = inTaskID;
			encodeTask.mIsConcatenation = false;
			mTranscodeTaskIDs.push_back(encodeTask);
		}

		/*
		**
		*/
		void AddConcatenationTaskID(ASL::Guid const& inTaskID)
		{
			EncodeTask encodeTask;
			encodeTask.mEncodeTaskID = inTaskID;
			encodeTask.mIsConcatenation = true;
			mTranscodeTaskIDs.push_back(encodeTask);
		}

		/*
		**
		*/
		bool CancelTask(ASL::Guid const& inTaskID)
		{
			TranscodeTaskIDs::iterator iter = std::find_if(mTranscodeTaskIDs.begin(), mTranscodeTaskIDs.end(), FindIf_TaskIDEqual(inTaskID));

			if (iter != mTranscodeTaskIDs.end())
			{
				MZ::EncoderManager::Instance()->CancelJob(inTaskID.AsString(), inTaskID.AsString());

				if ((*iter).mIsConcatenation)
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
						inTaskID,
						ASL::Guid::CreateUnique(),
						ASL::eUserCanceled,
						ASL::String(),
						ASL::String());
				}
				else
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeStatus(
						inTaskID,
						ASL::Guid::CreateUnique(),
						ASL::eUserCanceled,
						ASL::String(),
						ASL::String());
				}
				RemoveTranscodeTaskID(inTaskID);
				return true;
			}

			return false;
		}

	private:

		/*
		**
		*/
		void RemoveTranscodeTaskID(ASL::Guid const& inTaskID)
		{
			TranscodeTaskIDs::iterator iter = std::find_if(mTranscodeTaskIDs.begin(), mTranscodeTaskIDs.end(), FindIf_TaskIDEqual(inTaskID));

			if (iter != mTranscodeTaskIDs.end())
			{
				mTranscodeTaskIDs.erase(iter);

				MZ::EncoderManager::Instance()->RemoveRequest(inTaskID.AsString(), inTaskID.AsString());
			}
		}
		
		/*
		**
		*/
		inline TranscodeTaskIDs::iterator FindTaskByTaskID(ASL::Guid const& inTaskID)
		{
			TranscodeTaskIDs::iterator iter = std::find_if(mTranscodeTaskIDs.begin(), mTranscodeTaskIDs.end(), FindIf_TaskIDEqual(inTaskID));
			return iter;
		}


		/*
		**
		*/
		void OnTranscodeTaskProgress(
								RequesterID				inRequestID,
								JobID					inJobID,
								double					inPercent,
								dvacore::UTF16String	inEncoderID)
		{
			TranscodeTaskIDs::iterator it = FindTaskByTaskID(ASL::Guid(inRequestID));
			if (it != mTranscodeTaskIDs.end())
			{
				ASL::UInt32 latestProgress = static_cast<ASL::UInt32>(inPercent * 100);
				if (latestProgress != it->mProgress)
				{
					it->mProgress = latestProgress;
					if (it->mIsConcatenation)
					{
						PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationProgress(
							ASL::Guid(inRequestID),
							ASL::Guid::CreateUnique(),
							it->mProgress);
					}
					else
					{
						PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeProgress(
							ASL::Guid(inRequestID),
							ASL::Guid::CreateUnique(),
							it->mProgress);
					}
				}
			}
		}

		/*
		**
		*/
		void OnTranscodeTaskStatus(
								RequesterID				inRequestID,
								JobID					inJobID,
								dvacore::UTF16String	inEncoderID, /* encoderID */
								dvametadatadb::StatusType inStatus)
		{
			TranscodeTaskIDs::iterator it = FindTaskByTaskID(ASL::Guid(inRequestID));
			if (it != mTranscodeTaskIDs.end())
			{
				TranscodeResult failure;
				IngestUtils::TranscodeStatusToString(inStatus, failure);
			}
		}

		/*
		**
		*/
		void OnTranscodeTaskFinished(
								RequesterID				inRequestID,
								JobID					inJobID,
								dvacore::UTF16String	inOutputFilePath/* output filepath */,
								dvacore::UTF16String	inEncoderID/* encoderID */)
		{
			TranscodeTaskIDs::iterator it = FindTaskByTaskID(ASL::Guid(inRequestID));
			if (it != mTranscodeTaskIDs.end())
			{
				if (it->mIsConcatenation)
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
						ASL::Guid(inRequestID),
						ASL::Guid::CreateUnique(),
						ASL::kSuccess,
						ASL::String(),
						inOutputFilePath);
				}
				else
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeStatus(
						ASL::Guid(inRequestID),
						ASL::Guid::CreateUnique(),
						ASL::kSuccess,
						ASL::String(),
						inOutputFilePath);
				}

				RemoveTranscodeTaskID(ASL::Guid(inRequestID));
			}
		}

		/*
		**
		*/
		void OnTranscodeTaskError(
								RequesterID					inRequestID,
								JobID						inJobID,
								dvacore::UTF16String		inEncoderID,
								const dvacore::UTF16String&	inErrorInfo)
		{
			TranscodeTaskIDs::iterator it = FindTaskByTaskID(ASL::Guid(inRequestID));
			if (it != mTranscodeTaskIDs.end())
			{
				if (it->mIsConcatenation)
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
						ASL::Guid(inRequestID),
						ASL::Guid::CreateUnique(),
						ASL::ResultFlags::kResultTypeFailure,
						inErrorInfo,
						ASL::String());
				}
				else
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeStatus(
						ASL::Guid(inRequestID),
						ASL::Guid::CreateUnique(),
						ASL::ResultFlags::kResultTypeFailure,
						inErrorInfo,
						ASL::String());
				}
				RemoveTranscodeTaskID(ASL::Guid(inRequestID));
			}
		}

		/*
		**
		*/
		void OnAMEServerOffline()
		{
			TranscodeTaskIDs tempTaskIDS = mTranscodeTaskIDs;
			BOOST_FOREACH(EncodeTask const& encodeTask, tempTaskIDS)
			{
				ASL::String const& AMEOfflineError = 
					dvacore::ZString("$$$/Prelude/Mezzanine/IngestLibrarySupport/AMEOffline=AME is offline!");

				if (encodeTask.mIsConcatenation)
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
						encodeTask.mEncodeTaskID,
						ASL::Guid::CreateUnique(),
						ASL::ResultFlags::kResultTypeFailure,
						AMEOfflineError,
						ASL::String());
				}
				else
				{
					PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeStatus(
						encodeTask.mEncodeTaskID,
						ASL::Guid::CreateUnique(),
						ASL::ResultFlags::kResultTypeFailure,
						AMEOfflineError,
						ASL::String());
				}

				RemoveTranscodeTaskID(encodeTask.mEncodeTaskID);
			}
		}

	private:
		TranscodeTaskIDs			mTranscodeTaskIDs;

	};

	EncoderManagerMessageListener sEncoderManagerMessageListener;
}

namespace IngestLibrarySupport
{

	/*
	**
	*/
	void ConcatenationRequest(
		const ASL::Guid& inTaskID, 
		const EncoderHost::IPresetRef& inPreset,
		const ASL::String& inOutputDirectory,
		const ASL::String& inOutputFileName,
		const PL::ConcatenationItemList& inConcatenatnionItemList)
	{
		if (inConcatenatnionItemList.empty() || inPreset == NULL || inOutputFileName.empty() || inOutputDirectory.empty())
		{
			return;
		}

		PL::ConcatenationItemList concatenationItemList = inConcatenatnionItemList;
		RefineConcatenationItemList(concatenationItemList);

		if ( !ASL::PathUtils::ExistsOnDisk(inOutputDirectory) )
		{
			ASL::Directory::CreateOnDisk(inOutputDirectory, true);
		}

		//	Convert concatenate items into sequence
		PL::IngestTask::PathToErrorInfoVec errorInfos;
		BE::IProjectRef theProject;
		BE::ISequenceRef theSequence;
		PL::IngestTask::ClipSettingList clipSettingList = ConvertToClipSettingList(inConcatenatnionItemList);

		//	Create proxy sequence for concatenation
		if (!IngestUtils::CreateSequenceForConcatenateMedia(clipSettingList, theProject, theSequence, inOutputFileName, errorInfos))
		{
			return PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
				inTaskID,
				ASL::Guid::CreateUnique(),
				ASL::ResultFlags::kResultTypeFailure,
				FormatConcatenateError(errorInfos),
				ASL::String());
		}
		else
		{
			//	Do real concatenation
			if (!PL::IngestUtils::TranscodeConcatenateFile(
				inTaskID.AsString(),
				inTaskID.AsString(),
				inPreset,
				theProject,
				theSequence,
				inOutputDirectory,
				inOutputFileName,
				errorInfos) )
			{
				return PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->ConcatenationStatus(
					inTaskID,
					ASL::Guid::CreateUnique(),
					ASL::ResultFlags::kResultTypeFailure,
					FormatConcatenateError(errorInfos),
					ASL::String());
			}
		}

		//	If we get here, means request is success.
		sEncoderManagerMessageListener.AddConcatenationTaskID(inTaskID);
	}

	/*
	**
	*/
	void TranscodeRequest(
		const ASL::Guid& inTaskID, 
		const EncoderHost::IPresetRef& inPreset,
		const ASL::String& inOutputDirectory,
		const PL::TranscodeItemList& inTranscodeItemList)
	{
		if (inTranscodeItemList.empty() || inPreset == NULL|| inOutputDirectory.empty())
		{
			return;
		}

		PL::TranscodeItemList transcodeItemList = inTranscodeItemList;
		RefineTranscodeItemList(transcodeItemList);

		if ( !ASL::PathUtils::ExistsOnDisk(inOutputDirectory) )
		{
			ASL::Directory::CreateOnDisk(inOutputDirectory, true);
		}

		//	Single clip transcode
		if (transcodeItemList.size() == 1)
		{
			PL::TranscodeItemPtr transcodeItemPtr = transcodeItemList.front();

			ASL::String outputFileName = transcodeItemPtr->mOutputFileName;
			if (outputFileName.empty())
			{
				outputFileName = ASL::PathUtils::GetFilePart(transcodeItemPtr->mFilePath);
			}
			ASL::String presetFile(PL::Utilities::SavePresetToFile(inPreset));
			ASL::String errorInfo;
			if (!PL::IngestUtils::TranscodeMediaFile(
							BE::kCompileSettingsType_Movie,
							transcodeItemPtr->mFilePath, 
							inOutputDirectory,
							outputFileName,
							presetFile,
							transcodeItemPtr->mStartTime,
							transcodeItemPtr->mStartTime + transcodeItemPtr->mDuration,
							inTaskID.AsString(),
							inTaskID.AsString(),
							errorInfo) )
			{
				return PL::SRProject::GetInstance()->GetAssetLibraryNotifier()->TranscodeStatus(
					inTaskID,
					ASL::Guid::CreateUnique(),
					ASL::ResultFlags::kResultTypeFailure,
					errorInfo,
					ASL::String());
			}
		}

		//	If we get here, means request is success.
		sEncoderManagerMessageListener.AddTranscodeTaskID(inTaskID);

	}

	/*
	**
	*/
	void CancelTranscodeAsync(const ASL::Guid& inTaskID)
	{
		sEncoderManagerMessageListener.CancelTask(inTaskID);
	}

	/*
	**
	*/
	void CancelConcatenationAsync(const ASL::Guid& inTaskID)
	{
		sEncoderManagerMessageListener.CancelTask(inTaskID);
	}
}

}
