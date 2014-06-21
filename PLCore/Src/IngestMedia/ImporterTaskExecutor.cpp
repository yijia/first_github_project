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

// Prefix
#include "Prefix.h"

#include "IngestMedia/ImporterTaskExecutor.h"
#include "IngestMedia/PLIngestJob.h"
#include "IngestMedia/PLIngestMessage.h"

#include "PLProject.h"
#include "PLLibrarySupport.h"

namespace PL
{

	namespace
	{
		/**
		**	import xmp finished, the import xmp operation post this message and IngestJobImporter handle this message.
		**
		**	@param	ImportXMPElement	the element which contains msg id and import file.
		**	@param	ASL::Result			import result.
		**	@param	PL::CustomMetadata  {duration, dropFrame, frameRate}
		**	@param	ASL::String			xmp data.
		**	@param	FileCreateAndModifyTime		create time and modify time.
		**	@param	ImportResultVector	import failures if the result is ASL::kSuccess.
		*/

		ASL_DECLARE_MESSAGE_WITH_6_PARAM(
			InternalImportXMPFinishedMessage,
			ImportXMPElement,
			ASL::Result,
			PL::CustomMetadata,
			XMPText,
			FileCreateAndModifyTime,
			MZ::ImportResultVector);

		/**
		**	request thumbnail finished, the request thumbnail operation post this message and IngestJobImporter handle this message.
		**
		**	@param	RequestThumbnailElement	the element which contains msg id and import file.
		**	@param	ASL::Result			request thumbnail result.
		**	@param	ASL::String			thumbnail file name.
		*/
		ASL_DECLARE_MESSAGE_WITH_3_PARAM(
			InternalRequestThumbnailFinishedMessage,
			RequestThumbnailElement,
			ASL::Result,
			ASL::String);


		/**
		**	import roughcut finished, the import xmp operation post this message and IngestJobImporter handle this message.
		**
		**	@param	ImportXMPElement	the element which contains msg id and import file.
		**	@param	ASL::Result			import result.
		**	@param	AssetMediaInfoList	asset media info of roughcut.
		**	@param	AssetItemList		asset item of roughcut.
		*/

		ASL_DECLARE_MESSAGE_WITH_4_PARAM(
			InternalImportRoughCutFinishedMessage,
			ImportXMPElement,
			ASL::Result,
			PL::AssetMediaInfoList,
			PL::AssetItemList);

		/**
		**
		*/
		static void ThumbnailCompleteAsync(
			ASL::StationID const& inStationID,
			RequestThumbnailElement const & inImportElement,
			ASL::String const& inThumbnailFileName,
			ASL::Result inResult)
		{
			ASL::StationUtils::PostMessageToUIThread(
				inStationID, 
				InternalRequestThumbnailFinishedMessage(inImportElement, inResult, inThumbnailFileName));
		}

		/**
		**
		*/
		static void RequestThumbnailOperationProcessCallback(
			ASL::StationID const& inStationID,
			ASL::AtomicInt* inAbortFlag,
			RequestThumbnailElement const & inImportElement)
		{
			DVA_TRACE("RequestThumbnailOperationProcessCallback", 5, inImportElement.mImportFile);

			SRLibrarySupport::RequestThumbnailAsync(
				inImportElement.mImportFile,
				inImportElement.mClipTime,
				boost::bind<void>(ThumbnailCompleteAsync, inStationID, inImportElement, _1, _2));
		}

