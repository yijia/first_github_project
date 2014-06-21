/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2010 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

//	Prefix
#include "Prefix.h"

//	Self	
#include "PLSelectionWatcher.h"

//	ASL
#include "ASLStationUtils.h"

#include "MZActivation.h"
#include "MZSequenceActions.h"
#include "MZProject.h"
#include "PLUtilities.h"
#include "MZBEUtilities.h"
#include "PLModulePicker.h"
#include "PLConstants.h"
#include "PLMarker.h"
#include "PLMessage.h"
#include "PLProject.h"
#include "IPLPrimaryClipPlayback.h"
#include "IPLLoggingClip.h"
#include "IPLRoughCut.h"
#include "MZProject.h"

#include "BE/Sequence/IClipTrackItem.h"
#include "BE/Sequence/ITrackItem.h"
#include "BE/Clip/IChildClip.h"
#include "MLPlayerMessages.h"
#include "BENotification.h"
#include "EA/Project/IEAProjectItem.h"

#include "IEAMediaInfo.h"

namespace PL
{
	//	[Comment] Currently, we aren't listen to following msg.
	//	MZ::RefreshMasterAtPositionMessage, MZ::BinSelectedMessage,	MZ::FileItemSelectedMessage ,
	//	MZ::TransitionTrackItemInfoMessage, MZ::EmptyTrackItemInfoMessage, MZ::SourceClipSelectedMessage,
	//	ML::PlayerPositionChangedMessage
ASL_MESSAGE_MAP_DEFINE(SRSelectionWatcher)
	ASL_MESSAGE_HANDLER(MZ::NothingInfoMessage,				OnNothingInfo)					//	Clear Selection
	ASL_MESSAGE_HANDLER(MZ::MultipleItemsInfoMessage,		OnMultipleItemsInfo)			//	Select Multiple items
	ASL_MESSAGE_HANDLER(PL::LibraryClipSelectedMessage,		OnAssetMediaInfoSelection)		//	Select from library
	ASL_MESSAGE_HANDLER(PL::PLStaticMetadataChanged,		OnPLStaticMetadataChanged)//	Metadata internal change

	//	[COMMENT] We comment out these msgs is because current metadata panel only care TrackItem selection change msg
	//	and if we listen to TrackItemInfo change msg we may encounter problem like if you select multiple trackitems 
	//	and save one of them, the content in metadata panel will only show this saved clip's metadata.
	//ASL_MESSAGE_HANDLER(MZ::VideoTrackItemInfoMessage,		OnVideoTrackItemInfo)			//	Only select video item
	//ASL_MESSAGE_HANDLER(MZ::AudioTrackItemInfoMessage,		OnAudioTrackItemInfo)			//	Only select audio item
	//ASL_MESSAGE_HANDLER(MZ::AudioVideoTrackItemInfoMessage, OnAudioVideoTrackItemInfo)		//	Select Video & Audio items

