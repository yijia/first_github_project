//
//  PLUnassociatedMarkers.cpp
//  PLCore.mp
//
//  Created by labuser on 6/3/14.
//
//

#include "PLUnassociatedMarkers.h"
#include "PLUtilities.h"

namespace PL
{
    /*
     **
     */
    SRUnassociatedMarkers::SRUnassociatedMarkers()
    {
    }
    
    /*
     **
     */
    SRUnassociatedMarkers::~SRUnassociatedMarkers()
    {
    }
    
    /*
     **
     */
    PL::ISRUnassociatedMarkersRef SRUnassociatedMarkers::Create(ASL::String const& inFilePath)
    {
        SRUnassociatedMarkersRef unassociatedMarkers = CreateClassRef();
        unassociatedMarkers->Init(inFilePath);
        
        return PL::ISRUnassociatedMarkersRef(unassociatedMarkers);
    }
    
    /*
     **
     */
    void SRUnassociatedMarkers::Init(ASL::String const& inFilePath)
    {
        mFilePath = inFilePath;
    }
    
    /*
     **
     */
    bool SRUnassociatedMarkers::BuildMarkersFromXMPString(
                                                          XMPText inXMPString,
                                                          bool)
    {
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        
        // get rid of the old ones...
        mMarkers.clear();
        
        Utilities::BuildMarkersFromXMPString(*inXMPString.get(), mTrackTypes, mMarkers);
        
        return true;
    }
    
    /*
     **
     */
    bool SRUnassociatedMarkers::GetMarker(
                                          ASL::Guid inMarkerID,
                                          CottonwoodMarker& outMarker)
    {
        CottonwoodMarker marker;
        marker.SetGUID(inMarkerID);
        
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        MarkerSet::iterator itr = mMarkers.find(marker);
        if (itr != mMarkers.end())
        {
            outMarker = *itr;
            return true;
        }
        return false;
    }
    
    /**
     ** Checks, if there is a marker, matching the given properties
     */
    bool SRUnassociatedMarkers::GetMarkerGUID(
                                              dvacore::UTF16String inMarkerType,
                                              std::int64_t inMarkerStartTime,
                                              std::int64_t inMarkerDuration,
                                              ASL::Guid& outGUID)
    {
        dvamediatypes::TickTime startTime = dvamediatypes::TickTime::TicksToTime(inMarkerStartTime);
        dvamediatypes::TickTime duration = dvamediatypes::TickTime::TicksToTime(inMarkerDuration);
        
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
        {
            bool boolStartTimeEqual = false;
            if (startTime == (*itr).GetStartTime())
            {
                boolStartTimeEqual = true;
            }
            
            bool boolDurationEqual = false;
            if (duration == (*itr).GetDuration())
            {
                boolDurationEqual = true;
            }
            
            bool boolTypeEqual = false;
            if (inMarkerType == (*itr).GetType())
            {
                boolTypeEqual = true;
            }
            
            if (boolStartTimeEqual && boolDurationEqual && boolTypeEqual)
            {
                outGUID = (*itr).GetGUID();
                return true;
            }
        }
        
        outGUID = ASL::Guid();
        return false;
    }
    
    /**
     ** Find a marker in all MarkerTracks
     */
    bool SRUnassociatedMarkers::HasMarker(const ASL::Guid& inMarkerId)
    {
        MarkerSet::const_iterator end = mMarkers.end();
        MarkerSet::const_iterator it = mMarkers.begin();
        
        for (; it != end; ++it)
        {
            const CottonwoodMarker& marker = *it;
            if ( marker.GetGUID() == inMarkerId )
            {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     ** Get All markers mixed into a single track sorted by start time
     */
    MarkerTrack SRUnassociatedMarkers::GetMarkers()
    {
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        MarkerTrack track;
        for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
        {
            track.insert(std::make_pair((*itr).GetStartTime(), (*itr)));
        }
        return track;
    }
    
    /**
     ** Get all markers split into separate tracks
     */
    MarkerTracks SRUnassociatedMarkers::GetMarkerTracks()
    {
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        MarkerTracks tracks;
        
        for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
        {
            dvacore::UTF16String markerType = (*itr).GetType();
            tracks.insert(std::make_pair(markerType, MarkerTrack()));
        }
        
        for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
        {
            dvacore::UTF16String markerType = (*itr).GetType();
            dvamediatypes::TickTime startTime = (*itr).GetStartTime();
            CottonwoodMarker marker = *itr;
            tracks[markerType].insert(std::make_pair(startTime, marker));
        }
        return tracks;
    }
    
    /**
     ** Get a specific markers by type in a track
     */
    MarkerTrack SRUnassociatedMarkers::GetMarkerTrack(
                                                      const dvacore::UTF16String& inMarkerType)
    {
        ASL::CriticalSectionLock lock(mMarkerCriticalSection);
        MarkerTrack track;
        for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
        {
            if (itr->GetType() == inMarkerType)
            {
                track.insert(std::make_pair(itr->GetStartTime(), (*itr)));
            }
        }
        
        return track;
    }
    
    /**
     **	Get a time-sorted and filtered list of markers
     */
    void SRUnassociatedMarkers::GetTimeSortedAndFilterMarkerList(
                                                                 const MarkerTypeFilterList& inFilter,
                                                                 const dvacore::UTF16String& inFilterText,
                                                                 CottonwoodMarkerList& outMarkers,
                                                                 const dvamediatypes::TickTime& inInPoint,
                                                                 const dvamediatypes::TickTime& inDuration)
    {
        bool hasFilter = !inFilter.empty();
        bool hasFilterText = !inFilterText.empty();
        MarkerTrack markers(GetMarkers());
        MarkerTrack::iterator iter(markers.begin());
        for ( ; iter != markers.end(); ++iter)
        {
            bool insert = true;
            if (hasFilter)
            {
                insert = inFilter.find(iter->second.GetType()) != inFilter.end();
            }
            if (insert && hasFilterText)
            {
                insert = IsMarkerFilteredByString(iter->second, inFilterText);					
            }
            
            if (insert)
            {
                dvamediatypes::TickTime inPoint = iter->second.GetStartTime();
                dvamediatypes::TickTime outPoint = iter->second.GetStartTime() + iter->second.GetDuration();
                
                if (inPoint>= inInPoint && outPoint<= inInPoint + inDuration)
                {
                    outMarkers.push_back(iter->second);
                }
            }
        }
    }

}