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

#include "PLBEUtilities.h"

#include "PLProject.h"
#include "PLPendingMediaInfoPool.h"

// ASL
#include "ASLClassFactory.h"

// MZ
#include "MZProject.h"
#include "MZBEUtilities.h"
#include "MZUtilities.h"
#include "MZProjectActions.h"

// BE
#include "BE/Project/IClipProjectItem.h"
#include "BE/Project/ProjectItemType.h"
#include "BE/Content/IMediaContent.h"
#include "BE/Project/IProject.h"
#include "BE/Sequence/ISequence.h"
#include "BE/Sequence/ITrackGroup.h"
#include "BE/Sequence/ITrackItem.h"
#include "BE/Sequence/ITrackItemGroup.h"
#include "BE/Sequence/TrackItemSelectionFactory.h"
#include "BEProjectItem.h"
#include "BE/Core/IProperties.h"
#include "BE/Marker/IMarkers.h"
#include "BE/Marker/IMarkerOwner.h"
#include "BE/Sequence/IClipTrack.h"
#include "BE/Sequence/IClipTrackItem.h"
#include "BE/Clip/IChildClip.h"
#include "EA/Project//IEAProjectItem.h"
#include "EA/Project/IEAProjectItem.h"
#include "EA/Project/IEASetAssetIdentifier.h"

// DVA
#include "dvacore/config/Localizer.h"

namespace PL
{
	namespace BEUtilities
	{
		const dvacore::utility::ImmutableString kTemporalPreludeSequence(dvacore::utility::ImmutableString::CreateStatic("Prelude Temporal Sequence"));

		bool IsTemporalSequence(BE::IProjectItemRef inProjectItem)
		{
			//	Give this sequence an special mark so that EA can ignore this seuqenceItem from server synchronization.
			//	Prelude doesn't support 
			BE::IEAProjectItemRef eaProjectItem(inProjectItem);
			if (eaProjectItem)
			{
				return (eaProjectItem->GetAssetIdentifier() == kTemporalPreludeSequence);
			}

			return false;
		}

		void MarkSequenceTemporal(BE::IProjectItemRef inProjectItem)
		{
			//	Give this sequence an special mark so that EA can ignore this seuqenceItem from server synchronization.
			//	Prelude doesn't support 
			BE::IEASetAssetIdentifierRef eaAssetIdentifier(inProjectItem);
			if (eaAssetIdentifier)
			{
				eaAssetIdentifier->SetAssetIdentifier(BEUtilities::kTemporalPreludeSequence);

				BE::IEAProjectItemRef eaProjectItem(inProjectItem);
				bool flag = false;
				BE::IPropertiesRef props(eaProjectItem);
				props->SetValue(EACL::kPrefsEAAssetValid, flag, BE::kNonPersistent);
			}
		}

		/*
		**
		*/
		BE::IProjectItemRef CreateEmptySequence(
			BE::IProjectRef const& inProject,
			BE::IProjectItemRef const& inContainingBin/* = BE::IProjectItemRef()*/,
			ASL::String const& inSequenceName/* = ASL_STR("Dummy Sequence")*/)
		{
			BE::IProjectItemRef sequenceProjectItem;
			ASL::Rect videoFrameRect;
			videoFrameRect.SetWidth(1920);
			videoFrameRect.SetHeight(1080);
			ASL::Guid guid;

			ASL::UInt32 previewRenderCodec = 0;

			sequenceProjectItem = MZ::ProjectActions::CreateSequence(
				inProject,
				(inContainingBin == BE::IProjectItemRef()) ? inProject->GetRootProjectItem() : inContainingBin,
				dvamediatypes::kFrameRate_VideoNTSC,
				videoFrameRect,
				dvamediatypes::kPixelAspectRatio_Square,
				dvamediatypes::kFieldType_Progressive,
				true,
				false,
				dvamediatypes::kFrameRate_Audio48000,
				MF::kAudioChannelType_Mono,
				1,
				2,
				0,
				0,
				0,
				0,
				0,
				inSequenceName,
				guid,
				dvamediatypes::kTimeDisplay_2997NonDropTimecode,
				dvamediatypes::kTimeDisplay_AudioSamplesTimecode,
				DVA_STR(""),
				previewRenderCodec,
				videoFrameRect,
				MZ::kSequenceAudioTrackRule_Logging,
				MZ::ActionUndoableFlags::kNotUndoable);

			return sequenceProjectItem;
		}


