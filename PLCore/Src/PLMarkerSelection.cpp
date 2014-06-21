/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2014 Adobe Systems Incorporated                       */
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

//  PL
#include "PLMarkerSelection.h"
#include "PLMessage.h"
#include "PLConstants.h"

//  ASL
#include "ASLStationUtils.h"


namespace PL
{
    
    
    SRMarkerSelection::SRMarkerSelection()
    {
    }
    
    SRMarkerSelection::~SRMarkerSelection()
    {
    }
    
    /*
    **
    */
    void SRMarkerSelection::SetPrimaryClip(PL::ISRPrimaryClipPlaybackRef  inPrimaryClipPlayback)
    {
        mPrimaryClipPlayback = inPrimaryClipPlayback;
    }
    
    /*
    **
    */
    void SRMarkerSelection::ClearSelection()
    {
        if (AreMarkersSelected())
		{
			mMarkerSelection.clear();
			mMRUMarkerGUID = ASL::Guid();
			ASL::StationUtils::BroadcastMessage(MZ::kStation_PreludeProject, MarkerSelectionChanged());
		}
    }
    
    /*
    **	Add markers to the selection
    */
    void SRMarkerSelection::AddMarkersToSelection(const PL::MarkerSet& inMarkersToAdd)
    {
        bool selectionChanged(false);
		BOOST_FOREACH(MarkerSet::value_type const& marker, inMarkersToAdd)
		{
			if (!IsMarkerSelected(marker))
			{
				mMarkerSelection.insert(marker.GetGUID().AsString());
				mMRUMarkerGUID = marker.GetGUID();
				selectionChanged = true;
			}
		}
        
		if (selectionChanged)
		{
			ASL::StationUtils::PostMessageToUIThread(
                                                     MZ::kStation_PreludeProject, MarkerSelectionChanged(), true);
		}
    }

    /*
    **	Add a marker to the selection
    */
    void SRMarkerSelection::AddMarkerToSelection(
                                      const PL::CottonwoodMarker& inMarker)
    {
        PL::MarkerSet selectMap;
        selectMap.insert(inMarker);
        AddMarkersToSelection(selectMap);
    }
    
    /*
    **	Remove a marker from the selection
    */
    void SRMarkerSelection::RemoveMarkerFromSelection(
                                           const PL::CottonwoodMarker& inMarker)
    {
        MarkerSet selectMap;
        selectMap.insert(inMarker);
        RemoveMarkersFromSelection(selectMap);
    }
    
    /*
    **	Remove markers from the selection
    */
    void SRMarkerSelection::RemoveMarkersFromSelection(
                                            const PL::MarkerSet& inMarkersToRemove)
    {
        bool selectionChanged(false);
		BOOST_FOREACH(MarkerSet::value_type const& marker, inMarkersToRemove)
		{
			if (IsMarkerSelected(marker))
			{
				mMarkerSelection.erase(marker.GetGUID().AsString());
				mMRUMarkerGUID = ASL::Guid();
				selectionChanged = true;
			}
		}
        
        
		if (selectionChanged)
		{
			ASL::StationUtils::PostMessageToUIThread(
                                                     MZ::kStation_PreludeProject, MarkerSelectionChanged(), true);
		}

    }
    
    /*
    **	Is there a marker selection?
    */
    bool SRMarkerSelection::AreMarkersSelected()
    {
        return !mMarkerSelection.empty();
    }
    
    /*
    **	Is a marker selected?
    */
    bool SRMarkerSelection::IsMarkerSelected(
                                  const PL::CottonwoodMarker& inMarker)
    {
        return mMarkerSelection.find(inMarker.GetGUID().AsString()) != mMarkerSelection.end();
    }
    
    /*
    **	Get Marker Selection set
    */
    PL::MarkerSet SRMarkerSelection::GetMarkerSelection() const
    {
        PL::MarkerSet MarkerSet;
        if (mPrimaryClipPlayback)
        {
            const PL::SRMarkersList& srMarkerList = mPrimaryClipPlayback->GetMarkersList();
            
            BOOST_FOREACH (ASL::String markerGuid, mMarkerSelection)
            {
                SRMarkersList::const_iterator markerEnd = srMarkerList.end();
                SRMarkersList::const_iterator markerItr = srMarkerList.begin();
                for (; markerItr != markerEnd; ++markerItr)
                {
                    CottonwoodMarker marker;
                    if ( (*markerItr)->GetMarker(ASL::Guid(markerGuid), marker) )
                    {
                        MarkerSet.insert(marker);
                        break;
                    }
                }
            }
        }
        
        return MarkerSet;
    }
    
    /*
    ** Get the GUID for the most recently added marker
    */
    ASL::Guid SRMarkerSelection::GetMostRecentMarkerInSelection()
    {
        return mMRUMarkerGUID;
    }
}