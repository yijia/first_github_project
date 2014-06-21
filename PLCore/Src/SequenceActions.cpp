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

//	PL
#include "PLSequenceActions.h"
#include "PLUtilitiesPrivate.h"
#include "SequenceActions.h"

//	MZ
#include "MZMasterClip.h"
#include "MZBEUtilities.h"
#include "MZSequenceActions.h"
#include "MZSequenceUtils.h"
#include "MZProject.h"
#include "MZSequence.h"
#include "MZAction.h"
#include "MZClipItemActions.h"
#include "MZSequenceActions.h"
#include "MZTimecodeOverlay.h"
#include "MZApplicationAudio.h"

//	PL
#include "PLProject.h"
#include "IPLMarkers.h"
#include "PLBEUtilities.h"


//	PRM
#include "PRMApplicationTarget.h"

//	BE
#include "BEMasterClip.h"
#include "BESequenceEditor.h"
#include "BESequence.h"
#include "BE/Clip/IChildClip.h"
#include "BENotification.h"
#include "BETrack.h"
#include "BEClip.h"
#include "BE/Core/IUniqueSerializeable.h"
#include "BEProject.h"
#include "BEProjectItem.h"
#include "BEBackend.h"
#include "BEProperties.h"
#include "BEMasterClipFactory.h"
#include "Filter/IVideoFilterFactory.h"
#include "Filter/IAudioFilterFactory.h"
#include "BE/Component/IComponent.h"
#include "BE/Component/IComponentOwner.h"
#include "BE/Component/IComponentChain.h"
#include "BE/Component/ComponentFactory.h"
#include "BE/Component/IComponentChainEditor.h"
#include "BE/Component/IComponentParamEditor.h"
#include "BE/Audio/IAudioRenderable.h"
#include "BEActionImpl.h"
#include "BEUndoable.h"

//	ML
#include "MLAudioFilter.h"
#include "MLInternalPluginMatchnameConstants.h"
#include "Transition/IVideoTransitionFactory.h"
#include "Transition/IVideoTransitionModule.h"
#include "Transition/IAudioTransitionFactory.h"
#include "Transition/IAudioTransitionModule.h"

//	ASL
#include "ASLAlgorithm.h"
#include "ASLClassFactory.h"
#include "ASLDirectories.h"
#include "ASLDirectory.h"
#include "ASLDirectoryRegistry.h"
#include "ASLPathUtils.h"
#include "ASLStationUtils.h"
#include "ASLStringCompare.h"
#include "ASLLimits.h"

// dvacore
#include "dvacore/config/Localizer.h"

#include "PhotoshopServer.h"

namespace PL
{

	namespace
	{

		BE::IActionRef CreateMakeMasterClipMonoAction(BE::IMasterClipRef inMasterClip)
		{
			ASL::UInt32 numAudioClips = inMasterClip->GetClipCount(BE::kMediaType_Audio);
			BE::IMasterClip::ClipChannelGroupVector clipChannelGroupVector;
			for (ASL::UInt32 clipIdx = 0; clipIdx < numAudioClips; ++clipIdx)
			{
				if (BE::IAudioClipRef audioClip = BE::IAudioClipRef(inMasterClip->GetClip(BE::kMediaType_Audio, clipIdx)))
				{
					MF::AudioChannelType clipChannelType = audioClip->GetChannelType();
					ASL::UInt32 numChannels = MF::GetNumChannelsFromType(clipChannelType);

					for (ASL::UInt32 channel = 0; channel < numChannels; ++channel)
					{
						BE::IMasterClip::ClipChannelGroup clipChannelGroup;
						BE::IMasterClip::ClipChannel clipChannel;
						clipChannel.mSourceClipIndex = clipIdx;
						clipChannel.mSourceChannelIndex = channel;
						clipChannelGroup.mChannelSources.push_back(clipChannel);
						clipChannelGroup.mChannelType = MF::kAudioChannelType_Mono;
						clipChannelGroupVector.push_back(clipChannelGroup);
					}
				}
			}

			if (!clipChannelGroupVector.empty())
			{
				return inMasterClip->CreateSetAudioClipChannelGroupsAction(
					clipChannelGroupVector,
					ASL::Guid(),
					BE::IAudioChannelsPresetRef(),
					false);
			}

			return BE::IActionRef();
		}

		class AddItemsAction
		{
			class AddItemToSelection :
				public BE::IUndoable
			{
				typedef ASL::InterfaceRef<AddItemToSelection, BE::IUndoable> AddItemToSelectionRef;
				ASL_BASIC_CLASS_SUPPORT(AddItemToSelection);
				ASL_QUERY_MAP_BEGIN
					ASL_QUERY_ENTRY(IUndoable)
					ASL_QUERY_MAP_END

			public:
				static BE::IUndoableRef Create(BE::ITrackItemSelectionRef inSelection, BE::ITrackItemRef inTrackItem)
				{
					AddItemToSelectionRef addItem(CreateClassRef());
					addItem->mSelection = inSelection;
					addItem->mTrackItem = inTrackItem;
					return BE::IUndoableRef(addItem);
				}

				AddItemToSelection()
				{
				}

				virtual ~AddItemToSelection()
				{

				}

				virtual void Do()
				{
					mSelection->AddItem(mTrackItem);
				}

				virtual void Undo()
				{
					mSelection->RemoveItem(mTrackItem);
				}

				virtual bool IsSafeForPlayback() const
				{
					return false;
				}

			private:
				BE::ITrackItemSelectionRef	mSelection;
				BE::ITrackItemRef			mTrackItem;
			};

		public:
			AddItemsAction()
			{

			}

			AddItemsAction(
				BE::ISequenceRef inSequence,
				BE::ITrackItemSelectionRef inSelection,
				const dvamediatypes::TickTime& inTimeOffset,
				BE::TrackDelta inVideoTrackOffset,
				BE::TrackDelta inAudioTrackOffset,
				bool inInsert,
				bool inInsertToFit,
				bool inLimitedShift,
				bool inAlignToVideo,
				BE::IActionRef inPreAction)
				:
			mSequence(inSequence),
				mTrackItemSelection(inSelection),
				mTimeOffset(inTimeOffset),
				mVideoTrackOffset(inVideoTrackOffset),
				mAudioTrackOffset(inAudioTrackOffset),
				mInsert(inInsert),
				mInsertToFit(inInsertToFit),
				mLimitedShift(inLimitedShift),
				mAlignToVideo(inAlignToVideo),
				mPreAction(inPreAction)
			{

			}

