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

#include "PLMetadataActions.h"

#include "PLModulePicker.h"

#include "MZActionID.h"

// UIF
#include "UIFDocumentManager.h"

// BE
#include "BEAction.h"
#include "BEActionImpl.h"
#include "BETransaction.h"
#include "BEUndoable.h"
#include "BEMasterClip.h"
#include "BE/Media/IMediaMetaData.h"
#include "BEMedia.h"
#include "BECompoundAction.h"
#include "BE/Project/ProjectItemUtils.h"

// dvacore
#include "dvacore/config/Localizer.h"
#include "dvatemporalxmp/XMPTemporalMetadataServices.h"

// dvametadata
#include "dvametadata/MetaDataOperations.h"
#include "dvametadata/MetaDataProvider.h"
#include "dvametadata/MetadataProviders.h"

// MZ & PL
#include "PLMediaMetaDataProvider.h"
#include "IPLMarkerOwner.h"
#include "MZProject.h"
#include "MZBEUtilities.h"
#include "MZAction.h"

// ASL
#include "ASLClassFactory.h"

namespace PL
{

const char* kRemoveTemporalMarkersActionName = "$$$/Prelude/PL/SetMetadataAction/RemoveTemporalMarker=Remove Temporal Marker(s)";
const ASL::String kRemoveTemporalMarkersActionID = ASL_STR("PL.MetadataActions.RemoveCottonwoodMarkers");

class AddMarkersAction
{
	/*
	**
	*/
	class AddMarkers :
		public BE::IUndoable
	{
		typedef ASL::InterfaceRef<AddMarkers, BE::IUndoable> AddMarkersRef;
		ASL_BASIC_CLASS_SUPPORT(AddMarkers);
		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(IUndoable)
		ASL_QUERY_MAP_END

	public:
		/*
		**
		*/
		static BE::IUndoableRef Create(
            PL::ISRMarkersRef inMarkers,
			const CottonwoodMarkerList& inMarkerList,
			bool inIsSilent)
		{
			ASL_ASSERT(inMarkers != NULL);
			AddMarkersRef addMarkers(CreateClassRef());
			addMarkers->mMarkers = inMarkers;
			addMarkers->mMarkerList = inMarkerList;
			addMarkers->mIsSilent = inIsSilent;
			return BE::IUndoableRef(addMarkers);
		}

		/*
		**
		*/
		AddMarkers()
		{
		}

		/*
		**
		*/
		virtual ~AddMarkers()
		{
		}

		/*
		**
		*/
		virtual void Do()
		{
			mMarkers->AddMarkers(mMarkerList, mIsSilent);
		}

		/*
		**
		*/
		virtual void Undo()
		{
			mMarkers->RemoveMarkers(mMarkerList);
		}

		/*
		**
		*/
		virtual bool IsSafeForPlayback() const
		{
			return false;
		}

	private:
        PL::ISRMarkersRef       mMarkers;
		CottonwoodMarkerList	mMarkerList;
		bool					mIsSilent;
	};

public:
	/*
	**
	*/
	AddMarkersAction()
	{
	}

	/*
	**
	*/
	AddMarkersAction(
		BE::IMasterClipRef inMasterClip,
		const CottonwoodMarkerList& inMarkerList,
		bool inIsSilent) 
		:
		mMarkerList(inMarkerList)
	{
		ISRPrimaryClipPlaybackRef primaryClipPlayback = 
			ModulePicker::GetInstance()->GetLoggingClip();
		ASL_ASSERT(primaryClipPlayback != NULL);
		mMarkers = primaryClipPlayback->GetMarkers(inMasterClip);

		mIsSilent = inIsSilent;
	}

	/*
	**
	*/
	void Commit(
		BE::ITransactionRef inTransaction)
	{
		inTransaction->Append(AddMarkers::Create(mMarkers, mMarkerList, mIsSilent));
	}

private:
    PL::ISRMarkersRef       mMarkers;
	CottonwoodMarkerList	mMarkerList;
	bool					mIsSilent;
};

/*
**
*/
class UpdateMarkers :
	public BE::IUndoable
{
	typedef ASL::InterfaceRef<UpdateMarkers, BE::IUndoable> UpdateMarkersRef;
	ASL_BASIC_CLASS_SUPPORT(UpdateMarkers);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IUndoable)
		ASL_QUERY_MAP_END

public:
	/*
	**
	*/
	static BE::IUndoableRef Create(
		PL::ISRMarkersRef inMarkers,
		const CottonwoodMarkerList& inNewMarkerList,
		const CottonwoodMarkerList& inOldMarkerList)
	{
		ASL_ASSERT(inMarkers != NULL);
		UpdateMarkersRef updateMarkers(CreateClassRef());
		updateMarkers->mMarkers = inMarkers;
		updateMarkers->mNewMarkerList = inNewMarkerList;
		updateMarkers->mOldMarkerList = inOldMarkerList;
		return BE::IUndoableRef(updateMarkers);
	}