		bool IsMasterClipUsedInSRCore(BE::IMasterClipRef const& inMasterClip)
		{
			if (UIF::IsEAMode()) // fix bug 3712297: no need to check for EA
				return false;

			if (inMasterClip)
			{
				ASL::String const& mediaInstanceString = BE::MasterClipUtils::GetMediaInstanceString(inMasterClip);
				PL::ISRMediaRef srMediaRef = SRProject::GetInstance()->GetSRMedia(mediaInstanceString);
				return srMediaRef != NULL;
			}

			return false;
		}

		void RemoveMasterClipFromCurrentProject(BE::IMasterClipRef const& inMasterClip)
		{
			if ( inMasterClip == NULL || MZ::GetProject() == NULL)
			{
				return;
			}

			if (!UIF::IsEAMode()) // no need to check for EA
			{
				if (MZ::BEUtilities::IsMasterClipUsedInClient(inMasterClip) || IsMasterClipUsedInSRCore(inMasterClip))
				{
					return;
				}
			}

			BE::IProjectRef project = MZ::GetProject();
			BE::IProjectItemContainerRef container = BE::IProjectItemContainerRef(project->GetRootProjectItem());
			BE::ProjectItemVector containedItems;
			container->GetItems(containedItems);
			if (!containedItems.empty())
			{
				BE::ProjectItemVector::iterator iter = containedItems.begin();
				BE::ProjectItemVector::iterator end = containedItems.end();

				for (; iter != end; ++iter)
				{
					BE::ProjectItemType containedItemType = (*iter)->GetType();
					if (containedItemType == BE::kProjectItemType_Clip)
					{
						BE::IClipProjectItemRef clipProjectItem(*iter);
						ASL_ASSERT(clipProjectItem != NULL);

						BE::IMasterClipRef masterClip = clipProjectItem->GetMasterClip();
						if (inMasterClip == masterClip)
						{
							RemoveProjectItem(*iter);
							break;
						}
					}
				}
			}
		}

		void RemoveProjectItem(BE::IProjectItemRef const& inProjectItem)
		{
			if ( inProjectItem == NULL || MZ::GetProject() == NULL)
			{
				return;
			}

			DVA_ASSERT_MSG(inProjectItem->GetParent() != NULL, "Remove project item again?");
			if (inProjectItem->GetParent() == NULL)
			{
				return;
			}

			BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(inProjectItem);
			if (MZ::BEUtilities::IsMasterClipUsedInClient(masterClip) || IsMasterClipUsedInSRCore(masterClip))
			{
				return;
			}

			BE::IProjectItemSelectionRef selection(ASL::CreateClassInstanceRef(BE::kProjectItemSelectionClassID));
			selection->AddItem(inProjectItem);
			MZ::Project(MZ::GetProject()).DeleteItems(selection, false);
		}

		/*
		**
		*/
		BE::IMasterClipRef ImportFile(
						const ASL::String& inClipPath, 
						const BE::IProjectRef& inProject,
						bool inNeedErrorMsg,
						bool inIgnoreAudio,
						bool inIgnoreMetadata)
		{
			BE::IMasterClipRef masterClip;	
			MZ::ImportResultVector failureVector;
		
			masterClip = ImportFile(inClipPath, inProject, failureVector, inIgnoreAudio, inIgnoreMetadata);

			if (!failureVector.empty() && inNeedErrorMsg)
			{
				// Send a warning to the events window when substituting
				ASL::String finalWarning = 
					dvacore::ZString("$$$/Prelude/MZ/PreludeImportFileError=Import File Error");
				finalWarning += DVA_STR(": ");
				finalWarning += failureVector.front().second;
				ML::SDKErrors::SetSDKWarningString(finalWarning);
			}

			return masterClip;
		}

		/*
		**
		*/
		BE::IMasterClipRef ImportFile(
						const ASL::String& inClipPath, 
						const BE::IProjectRef& inProject,
						MZ::ImportResultVector& outFailureVector,
						bool inIgnoreAudio,
						bool inIgnoreMetadata)
		{

			if (!ASL::PathUtils::ExistsOnDisk(inClipPath))
			{
				ASL::String doesNotExistWarning = 
					dvacore::ZString("$$$/Prelude/MZ/PreludeImportFileOfflineWarning=@0 does not exist and can not be imported to the project.", inClipPath);
				outFailureVector.push_back(make_pair(ASL::ResultFlags::kResultTypeFailure, doesNotExistWarning));
				return BE::IMasterClipRef();
			}
			
			if (MZ::Utilities::IsEAMediaPath(inClipPath))
			{
				DVA_ASSERT_MSG(0, "we should not get here. In EA mode, we should never import again.");
				return BE::IMasterClipRef();
			}

			BE::IMasterClipRef masterClip;	

			BE::ProjectItemVector projectItemVector;
			if ( MZ::BEUtilities::CreateProjectItem(inClipPath, projectItemVector, outFailureVector, inIgnoreAudio, inIgnoreMetadata) )
			{
				if (!projectItemVector.empty())
				{
					if (inProject)
					{
						MZ::ProjectActions::AddImportedFilesToProject(
							inProject, 
							inProject->GetRootProjectItem(), 
							ASL::Guid(), 
							projectItemVector);
					}
					else
					{
						BOOST_FOREACH(BE::IProjectItemRef projectItem, projectItemVector)
						{
							BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);
							BE::IMediaInfoRef mediaInfo = BE::MasterClipUtils::GetDeprecatedMedia(masterClip)->GetMediaInfo();

							if (mediaInfo)
							{
								RegisterPendingMediaInfo(mediaInfo);
							}
						}
						
					}
				}

				BE::MasterClipVector masterClipVector;
				MZ::BEUtilities::GetMasterClipsFromProject(projectItemVector, masterClipVector);

				if (masterClipVector.size() == 1)
				{
					masterClip = masterClipVector.front();
				}
			}
			return masterClip;
		}

