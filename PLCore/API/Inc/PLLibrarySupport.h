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

#ifndef PLLIBRARYSUPPORT_H
#define PLLIBRARYSUPPORT_H

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

#ifndef IMPORTERHOST_H
#include "ImporterHost.h"
#endif

#ifndef PLMESSAGESTRUCT_H
#include "PLMessageStruct.h"
#endif

#ifndef PLINGESTTYPES_H
#include "IngestMedia/PLIngestTypes.h"
#endif

namespace PL
{
	struct CustomMetadata;

	namespace SRLibrarySupport
	{
		typedef boost::function<void(ASL::String, ASL::Result)> RequestThumbnailCallbackFn;
		
		/*
		**
		*/
		PL_EXPORT
		bool OpenLibraryItem(PL::AssetItemPtr const& inAssetInfo);
		
		/*
		**
		*/
		PL_EXPORT
		bool OpenRoughCut(PL::AssetItemPtr inRCAssetItem);

		/*
		**
		*/
		PL_EXPORT
		bool OpenLoggingClip(PL::AssetItemPtr const& inAssetInfo);
		
		/*
		**
		*/
		PL_EXPORT
		bool SaveXMPToDisk(PL::AssetMediaInfoPtr const& inAssetMediaInfo, bool writeCacheFile = true);

		/*
		**
		*/
		PL_EXPORT
		bool SaveRoughCutToDisk(PL::AssetMediaInfoPtr const& inAssetMediaInfo, PL::AssetItemList const& inAssetItemList);

		/*
		**
		*/
		PL_EXPORT
		void InsertClipsToRoughCut(
			PL::AssetItemList const & inAssetItemList,
			PL::ISRPrimaryClipPlaybackRef inRCPlayBack,
			dvamediatypes::TickTime inInsertPos);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result AppendClipToRoughCut(
						PL::AssetItemList const & inAssetItemList,
						PL::ISRPrimaryClipPlaybackRef inRCPlayBack,
						PL::ActionResultList& outActionResultList);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result InsertClipsToRoughCut(
						PL::AssetItemList const & inAssetItemList,
						PL::ISRPrimaryClipPlaybackRef inRCPlayBack,
						PL::ActionResultList& outActionResultList,
						bool inInsertAhead);

		/*
		**
		*/
		PL_EXPORT
		bool IsValidDataForDrop(
					PL::AssetItemList const & inAssetItemList,
					PL::ISRPrimaryClipPlaybackRef inPrimaryClipPlayback);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result DeleteAssetItems(
						PL::AssetItemList const & inAssetItemList, PL::ActionResultList& outActionResultList);

		/*
		**
		*/
		PL_EXPORT
		void SyncRoughCutFileContent(
						PL::AssetItemPtr& ioAssetItem);

		/*
		** Not only Read XMP from cache(if exist), but also get custom metadata from file(FrameRate, Duration..)
		*/
		PL_EXPORT
		ASL::Result ImportXMPAndCustomMetadata(
						const ASL::String& inFilePath,
						MZ::ImportResultVector& outFilesFailure,
						PL::CustomMetadata& ourCustomMetadata,
						XMPText outXMPBuffer);

		/*
		** Read XMP From cache if exist
		*/
		PL_EXPORT
		ASL::Result ReadXMPFromCache(
						ASL::String const& inFilePath,
						XMPText outXMPBuffer,
						ASL::String& outErrorInfo);

		/*
		** Direcly Read XMP from media file
		*/
		PL_EXPORT
		ASL::Result ReadXMPFromFile(
						ASL::String const& inFilePath,
						XMPText outXMPBuffer,
						ASL::String& outErrorInfo);

		/*
		** Direcly Read XMP from masterclip
		*/
		PL_EXPORT
		ASL::Result ReadXMPFromMasterClip(
						const BE::IMasterClipRef& inMasterClip,
						XMPText outXMPBuffer,
						ASL::String& outErrorInfo);

		/*
		** Load XMP&Custom Metadta for media  and Load assetItems for RC
		*/
		PL_EXPORT
		void AsyncLoadingFileInWorkingThread(
						const ASL::Guid& inTaskGuid, 
						const ASL::Guid& inMsgGuid, 
						const ASL::String& inPath, 
						const ASL::Guid& inAssetID,
						bool inIsMediaPropertyOnly);

		/*
		**
		*/
		PL_EXPORT
		void Import(
					const ASL::Guid& inTaskID, 
					const ASL::Guid& inMsgID, 
					const ASL::String& inPath, 
					const ASL::Guid& inAssetID,
					ASL::Guid const& inBatchID,
					ASL::Guid const& inHostTaskID);

		/*
		**
		*/
		PL_EXPORT
		void RequestThumbnail(
					const ASL::Guid& inTaskID, 
					const ASL::Guid& inMsgID,
					ASL::String const& inImportFile,
					dvamediatypes::TickTime const& inClipTime,
					ASL::Guid const& inAssetID,
					ASL::Guid const& inBatchID,
					ASL::Guid const& inHostTaskID);

		/*
		**
		*/
		PL_EXPORT
		void RequestThumbnailAsync(
				ASL::String const& inImportFile,
				dvamediatypes::TickTime const& inClipTime,
				RequestThumbnailCallbackFn inRequestThumbnailCallbackFn);

