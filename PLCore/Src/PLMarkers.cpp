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

#include "Prefix.h"

// Local
#include "PLMarkers.h"

//	PL
#include "IPLMarkers.h"
#include "PLConstants.h"
#include "PLUtilities.h"
#include "PLMessage.h"
#include "PLProject.h"
#include "PLUtilitiesPrivate.h"
#include "PLModulePicker.h"
#include "PLMarkerOwner.h"

//	MZ
#include "MZActivation.h"
#include "MZBEUtilities.h"

//	ASL
#include "ASLStationUtils.h"
#include "ASLStringCompare.h"
#include "ASLStationUtils.h"
#include "ASLPathUtils.h"

//	BE
#include "BE/Clip/IClip.h"



namespace PL
{
	static const dvacore::StdString kXMPPartID_Tracks("/metadata/xmpDM/Tracks");
	static const dvacore::StdString kXMPPartID_Metadata("/metadata");

    bool IsMarkerFilteredByString(CottonwoodMarker const& inMarker, dvacore::UTF16String const& inFilterText)
	{
		std::vector<dvacore::UTF16String> searchAreas;
		searchAreas.push_back(inMarker.GetComment());
		searchAreas.push_back(inMarker.GetName());
		searchAreas.push_back(MarkerType::GetDisplayNameForType(inMarker.GetType()));
		searchAreas.push_back(inMarker.GetLocation());
		searchAreas.push_back(inMarker.GetProbability());
		searchAreas.push_back(inMarker.GetSpeaker());
		searchAreas.push_back(inMarker.GetTarget());
		searchAreas.push_back(inMarker.GetCuePointType());
		for (size_t i = 0; i < inMarker.GetCuePointList().size(); ++i)
		{
			searchAreas.push_back(inMarker.GetCuePointList()[i].mValue);
		}

		TagParamMap tagParams = inMarker.GetTagParams();
		for (TagParamMap::iterator it = tagParams.begin(); it != tagParams.end(); it++)
		{
			searchAreas.push_back((*it).second->GetName());
			searchAreas.push_back((*it).second->GetPayload());
		}

		std::vector<dvacore::UTF16String>::const_iterator searchAreaIter = searchAreas.begin(); 

		for (; searchAreaIter != searchAreas.end(); ++searchAreaIter)
		{
			if (ASL::CaseInsensitive::StringContains(*searchAreaIter, inFilterText))
			{
				return true;
			}
		}
		return false;
	}

	static void SelectMarkers(ISRMarkerSelectionRef ioMarkerSelection, const CottonwoodMarkerList& inMarkers)
	{
		DVA_ASSERT(ioMarkerSelection);

		if ( ioMarkerSelection == NULL || inMarkers.empty() )
		{
			return;
		}

		ioMarkerSelection->ClearSelection();

		MarkerSet addedMarkers;
		BOOST_FOREACH(const CottonwoodMarker& marker, inMarkers)
		{
			addedMarkers.insert(marker);
		}

		ioMarkerSelection->AddMarkersToSelection(addedMarkers);
	}

	static void DeselectMarkers(ISRMarkerSelectionRef ioMarkerSelection, const CottonwoodMarkerList& inMarkers)
	{
		DVA_ASSERT(ioMarkerSelection);

		if ( ioMarkerSelection == NULL || inMarkers.empty() )
		{
			return;
		}

		MarkerSet removedMarkers;
		BOOST_FOREACH(const CottonwoodMarker& marker, inMarkers)
		{
			if (ioMarkerSelection->IsMarkerSelected(marker))
			{
				removedMarkers.insert(marker);
			}
		}

		if (!removedMarkers.empty())
		{
			ioMarkerSelection->RemoveMarkersFromSelection(removedMarkers);
		}
	}

