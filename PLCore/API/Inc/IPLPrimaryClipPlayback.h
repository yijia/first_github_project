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

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#define IPLPRIMARYCLIPPLAYBACK_H

// ASL
#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef ASLCOLOR_H
#include "ASLColor.h"
#endif

// DVA
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

// MZ
#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

// BE
#ifndef BE_CLIP_IMASTERCLIPFWD_H
#include "BE/Clip/IMasterClipFwd.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

#ifndef BE_SEQUENCE_ISEQUENCEFWD_H
#include "BE/Sequence/ISequenceFwd.h"
#endif

namespace PL
{

ASL_DEFINE_IREF(ISRPrimaryClipPlayback);
ASL_DEFINE_IID(ISRPrimaryClipPlayback, 0x1b991e43, 0xec4f, 0x4c22, 0x97, 0x0, 0x98, 0x84, 0xc6, 0xeb, 0xa0, 0x10);

ASL_INTERFACE ISRPrimaryClipPlayback : public ASLUnknown
{
public:
	/*
	**
	*/
	virtual BE::ISequenceRef GetBESequence() = 0;
	
	/*
	**
	*/
	virtual PL::ISRMarkersRef GetMarkers(BE::IMasterClipRef inMasterClip) const = 0;
	
	/*
	**
	*/
	virtual PL::SRMarkersList GetMarkersList() const = 0;

	/*
	**
	*/
	virtual ASL::Color GetTrackItemLabelColor() const= 0;

	/*
	**
	*/
	virtual bool IsSupportMarkerEditing() const = 0;

	/*
	**
	*/
	virtual bool IsSupportTimeLineEditing() const = 0;
	
	/*
	**
	*/
	virtual BE::IMasterClipRef GetSequenceMasterClip() const = 0;

	/*
	**
	*/
	virtual ASL::String GetSubClipName(const BE::ITrackItemRef inTrackItem) const = 0;

	/*
	**
	*/
	virtual ASL::Guid GetSubClipGUID(const BE::ITrackItemRef inTrackItem) const = 0;

	/*
	**
	*/
	virtual void SetEditTime(const dvamediatypes::TickTime& inEditTime) = 0;

	/*
	**
	*/
	virtual dvamediatypes::TickTime GetEditTime() const = 0;


};

} // namespace PL

#endif
