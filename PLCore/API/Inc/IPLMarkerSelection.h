/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2010 Adobe Systems Incorporated	                   */
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

#ifndef IPLMARKERSELECTION_H
#define IPLMARKERSELECTION_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

namespace PL
{


ASL_DEFINE_IREF(ISRMarkerSelection);
ASL_DEFINE_IID(ISRMarkerSelection, 0x3cea565b, 0x420a, 0x4882, 0x95, 0x8e, 0x65, 0xa4, 0x2, 0x1d, 0x3a, 0xb9);

	
ASL_INTERFACE ISRMarkerSelection : public ASLUnknown
{
public:

	/*
	**
	*/
	virtual void ClearSelection() = 0;

	/*
	**	Add markers to the selection
	*/
	virtual void AddMarkersToSelection(
							const MarkerSet& inMarkersToAdd) =0;


	/*
	**	Add a marker to the selection
	*/
	virtual void AddMarkerToSelection(
							const CottonwoodMarker& inMarker)
	{
		MarkerSet selectMap;
		selectMap.insert(inMarker);
		AddMarkersToSelection(selectMap);
	}

	/*
	**	Remove a marker from the selection
	*/
	virtual void RemoveMarkerFromSelection(
							const CottonwoodMarker& inMarker)
	{
		MarkerSet selectMap;
		selectMap.insert(inMarker);
		RemoveMarkersFromSelection(selectMap);
	}

	/*
	**	Remove markers from the selection
	*/
	virtual void RemoveMarkersFromSelection(
							const MarkerSet& inMarkersToRemove) =0;

	/*
	**	Is there a marker selection?
	*/
	virtual bool AreMarkersSelected() =0;

	/*
	**	Is a marker selected?
	*/
	virtual bool IsMarkerSelected(
							const CottonwoodMarker& inMarker) =0;

	/*
	**	Get Marker Selection set
	*/
	virtual MarkerSet GetMarkerSelection() const =0;

	/*
	** Get the GUID for the most recently added marker 
	*/
	virtual ASL::Guid GetMostRecentMarkerInSelection() =0;
};
}
#endif