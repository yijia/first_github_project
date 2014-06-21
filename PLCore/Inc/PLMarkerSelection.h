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
#pragma once

#ifndef PLMARKERSELECTION_H
#define PLMARKERSELECTION_H


#ifndef IPLMARKERSELECTION_H
#include "IPLMarkerSelection.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef PLUTILITIES_h
#include "PLUtilities.h"
#endif 

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef ASLSTRINGVECTOR_H
#include "ASLStringVector.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif
    
namespace PL
{
    ASL_DEFINE_CLASSREF(SRMarkerSelection, ISRMarkerSelection);
    ASL_DEFINE_IMPLID(SRMarkerSelection, 0xAD239D9B, 0xd4c4, 0x49af, 0x8e, 0x1b, 0x87, 0xab, 0x5d, 0x39, 0x50, 0xed);
        
    class SRMarkerSelection : public ISRMarkerSelection
    {
        ASL_QUERY_MAP_BEGIN
        ASL_QUERY_ENTRY(ISRMarkerSelection)
        ASL_QUERY_ENTRY(SRMarkerSelection)
        ASL_QUERY_MAP_END
        
    public:
        SRMarkerSelection();
        
        virtual ~SRMarkerSelection();
        
        
        /*
        **
        */
        void SetPrimaryClip(PL::ISRPrimaryClipPlaybackRef  inPrimaryClipPlayback);
        
        /*
        **
        */
        virtual void ClearSelection();
        
        /*
        **	Add markers to the selection
        */
        virtual void AddMarkersToSelection(
                                           const MarkerSet& inMarkersToAdd);
        
        
        /*
        **	Add a marker to the selection
        */
        virtual void AddMarkerToSelection(
                                          const CottonwoodMarker& inMarker);
        /*
        **	Remove a marker from the selection
        */
        virtual void RemoveMarkerFromSelection(
                                               const CottonwoodMarker& inMarker);
        
        /*
        **	Remove markers from the selection
        */
        virtual void RemoveMarkersFromSelection(
                                                const MarkerSet& inMarkersToRemove);
        
        /*
        **	Is there a marker selection?
        */
        virtual bool AreMarkersSelected();
        
        /*
        **	Is a marker selected?
        */
        virtual bool IsMarkerSelected(
                                      const CottonwoodMarker& inMarker);
        
        /*
        **	Get Marker Selection set
        */
        virtual MarkerSet GetMarkerSelection() const;
        
        /*
        ** Get the GUID for the most recently added marker
        */
        virtual ASL::Guid GetMostRecentMarkerInSelection();
        
        
    private:
        
        ASLStringSet            mMarkerSelection;
        ASL::Guid				mMRUMarkerGUID;
        PL::ISRPrimaryClipPlaybackRef  mPrimaryClipPlayback;
    };

}


#endif