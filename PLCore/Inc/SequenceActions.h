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

#ifndef SEQUENCEACTIONS_H
#define SEQUENCEACTIONS_H

#ifndef BECOMPOUNDACTION_H
#include "BECompoundAction.h"
#endif

#ifndef BESEQUENCEFWD_H
#include "BESequenceFwd.h"
#endif

#ifndef BEFILTERPRESETFWD_H
#include "BEFilterPresetFwd.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef MZACTION_H
#include "MZAction.h"
#endif


#ifndef BE_SEQUENCE_TRACKITEMSELECTIONFACTORY_H
#include "BE/Sequence/TrackItemSelectionFactory.h"
#endif

namespace PL
{

namespace SequenceActions
{


/*
**
*/
void CreateAddMotionVideoIntrinsicAction(
	ASL::UInt32				inInsertIndex,
	BE::ISequenceRef		inSequence,
	BE::ITrackItemRef		inTrackItem,
	BE::ICompoundActionRef  inOutCompoundAction);

/*
**
*/
void CreateAddOpacityVideoIntrinsicAction(
	ASL::UInt32				inInsertIndex,
	BE::ISequenceRef		inSequence,
	BE::ITrackItemRef		inTrackItem,
	BE::ICompoundActionRef  inOutCompoundAction);

/**
**
*/
BE::IActionRef CreateAddAudioIntrinsicsAction(BE::ITrackItemRef inTrackItem);

/**
**
*/
BE::IActionRef CreateAddVideoIntrinsicsAction(
    BE::ISequenceRef inSequence,
	BE::ITrackItemRef inTrackItem);



} // namespace SequenceActions

} // namespace PL

#endif
