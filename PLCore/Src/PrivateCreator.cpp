/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
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

#include "Prefix.h"

//	Local
#include "PLPrivateCreator.h"

//	SR
#include "PLProject.h"
#include "PLUtilitiesPrivate.h"
#include "PLFactory.h"
#include "IPLMedia.h"
#include "PLSequenceActions.h"

//	MZ
#include "MZProject.h"
#include "MZAction.h"
#include "MZSequenceActions.h"
#include "MZSequence.h"
#include "MZBEUtilities.h"
#include "MZClipItemActions.h"
#include "MZApplicationAudio.h"
#include "PLUtilities.h"
#include "PLBEUtilities.h"

//	BE
#include "BE/Project/IProject.h"
#include "BE/Project/IClipProjectItem.h"
#include "BE/Project/IProjectItemSelection.h"
#include "BE/Sequence/TrackItemSelectionFactory.h"
#include "BE/Sequence/IClipTrackItem.h"
#include "BE/Sequence/IClipTrack.h"
#include "BE/Clip/IClip.h"
#include "BEExecutorFwd.h"
#include "BEProjectItemContainerActions.h"

//	ASL
#include "ASLAsyncCallFromMainThread.h"
#include "ASLClassFactory.h"

//	DVACORE
#include "dvacore/config/Localizer.h"


namespace PL
{
	/*
	**
	*/
	void SilkRoadPrivateCreator::Refresh(BE::IProjectItemRef sequenceItem)
	{
		BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(sequenceItem);
		DVA_ASSERT(sequence);
		MZ::ApplicationAudio::ConformSequenceToAudioSettings(sequence);
		SRUtilitiesPrivate::OpenSequenceAndWrapOpenProjectItem(
			sequence,
			MZ::GetProject(),
			sequenceItem,
			ASL::Guid(),
			false);
	}

	/*
	**
	*/
	bool SilkRoadPrivateCreator::CheckIfSequenceInProject(BE::IProjectItemRef sequenceItem)
	{
		//	Judge if the sequence is already exist in the project
		BE::IProjectRef project = MZ::GetProject();
		BE::IProjectItemSelectionRef selection(ASL::CreateClassInstanceRef(BE::kProjectItemSelectionClassID));
		selection->AddItem(sequenceItem);

		ASL::UInt32  whyNot;
		bool needAddToProject = project->QueryAddItems(
			selection, 
			BE::IProjectItemContainerRef(project ->GetRootProjectItem()),
			whyNot);

		return !needAddToProject;
	}

	/*
	**
	*/
	void SilkRoadPrivateCreator::AttachSequenceToTimeLineEditor(BE::IProjectItemRef sequenceItem)
	{
		bool existInProject = CheckIfSequenceInProject(sequenceItem);

		if (!existInProject)
		{
			BE::IProjectRef project = MZ::GetProject();
			BE::IProjectItemSelectionRef selection(ASL::CreateClassInstanceRef(BE::kProjectItemSelectionClassID));
			selection->AddItem(sequenceItem);

			//	If not, we Add the sequence to the project
			BE::IActionRef addAction(project->CreateAddItemsAction(
				selection, 
				BE::IProjectItemContainerRef(project ->GetRootProjectItem())));
			ASL_ASSERT(addAction != NULL);

			MZ::ExecuteActionWithoutUndo(
				BE::IExecutorRef(project), 
				addAction,
				true);
		}

		Refresh(sequenceItem);
	}

	/*
	**
	*/
	void SilkRoadPrivateCreator::DetachSequenceFromTimeLineEditor(BE::IProjectItemRef sequenceItem)
	{
		bool existInProject = CheckIfSequenceInProject(sequenceItem);

		if (existInProject)
		{
			RemoveSequenceFromProject(sequenceItem);
		}
	}