	PL::CottonwoodMarkerList BuildMarkersFromSelection(const PL::MarkerSet & inMarkerSelection)
	{
		PL::CottonwoodMarkerList markerList;

		PL::ISRPrimaryClipPlaybackRef primaryClipPlayback = 
			PL::ModulePicker::GetInstance()->GetCurrentModuleData();
		if (primaryClipPlayback != NULL)
		{
			PL::MarkerSet::const_iterator selectionIter = 
				inMarkerSelection.begin();
			for ( ; selectionIter != inMarkerSelection.end(); ++selectionIter)
			{
				PL::CottonwoodMarker cwMarker;
                PL::ISRMarkersRef markers = selectionIter->GetMarkerOwner()->GetMarkers();
				if (markers && markers->GetMarker(selectionIter->GetGUID(), cwMarker))
				{
					markerList.push_back(cwMarker);
				}
			}
		}

		return markerList;
	}

	
    SRMarkers::SRMarkers()
    {
    }

	SRMarkers::~SRMarkers()
	{
	}
    
    PL::ISRMarkersRef SRMarkers::Create(ISRMediaRef inSRMedia)
    {
        SRMarkersRef srMarkers = CreateClassRef();
        if (srMarkers)
        {
            srMarkers->Init(inSRMedia);
        }
        
        return PL::ISRMarkersRef(srMarkers);
    }
    
    
    void SRMarkers::Init(ISRMediaRef inSRMedia)
    {
        mSRMedia = inSRMedia;
        ASL::StationUtils::AddListener(
                                       SRProject::GetInstance()->GetAssetMediaInfoWrapper(mSRMedia->GetClipFilePath())->GetStationID(),
                                       this);
    }

	void SRMarkers::RefineMarker(CottonwoodMarker& ioMarker)
	{
		dvamediatypes::FrameRate mediaFrameRate = GetMediaFrameRate();
		dvamediatypes::TickTime startTime = ioMarker.GetStartTime();
		dvamediatypes::TickTime endTime = ioMarker.GetStartTime() + ioMarker.GetDuration();

		startTime.AlignToFrame(mediaFrameRate);
		endTime.AlignToFrame(mediaFrameRate);

		dvamediatypes::TickTime mediaDuration = GetMediaDuration();
		if (startTime < dvamediatypes::kTime_Zero)
		{
			startTime = dvamediatypes::kTime_Zero;
		}
		else if(startTime >= mediaDuration)
		{
			startTime  = mediaDuration - dvamediatypes::TickTime(1, mediaFrameRate);
		}

		if (endTime < startTime)
		{
			endTime = startTime;
		}
		else if(endTime > mediaDuration)
		{
			endTime = mediaDuration;
		}

		dvamediatypes::TickTime markerDuation = endTime - startTime;

		if (ioMarker.GetType() == MarkerType::kInOutMarkerType && markerDuation == dvamediatypes::kTime_Zero)
		{
			markerDuation  = dvamediatypes::TickTime(1, mediaFrameRate);
		}

		ioMarker.SetStartTime(startTime);
		ioMarker.SetDuration(markerDuation);
	}

	dvamediatypes::TickTime SRMarkers::GetMediaDuration()
	{
		return Utilities::GetClipDurationRegardlessBoundaries(mSRMedia->GetMasterClip());
	}

	ASL::Guid SRMarkers::GetMediaInfoID()
	{
		return SRProject::GetInstance()->GetAssetMediaInfo(mSRMedia->GetClipFilePath())->GetAssetMediaInfoGUID();
	}

	dvamediatypes::FrameRate SRMarkers::GetMediaFrameRate()
	{
		dvamediatypes::FrameRate frameRate = dvamediatypes::kFrameRate_VideoNTSC;
		if(mSRMedia)
		{
			BE::IClipRef clip; 
			clip = mSRMedia->GetMasterClip()->GetClip(MF::kMediaType_Video, 0);
			if (clip)
			{
				frameRate = clip->GetFrameRate();
			}
			else
			{
				//	For audio only clip, we use it's the outmost sequence's framerate to refine markers, so that we wouldn't met 
				//	audio subclips with 1 sample length.
				frameRate = MZ::GetRecentSequenceVideoFrameRate();
			}

		}

		return frameRate;
	}

