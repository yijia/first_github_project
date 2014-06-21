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

#ifndef PLDRAGMARKERINFO_H
#define PLDRAGMARKERINFO_H

//	MediaCore
#include "XMLUtils.h"

// dvacore
#ifndef DVACORE_SERIALIZATION_SERIALIZEDOBJECT_H
#include "dvacore/serialization/SerializedObject.h"
#endif

#ifndef DVACORE_SERIALIZATION_SERIALIZER_H
#include "dvacore/serialization/Serializer.h"
#endif

// MZ
#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif


namespace PL 
{

struct DragMarkerInfo
{
public:
	DragMarkerInfo() {}
	explicit DragMarkerInfo(
	dvacore::utility::Guid inMarkerGUID,
	dvacore::UTF16String inMediaPath,
	bool inIsZeroDuration) :
	mMarkerGUID(inMarkerGUID),
	mMediaPath(inMediaPath),
	mIsZeroDuration(inIsZeroDuration)
	{}

	dvacore::utility::Guid		mMarkerGUID;
	dvacore::UTF16String		mMediaPath;
	bool	mIsZeroDuration;
};

typedef std::vector<DragMarkerInfo> DragMarkerInfoList;

PL_EXPORT
ASL::String EncodeDragMarkerInfoList(
	DragMarkerInfoList const& inDragMarkerInfoList);
PL_EXPORT
bool DecodeDragMarkerInfoList(
	ASL::String const&	inXMLString,
	DragMarkerInfoList& outDragMarkerInfoList);
void CreateAndSetNodeValue(
	DOMElement* inParent, 
	ASL::String const& inNodeName,
	ASL::String const& inNodeValue);
void CreateAndSetNodeValue(
	DOMElement* inParent, 
	ASL::String const& inNodeName,
	dvamediatypes::TickTime const& inTickTime);

}

#endif
