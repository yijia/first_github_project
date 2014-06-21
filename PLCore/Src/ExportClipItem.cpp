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

//	Prefix
#include "Prefix.h"

// local
#include "PLExportClipItem.h"
#include "PLFactory.h"
#include "MZBEUtilities.h"
#include "PLBEUtilities.h"
#include "MZAction.h"
#include "PLUtilities.h"

// BE
#include "BE/Project/IProject.h"

// ASL
#include "ASLPathUtils.h"

namespace PL
{
	//Definition of ExportClipItem
	ExportClipItem::ExportClipItem(AssetItemPtr const& inAssetItem)
		:
		mAssetItem(inAssetItem)
	{
		if (!mAssetItem)
		{
			DVA_ASSERT_MSG(0, "NULL AssetItem passed!");
			return;
		}

		ASL::String const& clipPath = inAssetItem->GetMediaPath();
		if (ASL::PathUtils::ExistsOnDisk(clipPath))
		{
			mMasterClip = BEUtilities::ImportFile(
				clipPath,
				BE::IProjectRef(),
				true,	//	inNeedErrorMsg
				false,	//	inIgnoreAudio
				false); //	inIgnoreMetadata

			if (mMasterClip)
			{
				dvamediatypes::TickTime duration = GetDuration();
				dvamediatypes::TickTime start = GetStartTime();

				if (mAssetItem->GetAssetMediaType() == kAssetLibraryType_SubClip)
				{
					// ***For subclip export we need to handle here separately to set soft boundary***
					BE::IActionRef setSoftBoundary = mMasterClip->CreateSetContentBoundariesAction(
						start,
						start + duration,
						false);
					MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(mMasterClip), setSoftBoundary, false);
				}
				else if (MZ::BEUtilities::IsMasterClipStill(mMasterClip))
				{
					// ***For still image, set boundary has not effect, so we need to set in/out***
					if (start != dvamediatypes::kTime_Invalid)
					{
						BE::IActionRef setInAction = mMasterClip->CreateSetInPointAction(BE::kMediaType_Any, start);
						MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(mMasterClip),
							setInAction,
							false);

						if (duration != dvamediatypes::kTime_Invalid)
						{
							BE::IActionRef setOutAction = mMasterClip->CreateSetOutPointAction(BE::kMediaType_Any, start + duration);
							MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(mMasterClip),
								setOutAction,
								false);
						}
					}
				}
			}
		}
		
		if (mMasterClip == NULL)
		{
			mMasterClip = MZ::BEUtilities::CreateOfflineMasterClip(clipPath, true);
		}
	}

	ExportClipItem::ExportClipItem(AssetItemPtr const& inAssetItem, BE::IMasterClipRef const& inMasterClip)
		:
		mAssetItem(inAssetItem),
		mMasterClip(inMasterClip)
	{

	}

	BE::IMasterClipRef ExportClipItem::GetMasterClip() const
	{
		return mMasterClip;
	}

	dvamediatypes::TickTime ExportClipItem::GetStartTime() const
	{
		if (!mAssetItem)
		{
			return dvamediatypes::kTime_Zero;
		}

		const dvamediatypes::TickTime inPoint = mAssetItem->GetInPoint();
		return (inPoint != dvamediatypes::kTime_Invalid) ? 
				inPoint : 
				dvamediatypes::kTime_Zero;
	}

	dvamediatypes::TickTime ExportClipItem::GetDuration() const
	{
		if (!mAssetItem || !mMasterClip)
		{
			return dvamediatypes::kTime_Zero;
		}

		const dvamediatypes::TickTime duration = mAssetItem->GetDuration();
		return (duration != dvamediatypes::kTime_Invalid) ? 
				duration : 
				PL::Utilities::GetMasterClipMaxDuration(mMasterClip);
	}

	ASL::String ExportClipItem::GetAliasName() const
	{
		return mAssetItem != NULL ? mAssetItem->GetAssetName() : ASL::String();
	}

	ASL::String ExportClipItem::GetExportName()
	{
		ASL::String mediaName = GetMasterClip()->GetName();
		ASL::String subclipName = GetAliasName();
		if (!MZ::BEUtilities::IsEAMasterClip(GetMasterClip()))
		{
			return mediaName + ASL_STR(".") + subclipName;
		}
		else
		{
			return subclipName;
		}
	}
}