	void SRMarkers::SetDirty(bool inDirty)
	{
		ASL_ASSERT(mSRMedia != NULL);
		mSRMedia->SetDirty(inDirty);
	}
	
	XMPText SRMarkers::BuildXMPStringFromMarkers()
	{
		PL::AssetMediaInfoPtr assetMediaInfo = 
			SRProject::GetInstance()->GetAssetMediaInfo(mSRMedia->GetClipFilePath());
		ASL::StdString xmpContent(*assetMediaInfo->GetXMPString().get());
		Utilities::BuildXMPStringFromMarkers(xmpContent , mMarkers, mTrackTypes, GetMediaFrameRate());
		XMPText xmpText(new ASL::StdString(xmpContent));

		mMarkerState = GetMarkerState(xmpText);

		return xmpText;
	}

	bool SRMarkers::BuildMarkersFromXMPString(
					XMPText inXMPString, 
					bool inSendNotification)
	{
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);

		dvacore::StdString latestMarkerState = GetMarkerState(inXMPString);
		if (latestMarkerState != mMarkerState)
		{
			// get rid of the old ones...
			mMarkers.clear();
			Utilities::BuildMarkersFromXMPString(*inXMPString.get(), mTrackTypes, mMarkers, ISRMarkerOwnerRef(mSRMedia));
			mMarkerState = latestMarkerState;

			// If this is not called by the initialization, trigger marker bar and 
			// marker list view refresh with CottonwoodMarkerChangedEvent
			if (inSendNotification)
			{
				CottonwoodMarkerList changedMarkers;
				ASL::StationUtils::PostMessageToUIThread(
					MZ::kStation_PreludeProject, 
					SilkRoadMarkerChanged(changedMarkers),
					true);
			}
		}
		
