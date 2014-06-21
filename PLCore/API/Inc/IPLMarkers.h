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

#ifndef IPLMARKERS_H
#define IPLMARKERS_H

// MZ
#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef PLMARKERTYPEREGISTRY_H
#include "PLMarkerTypeRegistry.h"
#endif

// BE
#ifndef BEMASTERCLIP_H
#include "BEMasterClip.h"
#endif

// DVA
#ifndef DVAMETADATA_METADATAPROVIDERS_H
#include "dvametadata/MetaDataProviders.h"
#endif

// DVA
#ifndef DVACORE_UTILITY_SHAREDREFPTR_H
#include "dvacore/utility/SharedRefPtr.h"
#endif

// boost
#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

// ASL
#ifndef ASLCRITICALSECTION_H
#include "ASLCriticalSection.h"
#endif

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

namespace PL
{
    
    /**
     **	InterfaceRef & ID for IMarkerOwner.
     */
    ASL_DEFINE_IREF(ISRMarkers);
    ASL_DEFINE_IID(ISRMarkers, 0xc634e0a6, 0x2e3f, 0x4042, 0xa3, 0xca, 0x33, 0xcb, 0xdf, 0xab, 0x37, 0x9a)
    typedef std::vector<PL::ISRMarkersRef> SRMarkersList;
    
	ASL_INTERFACE ISRMarkers : public ASLUnknown
	{

	public:

		virtual bool BuildMarkersFromXMPString(XMPText inXMPString, bool inIsInitialized) = 0;

		virtual XMPText BuildXMPStringFromMarkers() = 0;

		virtual bool GetMarker(
					   ASL::Guid inMarkerID,
					   CottonwoodMarker& outMarker) = 0;
		
		virtual void AddMarker(const CottonwoodMarker& inNewMarker, bool inIsSilent) = 0;
		
		virtual void AddMarkers(const CottonwoodMarkerList& inMarkers, bool inIsSilent) = 0;
		
		virtual void RemoveMarker(const CottonwoodMarker& inNewMarker) = 0;
		
		virtual void RemoveMarkers(const CottonwoodMarkerList& inMarkers) = 0;
		
		virtual void UpdateMarker(
					  const CottonwoodMarker& inOldMarker,
					  const CottonwoodMarker&	inNewMarker) = 0;

		virtual void UpdateMarkers(const CottonwoodMarkerList& inMarkers) = 0;
								  
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
		
		virtual BE::IMasterClipRef GetMediaMasterClip() const = 0;
	};
}

#endif