		/*
		**
		*/
		PL_EXPORT
		void UpdateImportProgress(
						ASL::Guid const& inTaskID,
						ASL::Guid const& inMsgID, 
						ASL::UInt32 inProgress);

		/*
		**
		*/
		PL_EXPORT
		void FinishImportFile(
						ASL::Guid const& inTaskID,
						ASL::Guid const& inMsgID,
						ASL::String const& inMediaPath,
						ASL::Guid const& inBatchID,
						ASL::Guid const& inHostTaskID,
						ASL::Result const& inResult,
						ASL::String const& inErrorInfo);

		/*
		**
		*/
		PL_EXPORT
		void NotifyCreateRoughCutCommand();

		/*
		**
		*/
		PL_EXPORT
		void NotifyAppendToRoughCutCommand();

		/*
		**
		*/
		PL_EXPORT
		void NotifyInsertClipsToRoughCutCommand(bool inInsertAhead);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result ImportXMPImpBackDoor(
			const ASL::String& inFilePath,
			MZ::ImportResultVector& outFilesFailure,
			CustomMetadata& outCustomMetadata,
			XMPText outXMPBuffer);
		
		/*
		**
		*/
		PL_EXPORT
		ASL::String GetFileCreateTime(ASL::String const& inFilePath);

		/*
		**
		*/
		PL_EXPORT
		bool RelinkFileInCore(PL::AssetItemList const& inRelinkedAssetItemList);

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetFileModifyTime(ASL::String const& inFilePath);

		PL_EXPORT
		ASL::Result UpdateXMPWithMinimumMetadata(
			XMPText ioXMP,
			const PL::MinimumMetadata& inMiniMetadata);

		PL_EXPORT
		ASL::Result UpdateXMPWithMinimumMetadata(
					PL::AssetMediaInfoList const& inAssetMediaInfoList, 
					PL::IngestCustomData const& inCustomData, 
					bool inUseLocalFile,
					PL::AssetMediaInfoList& outAssetMediaInfoList);

		PL_EXPORT
		extern const char* kXMPAliasNameProperty;

		PL_EXPORT
		extern const char* kXMPAliasNameNamespcae;

		PL_EXPORT
		ASL::Result UpdateXMPWithRenameData(
					XMPText ioXMP, 
					ASL::String const& inNewName);

		PL_EXPORT
		ASL::Result GetNameFromXMP(
					XMPText const& inXMP, 
					ASL::String& outName);


		PL_EXPORT
		ASL::Result UpdateXMPWithMetadata(
					XMPText ioXMP, 
					PL::NamespaceMetadataList const& inMetadataList,
					bool inStoreEmpty);

		PL_EXPORT
		ASL::Result WriteClipIDToXMPIfNeeded(XMPText ioXMP, const ASL::Guid& inGuid);

		PL_EXPORT
		void XMPFileTimeChanged(ASL::Guid const& inAssetID, ASL::String const& inFilePath, ASL::Result inResult);

		/*
		**
		*/
		PL_EXPORT
		void SendUnassociatedMetadata(PL::UnassociatedMetadataList const& inUnassociatedMetadataList);

		PL_EXPORT
		void ApplyUnassociatedMetadata(
					PL::UnassociatedMetadataList const& inUnassociatedMetadataList,
					PL::ApplyUnassociatedMetadataPositionType inApplyPos);

		PL_EXPORT
		void ApplyUnassociatedMetadataToMedias(
					const PL::UnassociatedMetadataList& inUnassociatedMetadataList,
					const ASL::PathnameList& inPaths);

		PL_EXPORT
		ASL::Result IsSupportedMedia(const BE::IMasterClipRef& inMasterClip);

		/**
		**	Used by transfer API b/w clients and core. Process updating message will be sent from the implement directly.
		**	Use CancelAsyncCopy to cancel a running batch copy.
		*/
		PL_EXPORT
		void RequestAsyncCopy(
			const ASL::Guid& inBatchID,
			const TransferItemList& inItemList,
			TransferOptionType inOption,
			FTPSettingsInfo const& inFTPSettingsInfo,
			TransferType inTransferType);

		/**
		**	Cancel a running batch copy.
		*/
		PL_EXPORT
		void CancelAsyncCopy(const ASL::Guid& inBatchID);

		/**
		**	Used by transfer API b/w clients and core. Rename result message will be sent from the implement directly.
		*/
		PL_EXPORT
		void RequestAsyncRename(
			const ASL::Guid& inBatchID,
			const RenameItemList& inItemList);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result CreateRoughCut(
			const ASL::String& inFilePath,
			bool inShouldSaveToFile,
			ASL::String& outContentText,
			PL::AssetMediaInfoList& outAssetMediaInfoList,
			ASL::String& outErrorMessage);

		/*
		**
		*/
		PL_EXPORT
		ASL::Result OpenRoughCut(
			const PL::AssetItemPtr& inAssetItem,
			ASL::String outErrorInfo);

		/*
		**
		*/
		PL_EXPORT
		void RegisterAssetMediaInfoToSRCore(
			const PL::AssetMediaInfoList& inAssetMediaInfoList,
			const PL::AssetItemList& inAssetItemList,
			bool inMediaUseLocalFile,
			bool inRoughCutUseLocalFile,
			bool inNeedForceRegister,
			bool inIsRelink);
	}
}


#endif
