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

//	Prefix
#include "Prefix.h"

#include "PLConstants.h"

namespace PL
{

extern const ASL::StationID kStation_SubclipInOut(ASL_STATIONID("MZ.SubclipInOut"));
//extern const ASL::StationID kStation_PreludeProject(ASL_STATIONID("MZ.PreludeProject"));
extern const ASL::StationID kStation_PreludeStoredMetadata(ASL_STATIONID("MZ.MetadataStored"));

extern const BE::PropertyKey kLastUsedRoughCutSaveDirKey(BE_PROPERTYKEY("RoughCut.last.used.directory"));

//	Property keys for ProjectCache files
extern const BE::PropertyKey kPrefsProjectCacheFilesLocation(BE_PROPERTYKEY("MZ.Prefs.Project.ClipCacheFilesLocation"));
extern const BE::PropertyKey kPrefsProjectCacheLongestReservedDays(BE_PROPERTYKEY("MZ.Prefs.Project.LongestReservedDays"));
extern const BE::PropertyKey kPrefsProjectCacheMaximumReservedNumber(BE_PROPERTYKEY("MZ.Prefs.Project.MaximumReservedNumber"));
extern const BE::PropertyKey kPrefsProjectCacheEnable(BE_PROPERTYKEY("MZ.Prefs.Project.IsEnable"));
extern const BE::PropertyKey kPrefsProjectCacheRadioIndex(BE_PROPERTYKEY("MZ.Prefs.Project.RadioIndex"));
extern const BE::PropertyKey kPropertyKey_ShowQuickstartDialog (BE_PROPERTYKEY("MZ.Prefs.ShowQuickstartDialog"));
extern const BE::PropertyKey kPropertyKey_LastTagTemplateDirectory (BE_PROPERTYKEY("PL.Prefs.LastTagTemplateDirectory"));

//	Property keys for CreativeCloud pereference
extern const BE::PropertyKey kCreativeCloudDownloadLocation(BE_PROPERTYKEY("MZ.Prefs.CreativeCloudDownloadLocation"));

// Ingest
extern const BE::PropertyKey kPrefsKeepTranscodeState(BE_PROPERTYKEY("PL.Prefs.Ingest.KeepTranscodeState"));
} //namespace PL
