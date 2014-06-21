/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

#ifndef PLCONSTANTS_H
#define PLCONSTANTS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

// ASL
#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef BEPROPERTYKEY_H
#include "BEPropertyKey.h"
#endif

// MZ
#ifndef MZEXTERNALDATA_H
#include "MZExternalData.h"
#endif

namespace PL
{

PL_EXPORT extern const ASL::StationID kStation_SubclipInOut;
//PL_EXPORT extern const ASL::StationID kStation_PreludeProject;
PL_EXPORT extern const ASL::StationID kStation_PreludeStoredMetadata;

PL_EXPORT extern const BE::PropertyKey kLastUsedRoughCutSaveDirKey;

PL_EXPORT extern const BE::PropertyKey kPrefsProjectCacheFilesLocation;
PL_EXPORT extern const BE::PropertyKey kPrefsProjectCacheLongestReservedDays;
PL_EXPORT extern const BE::PropertyKey kPrefsProjectCacheMaximumReservedNumber;
PL_EXPORT extern const BE::PropertyKey kPrefsProjectCacheEnable;
PL_EXPORT extern const BE::PropertyKey kPrefsProjectCacheRadioIndex;
PL_EXPORT extern const BE::PropertyKey kPropertyKey_ShowQuickstartDialog;
PL_EXPORT extern const BE::PropertyKey kPropertyKey_LastTagTemplateDirectory;


PL_EXPORT extern const BE::PropertyKey kCreativeCloudDownloadLocation;

// Ingest
PL_EXPORT extern const BE::PropertyKey kPrefsKeepTranscodeState;

} //namespace PL

#endif //#ifndef PLCONSTANTS_H
