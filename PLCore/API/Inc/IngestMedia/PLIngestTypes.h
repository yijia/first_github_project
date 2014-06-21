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

#ifndef PLINGESTTYPES_H
#define PLINGESTTYPES_H

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLINGESTVERIFYTYPES_H
#include "IngestMedia/PLIngestVerifyTypes.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
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

	enum CopyAction
	{
		kCopyAction_Copied,
		kCopyAction_Ignored,
		kCopyAction_Replaced,
		kCopyAction_Renamed,
		kCopyAction_NoFurtherAction,
	};


	enum CopyExistOption
	{
		kExist_WarnUser,
		kExist_Ignore,
		kExist_Replace, // To exist folder, replace means merge. Truly replace for file.
	};


#ifndef XMPText
	typedef boost::shared_ptr<ASL::StdString> XMPText;
#endif

	struct FileCreateAndModifyTime 
	{
		ASL::String mFileCreateTime;
		ASL::String mFileModifyTime;
	};

	/**
	**
	*/
	typedef std::map<ASL::String, ASL::String>  NamespaceMetadataValueMap;
	struct NamespaceMetadata
	{
		ASL::String mNamespaceUri;
		ASL::String mNamespacePrefix;
		NamespaceMetadataValueMap mNamespaceMetadataValueMap;
	};
	typedef boost::shared_ptr<NamespaceMetadata> NamespaceMetadataPtr;
	typedef std::list<NamespaceMetadataPtr> NamespaceMetadataList;
	typedef std::set<ASL::String>  KeywordSet;
	struct MinimumMetadata
	{
		MinimumMetadata()
		: 
		mStoreEmptyInfo(false),
		mIsApplyToAllDestinations(false)
		{};

		NamespaceMetadataList mNamespaceMetadataList;
		KeywordSet	mKeywordSet;
		bool mStoreEmptyInfo;
		bool IsEmpty() const { return mNamespaceMetadataList.empty() && mKeywordSet.empty(); }
		bool mIsApplyToAllDestinations;
	};

	struct IngestCustomData
	{
		NamespaceMetadataList mMetadata;
		MinimumMetadata mMinimumMetadata;
	};

	PL_EXPORT extern const ASL::StationID kStation_IngestMedia;

	// General result list typedef. Infact CopyResult is same with TranscodeResult, I hope we can replace them with this general types.
	typedef std::pair<ASL::Result, ASL::String> ResultReport;
	typedef std::vector<ResultReport>			ResultReportVector;

	// Copy Result
	typedef ResultReport CopyResult;
	typedef ResultReportVector CopyResultVector;

	// Transcode result
	typedef ResultReport TranscodeResult;
	typedef ResultReportVector TranscodeResultVector;
} // namespace PL

#endif