		return true;
	}

	void SRMarkers::AddMarker(const CottonwoodMarker& inNewMarker, bool inIsSilent)
	{
		CottonwoodMarker addedMarker = inNewMarker;
        addedMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));
		{
			ASL::CriticalSectionLock lock(mMarkerCriticalSection);
			RefineMarker(addedMarker);
			mMarkers.insert(addedMarker);
		}
		SetDirty(true);
		
		CottonwoodMarkerList changedMarkers;
		changedMarkers.push_back(addedMarker);

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMarkerAdded(changedMarkers, inIsSilent),
			true);
	}

	void SRMarkers::AddMarkers(const CottonwoodMarkerList& inMarkers, bool inIsSilent)
	{
		CottonwoodMarkerList changedMarkers;
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		for (CottonwoodMarkerList::const_iterator itr = inMarkers.begin(); itr != inMarkers.end(); ++itr)
		{
			CottonwoodMarker addedMarker = (*itr);
            addedMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));
			RefineMarker(addedMarker);
			mMarkers.insert(addedMarker);

			changedMarkers.push_back(addedMarker);
		}
		SetDirty(true);

		if (!inIsSilent)
		{
			SelectMarkers(ISRMarkerSelectionRef(ModulePicker::GetInstance()->GetLoggingClip()), changedMarkers);
		}

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMarkerAdded(changedMarkers, inIsSilent),
			true);
	}


	dvacore::StdString SRMarkers::GetMarkerState(XMPText inXMPString)
	{
		ASL::Result result(-1);
		dvacore::StdString metadataState;

		dvacore::StdString partID = std::string(kXMPPartID_Tracks);

		try	
		{
			SXMPDocOps xmpDocOps;
			SXMPMeta meta(inXMPString->c_str(), static_cast<XMP_StringLen>(inXMPString->length()));
			xmpDocOps.OpenXMP(reinterpret_cast<SXMPMeta*>(&meta), "");

			//	We ask the doc ops if there is a part ID in this file.
			xmpDocOps.GetPartChangeID(partID.c_str(), &metadataState);
		}
		catch(...)
		{
		}

		return metadataState;
	}

	void SRMarkers::MarkerChanged(const ASL::String& inMediaLocatorID)
	{
		XMPText xmpStr =SRProject::GetInstance()->GetAssetMediaInfo(mSRMedia->GetClipFilePath())->GetXMPString();
		BuildMarkersFromXMPString(xmpStr, true);
	}
	
	void SRMarkers::RemoveMarker(const CottonwoodMarker& inMarker)
	{
		{
			ASL::CriticalSectionLock lock(mMarkerCriticalSection);
			mMarkers.erase(inMarker);
		}
		SetDirty(true);
		
		CottonwoodMarker changedMarker(inMarker);
        changedMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));

		CottonwoodMarkerList changedMarkers;
		changedMarkers.push_back(changedMarker);
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMarkerDeleted(changedMarkers),
			true);
	}

	void SRMarkers::RemoveMarkers(const CottonwoodMarkerList& inMarkers)
	{
		CottonwoodMarkerList changedMarkers;
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		for (CottonwoodMarkerList::const_iterator itr = inMarkers.begin(); itr != inMarkers.end(); ++itr)
		{
			mMarkers.erase(*itr);
			
			CottonwoodMarker changedMarker(*itr);
			changedMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));
			changedMarkers.push_back(changedMarker);
		}
		SetDirty(true);
		
		DeselectMarkers(ISRMarkerSelectionRef(ModulePicker::GetInstance()->GetLoggingClip()), changedMarkers);

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMarkerDeleted( changedMarkers),
			true);
	}
	
	void SRMarkers::UpdateMarker(
		  const CottonwoodMarker& inOldMarker,
		  const CottonwoodMarker&	inNewMarker)
	{
		CottonwoodMarker addedMarker = inNewMarker;
        addedMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));
		ASL_REQUIRE(inOldMarker.GetGUID() == addedMarker.GetGUID());
		{
			ASL::CriticalSectionLock lock(mMarkerCriticalSection);
			
			mMarkers.erase(inOldMarker);
			RefineMarker(addedMarker);
			mMarkers.insert(addedMarker);
		}
		SetDirty(true);


		CottonwoodMarkerList changedMarkers;
		changedMarkers.push_back(addedMarker);
		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_PreludeProject, 
			SilkRoadMarkerChanged(changedMarkers),
			true);
	}

	void SRMarkers::UpdateMarkers(const CottonwoodMarkerList& inMarkers)
	{
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);

		CottonwoodMarkerList changedMarkers;
		BOOST_FOREACH(CottonwoodMarker newMarker, inMarkers)
		{
            newMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(mSRMedia));
			CottonwoodMarker marker;
			marker.SetGUID(newMarker.GetGUID());
			MarkerSet::iterator it = mMarkers.find(marker);
			if (it != mMarkers.end())
			{
				RefineMarker(newMarker);
				mMarkers.erase(it);
				mMarkers.insert(newMarker);

				CottonwoodMarker changedMarker(newMarker);
				changedMarkers.push_back(changedMarker);
			}
		}

		if (!changedMarkers.empty())
		{
			SetDirty(true);

			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject, 
				SilkRoadMarkerChanged(changedMarkers),
				true);
		}
	}
	
	bool SRMarkers::GetMarker(
		ASL::Guid inMarkerID,
		CottonwoodMarker& outMarker)
	{
		CottonwoodMarker marker;
		marker.SetGUID(inMarkerID);

		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		MarkerSet::iterator itr = mMarkers.find(marker);
		if (itr != mMarkers.end())
		{
			outMarker = *itr;
			return true;
		}

		return false;
	}

	/**
	** Checks, if there is a marker, matching the given properties
	*/
	bool SRMarkers::GetMarkerGUID(
		dvacore::UTF16String inMarkerType,
		std::int64_t inMarkerStartTime,
		std::int64_t inMarkerDuration,
		ASL::Guid& outGUID)
	{
		dvamediatypes::TickTime startTime = dvamediatypes::TickTime::TicksToTime(inMarkerStartTime);
		dvamediatypes::TickTime duration = dvamediatypes::TickTime::TicksToTime(inMarkerDuration);

		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
		{
			bool boolStartTimeEqual = false;
			if (startTime == (*itr).GetStartTime())
			{
				boolStartTimeEqual = true;
			}

			bool boolDurationEqual = false;
			if (duration == (*itr).GetDuration())
			{
				boolDurationEqual = true;
			}

			bool boolTypeEqual = false;
			if (inMarkerType == (*itr).GetType())
			{
				boolTypeEqual = true;
			}

			if (boolStartTimeEqual && boolDurationEqual && boolTypeEqual)
			{
				outGUID = (*itr).GetGUID();
				return true;
			}
		}

		outGUID = ASL::Guid();
		return false;
	}

	/**
	** Get All markers mixed into a single track sorted by start time
	*/
	MarkerTrack SRMarkers::GetMarkers()
	{
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		MarkerTrack track;
		for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
		{
			track.insert(std::make_pair((*itr).GetStartTime(), (*itr)));
		}
		return track;
	}

	/**
	** Get all markers split into separate tracks
	*/
	MarkerTracks SRMarkers::GetMarkerTracks()
	{
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		MarkerTracks tracks;

		for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
		{
			dvacore::UTF16String markerType = (*itr).GetType();
			tracks.insert(std::make_pair(markerType, MarkerTrack()));
		}

		for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
		{
			dvacore::UTF16String markerType = (*itr).GetType();
			dvamediatypes::TickTime startTime = (*itr).GetStartTime();
			CottonwoodMarker marker = *itr;
			tracks[markerType].insert(std::make_pair(startTime, marker));
		}
		return tracks;
	}

	/**
	** Get a specific markers by type in a track
	*/
	MarkerTrack SRMarkers::GetMarkerTrack(
		const dvacore::UTF16String& inMarkerType)
	{
		ASL::CriticalSectionLock lock(mMarkerCriticalSection);
		MarkerTrack track;
		for (MarkerSet::iterator itr = mMarkers.begin(); itr != mMarkers.end(); ++itr)
		{
			if (itr->GetType() == inMarkerType)
			{
				track.insert(std::make_pair(itr->GetStartTime(), (*itr)));
			}
		}

		return track;
	}

	/**
	** Find a marker in all MarkerTracks
	*/
	bool SRMarkers::HasMarker(const ASL::Guid& inMarkerId)
	{
		MarkerSet::const_iterator end = mMarkers.end();
		MarkerSet::const_iterator it = mMarkers.begin();

		for (; it != end; ++it)
		{
			const CottonwoodMarker& marker = *it;
			if ( marker.GetGUID() == inMarkerId )
			{
				return true;
			}
		}

		return false;
	}


	/**
	**	Get a time-sorted and filtered list of markers
	*/
	void SRMarkers::GetTimeSortedAndFilterMarkerList(
		const MarkerTypeFilterList& inFilter,
		const dvacore::UTF16String& inFilterText,
		CottonwoodMarkerList& outMarkers,
		const dvamediatypes::TickTime& inInPoint,
		const dvamediatypes::TickTime& inDuration)
	{
		bool hasFilter = !inFilter.empty();
		bool hasFilterText = !inFilterText.empty();
		MarkerTrack markers(GetMarkers());
		MarkerTrack::iterator iter(markers.begin());
		for ( ; iter != markers.end(); ++iter)
		{
			bool insert = true;
			if (hasFilter)
			{
				insert = inFilter.find(iter->second.GetType()) != inFilter.end();
			}
			if (insert && hasFilterText)
			{
				insert = IsMarkerFilteredByString(iter->second, inFilterText);					
			}
			
			if (insert)
			{
				dvamediatypes::TickTime inPoint = iter->second.GetStartTime();
				dvamediatypes::TickTime outPoint = iter->second.GetStartTime() + iter->second.GetDuration();

				if (!(inPoint > inInPoint + inDuration || outPoint < inInPoint))
				{
					outMarkers.push_back(iter->second);
				}
			}
		}
	}

	BE::IMasterClipRef SRMarkers::GetMediaMasterClip() const
	{
		return mSRMedia != NULL ? mSRMedia->GetMasterClip() : BE::IMasterClipRef();
	}

}



