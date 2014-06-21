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

#ifndef PLCLIPFINDERUTILS_H
#define PLCLIPFINDERUTILS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef MBCPROVIDERUTILS_H
#include "Inc/MBCProviderUtils.h"
#endif

// ASL
#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

namespace PL
{
	PL_EXPORT
	void GetLogicalClipFromMediaPath(
				ASL::String const& inMediaPath,
				ASL::String& outSourceStem,
				ASL::PathnameList& outFileList,
				ASL::String& outClipName,
				ASL::String& outProviderID);

	PL_EXPORT
	void GetLogicalClipNameFromMediaPath(
				ASL::String const& inMediaPath,
				ASL::String& outClipName);

	PL_EXPORT
	void ExtractPaths(
				const PLMBC::Provider::Datum_t& inDatum, 
				ASL::PathnameList& outPaths);
}

#endif