		/*
		**
		*/
		BE::IMasterClipRef GetTheFirstMasterClipFromSequence(
			const BE::IMasterClipRef& inSequenceMasterClip,
			BE::MediaType inMediaType)
		{
			BE::IMasterClipRef masterClip;

			BE::ISequenceRef theSequence = BE::MasterClipUtils::GetSequence(inSequenceMasterClip);
			if (theSequence)
			{
				BE::ITrackGroupRef trackGroup;
				if (inMediaType == BE::kMediaType_Any)
				{
					trackGroup = theSequence->GetTrackGroup(BE::kMediaType_Video);

					if (!trackGroup)
					{
						trackGroup = theSequence->GetTrackGroup(BE::kMediaType_Audio);
					}
				}
				else
				{
					trackGroup = theSequence->GetTrackGroup(inMediaType);
				}

				if (trackGroup != NULL)
				{
					BE::IClipTrackRef clipTrack = trackGroup->GetClipTrack(0);
					if (clipTrack != NULL)
					{
						BE::ITrackItemRef trackItem = clipTrack->GetTrackItem(BE::kTrackItemType_Clip, dvamediatypes::kTime_Zero);
						if (trackItem != NULL)
						{
							BE::IClipTrackItemRef clipTrackItem(trackItem);
							if (clipTrackItem)
							{
								BE::IChildClipRef childClip(clipTrackItem->GetChildClip());
								if (childClip)
								{
									masterClip = childClip->GetMasterClip();
								}
							}
						}
					}
				}
			}

			return masterClip;
		}

		/*
		**
		*/
		bool HasTrackItem(
			const BE::ITrackItemGroupRef& inTrackItemSelection,
			BE::MediaType inMediaType)
		{
			BE::ITrackItemGroupRef group(inTrackItemSelection);

			BE::TrackItemVector videoTrackItems;
			group->GetItems(BE::kMediaType_Video, videoTrackItems);
			BE::TrackItemVector audioTrackItems;
			group->GetItems(BE::kMediaType_Audio, audioTrackItems);

			if (inMediaType == BE::kMediaType_Video)
			{
				return (!videoTrackItems.empty());
			}
			else if (inMediaType == BE::kMediaType_Audio)
			{
				return (!audioTrackItems.empty());
			}
			else	//Any
			{
				return (!videoTrackItems.empty() || !audioTrackItems.empty());
			}
		}

		/*
		**
		*/
		BE::ITrackItemRef GetLinkedFirstTrackItem(
			const BE::ISequenceRef& inSequence, 
			const BE::ITrackItemRef& inTrackItem, 
			BE::MediaType inMediaType)
		{
			if (!inSequence || !inTrackItem || inTrackItem->GetType() != BE::kTrackItemType_Clip)
			{
				return BE::ITrackItemRef();
			}

			BE::ITrackItemGroupRef theLink = inSequence->FindLink(inTrackItem);
			if (theLink)
			{
				BE::TrackItemVector items;
				if (inMediaType == BE::kMediaType_Any)
				{
					// always get from track 0, that is the "first"
					theLink->GetItems(BE::kMediaType_Video, 0, items);
					if (!items.empty())
					{
						return items[0];
					}

					theLink->GetItems(BE::kMediaType_Audio, 0, items);
					return items.empty() ? BE::ITrackItemRef() : items[0];
				}
				else
				{
					theLink->GetItems(inMediaType, 0, items);
					return items.empty() ? BE::ITrackItemRef() : items[0];
				}
			}

			// if we don't have a link, this might be a single track item
			return (inTrackItem->GetMediaType() == inMediaType || inMediaType == BE::kMediaType_Any) ?
					inTrackItem :
					BE::ITrackItemRef();
		}
	}
}