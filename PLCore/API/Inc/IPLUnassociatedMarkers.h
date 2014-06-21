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

#ifndef IPLUNASSOCIATEDMARKERS_H
#define IPLUNASSOCIATEDMARKERS_H

// MZ
#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef PLMARKERTYPEREGISTRY_H
#include "PLMarkerTypeRegistry.h"
#endif



// DVA
#ifndef DVAMETADATA_METADATAPROVIDERS_H
#include "dvametadata/MetaDataProviders.h"
#endif


#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

namespace PL
{
    
    /**
     **	InterfaceRef & ID for IUnassociatedMarkers.
     */
    ASL_DEFINE_IREF(ISRUnassociatedMarkers);
    ASL_DEFINE_IID(ISRUnassociatedMarkers, 0xa5aa1a90, 0xc811, 0x4aca, 0x89, 0x5e, 0xeb, 0xf4, 0x96, 0xb4, 0x18, 0xf9)
    
	ASL_INTERFACE ISRUnassociatedMarkers : public ASLUnknown
	{
        
	public:
        
		virtual bool BuildMarkersFromXMPString(XMPText inXMPString, bool inIsInitialized) = 0;
        
    
		virtual bool GetMarker(
                               ASL::Guid inMarkerID,
                               CottonwoodMarker& outMarker) = 0;
        
		/**
         ** Checks, if there is a marker, matching the given properties
         */
		virtual bool GetMarkerGUID(
                                   dvacore::UTF16String inMarkerType,
                                   std::int64_t inMarkerStartTime,
                                   std::int64_t inMarkerDuration,
                                   ASL::Guid& outGUID) = 0;
        
		/**
         ** Get All markers mixed into a single track sorted by start time
         */
		virtual MarkerTrack GetMarkers() = 0;
        
		/**
         ** Get all markers split into separate tracks
         */
		virtual MarkerTracks GetMarkerTracks() = 0;
        
		/**
         ** Get a specific markers by type in a track
         */
		virtual MarkerTrack GetMarkerTrack(const dvacore::UTF16String& inMarkerType) = 0;
        
		/**
         ** Find a marker in all MarkerTracks
         */
		virtual bool HasMarker(const ASL::Guid& inMarkerId) = 0;
        
		/**
         **	Get a time-sorted and filtered list of markers
         */
		virtual void GetTimeSortedAndFilterMarkerList(
                                                      const MarkerTypeFilterList& inFilter,
                                                      const dvacore::UTF16String& inFilterText,
                                                      CottonwoodMarkerList& outMarkers,
                                                      const dvamediatypes::TickTime& inInPoint = dvamediatypes::kTime_Min,
                                                      const dvamediatypes::TickTime& inDuration = (dvamediatypes::kTime_Max - dvamediatypes::kTime_Min)) = 0;
	};
}

#endif
