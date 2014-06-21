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

#ifndef PLMARKERS_H
#define PLMARKERS_H

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

ASL_DEFINE_CLASSREF(SRMarkers, ISRMarkers);
ASL_DEFINE_IMPLID(SRMarkers, 0x7d625a8d, 0x611, 0x43db, 0x84, 0xf, 0x34, 0xa2, 0x3b, 0x78, 0xe7, 0x51);
    
class SRMarkers
	: 
	public ISRMarkers,
	public ASL::Listener
{
	ASL_STRONGWEAK_CLASS_SUPPORT(SRMarkers);
	ASL_QUERY_MAP_BEGIN
        ASL_QUERY_ENTRY(ISRMarkers)
	ASL_QUERY_MAP_END

    
public:

	ASL_MESSAGE_MAP_BEGIN(SRMarkers)
			ASL_MESSAGE_HANDLER(PL::MediaMetaDataMarkerChanged, MarkerChanged)
	ASL_MESSAGE_MAP_END

    static PL::ISRMarkersRef Create(ISRMediaRef inSRMedia);
    
	SRMarkers();
	virtual ~SRMarkers();
	
	virtual bool BuildMarkersFromXMPString(XMPText inXMPString, bool inSendNotification);

	virtual XMPText BuildXMPStringFromMarkers();
	
	virtual bool GetMarker(
						   ASL::Guid inMarkerID,
						   CottonwoodMarker& outMarker);
	
	virtual void AddMarker(const CottonwoodMarker& inNewMarker, bool inIsSilent);
	
	virtual void AddMarkers(const CottonwoodMarkerList& inMarkers, bool inIsSilent);
	
	virtual void RemoveMarker(const CottonwoodMarker& inNewMarker);
	
	virtual void RemoveMarkers(const CottonwoodMarkerList& inMarkers);
	
	virtual void UpdateMarker(
							  const CottonwoodMarker& inOldMarker,
							  const CottonwoodMarker&	inNewMarker);

	virtual void UpdateMarkers(const CottonwoodMarkerList& inMarkers);
	
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
	** Find a marker in all MarkerTracks
	*/
	virtual bool HasMarker(const ASL::Guid& inMarkerId);

	
	/**
	 **	Get a time-sorted and filtered list of markers
	 */
	virtual void GetTimeSortedAndFilterMarkerList(
												  const MarkerTypeFilterList& inFilter,
												  const dvacore::UTF16String& inFilterText,
												  CottonwoodMarkerList& outMarkers,
												  const dvamediatypes::TickTime& inInPoint = dvamediatypes::kTime_Min,
												  const dvamediatypes::TickTime& inDuration = (dvamediatypes::kTime_Max - dvamediatypes::kTime_Min));

	
	virtual BE::IMasterClipRef GetMediaMasterClip() const;

private:
    void Init(ISRMediaRef inSRMedia);
	void MarkerChanged(const ASL::String& inMediaLocatorID);
	void SetDirty(bool inDirty);
	void RefineMarker(CottonwoodMarker& ioMarker);
	dvamediatypes::FrameRate GetMediaFrameRate();
	dvamediatypes::TickTime  GetMediaDuration();
	ASL::Guid GetMediaInfoID();
	dvacore::StdString GetMarkerState(XMPText inXMPString);
	
private:
	MarkerSet               mMarkers;
	ASL::CriticalSection	mMarkerCriticalSection;
	TrackTypes				mTrackTypes;
	ISRMediaRef				mSRMedia;
	dvacore::StdString		mMarkerState;
};

}

#endif
