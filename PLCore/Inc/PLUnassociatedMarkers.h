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

#ifndef PLUNASSOCIATEDMARKERS_H
#define PLUNASSOCIATEDMARKERS_H

// local

#ifndef IPLMEDIA_H
#include "IPLMedia.h"
#endif

#ifndef IPLROUGHCUT_H
#include "IPLRoughCut.h"
#endif

#ifndef IPLSRMEDIA_H
#include "IPLMedia.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef IPLUNASSOCIATEDMARKERS_H
#include "IPLUnassociatedMarkers.h"
#endif

#ifndef PLMESSAGE_H
#include "PLMessage.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

namespace PL
{
    ASL_DEFINE_CLASSREF(SRUnassociatedMarkers, ISRUnassociatedMarkers);
    ASL_DEFINE_IMPLID(SRUnassociatedMarkers, 0x08fe36d4, 0xe8a7, 0x11e3, 0xab, 0x0c, 0x08, 0x5b, 0x1e, 0x5d, 0x46, 0xb0);
    
    class SRUnassociatedMarkers : public ISRUnassociatedMarkers
    {
        
        ASL_BASIC_CLASS_SUPPORT(SRUnassociatedMarkers);
        ASL_QUERY_MAP_BEGIN
        ASL_QUERY_ENTRY(ISRUnassociatedMarkers)
        ASL_QUERY_MAP_END
        
    public:
        
        static PL::ISRUnassociatedMarkersRef Create(ASL::String const& inFilePath);
        
        SRUnassociatedMarkers();
        virtual ~SRUnassociatedMarkers();
        
    public:
        
        virtual bool BuildMarkersFromXMPString(XMPText inXMPString, bool/*useless*/);
        
        virtual bool GetMarker(
                               ASL::Guid inMarkerID,
                               CottonwoodMarker& outMarker);
        
        /**
         ** Find a marker in all MarkerTracks
         */
        virtual bool HasMarker(const ASL::Guid& inMarkerId);
        
        /**
         ** Checks, if there is a marker, matching the given properties
         */
        virtual bool GetMarkerGUID(
                                   dvacore::UTF16String inMarkerType,
                                   std::int64_t inMarkerStartTime,
                                   std::int64_t inMarkerDuration,
                                   ASL::Guid& outGUID);
        
        /**
         ** Get All markers mixed into a single track sorted by start time
         */
        virtual MarkerTrack GetMarkers();
        
        /**
         ** Get all markers split into separate tracks
         */
        virtual MarkerTracks GetMarkerTracks();
        
        /**
         ** Get a specific markers by type in a track
         */
        virtual MarkerTrack GetMarkerTrack(const dvacore::UTF16String& inMarkerType);
        
        /**
         **	Get a time-sorted and filtered list of markers
         */
        virtual void GetTimeSortedAndFilterMarkerList(
                                                      const MarkerTypeFilterList& inFilter,
                                                      const dvacore::UTF16String& inFilterText,
                                                      CottonwoodMarkerList& outMarkers,
                                                      const dvamediatypes::TickTime& inInPoint = dvamediatypes::kTime_Min,
                                                      const dvamediatypes::TickTime& inDuration = (dvamediatypes::kTime_Max - dvamediatypes::kTime_Min));
    
        
    private:
        
        void Init(ASL::String const& inFilePath);
    private:
        MarkerSet               mMarkers;
        ASL::String				mFilePath;
        ASL::CriticalSection	mMarkerCriticalSection;
        TrackTypes				mTrackTypes;
    };
}

#endif
