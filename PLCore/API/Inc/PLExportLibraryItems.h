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

#ifndef PLEXPORTLIBRARYITEMSOPERATION_H
#define PLEXPORTLIBRARYITEMSOPERATION_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef BEPROJECTFWD_H
#include "BEProjectFwd.h"
#endif

#ifndef MZPROJECTCONVERTERS_H
#include "MZProjectConverters.h"
#endif

#ifndef	PLFTPSETTINGSDATA_H
#include "PLFTPSettingsData.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLMESSAGESTRUCT_H
#include "PLMessageStruct.h"
#endif

namespace PL
{
/**
**	InterfaceRef & ID for IExportLibraryItemsOperation.
*/
ASL_DEFINE_IREF(IExportLibraryItemsOperation);
ASL_DEFINE_IID(IExportLibraryItemsOperation, 0xcf059692, 0x572c, 0x43f0, 0x88, 0xb1, 0xb8, 0xd7, 0x2, 0x96, 0x76, 0x3);

/**
**	IExportLibraryItemsOperation.
**	@see ASL::IBatchOperation.
*/
ASL_INTERFACE IExportLibraryItemsOperation : public ASLUnknown
{
	/**
	**	Get the result of the export media operation (once completed.)
	**
	**	@return					the export media result.
	*/
	virtual ASL::Result GetResult() const = 0;

	/**
	 **	Get export errors
	 */
	virtual ASL::String GetExportErrorMessage() const = 0;

	/**
	** Get export warnings
	*/
	virtual ASL::String GetExportWarningMessage() const = 0;
	
	/**
	 **	Set export data
	 */
	virtual void SetExportInfo(
		ASL::String const& inSeedPremiereProjectFile,
		std::set<ASL::String> const& inExportMediaFiles) = 0;

	/**
	 **	Set result list
	 */
	virtual void SetResultList(ActionResultList const& inActionResultList) = 0;
	
	/**
	**  If passed the check conditions, return ASL::kSuccess;
	**  If users cancel, return ASL::eUserCanceled
	*/
	virtual ASL::Result PrecheckExportFiles() const = 0;

	virtual void SetResult(ASL::Result inResult) = 0;

	virtual ASL::String GetFullProjectPath() const = 0;
};

/**
**
*/
PL_EXPORT
IExportLibraryItemsOperationRef CreateLocalExportLibraryItemsOperation(
	ML::IProjectConverterModuleRef inConverterModule, 
	ASL::String const& inDestinationFolder,
	ASL::String const& inProjectFile);

/**
**
*/
PL_EXPORT
IExportLibraryItemsOperationRef CreateFTPExportLibraryItemsOperation(
	ML::IProjectConverterModuleRef inConverterModule,
	ASL::String const& inDestinationFolder,
	ASL::String const& inProjectFile,
	PL::FTPSettingsData::SharedPtr inFTPSettingsDataPtr
	);

} // namespace PL

#endif