		/**
		**
		*/
		static void ImportXMPOperationProcessCallback(
			ASL::StationID const& inStationID,
			ASL::AtomicInt* inAbortFlag,
			ImportXMPElement const & inImportElement)
		{

			if (IsValidRoughCutFile(inImportElement.mImportFile))
			{
				PL::AssetMediaInfoList mediaInfoList;
				PL::AssetItemList assetItemList;

				PL::AssetItemPtr assetItem;
				PL::AssetMediaInfoPtr assetMediaInfo;
				ASL::Result result = PL::LoadRoughCut(
											inImportElement.mImportFile,
											inImportElement.mAssetItemID,
											assetMediaInfo,
											assetItem);
				assetItemList.push_back(assetItem);
				mediaInfoList.push_back(assetMediaInfo);

				ASL::StationUtils::PostMessageToUIThread(
					inStationID, 
					InternalImportRoughCutFinishedMessage(inImportElement, result, mediaInfoList, assetItemList));
			}
			else
			{
				XMPText xmpBuffer(new ASL::StdString);
				//NOTE: import in main thread for SWF file.
				MZ::ImportResultVector importFailure;
				PL::CustomMetadata customMetadata;
				ASL::Result result = SRLibrarySupport::ImportXMPAndCustomMetadata(inImportElement.mImportFile, importFailure, customMetadata, xmpBuffer);

				FileCreateAndModifyTime createAndModifyTime;

				if (ASL::ResultSucceeded(result))
				{
					createAndModifyTime.mFileCreateTime = PL::SRLibrarySupport::GetFileCreateTime(inImportElement.mImportFile);
					createAndModifyTime.mFileModifyTime = PL::SRLibrarySupport::GetFileModifyTime(inImportElement.mImportFile);
				}
				ASL::StationUtils::PostMessageToUIThread(
					inStationID, 
					InternalImportXMPFinishedMessage(inImportElement, result, customMetadata, xmpBuffer, createAndModifyTime, importFailure));
			}
		}
	}


	namespace IngestTask
	{

		/**
		**
		*/
		ImporterTaskExecutor::ImporterTaskExecutor(
							ASL::Guid const& inJobID,
							ASL::StationID const& inStationID)
			:
			BaseClass(inStationID),
			mJobID(inJobID)
		{
			mAssetLibraryNotifier = SRProject::GetInstance()->GetAssetLibraryNotifier();
		}

		/**
		**
		*/
		ImporterTaskExecutor::~ImporterTaskExecutor()
		{

		}

		/**
		**
		*/
		bool ImporterTaskExecutor::Pause()
		{
			mImportXMPElements.Pause();
			mRequestThumbnailElements.Pause();
			return BaseClass::Pause();
		}

		/**
		**
		*/
		bool ImporterTaskExecutor::Resume()
		{
			mImportXMPElements.Resume();
			mRequestThumbnailElements.Resume();
			return BaseClass::Resume();
		}

		/**
		**
		*/
		bool ImporterTaskExecutor::Cancel()
		{
			if ( mImportXMPOperation )
			{
				mImportXMPOperation->Abort();
				//	Have to reset Operation here, or there will be object leak when quit APP.
				mImportXMPOperation = ASL::IBatchOperationRef();
			}

			if ( mRequestThumbnailOperation )
			{
				mRequestThumbnailOperation->Abort();
				mRequestThumbnailOperation = ASL::IBatchOperationRef();
			}

			return BaseClass::Cancel();
		}

		/**
		**
		*/
		void ImporterTaskExecutor::Done()
		{
			if (mImportXMPOperation)
			{
				mImportXMPOperation->Abort();
				mImportXMPOperation = ASL::IBatchOperationRef();
			}


			if ( mRequestThumbnailOperation )
			{
				mRequestThumbnailOperation->Abort();
				mRequestThumbnailOperation = ASL::IBatchOperationRef();
			}
			return BaseClass::Done();
		}

		/**
		**
		*/
		void ImporterTaskExecutor::ImportFilesXMP(ImportXMPElements const& inImportElements)
		{
			for_each(
				inImportElements.begin(), 
				inImportElements.end(), 
				boost::bind(&ImportElementsQueue<ImportXMPElement>::PushBack, &mImportXMPElements, _1));
		}

		/**
		**
		*/
		void ImporterTaskExecutor::RequestThumbnail(RequestThumbnailElements const& inRequestThumbnailElements)
		{
			for_each(
				inRequestThumbnailElements.begin(), 
				inRequestThumbnailElements.end(), 
				boost::bind(&ImportElementsQueue<RequestThumbnailElement>::PushBack, &mRequestThumbnailElements, _1));
		}

