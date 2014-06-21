/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2013 Adobe Systems Incorporated                       */
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

#ifndef PLBEUTILITIES_H
#define PLBEUTILITIES_H

// BE
#ifndef BEMASTERCLIP_H
#include "BEMasterClip.h"
#endif

// DVA
#ifndef DVAMEDIATYPES_TIMEDISPLAY_H
#include "dvamediatypes/TimeDisplay.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEMFWD_H
#include "BE/Project/IProjectItemFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTFWD_H
#include "BE/Project/IProjectFwd.h"
#endif

#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

#ifndef BE_SEQUENCE_ISEQUENCEFWD_H
#include "BE/Sequence/ISequenceFwd.h"
#endif

#ifndef BE_SEQUENCE_ITRACKITEMSELECTIONFWD_H
#include "BE/Sequence/ITrackItemSelectionFwd.h"
#endif

#ifndef BE_SEQUENCE_ICLIPTRACKITEM_H
#include "BE/Sequence/IClipTrackItem.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

namespace PL
{
	namespace BEUtilities
	{
		extern const dvacore::utility::ImmutableString kTemporalPreludeSequence;
		typedef std::map<dvamediatypes::TickTime, BE::IClipTrackItemRef> StartTimeTrackItemMap;

		/*
		**
		*/
		PL_EXPORT
		bool IsTemporalSequence(BE::IProjectItemRef inProjectItem);

		/*
		**
		*/
		PL_EXPORT
		void MarkSequenceTemporal(BE::IProjectItemRef inProjectItem);


		/*
		** 
		*/
		PL_EXPORT
		BE::IProjectItemRef CreateEmptySequence(
				BE::IProjectRef const& inProject,
				BE::IProjectItemRef const& inContainingBin = BE::IProjectItemRef(),
				ASL::String const& inSequenceName = ASL_STR("Dummy Sequence"));

		/*
		**
		*/
		PL_EXPORT
		bool IsMasterClipUsedInSRCore(BE::IMasterClipRef const& inMasterClip);

		/*
		**
		*/
		PL_EXPORT
		void RemoveMasterClipFromCurrentProject(BE::IMasterClipRef const& inMasterClip);

		/*
		**
		*/
		PL_EXPORT
		void RemoveProjectItem(BE::IProjectItemRef const& inProjectItem);

		// Moved from MZBEUtilities.h
		/*
		**
		*/
		PL_EXPORT
		BE::IMasterClipRef ImportFile(
							const ASL::String& inClipPath, 
							const BE::IProjectRef& inProject,
							bool inNeedErrorMsg,
							bool inIgnoreAudio,
							bool inIgnoreMetadata);

		/*
		**
		*/
		PL_EXPORT
		BE::IMasterClipRef ImportFile(
						const ASL::String& inClipPath, 
						const BE::IProjectRef& inProject,
						MZ::ImportResultVector& outFailureVector,
						bool inIgnoreAudio,
						bool inIgnoreMetadata);

		/*
		**
		*/
		PL_EXPORT
		BE::IMasterClipRef GetTheFirstMasterClipFromSequence(
			const BE::IMasterClipRef& inSequenceMasterClip,
			BE::MediaType inMediaType);

		/*
		**
		*/
		PL_EXPORT
		bool HasTrackItem(
			const BE::ITrackItemGroupRef& inTrackItemSelection,
			BE::MediaType inMediaType);

		/*
		** Get a specified media type track item from the link group of inTrackItem
		*/
		PL_EXPORT
		BE::ITrackItemRef GetLinkedFirstTrackItem(
			const BE::ISequenceRef& inSequence,
			const BE::ITrackItemRef& inTrackItem,
			BE::MediaType inMediaType);
	}
}

#endif