			void Commit(BE::ITransactionRef inTransaction)
			{
				inTransaction->Append(MZ::SequenceActions::CreateAddItemsInternalAction(
					mSequence,
					mTrackItemSelection,
					mTimeOffset,
					mVideoTrackOffset,
					mAudioTrackOffset,
					mInsert,
					mInsertToFit,
					mLimitedShift,
					mAlignToVideo,
					mPreAction));

				BE::ITrackItemGroupRef group(mTrackItemSelection);
				BE::TrackItemVector videoTrackItems;
				group->GetItems(BE::kMediaType_Video, videoTrackItems);

				BE::TrackItemVector::iterator iter(videoTrackItems.begin());
				for ( ; iter != videoTrackItems.end(); ++iter)
				{
					if ((*iter)->GetType() == BE::kTrackItemType_Clip)
					{
						BE::IClipTrackItemRef clipTrackItem(*iter);
						BE::IChildClipRef subClip(clipTrackItem->GetChildClip());
						BE::IMasterClipRef masterClip = subClip->GetMasterClip();

						BE::ISequenceRef audioSequence = BE::MasterClipUtils::GetSequence(masterClip);
						if (audioSequence)
						{
							BE::ITrackItemRef nestedTrackItem = MZ::GetFirstNestedAudioTrackItem(*iter);

							BE::IClipTrackItemRef nestedClipTrackItem(nestedTrackItem);
							BE::IChildClipRef nestedSubClip(nestedClipTrackItem->GetChildClip());
							BE::IMasterClipRef nestedMasterClip = nestedSubClip->GetMasterClip();

							// now we know we are an audio clip.
							// remove all of the audio track items
							// replace with the tracks from the master clip instead.
							// create a selection with the video track item (this is from the nested sequence)
							BE::ITrackItemSelectionRef selection = BE::TrackItemSelectionFactory::CreateSelection();
							selection->AddItem(*iter);

							// now unlink the track items that are part of the nested sequence
							inTransaction->Append(MZ::SequenceActions::CreateUnlinkItemsAction(mSequence, selection));

							// set the nest  master clip in/out point before it is added into sequence 
							{
								dvamediatypes::TickTime inPoint = subClip->GetClip()->GetInPoint();
								BE::IActionRef setInpointAction;
								setInpointAction = nestedMasterClip->CreateSetInPointAction(
									BE::kMediaType_Any, 
									inPoint);
								MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(nestedMasterClip), setInpointAction, false);

								dvamediatypes::TickTime outPoint = subClip->GetClip()->GetOutPoint();
								BE::IActionRef setOutpointAction;
								setOutpointAction = nestedMasterClip->CreateSetOutPointAction(
									BE::kMediaType_Any,
									outPoint);
								MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(nestedMasterClip), setOutpointAction, false);
							}

							// create a new selection based on the audio master clip
							BE::ISourceFactoryRef previewFactory(ASL::CreateClassInstanceRef(kAudioSourceFactoryClassID));
							ASL_ASSERT(previewFactory != NULL);
							// get the alignment frame rate
							dvamediatypes::FrameRate alignmentFrameRate(mSequence->GetTrackGroup(BE::kMediaType_Video)->GetFrameRate());
							//	Create a track item selection for the master clip.

							BE::IMasterClip::ClipChannelGroupVector nativeChannelGroupVector;
							nestedMasterClip->GetAudioClipChannelGroups(nativeChannelGroupVector);
							PL::SequenceActions::MakeMasterClipMono(nestedMasterClip);
							BE::ITrackItemSelectionRef trackItemSelection(BE::TrackItemSelectionFactory::CreateSelection(nestedMasterClip,
								alignmentFrameRate,
								previewFactory,
								false));

							// add the audio items in the same location as the audio sequence was
							// using overlay so we will just overwrite the existing audio track items which we don't want.
							inTransaction->Append(MZ::SequenceActions::CreateAddItemsInternalAction(
								mSequence, 
								trackItemSelection, 
								(*iter)->GetStart(), 
								0, 0, 
								false, false, false, true, 
								BE::IActionRef()));

							// When creating a selection, the items will be linked.
							// If we add this selection to sequence, only the linked item will be added.
							// So we have to link the video and audio trackitems manually after add to sequence.
							BE::ITrackItemGroupRef trackItemGroup(trackItemSelection);
							if (trackItemGroup->GetItemCount() > 1)
							{
								BE::IActionRef unlinkAction = MZ::SequenceActions::CreateUnlinkItemsAction(mSequence, trackItemSelection);
								if (unlinkAction)
								{
									inTransaction->Append(unlinkAction);
								}
							}
							// add the video track item from the audio sequence to our new selection
							inTransaction->Append(AddItemToSelection::Create(trackItemSelection, *iter));
							// and link them up
							{
								BE::IActionRef linkAction = MZ::SequenceActions::CreateLinkItemsAction(mSequence, trackItemSelection);
								if (linkAction)
								{
									inTransaction->Append(linkAction);
								}
								else
								{
									DVA_ASSERT_MSG(0, "There should be at least two trackitem here!");
								}
							}

							// now what we should have is:
							// V - from the nested sequence
							// A1-An from the audio clip itself.
							// The main sequence will have the appropriate number of audio tracks for the audio file
							// and we are no longer limited by the master track type of the nested sequence.

							// Remove in/out point from the MasterClip after it is added to sequence.
							{
								BE::IActionRef clearInpointAction = nestedMasterClip->CreateClearInPointAction(BE::kMediaType_Any);
								MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(nestedMasterClip), clearInpointAction, false);

								BE::IActionRef clearOutpointAction = nestedMasterClip->CreateClearOutPointAction(BE::kMediaType_Any);
								MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(nestedMasterClip), clearOutpointAction, false);

								MZ::ExecuteActionWithoutUndo(
									BE::IExecutorRef(nestedMasterClip),
									nestedMasterClip->CreateSetAudioClipChannelGroupsAction(nativeChannelGroupVector, ASL::Guid(), BE::IAudioChannelsPresetRef(), false),
									false);
							}
						}
					}
				}
			}

		private:
			BE::ISequenceRef			mSequence;
			BE::ITrackItemSelectionRef	mTrackItemSelection;
			dvamediatypes::TickTime		mTimeOffset;
			BE::TrackDelta				mVideoTrackOffset;
			BE::TrackDelta				mAudioTrackOffset;
			bool						mInsert;
			bool						mInsertToFit;
			bool						mLimitedShift;
			bool						mAlignToVideo;
			BE::IActionRef				mPreAction;
		};

		/*
		**
		*/
		void TrimAudioOnlyItems(
			BE::ISequenceRef inSequence,
			BE::ITrackItemSelectionRef inSelection,
			BE::ISequenceRef inNestedSequence,
			BE::ITrackItemSelectionRef inNestedSelection,
			BE::SequenceTrimType inSideToTrim,
			dvamediatypes::TickTime inTimeOffset,
			bool inRipple,
			bool inRippleToFit,
			bool inForceRippleNextItem,
			bool inRateStretch,
			bool inAlignToVideo,
			bool inIsUndoable)
		{
			// check if there is anything to do?
			if (inTimeOffset == dvamediatypes::kTime_Zero)
			{
				return;
			}

			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));

			BE::ISequenceEditorRef nestedSequenceEditor(inNestedSequence);
			BE::IActionRef nestedAction = nestedSequenceEditor->CreateTrimItemsAction(
				inNestedSelection,
				inSideToTrim,
				inTimeOffset,
				inRipple,
				inRippleToFit,
				inForceRippleNextItem,
				false,	// inRippleWithoutShifting
				inRateStretch,
				BE::kMediaType_Audio);
			compoundAction->AddAction(nestedAction);

			BE::ISequenceEditorRef sequenceEditor(inSequence);
			BE::IActionRef action = sequenceEditor->CreateTrimItemsAction(
				inSelection,
				inSideToTrim,
				inTimeOffset,
				inRipple,
				inRippleToFit,
				inForceRippleNextItem,
				false,	// inRippleWithoutShifting
				inRateStretch,
				inAlignToVideo ? 
				BE::kMediaType_Video :
			BE::kMediaType_Audio);
			compoundAction->AddAction(action);

			ASL::String string;
			if (inRateStretch)
			{
				string = dvacore::ZString("$$$/Prelude/PL/SequenceActions/RateStretchAction=Rate Stretch");
			}
			else if (inRipple)
			{
				string = dvacore::ZString("$$$/Prelude/PL/SequenceActions/RippleEditAction=Ripple Trim");
			}
			else
			{
				string = dvacore::ZString("$$$/Prelude/PL/SequenceActions/TrimAction=Trim");
			}

			if (inIsUndoable)
			{
				MZ::ExecuteAction(BE::IExecutorRef(inSequence), 
					BE::IActionRef(compoundAction),
					MZ::SequenceActions::kAction_TrimItems,
					string);
			}
			else
			{
				MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), 
					BE::IActionRef(compoundAction),
					true);
			}
		}

		/*
		**
		*/
		void MapClipItemWithTrackItem(
			BE::ITrackItemSelectionRef inTrackItemSelection,
			PL::SRClipItemPtr ioCWSubclip)
		{
			BE::ITrackItemGroupRef group(inTrackItemSelection);

			BE::TrackItemVector videoTrackItems;
			group->GetItems(BE::kMediaType_Video, videoTrackItems);
			BE::TrackItemVector audioTrackItems;
			group->GetItems(BE::kMediaType_Audio, audioTrackItems);

			DVA_ASSERT(videoTrackItems.size() == 1 || audioTrackItems.size() > 0);

			ioCWSubclip->SetTrackItem(videoTrackItems.size() > 0 ?
										videoTrackItems[0] :
										audioTrackItems[0]);
		}

		/*
		**
		*/
		bool ItemHasIntrinsics(
			BE::ITrackItemRef inTrackItem)
		{
			BE::IComponentOwnerRef componentOwner(inTrackItem);
			if (componentOwner == NULL)
			{
				return false;
			}

			BE::IComponentChainRef componentChain(componentOwner->GetComponentChain());
			BE::ComponentVector components;
			componentChain->GetComponents(components);
			BE::ComponentVector::iterator end(components.end());
			for (BE::ComponentVector::iterator it(components.begin()); it != end; ++it)
			{
				if ((*it)->IsIntrinsic())
				{
					return true;
				}
			}

			return false;
		}

		/*
		**
		*/
		void AddIntrinsicFilters(
			BE::ISequenceRef inSequence,
			BE::ITrackItemSelectionRef inSelection)
		{
			ASL_REQUIRE(inSelection != NULL);

			// Create a compound to add all the actions
			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));

			BE::ITrackItemGroupRef trackItemGroup(inSelection);
			ASL_ASSERT(trackItemGroup != NULL);

			// Handle the video filters
			BE::MediaType ourMediaType = BE::kMediaType_Video;
			BE::TrackItemVector trackItemVector;
			trackItemGroup->GetItems(BE::kTrackItemType_Clip, ourMediaType, trackItemVector);
			BE::TrackItemVector::const_iterator iter, endIter = trackItemVector.end();
			for (iter = trackItemVector.begin() ; iter != endIter ; ++iter )
			{
				if (!ItemHasIntrinsics(*iter))
				{
					compoundAction->AddAction(PL::SequenceActions::CreateAddVideoIntrinsicsAction(inSequence, *iter));
				}
			}

			// Handle the audio filters
			ourMediaType = BE::kMediaType_Audio;
			trackItemVector.clear();
			trackItemGroup->GetItems(BE::kTrackItemType_Clip, ourMediaType, trackItemVector);
			endIter = trackItemVector.end();
			for (iter = trackItemVector.begin() ; iter != endIter ; ++iter )
			{
				if (!ItemHasIntrinsics(*iter))
				{
					compoundAction->AddAction(PL::SequenceActions::CreateAddAudioIntrinsicsAction(*iter));
				}
			}

			// Add them all
			if (!compoundAction->Empty())
			{
				MZ::ExecuteActionWithoutUndo(	
					BE::IExecutorRef(trackItemGroup),
					BE::IActionRef(compoundAction),
					false);
			}
		}
	}


	namespace SequenceActions
	{

		/*
		**
		*/
		BE::IActionRef CreateAddAudioIntrinsicsAction(
			BE::ITrackItemRef inTrackItem)
		{
			//	We'll create a compound action to add the intrinsics.
			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));

			// Get the audio channel type
			BE::IAudioRenderableRef audioSource(inTrackItem);
			ASL_ASSERT(audioSource != NULL);
			MF::AudioChannelType audioChannelType = audioSource->GetChannelType();
			dvamediatypes::FrameRate audioFrameRate = audioSource->GetRenderFrameRate();

			// Create an audio factory
			ML::IAudioFilterFactoryRef audioFactory(ASL::CreateClassInstanceRef(kAudioFilterFactoryClassID));
			ASL_ASSERT(audioFactory != NULL);

			// Determine the match name
			ASL::ImmutableString	matchName;
			switch (audioChannelType)
			{
			case MF::kAudioChannelType_Multichannel: ASL_ASSERT(!"unhandled case"); break;

			case MF::kAudioChannelType_None: // [TODO] Handle this.
				throw "not handled";
			case MF::kAudioChannelType_Any: // [TODO] Handle this.
				throw "not handled";
			case MF::kAudioChannelType_Mono:
				matchName = ML::kMatchnameVolumeMono;
				break;
			case MF::kAudioChannelType_Stereo:
				matchName = ML::kMatchnameVolumeStereo;
				break;
			case MF::kAudioChannelType_51:
				matchName = ML::kMatchnameVolume51;
				break;
			}

			AR::IAudioFilterRef volumeFilter(audioFactory->CreateAudioFilter(matchName, audioChannelType, audioFrameRate));
			ASL_ASSERT_MSG(volumeFilter != NULL, "Volume filter couldn't be found.  Okay to ignore.");

			if (volumeFilter != NULL)
			{
				// Create a component for the filter
				BE::IComponentRef volumeFilterComponent = BE::ComponentFactory::CreateAudioFilterComponent(volumeFilter, audioChannelType, MF::kAudioChannelMaxLayoutSize, audioFrameRate);
				ASL_ASSERT(volumeFilterComponent != NULL);

				// Set the filter as intrinsic
				volumeFilterComponent->SetIsIntrinsic(true);

				// Set the volume parameter as time varying in Pro, not time varying in Elements
				BE::IComponentParamEditorRef volumeParameter(volumeFilterComponent->GetParam(ML::kVolumeFilter_VolumeIndex));
				ASL_ASSERT(volumeParameter != NULL);
				if (PRM::GetApplicationType() == PRM::kApplicationType_PremiereElements)
				{
					BE::IComponentParamEditorRef bypassParameter(volumeFilterComponent->GetParam(ML::kVolumeFilter_BypassIndex));
					if (bypassParameter != NULL)
					{
						compoundAction->AddAction(bypassParameter->CreateSetTimeVaryingAction(false));
					}
					compoundAction->AddAction(volumeParameter->CreateSetTimeVaryingAction(false));
				}
				else
				{
					compoundAction->AddAction(volumeParameter->CreateSetTimeVaryingAction(true));		
				}

				// Get the component chain editor
				BE::IComponentOwnerRef componentOwner(inTrackItem);
				ASL_ASSERT(componentOwner != NULL);
				BE::IComponentChainEditorRef componentChainEditor(componentOwner->GetComponentChain());

				// Append the component to the chain
				compoundAction->AddAction(componentChainEditor->CreateInsertComponentAction(volumeFilterComponent, 0));
			}

			return BE::IActionRef(compoundAction);
		}


		/*
		**
		*/
		void CreateAddOpacityVideoIntrinsicAction(
			ASL::UInt32				inInsertIndex,
			BE::ISequenceRef		inSequence,
			BE::ITrackItemRef		inTrackItem,
			BE::ICompoundActionRef  inOutCompoundAction)
		{
			ASL_REQUIRE(inTrackItem != NULL);

			// Create a video factory
			ML::IVideoFilterFactoryRef videoFactory(ASL::CreateClassInstanceRef(kVideoFilterFactoryClassID));
			ASL_ASSERT(videoFactory != NULL);

			// Get the component chain editor
			BE::IComponentOwnerRef componentOwner(inTrackItem);
			ASL_ASSERT(componentOwner != NULL);
			BE::IComponentChainEditorRef componentChainEditor(componentOwner->GetComponentChain());
			ASL_ASSERT(componentChainEditor != NULL);

			// Create the opacity filter
			BE::IVideoFilterRef opacityVideoFilter(videoFactory->CreateVideoFilter(ML::kMatchnameOpacity, BE::kVideoFilterType_AfterEffects));
			ASL_ASSERT_MSG(opacityVideoFilter != NULL, "Opacity filter couldn't be found.  Okay to ignore.");

			if (opacityVideoFilter != NULL)
			{
				// Create a component for the opacity filter
				BE::IComponentRef opacityComponent = BE::ComponentFactory::CreateVideoFilterComponent(opacityVideoFilter);
				ASL_ASSERT(opacityComponent != NULL);

				// Set the opacity filter as intrinsic
				opacityComponent->SetIsIntrinsic(true);

				// Set the opacity parameter as time varying in Pro, not time varying in Elements
				BE::IComponentParamEditorRef opacityParameter(opacityComponent->GetParam(ML::kOpacityFilter_OpacityIndex));
				ASL_ASSERT(opacityParameter != NULL);
				if (PRM::GetApplicationType() == PRM::kApplicationType_PremiereElements)
				{
					inOutCompoundAction->AddAction(opacityParameter->CreateSetTimeVaryingAction(false));
				}
				else
				{
					inOutCompoundAction->AddAction(opacityParameter->CreateSetTimeVaryingAction(true));		
				}

				// add blend mode if applicable
				BE::IClipTrackItemRef clipTrackItem = BE::IClipTrackItemRef(inTrackItem);
				BE::IChildClipRef childClip =
					clipTrackItem
					? clipTrackItem->GetChildClip()
					: BE::IChildClipRef();
				BE::IMasterClipRef masterClip =
					childClip
					? childClip->GetMasterClip()
					: BE::IMasterClipRef();
				if (BE::IMediaRef media =
					masterClip
					? BE::MasterClipUtils::GetDeprecatedMedia(masterClip)
					: BE::IMediaRef())
				{
					// Apply if Photoshop file
					// [TODO] we need a better way to see if it is a Photoshop file
					// Make we have a valid path before continuing; GetExtensionPart requires the input path be 
					// valid, but media->GetInstanceString can return an empty string.
					ASL::String instanceString = media->GetInstanceString();
					if (ASL::PathUtils::IsValidPath(instanceString) && ASL::CaseInsensitive::StringEquals(DVA_STR(".psd"), ASL::PathUtils::GetExtensionPart(media->GetInstanceString())))
					{
						MF::BinaryData data = media->GetMediaOpaqueData();
						if (data.GetSize())
						{
							BE::IComponentParamEditorRef blendModeParameter(opacityComponent->GetParam(ML::kOpacityFilter_BlendModeIndex));
							ASL_ASSERT(blendModeParameter != NULL);

							PhotoshopPrefRec11* prefs = (PhotoshopPrefRec11*)data.GetData();
							size_t layerIndex = media->GetStreamGroup();

							opacityParameter->SetStartValue(
								MF::ParamValue<ASL::Float32>(
								static_cast<ASL::Float32>(prefs->mLayerOpacity[layerIndex])));
							blendModeParameter->SetStartValue(
								MF::ParamValue<ASL::SInt32>(
								static_cast<ASL::SInt32>(prefs->mLayerBlendMode[layerIndex])));
						}
					}
				}

				// Append opacity to the chain
				inOutCompoundAction->AddAction(componentChainEditor->CreateInsertComponentAction(opacityComponent, inInsertIndex));
			}
		}

		/*
		**
		*/
		void CreateAddMotionVideoIntrinsicAction(
			ASL::UInt32				inInsertIndex,
			BE::ISequenceRef		inSequence,
			BE::ITrackItemRef		inTrackItem,
			BE::ICompoundActionRef  inOutCompoundAction)
		{
			ASL_REQUIRE(inTrackItem != NULL);

			// Create a video factory
			ML::IVideoFilterFactoryRef videoFactory(ASL::CreateClassInstanceRef(kVideoFilterFactoryClassID));
			ASL_ASSERT(videoFactory != NULL);

			// Get the component chain editor
			BE::IComponentOwnerRef componentOwner(inTrackItem);
			ASL_ASSERT(componentOwner != NULL);
			BE::IComponentChainEditorRef componentChainEditor(componentOwner->GetComponentChain());
			ASL_ASSERT(componentChainEditor != NULL);

			// Create the motion filter
			BE::IVideoFilterRef motionVideoFilter(videoFactory->CreateVideoFilter(ML::kMatchnameMotion, BE::kVideoFilterType_AfterEffects));
			ASL_ASSERT_MSG(motionVideoFilter != NULL, "Motion filter couldn't be found.  Okay to ignore.");

			if (motionVideoFilter != NULL)
			{
				// Create a component for the motion filter
				BE::IComponentRef motionComponent = BE::ComponentFactory::CreateVideoFilterComponent(motionVideoFilter);
				ASL_ASSERT(motionComponent != NULL);

				// Set the motion filter as intrinsic
				motionComponent->SetIsIntrinsic(true);

				// Append motion to the chain
				inOutCompoundAction->AddAction(componentChainEditor->CreateInsertComponentAction(motionComponent, inInsertIndex));
			}
		}

		/*
		**
		*/
		BE::IActionRef CreateAddVideoIntrinsicsAction(
			BE::ISequenceRef inSequence,
			BE::ITrackItemRef inTrackItem)
		{
			ASL_REQUIRE(inTrackItem != NULL);

			//	We'll create a compound action to add the intrinsics.
			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));

			CreateAddMotionVideoIntrinsicAction(0, inSequence, inTrackItem, compoundAction);
			CreateAddOpacityVideoIntrinsicAction(0, inSequence, inTrackItem, compoundAction);

			return BE::IActionRef(compoundAction);
		}

		/*
		**
		*/
		void TrimItems(
			BE::ISequenceRef inSequence,
			BE::ITrackItemSelectionRef inSelection,
			BE::SequenceTrimType inSideToTrim,
			dvamediatypes::TickTime inTimeOffset,
			bool inRipple,
			bool inRippleToFit,
			bool inForceRippleNextItem,
			bool inRateStretch,
			bool inAlignToVideo,
			bool inIsUndoable)
		{
			// check if there is anything to do?
			if (inTimeOffset == dvamediatypes::kTime_Zero)
			{
				return;
			}

			BE::ISequenceRef nestedSequence = MZ::BEUtilities::GetNestedSequence(inSelection);
			if (nestedSequence)
			{
				// if we can get nested sequence, it's in audio only case
				MZ::SequenceUtils::SelectAllClips(nestedSequence);
				BE::ITrackItemSelectionRef trimSelection = nestedSequence->GetCurrentSelection();

				TrimAudioOnlyItems(
					inSequence,
					inSelection,
					nestedSequence,
					trimSelection,
					inSideToTrim,
					inTimeOffset,
					inRipple,
					inRippleToFit,
					inForceRippleNextItem,
					inRateStretch,
					inAlignToVideo,
					inIsUndoable);
			}
			else
			{
				MZ::SequenceActions::TrimItems(
					inSequence,
					inSelection,
					inSideToTrim,
					inTimeOffset,
					inRipple,
					inRippleToFit,
					inForceRippleNextItem,
					inRateStretch,
					inAlignToVideo,
					inIsUndoable);
			}
		}



		/*
		**	Construct a temporary "floating " selection group of track items from the list of master clips.
		*/
		bool ConstructSelectionFromMasterClips(
			BE::MasterClipVector* inMasterClipVector,
			ASL::UInt32 inAudioFrameAlignment,
			bool inTakeVideo,
			bool inTakeAudio,
			BE::ITrackItemSelectionRef& outSelection,
			BE::ISequenceRef inSequence)
		{
			//	Create a preview factory object, which will allow the
			//	sequence to request preview files when necessary.
			BE::ISourceFactoryRef previewFactory(ASL::CreateClassInstanceRef(kAudioSourceFactoryClassID));
			ASL_ASSERT(previewFactory != NULL);

			dvamediatypes::FrameRate frameAlignmentRate;
			dvamediatypes::FrameRate audioFrameAlignmentRate;
			bool frameRateSet = false;
			// check if there is any video in the clips,
			// since must align all to video even if caller asked for audio alignment
			bool hasVideo = false;
			bool hasAudio = false;

			BE::MasterClipVector selectionMasterClips;

			BE::MasterClipVector::iterator iterEnd = (*inMasterClipVector).end();
			for (BE::MasterClipVector::iterator iter = (*inMasterClipVector).begin(); iter != iterEnd; ++iter)
			{
				BE::IMasterClipRef masterClip(*iter);
				BE::ISequenceRef sequenceFromMasterClip(BE::MasterClipUtils::GetSequence(*iter));
				hasVideo = masterClip->ContainsMedia(BE::kMediaType_Video);
				hasAudio = masterClip->ContainsMedia(BE::kMediaType_Audio);

				if (sequenceFromMasterClip != NULL)
				{
					if (inAudioFrameAlignment && !hasVideo)
					{
						frameAlignmentRate = sequenceFromMasterClip->GetAudioFrameRate();
					}
					else
					{
						frameAlignmentRate = sequenceFromMasterClip->GetVideoFrameRate();
					}
				}
				else
				{
					if (hasVideo)
					{
						if (!frameRateSet)
						{
							BE::IVideoClipRef videoClip = BE::IVideoClipRef(masterClip->GetClip(BE::kMediaType_Video, 0));
							BE::IMediaContentRef mediaContent(BE::IClipRef(videoClip)->GetContent());
							BE::IMediaRef media(mediaContent->GetDeprecatedMedia());
							BE::IMediaStreamRef mediaStream(media->GetMediaStream(BE::kMediaType_Video));
							BE::IVideoStreamRef videoStream(mediaStream);
							frameAlignmentRate = videoStream->GetFrameRate();
							frameRateSet = true;
						}

						hasVideo = true;
						// If I break here and there are audio clips later in the selection
						// they will not be processed below and we will have audio that 
						// you cannot "see" in the timeline.
						//break;
					}
					else if (!frameRateSet && hasAudio)
					{
						BE::IAudioClipRef audioClip = BE::IAudioClipRef(masterClip->GetClip(BE::kMediaType_Audio, 0));
						audioFrameAlignmentRate = audioClip->GetFrameRate();
						frameRateSet = true;
					}
				}

				selectionMasterClips.push_back(masterClip);
			}


			// figure out the correct frame alignment framerate
			if (inSequence != NULL)
			{
				if (inAudioFrameAlignment && ! hasVideo)
				{
					frameAlignmentRate = inSequence->GetAudioFrameRate();
				}
				else
				{
					frameAlignmentRate = inSequence->GetVideoFrameRate();
				}
			}
			else
			{
				if ((inAudioFrameAlignment && !hasVideo) || frameAlignmentRate == dvamediatypes::kFrameRate_Zero)
				{
					frameAlignmentRate = audioFrameAlignmentRate;
				}
			}

			if (frameAlignmentRate == dvamediatypes::kFrameRate_Zero)
			{
				// Acquire properties interface from the backend
				BE::IBackendRef backend = BE::GetBackend();
				BE::IPropertiesRef bProp(backend);

				// get timebase for indeterminate media (eg. still images)
				dvamediatypes::FrameRate currentIndeterminateFrameRate;
				if (! bProp->GetValue(BE::kPrefsStillImagesDefaultFramerate, frameAlignmentRate))
				{
					// provide reasonable default in case Prefs haven't been set
					frameAlignmentRate = dvamediatypes::kFrameRate_VideoNTSC;
				}
			}


			outSelection = BE::TrackItemSelectionFactory::CreateSelection(
				selectionMasterClips,
				frameAlignmentRate,
				previewFactory,
				inTakeVideo,
				inTakeAudio,
				inSequence);

			return (outSelection != NULL);
		}

		/*
		**
		*/
		void MakeMasterClipMono(BE::IMasterClipRef ioMasterClip)
		{
			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));
			BE::IActionRef makeMasterClipMonoAction = CreateMakeMasterClipMonoAction(ioMasterClip);

			if (makeMasterClipMonoAction)
			{
				compoundAction->AddAction(makeMasterClipMonoAction);
				MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(ioMasterClip), BE::IActionRef(compoundAction), false);
			}
		}

		/*
		**
		*/
		bool AddClipItemsToSequence(
			BE::ISequenceRef inSequence, 
			SRClipItems& ioClipItems,
			dvamediatypes::TickTime inInsertTime,
			boost::function<void(SRClipItemPtr)> const& inAddToRCClipItemsFxn,
			MZ::ActionUndoableFlags::Type inUndoableType,
			const ASL::String& inActionText,
			dvamediatypes::TickTime* outChangedDuration,
            bool isAudioForceMono)
		{
			bool isSuccess = true;
			SRClipItems successItems;

			BE::IExecutorRef executor(inSequence);
			dvamediatypes::TickTime oldSeqDuration = MZ::SequenceActions::GetEnd(inSequence);

			// [NOTE] Should use reverse iterator to make sure insert operation correct, 
			// especially when insert sub clips into middle of sequence and maybe one of them is inserted failed.
			SRClipItems::reverse_iterator iter = ioClipItems.rbegin();
			SRClipItems::reverse_iterator end = ioClipItems.rend();

			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));
			dvamediatypes::TickTime addedDuration = dvamediatypes::kTime_Zero;
			for (; iter != end; ++iter)
			{
				BE::MasterClipVector masterClipVec;
				bool needSetInOutPoint = true;

				BE::IMasterClipRef masterClip = (*iter)->GetSRMedia()->GetMasterClip();
				// For every masterclip inserted into sequence, we must set its audio mono
				BE::IMasterClip::ClipChannelGroupVector nativeChannelGroupVector;
                
                if (isAudioForceMono)
                {
                    masterClip->GetAudioClipChannelGroups(nativeChannelGroupVector);
                    PL::SequenceActions::MakeMasterClipMono(masterClip);
                }

				if ((*iter)->GetStartTime() == dvamediatypes::kTime_Invalid||
					(*iter)->GetDuration() == dvamediatypes::kTime_Invalid)
				{
					needSetInOutPoint = false;
				}

				dvamediatypes::TickTime originalStart = dvamediatypes::kTime_Invalid;
				dvamediatypes::TickTime originalEnd = dvamediatypes::kTime_Invalid;

				if (needSetInOutPoint)
				{
					// backup the original in/out point, for now, we will restore for image only
					if (MZ::BEUtilities::IsMasterClipStill(masterClip))
					{
						BE::IClipRef clipRef = masterClip->GetClip(BE::kMediaType_Any, 0);
						if (clipRef->IsInPointSet())
							originalStart = clipRef->GetInPoint();
						if (clipRef->IsOutPointSet())
							originalEnd = clipRef->GetOutPoint();
					}

					MZ::ExecuteActionWithoutUndo(
						executor,
						masterClip->CreateSetInPointAction(BE::kMediaType_Any, (*iter)->GetStartTime()),
						false);

					MZ::ExecuteActionWithoutUndo(
						executor,
						masterClip->CreateSetOutPointAction(BE::kMediaType_Any, (*iter)->GetStartTime() + (*iter)->GetDuration()),
						false);
				}

				masterClipVec.push_back(masterClip);

				BE::ITrackItemSelectionRef trackItemSelection;
				isSuccess = PL::SequenceActions::ConstructSelectionFromMasterClips(&masterClipVec,
					1,
					true,
					true,
					trackItemSelection,
					inSequence);

				BE::ITrackItemGroupRef group(trackItemSelection);
				isSuccess &= PL::BEUtilities::HasTrackItem(group, BE::kMediaType_Any);

				if (isSuccess)
				{
					dvamediatypes::TickTime groupDuration  = group->GetEnd() - group->GetStart();
					if (groupDuration + addedDuration + oldSeqDuration > dvamediatypes::TickTime(static_cast<double>(24*3600)))
					{
						isSuccess = false;
					}
					else
					{
						addedDuration += groupDuration;
					}
				}
				if (isSuccess)
				{
					MapClipItemWithTrackItem(trackItemSelection, (*iter));

					// Add the intrinsic filters
					AddIntrinsicFilters(inSequence, trackItemSelection);

					// Tricky: To Rough Cut, must to add the clipitem to SRRoughCut mClipItems before calling MZ::SequenceActions::AddItems
					// AddItems will notify SRMarkerbar to build trick items, in this step, mClipItems are used to be a filter. W#2838621
					if (NULL != inAddToRCClipItemsFxn)
						inAddToRCClipItemsFxn(*iter);

					bool hasVideo = masterClip->ContainsMedia(BE::kMediaType_Video);
					bool hasAudio = masterClip->ContainsMedia(BE::kMediaType_Audio);
					if (!hasVideo && hasAudio)
					{
						// To support undo in rough cut, we must use single action when adding audio clips
						compoundAction->AddAction(BE::CreateAction(AddItemsAction(
							inSequence,
							trackItemSelection,
							inInsertTime,
							0,
							0,
							true,
							false,
							false,
							true,
							BE::IActionRef())));
					}
					else
					{
						compoundAction->AddAction(MZ::SequenceActions::CreateAddItemsInternalAction(
							inSequence,
							trackItemSelection, 
							inInsertTime,
							0,
							0,
							true,
							false,
							false,
							true,
							BE::IActionRef()));
					}

					// Update the subclip name to readable timecode which will be replaced by subclip
					// unique name in future
					// [TODO]

					successItems.insert(successItems.begin(), *iter);
				}

				if (needSetInOutPoint)
				{
					if (MZ::BEUtilities::IsMasterClipStill(masterClip))
					{
						// restore the in/out point for image clip
						if (originalStart != dvamediatypes::kTime_Invalid)
						{
							MZ::ExecuteActionWithoutUndo(
								executor,
								masterClip->CreateSetInPointAction(BE::kMediaType_Any, originalStart),
								false);
						}

						if (originalEnd != dvamediatypes::kTime_Invalid)
						{
							MZ::ExecuteActionWithoutUndo(
								executor,
								masterClip->CreateSetOutPointAction(BE::kMediaType_Any, originalEnd),
								false);
						}
					}
					else
					{
						//if ( (*iter)->GetAssetItem()->GetCustomInPoint() == dvamediatypes::kTime_Invalid )
						{
							MZ::ExecuteActionWithoutUndo(
								executor,
								masterClip->CreateClearInPointAction(BE::kMediaType_Any),
								false);
						}

						//if ( (*iter)->GetAssetItem()->GetCustomOutPoint() == dvamediatypes::kTime_Invalid )
						{
							MZ::ExecuteActionWithoutUndo(
								executor,
								masterClip->CreateClearOutPointAction(BE::kMediaType_Any),
								false);
						}
					}
				}

                if (isAudioForceMono)
                {
                    // set channel group back, because this is the only masterclip used globally.
                    MZ::ExecuteActionWithoutUndo(
                                            executor,
                                            masterClip->CreateSetAudioClipChannelGroupsAction(nativeChannelGroupVector, ASL::Guid(), BE::IAudioChannelsPresetRef(), false),
                                            false);
                }
			}

			if (!compoundAction->Empty())
			{
				if (MZ::ActionUndoableFlags::kUndoable == inUndoableType)
				{
					MZ::ExecuteAction(executor, 
						BE::IActionRef(compoundAction),
						DVA_STR("Append Clips to RoughCut"),
						inActionText);
				}
				else
				{
					MZ::ExecuteActionWithoutUndo(
						executor, 
						BE::IActionRef(compoundAction),
						(inUndoableType == MZ::ActionUndoableFlags::kNotUndoableWantsNotification));
				}


				MZ::ConformSequenceToTimecodeOverlay(inSequence);
				PL::SequenceActions::ConformSequenceToClipSettings(inSequence);
				MZ::SequenceActions::RippleDeleteAllEmptySpace(inSequence);
				MZ::ApplicationAudio::ConformSequenceToAudioSettings(inSequence);
			}

			dvamediatypes::TickTime newSeqDuration = MZ::SequenceActions::GetEnd(inSequence);
			if (outChangedDuration)
			{
				*outChangedDuration = (newSeqDuration - oldSeqDuration);
			}
			ioClipItems.swap(successItems);
			return isSuccess;
		}

		/*
		**
		*/
		bool AddClipItemsToSequenceForExport(
								BE::ISequenceRef inSequence, 
								ExportClipItems& ioExportItems, 
								dvamediatypes::TickTime inInsertTime,
								std::list<BE::ITrackItemSelectionRef>& outAddedTrackItemSelection,
								MZ::ActionUndoableFlags::Type inUndoableType)
		{
			bool isSuccess = true;

			dvamediatypes::TickTime insertTime = inInsertTime;

			ExportClipItems::const_reverse_iterator clipItemIter = ioExportItems.rbegin();
			ExportClipItems::const_reverse_iterator clipItemEnd = ioExportItems.rend();

			BE::IExecutorRef executor(inSequence);
			BE::IActionRef setInpointAction;
			BE::IActionRef setOutpointAction;
			BE::IActionRef clearInpointAction;
			BE::IActionRef clearOutpointAction;
			BE::MasterClipVector masterClipVec;

			for (; clipItemIter != clipItemEnd && isSuccess; ++clipItemIter)
			{
				bool needSetInOutPoint = !MZ::BEUtilities::IsMasterClipStill((*clipItemIter)->GetMasterClip());

				if (needSetInOutPoint)
				{
					dvamediatypes::TickTime duration = (*clipItemIter)->GetDuration();
					if (duration != dvamediatypes::kTime_Zero)
					{
						setInpointAction = (*clipItemIter)->GetMasterClip()->CreateSetInPointAction(
							BE::kMediaType_Any, 
							(*clipItemIter)->GetStartTime());
						MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), setInpointAction, false);

						setOutpointAction = (*clipItemIter)->GetMasterClip()->CreateSetOutPointAction(
							BE::kMediaType_Any,
							(*clipItemIter)->GetStartTime() + duration);
						MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), setOutpointAction, false);
					}
					else
					{
						ASL_ASSERT_MSG(0, "Should not get 0 duration clip!");
						break;
					}
				}

				masterClipVec.push_back((*clipItemIter)->GetMasterClip());

				BE::ITrackItemSelectionRef trackItemSelection;
				isSuccess = PL::SequenceActions::ConstructSelectionFromMasterClips(&masterClipVec,
					1,
					true,
					true,
					trackItemSelection,
					inSequence);

				BE::ITrackItemGroupRef itemGroup(trackItemSelection);
				isSuccess &= (itemGroup->GetItemCount() > 0);

				if (isSuccess)
				{
					outAddedTrackItemSelection.push_back(trackItemSelection);
					MZ::SequenceActions::AddItemsForExport(inSequence, trackItemSelection, insertTime, inUndoableType);

					// Append subclip name for every trackitem
					BE::TrackItemVector trackItems;
					itemGroup->GetItems(trackItems);

					ASL::String trackName = (*clipItemIter)->GetExportName();
					if (!trackName.empty())
					{
						MZ::SequenceActions::RenameTrackItem(inSequence, BE::ITrackItemRef(), itemGroup, trackName, MZ::ActionUndoableFlags::kNotUndoable);
					}
					masterClipVec.clear();
				}

				if (needSetInOutPoint)
				{
					clearInpointAction = (*clipItemIter)->GetMasterClip()->CreateClearInPointAction(BE::kMediaType_Any);
					MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), clearInpointAction, false);

					clearOutpointAction = (*clipItemIter)->GetMasterClip()->CreateClearOutPointAction(BE::kMediaType_Any);
					MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), clearOutpointAction, false);
				}
			}

			return isSuccess;
		}

		/**
		**
		*/
		void ConformSequenceToClipSettings(
			BE::ISequenceRef inSequence)
		{
			BE::ITrackGroupRef videoTrackGroup(inSequence->GetTrackGroup(BE::kMediaType_Video));
			BE::IClipTrackRef videoTrack(videoTrackGroup->GetClipTrack(0));
			BE::TrackItemVector trackItems;
			videoTrack->GetTrackItems(BE::kTrackItemType_Clip, trackItems);

			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));
			BE::TrackItemVector::iterator iter(trackItems.begin());
			for ( ; iter != trackItems.end(); ++iter)
			{
				if ((*iter)->GetType() == BE::kTrackItemType_Clip)
				{
					BE::IChildClipRef childClip = BE::IClipTrackItemRef(*iter)->GetChildClip();
					BE::IVideoClipRef videoClip = BE::IVideoClipRef(childClip->GetClip());
					compoundAction->AddAction(videoClip->CreateSetFrameBlendAction(false));
				}
			}

			MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), BE::IActionRef(compoundAction), true);
		}

		/**
		**
		*/
		void AddTransitionItemsToSequence(
			BE::ISequenceRef inSequence,
			const PL::TrackTransitionMap& inTransitions,
			BE::MediaType inMediaType,
            MZ::ActionUndoableFlags::Type inUndoableType)
		{
			if (inTransitions.empty())
			{
				return;
			}

			if (!inSequence)
			{
				return;
			}

			if (inMediaType != BE::kMediaType_Audio && inMediaType != BE::kMediaType_Video)
			{
				DVA_ASSERT(0);
				return;
			}

			BE::ICompoundActionRef compoundAction(ASL::CreateClassInstanceRef(BE::kCompoundActionClassID));

			BE::ITrackGroupRef trackGroup = inSequence->GetTrackGroup(inMediaType);
			BE::TrackIndex trackCount = trackGroup->GetClipTrackCount();

			for (BE::TrackIndex trackIndex = 0; trackIndex < trackCount; ++trackIndex)
			{
				PL::TrackTransitionMap::const_iterator iter = inTransitions.find(trackIndex);
				if (iter == inTransitions.end())
				{
					// we dont have transition here
					continue;
				}

				const PL::TransitionItemSet& transitionsPerTrack = (*iter).second;
				for (PL::TransitionItemSet::const_iterator transitionIter = transitionsPerTrack.begin();
					transitionIter != transitionsPerTrack.end();
					++transitionIter)
				{
					const PL::TransitionItemPtr& theTransition = *transitionIter;

					ML::ITransitionModuleRef transitionModule;
					if (inMediaType == BE::kMediaType_Video)
					{
						ML::IVideoTransitionFactoryRef videoTransitionFactory(ASL::CreateClassInstanceRef(kVideoTransitionFactoryClassID));
						ML::IVideoTransitionModuleRef videoTransition(videoTransitionFactory->GetVideoTransition(ASL::ImmutableString::CreateDynamic(theTransition->GetEffectMatchName())));
						transitionModule = ML::ITransitionModuleRef(videoTransition);
					}
					else
					{
						ML::IAudioTransitionFactoryRef audioTransitionFactory(ASL::CreateClassInstanceRef(kAudioTransitionFactoryClassID));
						ML::IAudioTransitionModuleRef audioTransition(audioTransitionFactory->GetAudioTransition(ASL::ImmutableString::CreateDynamic(theTransition->GetEffectMatchName())));
						transitionModule = ML::ITransitionModuleRef(audioTransition);
					}

					if (!transitionModule)
					{
						DVA_ASSERT_MSG(0, "Fail to get transition module");
						continue;
					}

					dvamediatypes::TickTime duration = theTransition->GetEnd() - theTransition->GetStart();
					if (duration != dvamediatypes::kTime_Zero)
					{
						double alignment = double(theTransition->GetCutPoint().GetTicks()) / double(duration.GetTicks());
						dvamediatypes::TickTime editLine = theTransition->GetStart();

						BE::IClipTrackRef theClipTrack = trackGroup->GetClipTrack(trackIndex);
						BE::ITrackItemRef theTrackItem = 
							theClipTrack->GetTrackItem(BE::kTrackItemType_Clip, theTransition->GetStart());
						if (theTrackItem && theTrackItem->GetType() == BE::kTrackItemType_Clip)
						{
							if (std::abs(alignment) > 0.01)
							{
								editLine = theTrackItem->GetEnd();
							}
							else
							{
								editLine = theTrackItem->GetStart();
							}
						}

						BE::IActionRef action = MZ::SequenceActions::CreateApplyTransitionAction(
							inSequence,
							BE::ITrackRef(theClipTrack),
							ASLUnknownRef(transitionModule),
							editLine,
							duration,
							ASL::Float32(alignment),
							false,		//inForceSingleSided
							true,		//inAlignToVideo
							false);		//inShowSetupDialog

						if (action)
						{
							compoundAction->AddAction(action);
						}
					}
				}
			}

			if (!compoundAction->Empty())
			{
                if (inUndoableType == MZ::ActionUndoableFlags::kUndoable)
                {
                    MZ::ExecuteAction(BE::IExecutorRef(inSequence),
                                      BE::IActionRef(compoundAction),
                                      DVA_STR("Append transitions to RoughCut"),
                                      DVA_STR("Append transitions to RoughCut"));

                }
                else
                {
                    MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inSequence), BE::IActionRef(compoundAction), inUndoableType == MZ::ActionUndoableFlags::kNotUndoableWantsNotification);
                }
				
			}
		}

	}

} // namespace PL