	/*
	**
	*/
	void SilkRoadPrivateCreator::RemoveSequenceFromProject(BE::IProjectItemRef sequenceItem)
	{
		DVA_ASSERT(sequenceItem);
		BE::IProjectRef project = MZ::GetProject();

		// Remove this sequence from project
		BE::IProjectItemSelectionRef itemSelection(ASL::CreateClassInstanceRef(BE::kProjectItemSelectionClassID));
		itemSelection->AddItem(sequenceItem);
		BE::IActionRef action = project->CreateDeleteItemsAction(itemSelection, false);
		MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(project), action, true );
	}

	/*
	**
	*/
	BE::IProjectItemRef SilkRoadPrivateCreator::BuildSequence(
		SRClipItems& ioClipItems, 
		const PL::TrackTransitionMap& inVideoTransitions /*= PL::TrackTransitionMap()*/,
		const PL::TrackTransitionMap& inAudioTransitions /*= PL::TrackTransitionMap()*/,
		BE::IMasterClipRef inSequenceTempalte /*= BE::IMasterClipRef()*/, 
		MZ::SequenceAudioTrackRule inAudioTrackRule /*= MZ::kSequenceAudioTrackRule_Logging*/)
	{
		BE::IProjectItemRef sequenceItem;
		if (ioClipItems.empty())
		{
			sequenceItem = SRFactory::CreateEmptySequence(MZ::GetProject());
		}
		else
		{
			BE::IMasterClipRef sequenceTemplate = inSequenceTempalte;
			if ( sequenceTemplate == NULL)
			{
				sequenceTemplate = ioClipItems.front()->GetSRMedia()->GetMasterClip();
			}
			sequenceItem = BuildSequenceFromMasterClip(sequenceTemplate, inAudioTrackRule);
			
			AddClipItemsToSequence(
				sequenceItem,
				ioClipItems,
				dvamediatypes::kTime_Zero,
				boost::function<void(SRClipItemPtr)>(),
				MZ::ActionUndoableFlags::kNotUndoable,
                ASL::String(),
                NULL,
                inAudioTrackRule == MZ::kSequenceAudioTrackRule_Logging);

			BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(sequenceItem);
			PL::SequenceActions::AddTransitionItemsToSequence(
				sequence,
				inVideoTransitions,
				BE::kMediaType_Video);
			PL::SequenceActions::AddTransitionItemsToSequence(
				sequence,
				inAudioTransitions,
				BE::kMediaType_Audio);
		}

		if (UIF::IsEAMode())
		{
			PL::BEUtilities::MarkSequenceTemporal(sequenceItem);
		}

		return sequenceItem;
	}

	/*
	**
	*/
	void SilkRoadPrivateCreator::ResolveSequence(BE::IProjectItemRef& ioSequenceItem, BE::IMasterClipRef inSequenceTemplate, MZ::SequenceAudioTrackRule inAudioTrackRule)
	{
		BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(ioSequenceItem);

		if ( sequence->GetEnd() == dvamediatypes::kTime_Zero )
		{
			if (CheckIfSequenceInProject(ioSequenceItem))
			{
				RemoveSequenceFromProject(ioSequenceItem);
			}
			ioSequenceItem = BuildSequenceFromMasterClip(inSequenceTemplate, inAudioTrackRule);

			if (UIF::IsEAMode())
			{
				PL::BEUtilities::MarkSequenceTemporal(ioSequenceItem);
			}
		}
	}

	/*
	**
	*/
	BE::IProjectItemRef SilkRoadPrivateCreator::BuildSequenceFromMasterClip(BE::IMasterClipRef inMasterClip, MZ::SequenceAudioTrackRule inAudioTrackRule)
	{
		BE::IProjectRef project = MZ::GetProject();
		
		return Utilities::CreateSequenceItemFromMasterClip(
										inMasterClip, 
										project, 
										project->GetRootProjectItem(),
										inAudioTrackRule);
	}

	/*
	**
	*/
	bool SilkRoadPrivateCreator::AddClipItemsToSequence(
											BE::IProjectItemRef& inSequenceItem, 
											SRClipItems& ioClipItems,
											dvamediatypes::TickTime inInsertTime,
											boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn,
											MZ::ActionUndoableFlags::Type inUndoableType,
											const ASL::String& inActionText,
											dvamediatypes::TickTime* outChangedDuration,
                                            bool isAudioForceMono)
	{	
		BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(inSequenceItem);
		dvamediatypes::TickTime insertTime = inInsertTime;

		// If the time is invalid , we append the clip by default
		if (insertTime == dvamediatypes::kTime_Invalid)
		{
			insertTime = MZ::SequenceActions::GetEnd(sequence);
		}

		return PL::SequenceActions::AddClipItemsToSequence(
													sequence,
													ioClipItems,
													insertTime,
													inAddToRCClipItemsFxn,
													inUndoableType,
													inActionText,
													outChangedDuration,
                                                    isAudioForceMono);
	}
	
	/*
	**
	*/
	void SilkRoadPrivateCreator::RelinkClipItems(
									BE::IProjectItemRef& inSequenceItem, 
									RelinkClipItemPairVec & ioRelinkClipItemsPairVec,
									boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn)
	{
		BE::ISequenceRef sequence = MZ::BEUtilities::GetSequenceFromProjectitem(inSequenceItem);
		BE::IExecutorRef executor(sequence);

		BOOST_FOREACH(RelinkClipItemPair& relinkClipPair, ioRelinkClipItemsPairVec)
		{
			PL::SRClipItemPtr clipItem = relinkClipPair.first;
			BE::ITrackItemSelectionRef trackItemSelection(
				BE::TrackItemSelectionFactory::CreateSelection());

			BE::ITrackItemRef trackItem = clipItem->GetTrackItem();

			// Get insert point for this relink trackItem
			dvamediatypes::TickTime insertTime = trackItem->GetStart();
			BE::IClipTrackItemRef clipTrackItem(trackItem);
			BE::ITrackItemGroupRef aLink;
			ASL_REQUIRE(clipTrackItem != NULL);
			if(clipTrackItem->IsLinked())
			{
				aLink = sequence->FindLink(trackItem);
				trackItemSelection->AddLink(aLink);
			}
			else
			{
				trackItemSelection->AddItem(trackItem);
			}
			MZ::SequenceActions::RemoveItems(sequence, trackItemSelection, true, true);

			SRClipItems clipItems;
			clipItems.push_back(relinkClipPair.second);
			dvamediatypes::TickTime outChangedDuration;
			PL::SequenceActions::AddClipItemsToSequence(
				MZ::BEUtilities::GetSequenceFromProjectitem(inSequenceItem), 
				clipItems,
				insertTime,
				inAddToRCClipItemsFxn,
				MZ::ActionUndoableFlags::kUndoable,
				dvacore::ZString("$$$/Prelude/PLCore/RelinkClipItems=Add clip(s) to Rough Cut"),
				&outChangedDuration,
                false);
		}
	}

	/*
	**
	*/
	bool SilkRoadPrivateCreator::AddClipItemsToSequenceForExport(
									BE::ISequenceRef inSequence, 
									ExportClipItems& ioExportItems, 
									dvamediatypes::TickTime inInsertTime,
									MZ::ActionUndoableFlags::Type inUndoableType)
	{
		std::list<BE::ITrackItemSelectionRef> addedTrackItemSelection;
		return PL::SequenceActions::AddClipItemsToSequenceForExport(inSequence, ioExportItems, inInsertTime, addedTrackItemSelection, inUndoableType);
	}
}