	/*
	**
	*/
	UpdateMarkers()
	{
	}

	/*
	**
	*/
	virtual ~UpdateMarkers()
	{
	}

	/*
	**
	*/
	virtual void Do()
	{
		mMarkers->UpdateMarkers(mNewMarkerList);
	}

	/*
	**
	*/
	virtual void Undo()
	{
		mMarkers->UpdateMarkers(mOldMarkerList);
	}

	/*
	**
	*/
	virtual bool IsSafeForPlayback() const
	{
		return false;
	}

private:
	PL::ISRMarkersRef mMarkers;
	CottonwoodMarkerList mNewMarkerList;
	CottonwoodMarkerList mOldMarkerList;
};

class AssociateMarkersAction
{
public:
	/*
	**
	*/
	AssociateMarkersAction()
	{
	}

	/*
	**
	*/
	AssociateMarkersAction(
		const BE::IMasterClipRef& inMasterClip,
		const CottonwoodMarkerList& inMarkerList,
		bool inIsSilent)
		:
		mMasterClip(inMasterClip),
		mMarkerList(inMarkerList)
	{
		ISRPrimaryClipPlaybackRef primaryClipPlayback = ModulePicker::GetInstance()->GetLoggingClip();
		ASL_ASSERT(primaryClipPlayback != NULL);
		mMarkers = primaryClipPlayback->GetMarkers(inMasterClip);

		mIsSilent = inIsSilent;
	}

	/*
	**
	*/
	void Commit(BE::ITransactionRef inTransaction)
	{
		CottonwoodMarkerList toAddMarkers, newUpdateMarkers, oldUpdateMarkers;
		BOOST_FOREACH(const CottonwoodMarker& newMarker, mMarkerList)
		{
			CottonwoodMarker oldMarker;
			if (mMarkers->GetMarker(newMarker.GetGUID(), oldMarker))
			{
				newUpdateMarkers.push_back(newMarker);
				oldUpdateMarkers.push_back(oldMarker);
			}
			else
			{
				toAddMarkers.push_back(newMarker);
			}
		}

		if (!toAddMarkers.empty())
		{
			inTransaction->Append(BE::CreateAction(AddMarkersAction(mMasterClip, toAddMarkers, mIsSilent)));
		}

		if (!newUpdateMarkers.empty() && newUpdateMarkers.size() == oldUpdateMarkers.size())
		{
			inTransaction->Append(UpdateMarkers::Create(mMarkers, newUpdateMarkers, oldUpdateMarkers));
		}
	}

private:
	BE::IMasterClipRef		mMasterClip;
	PL::ISRMarkersRef	mMarkers;
	CottonwoodMarkerList	mMarkerList;
	bool					mIsSilent;
};

class RemoveCottonwoodMarkersAction
{
	/*
	**
	*/
	class RemoveCottonwoodMarkers :
		public BE::IUndoable
	{
		typedef ASL::InterfaceRef<RemoveCottonwoodMarkers, BE::IUndoable> RemoveCottonwoodMarkersRef;
		ASL_BASIC_CLASS_SUPPORT(RemoveCottonwoodMarkers);
		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(IUndoable)
		ASL_QUERY_MAP_END

	public:
		/*
		**
		*/
		static BE::IUndoableRef Create(
			const MarkerSet &inMarkerSelection)
		{
			RemoveCottonwoodMarkersRef removeMarkers(CreateClassRef());
			removeMarkers->InitWithMarkerSelection(inMarkerSelection);

			return BE::IUndoableRef(removeMarkers);
		}

		/*
		**
		*/
		RemoveCottonwoodMarkers()
		{
		}

		/*
		**
		*/
		virtual ~RemoveCottonwoodMarkers()
		{
		}

		/*
		**
		*/
		virtual void Do()
		{
			for (std::map<PL::ISRMarkersRef, CottonwoodMarkerList>::iterator mapItr = mMarkerMap.begin(); mapItr != mMarkerMap.end(); ++mapItr)
			{
				PL::ISRMarkersRef markers = mapItr->first;
				markers->RemoveMarkers(mapItr->second);
			}
		}

		/*
		**
		*/
		virtual void Undo()
		{
			for (std::map<PL::ISRMarkersRef, CottonwoodMarkerList>::iterator mapItr = mMarkerMap.begin(); mapItr != mMarkerMap.end(); ++mapItr)
			{
				PL::ISRMarkersRef markers = mapItr->first;
				markers->AddMarkers(mapItr->second, true);
			}
		}

		/*
		**
		*/
		virtual bool IsSafeForPlayback() const
		{
			return false;
		}

		void InitWithMarkerSelection(const MarkerSet& inMarkerSelection)
		{
			mMarkerSelection = inMarkerSelection;
			if (mMarkerMap.size() == 0)
			{
				ISRPrimaryClipPlaybackRef primaryClipPlayback = 
					PL::ModulePicker::GetInstance()->GetLoggingClip();
				ASL_ASSERT(primaryClipPlayback != NULL);
				for (MarkerSet::const_iterator selectionIter = mMarkerSelection.begin(); selectionIter != mMarkerSelection.end(); ++selectionIter)
				{
					CottonwoodMarker marker;
					PL::ISRMarkersRef markers = selectionIter->GetMarkerOwner()->GetMarkers();
					ASL_ASSERT(markers != NULL);
					if (markers->GetMarker(selectionIter->GetGUID(), marker))
					{
						mMarkerMap[markers].push_back(marker);
					}
				}
			}
		}

	private:
		MarkerSet	mMarkerSelection;
		std::map<PL::ISRMarkersRef, CottonwoodMarkerList> mMarkerMap;
	};

public:

