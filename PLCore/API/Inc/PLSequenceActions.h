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

#ifndef PLSEQUENCEACTIONS_H
#define PLSEQUENCEACTIONS_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef ASLAUTOPTR_H
#include "ASLAutoPtr.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef DVAMEDIATYPES_TIMEDISPLAY_H
#include "dvamediatypes/TimeDisplay.h"
#endif

#ifndef BEEXECUTOR_H
#include "BEExecutor.h"
#endif

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BEMEDIATYPE_H
#include "BEMediaType.h"
#endif

#ifndef BEPROJECTFWD_H
#include "BEProjectFwd.h"
#endif

#ifndef BEPROPERTYKEY_H
#include "BEPropertyKey.h"
#endif

#ifndef BESEQUENCEFWD_H
#include "BESequenceFwd.h"
#endif

#ifndef BEPROJECTITEMFWD_H
#include "BEProjectItemFwd.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

#ifndef BECOMPONENTFWD_H
#include "BEComponentFwd.h"
#endif

#ifndef	BECOMPOUNDACTION_H
#include "BECompoundAction.h"
#endif

#ifndef BEFILTERPRESETFWD_H
#include "BEFilterPresetFwd.h"
#endif

#ifndef IFILTERMODULEFWD_H
#include "Filter/IFilterModuleFwd.h"
#endif

#ifndef BE_MEDIA_IVIDEOSTREAMFWD_H
#include "BE/Media/IVideoStreamFwd.h"
#endif

#ifndef MZACTION_H
#include "MZAction.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ITRANSITIONMODULEFWD_H
#include "Transition/ITransitionModuleFwd.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef PLEXPORTCLIPITEM_H
#include "PLExportClipItem.h"
#endif

#ifndef BE_SEQUENCE_ITRACKITEMGROUP_H
#include "BE/Sequence/ITrackItemGroup.h"
#endif

namespace PL
{

namespace SequenceActions
{

	/**
	**	Trim a group of track items in a sequence.
	**
	**	@param	inSequence		the sequence to modify.
	**	@param	inSelection		the selection group to trim.
	**	@param	inSideToTrim	specifies head or tail side.
	**	@param	inTimeOffset	the time offset to trim the group's edge to.
	**	@param	inRipple		if true, the trim should ripple-edit, else no shifting of others.
	**	@param	inRippleToFit	If true, shift only enough to fit the ripple operation (i.e. if rippling
	**							into blank space, no shift if needed).
	**							This is ignored if inRipple is false.
	**	@param	inForceRippleNextItem	If true, force the next clip to ripple after ripple trimming the tail of
	**									an unlinked clip.  If false, treat tail trimming of an unlinked clip just
	**									like trimming the head.
	**	@param	inRateStretch	if true, the trim should adjust the playback rate.
	**	@param	inAlignToVideo	if true, align the trim to a video frame
	**							boundary, otherwise align the trim to an
	**							audio frame boundary if permissible by the
	**							items in the selection being trimmed.
	**
	**	@see ISequenceEditor::CreateTrimItemsAction for detailed comments.
	*/
	PL_EXPORT
	void TrimItems(
		BE::ISequenceRef					inSequence,
		BE::ITrackItemSelectionRef			inSelection,
		BE::SequenceTrimType				inSideToTrim,
		dvamediatypes::TickTime				inTimeOffset,
		bool								inRipple,
		bool								inRippleToFit,
		bool								inForceRippleNextItem,
		bool								inRateStretch,
		bool								inAlignToVideo,
		bool								inIsUndoable = false);



	/**
	**	Construct a temporary "floating " selection group of track items from 
	**	the list of master clips.
	*/
	PL_EXPORT
	bool ConstructSelectionFromMasterClips(BE::MasterClipVector* inMasterClipVector,
		ASL::UInt32 inAudioFrameAlignment,
		bool inTakeVideo,
		bool inTakeAudio,
		BE::ITrackItemSelectionRef& outGroup,
		BE::ISequenceRef inSequence);

	/**
	**	Force to set masterclip to mono audio when it is added to time line, 
	**	no matter what kind of audio in original clip.
	**/
	PL_EXPORT
	void MakeMasterClipMono(BE::IMasterClipRef ioMasterClip);

	/**
	**
	*/
	PL_EXPORT
	bool AddClipItemsToSequence(
		BE::ISequenceRef inSequence, 
		SRClipItems& ioClipItems,
		dvamediatypes::TickTime inInsertTime,
		boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn,
		MZ::ActionUndoableFlags::Type inUndoableType,
		const ASL::String& inActionText,
		dvamediatypes::TickTime* outChangedDuration,
        bool isAudioForceMono);

	/*
	**
	*/
	PL_EXPORT
	bool AddClipItemsToSequenceForExport(
						BE::ISequenceRef inSequence, 
						ExportClipItems& ioExportItems, 
						dvamediatypes::TickTime inInsertTime,
						std::list<BE::ITrackItemSelectionRef>& outAddedTrackItemSelection,
						MZ::ActionUndoableFlags::Type inUndoableType = MZ::ActionUndoableFlags::kNotUndoable);
	/**
	**
	*/
	PL_EXPORT
	void AddTransitionItemsToSequence(
		BE::ISequenceRef inSequence,
		const PL::TrackTransitionMap& inTransitions,
		BE::MediaType inMediaType,
        MZ::ActionUndoableFlags::Type inUndoableType = MZ::ActionUndoableFlags::kNotUndoable);

	/**
	**
	*/
	PL_EXPORT
	void ConformSequenceToClipSettings(
		BE::ISequenceRef inSequence);

}

} // namespace PL

#endif // #ifndef PLSEQUENCEACTIONS_H
