//
//  PLMarkerOwner.cpp
//  PLCore.mp
//
//  Created by labuser on 6/2/14.
//
//

#include "PLMarkerOwner.h"



namespace PL
{
    PL::ISRMarkersRef SRMarkerOwner::GetMarkers() const
    {
        return mMarkers;
    }
    
    
    void SRMarkerOwner::SetMarkers(PL::ISRMarkersRef inMarkers)
    {
        mMarkers = inMarkers;
    }
}