		/**
		**
		*/
		void ImporterTaskExecutor::Process()
		{
			//	Create the operation.
			if (mImportXMPOperation == NULL)
			{
				mImportXMPOperation = ASL::IBatchOperationRef(CreateImportXMPOperation(&mImportXMPElements, ImportXMPOperationProcessCallback));
				ASL::StationUtils::AddListener(mImportXMPOperation->GetStationID(), this);
				mImportXMPOperation->Start();
			}

			if (mRequestThumbnailOperation == NULL)
			{
				mRequestThumbnailOperation = ASL::IBatchOperationRef(CreateRequestThumbnailOperation(&mRequestThumbnailElements, RequestThumbnailOperationProcessCallback));
				ASL::StationUtils::AddListener(mRequestThumbnailOperation->GetStationID(), this);
				mRequestThumbnailOperation->Start();
			}

			//	Make Sure our executor is ready for new tasks. 
			Resume();

			BaseClass::Process();
		}

		/*
		**
		*/
		void ImporterTaskExecutor::OnImportXMPFinished(
								ImportXMPElement const& inImportXMPElement,
								ASL::Result inResult,
								CustomMetadata const& inCustomMetadata,
								XMPText inXMP,
								FileCreateAndModifyTime const& inCreateAndModifyTime,
								MZ::ImportResultVector const& inImportResults)
		{
			mAssetLibraryNotifier->ImportXMPFinished(
				mJobID, 
				inImportXMPElement.mMessageID, 
				inResult, 
				inImportXMPElement.mImportFile, 
				inImportXMPElement.mAssetItemID,
				inCustomMetadata,
				inXMP,
				inCreateAndModifyTime.mFileCreateTime,
				inCreateAndModifyTime.mFileModifyTime,
				inImportXMPElement.mBatchID,
				inImportXMPElement.mHostTaskID,
				inImportResults);
		}

		/*
		**
		*/
		void ImporterTaskExecutor::OnRequestThumbnaiFinished(
								RequestThumbnailElement const& inRequestThumbnailElement,
								ASL::Result inResult, 
								ASL::String const& inThumbnailFilePath)
		{
			mAssetLibraryNotifier->RequestThumbnaiFinished(
				mJobID, 
				inRequestThumbnailElement.mMessageID,
				inResult,
				inRequestThumbnailElement.mImportFile, 
				inRequestThumbnailElement.mAssetItemID,
				inRequestThumbnailElement.mClipTime,
				inThumbnailFilePath,
				inRequestThumbnailElement.mBatchID,
				inRequestThumbnailElement.mHostTaskID);	
		}

		/*
		**
		*/
		void ImporterTaskExecutor::OnImportRoughCutFinished(
								ImportXMPElement const& inImportXMPElement, 
								ASL::Result inResult, 
								const PL::AssetMediaInfoList& inAssetMediaInfoList,
								const PL::AssetItemList& inAssetItemList)
		{
			mAssetLibraryNotifier->ImportRoughCutFinished(
				mJobID, 
				inImportXMPElement.mMessageID, 
				inResult, 
				inImportXMPElement.mBatchID,
				inImportXMPElement.mHostTaskID,
				inAssetMediaInfoList,
				inAssetItemList);

		}

		ASL_MESSAGE_MAP_DEFINE(ImporterTaskExecutor)
			ASL_MESSAGE_HANDLER(InternalImportXMPFinishedMessage, OnImportXMPFinished)
			ASL_MESSAGE_HANDLER(InternalRequestThumbnailFinishedMessage, OnRequestThumbnaiFinished)
			ASL_MESSAGE_HANDLER(InternalImportRoughCutFinishedMessage, OnImportRoughCutFinished)	
		ASL_MESSAGE_MAP_END


	}//	IngestTask

}//	MZ