	RemoveCottonwoodMarkersAction()
	{
	}

	/*
	**
	*/
	RemoveCottonwoodMarkersAction(
		const MarkerSet &inMarkerSelection) 
		:
		mMarkerSelection(inMarkerSelection)
	{
	}

	/*
	**
	*/
	void Commit(
		BE::ITransactionRef inTransaction)
	{
		inTransaction->Append(RemoveCottonwoodMarkers::Create(mMarkerSelection));
	}

private:
	MarkerSet	mMarkerSelection;
};


class UpdateCottonwoodMarkerAction
{
	/*
	**
	*/
	class UpdateCottonwoodMarker :
		public BE::IUndoable
	{
		typedef ASL::InterfaceRef<UpdateCottonwoodMarker, BE::IUndoable> UpdateCottonwoodMarkerRef;
		ASL_BASIC_CLASS_SUPPORT(UpdateCottonwoodMarker);
		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(IUndoable)
		ASL_QUERY_MAP_END

	public:
		/*
		**
		*/
		static BE::IUndoableRef Create(
			PL::ISRMarkersRef inMarkers,
			const CottonwoodMarker& inOldMarker,
			const CottonwoodMarker& inNewMarker) 
		{
			UpdateCottonwoodMarkerRef addMarker(CreateClassRef());
			addMarker->mMarkers = inMarkers;
			addMarker->mOldMarker = inOldMarker;
			addMarker->mNewMarker = inNewMarker;
			return BE::IUndoableRef(addMarker);
		}

		/*
		**
		*/
		UpdateCottonwoodMarker()
		{
		}

		/*
		**
		*/
		virtual ~UpdateCottonwoodMarker()
		{
		}

		/*
		**
		*/
		virtual void Do()
		{
			if (mMarkers)
			{
				mMarkers->UpdateMarker(mOldMarker, mNewMarker);
			}
		}

		/*
		**
		*/
		virtual void Undo()
		{
			if (mMarkers)
			{
				mMarkers->UpdateMarker(mNewMarker, mOldMarker);
			}
		}

		/*
		**
		*/
		virtual bool IsSafeForPlayback() const
		{
			return false;
		}

	private:
		PL::ISRMarkersRef mMarkers;
		CottonwoodMarker mNewMarker;
		CottonwoodMarker mOldMarker;
	};

public:
	/*
	**
	*/
	UpdateCottonwoodMarkerAction()
	{
	}

	/*
	**
	*/
	UpdateCottonwoodMarkerAction(
		BE::IMasterClipRef inMasterClip,
		const CottonwoodMarker& inOldMarker,
		const CottonwoodMarker& inNewMarker) 
		:
		mOldMarker(inOldMarker),
		mNewMarker(inNewMarker)
	{
		ISRPrimaryClipPlaybackRef primaryClipPlayback = 
			ModulePicker::GetInstance()->GetLoggingClip();
		ASL_ASSERT(primaryClipPlayback != NULL);
		mMarkers = primaryClipPlayback->GetMarkers(inMasterClip);
		ASL_ASSERT(mMarkers != NULL);
	}

