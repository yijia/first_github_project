/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2010 Adobe Systems Incorporated
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

#ifndef PLINGESTMEDIAMESSAGE_H
#define PLINGESTMEDIAMESSAGE_H

// MZ
#ifndef PLINGESTJOB_H
#include "IngestMedia/PLIngestJob.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

// ASL
#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

namespace PL
{
//typedef std::list<ASL::String> FailureMessageList;
//typedef std::list<ASL::String> WarningMessageList;
//
//ASL_DECLARE_MESSAGE_WITH_2_PARAM(
//	IngestJobStartMessage,
//	ASL::Guid,
//	ASL::String);
//
//ASL_DECLARE_MESSAGE_WITH_2_PARAM(
//	IngestJobFinishMessage,
//	ASL::Guid,
//	ASL::String);
//	
//ASL_DECLARE_MESSAGE_WITH_2_PARAM(
//	IngestJobProgressMessage,
//	ASL::Guid,
//	IIngestJobProgressRef);
//
//ASL_DECLARE_MESSAGE_WITH_1_PARAM(
//	IngestJobInitFailedMessage,
//	IIngestJobItemRef);
//
//ASL_DECLARE_MESSAGE_WITH_2_PARAM(
//	IngestJobFailureReportMessage,
//	ASL::Guid,
//	FailureMessageList);
//
//ASL_DECLARE_MESSAGE_WITH_2_PARAM(
//	IngestJobWarningReportMessage,
//	ASL::Guid,
//	WarningMessageList);

//----------------------------------------------
ASL_DECLARE_MESSAGE_WITH_3_PARAM(
	IngestTaskStatusMessage,
	ASL::Guid,
	std::size_t,
	ASL::String);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(
	IngestStartMessage);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(
	IngestFinishMessage);

ASL_DECLARE_MESSAGE_WITH_1_PARAM(
	IngestTaskProgressMessage,
	double);

// Ingest Job control message from status bar 
ASL_DECLARE_MESSAGE_WITH_2_PARAM(
	StartIngestMessage,
	ASL::Guid,
	ASL::String);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(
	PauseIngestMessage);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(
	CancelIngestMessage);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(
	ResumeIngestMessage);

// Copy related messages
ASL_DECLARE_MESSAGE_WITH_5_PARAM(   
	CopyFinishedMessage,
	ASL::Guid,
	ASL::Result,
	CopyResultVector,
	std::size_t /*success count*/,
	std::size_t /*failed count*/);

ASL_DECLARE_MESSAGE_WITH_1_PARAM(   
	CopyProgressMessage,
	double);

// Update metadata related messages
ASL_DECLARE_MESSAGE_WITH_3_PARAM(   
	UpdateMetadataFinishedMessage,
	ASL::Guid,
	ASL::Result,
	ResultReportVector);

// Import related messages
ASL_DECLARE_MESSAGE_WITH_1_PARAM(	
	ImportFinishedMessage,
	ASL::Guid);

ASL_DECLARE_MESSAGE_WITH_1_PARAM(
	ImportXMPMessage,
	PL::IngestTask::ImportXMPElement);

ASL_DECLARE_MESSAGE_WITH_1_PARAM(
	RequestThumbnailMessage,
	PL::IngestTask::RequestThumbnailElement);

ASL_DECLARE_MESSAGE_WITH_5_PARAM(
	FinishImportFileMessage,
	ASL::String,
	ASL::Guid,
	ASL::Guid,
	ASL::Result,
	ASL::String);

ASL_DECLARE_MESSAGE_WITH_3_PARAM(
	UpdateTargetBinMessage,
	ASL::Guid,
	ASL::Guid,
	ASL::String);

ASL_DECLARE_MESSAGE_WITH_2_PARAM(
	MiniMetadataFinishedMessage,
	bool,
	PL::MinimumMetadata);

/**
**	Station IngestMedia messages are sent over.
*/
// PL_EXPORT extern ASL::StationID const kStation_IngestMedia;

} // namespace PL

#endif // PLINGESTMEDIAMESSAGE_H