	ASL_MESSAGE_HANDLER(MZ::TrackItemSelectedMessage, OnTrackItemSelected)						//	Select TrackItem, can be video/audio
	ASL_MESSAGE_HANDLER(MZ::LinkedTrackItemPairSelectedMessage, OnLinkedTrackItemSelected)	    //  Select TrackItemPair, Group video& Audio
ASL_MESSAGE_MAP_END

/*
**
*/
SRSelectionWatcher::SRSelectionWatcher()
{
	ASL::StationUtils::AddListener(MZ::kStation_Info, this);
	ASL::StationUtils::AddListener(MZ::kStation_TrackItemActivation, this);
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
SRSelectionWatcher::~SRSelectionWatcher()
{
	mClientList.clear();
}

/*
**
*/
void SRSelectionWatcher::OnMultipleItemsInfo(
	MZ::MultipleItemsInfo inInfo)
{
	// we will never have project item to be select.
	DVA_ASSERT(inInfo.mProjectItemVector.empty());
	if (!inInfo.mTrackItemVector.empty())
	{
		std::vector<BE::IMasterClipRef> items;
		
		BE::TrackItemVector::iterator iter = inInfo.mTrackItemVector.begin();
		BE::TrackItemVector::iterator end = inInfo.mTrackItemVector.end();
		for( ; iter != end; ++iter)
		{
			BE::IMasterClipRef aMasterClipRef = MZ::BEUtilities::GetMasterClipFromTrackItem(*iter);
			if(aMasterClipRef)
			{
				items.push_back(aMasterClipRef);
			}	
		}
		SelectMultipleItems(items);		
	}
}

/*
**
*/
void SRSelectionWatcher::OnNothingInfo(bool)
{
	if (PL::Utilities::NothingSelectedInTimeline())
	{
		SelectZeroItems();
	}
}

/*
**
*/
void SRSelectionWatcher::OnTrackItemSelected(
						BE::ISequenceRef inSequence,
						BE::ITrackItemRef inTrackItem,
						bool )
{
	BE::IClipTrackItemRef clipTrackItem(inTrackItem);

	if (clipTrackItem)
	{
		BE::IChildClipRef theChildClip = clipTrackItem->GetChildClip();
		BE::IMasterClipRef theMasterClip = theChildClip->GetMasterClip();
		if (theMasterClip)
		{
			SelectSingleItem(theMasterClip);
		}
	}
}

/*
**
*/
void SRSelectionWatcher::OnLinkedTrackItemSelected(
								BE::ISequenceRef inSequence,
								BE::ITrackItemRef inFirstTrackItem,
								BE::ITrackItemRef inSecondTrackItem,
								bool)
{
	//	[COMMENT] FirstTrackItem should be video trackItem, SecondTrackItem should be first audio track Item
	//	Premiere will gather info for video&audio from these trackItems
	//	Prelude only care which masterClip it refer to, so, get masterClip from FirstTrackItem will be enouth. 
	BE::ITrackItemRef trackItem = inFirstTrackItem ? inFirstTrackItem : inSecondTrackItem;
	BE::IClipTrackItemRef clipTrackItem(trackItem);
	if (clipTrackItem)
	{
		BE::IChildClipRef theChildClip = clipTrackItem->GetChildClip();
		BE::IMasterClipRef theMasterClip = theChildClip->GetMasterClip();
		if (theMasterClip)
		{
			SelectSingleItem(theMasterClip);
		}
	}
}

/*
**
*/
void SRSelectionWatcher::SelectZeroItems()
{
	PL::SRAssetSelectionManager::MediaURLSet mediaPathSet;
	SRProject::GetInstance()->GetAssetSelectionManager()->TriggerMetaDataSelection(mediaPathSet);
}

/*
**
*/
void SRSelectionWatcher::SelectSingleItem(BE::IMasterClipRef inItem)
{
	if(inItem)
	{
		ASL::PathnameList mediaURLList;
		ASL::String mediaURL;

		BE::IMediaRef media = BE::MasterClipUtils::GetMedia(inItem);
		if (!media)
		{
			DVA_ASSERT(0);
			return;
		}

		BE::IProjectItemRef projectItem(inItem->GetOwner());
		if (MZ::BEUtilities::IsEAProjectItem(projectItem))
		{
			mediaURL = MZ::BEUtilities::GetEAAssetIDInProjectItem(projectItem);
		}
		else
		{
			mediaURL = MZ::BEUtilities::GetMediaPathFromMasterClip(inItem);
		}
			
			
		PL::SRAssetSelectionManager::MediaURLSet mediaURLSet;
		if (!mediaURL.empty())
		{
			mediaURLSet.insert(mediaURL);
		}

		if(!mediaURLSet.empty())
		{
			SRProject::GetInstance()->GetAssetSelectionManager()->TriggerMetaDataSelection(mediaURLSet);
		}
	}	
}

/*
**
*/
void SRSelectionWatcher::SelectMultipleItems(const std::vector<BE::IMasterClipRef>& inItem)
{
	std::vector<BE::IMasterClipRef>::const_iterator iter = inItem.begin();
	PL::SRAssetSelectionManager::MediaURLSet mediaPathSet;
	for(; iter != inItem.end(); ++iter)
	{
		ASL::String mediaPath;
		BE::IProjectItemRef projectItem((*iter)->GetOwner());
		if (MZ::BEUtilities::IsEAProjectItem(projectItem))
		{
			mediaPath = MZ::BEUtilities::GetEAAssetIDInProjectItem(projectItem);
		}
		else
		{
			mediaPath = MZ::BEUtilities::GetMediaPathFromMasterClip(*iter);
		}

		if (!mediaPath.empty())
		{
			mediaPathSet.insert(mediaPath);
		}
	}

	if(!mediaPathSet.empty())
	{
		SRProject::GetInstance()->GetAssetSelectionManager()->TriggerMetaDataSelection(mediaPathSet);
	}
}

/*
**
*/
bool SRSelectionWatcher::ClearStoredSelection()
{
	mSelectedItems.clear();
	return true;
}

/*
**
*/
void SRSelectionWatcher::Register(SelectionWatcherClientPtr inClient)
{
	if (NULL == inClient)
	{
		return;
	}

	SelectionWatcherClientList::const_iterator it;
	SelectionWatcherClientList::const_iterator begin = mClientList.begin();
	SelectionWatcherClientList::const_iterator end = mClientList.end();

	for (it = begin; it != end; ++it)
	{
		if ( (*it).get() == inClient.get() )
		{
			break;
		}
	}

	if ( it == end )
	{
		mClientList.push_back(inClient);
	}
}

/*
**
*/
void SRSelectionWatcher::UnRegister(SelectionWatcherClientPtr inClient)
{
	if (NULL == inClient)
	{
		return;
	}

	SelectionWatcherClientList::iterator iter = mClientList.begin();
	SelectionWatcherClientList::iterator end = mClientList.end();
	for (; iter != end; ++iter)
	{
		if ( (*iter).get() == inClient.get() )
		{
			break;
		}
	}

	if ( iter != end )
	{
		mClientList.erase(iter);
	}
}

/*
**
*/
void SRSelectionWatcher::NotifySelectionChanged()
{
	SelectionWatcherClientList::const_iterator begin = mClientList.begin();
	SelectionWatcherClientList::const_iterator end = mClientList.end();
	for (SelectionWatcherClientList::const_iterator it = begin; it != end; ++it)
	{
		(*it)->SelectionDidChange();
	}
}

/*
**
*/
void SRSelectionWatcher::OnPLStaticMetadataChanged()
{
	SelectionWatcherClientList::const_iterator begin = mClientList.begin();
	SelectionWatcherClientList::const_iterator end = mClientList.end();
	for (SelectionWatcherClientList::const_iterator it = begin; it != end; ++it)
	{
		(*it)->UpdateMetadata();
	}
}

/*
**
*/
void SRSelectionWatcher::OnAssetMediaInfoSelection(
	PL::AssetMediaInfoSelectionList const& inSelection)
{
	if (ClearStoredSelection())
	{
		mSelectionType = typeid(PL::AssetMediaInfoSelectionRef);
		PL::AssetMediaInfoSelectionList::const_iterator iter = inSelection.begin();
		PL::AssetMediaInfoSelectionList::const_iterator end = inSelection.end();
		for (; iter != end; ++iter)
		{
			mSelectedItems.push_back(ASL::ASLUnknownRef(*iter));
		}

		NotifySelectionChanged();
	}
}

/*
**
*/
SelectionContainer SRSelectionWatcher::GetSelectedItems()
{
	return mSelectedItems;
}

/*
**
*/
ASL::TypeInfo& SRSelectionWatcher::GetSelectedItemsType()
{
	return mSelectionType;
}


} // namespace PL
