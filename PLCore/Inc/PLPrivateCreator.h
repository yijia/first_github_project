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

#ifndef PLPRIVATECREATOR_H
#define PLPRIVATECREATOR_H


#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef PLEXPORTCLIPITEM_H
#include "PLExportClipItem.h"
#endif

#ifndef ASLMACROS_H
#include "ASLMacros.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEMFWD_H
#include "BE/Project/IProjectItemFwd.h"
#endif

#ifndef MZACTION_H
#include "MZAction.h"
#endif

#ifndef MZSEQUENCE_H
#include "MZSequence.h"
#endif

namespace PL
{

	class SilkRoadPrivateCreator
	{
		ASL_NO_DEFAULT(SilkRoadPrivateCreator);
		ASL_NO_ALLOCATION(SilkRoadPrivateCreator);
	public:

		/*
		**
		*/
		static BE::IProjectItemRef BuildSequence(
			SRClipItems& ioClipItems, 
			const PL::TrackTransitionMap& inVideoTransitions = PL::TrackTransitionMap(),
			const PL::TrackTransitionMap& inAudioTransitions = PL::TrackTransitionMap(),
			BE::IMasterClipRef inSequenceTempalte = BE::IMasterClipRef(), 
			MZ::SequenceAudioTrackRule inAudioTrackRule = MZ::kSequenceAudioTrackRule_Logging);

		/*
		**
		*/
		static  bool AddClipItemsToSequence(
							BE::IProjectItemRef& inSequenceItem, 
							SRClipItems& ioClipItems,
							dvamediatypes::TickTime inInsertTime = dvamediatypes::kTime_Zero,
							boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn = boost::function<void(SRClipItemPtr)>(),
							MZ::ActionUndoableFlags::Type inUndoableType = MZ::ActionUndoableFlags::kUndoable,
							const ASL::String& inActionText = ASL::String(),
							dvamediatypes::TickTime* outChangedDuration = NULL,
                            bool isAudioForceMono = true);
		
		/*
		**
		*/
		static bool AddClipItemsToSequenceForExport(
							BE::ISequenceRef inSequence, 
							ExportClipItems& ioExportItems, 
							dvamediatypes::TickTime inInsertTime,
							MZ::ActionUndoableFlags::Type inUndoableType = MZ::ActionUndoableFlags::kNotUndoable);

		/*
		**
		*/
		typedef std::pair<PL::SRClipItemPtr, PL::SRClipItemPtr> RelinkClipItemPair;
		typedef std::vector<RelinkClipItemPair> RelinkClipItemPairVec;
		static void RelinkClipItems(
							BE::IProjectItemRef& inSequenceItem, 
							RelinkClipItemPairVec & ioRelinkClipItemsPairVec,
							boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn);


		/*
		**
		*/
		static  void AttachSequenceToTimeLineEditor(BE::IProjectItemRef sequenceItem);

		/*
		**
		*/
		static  void DetachSequenceFromTimeLineEditor(BE::IProjectItemRef sequenceItem);

		/*
		**
		*/
		static void RemoveSequenceFromProject(BE::IProjectItemRef sequenceItem);

		/*
		**
		*/
		static void Refresh(BE::IProjectItemRef sequenceItem);

		/*
		**
		*/
		static void ResolveSequence(BE::IProjectItemRef& ioSequenceItem, BE::IMasterClipRef inSequenceTemplate, MZ::SequenceAudioTrackRule inAudioTrackRule = MZ::kSequenceAudioTrackRule_Logging);

	private:

		/*
		**
		*/
		static bool CheckIfSequenceInProject(BE::IProjectItemRef sequenceItem);

		/*
		**
		*/
		static BE::IProjectItemRef BuildSequenceFromMasterClip(BE::IMasterClipRef inMasterClip, MZ::SequenceAudioTrackRule inAudioTrackRule);

	};
}

#endif
