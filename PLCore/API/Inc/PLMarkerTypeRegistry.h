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

#ifndef PLMARKERTYPEREGISTRY_H
#define PLMARKERTYPEREGISTRY_H

#include "prefix.h"
#include "dvacore/config/UnicodeTypes.h"
#include "ASLMap.h"
#include <map>

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLCOLOR_H
#include "ASLColor.h"
#endif

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

namespace PL
{

struct MarkerTypeHandlerInfo
{
	dvacore::UTF16String mName;
	dvacore::UTF16String mSwfPath;
	std::uint32_t mPriority;
	ASL::Color mColor;
	bool	mEnabled;
};

typedef std::map<dvacore::UTF16String, MarkerTypeHandlerInfo> MarkerTypeRegistry;
typedef std::vector<dvacore::UTF16String> MarkerTypeNameList;

namespace MarkerType
{

	PL_EXPORT extern const dvacore::UTF16String kInOutMarkerType;
	PL_EXPORT extern const dvacore::UTF16String kCommentMarkerType;
	PL_EXPORT extern const dvacore::UTF16String kFLVCuePointMarkerType;
	PL_EXPORT extern const dvacore::UTF16String kWebLinkMarkerType;
	PL_EXPORT extern const dvacore::UTF16String kChapterMarkerType;
	PL_EXPORT extern const dvacore::UTF16String kSpeechMarkerType;


/**
**	Sent when there is a change in the marker type registry
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(MarkerTypeRegistryChanged);

/**
**	Set MarkerType
*/
PL_EXPORT
void Initialize();

/**
**	Shut down the MarkerType
*/
PL_EXPORT
void Shutdown();

/**
**	Is a Custom Marker Type
*/
PL_EXPORT
bool IsCustomMarkerType(const ASL::String& inMarkerType);

/**
**  Custom Marker ID string
*/
PL_EXPORT
dvacore::UTF16String GetCustomMarkerIDStr();

/**
**
*/
PL_EXPORT
ASL::StationID GetStationID();

/**
*	Register marker types from the designated folder /MarkerTypeHandlers.
*
*	The folder structure for Marker Type Handlers is /MarkerTypeHandlers/<Marker Type Name>/<Marker Type Name>_MarkerTypeHandler.swf. 
*	The name of the folder that contains the marker type handler swf does not matter.
*
*	It is important that the part before the "_" in the name of the swf is the actual marker type name, 
*	since this is the marker type name that the registry will store. Also, the name must contain the string
*	"_MarkerTypeHandler", as this is how marker type handlers are differentiated from regular CSXS extensions.
 
*/
PL_EXPORT
void RegisterMarkerTypes();

PL_EXPORT
void RegisterMarkerType(
	dvacore::UTF16String markerType,
	MarkerTypeHandlerInfo& inInfo);

PL_EXPORT
void UnregisterMarkerType(
	dvacore::UTF16String markerType);

PL_EXPORT
void RegisterCustomMarkerType(
	const dvacore::UTF16String& inLibraryFullPath
	);

PL_EXPORT
void AddMarkerToRegistry(
	const dvacore::UTF16String& inMarkerName,
	const dvacore::UTF16String& inMarkerDisplayName,
	const dvacore::UTF16String& inMarkerHandlerName,
	const dvacore::UTF16String& swfPath);

/**
**	Get all registered marker types
*/
PL_EXPORT
const MarkerTypeRegistry GetAllMarkerTypes();

/**
**	Get all registered marker types
*/
PL_EXPORT
const MarkerTypeNameList GetSortedMarkerTypes();

/**
**	Get the color associated with a marker type
*/
PL_EXPORT
ASL::Color GetColorForType(
	const dvacore::UTF16String& inType);

/**
**	Set the color associated with a marker type. Returns true if this is a change.
*/
PL_EXPORT
bool SetColorForType(
	const dvacore::UTF16String& inType,
	const ASL::Color& inColor);

PL_EXPORT
dvacore::UTF16String GetDisplayNameForType( 
	const dvacore::UTF16String& inType);

} //namespace MarkerType

} // namespace PL
#endif