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

#pragma once

#ifndef IMPORTERTASKEXECUTOR_H
#define IMPORTERTASKEXECUTOR_H

#ifndef PLINGESTJOB_H
#include "IngestMedia/PLIngestJob.h"
#endif

#ifndef IMPORTTYPES_H
#include "IngestMedia/ImportTypes.h"
#endif

#ifndef IPLASSETLIBRARYNOTIFIER_H
#include "IPLAssetLibraryNotifier.h"
#endif

namespace PL
{
	namespace IngestTask
	{
		class ImporterTaskExecutor;
		typedef boost::shared_ptr<ImporterTaskExecutor> ImporterTaskExecutorPtr;

		class ImporterTaskExecutor
			: 
			public BaseOperation,
			public ASL::Listener
		{
			ASL_MESSAGE_MAP_DECLARE();

		public:
			typedef BaseOperation BaseClass;

			/**
			**	
			*/
			ImporterTaskExecutor(
					ASL::Guid const& inJobID, 
					ASL::StationID const& inStationID);

			/**
			**	
			*/
			~ImporterTaskExecutor();

			/**
			**
			*/
			virtual void Process();

			/**
			**
			*/
			virtual bool Pause();

			/**
			**
			*/
			virtual bool Resume();

			/**
			**
			*/
			virtual bool Cancel();

			/**
			**
			*/
			virtual void Done();
			/**
			**	The method is part of import API for 3rd client, async.
			*/
			void ImportFilesXMP(ImportXMPElements const& inImportElements);

			/**
			**	The method is part of import API for 3rd client, async.
			*/
			void RequestThumbnail(RequestThumbnailElements const& inImportElements);

		private:

			/**
			**
			*/
			void OnImportXMPFinished(
				ImportXMPElement const& inImportXMPElement, 
				ASL::Result inResult, 
				PL::CustomMetadata const& inCustomMetadata,
				XMPText inXMP,
				FileCreateAndModifyTime const& inCreateAndModifyTime,
				MZ::ImportResultVector const& inImportResults);

			/**
			**
			*/
			void OnRequestThumbnaiFinished(
				RequestThumbnailElement const& inRequestThumbnailElement,
				ASL::Result inResult, 
				ASL::String const& inThumbnailFilePath);

			/**
			**
			*/
			void OnImportRoughCutFinished(
				const ImportXMPElement& inImportXMPElement, 
				ASL::Result inResult, 
				const PL::AssetMediaInfoList& inAssetMediaInfoList,
				const PL::AssetItemList& inAssetItemList);

		private:

			ASL::Guid										mJobID;
			ASL::IBatchOperationRef							mImportXMPOperation;
			ASL::IBatchOperationRef							mRequestThumbnailOperation;

			ImportElementsQueue<ImportXMPElement>			mImportXMPElements;
			ImportElementsQueue<RequestThumbnailElement>	mRequestThumbnailElements;
			IAssetLibraryNotifier*							mAssetLibraryNotifier;
		};

	}//	IngestTask

}//	MZ

#endif