	/*
	**
	*/
	void Commit(
		BE::ITransactionRef inTransaction)
	{
		inTransaction->Append(UpdateCottonwoodMarker::Create(mMarkers, mOldMarker, mNewMarker));
	}

private:
	PL::ISRMarkersRef mMarkers;
	CottonwoodMarker mNewMarker;
	CottonwoodMarker mOldMarker;

};

bool RemoveCottonwoodMarkers(
	const MarkerSet &markerSelection)
{
	if (markerSelection.size() == 0)
	{
		return true;
	}

	// If not logging clip mode, it should be deleting from project panel, so don't need to ask.
	// This optimize should be here for PL::BuildMarkersFromSelection will use current module data.
	// If logging clip mode, because all to be deleted markers' associated sub clip project items have been deleted by client,
	// we will find nothing, so no warning dialog will be poped up.
	if (UIF::IsEAMode() && PL::ModulePicker::GetInstance()->GetCurrentEditingModule() == PL::SRLoggingModule)
	{
		bool containsAnyInUsedMarkers = false;
		PL::CottonwoodMarkerList markers = PL::BuildMarkersFromSelection(markerSelection);
		BOOST_FOREACH (const CottonwoodMarker& marker, markers)
		{
			if (marker.GetType() == MarkerType::kInOutMarkerType)
			{
				if (PL::Utilities::IsMarkerInUsed(marker.GetGUID()))
				{
					containsAnyInUsedMarkers = true;
					break;
				}
			}
		}

		if (containsAnyInUsedMarkers)
		{
			ASL::String const& warningTitle = 
				dvacore::ZString("$$$/Prelude/PLCore/InUsedMarkerDeleteTitle=Adobe Prelude");
			ASL::String warningText = dvacore::ZString(
				"$$$/Prelude/PLCore/InUsedMarkerDeleteMessage=The selection you are deleting contains in/out markers references in one or more rough cuts or sequences. If you continue these references will also be deleted. Do you want to continue?");

			UIF::MBResult::Type result = UIF::MessageBox(warningText, warningTitle, UIF::MBFlag::kMBFlagYesNo, UIF::MBIcon::kMBIconWarning);
			if (result != UIF::MBResult::kMBResultYes)
			{
				return false;
			}
		}
	}

	BE::IActionRef action = BE::CreateAction(RemoveCottonwoodMarkersAction(markerSelection), true);

	MZ::ActionID actionIdentifier(kRemoveTemporalMarkersActionID);
	ASL::String actionDisplayName = dvacore::ZString(kRemoveTemporalMarkersActionName);

	MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
		action,
		actionIdentifier,
		actionDisplayName);

	return true;
}

void AddCottonwoodMarker(
	const CottonwoodMarker& inMarker,
	BE::IMasterClipRef	inMasterClip,
	bool inIsSilent)
{
	CottonwoodMarkerList addedMarkerList;
	addedMarkerList.push_back(inMarker);
	BE::IActionRef action = BE::CreateAction(AddMarkersAction(inMasterClip, addedMarkerList, inIsSilent), true);

	MZ::ActionID actionIdentifier(ASL_STR("PL.MetadataActions.AddCottonwoodMarker"));
	ASL::String actionDisplayName = dvacore::ZString("$$$/Prelude/PL/SetMetadataAction/AddTemporalMarker=Add Temporal Marker");

	MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
		action,
		actionIdentifier,
		actionDisplayName);
}

void AddCottonwoodMarkers(
	const CottonwoodMarkerList& inMarkerList,
	BE::IMasterClipRef inMasterClip,
	bool inIsSilent)
{
	if (inMasterClip == NULL || inMarkerList.empty())
	{
		return;
	}

	dvamediatypes::TickTime masterClipDuration = inMasterClip->GetMaxTrimmedDuration(BE::kMediaType_Any);
	CottonwoodMarkerList addedMarkerList;
	CottonwoodMarkerList::const_iterator iter = inMarkerList.begin();
	for ( ; iter != inMarkerList.end(); ++iter)
	{
		//	Only paste the marker if it is not after the end of the clip
		if (iter->GetStartTime() < masterClipDuration)
		{
			CottonwoodMarker marker(*iter);
			//	If the marker is too long, truncate it
			if (marker.GetStartTime() + marker.GetDuration() > masterClipDuration)
			{
				marker.SetDuration(masterClipDuration - marker.GetStartTime());
			}
			addedMarkerList.push_back(marker);
		}
	}
	BE::IActionRef action = BE::CreateAction(AddMarkersAction(inMasterClip, addedMarkerList, inIsSilent), true);

	MZ::ActionID actionIdentifier(ASL_STR("PL.MetadataActions.AddCottonwoodMarkers"));
	ASL::String actionDisplayName = dvacore::ZString("$$$/Prelude/PL/SetMetadataAction/AddTemporalMarkers=Paste Temporal Markers");

	MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
		action,
		actionIdentifier,
		actionDisplayName);
}

