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

#pragma once

#ifndef IPLASSETLIBRARYNOTIFIER_H
#define IPLASSETLIBRARYNOTIFIER_H

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

#ifndef PLROUGHCUTUTILS_H
#include "PLRoughCutUtils.h"
#endif

// ASL
#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

// dva
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef PLMESSAGESTRUCT_H
#include "PLMessageStruct.h"
#endif

#ifndef PLINGESTTYPES_H
#include "IngestMedia/PLIngestTypes.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

namespace PL
{

struct CustomMetadata
{
	dvamediatypes::FrameRate	mVideoFrameRate;
	bool						mDropFrame;
	dvamediatypes::TickTime		mDuration;
	dvamediatypes::TickTime		mMediaStart;
	bool						mIsAudioOnly;
	bool						mIsStillImage;
	ASL::String					mName;
	
	CustomMetadata()
		:
		mVideoFrameRate(dvamediatypes::kFrameRate_Zero),
		mDropFrame(false),
		mDuration(dvamediatypes::kTime_Invalid),
		mMediaStart(dvamediatypes::kTime_Invalid),
		mIsAudioOnly(false),
		mIsStillImage(false)
		{
		}
};

ASL_DEFINE_IREF(IAssetLibraryNotifier);
ASL_DEFINE_IID(IAssetLibraryNotifier, 0x418cacca, 0xc7ad, 0x4311, 0xb2, 0x2d, 0x2, 0xb, 0xf2, 0x81, 0x51, 0xda);

ASL_INTERFACE IAssetLibraryNotifier : public ASLUnknown
{
	virtual void IngestTaskStarted(
				ASL::Guid const& inTaskID) = 0;

	virtual void IngestTaskFinished(
				ASL::Guid const& inTaskID) = 0;

	virtual void IngestTaskCancelled(
					ASL::Guid const& inTaskID) = 0;
	
	virtual void IngestTaskPaused(
					TaskIDList const& inTaskIDList) = 0;

	virtual void IngestTaskResumed(
					TaskIDList const& inTaskIDList) = 0;

	virtual void ImportItemsAreReady(
					ASL::Guid const& inJobID,
					ASL::Guid const& inBatchID,
					ASL::Guid const& inTaskItemID,
					PL::IngestItemList const& inIngestItems,
					PL::IngestCustomData const& inCustomData) = 0;

	virtual void BackupItemsAreReady(
					ASL::Guid const& inJobID,
					ASL::Guid const& inBatchID,
					PL::IngestItemList const& inIngestItems,
					PL::IngestCustomData const& inCustomData) = 0;

	virtual void IngestBatchStarted(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inBatchID,
					ASL::String const& inTargetBinID) = 0;
	
	virtual void IngestBatchFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inBatchID) = 0;

