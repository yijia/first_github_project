/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

// Local
#include "PLSequenceContextInfo.h"
#include "IPLPrimaryClipPlayback.h"
#include "PLMessage.h"
#include "PLModulePicker.h"
#include "PLProject.h"
#include "IPLRoughCut.h"
#include "IPLLoggingClip.h"

// ASL
#include "ASLStationUtils.h"


namespace PL {

extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_NULL = 0;
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_Clip = 1;
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_RoughCut = 2;
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_Empty = 99999;

SequenceContextInfoSupport::SharedPtr SequenceContextInfoSupport::sSequenceContextInfoSupport;

void SequenceContextInfoSupport::Initialize()
{
	if (!sSequenceContextInfoSupport)
	{
		sSequenceContextInfoSupport.reset(new SequenceContextInfoSupport());
	}
}

void SequenceContextInfoSupport::Terminate()
{
	if (sSequenceContextInfoSupport)
	{
		sSequenceContextInfoSupport.reset();
	}
}

SequenceContextInfoSupport::SharedPtr SequenceContextInfoSupport::GetInstance()
{
	DVA_ASSERT(sSequenceContextInfoSupport);
	return sSequenceContextInfoSupport;
}

ASL_MESSAGE_MAP_DEFINE(SequenceContextInfoSupport)
	ASL_MESSAGE_HANDLER(PreludeEditModeChanged, OnEditModeChanged);
	ASL_MESSAGE_HANDLER(SilkRoadMediaDirtyChanged, OnSilkRoadMediaDirtyChanged);
	ASL_MESSAGE_HANDLER(SilkRoadRoughCutDirtyChanged, OnSilkRoadRoughCutDirtyChanged);
	ASL_MESSAGE_HANDLER(PrimaryClipRenamedMessage, OnPrimaryClipRenamed);
	ASL_MESSAGE_HANDLER(SilkRoadLoggingClipRelinked, OnLoggingClipRelinked);
	ASL_MESSAGE_HANDLER(SilkRoadRoughCutRelinked, OnRoughCutRelinked);
ASL_MESSAGE_MAP_END

void SequenceContextInfoSupport::InitializeSequenceContextInfos()
{
	RemoveAllSequenceContextInfos();

	ISRPrimaryClipPlaybackRef rcPlayback = 
		ModulePicker::GetInstance()->GetRoughCutClip();
	ISRPrimaryClipPlaybackRef loggingPlayback = 
		ModulePicker::GetInstance()->GetLoggingClip();
	ISRPrimaryClipPlaybackRef currentPlayback = 
		ModulePicker::GetInstance()->GetCurrentModuleData();

	if (rcPlayback)
	{
		if (rcPlayback->GetBESequence())
		{
			SequenceContextInfoPtr sequenceContextInfo(
				new SequenceContextInfo(
					kSequenceContextID_RoughCut, 
					ISRPrimaryClipRef(rcPlayback), 
					GetAssetName(ISRPrimaryClipRef(rcPlayback)),
					rcPlayback == currentPlayback));
			mSequenceContextInfoMap[kSequenceContextID_RoughCut] = sequenceContextInfo;
		}
	}
	if (loggingPlayback)
	{
		if (loggingPlayback->GetBESequence())
		{
			SequenceContextInfoPtr sequenceContextInfo(
				new SequenceContextInfo(
				kSequenceContextID_Clip, 
				ISRPrimaryClipRef(loggingPlayback), 
				GetAssetName(ISRPrimaryClipRef(loggingPlayback)),
				loggingPlayback == currentPlayback));
			mSequenceContextInfoMap[kSequenceContextID_Clip] = sequenceContextInfo;
		}
	}
}

void SequenceContextInfoSupport::RemoveAllSequenceContextInfos()
{
	mSequenceContextInfoMap.clear();
}

SequenceContextInfoMap SequenceContextInfoSupport::GetSequenceContextInfoMap()
{
	return mSequenceContextInfoMap;
}

SequenceContextInfoPtr SequenceContextInfoSupport::GetSequenceContextInfo(UIF::MultiContextHandler::ContextID inContextID)
{
	SequenceContextInfoMap::iterator pos = mSequenceContextInfoMap.find(inContextID);
	if (pos != mSequenceContextInfoMap.end())
	{
		return pos->second;
	}
	return SequenceContextInfoPtr();
}

SequenceContextInfoPtr SequenceContextInfoSupport::GetActiveSequenceContextInfo()
{
	SequenceContextInfoMap::iterator it = mSequenceContextInfoMap.begin();
	SequenceContextInfoMap::iterator end = mSequenceContextInfoMap.end();
	for (; it != end; ++it)
	{
		SequenceContextInfoPtr info = it->second;
		if (info->mIsActive)
			return info;
	}
	return SequenceContextInfoPtr();
}

bool SequenceContextInfoSupport::AllowCloseContext(UIF::MultiContextHandler::ContextID inContextID)
{
	ISRPrimaryClipPlaybackRef playback;
	if (inContextID == kSequenceContextID_RoughCut)
	{
		playback = ModulePicker::GetInstance()->GetRoughCutClip();
	}
	else if (inContextID == kSequenceContextID_Clip)
	{
		playback = ModulePicker::GetInstance()->GetLoggingClip();
	}

	if (playback != NULL)
	{
		SequenceContextInfoPtr sequenceContextInfo = GetSequenceContextInfo(inContextID);
		if (sequenceContextInfo && sequenceContextInfo->mPrimaryClip == PL::ISRPrimaryClipRef(playback))
		{
			ISRPrimaryClipRef primaryClip = ISRPrimaryClipRef(playback);
			if (primaryClip)
			{
				if ( primaryClip->IsDirty() )
				{
					UIF::MBResult::Type saveResult = primaryClip->Save(ModulePicker::IsAutoSaveEnabled(), true);
					if (saveResult == UIF::MBResult::kMBResultCancel)
						return false;
				}
			}
		}
	}
	return true;
}

bool SequenceContextInfoSupport::SwitchContext(UIF::MultiContextHandler::ContextID inContextID)
{
	SequenceContextInfoPtr sequenceContextInfo = GetSequenceContextInfo(inContextID);
	if (sequenceContextInfo == NULL)
		return false;

	ISRPrimaryClipPlaybackRef playback;
	if (inContextID == kSequenceContextID_RoughCut)
	{
		playback = ModulePicker::GetInstance()->GetRoughCutClip();
	}
	else if (inContextID == kSequenceContextID_Clip)
	{
		playback = ModulePicker::GetInstance()->GetLoggingClip();
	}
	else
	{
		DVA_ASSERT(false);
	}

	if (playback != NULL)
	{
		if (sequenceContextInfo->mPrimaryClip == PL::ISRPrimaryClipRef(playback))
		{
			ISRPrimaryClipRef primaryClip = ISRPrimaryClipRef(playback);
			if (primaryClip)
			{
				ModulePicker::GetInstance()->ActiveModule(primaryClip->GetAssetItem(), false);
				return true;
			}
		}
	}
	return false;
}

void SequenceContextInfoSupport::CloseContext(UIF::MultiContextHandler::ContextID inContextID)
{
	SequenceContextInfoPtr sequenceContextInfo = GetSequenceContextInfo(inContextID);
	if (sequenceContextInfo == NULL)
		return;

	ISRPrimaryClipPlaybackRef playback;
	if (inContextID == kSequenceContextID_RoughCut)
	{
		playback = ModulePicker::GetInstance()->GetRoughCutClip();
	}
	else if (inContextID == kSequenceContextID_Clip)
	{
		playback = ModulePicker::GetInstance()->GetLoggingClip();
	}
	else
	{
		DVA_ASSERT(false);
	}

	if (playback != NULL)
	{
		DVA_ASSERT (sequenceContextInfo->mPrimaryClip == PL::ISRPrimaryClipRef(playback));
		if (sequenceContextInfo->mPrimaryClip == PL::ISRPrimaryClipRef(playback))
		{
			ISRPrimaryClipRef primaryClip = ISRPrimaryClipRef(playback);
			if (primaryClip)
			{
				ModulePicker::GetInstance()->CloseModule(primaryClip->GetAssetItem()->GetMediaPath());
			}
		}
	}
}

SequenceContextInfoSupport::SequenceContextInfoSupport()
{
	ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
	InitializeSequenceContextInfos();
}

SequenceContextInfoSupport::~SequenceContextInfoSupport()
{
	RemoveAllSequenceContextInfos();
	ASL::StationUtils::RemoveListener(MZ::kStation_PreludeProject, this);
}

/*
**
*/
void SequenceContextInfoSupport::OnEditModeChanged()
{
	InitializeSequenceContextInfos();
	ASL::StationUtils::PostMessageToUIThread(
		MZ::kStation_PreludeProject, 
		SequenceContextInfoMapChanged(),
		true);
}

/*
**Set dirty flag for LoggingClip
*/
void SequenceContextInfoSupport::OnSilkRoadMediaDirtyChanged()
{
	ISRPrimaryClipPlaybackRef loggingPlayback = ModulePicker::GetInstance()->GetLoggingClip();
	if (loggingPlayback == NULL)
		return;

	SequenceContextInfoPtr sequenceContextInfo = mSequenceContextInfoMap[kSequenceContextID_Clip];
	DVA_ASSERT(sequenceContextInfo);

	ASL::String itemName = GetAssetName(ISRPrimaryClipRef(loggingPlayback));
	sequenceContextInfo->mName = itemName;

	ASL::StationUtils::BroadcastMessage(
		MZ::kStation_PreludeProject, 
		SequenceContextInfoChanged(kSequenceContextID_Clip));
}

/*
**
*/
void SequenceContextInfoSupport::OnRoughCutRelinked()
{
	ISRPrimaryClipPlaybackRef rcPlayback = ModulePicker::GetInstance()->GetRoughCutClip();
	if (rcPlayback == NULL)
		return;

	SequenceContextInfoPtr sequenceContextInfo = mSequenceContextInfoMap[kSequenceContextID_RoughCut];
	DVA_ASSERT(sequenceContextInfo);

	sequenceContextInfo->mPrimaryClip = PL::ISRPrimaryClipRef(rcPlayback);

	ASL::StationUtils::BroadcastMessage(
		MZ::kStation_PreludeProject, 
		SequenceContextInfoChanged(kSequenceContextID_RoughCut));
}

/*
**
*/
void SequenceContextInfoSupport::OnLoggingClipRelinked()
{
	ISRPrimaryClipPlaybackRef loggingPlayback = ModulePicker::GetInstance()->GetLoggingClip();
	if (loggingPlayback == NULL)
		return;

	SequenceContextInfoPtr sequenceContextInfo = mSequenceContextInfoMap[kSequenceContextID_Clip];
	DVA_ASSERT(sequenceContextInfo);

	sequenceContextInfo->mPrimaryClip = PL::ISRPrimaryClipRef(loggingPlayback);

	ASL::StationUtils::BroadcastMessage(
		MZ::kStation_PreludeProject, 
		SequenceContextInfoChanged(kSequenceContextID_Clip));
}


/*
**Set dirty flag for RoughCut
*/
void SequenceContextInfoSupport::OnSilkRoadRoughCutDirtyChanged()
{
	ISRPrimaryClipPlaybackRef rcPlayback = ModulePicker::GetInstance()->GetRoughCutClip();
	if (rcPlayback == NULL)
		return;

	// in some case, the Roughcut's sequence point will change when it change to dirty.
	SequenceContextInfoPtr sequenceContextInfo = mSequenceContextInfoMap[kSequenceContextID_RoughCut];
	DVA_ASSERT(sequenceContextInfo);

	ASL::String itemName = GetAssetName(ISRPrimaryClipRef(rcPlayback));
	sequenceContextInfo->mName = itemName;
	sequenceContextInfo->mPrimaryClip = PL::ISRPrimaryClipRef(rcPlayback);


	ASL::StationUtils::BroadcastMessage(
		MZ::kStation_PreludeProject, 
		SequenceContextInfoChanged(kSequenceContextID_RoughCut));
}

/*
**
*/
void SequenceContextInfoSupport::OnPrimaryClipRenamed(
	const ASL::String& inMediaPath,
	const ASL::String& inNewName,
	const ASL::String& inOldName)
{
	ISRPrimaryClipRef  primaryClip = PL::SRProject::GetInstance()->GetPrimaryClip(inMediaPath);
	if (primaryClip)
	{
		UIF::MultiContextHandler::ContextID contextID = kSequenceContextID_NULL;
		ISRLoggingClipRef loggingClip(primaryClip);
		ISRRoughCutRef roughCut(primaryClip);
		if (loggingClip)
		{
			contextID = kSequenceContextID_Clip;
		}
		else if (roughCut)
		{
			contextID = kSequenceContextID_RoughCut;
		}

		if (contextID != kSequenceContextID_NULL)
		{
			SequenceContextInfoPtr sequenceContextInfo = mSequenceContextInfoMap[contextID];
			DVA_ASSERT(sequenceContextInfo);

			ASL::String itemName = GetAssetName(primaryClip);
			sequenceContextInfo->mName = itemName;

			ASL::StationUtils::BroadcastMessage(
				MZ::kStation_PreludeProject, 
				SequenceContextInfoChanged(contextID));
		}
	}
}

ASL::String SequenceContextInfoSupport::GetAssetName(ISRPrimaryClipRef inPrimaryClip)
{
	if (inPrimaryClip)
	{
		ASL::String name = inPrimaryClip->GetName();
		if (!ModulePicker::IsAutoSaveEnabled() && inPrimaryClip->IsDirty())
			name += ASL_STR("*");
		return name;
	}
	return ASL::String();
}

} // namespace PL
