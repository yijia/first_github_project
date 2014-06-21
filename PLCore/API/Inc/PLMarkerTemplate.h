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

#ifndef PLMARKERTEMPLATE_H
#define PLMARKERTEMPLATE_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

namespace PL
{

namespace MarkerTemplate
{

/**
**	Sent when there is a template change for markers
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(MarkerTemplateChanged);

/**
**	Sent when there is a change in the list of marker template sets
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(MarkerTemplateSetListChanged);

struct Template
{
	dvacore::UTF16String mName;
	CottonwoodMarker mMarker;
};
typedef std::vector<Template> TemplateSet;

typedef std::vector<dvacore::UTF16String> TemplateSetNames;

void Initialize();

void Shutdown();

PL_EXPORT
ASL::StationID GetStationID();

PL_EXPORT
void GetCurrentTemplateSet(
	TemplateSet& outTemplateSet);

PL_EXPORT
dvacore::UTF16String GetCurrentTemplateSetName();

PL_EXPORT
bool IsCurrentTemplateSetDefault();

PL_EXPORT
bool IsCurrentTemplateSetSaved();

PL_EXPORT
void GetTemplateSetNames(
	TemplateSetNames& outTemplateSetNames);

PL_EXPORT
void SetCurrentTemplateSetName(
	const dvacore::UTF16String& inNewName);

PL_EXPORT
bool SaveMarkerToCurrentTemplateSet(
	const CottonwoodMarker& inMarker,
	const dvacore::UTF16String& inName,
	bool inForceOverwrite);

PL_EXPORT
void DeleteMarkerFromCurrentSet(
	const dvacore::UTF16String& inName);
	
PL_EXPORT
void MoveMarkerInCurrentSet(
	size_t inOldIndex,
	size_t inNewIndex);

PL_EXPORT
ASL::Result SaveCustomMarkerTemplateSet(
	const dvacore::UTF16String& inSetName,
	bool inForceOverwrite);

PL_EXPORT
bool IsSameAsDefaultName(
	 const dvacore::UTF16String& inSetName);

PL_EXPORT
void DeleteMarkerTemplateSet(
	const dvacore::UTF16String& inSetName);

PL_EXPORT
void RefreshDefaultMarkerTemplateSet();

PL_EXPORT
void SerializeTemplateToXMLString(
	Template inTemplate,
	dvacore::UTF16String& outString);

void AddTagString(
	const dvacore::UTF16String& inTagName,
	const dvacore::UTF16String& inValue,
	dvacore::UTF16String& outString);

PL_EXPORT
void SerializeTemplateFromXMLString(
	const dvacore::UTF16String& inString,
	Template& outTemplate);
	

}

}

#endif