/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2013 Adobe Systems Incorporated                       */
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

#ifndef PLMETADATAACTIONS_H
#define PLMETADATAACTIONS_H

#ifndef BE_MEDIA_IMEDIAMETADATAFWD_H
#include "BE/Media/IMediaMetaDataFwd.h"
#endif

#ifndef BE_CLIP_IMASTERCLIPFWD_H
#include "BE/Clip/IMasterClipFwd.h"
#endif

#ifndef BEEXECUTORFWD_H
#include "BEExecutorFwd.h"
#endif

#ifndef MFBINARYDATA_H
#include "MFBinaryData.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef DVAMETADATA_METADATAPROVIDER_H
#include "dvametadata/MetadataProvider.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef IPLMARKERSELECTION_H
#include "IPLMarkerSelection.h"
#endif

#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

namespace PL
{

/**
**	Create an action to remove a set of markers.
**
**	@param	MarkerSet		set of selected markers
*/


PL_EXPORT
	bool RemoveCottonwoodMarkers(
	const PL::MarkerSet& MarkerSet);

PL_EXPORT
	bool UpdateCottonwoodMarker(
	BE::IMasterClipRef inMasterClip,
	const PL::CottonwoodMarker& inOldMarker,
	const PL::CottonwoodMarker& inNewMarker);

PL_EXPORT
	bool UpdateCottonwoodMarkersWithoutTypeChange(
	BE::IMasterClipRef inMasterClip,
	MarkerToUpdatePairList inMarkersToUpdate);

/**
**	Create an action to add a custom marker.
**
**	@param	inMarker		CWMarker to add
**	@param	inMasterClip	Which masterclip we want to add marker
**	@param	inIsSilent		If we need to add new marker to selection
*/

PL_EXPORT
	void AddCottonwoodMarker(
	const PL::CottonwoodMarker& inMarker,
	BE::IMasterClipRef	inMasterClip,
	bool inIsSilent);


PL_EXPORT
	void AddCottonwoodMarkers(
	const PL::CottonwoodMarkerList& inMarkerList,
	BE::IMasterClipRef	inMasterClip,
	bool inIsSilent);

/*
** Create an action to associate markers with the passed in master clip.
*/
PL_EXPORT
	void AssociateMarkers(
	const PL::CottonwoodMarkerList& inMarkerList,
	BE::IMasterClipRef inMasterClip,
	bool inIsSilent);
}

#endif