	virtual void ImportXMPFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inImportResult,
					ASL::String const& inFilePath, 
					ASL::Guid const& inAssetID,
					CustomMetadata const& inCustomMetadata,
					XMPText inXMP,
					ASL::String const& inCreateTime,
					ASL::String const& inLastModifyTime,
					ASL::Guid const& inBatchID,
					ASL::Guid const& inHostTaskID,
					const MZ::ImportResultVector& inErrors) = 0;
					
	virtual void RequestThumbnaiFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inRequestResult,
					ASL::String const& inMediaFilePath, 
					ASL::Guid const& inAssetID,
					dvamediatypes::TickTime const& inClipTime,
					ASL::String const& inThumbnailFilePath,
					ASL::Guid const& inBatchID,
					ASL::Guid const& inHostTaskID) = 0;
					
	virtual void OpenLoggingClipFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					PL::ActionResultPtr inActionResult,
					ASL::Result inOpenResult) = 0;

	virtual void OpenRoughCutFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		PL::ActionResultPtr inActionResult,
		ASL::Result inOpenResult) = 0;

	virtual void AssetItemClosed(
					ASL::Guid const& inTaskID,
					ASL::Guid const& inMsgID,
					PL::AssetItemList const& inAssetItems) = 0;

	virtual void AssetItemOpened(
					ASL::Guid const& inTaskID,
					ASL::Guid const& inMsgID,
					PL::AssetItemList const& inAssetItems) =0;

	
	virtual void CreateRoughCutFinished(
					ASL::Guid const& inTaskID,
					ASL::Guid const& inMsgID,
					ASL::String const& inBinId,
					ASL::Result inResult,
					ASL::String const& inErrMsg,
					ASL::String const& inFileContent,
					PL::AssetMediaInfoList const& inAssetMediaInfos) = 0;

	virtual void SaveAssetMediaInfo(
					ASL::Guid const& inTaskID,
					ASL::Guid const& inMsgID,
					PL::AssetMediaInfoList const& inAssetMediaInfos,
					const PL::AssetItemPtr& inAssetItem) = 0;
	
	virtual void SaveRoughCut(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::AssetItemPtr const& inRCAssetItem,
		PL::AssetMediaInfoPtr const& inRCMediaInfo) = 0;

	virtual void TriggerSaveAssetMediaInfoStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorMsg,
		PL::AssetMediaInfoList const& inAssetMediaInfos) = 0;

	virtual void TriggerSaveRoughCutStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorMsg,
		PL::AssetItemPtr const& inRCAssetItem,
		PL::AssetMediaInfoPtr const& inRCMediaInfo) = 0;

	virtual void WriteXMPToDiskStatus(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inSaveResult,
		PL::ActionResultList const& inActionResults) = 0;

	virtual void XMPFileTimeChanged(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void WriteRoughCutToDiskFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void AppendClipsFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void InsertClipsFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void SendToPremiereProFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void DeleteAssetFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void CloseAssetFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inSaveResult,
					ASL::String const& inErrMsg) = 0;

	virtual void AssetDirtyStateChanged(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					PL::AssetItemPtr const& inAssetItem,
					bool  isDirty) = 0;

	virtual void AppExit(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID) = 0;

	virtual void LibraryUnload(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					bool inAllowCancel) = 0;

	virtual void ExportTaskFinished(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::Result inExportResult,
					PL::ActionResultList const& inActionResults) = 0;

	virtual void ImportRoughCutFinished(
					const ASL::Guid& inTaskID, 
					const ASL::Guid& inMsgID,
					ASL::Result inImportResult,
					const ASL::Guid& inBatchID,
					const ASL::Guid& inHostTaskID,
					const PL::AssetMediaInfoList& inAssetMediaInfoList,
					const PL::AssetItemList& inAssetItemList) = 0;

	virtual void NotifyAssetMediaInfoState(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					PL::AssetMediaInfoPtr const& inAssetMediaInfo,
					ASL::Result inMediaInfoState) = 0;

	virtual void NotifyOffLineFiles(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					ASL::PathnameList const& inOffLineFiles) = 0;

	virtual void RequestRelinkAssetItems(
					ASL::Guid const& inTaskID, 
					ASL::Guid const& inMsgID,
					PL::AssetItemList& inAssetItemList) = 0;

	virtual void OnCreateRoughCutNotify(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID) = 0;

	virtual void OnAppendToRoughCutNotify(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID) = 0;

	virtual void OnInsertClipsToRoughCutNotify(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		bool inInsertAhead) = 0;

	virtual void SelectAssetFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::ActionResultList const& inActionResults) = 0;

	//	Monitor API
	virtual void RegisterMonitorXMPFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::ActionResultList const& inActionResults) =0;

	virtual void UnregisterMonitorXMPFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::ActionResultList const& inActionResults) =0;

	virtual void RefreshXMPFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::ActionResultList const& inActionResults) =0;

	virtual void NotifyXMPChanged(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		PL::AssetMediaInfoList const& inAssetMediaInfos) =0;

	virtual void MediaInfoChanged(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		PL::AssetMediaInfoList const& inAssetMediaInfos) =0;

	virtual void ReSyncAssetFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::ActionResultList const& inActionResultList) =0;

	virtual void UpdateXMPWithCustomDataFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::AssetMediaInfoList const& inAssetMediaInfos) =0;

	virtual void RelinkAssetFinished(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID, 
		PL::AssetItemList& inAssetItemList,
		PL::AssetMediaInfoList const& inAssetMediaInfoList,
		PL::ActionResultList const& inActionResultList) =0;

	virtual void TransferProgress(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::SInt32 inPercent) = 0;

	virtual void TransferStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::TransferResultList const& inResultList) = 0;

	virtual void RenameStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::TransferResultList const& inResultList) = 0;

	virtual void SelectedItemInfo(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::SelectedItemInfoList const& inSelectedItemInfoList) = 0;

	virtual void GetMediaPropertyInfoStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::MediaPropertyInfo const& inMediaPropertyInfo) = 0;

	virtual void GetAssociatedFilesStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::String const& inMediaPath,
		ASL::PathnameList const& inAssociatedPathList,
		ASL::String const& inFormat) = 0;

	virtual void InitMiniMetadata(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::IngestItemList const& inIngestItems) = 0;

	virtual void AssetNodeRenamedStatus(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		PL::AssetMediaInfoList const& inAssetMediaInfos) = 0;

	virtual void TranscodeStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorInfo,
		ASL::String const& inOutPutFilePath) = 0;

	virtual void ConcatenationStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorInfo,
		ASL::String const& inOutPutFilePath) = 0;

	virtual void TranscodeProgress(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::SInt32 inPercent) = 0;

	virtual void ConcatenationProgress(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::SInt32 inPercent) = 0;

	virtual void ExportResultInformation(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorMsg,
		PL::ExportResultInfo const& inExportResultInfo) = 0;

	virtual void GetTargetBinPath(
		ASL::Guid const& inTaskID, 
		ASL::Guid const& inMsgID) = 0;

	virtual void GetCurrentWorkModeStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::String const& inCurrentMode) = 0;

	virtual void IsPProInstalledStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		bool inPProInstalled) = 0;

	virtual void InsertDynamicColumnsStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorInfo) = 0;

	virtual void DeleteDynamicColumnsStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorInfo) = 0;

	virtual void UpdateDynamicColumnFieldsStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		ASL::Result inResult,
		ASL::String const& inErrorInfo) = 0;

	virtual void SendTagTemplateStatus(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::TagTemplateMessageResultList const& inResultInfoList) = 0;

	virtual void MarkerSelectionChanged(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::ChangedMarkerInfoList const& inSelectedMarkers) = 0;

	virtual void SilkRoadMarkerChanged(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::ChangedMarkerInfoList const& inChangedMarkers) = 0;

	virtual void SilkRoadMarkerAdded(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::ChangedMarkerInfoList const& inAddedMarkers) = 0;

	virtual void SilkRoadMarkerDeleted(
		ASL::Guid const& inTaskID,
		ASL::Guid const& inMsgID,
		PL::ChangedMarkerInfoList const& inDeletedMarkers) = 0;
};

} // namespace PL

#endif // IPLASSETLIBRARYNOTIFIER_H
