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

#ifndef IPLMEDIA_H
#define IPLMEDIA_H

// BE
#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BECLIPFWD_H
#include "BEClipFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEM_H
#include "BE/Project/IProjectItem.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

#ifndef BEORPHANITEM_H
#include "BEOrphanItem.h"
#endif

#ifndef BE_MEDIA_IMEDIAFWD_H
#include "BE/Media/IMediaFwd.h"
#endif

// MZ
#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

namespace PL
{
    /**
    **	InterfaceRef & ID for IMarkerOwner.
    */
    ASL_DEFINE_IREF(ISRMedia);
    ASL_DEFINE_IID(ISRMedia, 0x4692398d, 0x052d, 0x4511, 0x83, 0x6f, 0x5e, 0x5c, 0x96, 0x45, 0xc1, 0x5e)
    typedef std::set<ISRMediaRef>  SRMediaSet;
    ASL_DECLARE_MESSAGE_WITH_1_PARAM(SRMediaChangedMessage, ISRMediaRef);
    
    /**
     **	Interface to an owner of markers.
     */
    ASL_INTERFACE ISRMedia : public ASLUnknown
    {
        /*
        **
        */
        virtual BE::IMasterClipRef GetMasterClip() const = 0;
        
        /*
        **
        */
        virtual ASL::String GetClipFilePath() const = 0;
        
        /*
        **
        */
        virtual bool IsDirty() const = 0;
        
        /*
        **	Called if PL found save failure or read-only changes after resume.
        */
        virtual void UpdateWritableState(bool inIsWritable) = 0;
        
        /*
        **
        */
        virtual bool IsWritable() const = 0;
        
        
        /*
        **
        */
        virtual bool IsOffLine() const = 0;
    
        /*
        **
        */
        virtual void SetDirty(bool inIsDirty) = 0;
        
        /*
        **
        */
        virtual void RefreshOfflineMedia() = 0;
        
        /*
        **
        */
        virtual ASL::StationID GetStationID() const = 0;

    };

}


#endif