/*
**
*/
void AssociateMarkers(
	const CottonwoodMarkerList& inMarkerList,
	BE::IMasterClipRef inMasterClip,
	bool inIsSilent)
{
	if (inMasterClip == NULL || inMarkerList.empty())
	{
		return;
	}

	// If we allow updating markers, use "AssociateMarkersAction"
	BE::IActionRef action = BE::CreateAction(AddMarkersAction(inMasterClip, inMarkerList, inIsSilent));

	MZ::ActionID actionIdentifier(ASL_STR("PL.MetadataActions.AssociateMarkers"));
	ASL::String actionDisplayName = dvacore::ZString("$$$/Prelude/PL/SetMetadataAction/AssociateMarkers=Associate Markers");

	MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
		action,
		actionIdentifier,
		actionDisplayName);
}


bool UpdateCottonwoodMarker(
	BE::IMasterClipRef inMasterClip,
	const CottonwoodMarker& inOldMarker,
	const CottonwoodMarker& inNewMarker)
{
	if (inOldMarker == inNewMarker)
	{
		//nothing to update
		return true;
	}

	if (UIF::IsEAMode()
		&& inOldMarker.GetType() == MarkerType::kInOutMarkerType
		&& inNewMarker.GetType() != MarkerType::kInOutMarkerType)
	{
		if (PL::Utilities::IsMarkerInUsed(inOldMarker.GetGUID()))
		{
			ASL::String const& warningTitle = 
				dvacore::ZString("$$$/Prelude/PLCore/InUsedMarkerTypeChangeTitle=Adobe Prelude");
			ASL::String warningText = dvacore::ZString(
				"$$$/Prelude/PLCore/InUsedMarkerTypeChangeMessage=The in/out marker you are changing has been referred in one or more rough cuts or sequences. If you continue these references will be deleted. Do you want to continue?");

			UIF::MBResult::Type result = UIF::MessageBox(warningText, warningTitle, UIF::MBFlag::kMBFlagYesNo, UIF::MBIcon::kMBIconWarning);
			if (result != UIF::MBResult::kMBResultYes)
			{
				return false;
			}
		}
	}

	BE::IActionRef action = BE::CreateAction(UpdateCottonwoodMarkerAction(inMasterClip, inOldMarker, inNewMarker), true);

	MZ::ActionID actionIdentifier(ASL_STR("PL.MetadataActions.UpdateCottonwoodMarker"));
	ASL::String actionDisplayName = dvacore::ZString("$$$/Prelude/PL/SetMetadataAction/UpdateTemporalMarker=Update Temporal Marker");

	if (PL::ModulePicker::GetInstance()->GetCurrentEditingModule() == PL::SRLoggingModule)
	{
		MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
			action,
			actionIdentifier,
			actionDisplayName);
	}
	else
	{
		// If logging clip is not front, we should never put this action into undo/redo stack (in this case undo/redo stack should only take RC changes).
		MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(MZ::GetProject()), 
			action,
			true);
	}

	return true;
}

bool UpdateCottonwoodMarkersWithoutTypeChange(
	BE::IMasterClipRef inMasterClip,
	MarkerToUpdatePairList inMarkersToUpdate)
{
	BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));
	bool doSomething = false;

	BOOST_FOREACH(MarkerToUpdatePair item, inMarkersToUpdate)
	{
		const CottonwoodMarker& oldMarker = item.first;
		const CottonwoodMarker& newMarker = item.second;

		if (oldMarker == newMarker)
		{
			//nothing to update
			return true;
		}

		BE::IActionRef action = BE::CreateAction(UpdateCottonwoodMarkerAction(inMasterClip, oldMarker, newMarker), true);

		compoundAction->AddAction(action);

		doSomething = true;
	}

	MZ::ActionID actionIdentifier(ASL_STR("PL.MetadataActions.UpdateCottonwoodMarkers"));
	ASL::String actionDisplayName = dvacore::ZString("$$$/Prelude/PL/SetMetadataAction/UpdateTemporalMarkers=Update a bunch of Temporal Markers");

	if (doSomething)
	{
		MZ::ExecuteAction(BE::IExecutorRef(MZ::GetProject()), 
			BE::IActionRef(compoundAction),
			actionIdentifier,
			actionDisplayName);
	}

	return true;
}

}