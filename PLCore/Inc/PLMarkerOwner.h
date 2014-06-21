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

#ifndef PLMARKEROWNER_H
#define PLMARKEROWNER_H

#ifndef IPLMARKEROWNER_H
#include "IPLMarkerOwner.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

#ifndef ASLWEAKREFERENCECAPABLE_H
#include "ASLWeakReferenceCapable.h"
#endif

namespace PL
{
    ASL_DEFINE_CLASSREF(SRMarkerOwner, ISRMarkerOwner);
    ASL_DEFINE_IMPLID(SRMarkerOwner, 0x871C292C, 0xEA0A, 0x11E3, 0xA8, 0x90, 0x44, 0x27, 0x1D, 0x5D, 0x46, 0xB0);
    
    class SRMarkerOwner
    :
    public PL::ISRMarkerOwner,
    public ASL::IWeakReferenceCapable
    {
        ASL_STRONGWEAK_CLASS_SUPPORT(SRMarkerOwner);
        ASL_QUERY_MAP_BEGIN
        ASL_QUERY_ENTRY(ISRMarkerOwner)
        ASL_QUERY_ENTRY(ASL::IWeakReferenceCapable)
        ASL_QUERY_MAP_END
        
    public:
        /**
         **	Get the markers on this object.
         **
         **	@return					the markers.
         */
        virtual PL::ISRMarkersRef GetMarkers() const;
        
        void SetMarkers(PL::ISRMarkersRef inMarkers);
        
    private:
        
        PL::ISRMarkersRef  mMarkers;
    };
}

#endif