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

#ifndef IPLMARKEROWNER_H
#define IPLMARKEROWNER_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

#ifndef IPLMARKEROWNERFWD_H
#include "IPLMarkerOwnerFwd.h"
#endif

#ifndef ASLREFERENCEBROKER_H
#include "ASLReferenceBroker.h"
#endif

namespace PL
{
    /**
     **	Interface to an owner of markers.
     */
    ASL_INTERFACE ISRMarkerOwner : public ASLUnknown
    {
        /**
         **	Get the markers on this object.
         **
         **	@return					the markers.
         */
        virtual PL::ISRMarkersRef GetMarkers() const = 0;
    };
    
} // namespace PL

#endif