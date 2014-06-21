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

#include "ASLString.h"
#include "ASLClassFactory.h"
#include "IncludeXMP.h"
#include "dvatemporalxmp/XMPTemporalMetadataServices.h"
#include "dvamediatypes/PixelFormatUtils.h"

//	SR
#include "PLUtilities.h"
#include "IngestMedia/PLIngestUtils.h"
#include "PLProject.h"
#include "PLModulePicker.h"
#include "PLUtilitiesPrivate.h"
#include "IPLLoggingClip.h"
#include "IPLRoughCut.h"
#include "PLRoughCut.h"
#include "PLFactory.h"
#include "PLMessage.h"
#include "PLPrivateCreator.h"

#include "MZUtilities.h"
#include "MZBEUtilities.h"
#include "MZProject.h"
#include "MZUndo.h"
#include "PLXMLUtilities.h"
#include "MZMasterClip.h"
#include "MZProjectItemHelpers.h"
#include "MZCuePointEditor.h"
#include "MZProjectActions.h"
#include "MZLocale.h"
#include "MZSequencePreviewPresets.h"
#include "MZProjectProperties.h"

#include "PLBEUtilities.h"
#include "PLLibrarySupport.h"
#include "PLMetadataActions.h"
#include "PLKeywordsAccessUtils.h"

//	MC
#include "BEBackend.h"
#include "BEProperties.h"
#include "MFSource.h"
#include "BESource.h"
#include "BEClip.h"
#include "BE/PreferencesConstants.h"
#include "BE/Sequence/TrackItemSelectionFactory.h"
#include "BE/Clip/IMasterClip.h"
#include "BE/Sequence/IClipTrack.h"

#include "BEProjectItem.h"
#include "BESequence.h"
#include "BETrack.h"
#include "BE/Project/Visitor.h"
#include "BEContent.h"
#include "BEMedia.h"
#include "BEMergedClips.h"
#include "BETimeRemapping.h"
#include "BE/Sequence/IVideoTransitionTrackItem.h"

//	ML
#include "SDKErrors.h"
#include "MLMetadataManager.h"
#include "MLCustomMetadata.h"
#include "Player/IPlayer.h"

// UIF
#include "UIFWorkspace.h"
#include "UIFHeadlights.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLStringCompare.h"
#include "ASLCoercion.h"
#include "ASLStationUtils.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLFile.h"
#include "ASLTokenizer.h"
#include "ASLWeakReferenceCapable.h"
#include "ASLAsyncCallFromMainThread.h"

//	DVA
#include "dvacore/config/Localizer.h"
#include "dvametadata/Definitions.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/utility/StringCompare.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/xml/XMLUtils.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/threads/AsyncEventLoopExecutor.h"
#include "dvacore/threads/SharedThreads.h"
#include "dvacore/utility/SmallBlockAllocator.h"

#include "dvametadataui/TextTranscriptEditorView.h"

// dvamediatypes
#include "dvamediatypes/TimeUtils.h"
#include "dvamediatypes/TimeDisplayUtils.h"
#include "dvamediatypes/Timecode.h"

//	boost
#include <boost/foreach.hpp>

//	EAClient
#include "IEACDataServer.h"
#include "IEACProduction.h"
#include "IEACMediaLocator.h"
#include "IEACAsset.h"
#include "EACLSandbox.h"
#include "EACLCurrentSandbox.h"

#include "EA/Project/IEAProjectItem.h"

// XML
#include "XMLUtils.h"

// MediaCore
#include "PrSDKImport.h"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include "boost/filesystem.hpp"

// PRM
#include "PRMApplicationTarget.h"


namespace PL
{
	namespace
	{
		#define kXMP_NS_CONTENT_ANALYSIS					"http://ns.adobe.com/xmp/1.0/ContentAnalysis/"
		const char* kXMP_NS_CONTENT_ANALYSIS_Prefix = "xmpCA";

		#define kXMP_Property_Analyzer_Bag				"analyzers"
		#define kXMP_Property_Analyzer_Type				"type"
		#define kXMP_Property_Analyzer_Version			"version"
		#define kXMP_Property_Analyzer_Manufacturer		"manufacturer"
		#define kXMP_Property_Analyzer_Performed		"performed"

		const ASL::StdString kXMP_Property_HistoryArray     = "History";
		const ASL::StdString kXMP_Property_InstanceID       = "InstanceID";
		const ASL::StdString kXMP_Property_MetadataDate     = "MetadataDate";
		const ASL::StdString kXMP_Property_ModifyDate       = "ModifyDate";
		const ASL::StdString kXMP_Property_OriginalDocumentID  = "OriginalDocumentID";

		const dvacore::UTF16String kCustomMetadataContent	= DVA_STR("CustomMetadataContent");
		const dvacore::UTF16String kDuration				= DVA_STR("Duration");
		const dvacore::UTF16String kDropFrame				= DVA_STR("DropFrame");
		const dvacore::UTF16String kVideoFrameRate			= DVA_STR("VideoFrameRate");
		const dvacore::UTF16String kMediaStart				= DVA_STR("MediaStart");
		const dvacore::UTF16String kIsAudioOnly				= DVA_STR("IsAudioOnly");
		const dvacore::UTF16String kIsStillImage			= DVA_STR("IsStillImage");
		const dvacore::UTF16String kName					= DVA_STR("Name");

		bool sAssetItemsAppending = false;
		const ASL::String kSingleFileFormat = ASL_STR("Single");

#define EXTENSION_CONFIG_TEXT "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" \
	"<ExtensionConfig>\n" \
	"<!-- Native, enable native project panel. Flash, hide native project panel and enable flash library. -->\n"

#define EXTENSION_CONFIG_NATIVE	"<ProjectPanel>Native</ProjectPanel>\n" \
	"</ExtensionConfig>\n"

#define EXTENSION_CONFIG_FLASH	"<ProjectPanel>Flash</ProjectPanel>\n" \
	"</ExtensionConfig>\n"

		const ASL::SInt32 kExtensionConfig_Uninit	= 0;
		const ASL::SInt32 kExtensionConfig_Native	= 1;
		const ASL::SInt32 kExtensionConfig_Flash	= 2;

		static const ASL::String kProjectPanelName = DVA_STR("ProjectPanel");									
		static const ASL::String kPanelTypeFlash = DVA_STR("Flash");
		static const ASL::String kPanelTypeNative = DVA_STR("Native");

		const dvacore::UTF16String kProjectCacheFolderName		= DVA_STR("ProjectCache");

		const dvacore::UTF16String kCreativeCloudDownloadLocationFolderName	= DVA_STR("CreativeCloudCache");

		const dvacore::UTF16String kConfigurationFileFullName = DVA_STR("Configurations.xml");
		const dvacore::UTF16String kConfigIngestDialogNodeName = DVA_STR("IngestDialog");
		const dvacore::UTF16String kConfigDisableIngestSettingsPropertyName = DVA_STR("DisableAllIngestSettings");
		const dvacore::UTF16String kConfigDefaultSubFolderPropertyName = DVA_STR("DefaultSubFolder");

		const size_t kInvalidChannelIndex = (size_t)-1;

		static const dvametadata::Path kTracksPath =
		{
			false,
			dvacore::utility::CharAsUTF8("http://ns.adobe.com/xmp/1.0/DynamicMedia/"),
			dvacore::utility::CharAsUTF8("Tracks"),
			dvacore::utility::CharAsUTF8("Tracks"),
			dvacore::utility::CharAsUTF8("http://ns.adobe.com/xmp/1.0/DynamicMedia/"),
		};

		/*
		**
		*/
		static boost::filesystem::path GetBoostFileSystemPath(
			const dvacore::UTF16String& inFilePath)
		{
			// const boost::filesystem::path::value_type is defined as wchar_t on Windows only, in all other cases it's a char!
#if ASL_TARGET_OS_WIN
			boost::filesystem::path filePath(reinterpret_cast<const boost::filesystem::path::value_type*>(inFilePath.c_str()));
#else
			boost::filesystem::path filePath(reinterpret_cast<const boost::filesystem::path::value_type*>(dvacore::utility::UTF16to8(inFilePath).c_str()));
#endif
			return filePath;
		}

		/*
		**
		*/
		bool IsValidXMPDate(XMP_DateTime const& inDateTime)
		{
			return (inDateTime.hasDate && inDateTime.hasTime);
		}

		/*
		**
		*/
		void RemoveFieldsFromNamespaceMM(SXMPMeta& inXMPMeta)
		{
			inXMPMeta.DeleteProperty(
				kXMP_NS_XMP_MM,
				kXMP_Property_HistoryArray.c_str());

			inXMPMeta.DeleteProperty(
				kXMP_NS_XMP_MM,
				kXMP_Property_InstanceID.c_str());

			inXMPMeta.DeleteProperty(
				kXMP_NS_XMP_MM,
				kXMP_Property_OriginalDocumentID.c_str());
		}

		/*
		**
		*/
		void RemoveFieldsFromNamespaceXMP(SXMPMeta& inXMPMeta)
		{
			inXMPMeta.DeleteProperty(
				kXMP_NS_XMP,
				kXMP_Property_MetadataDate.c_str());

			inXMPMeta.DeleteProperty(
				kXMP_NS_XMP,
				kXMP_Property_ModifyDate.c_str());
		}

		/*
		**
		*/
		void RemoveVariableFields(SXMPMeta& inXMPMeta)
		{
			RemoveFieldsFromNamespaceMM(inXMPMeta);
			RemoveFieldsFromNamespaceXMP(inXMPMeta);
		}

		/*
		**
		*/
		dvatemporalxmp::XMPTrack* CreateTrack(
			const dvacore::UTF16String& inTrackType,
			const dvacore::UTF16String& inTrackName,
			dvatemporalxmp::XMPDocOpsTemporal* inDocOps,
			dvamediatypes::FrameRate const& inMediaFrameRate)
		{
			ASL_REQUIRE(inDocOps != NULL);
			if (dvatemporalxmp::XMPTrack* track = Utilities::GetTrack(inTrackType, inDocOps))
			{
				return track;
			}

			// Create the root level speech track amalgamation
			dvamediatypes::RatioTime frameRateRatio = dvamediatypes::FrameRateToRatio(inMediaFrameRate);
			dvatemporalxmp::XMPTrack trackTemplate(frameRateRatio.numerator(), frameRateRatio.denominator());
			trackTemplate.SetTrackType(reinterpret_cast<XMP_StringPtr>((dvacore::utility::UTF16to8(inTrackType).c_str())));
			inDocOps->AddTrack(
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inTrackName).c_str()),
				trackTemplate);		// input parameter

			return Utilities::GetTrack(inTrackType, inDocOps);
		}

		dvatemporalxmp::XMPTemporalMetadataInfo CreateTemporalMetadataInfo(const CottonwoodMarker& inMarker)
		{
			dvatemporalxmp::XMPCuePointParamList xmpCuePointParamList;
			BOOST_FOREACH(const dvatemporalxmp::CustomMarkerParam& param, inMarker.GetCuePointList())
			{
				xmpCuePointParamList.push_back(
					dvatemporalxmp::XMPCuePointParam(
					reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(param.mKey).c_str()), 
					reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(param.mValue).c_str())));
			}


			dvatemporalxmp::XMPTemporalMetadataInfo markerMetadataInfo(
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetName()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetComment()).c_str()),
				inMarker.GetStartTime().GetTicks(),
				inMarker.GetDuration().GetTicks(),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetType()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetLocation()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetTarget()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetCuePointType()).c_str()),
				xmpCuePointParamList,
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetSpeaker()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetProbability()).c_str()),
				reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inMarker.GetGUID().AsString()).c_str()));

			// xinchen todo: checkin after code in dvatemporalxmp locked down
			dvatemporalxmp::XMPKeywordList keywords = 
				KeywordsAccessUtils::GetKeywordListFromTagParams(inMarker.GetTagParams());
			for (dvatemporalxmp::XMPKeywordList::iterator it = keywords.begin();
				it != keywords.end();
				it++)
			{
				markerMetadataInfo.AddKeyword(*it);
			}
			//////////////////////////////////////////////////////////////////////////

			return markerMetadataInfo;
		}

		/*
		**
		*/
		dvamediatypes::FrameRate GetRoughCutFrameRate(AssetItemPtr const& assetItem)
		{
			//[TODO RC] need refactor for loading roughcut
			// Set default value
			dvamediatypes::FrameRate frameRate = dvamediatypes::kFrameRate_VideoNTSC;

			// If not empty, get frame rate from the first sub clip
			AssetItemList subAssetItems;// = assetItem->GetAllSubAssetItems();
			if (!subAssetItems.empty())
			{
				// Need error message during import (true), ignore audio (true) and metadata (true)
				BE::IMasterClipRef masterClip = 
					BEUtilities::ImportFile(subAssetItems.front()->GetMediaPath(), BE::IProjectRef(), true, true, true);
				const ASL::UInt32 videoClipCount = masterClip->GetClipCount(BE::kMediaType_Video);
				BE::IClipRef videoClip;
				// Find not-null video clip
				for (ASL::UInt32 i = 0; i < videoClipCount; ++i)
				{
					videoClip = masterClip->GetClip(BE::kMediaType_Video, i);
					if (videoClip != NULL)
					{
						break;
					}
				}
				if (videoClip != NULL)
				{
					frameRate = videoClip->GetFrameRate();
				}
			}

			return frameRate;
		}

		typedef std::map<ASL::Guid, dvatemporalxmp::XMPTemporalMetadataInfo> MarkerMap;
		typedef std::map<std::string/*trackType*/,  MarkerMap> TrackMarkerMap;
		typedef std::map<std::string/*trackType*/,  dvatemporalxmp::XMPTrack> TrackInfoMap;
		void BuildUnDupMarkers(ASL::StdString const& inXMPString, TrackInfoMap& outTrackInfoMap, TrackMarkerMap& outTrackMarkerMap)
		{
			SXMPMeta meta(inXMPString.c_str(), static_cast<XMP_StringLen>(inXMPString.length()));
			dvatemporalxmp::XMPDocOpsTemporal docOps;
			docOps.OpenXMP(&meta);


			for (dvatemporalxmp::XMPDocOpsTemporal::XMPTrackListIter it = docOps.TrackListIteratorBegin();
				it != docOps.TrackListIteratorEnd();
				++it)
			{
				dvacore::StdString stdTrackType;

				it->GetTrackType(&stdTrackType);
				if (stdTrackType.empty())
				{
					stdTrackType = "Unknown";
				}

				if(outTrackInfoMap.find(stdTrackType) == outTrackInfoMap.end())
				{
					dvatemporalxmp::XMPTrack markerTrack = (*it);
					markerTrack.ClearMarkers();
					outTrackInfoMap[stdTrackType] = markerTrack;
				}

				dvatemporalxmp::XMPTrack::XMPMarkerListIter iter = it->MarkerListIteratorBegin();
				dvatemporalxmp::XMPTrack::XMPMarkerListIter endIter = it->MarkerListIteratorEnd();
				for (; iter != endIter ; ++iter)
				{
					dvacore::StdString idString;
					ASL::Guid markerID;
					iter->second.GetMarkerField(&idString, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_MarkerID);
					if ( idString.empty() )
					{
						markerID = ASL::Guid::CreateUnique();
					}
					else
					{
						markerID = ASL::Guid(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(idString.c_str())));
					}

					if (outTrackMarkerMap[stdTrackType].find(markerID) == outTrackMarkerMap[stdTrackType].end())
					{
						outTrackMarkerMap[stdTrackType].insert(MarkerMap::value_type(markerID ,iter->second));
					}
				}
			}
		}

		void InitializeMarkerFromXMPMetaInfo(
			CottonwoodMarker& outMarker,
			const dvacore::UTF16String& inTrackType,
			const dvatemporalxmp::XMPTemporalMetadataInfo& inXMP)
		{
			dvamediatypes::TickTime startTime = dvamediatypes::TickTime::TicksToTime(inXMP.GetStartTime());
			dvamediatypes::TickTime duration = dvamediatypes::TickTime::TicksToTime(inXMP.GetDuration());

			if (startTime < dvamediatypes::kTime_Zero)
			{
				duration += startTime;
				startTime = dvamediatypes::kTime_Zero;
			}

			if (duration < dvamediatypes::kTime_Zero)
			{
				duration = dvamediatypes::kTime_Zero;
			}

			const dvacore::StdString* valuePtr;
			valuePtr = inXMP.GetMarkerFieldPtr(dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_MarkerType);
			ASL_ASSERT(valuePtr);
			dvacore::UTF16String markerType = outMarker.GetType();
			if (valuePtr)
			{
				markerType = dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8((*valuePtr).c_str()));
				if (markerType.size() == 0)
				{
					markerType = inTrackType;
				}

				// Check if the type is in the Marker Type Registry.
				// If not, register the type
				MarkerTypeRegistry types = MarkerType::GetAllMarkerTypes();
				if (types.find(markerType) == types.end())
				{
					MarkerType::AddMarkerToRegistry(markerType, markerType, markerType, dvacore::UTF16String());
				}
			}
			else
			{
				markerType = dvacore::utility::AsciiToUTF16(kXMP_MarkerType_Comment);
			}

			dvacore::StdString field;
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Name);
			outMarker.SetName(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Comment);
			outMarker.SetComment(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Location);
			outMarker.SetLocation(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Target);
			outMarker.SetTarget(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_CuePointType);
			outMarker.SetCuePointType(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));

			ASL::Guid markerID;
			try 
			{
				// Get marker ID
				inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_MarkerID);
				if ( field.empty() )
				{
					markerID = ASL::Guid::CreateUnique();
				}
				else
				{
					markerID = ASL::Guid(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
				}
			}
			catch (...)
			{
				DVA_ASSERT_MSG(0, "Get invalid marker ID from XMP.");
				markerID = ASL::Guid::CreateUnique();
			}

			dvatemporalxmp::CustomMarkerParamList markerCuePointList;
			dvatemporalxmp::XMPCuePointParamList cuePointList = inXMP.GetCuePointParamList();
			for (dvatemporalxmp::XMPCuePointParamList::iterator itr = cuePointList.begin(); itr != cuePointList.end(); ++itr)
			{
				dvatemporalxmp::CustomMarkerParam param(
					dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(itr->GetKeyPtr()->c_str())),
					dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(itr->GetValuePtr()->c_str())));
                
                markerCuePointList.push_back(param);
			}

			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Speaker);
			outMarker.SetSpeaker(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));
			inXMP.GetMarkerField(&field, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_Probability);
			outMarker.SetProbability(dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(field.c_str())));

			outMarker.SetGUID(markerID);
			outMarker.SetStartTime(startTime);
			outMarker.SetDuration(duration);
			outMarker.SetType(markerType);
			outMarker.SetCuePointList(markerCuePointList);

			// xinchen todo: checkin after code in dvatemporalxmp locked down
			// read keyword from XMPTemporalMetadataInfo
			PL::TagParamMap tagParams = KeywordsAccessUtils::GetTagParamsFromKeywordList(inXMP.GetKeywords());
			for (PL::TagParamMap::const_iterator tagIter = tagParams.begin();
				tagIter != tagParams.end();
				tagIter++)
			{
				outMarker.AddTagParam((*tagIter).second);
			}
		}

		// Moved from MZ::Utilities
		bool ReadMetadataForPathFn(const dvacore::UTF16String& inMediaPath, ML::CustomMetadata::BrowserAssetInfo& outMetadataXMP)
		{
			PL::AssetMediaInfoPtr assetMediaInfo = PL::SRProject::GetInstance()->GetAssetMediaInfo(inMediaPath);
			if (assetMediaInfo)
			{
				outMetadataXMP.mMediaInfoID = assetMediaInfo->GetAssetMediaInfoGUID();
				outMetadataXMP.mAssetID = assetMediaInfo->GetAssetMediaInfoGUID();
				outMetadataXMP.mParentAssetID = assetMediaInfo->GetAssetMediaInfoGUID();
				PL::XMPText xmp = assetMediaInfo->GetXMPString();
				outMetadataXMP.mXMP = MF::BinaryData(xmp->c_str(), xmp->size());
				return true;
			}
			return false;
		}
		bool WriteMetadataForPathFn(const dvacore::UTF16String& inMediaPath, const MF::BinaryData& inAssetInfo)
		{
			return true;
		}
		bool GetModTimeForPathFn(const dvacore::UTF16String& inMediaPath, dvacore::utility::Guid& outModTime)
		{
			return true;
		}
		bool GetReadOnlyStatusForPathFn(const dvacore::UTF16String& inMediaPath, bool& outReadOnly)
		{
			outReadOnly = false;
			return true;
		}
		bool RequestInfoForAssetFn(const ASL::Guid& inAssetID, BE::IMediaRef inMedia)
		{
			return true;
		}
		const dvacore::UTF8String kDBMetadataProviderID = dvacore::utility::AsciiToUTF8("DBMetadataProvider");

		// Moved from MZ/Sequence.cpp
		static void SelectInitialNumberOfTracks(
			ASL::UInt32 inAudioChannelGroupCount,
			BE::ISequencePreset::AudioTrackDefinitions& outAudioTracks)
		{
			if (inAudioChannelGroupCount == 0)
			{
				inAudioChannelGroupCount = 2;
			}
			BE::ISequencePreset::AudioTrackDefinition track;
			track.mIsSubmix = false;
			track.mPan = 0.0f;
			track.mChannelType = MF::kAudioChannelType_Mono;

			for (ASL::UInt32 i = 0; i < inAudioChannelGroupCount; ++i)
			{
				outAudioTracks.push_back(track);
			}
		}

		static BE::IProjectItemRef CreateSequenceItemFromPreset(
			const BE::IMasterClipRef& inMasterClip,
			const BE::IProjectRef& inProject,
			const BE::IProjectItemRef& inContainingBin,
			const BE::ISequencePresetRef& inPreset,
			MZ::SequenceAudioTrackRule inUseType,
			const ASL::String& inSequenceName = ASL_STR("")) 
		{
			ASL::String sequenceName = (inSequenceName == ASL_STR("")) ? inMasterClip->GetName() : inSequenceName;

			return MZ::ProjectActions::CreateSequence(
				inProject,
				inContainingBin,
				inPreset->GetVideoFrameRate(),
				inPreset->GetVideoFrameRect(),
				inPreset->GetVideoPixelAspectRatio(),
				inPreset->GetVideoFieldType(),
				inPreset->GetUseMaximumBitDepth(),
				inPreset->GetUseMaximumRenderQuality(),
				inPreset->GetAudioFrameRate(),
				inPreset->GetAudioChannelType(),
				inPreset->GetInitialNumberOfVideoTracks(),
				inPreset->GetInitialNumberOfMonoTracks(),
				inPreset->GetInitialNumberOfStereoTracks(),
				inPreset->GetInitialNumberOfFiveOneTracks(),
				inPreset->GetInitialNumberOfMonoSubmixTracks(),
				inPreset->GetInitialNumberOfStereoSubmixTracks(),
				inPreset->GetInitialNumberOfFiveOneSubmixTracks(),
				sequenceName,
				inPreset->GetEditingModeGUID(),
				inPreset->GetVideoTimeDisplay(),
				inPreset->GetAudioTimeDisplay(),
				inPreset->GetPreviewPresetFileName(),
				inPreset->GetPreviewPresetSelectedCodec(),
				inPreset->GetPreviewVideoFrameRect(),
				inUseType,
				MZ::ActionUndoableFlags::kNotUndoable);
		}

		dvamediatypes::FrameRate GetVideoFrameRateFromXMP(const BE::IMediaMetaDataRef& inMediaMetadataRef)
		{
			dvamediatypes::FrameRate frameRate(dvamediatypes::kFrameRate_Zero);

			MF::BinaryData buffer;
			ASL::Result result = inMediaMetadataRef->ReadMetaDatum(
				MF::BinaryData(),
				BE::kMetaDataType_XMPBinary,
				0,
				buffer);

			if (ASL::ResultSucceeded(result))
			{
				int timeScale(0);
				int timeSampleSize(0);
				std::string existingXMPString;
				XMP_StringLen stringLen;

				SXMPMeta metadata(buffer.GetData(), (XMP_StringLen)buffer.GetSize());
				if (metadata.GetProperty(kXMP_NS_DM, "startTimeScale", &existingXMPString, &stringLen))
				{
					timeScale = ASL::Coercion<int>::Result(existingXMPString);
				}

				if (metadata.GetProperty(kXMP_NS_DM, "startTimeSampleSize", &existingXMPString, &stringLen))
				{
					timeSampleSize = ASL::Coercion<int>::Result(existingXMPString);
				}

				if (timeScale && timeSampleSize)
				{
					frameRate = dvamediatypes::FrameRate(timeScale / timeSampleSize);
				}
			}

			return frameRate;
		}

		static BE::ISequencePresetRef CreateSequencePresetFromSequence(
			const BE::ISequenceRef& inSequence,
			const BE::IProjectRef& inProject,
            bool inIsForceMono)
		{
			BE::ISequencePresetRef sequencePreset = MZ::SequencePreviewPresets::CreateMatchingSequencePreset(inSequence, inProject);

            if (inIsForceMono)
            {
                //	This logic is prelude specific. What we want do is set audioTrack mono, and set initial number of AudioTracks
                //	according to sequencePreset.
                int numAudioChannels = 0;
                MF::AudioChannelType audioChannelType = sequencePreset->GetAudioChannelType();
                numAudioChannels = MF::GetNumChannelsFromType(audioChannelType);
                BE::ISequencePreset::AudioTrackDefinitions audioTracks;
                SelectInitialNumberOfTracks(numAudioChannels, audioTracks);
                
                sequencePreset->SetAudioTracks(audioTracks);
            }
            else
            {
                sequencePreset->SetAudioTracks(BE::ISequencePreset::AudioTrackDefinitions());

            }

			return sequencePreset;
		}


		static BE::ISequencePresetRef CreateSequencePresetFromMedia(
			const BE::IMasterClipRef& inMasterClip,
			bool inIsForceMono)
		{
			BE::ISequencePresetRef sequencePreset = BE::ISequencePresetRef(ASL::CreateClassInstanceRef(BE::kSequencePresetClassID));

			BE::VideoCodecType codec = 0;

			bool isStillImage = MZ::BEUtilities::IsMasterClipStill(inMasterClip);

			dvamediatypes::FrameRate videoFrameRate = isStillImage ? 
													Utilities::GetStillFrameRate() :
													Utilities::GetVideoFrameRate(inMasterClip);
			if (videoFrameRate == dvamediatypes::kFrameRate_Zero)
			{
				videoFrameRate = Utilities::GetDefaultVideoFrameRate();
			}

			ASL::Rect videoFrameRect = MZ::GetRecentSequenceVideoFrameRect();
			dvamediatypes::PixelAspectRatio videoPixelAspectRatio = MZ::GetRecentSequenceVideoPixelAspectRatio();
			dvamediatypes::FieldType fieldType = MZ::GetRecentSequenceFieldType();
			dvamediatypes::FrameRate audioFrameRate = dvamediatypes::kFrameRate_Audio48000;

			bool isDropFrame = isStillImage ?
								Utilities::StillIsDropFrame() :
								MZ::Utilities::IsDropFrame(inMasterClip);

			dvamediatypes::TimeDisplay timeDisplay = MZ::Utilities::GetTimeDisplayForFrameRate(videoFrameRate, isDropFrame);

			bool maximumBitDepth = false;
			bool maximumRenderQuality = false;
			ASL::UInt32 numAudioChannels = 0;
			MZ::MediaParams mediaParams;

			if (BE::MasterClipUtils::GetContentType(inMasterClip) == BE::kContentType_Sequence)
			{
				BE::ISequenceRef sequence = BE::MasterClipUtils::GetSequence(inMasterClip);
				numAudioChannels = MF::GetNumChannelsFromType(sequence->GetMasterTrackChannelType());
				mediaParams = MZ::MediaParams::Create(sequence);
			}
			else
			{
				mediaParams = MZ::MediaParams::Create(inMasterClip);
				if (BE::IVideoClipRef videoClip = BE::IVideoClipRef(inMasterClip->GetClip(BE::kMediaType_Video, 0)))
				{
					BE::IMediaContentRef mediaContent(BE::IClipRef(videoClip)->GetContent());
					BE::IMediaRef media(mediaContent->GetDeprecatedMedia());
					BE::IMediaStreamRef mediaStream(media->GetMediaStream(BE::kMediaType_Video));
					BE::IVideoStreamRef videoStream(mediaStream);
					BE::IVideoSourceRef videoSource(mediaStream->GetSource());

					ASL::Rect clipFrameRect = videoClip->GetFrameRect();
					if (clipFrameRect != ASL::Rect())
					{
						videoFrameRect = clipFrameRect;
					}
					dvamediatypes::PixelAspectRatio clipPixelAspectRatio = videoClip->GetPixelAspectRatio();
					if (clipPixelAspectRatio != dvamediatypes::kPixelAspectRatio_Unknown && clipPixelAspectRatio != dvamediatypes::kPixelAspectRatio_Any)
					{
						videoPixelAspectRatio = clipPixelAspectRatio;
					}

					dvamediatypes::FrameRate clipFrameRate = videoStream->GetFrameRate();
					if (videoStream->IsStill())
					{
						videoFrameRate = PL::Utilities::GetStillFrameRate();
					}
					else if (clipFrameRate != dvamediatypes::kFrameRate_Zero)
					{
						videoFrameRate = clipFrameRate;
					}

					BE::IMediaInfoRef mediaInfo(media->GetMediaInfo());
					BE::IMediaMetaDataRef mediaMetaData(mediaInfo);
					if (videoStream->IsStill())
					{
						isDropFrame = PL::Utilities::StillIsDropFrame();
					}
					else if (mediaMetaData)
					{
						isDropFrame = mediaMetaData->GetIsDropFrame();
					}

					// if we're pulling from a source that needs indexing, it might be lying about its framerate,
					// such as ImporterFastMPEG, which arbitrarily sets itself to 29.97 as an initial value.
					if (mediaInfo && mediaInfo->IsPending() && mediaMetaData)
					{
						dvamediatypes::FrameRate xmpFrameRate = GetVideoFrameRateFromXMP(mediaMetaData);
						if (xmpFrameRate != dvamediatypes::kFrameRate_Zero)
						{
							videoFrameRate = xmpFrameRate;
						}
					}

					timeDisplay = MZ::Utilities::GetTimeDisplayForFrameRate(
						videoFrameRate, isDropFrame);

					fieldType = videoStream->GetFieldType();
					if (!dvamediatypes::HasFields(fieldType))
					{
						fieldType = dvamediatypes::kFieldType_Progressive;
					}

					if (videoSource)
					{
						codec = videoSource->GetFrameVidSubType();

						MF::PixelFormatVector supportedTypes;
						videoSource->GetSupportedPixelTypes(supportedTypes);
						for (MF::PixelFormatVector::const_iterator it = supportedTypes.begin();
							it != supportedTypes.end() && !maximumRenderQuality; ++it)
						{
							maximumRenderQuality = dvamediatypes::GetChannelBitDepth(*it) != dvamediatypes::kChannelBitDepth_8;
						}
					}
				}
			}

			// About numAudioChannels: used to define audio track in the sequence for logging,
			// it will be the channel num based on the MasterClip's audio type.
			// If it is zero, "SelectInitialNumberOfTracks" we make 2 by default, but we set it to non-zero before that
			numAudioChannels = 0;

			// we need to set correct channel type to sequence preset, to playback clips correctly:
			//	1. stereo clip of one track
			//	2. clip of two mono tracks
			BE::IMasterClip::ClipChannelGroupVector channelGroupVector;
			inMasterClip->GetAudioClipChannelGroups(channelGroupVector);
			MF::AudioChannelType channelType;
			if (channelGroupVector.empty())
			{
				channelType = MF::kAudioChannelType_Stereo;
				numAudioChannels = MF::GetNumChannelsFromType(channelType);
			}
			else
			{
				MF::AudioChannelType masterTrackChannelType = channelGroupVector[0].mChannelType;
				channelType = masterTrackChannelType;

				if (masterTrackChannelType == MF::kAudioChannelType_Mono && channelGroupVector.size() > 1)
				{
					numAudioChannels = channelGroupVector.size() > MF::kAudioChannelMaxLayoutSize 
						? MF::kAudioChannelMaxLayoutSize
						: channelGroupVector.size();

					channelType = MF::kAudioChannelType_Multichannel;
				}
				else
				{
					numAudioChannels = MF::GetNumChannelsFromType(channelType);
				}
			}

			BE::ISequencePreset::AudioTrackDefinitions audioTracks;
			if (inIsForceMono)
			{
				SelectInitialNumberOfTracks(numAudioChannels, audioTracks);
			}
			else
			{
				BOOST_FOREACH(BE::IMasterClip::ClipChannelGroup const& channelGroup, channelGroupVector)
				{
					BE::ISequencePreset::AudioTrackDefinition track;
					track.mIsSubmix = false;
					track.mPan = 0.0f;
					track.mChannelType = channelGroup.mChannelType;
					audioTracks.push_back(track);
				}
			}

			BE::ISequencePreset::VideoTrackDefinitions videoTracks;
			sequencePreset->GetVideoTracks(videoTracks);

			int inAdaptiveNumChannels(0);

			/////******************** Comment for the editing mode: ************************/////
			// If we use GUID_NULL as the editing mode, Premiere will give an assert when opening the exported sequence
			// if its frame rate is not 29.97. There's a related watson bug #2989361.
			// The assert is because Prelude use GUID_NULL as editing mode for a sequence, 
			// while Premiere always sets the frame rate to 29.97 of a sequence if its editing mode is GUID_NULL, 
			// if the frame rate of the exported sequence is not 29.97, there will be an assert. 
			// To fix it, we use a desktop GUID (integrated from Premiere) when creating a sequence preset.
			sequencePreset->SetValues(
				ASL_STR("GeneratedPreset_Name"),
				ASL_STR("GeneratedPreset_Description"),
				MZ::Utilities::GetDesktopEditingModeGuid(),	//ASL::Guid(),
				videoFrameRect,
				videoFrameRate,
				videoPixelAspectRatio,
				fieldType,
				true, //const bool& inVideoUseMaxBitDepth,
				false, //const bool& inVideoUseMaxRenderQuality,
				audioFrameRate,//const dvamediatypes::FrameRate& inAudioFrameRate,
				channelType, //audioChannelType,
				inAdaptiveNumChannels,		//inAdaptiveNumChannels
				timeDisplay, //const dvamediatypes::TimeDisplay& inVideoTimeDisplay,
				dvamediatypes::kTimeDisplay_AudioSamplesTimecode, //const dvamediatypes::TimeDisplay& inAudioTimeDisplay,
				ASL::String(), //const ASL::String& inPreviewPresetFileName,
				codec, //const BE::VideoCodecType& inVideoCodecType,
				1, //const ASL::UInt32& inInitialNumberOfVideoTracks,
				videoTracks,	// const VideoTrackDefinitions& inVideoTracks
				audioTracks,//const AudioTrackDefinitions& inAudioTracks,
				videoFrameRect//const ASL::Rect& inPreviewVideoRect
				);

			return sequencePreset;
		}

		/**
		** Only consider cases of XX:XX:XX:XX or XX;XX;XX;XX
		*/
		static void TimecodeStringToHMSF(
			const ASL::String& inTimecodeString,
			bool inIsDropFrame,
			ASL::UInt32& outHours,
			ASL::UInt32& outMinutes,
			ASL::UInt32& outSeconds,
			ASL::UInt32& outFrames)
		{
			ASL::String delimiter = inIsDropFrame ? ASL_STR(";") : ASL_STR(":");

			const ASL::UInt32 fieldCount = 4;
			ASL::SInt32 timeFields[fieldCount];
			ASL::String timecodeStr = inTimecodeString;

			ASL::UInt32 idx = 0;
			while (!timecodeStr.empty())
			{
				size_t delimiterPos = timecodeStr.find_first_of(delimiter);
				ASL::String subStr = timecodeStr.substr(0, delimiterPos);

				timeFields[idx++] = ASL::Coercion<ASL::UInt32>::Result(subStr);
				if (idx >= fieldCount)
				{
					break;
				}
			
				timecodeStr = timecodeStr.substr(delimiterPos + 1);
			}

			outHours = timeFields[0];
			outMinutes = timeFields[1];
			outSeconds = timeFields[2];
			outFrames = timeFields[3];
		}

		static ASL::String ConvertToTimecodeSectionString(ASL::UInt32 inIntVal)
		{
			return (inIntVal < 10) ?
					ASL_STR("0") + ASL::Coercion<ASL::String>::Result(inIntVal):
					ASL::Coercion<ASL::String>::Result(inIntVal);
		}

		static bool EnsureDirectoryExist(const ASL::String& inPath)
		{
			if (!inPath.empty())
			{
				return ASL::PathUtils::ExistsOnDisk(inPath) ? 
					true : 
				ASL::ResultSucceeded(ASL::Directory::CreateOnDisk(inPath, true));
			}

			return false;
		}

		static
		size_t GetMasterClipAudioChannels(BE::IMasterClipRef inMasterClip)
		{
			size_t audioChannels = 0;
			size_t audioClipCount = inMasterClip->GetClipCount(BE::kMediaType_Audio);
			for (size_t index = 0; index < audioClipCount; ++index)
			{
				BE::IClipRef clip(inMasterClip->GetClip(BE::kMediaType_Audio, index));
				BE::IMediaContentRef mediaContent(clip->GetContent());
				if (mediaContent)
				{
					BE::IAudioStreamRef audioStream(mediaContent->GetMedia()->GetMediaStream(BE::kMediaType_Audio));
					if (audioStream)
						audioChannels += audioStream->GetNumChannels();
				}
			}
			return audioChannels;
		}

		class RoughCutVisitor
		{
		public:
			RoughCutVisitor(BE::ISequenceRef inSequence)
				: mSequence(inSequence)
				, mHasChecked(false)
				, mResult(ASL::kSuccess)
                , mNumberOfNonEmptyVideoTrack(0)
			{
			}

			void DoVisit()
			{
				BE::VisitSequence(mSequence, boost::bind(&RoughCutVisitor::VisitObject, this, _1));
			}

			ASL::Result Check()
			{
				!mHasChecked
				&& !IsFinished()
				&& CheckRCSubClipCompleteness()
				&& CheckUnsupportedGaps();

				mHasChecked = true;
				return mResult;
			}

			ASL::Result GetResult()
			{
				return Check();
			}

		protected:
			struct RCSubClipTrait
			{
				BE::IMasterClipRef		mMasterClip;
				dvamediatypes::TickTime mEnd;
				dvamediatypes::TickTime mInPoint;
				dvamediatypes::TickTime mOutPoint;
				ASL::UInt32				mVideoTrackItemCount;
                
                //  This is used for counting the audio channel referenced by RCSubClipTrait.
                //
				dvacore::utility::SmallBlockAllocator::multiset<size_t>	 mAudioChannels;
                
                //  This is used for recording the trackIndex which coverred by RCSubClipTrait.
                typedef dvacore::utility::SmallBlockAllocator::set<BE::TrackIndex> TrackIndexSet;
                TrackIndexSet mAudioTrackIndexSet;

				RCSubClipTrait() : mVideoTrackItemCount(0) {}
			};

			bool IsFinished() const
			{
				return ASL::ResultFailed(mResult);
			}

			BE::VisitorStatus VisitObject(const ASL::ASLUnknownRef& inObject)
			{
				// Add more interfaces to query here if necessary
				Dispatch<BE::ITrackRef>(inObject);
				Dispatch<BE::IClipTrackItemRef>(inObject);
				Dispatch<BE::ISequenceContentRef>(inObject);
				Dispatch<BE::IVideoClipRef>(inObject);
				Dispatch<BE::IMediaRef>(inObject);

				return IsFinished() ? BE::kVisitorStatus_Abort : BE::kVisitorStatus_Continue;
			}

			template<typename InterfaceRef, typename SourceRef> void Dispatch(const SourceRef& inObject)
			{
				if (IsFinished())
				{
					return;
				}

				InterfaceRef dstTypeObjRef(inObject);
				if (dstTypeObjRef)
				{
					Visit(dstTypeObjRef);
				}
			}

			void SetResult(ASL::Result inResult)
			{
				if (!IsFinished())
				{
					mResult = inResult;
				}
			}

			bool CheckUnsupportedGaps()
			{
				dvamediatypes::TickTime currentTime = dvamediatypes::kTime_Zero;
				BOOST_FOREACH (const auto& startTimeSubClipTraitPair, mSubClipTraits)
				{
					if (currentTime != startTimeSubClipTraitPair.first)
					{
						SetResult(eContainsGapsBetweenSubClips);
						return false;
					}
					currentTime = startTimeSubClipTraitPair.second.mEnd;
				}
				return true;
			}
            
            bool IsAudioTrackIndexMatch(const RCSubClipTrait::TrackIndexSet& inTrackIndexSet, RCSubClipTrait::TrackIndexSet& ioAudioMaximumTrackIndexSet)
            {
                RCSubClipTrait::TrackIndexSet::const_iterator trackIndexIter = inTrackIndexSet.begin();
                RCSubClipTrait::TrackIndexSet::const_iterator trackIndexEnd = inTrackIndexSet.end();
                RCSubClipTrait::TrackIndexSet::const_iterator maximumTrackIndexIter = ioAudioMaximumTrackIndexSet.begin();
                RCSubClipTrait::TrackIndexSet::const_iterator maximumTrackIndexIterEnd = ioAudioMaximumTrackIndexSet.end();
                
                bool result = true;
                for (; trackIndexIter !=  trackIndexEnd && maximumTrackIndexIter != maximumTrackIndexIterEnd; ++trackIndexIter, ++maximumTrackIndexIter)
                {
                    if (*trackIndexIter != *maximumTrackIndexIter)
                    {
                        return false;
                    }
                }
                
                if (ioAudioMaximumTrackIndexSet.size() < inTrackIndexSet.size())
                {
                    ioAudioMaximumTrackIndexSet = inTrackIndexSet;
                }
                    
                return result;
            }

			bool CheckRCSubClipCompleteness()
			{
                RCSubClipTrait::TrackIndexSet audioMaximumTrackIndexSet;
				BOOST_FOREACH (const auto& startTimeSubClipTraitPair, mSubClipTraits)
				{
					const RCSubClipTrait& subClipTrait = startTimeSubClipTraitPair.second;
					DVA_ASSERT_MSG (subClipTrait.mMasterClip != NULL, "No master clip for child clips?");
					if (subClipTrait.mMasterClip == NULL)
					{
						SetResult(ASL::eUnknown);
						return false;
					}
                    
                    // The AudioTrackIndex coverred by this RCSubClipTrait should be "compatible" with audioMaximumTrackIndex
                    // Compatible means The previous "x" number of TrackIndex should be exactly match.
                    //  x = mini(subClipTrait.size(), audioMaximunTrackIndex.size())
                    if (!IsAudioTrackIndexMatch(subClipTrait.mAudioTrackIndexSet, audioMaximumTrackIndexSet))
                    {
                        SetResult(eAudioTrackClipItemIsNotAligned);
						return false;
                    }
                        

					ASL::UInt32 videoClipCount = subClipTrait.mMasterClip->GetClipCount(BE::kMediaType_Video);

					// [TODO] No duplication check right now. When we know how to check, should change here.
					// ensure all src channels are included
					//dvacore::utility::SmallBlockAllocator::set<size_t> uniqueChannelIndexes(
					//	subClipTrait.mAudioChannels.begin(), subClipTrait.mAudioChannels.end());

					//if (uniqueChannelIndexes.size() != subClipTrait.mAudioChannels.size() // should no duplicated audio channels
					//	|| (subClipTrait.mVideoTrackItemCount != videoClipCount) // all video should be included and no duplication
					//	|| (GetMasterClipAudioChannels(subClipTrait.mMasterClip) != subClipTrait.mAudioChannels.size()) // audio channels count not match
					//	|| (uniqueChannelIndexes.size() != subClipTrait.mAudioChannels.size())) // audio channel index not match

					if ((subClipTrait.mVideoTrackItemCount != videoClipCount) // all video should be included and no duplication
						|| (GetMasterClipAudioChannels(subClipTrait.mMasterClip) != subClipTrait.mAudioChannels.size())) // audio channels count not match
					{
						SetResult(eSubClipTrackItemsNotCompleteness);
						return false;
					}
				}
				return true;
			}

			void Visit(const BE::ITrackRef& inTrack)
			{
				if (inTrack->IsLocked())
				{
					SetResult(eContainsLockedTrack);
					return;
				}

				BE::IClipTrackRef clipTrack = BE::IClipTrackRef(inTrack);
				if (clipTrack)
				{
					if (clipTrack->GetNumberOfTrackItems(BE::kTrackItemType_Transition) > 0)
					{
						//SetResult(eContainsTransition);
					}
                    
                    if (clipTrack->GetNumberOfTrackItems(BE::kTrackItemType_Clip) > 0 && inTrack->GetMediaType() == BE::kMediaType_Video)
                    {
                        mNumberOfNonEmptyVideoTrack++;
                        if (mNumberOfNonEmptyVideoTrack > 1)
                        {
                            SetResult(eContainsMoreThanOneNonEmptyVideoTrack);
                        }
                    }
				}
			}

			void Visit(const BE::IVideoClipRef& inVideoClip)
			{
				if (inVideoClip->IsAdjustmentLayer())
				{
					SetResult(eContainsAdjustmentLayer);
				}
			}

			RCSubClipTrait CreateRCSubClipTrait(const BE::IClipTrackItemRef& inClipTrackItem)
			{
				BE::ITrackItemRef trackItem = BE::ITrackItemRef(inClipTrackItem);
				RCSubClipTrait subClipTrait;
				BE::IChildClipRef childClip = inClipTrackItem->GetChildClip();
				BE::IClipRef clip = childClip->GetClip();
				subClipTrait.mMasterClip = childClip->GetMasterClip();
				subClipTrait.mEnd = trackItem->GetEnd();
				subClipTrait.mInPoint = clip->GetInPoint();
				subClipTrait.mOutPoint = clip->GetOutPoint();
                subClipTrait.mAudioTrackIndexSet.insert(trackItem->GetTrackIndex());

				if (trackItem->GetMediaType() == BE::kMediaType_Video)
					subClipTrait.mVideoTrackItemCount = 1;
				else
				{
					// ensure all audio channels are included
					BE::IAudioClipRef audioClip(clip);
					size_t count = audioClip->GetSecondaryAssignmentCount();
					for (size_t channelIndx = 0; channelIndx < count; ++channelIndx)
					{
						size_t srcChannelIndex = kInvalidChannelIndex;
						audioClip->GetSecondaryAssignment(channelIndx, srcChannelIndex);
						subClipTrait.mAudioChannels.insert(srcChannelIndex);
					}
				}
				return subClipTrait;
			}

			bool CheckRCSubClipTrait(const RCSubClipTrait& inLeft, const RCSubClipTrait& inRight)
			{
				if (inLeft.mMasterClip != inRight.mMasterClip)
					SetResult(eContainsMixedTrackItems);
				else if (inLeft.mEnd != inRight.mEnd
						|| inLeft.mInPoint != inRight.mInPoint
						|| inLeft.mOutPoint != inRight.mOutPoint)
					SetResult(eContainsMisalignedTrackItems);
				else
					return true;
				return false;
			}

			void Visit(const BE::IClipTrackItemRef& inClipTrackItem)
			{
				BE::ITrackItemRef trackItem(inClipTrackItem);
				dvamediatypes::TickTime startTime = trackItem->GetStart();

				BE::IChildClipRef childClip = inClipTrackItem->GetChildClip();
				BE::IClipRef clip = childClip->GetClip();

				if (clip->GetPlaybackSpeed() != 1.0f || !clip->GetTimeRemapping()->IsIdentity())
				{
					SetResult(eUnsafeSpeedSetting);
					return;
				}

				BE::IMasterClipRef masterClip = childClip->GetMasterClip();
				Dispatch<BE::IMasterClipRef>(masterClip);

				RCSubClipTrait subClipTrait = CreateRCSubClipTrait(inClipTrackItem);

				auto iter = mSubClipTraits.find(startTime);
				if (iter == mSubClipTraits.end())
					mSubClipTraits[startTime] = subClipTrait;
				else
				{
					RCSubClipTrait& latestSubClipTrait = iter->second;
					if (CheckRCSubClipTrait(latestSubClipTrait, subClipTrait))
					{
						latestSubClipTrait.mVideoTrackItemCount += subClipTrait.mVideoTrackItemCount;
						latestSubClipTrait.mAudioChannels.insert(
							subClipTrait.mAudioChannels.begin(),
							subClipTrait.mAudioChannels.end());
                        
                        latestSubClipTrait.mAudioTrackIndexSet.insert(
                            subClipTrait.mAudioTrackIndexSet.begin(),
                            subClipTrait.mAudioTrackIndexSet.end());
					}
					else
						return;
				}


				// don't allow cross sub clips link or group
				dvacore::utility::SmallBlockAllocator::vector<BE::ITrackItemGroupRef> trackItemGroups;
				if (mSequence != NULL)
				{
					trackItemGroups.push_back(mSequence->FindGroup(trackItem));
					trackItemGroups.push_back(mSequence->FindLink(trackItem, false));
				}
				BOOST_FOREACH (BE::ITrackItemGroupRef trackItemGroup, trackItemGroups)
				{
					if (trackItemGroup != NULL
						&& (trackItemGroup->GetStart() != trackItem->GetStart()
							|| trackItemGroup->GetEnd() != trackItem->GetEnd()))
					{
						SetResult(eContainsCrossSubClipLinkOrGroup);
						return;
					}
				}
			}

			void Visit(const BE::IMasterClipRef& inReferencedMasterClip)
			{
				// MZ::Utilities::IsSyntheticMedia can't be used to judge synthetic cause Prelude may 
				// miss some plugin.
				BE::IProjectItemRef projectItem(inReferencedMasterClip->GetOwner());
				DVA_ASSERT(projectItem);
				if (projectItem)
				{
					ASL::String locator;
					ASL::String assetType;
					MZ::Utilities::GetEAMediaLocatorAndType(projectItem, locator, assetType);
					if (assetType != EACL::kAssetType_MasterClip.ToString() && assetType != EACL::kAssetType_SubClip.ToString())
					{
						SetResult(eContainsSynthetic);
					}
				}
				else
				{
					SetResult(eUnsafeSequenceStructure);
				}
			}

			void Visit(const BE::ISequenceContentRef& inSequenceContent)
			{
				// If we have a nested Sequence, we are not a rough cut
				SetResult(eContainsNestedSequence);
			}

			void Visit(const BE::IMediaRef& inMedia)
			{
				BE::ITitlerMediaRef titlerMedia(inMedia->GetMediaInfo());
				if (titlerMedia)
				{
					SetResult(eContainsTitle);
				}
			}


			dvacore::utility::SmallBlockAllocator::map<dvamediatypes::TickTime, RCSubClipTrait>	mSubClipTraits;
			BE::ISequenceRef								mSequence;
			bool											mHasChecked;
			ASL::Result										mResult;
            ASL::UInt32                                     mNumberOfNonEmptyVideoTrack;
		};

		/*
		**
		*/
		ASL::String GetTransitionAlignment(
			const BE::ISequenceRef& inSequence,
			const BE::ITransitionTrackItemRef& inTransitionItem)
		{
			BE::ITrackItemRef trackItem(inTransitionItem);
			BE::TrackIndex trackIndex = trackItem->GetTrackIndex();
			BE::MediaType mediaType = trackItem->GetMediaType();

			BE::ITrackGroupRef trackGroup = inSequence->GetTrackGroup(mediaType);
			BE::IClipTrackRef clipTrack = trackGroup->GetClipTrack(trackIndex);

			BE::ITrackItemRef theTrackItem = clipTrack->GetTrackItem(BE::kTrackItemType_Clip, trackItem->GetStart());
			BE::ITrackItemRef nextTrackItem = clipTrack->GetNextTrackItem(BE::kTrackItemType_Clip, theTrackItem);

			bool hasOutgoingClip = false;
			bool hasIncomingClip = false;

			BE::IClipTrackItemRef theClipTrackItem(theTrackItem);
			BE::IClipTrackItemRef nextClipTrackItem(nextTrackItem);

			if (theClipTrackItem)
			{
				if (theClipTrackItem->GetHeadTransition() == inTransitionItem)
				{
					hasIncomingClip = true;
				}
				else	// tail transition
				{
					hasOutgoingClip = true;
					if (nextClipTrackItem && nextClipTrackItem->GetHeadTransition() == inTransitionItem)
					{
						hasIncomingClip = true;
					}
				}
			}

			DVA_ASSERT(hasIncomingClip || hasOutgoingClip);

			if (! hasOutgoingClip)
			{
				return ASL_STR("start-black");
			}
			else if (! hasIncomingClip)
			{
				return ASL_STR("end-black");
			}
		
			return ASL_STR("center");
		}

		PL::TransitionItemPtr ConvertTransitionItem(
			const BE::ISequenceRef& inSequence,
			const BE::ITransitionTrackItemRef& inTransitionItem)
		{
			if (!inSequence || !inTransitionItem)
			{
				return PL::TransitionItemPtr();
			}

			BE::ITrackItemRef trackItem(inTransitionItem);
			BE::MediaType mediaType = trackItem->GetMediaType();

			dvamediatypes::TickTime start = trackItem->GetStart();
			dvamediatypes::TickTime end = trackItem->GetEnd();

			// todo: need get correct alignment string
			ASL::String alignmentStr = GetTransitionAlignment(inSequence, inTransitionItem);
			ASL::Float32 alignment = inTransitionItem->GetAlignment();
			dvamediatypes::TickTime cutPoint = 
				dvamediatypes::TickTime::TicksToTime(alignment * (end.GetTicks() - start.GetTicks()));

			BE::TrackIndex trackIndex = trackItem->GetTrackIndex();

			bool isReverse = false;
			ASL::Float32 startRatio = 0.0f;
			ASL::Float32 endRatio = 1.0f;

			ASL::String matchName = inTransitionItem->GetMatchName().ToString();

			if (mediaType == BE::kMediaType_Video)
			{
				BE::IVideoTransitionTrackItemRef videoTransitionItem(inTransitionItem);
				if (videoTransitionItem)
				{
					isReverse = videoTransitionItem->GetReverse();
					startRatio = videoTransitionItem->GetStartPercent();
					endRatio = videoTransitionItem->GetEndPercent();
				}
			}

			// add transition items to asset item
			PL::TransitionItemPtr theTransitionItem(new PL::TransitionItem(
				start,
				end,
				inSequence->GetVideoFrameRate(),
				alignmentStr,
				cutPoint,
				trackIndex,
				inTransitionItem->GetMatchName().ToString(),
				mediaType,
				startRatio,
				endRatio,
				isReverse));
			
			return theTransitionItem;
		}
	}

	namespace Utilities
	{
		extern const char* kTimeValue = "timeValue";
		extern const char* kTimeFormat = "timeFormat";
		extern const char* kStartTimecode = "startTimecode";
		extern const char* kAltTimecode = "altTimecode";

		void RegisterCloudData(const CloudData& inCloudData)
		{
			SRUnassociatedMetadataList addedList, changedList;
			BOOST_FOREACH(CloudData::value_type pair, inCloudData)
			{
				bool exist = false;
				SRUnassociatedMetadataRef srUnassociatedMetadata = 
					SRProject::GetInstance()->FindSRUnassociatedMetadata(pair.first);
				if (srUnassociatedMetadata)
				{
					SRProject::GetInstance()->RemoveSRUnassociatedMetadata(srUnassociatedMetadata);
					exist = true;
				}

				SRUnassociatedMetadataRef newUnassociatedMetadata = 
					SRFactory::CreateFileBasedSRUnassociatedMetadata(pair.first, pair.second);
				newUnassociatedMetadata->SetAliasName(ASL::PathUtils::GetFilePart(pair.second));
				newUnassociatedMetadata->SetIsCreativeCloud(true);
				SRProject::GetInstance()->AddUnassociatedMetadata(newUnassociatedMetadata);

				if (exist)
				{
					changedList.push_back(newUnassociatedMetadata);
				}
				else
				{
					addedList.push_back(newUnassociatedMetadata);
				}
			}

			if (!changedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					MZ::kStation_PreludeProject, 
					UnassociatedMetadataChangedMessage(changedList));
			}

			if (!addedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					MZ::kStation_PreludeProject, 
					UnassociatedMetadataAddedMessage(addedList));
			}
		}

		/*
		**	
		*/
		void RegisterUnassociatedMetadata(
			const ASL::String& inMetadataID,
			const ASL::String& inMetadataPath/* = ASL::String()*/, 
			const ASL::String& inName/* = ASL::String()*/, 
			bool isCreativeCloud/* = false*/)
		{
			// Look up existing SRUnassociatedMetadata object or create one for the
			// passed in file path, then collect marker information

			SRUnassociatedMetadataList metadataList;
			SRUnassociatedMetadataRef srUnassociatedMetadata = SRProject::GetInstance()->FindSRUnassociatedMetadata(inMetadataID);

			bool exist = false;
			if (srUnassociatedMetadata)
			{
				SRProject::GetInstance()->RemoveSRUnassociatedMetadata(srUnassociatedMetadata);
				exist = true;
			}

			srUnassociatedMetadata = SRFactory::CreateFileBasedSRUnassociatedMetadata(inMetadataID, inMetadataPath);
			if (srUnassociatedMetadata)
			{
				srUnassociatedMetadata->SetAliasName(inName);
				srUnassociatedMetadata->SetIsCreativeCloud(isCreativeCloud);
				SRProject::GetInstance()->AddUnassociatedMetadata(srUnassociatedMetadata);

				metadataList.push_back(srUnassociatedMetadata);
			}

			if (!metadataList.empty())
			{
				if (exist)
				{
					ASL::StationUtils::BroadcastMessage(
						MZ::kStation_PreludeProject, 
						UnassociatedMetadataChangedMessage(metadataList));
				}
				else
				{
					ASL::StationUtils::BroadcastMessage(
						MZ::kStation_PreludeProject, 
						UnassociatedMetadataAddedMessage(metadataList));
				}
			}
		}

		void RegisterUnassociatedMetadata(const UnassociatedMetadataList& inUnassociatedMetadataList)
		{
			SRUnassociatedMetadataList changedList, addedList;
			BOOST_FOREACH(const UnassociatedMetadataPtr& eachMetadata, inUnassociatedMetadataList)
			{
				bool exist = false;

				SRUnassociatedMetadataRef srUnassociatedMetadata = 
					SRProject::GetInstance()->FindSRUnassociatedMetadata(eachMetadata->mMetadataId);
				if (srUnassociatedMetadata)
				{
					SRProject::GetInstance()->RemoveSRUnassociatedMetadata(srUnassociatedMetadata);
					exist = true;
				}

				SRUnassociatedMetadataRef lastMetadata;
				if (eachMetadata->mXMP)
				{
					lastMetadata = 
						SRFactory::CreateSRUnassociatedMetadata(eachMetadata->mMetadataId, eachMetadata->mMetadataPath, eachMetadata->mXMP);
				}
				else
				{
					lastMetadata = 
						SRFactory::CreateFileBasedSRUnassociatedMetadata(eachMetadata->mMetadataId, eachMetadata->mMetadataPath);
				}

				if (lastMetadata)
				{
					lastMetadata->SetAliasName(eachMetadata->mAliasName);
					SRProject::GetInstance()->AddUnassociatedMetadata(lastMetadata);

					if (exist)
					{
						changedList.push_back(lastMetadata);
					}
					else
					{
						addedList.push_back(lastMetadata);
					}
				}
			}

			if (!changedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					MZ::kStation_PreludeProject, 
					UnassociatedMetadataChangedMessage(changedList));
			}

			if (!addedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					MZ::kStation_PreludeProject, 
					UnassociatedMetadataAddedMessage(addedList));
			}
		}

		void UnregisterUnassociatedMetadata(const PL::UnassociatedMetadataList& inUnassociatedMetadataList)
		{
			PL::SRUnassociatedMetadataList metadataList;
			BOOST_FOREACH(const UnassociatedMetadataPtr& eachMetadata, inUnassociatedMetadataList)
			{
				SRUnassociatedMetadataRef srUnassociatedMetadata = 
					SRProject::GetInstance()->FindSRUnassociatedMetadata(eachMetadata->mMetadataId);
				if (srUnassociatedMetadata)
				{
					SRProject::GetInstance()->RemoveSRUnassociatedMetadata(srUnassociatedMetadata);
					metadataList.push_back(srUnassociatedMetadata);
				}
			}

			if (!metadataList.empty())
			{
				ASL::StationUtils::PostMessageToUIThread(
					MZ::kStation_PreludeProject,
					UnassociatedMetadataRemovedMessage(metadataList),
					true);
			}
		}

		/*
		**
		*/
		void BuildClipItemFromLibraryInfo(AssetItemList& ioAsseteItemList, SRClipItems& ioClipItems)
		{
			DVA_ASSERT(!ioAsseteItemList.empty());

			AssetItemList::iterator iter = ioAsseteItemList.begin();
			AssetItemList::iterator end = ioAsseteItemList.end();

			for (; iter != end; ++iter)
			{
				SRClipItemPtr srClipItem(new SRClipItem(*iter));
				srClipItem->SetSubClipGUID((*iter)->GetAssetClipItemGuid());
				ioClipItems.push_back(srClipItem);

				dvamediatypes::TickTime inPoint = srClipItem->GetStartTime();
				dvamediatypes::TickTime duration = srClipItem->GetDuration();
					
				if ( (*iter)->GetCustomInPoint() != dvamediatypes::kTime_Invalid && (*iter)->GetCustomOutPoint() != dvamediatypes::kTime_Invalid )
				{
					// Use the custom In/Out point and duration
					inPoint += (*iter)->GetCustomInPoint();
					duration = (*iter)->GetCustomOutPoint() - (*iter)->GetCustomInPoint();
				}
				else if ( (*iter)->GetCustomInPoint() == dvamediatypes::kTime_Invalid && (*iter)->GetCustomOutPoint() != dvamediatypes::kTime_Invalid )
				{
					// The user set the Custom Out point with no Custom In Point, use the asset InPoint and the custom Out point as duration
					duration = (*iter)->GetCustomOutPoint();
				}
				else if ( (*iter)->GetCustomInPoint() != dvamediatypes::kTime_Invalid && (*iter)->GetCustomOutPoint() == dvamediatypes::kTime_Invalid )
				{
					// The user sets the Custom In Point without Custom Out Point, use the asset InPoint + CustomInPoint, Asset Duration - CustomInPoint
					inPoint += (*iter)->GetCustomInPoint();
					duration -= (*iter)->GetCustomInPoint();
				}

				(*iter)->SetInPoint(inPoint);
				(*iter)->SetDuration(duration);
			}
		}

		/*
		**
		*/
		bool IsAssetPathOffline(ASL::String const& assetPath)
		{
			bool result = true;
			if (MZ::Utilities::IsEAMediaPath(assetPath))
			{
				BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), assetPath);
				BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(projectItem);
				BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(masterClip);
				if (media)
				{
					result =  media->GetMediaInfo() == NULL;
				}
			}
			else
			{
				result = !ASL::PathUtils::ExistsOnDisk(assetPath);
			}

			return result;
		}

		/*
		**
		*/
		bool IsAssetItemOffline(const AssetItemPtr& inAssetItem)
		{
			bool result = true;
			if (inAssetItem)
			{
				result = IsAssetPathOffline(inAssetItem->GetMediaPath());
			}

			return result;
		}


		/*
		**
		*/
		void SwitchToLoggingModule(ISRRoughCutRef inRoughCut, BE::ITrackItemSelectionRef inTrackItemSelection)
		{
			if ( inRoughCut == NULL || inTrackItemSelection == NULL )
			{
				return;
			}

			BE::ITrackItemGroupRef group(inTrackItemSelection);

			BE::TrackItemVector items;
			group->GetItems(BE::kMediaType_Video, items);
			if (items.empty())
			{
				group->GetItems(BE::kMediaType_Audio, 0, items);
			}

			ASL_REQUIRE(items.size());
			BE::ITrackItemRef item(items[0]);

			SRClipItemPtr clipItem = inRoughCut->GetSRClipItem(item);

			if ( NULL != clipItem )
			{
				AssetItemPtr assetItem = clipItem->GetAssetItem();

				if (PL::Utilities::IsAssetItemOffline(assetItem))
				{
					dvacore::UTF16String errorMsg = dvacore::ZString(
						"$$$/Prelude/Mezzanine/ModulePicker/WarnAssetOffline=The media you are attempting to work on is offline. Please locate your media and bring it back online. Then retry this operation.");
					if (MZ::Utilities::IsEAMediaPath(assetItem->GetMediaPath()))
					{
						errorMsg = dvacore::ZString(
							"$$$/Prelude/Mezzanine/ModulePicker/WarnAssetOfflineEA=The media you are attempting to work on is offline.");
					}

					const ASL::String& warningCaption = 
						dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/WarnAssetOfflineCaption=Asset Offline");

					UIF::MessageBox(errorMsg, warningCaption, UIF::MBFlag::kMBFlagOK, UIF::MBIcon::kMBIconError);
					return;
				}

					

				//	Here we need pass 
				ASL::String mediaPath = assetItem->GetMediaPath();
				ASL::Guid assetItemGuid = ASL::Guid::CreateUnique();
				ASL::String assetName = assetItem->GetAssetName();
				if (MZ::Utilities::IsEAMediaPath(mediaPath))
				{
					assetItemGuid = ASL::Guid(mediaPath);
					BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), mediaPath);
					assetName = projectItem->GetName();
				}
				dvamediatypes::TickTime startTime = assetItem->GetInPoint();
				if (MZ::BEUtilities::IsMasterClipStill(clipItem->GetMasterClip()))
				{
					startTime = dvamediatypes::kTime_Zero;
				}
				AssetItemPtr  subClipAssetItem(new AssetItem(
						kAssetLibraryType_SubClip,						//	inType
						mediaPath,						//	inMediaPath
						assetItemGuid,						//	inGUID
						ASL::Guid(),	//	inParentGUID
						assetItem->GetAssetMediaInfoGUID(),				//	inAssetMediaInfoGUID
						assetName,						//	inAssetName
						DVA_STR(""),									//	inAssetMetadata
						startTime,						//	InPoint
						assetItem->GetDuration(),						//	inDuration 
						assetItem->GetCustomInPoint(),
						assetItem->GetCustomOutPoint(),
						ASL::Guid::CreateUnique()));					//	inClipItemGuid

				ModulePicker::GetInstance()->ActiveModule(subClipAssetItem);
			}
		}

		/*
		**
		*/
		void SwitchToLoggingModule(
						const ASL::String& inMediaPath,
						const dvamediatypes::TickTime& inEditTime,
						const dvamediatypes::TickTime& inStart,
						const dvamediatypes::TickTime& inDuration)
		{
			//if (inMediaPath.empty() || !ASL::PathUtils::ExistsOnDisk(inMediaPath))
			//{
			//	const ASL::String& warningCaption = 
			//		dvacore::ZString("$$$/Prelude/Mezzanine/ModulePicker/WarnAssetOfflineCaption=Asset Offline");
			//	dvacore::UTF16String errorMsg = dvacore::ZString(
			//		"$$$/Prelude/Mezzanine/ModulePicker/WarnAssetOffline=The media you are attempting to work on is offline. Please locate your media and bring it back online and then retry this operation.");

			//	UIF::MessageBox(errorMsg, warningCaption, UIF::MBFlag::kMBFlagOK, UIF::MBIcon::kMBIconError);
			//	return;
			//}

			//AssetMediaInfoPtr assetMediaInfo = 
			//					SRProject::GetInstance()->GetAssetMediaInfo(inMediaPath);
			//if (assetMediaInfo)
			//{
			//	AssetItemPtr assetItem(new AssetItem(
			//		kAssetLibraryType_MasterClip,
			//		assetMediaInfo->GetMediaPath(),
			//		assetMediaInfo->GetAssetMediaInfoGUID(),
			//		assetMediaInfo->GetAssetMediaInfoGUID(),
			//		assetMediaInfo->GetAssetMediaInfoGUID(),
			//		assetMediaInfo->GetAliasName(),
			//		DVA_STR(""), // Dummy metadata
			//		inStart,
			//		inDuration));
			//	ModulePicker::GetInstance()->ActiveModule(assetItem);
			//}
			////	[TODO] Need set EditTime , Start, End for logging clip
		}

		/*
		**
		*/
		SREditingModule GetCurrentEditingModule()
		{
			return ModulePicker::GetInstance()->GetCurrentEditingModule();
		}

		/*
		**
		*/
		ClipType GetClipType(const ISRPrimaryClipPlaybackRef inPrimaryClipPlayback)
		{
			ClipType clipType = kClipType_Unknown;
			ISRLoggingClipRef loggingClip(inPrimaryClipPlayback);
			ISRRoughCutRef roughCut(inPrimaryClipPlayback);

			if(loggingClip != NULL)
			{
				clipType = kClipType_Clip;
			}
			else if (roughCut != NULL)
			{
				clipType = kClipType_RoughCut;
			}
			return clipType;
		}

		/*
		**
		*/
		bool PathSupportsXMP(
			const ASL::String& inMediaPath)
		{
			if (inMediaPath.empty())
			{
				return false;
			}

			//	If this file is Json String which mean EA clip, we should write XMP to it
			// This path should NOT be a media locator.
			if (MZ::Utilities::IsEAMediaPath(inMediaPath))
			{
				return true;
			}

			try
			{
				return (ASL::PathUtils::ExistsOnDisk(inMediaPath) && ML::MetadataManager::PathSupportsXMP(inMediaPath));
			}
			catch (XMP_Error&)
			{
				DVA_ASSERT_MSG(0, "Run into exception in SXMPFiles::GetFormatInfo in Utilities::PathSupportsXMP");
			}
			return false;
		}

		/*
		**
		*/
		void BuildXMPStringFromMarkers(
						ASL::StdString& ioXMPString, 
						MarkerSet const& inMarkers, 
						TrackTypes const& inTrackTypes, 
						dvamediatypes::FrameRate const& inMediaFrameRate)
		{
			TrackTypes trackTypes(inTrackTypes);
			SXMPMeta meta(ioXMPString.c_str(), static_cast<XMP_StringLen>(ioXMPString.length()));
			dvatemporalxmp::XMPDocOpsTemporal docOps;
			docOps.OpenXMP(&meta);

			// Delete any existing tracks
			docOps.DeleteAllTracks();

			// send all of the markers to the XMP
			MarkerTracks markerTracks;

			for (MarkerSet::iterator itr = inMarkers.begin(); itr != inMarkers.end(); ++itr)
			{
				dvacore::UTF16String markerType = (*itr).GetType();
				dvamediatypes::TickTime startTime = (*itr).GetStartTime();
				CottonwoodMarker marker = *itr;
				markerTracks[markerType].insert(std::make_pair(startTime, marker));
			}

			for (MarkerTracks::iterator tracksItr = markerTracks.begin(); tracksItr  != markerTracks.end(); ++tracksItr)
			{
				dvacore::UTF16String trackType = tracksItr->first;

				WriteMarkers(docOps, trackType, trackTypes[trackType].trackName, tracksItr->second, inMediaFrameRate);
			}

			docOps.TrackListChanged();
			docOps.NoteChange(						// Add a change history note
				reinterpret_cast<XMP_StringPtr>(MakePathChangePart(kTracksPath).c_str()));
			docOps.PrepareForSave("");				// Flush to the XMP

			meta.SerializeToBuffer(&ioXMPString);
		}

		/*
		**
		*/
		void BuildMarkersFromXMPString(
					ASL::StdString const& inXMPString, 
					TrackTypes& outTrackTypes,
					MarkerSet& outMarkers,
                    PL::ISRMarkerOwnerRef inMarkerOwner)
		{
			if (!inXMPString.empty())
			{
				SXMPMeta meta(inXMPString.c_str(), static_cast<XMP_StringLen>(inXMPString.length()));
				dvatemporalxmp::XMPDocOpsTemporal docOps;
				docOps.OpenXMP(&meta);

				for (dvatemporalxmp::XMPDocOpsTemporal::XMPTrackListIter it = docOps.TrackListIteratorBegin();
					it != docOps.TrackListIteratorEnd();
					++it)
				{
					dvacore::StdString stdTrackType;
					dvacore::UTF16String trackType;
					dvatemporalxmp::XMPTrack* xmpTrack = &*it;

					xmpTrack->GetTrackType(&stdTrackType);
					trackType = dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(stdTrackType.c_str()));
					if (trackType.empty())
					{
						trackType = ASL_STR("Unknown");
					}

					dvatemporalxmp::XMPTrack::XMPMarkerListIter iter;
					dvatemporalxmp::XMPTrack::XMPMarkerListIter endIter = xmpTrack->MarkerListIteratorEnd();

					for (iter = xmpTrack->MarkerListIteratorBegin(); iter != endIter ; ++iter)
					{
                        PL::SRMarkerOwnerWeakRef markerWeakRef;
                        ASL::IWeakReferenceCapableRef weakReference = ASL::IWeakReferenceCapableRef(inMarkerOwner);
                        if (weakReference)
                        {
                            markerWeakRef = weakReference->GetWeakReference();
                        }
						CottonwoodMarker marker(markerWeakRef);
						InitializeMarkerFromXMPMetaInfo(marker, trackType, iter->second);
						outMarkers.insert(marker);
						trackType = marker.GetType();
					}

					// store the track type and name that we have seen
					dvacore::StdString trackName;
					xmpTrack->GetTrackName(&trackName);

					TrackInfo trackInfo;
					trackInfo.trackName = dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(trackName.c_str()));
					trackInfo.frameRate = dvamediatypes::FrameRate(xmpTrack->GetFrameRateNumerator() * 1.0 / xmpTrack->GetFrameRateDenominator());
					outTrackTypes.insert(std::make_pair(trackType, trackInfo));
				}
			}
		}


		/*
		**
		*/
		void MergeTemporalMarkers(
							ASL::StdString const& inNewXMPString, 
							ASL::StdString const& inOldXMPString, 
							ASL::StdString& outMergedString)
		{
			if (inNewXMPString.empty() || inOldXMPString.empty())
			{
				DVA_ASSERT("Param for MergeTemporalMarkers is wrong");
				return;
			}
			TrackMarkerMap trackMarkerMap;
			TrackInfoMap trackInfoMap;
			BuildUnDupMarkers(inNewXMPString, trackInfoMap, trackMarkerMap);
			BuildUnDupMarkers(inOldXMPString, trackInfoMap, trackMarkerMap);

			//	now , we have Merged marker map, we need to generate XMP string by them
			SXMPMeta meta(outMergedString.c_str(), static_cast<XMP_StringLen>(outMergedString.length()));
			dvatemporalxmp::XMPDocOpsTemporal docOps;
			docOps.OpenXMP(&meta);

			docOps.DeleteAllTracks();

			BOOST_FOREACH (const TrackInfoMap::value_type& trackInfoPair, trackInfoMap)
			{
				dvacore::StdString trackName;
				trackInfoPair.second.GetTrackName(&trackName);
				docOps.AddTrack(trackName.c_str(), trackInfoPair.second);
				dvatemporalxmp::XMPTrack* addedTrack  = GetTrack(ASL::MakeString(trackInfoPair.first), &docOps);

				BOOST_FOREACH (const MarkerMap::value_type& markerIter, trackMarkerMap[trackInfoPair.first])
				{
					addedTrack->AddMarker(markerIter.second);
				}
			}

			docOps.TrackListChanged();
			docOps.NoteChange(						// Add a change history note
				reinterpret_cast<XMP_StringPtr>(MakePathChangePart(kTracksPath).c_str()));
			docOps.PrepareForSave("");				// Flush to the XMP

			meta.SerializeToBuffer(&outMergedString);
		}


		/*
		**
		*/
		bool IsNewerMetaData(
			ASL::StdString const& inNewXMPString, 
			ASL::StdString const& inOldXMPString)
		{
			bool result(true);
			SXMPMeta  newXMPMeta;
			SXMPMeta  oldXMPMeta;

			try
			{
				newXMPMeta.ParseFromBuffer(inNewXMPString.c_str(), static_cast<XMP_StringLen>(inNewXMPString.length()));
				oldXMPMeta.ParseFromBuffer(inOldXMPString.c_str(), static_cast<XMP_StringLen>(inOldXMPString.length()));

				XMP_DateTime newXMPTime;
				XMP_DateTime oldXMPTime;
				newXMPMeta.GetProperty_Date(kXMP_NS_XMP, "MetadataDate", &newXMPTime, NULL);
				oldXMPMeta.GetProperty_Date(kXMP_NS_XMP, "MetadataDate", &oldXMPTime, NULL);

				if (newXMPTime.hasDate && newXMPTime.hasTime && 
					oldXMPTime.hasDate && oldXMPTime.hasTime &&
					SXMPUtils::CompareDateTime(newXMPTime, oldXMPTime) < 0)
				{
					result = false;
				}
			}
			catch (XMP_Error& p)
			{
				ASL_UNUSED(p);
			}

			return result;
		}

		/*
		**
		*/
		bool  IsDateTimeEqual(
			const XMP_DateTime& newXMPTime,
			const XMP_DateTime& oldXMPTime)
		{
			bool result(false);
			if (IsValidXMPDate(newXMPTime)&& IsValidXMPDate(oldXMPTime))
			{
				return SXMPUtils::CompareDateTime(newXMPTime, oldXMPTime) == 0;
			}
			else if(!IsValidXMPDate(newXMPTime) &&  !IsValidXMPDate(oldXMPTime))
			{
				return true;
			}
			return result;
		}


		/*
		**
		*/
		ASL::Result SaveSelectedAssetItems(bool isSilent)
		{
			enum {
				bitAssetItemDirty_True = 0x01,
				bitAssetItemDirty_RoughCut = 0x02,
				bitAssetItemDirty_MasterClip = 0x04
			};

			// Check any dirty on the selected clips. If no dirty, we return false.
			ASL::UInt8 dirtyFlag = 0x00;

			AssetItemList assetItemList = 
							SRProject::GetInstance()->GetAssetSelectionManager()->GetSelectedAssetItemList();
			AssetItemList::const_iterator it = assetItemList.begin();
			AssetItemList::const_iterator endIter = assetItemList.end();
			for (; it != endIter; ++it)
			{
				AssetMediaType type = (*it)->GetAssetMediaType();
				ISRPrimaryClipRef primaryClip = SRProject::GetInstance()->GetPrimaryClip((*it)->GetMediaPath()); 

				if (primaryClip && primaryClip->IsDirty())
				{
					dirtyFlag |= bitAssetItemDirty_True;

					if (type == kAssetLibraryType_RoughCut)
					{
						dirtyFlag |= bitAssetItemDirty_RoughCut;
					} else if (type == kAssetLibraryType_MasterClip)
					{
						dirtyFlag |= bitAssetItemDirty_MasterClip;
					}

					if ((dirtyFlag & bitAssetItemDirty_RoughCut) && (dirtyFlag & bitAssetItemDirty_MasterClip))
					{
						break;
					}
				}
			}

			if (dirtyFlag & bitAssetItemDirty_True)
			{
				// Prompt user to choose save it or not.
				if (!isSilent)
				{
					const ASL::String& warningCaption = 
						dvacore::ZString("$$$/Prelude/Mezzanine/Utilities/SaveSelectedClipsTitle=Save Changes");
					const ASL::String& warningMessage = 
						dvacore::ZString("$$$/Prelude/Mezzanine/Utilities/SaveSelectedClipsMessage=Do you want to save changes to the selected clip?");
					const ASL::String& warningMessageRoughCut = 
						dvacore::ZString("$$$/Prelude/Mezzanine/Utilities/SaveSelectedRoughCutsMessage=Do you want to save changes to the selected rough cut?");
					const ASL::String& warningMessageRoughCutAndClip = 
						dvacore::ZString("$$$/Prelude/Mezzanine/Utilities/SaveSelectedClipsAndRoughCutsMessage=Do you want to save changes to the selected clip and rough cut?");
				
					UIF::MBResult::Type ret = UIF::MBResult::kMBResultNone;
					if ((dirtyFlag & bitAssetItemDirty_RoughCut) && (dirtyFlag & bitAssetItemDirty_MasterClip))
					{
						ret = SRUtilitiesPrivate::PromptForSave(warningCaption, warningMessageRoughCutAndClip);
					} else if (dirtyFlag & bitAssetItemDirty_RoughCut)
					{
						ret = SRUtilitiesPrivate::PromptForSave(warningCaption, warningMessageRoughCut);
					} else {
						ret = SRUtilitiesPrivate::PromptForSave(warningCaption, warningMessage);
					}

					if (ret == UIF::MBResult::kMBResultCancel)
					{
						return ASL::eUserCanceled;
					}
					else if (ret == UIF::MBResult::kMBResultNo)
					{
						return ASL::kSuccess;
					}
				}

				// Save the libraryClipInfo
				for (it = assetItemList.begin(); it != endIter; ++it)
				{
					ISRPrimaryClipRef primaryClip = SRProject::GetInstance()->GetPrimaryClip((*it)->GetMediaPath()); 
					if (primaryClip && primaryClip->IsDirty())
					{
						primaryClip->Save(true, false);
						if (isSilent)
						{
							ASL::String infoStr = ASL::DateToString(ASL::Date()) + ASL_STR("  ");
							infoStr += dvacore::ZString("$$$/Prelude/Mezzanine/AutoSave=Automatically Save "); 
							infoStr += primaryClip->GetName();
							ML::SDKErrors::SetSDKInfoString(infoStr);
						}
					}
				}

				// Clear the records in History Panel. 
				MZ::ClearUndoStack(MZ::GetProject());
			}

			return ASL::kSuccess;
		}

		/*
		**
		*/
		bool ExtractElementValue(
			const ASL::String& inSrc,
			const ASL::String& inTagName,
			ASL::String& outValue,
			ASL::String* outTheRestString)
		{
			const dvacore::UTF16String kIDBeginTag = DVA_STR("<") + inTagName + DVA_STR(">");
			const dvacore::UTF16String kIDEndTag = DVA_STR("</") + inTagName + DVA_STR(">");

			dvacore::UTF16String::size_type p1 = inSrc.find(kIDBeginTag);
			dvacore::UTF16String::size_type p2 = inSrc.find(kIDEndTag);

			bool findSuccess = false;
			if ((p1 != dvacore::UTF16String::npos) && (p2 != dvacore::UTF16String::npos))
			{
				const dvacore::UTF16String::size_type startPos = p1 + kIDBeginTag.size();
				outValue = inSrc.substr(startPos, p2 - startPos);
				findSuccess = true;
				if (outTheRestString != NULL)
				{
					*outTheRestString = inSrc.substr(0, p1) + inSrc.substr(p2 + kIDEndTag.size());
				}
			}
			// check empty element style
			else
			{
				ASL::String emptyElement = DVA_STR("<") + inTagName + DVA_STR("/>");
				dvacore::UTF16String::size_type pos = inSrc.find(emptyElement);
				if (pos != dvacore::UTF16String::npos)
				{
					outValue.clear();
					findSuccess = true;
					if (outTheRestString != NULL)
					{
						*outTheRestString = inSrc.substr(0, pos) + inSrc.substr(pos + emptyElement.size());
					}
				}
				else
				{
					findSuccess = false;
				}
			}

			return findSuccess;
		}

		/*
		**
		*/
		dvacore::UTF16String BuildSubClipName(MarkerTrack const& inSubclipMarkers)
		{
			ISRPrimaryClipRef currentMasterClip = ISRPrimaryClipRef(ModulePicker::GetInstance()->GetLoggingClip());
			if (currentMasterClip == NULL)
			{
				DVA_ASSERT_MSG(0, "Build sub clip name in wrong type media.");
				return dvacore::UTF16String();
			}

			dvacore::UTF16String clipName = currentMasterClip->GetName();

			//displayName add clipName with "_" suffix
			dvacore::UTF16String displayName = clipName.append(ASL_STR("_"));

			size_t posix = 0;			
			const size_t posixIndex = displayName.length();
			
			
			for (MarkerTrack::const_iterator f = inSubclipMarkers.begin(), l = inSubclipMarkers.end(); f != l; ++f)
			{
				dvacore::UTF16String const& name = f->second.GetName();
				dvacore::UTF16String::size_type index = name.find(displayName);
							     
				if (index == 0)
				{
					size_t currPosix = 0;
					size_t nameLen = name.length();
					for (std::size_t i = posixIndex; i < nameLen; ++i)
					{
						dvacore::UTF16Char ch = name[i];
						if (ch >= '0' && ch <= '9')
						{
							currPosix = currPosix * 10 + ch - '0';
						}
						else
						{
							currPosix = 0;
							break;
						}
					}

					if (currPosix > posix)
						posix = currPosix;
				}

			}

			++posix;

			return posix < 10 
				? displayName.append(ASL_STR("0"))+ ASL::CoerceAsString::Result<ASL::UInt32>(static_cast<ASL::UInt32>(posix))
				: displayName + ASL::CoerceAsString::Result<ASL::UInt32>(static_cast<ASL::UInt32>(posix));
		}

		// Copy from SharedView/SettingUtilities.cpp
		/* Encode: / -> /x ; /x -> /XX; /Y -> /XY ; \ -> /Y
		Decode: /x ->/; /XX ->/X; /XY -> /Y ; /Y -> \  */
		ASL::String EncodeNameCell(const ASL::String& inPath)
		{
			ASL::String encodeString = inPath;
			dvacore::utility::ReplaceAllStringInString(encodeString, DVA_STR("/"),DVA_STR("/X"));
			dvacore::utility::ReplaceAllStringInString(encodeString, DVA_STR("\\"), DVA_STR("/Y"));
			return encodeString;
		}

		ASL::String DecodeNameCell(const ASL::String& inPath)
		{
			ASL::String decodeString = inPath;
			dvacore::utility::ReplaceAllStringInString(decodeString, DVA_STR("/Y"), DVA_STR("\\"));
			dvacore::utility::ReplaceAllStringInString(decodeString, DVA_STR("/X"), DVA_STR("/"));
			return decodeString;
		}

		/*
		** Restore Project cache location to default place
		*/
		ASL::String RestoreProjectCacheFilesLocationToDefault()
		{
			dvacore::UTF16String fullPath;
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			ASL::String premiereDocuments = ASL::MakeString(ASL::kUserDocumentsDirectory);
			fullPath = ASL::DirectoryRegistry::FindDirectory(premiereDocuments);
			if (!fullPath.empty())
			{
				fullPath = ASL::PathUtils::CombinePaths(fullPath, kProjectCacheFolderName);
				fullPath = ASL::PathUtils::AddTrailingSlash(fullPath);
			}
			bProp->SetValue(kPrefsProjectCacheFilesLocation, fullPath, BE::kPersistent);

			return fullPath;
		}

		/*
		**
		*/
		ASL::String GetProjectCacheFilesLocation()
		{
			ASL::String fullPath;
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			bProp->GetValue(kPrefsProjectCacheFilesLocation, fullPath);

			if (fullPath.empty())
			{
				fullPath = RestoreProjectCacheFilesLocationToDefault();
			}

			return fullPath;
		}

		/*
		**
		*/
		int GetProjectCacheFilesLongestReservedDays()
		{
			int longestReservedDays(0);
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			bProp->GetValue(kPrefsProjectCacheLongestReservedDays, longestReservedDays);

			if (longestReservedDays == 0)
			{
				longestReservedDays = 30;
				bProp->SetValue(kPrefsProjectCacheLongestReservedDays, longestReservedDays, BE::kPersistent);
			}

			return longestReservedDays;
		}

		/*
		**
		*/
		int GetProjectCacheFilesMaximumNumbers()
		{
			int maximumNumber(0);
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			bProp->GetValue(kPrefsProjectCacheMaximumReservedNumber, maximumNumber);

			if (maximumNumber == 0)
			{
				maximumNumber = 5000;
				bProp->SetValue(kPrefsProjectCacheMaximumReservedNumber, maximumNumber, BE::kPersistent);
			}

			return maximumNumber;
		}

		/*
		**
		*/
		void CleanProjectCacheFiles()
		{
			ASL::StationUtils::PostMessageToUIThread(
								MZ::kStation_PreludeProject, 
								CleanProjectCacheFilesMessage());
		}

		/*
		**
		*/
		void MoveProjectCacheFilesToNewDestination(ASL::String const& inFolderPath)
		{
			ASL::StationUtils::PostMessageToUIThread(
				MZ::kStation_PreludeProject, 
				MoveProjectCacheFilesToNewDestinationMessage(inFolderPath));
		}

		/*
		**
		*/
		void SaveStartMode(bool inIsNative)
		{
			ASL::String const& premiereDocuments = ASL::MakeString(ASL::kPremiereDocumentsDirectory);
			ASL::String configFilePath = ASL::DirectoryRegistry::FindDirectory(premiereDocuments) + DVA_STR("Extension Config.xml");
			ASL::File file;
			if (ASL::ResultSucceeded(file.Create(
					configFilePath,
					ASL::FileAccessFlags::kWrite,
					ASL::FileShareModeFlags::kNone,
					ASL::FileCreateDispositionFlags::kCreateAlways,
					ASL::FileAttributesFlags::kAttributeNormal)))
			{
				ASL::UInt32 bytesOfWrite = 0;
				file.Write(EXTENSION_CONFIG_TEXT, sizeof(EXTENSION_CONFIG_TEXT) - 1, bytesOfWrite);
				if (inIsNative)
				{
					file.Write(EXTENSION_CONFIG_NATIVE, sizeof(EXTENSION_CONFIG_NATIVE) - 1, bytesOfWrite);
				}
				else
				{
					file.Write(EXTENSION_CONFIG_FLASH, sizeof(EXTENSION_CONFIG_FLASH) - 1, bytesOfWrite);
				}
			}	
		}

		/*
		**
		*/
		bool NeedEnableMediaCollection()
		{
			static ASL::SInt32 configProjectPanle = kExtensionConfig_Uninit;
			do {
				if (configProjectPanle != kExtensionConfig_Uninit)
					break;

				configProjectPanle = kExtensionConfig_Native;

				ASL::String const& premiereDocuments = ASL::MakeString(ASL::kPremiereDocumentsDirectory);
				ASL::String configFilePath = ASL::DirectoryRegistry::FindDirectory(premiereDocuments) + DVA_STR("Extension Config.xml");
				if (!ASL::PathUtils::ExistsOnDisk(configFilePath))
				{
					SaveStartMode(true);
				}

				if (!ASL::PathUtils::ExistsOnDisk(configFilePath))
					break;

				XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
				XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
				XMLUtils::CreateDOM(&parser, configFilePath.c_str(), &doc);
				if (doc != NULL)
				{
					DOMElement* rootElement = doc->getDocumentElement();
					DVA_ASSERT(rootElement != NULL);
					if (rootElement != NULL)
					{
						ASL::String panelType;
						XMLUtils::GetChildData(rootElement, kProjectPanelName.c_str(), panelType);
						if (dvacore::utility::CaseInsensitive::StringEquals(panelType, kPanelTypeFlash))
							configProjectPanle = kExtensionConfig_Flash;
					}
				}
			} while (false);

			return configProjectPanle == kExtensionConfig_Native;
		}

		/*
		**
		*/
		IngestUtils::IngestSettings GetConfigurableIngestSettings()
		{
			IngestUtils::IngestSettings ingestSettings;

			try
			{
				dvacore::filesupport::Dir sharedDocsDir(dvacore::filesupport::Dir::SharedDocsDir());
				if ( !sharedDocsDir.Exists() )
				{
					return ingestSettings;
				}

				// Get the user presets folder
				dvacore::filesupport::Dir userAdobeDir(sharedDocsDir, DVA_STR("Adobe"));
				if (!userAdobeDir.Exists()) 
				{
					return ingestSettings;
				}

				dvacore::filesupport::Dir userPreludeDir(userAdobeDir, ASL::MakeString(PRM::GetApplicationName()));
				if (!userPreludeDir.Exists()) 
				{
					return ingestSettings;
				}

				dvacore::filesupport::Dir userPreludeVersionDir(userPreludeDir, ASL::MakeString(PRM::GetApplicationVersion()));
				if (!userPreludeVersionDir.Exists()) 
				{
					return ingestSettings;
				}

				ASL::String configFilePath = ASL::PathUtils::AddTrailingSlash(userPreludeVersionDir.FullPath()) + kConfigurationFileFullName;
				if (!ASL::PathUtils::ExistsOnDisk(configFilePath))
				{
					return ingestSettings;
				}

				XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
				XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
				XMLUtils::CreateDOM(&parser, configFilePath.c_str(), &doc);
				if (doc != NULL)
				{
					DOMElement* rootElement = doc->getDocumentElement();
					if (rootElement != NULL)
					{
						const DOMNode* ingestDialogNode = XMLUtils::FindChild(rootElement, kConfigIngestDialogNodeName.c_str());
						if (ingestDialogNode != NULL)
						{
							XMLUtils::GetChildData(ingestDialogNode, kConfigDisableIngestSettingsPropertyName.c_str(), &ingestSettings.mDisableAllIngestSettings);
							XMLUtils::GetChildData(ingestDialogNode, kConfigDefaultSubFolderPropertyName.c_str(), ingestSettings.mDefaultSubFolderName);
						}
					}
				}
			}
			catch(...)
			{
			}

			return ingestSettings;
		}

		/*
		**
		*/
		bool IsAssetItemsAppending()
		{
			return sAssetItemsAppending;
		}

		/*
		**
		*/
		ASL::String GetClipName(BE::ITrackItemRef inTrackItem)
		{
			ASL::String theName;
			ASL_ASSERT(inTrackItem->GetType() == BE::kTrackItemType_Clip);

			ISRPrimaryClipPlaybackRef primaryClipPlayback = ModulePicker::GetInstance()->GetCurrentModuleData();
			ISRLoggingClipRef loggingClip = ISRLoggingClipRef(primaryClipPlayback);

			//	identify EA logging Clip case
			if (loggingClip)
			{
				if (loggingClip->IsLoggingClipForEA())
				{
					return primaryClipPlayback->GetSubClipName(inTrackItem);
				}
			}

			//	Other cases
			if (UIF::IsEAMode())
			{
				return primaryClipPlayback->GetSubClipName(inTrackItem);
			}
			else
			{
				BE::IClipTrackItemRef aClipTrackItem = BE::IClipTrackItemRef(inTrackItem);
				BE::IChildClipRef aChildClip = aClipTrackItem->GetChildClip();
				if (aChildClip != NULL)
				{
					theName = aChildClip->GetName();
				}

				ASL::String subClipName(DVA_STR(""));
				if ( primaryClipPlayback )
				{
					subClipName = primaryClipPlayback->GetSubClipName(inTrackItem);
				}

				return subClipName.length() > 0
					? subClipName + DVA_STR(" - ") + theName
					: theName;
			}
		}

		/*
		**
		*/
		dvamediatypes::TickTime GetClipDurationRegardlessBoundaries(BE::IMasterClipRef const& inMasterClip)
		{
			dvamediatypes::TickTime maxTime = dvamediatypes::kTime_Zero; 
			BE::IMediaRef media = BE::MasterClipUtils::GetMedia(inMasterClip);
			if (!media)
			{
				DVA_ASSERT(0);
				return dvamediatypes::kTime_Zero;
			}

			BE::IMediaInfoRef mediaInfo = media->GetMediaInfo();

			bool hasVideo = inMasterClip->ContainsMedia(BE::kMediaType_Video);
			MF::ISourceRef mediaSource;
			if (mediaInfo)
			{
				if (hasVideo)
				{
					mediaSource = mediaInfo->GetStream(BE::kMediaType_Video, 0);
				}
				else
				{
					mediaSource = mediaInfo->GetStream(BE::kMediaType_Audio, 0);
				}
			}

			if (mediaSource)
			{
				maxTime = mediaSource->GetDuration();
			}
			//	Here, means we met an offline clip.
			else
			{
				maxTime = media->GetDuration();
			}

			return maxTime;
		}

		/*
		**
		*/
		void SetAssetItemsAppending(bool isAppending)
		{
			if (sAssetItemsAppending  != isAppending)
			{
				sAssetItemsAppending = isAppending;
				ASL::StationUtils::BroadcastMessage(MZ::kStation_PreludeProject, AssetsItemAppendingStateChanged());
			}
		}

		/*
		**
		*/
		CustomMetadata GetCustomMedadata(BE::IMasterClipRef const& inMasterClip)
		{
			CustomMetadata customMetadata;
			if (inMasterClip)
			{
				try
				{
					// still image doesn't have valid media duration
					bool isStillImage = MZ::BEUtilities::IsMasterClipStill(inMasterClip);
					customMetadata.mIsStillImage = isStillImage;
					customMetadata.mDuration = isStillImage
													? inMasterClip->GetMaxTrimmedDuration(BE::kMediaType_Video)
													: MZ::ProjectItemHelpers::GetMediaDuration(inMasterClip);
					customMetadata.mName = inMasterClip->GetName();

					// Get FrameRate and dropFrame from masterClip 
					MZ::MediaParams mediaParams = MZ::MediaParams::CreateDefault();

					mediaParams = MZ::MediaParams::Create(inMasterClip);

					if (inMasterClip->GetClipLoggingInfo())
					{
						bool hasAudio = inMasterClip->ContainsMedia(BE::kMediaType_Audio);
						bool hasVideo = inMasterClip->ContainsMedia(BE::kMediaType_Video);
						if (hasAudio || hasVideo)
						{
							if (hasVideo)
							{
								customMetadata.mVideoFrameRate = mediaParams.GetVideoFrameRate();
								BE::IMediaRef media(BE::MasterClipUtils::GetDeprecatedMedia(inMasterClip));
								if (media)
								{
									// Get dropness of the media timecode
									BE::IMediaInfoRef mediaInfo(media->GetMediaInfo());
									BE::IMediaMetaDataRef mediaMetaData(mediaInfo);
									if (mediaMetaData)
									{
										customMetadata.mDropFrame = mediaMetaData->GetIsDropFrame();
									}
								}
							}

							// Get media start time
							MZ::MasterClip mzMasterClip(inMasterClip);
							customMetadata.mMediaStart = mzMasterClip.GetMediaStart();
						}

						customMetadata.mIsAudioOnly = hasAudio && !hasVideo;
					}

					if (isStillImage)
					{
						customMetadata.mVideoFrameRate = GetStillFrameRate();
						customMetadata.mDropFrame = StillIsDropFrame();
					}
				}	
				catch (...)
				{
					DVA_ASSERT(0);
				}
			}

			return customMetadata;
		}

		/*
		**
		*/
		dvatemporalxmp::XMPTrack* GetTrack(
			const dvacore::UTF16String& inTrackType,
			dvatemporalxmp::XMPDocOpsTemporal* inDocOps)
		{
			if (inDocOps)
			{
				dvacore::StdString trackTypeSearchString = reinterpret_cast<XMP_StringPtr>(dvacore::utility::UTF16to8(inTrackType).c_str());
				return inDocOps->GetTrack(&trackTypeSearchString);
			}

			return NULL;
		}

		/*
		**
		*/
		ASL::String CreateCustomMetadataStr(CustomMetadata const& inCustomMetadata)
		{
			//	create DOMDocument
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = XMLUtilities::CreateDomDocument();

			DOMElement* rootElement = pDoc->createElement(kCustomMetadataContent.c_str());
			pDoc->appendChild(rootElement);

			rootElement->setAttribute(kDuration.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mDuration.GetTicks()).c_str());
			rootElement->setAttribute(kDropFrame.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mDropFrame).c_str());
			rootElement->setAttribute(kVideoFrameRate.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mVideoFrameRate.GetTicksPerFrame()).c_str());
			rootElement->setAttribute(kMediaStart.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mMediaStart.GetTicks()).c_str());
			rootElement->setAttribute(kIsAudioOnly.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mIsAudioOnly).c_str());
			rootElement->setAttribute(kIsStillImage.c_str(), ASL::Coercion<ASL::String>::Result(inCustomMetadata.mIsStillImage).c_str());
			rootElement->setAttribute(kName.c_str(), inCustomMetadata.mName.c_str());

			ASL::String result = XMLUtilities::ConvertDomToXMLString(pDoc);
			pDoc->release();
			return result;
		}

		/*
		**
		*/
		bool WriteMarkers(
			dvatemporalxmp::XMPDocOpsTemporal& inDocOps,
			const dvacore::UTF16String& inTrackType, 
			const dvacore::UTF16String& inTrackName, 
			MarkerTrack& inMarkerTrack,
			dvamediatypes::FrameRate const& inMediaFrameRate)
		{
			dvatemporalxmp::XMPTrack *track = NULL;
			track = CreateTrack(inTrackType, inTrackName, &inDocOps, inMediaFrameRate);

			if (track != NULL)
			{
				track->ClearMarkers();

				for (MarkerTrack::iterator markerItr = inMarkerTrack.begin(); markerItr != inMarkerTrack.end(); ++markerItr)
				{
					const CottonwoodMarker &marker = markerItr->second;

					dvatemporalxmp::XMPTemporalMetadataInfo markerInfo = CreateTemporalMetadataInfo(marker);
					track->AddMarker(markerInfo);

				}
			}

			return true;
		}

		/*
		**
		*/
		bool UpdateInOutMarkersInEAMode(
			dvatemporalxmp::XMPDocOpsTemporal& inDocOps,
			const dvacore::UTF16String& inTrackType, 
			const dvacore::UTF16String& inTrackName, 
			MarkerTrack& inMarkerTrack,
			dvamediatypes::FrameRate const& inMediaFrameRate)
		{
			typedef std::map<ASL::Guid, dvatemporalxmp::XMPKeywordList>		MarkerIDToKeywordsMap;
			MarkerIDToKeywordsMap markerIDToKeywords;

			dvatemporalxmp::XMPTrack *track = NULL;
			track = CreateTrack(inTrackType, inTrackName, &inDocOps, inMediaFrameRate);

			if (track != NULL)
			{
				// if possible, cache <markerID, keywords> for new subclips
				for (dvatemporalxmp::XMPTrack::XMPMarkerListIter trackItr = track->MarkerListIteratorBegin();
					trackItr != track->MarkerListIteratorEnd();
					++trackItr)
				{
					dvacore::StdString markerIDString;
					(*trackItr).second.GetMarkerField(
						&markerIDString, dvatemporalxmp::XMPTemporalMetadataInfo::kInfoField_MarkerID);

					for (MarkerTrack::iterator markerItr = inMarkerTrack.begin(); 
						markerItr != inMarkerTrack.end(); 
						++markerItr)
					{
						if ((*markerItr).second.GetGUID() == ASL::Guid(markerIDString))
						{
							markerIDToKeywords[ASL::Guid(markerIDString)] = (*trackItr).second.GetKeywords();
							break;
						}
					}
				}

				track->ClearMarkers();

				for (MarkerTrack::iterator markerItr = inMarkerTrack.begin(); markerItr != inMarkerTrack.end(); ++markerItr)
				{
					const CottonwoodMarker &marker = markerItr->second;

					dvatemporalxmp::XMPTemporalMetadataInfo markerInfo = CreateTemporalMetadataInfo(marker);

					// restore keywords to subclip markers
					MarkerIDToKeywordsMap::iterator it = markerIDToKeywords.find(marker.GetGUID());
					if (it != markerIDToKeywords.end())
					{
						for (dvatemporalxmp::XMPTemporalMetadataInfo::XMPKeywordListIter keywordItr = (*it).second.begin();
							keywordItr != (*it).second.end();
							++keywordItr)
						{
							markerInfo.AddKeyword(*keywordItr);
						}
					}

					track->AddMarker(markerInfo);
				}
			}

			return true;
		}

		/*
		**
		*/
		dvamediatypes::TickTime GetMaxMarkerOutPoint(BE::IMasterClipRef inMasterClip)
		{
			if (inMasterClip)
			{
				return GetMinMarkerInPoint(inMasterClip) + inMasterClip->GetMaxUntrimmedDuration(BE::kMediaType_Any);
			}
			else
			{
				return dvamediatypes::kTime_Zero;
			}
		}

		/*
		**
		*/
		dvamediatypes::TickTime GetMinMarkerInPoint(BE::IMasterClipRef inMasterClip)
		{
			if (inMasterClip)
			{
				dvamediatypes::TickTime startTime;
				dvamediatypes::TickTime endTime;
				bool isHardBoundary;

				inMasterClip->GetHardOrSoftContentBoundaries(startTime, endTime, isHardBoundary);
				if (isHardBoundary)
				{
					return startTime;
				}
			}

			return dvamediatypes::kTime_Zero;
		}

		/*
		** Restore CCM download location to default place
		*/
		ASL::String RestoreCCMDownLoadLocationToDefault()
		{
			dvacore::UTF16String fullPath;
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			ASL::String premiereDocuments = ASL::MakeString(ASL::kUserDocumentsDirectory);
			fullPath = ASL::DirectoryRegistry::FindDirectory(premiereDocuments);
			if (!fullPath.empty())
			{
				fullPath = ASL::PathUtils::CombinePaths(fullPath, kCreativeCloudDownloadLocationFolderName);
				fullPath = ASL::PathUtils::AddTrailingSlash(fullPath);
			}
			bProp->SetValue(kCreativeCloudDownloadLocation, fullPath, BE::kPersistent);

			return fullPath;
		}

		/*
		**
		*/
		ASL::String GetCCMDownLoadLocation()
		{
			dvacore::UTF16String fullPath;
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			bProp->GetValue(kCreativeCloudDownloadLocation, fullPath);

			if (fullPath.empty())
			{
				fullPath = RestoreCCMDownLoadLocationToDefault();
			}

			return fullPath;
		}

		ASL::String ConvertProviderIDToFormat(ASL::String const& inProviderID)
		{
			if (inProviderID.empty())
			{
				return kSingleFileFormat;
			}
			ASL::String::size_type pos = inProviderID.rfind(DVA_STR("."));
			if (pos == ASL::String::npos)
			{
				DVA_ASSERT_MSG(0, "Wrong provider format.");
				return kSingleFileFormat;
			}

			ASL::String keyPart = inProviderID.substr(pos + 1);
			ASL::String provider = DVA_STR("Provider");
			pos = keyPart.find(provider);
			if (pos == ASL::String::npos)
			{
				DVA_ASSERT_MSG(0, "Wrong provider format.");
				return keyPart;
			}

			ASL::String namePart = keyPart.substr(pos + provider.size());
			if (namePart == DVA_STR("Directory"))
				return kSingleFileFormat;
			return namePart;
		}

		bool IsXMPStemEqual(const XMPText& inNewXMPData, const XMPText& inOldXMPData)
		{
			ASL::StdString newXMPString = *inNewXMPData.get();
			ASL::StdString oldXMPString = *inOldXMPData.get();
			bool result(false);
			try
			{
				SXMPMeta newXMPMeta(newXMPString.c_str(), static_cast<XMP_StringLen>(newXMPString.length()));
				SXMPMeta oldXMPMeta(oldXMPString.c_str(), static_cast<XMP_StringLen>(oldXMPString.length()));

				RemoveVariableFields(newXMPMeta);
				RemoveVariableFields(oldXMPMeta);


				newXMPMeta.SerializeToBuffer(&newXMPString);
				oldXMPMeta.SerializeToBuffer(&oldXMPString);

				return (newXMPString == oldXMPString);
			}
			catch(...)
			{

			}

			return result;
		}

		// Moved from MZ::Utilities


		static bool IsValidDate(
			XMP_DateTime* inDate)
		{
			if (!inDate)
			{
				return false;
			}

			bool timeZoneOK = false;		
			if(inDate->tzSign == 0)
				timeZoneOK = ((inDate->tzHour == 0) && (inDate->tzMinute == 0));
			else
				timeZoneOK = ((inDate->tzHour != 0) || (inDate->tzMinute != 0));

			return inDate->year > 1970
				&& 1 <= inDate->month && inDate->month <= 12
				&& 1 <= inDate->day && inDate->day <= 31
				&& 0 <= inDate->hour && inDate->hour <= 23
				&& 0 <= inDate->minute && inDate->minute <= 59
				&& 0 <= inDate->second && inDate->second <= 59
				&& timeZoneOK;
		}

		static bool DateToString(
			XMP_DateTime* inDate,
			dvacore::StdString& outString)
		{
			if (!inDate)
			{
				outString = dvacore::StdString();
				return false;
			}

			// The XMP date-time string parsing code allows +-00:00 hours as a time zone offset, 
			// but the code that converts an XMP_DateTime rec to a user string does not. Setting 
			// the time zone offset sign to zero when the minute/hour offset is zero avoids the 
			// issue.
			if ((inDate->tzHour == 0) && (inDate->tzMinute == 0))
				inDate->tzSign = 0;

			if (!IsValidDate(inDate))
			{
				return false;
			}

			try
			{
				SXMPUtils::ConvertToLocalTime(inDate);
			}
			catch(XMP_Error& err)
			{
				ASL_UNUSED(err);
				// For some xmp date such as 1970-01-01T00:00Z, ConvertToLocalTime() will throw XMP_Error exception
				// but in this case, we should simply display the string without any format.
				tm time =
				{
					inDate->second,
					inDate->minute,
					inDate->hour,
					inDate->day,
					inDate->month - 1,
					inDate->year - 1900
				};
				outString = ASL::MakeStdString(IngestUtils::CreateDateString(time));
				return true;
			}
			catch(...)
			{
				outString = dvacore::StdString();
				return false;
			}

			tm time =
			{
				inDate->second,
				inDate->minute,
				inDate->hour,
				inDate->day,
				inDate->month - 1,
				inDate->year - 1900
			};

			static const int kBufferSize = 255;
			char buffer[kBufferSize] = {0};
			strftime(
				buffer,
				kBufferSize,
				"%x, %X",
				&time); 

			outString.assign(buffer, kBufferSize);
			return true;
		}

		bool ExtractDateFromXMP(XMPText const& inXMP, 
			const char* const inNamespace, 
			const char* const inProperty,
			ASL::Date::ExtendedTimeType& outDate)
		{
			if (!inXMP)
			{
				return false;
			}

			SXMPMeta xmpMeta(inXMP->c_str(), static_cast<XMP_StringLen>(inXMP->length()));

			// Get shotDate
			try
			{
				XMP_DateTime dateTime;
				if (xmpMeta.GetProperty_Date(inNamespace, inProperty, &dateTime, 0))
				{
					SXMPUtils::ConvertToUTCTime(&dateTime);

					outDate.tm_year = dateTime.year - 1900;
					outDate.tm_mon = dateTime.month - 1;
					outDate.tm_mday = dateTime.day;
					outDate.tm_hour = dateTime.hour;
					outDate.tm_min = dateTime.minute;
					outDate.tm_sec = dateTime.second;
					outDate.tm_wday = 0;		
					outDate.tm_yday = 0;	
					outDate.tm_isdst = 0;
					return true;
				}
			}
			catch (...)
			{
			}

			return false;
		}

		ASL::Date XMPDateTimeToASLDate(const XMP_DateTime& inXMPTime)
		{
			XMP_DateTime xmpDateTime = inXMPTime;

			if (!IsValidDate(&xmpDateTime))
			{
				return ASL::kDate_Invalid;
			}

			try
			{
				// when we create ASL::Date with tm, ASL::Date will treat it as local time, so before this,
				//	we need to convert date time to local version
				SXMPUtils::ConvertToLocalTime(&xmpDateTime);
			}
			catch(...)
			{
				// For some xmp date such as 1970-01-01T00:00Z, ConvertToLocalTime() will throw XMP_Error exception
				// we will treat it as invalid date time.
				return ASL::kDate_Invalid;
			}

			// local time
			std::tm time =
			{
				xmpDateTime.second,
				xmpDateTime.minute,
				xmpDateTime.hour,
				xmpDateTime.day,
				xmpDateTime.month - 1,
				xmpDateTime.year - 1900
			};

			return ASL::Date(time);
		}

		ASL::Date GetFileCreationDate(const ASL::String& inFilePath)
		{
			dvacore::filesupport::File file(inFilePath);
			if (!file.Exists())
			{
				return ASL::kDate_Invalid;
			}

			// utc time
			boost::posix_time::ptime pTime = file.Created();

			// Fix bug: 3742589
			// If the file creation date is invalid, the utc_to_local function throws exception 
			// Unhandled std::exception, what: Cannot convert dates prior to Jan 1, 1970 on Mac
			// To fix this, use the invalid date time by default.
			try 
			{
				if ( !pTime.is_not_a_date_time() )
				{
					// local tm time
					boost::posix_time::ptime pLocalTime = 
						boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(pTime);
					std::tm time = to_tm(pLocalTime);

					return ASL::Date(time);
				}

				return ASL::kDate_Invalid;
			}
			catch (const std::exception& e)
			{
				char const* what = e.what(); // Debugger friendly.
				if (what == NULL)
				{
					what = "<Exception:unknown>";
				}
				DVA_ASSERT_MSG(false, "std::exception, what: " << what << std::endl);
			}

			return ASL::kDate_Invalid;
		}

		ASL::Date::ExtendedTimeType ASLStringToDate(ASL::String const& inDateStr)
		{
			ASL::Date::ExtendedTimeType date = {0};

			try
			{
				ASL::StdString dateStr = ASL::MakeStdString(inDateStr);
				XMP_DateTime dateTime;
				SXMPUtils::ConvertToDate(dateStr, &dateTime);

				SXMPUtils::ConvertToUTCTime(&dateTime);

				date.tm_year = dateTime.year - 1900;
				date.tm_mon = dateTime.month - 1;
				date.tm_mday = dateTime.day;
				date.tm_hour = dateTime.hour;
				date.tm_min = dateTime.minute;
				date.tm_sec = dateTime.second;
				date.tm_wday = 0;		
				date.tm_yday = 0;	
				date.tm_isdst = 0;
			}
			catch (...)
			{
			}

			return date;
		}

		ASL::String GetMetadataCreateDate(XMPText const& inXMP)
		{
			if (!inXMP)
			{
				return ASL::String();
			}

			SXMPMeta xmpMeta(inXMP->c_str(), static_cast<XMP_StringLen>(inXMP->length()));
			XMP_DateTime xmpCreateDate;

			try
			{
				if ( !xmpMeta.GetProperty_Date(kXMP_NS_DM, "shotDate", &xmpCreateDate, 0) )
				{
					xmpMeta.GetProperty_Date(kXMP_NS_XMP, "CreateDate", &xmpCreateDate, 0);
				}
			}
#ifdef DEBUG
			catch(XMP_Error& err)
			{
				XMP_StringPtr message = err.GetErrMsg();
				DVA_ASSERT_MSG(false, "Could not Get CreateDate from Metadata: " << message);
				return ASL::String();
			}
#endif
			catch(...)
			{
				return ASL::String();
			}

			dvacore::StdString createDateStr;
			if (DateToString(&xmpCreateDate, createDateStr))
			{
				return ASL::MakeString(createDateStr);
			}
			else
			{
				return ASL::String();
			}
		}

		//ASL::DateToString will always return English date even if in other language systems on Mac,
		//So this function will return date format: MM/DD/YY, HH:MM:SS, no matter under which language, win or mac
		ASL::String ASLDateToString(ASL::Date const& inDate)
		{
			if (inDate == ASL::kInvalidDate || inDate == ASL::kDate_Invalid)
			{
				return ASL::String();
			}

			std::tm time;
			inDate.GetLocalTime(time);

			static const int kBufferSize = 255;
			char buffer[kBufferSize] = {0};
			strftime(
				buffer,
				kBufferSize,
				"%x, %X",
				&time); 

			return ASL::MakeString(buffer);
		}



		void RegisterDBMetadataProvider()
		{
			static bool isRegistered = false;
			if (isRegistered)
				return;

			ML::CustomMetadata::CustomProvider customProvider;
			customProvider.mBrowserID = kDBMetadataProviderID;
			customProvider.mReadFn = ReadMetadataForPathFn;
			customProvider.mWriteFn = WriteMetadataForPathFn;
			customProvider.mRequestInfoFn = RequestInfoForAssetFn;
			customProvider.mGetReadOnlyFn = GetReadOnlyStatusForPathFn;
			customProvider.mGetModTimeFn = GetModTimeForPathFn;
			ML::CustomMetadata::TemporaryRegisterCustomMetadataProvider(customProvider);
			isRegistered = true;
		}

		void RegisterDBMetadataProviderForPath(ASL::String const& inPath)
		{
			ML::CustomMetadata::TemporaryRegisterCustomMetadataProviderForPath(inPath, kDBMetadataProviderID);
		}

		void UnregisterDBMetadataProvider()
		{
			ML::CustomMetadata::TemporaryRemoveCustomMetadataProvider(kDBMetadataProviderID);
		}


		XMPText ReadXMPFromPhysicalFile(const ASL::String& inFilePath)
		{
			try
			{
				SXMPFiles docMeta;
				SXMPMeta xmpMeta;
				std::string buffer;
				MF::BinaryData metaDataBuffer;
				bool docFileOpen = docMeta.OpenFile(
					reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFilePath).c_str()), 
					kXMP_UnknownFile,
					kXMPFiles_OpenForRead | kXMPFiles_OpenUseSmartHandler | kXMPFiles_OpenLimitedScanning);

				if (docFileOpen)
				{
					docMeta.GetXMP(&xmpMeta);
				}
				xmpMeta.SerializeToBuffer(&buffer);
				metaDataBuffer = MF::BinaryData(buffer.c_str(), buffer.size());

				return XMPText(new ASL::StdString(metaDataBuffer.GetData(),metaDataBuffer.size()));
			}
			catch (XMP_Error & xmpErr)
			{
				DVA_ASSERT_MSG(false, "XMP threw exception ID " << xmpErr.GetID() << ", \"" << xmpErr.GetErrMsg() << "\"");
			}
			catch (std::exception& stdErr)
			{
				DVA_ASSERT_MSG(false, "XMP threw std exception: \"" << stdErr.what() << "\"");
			}
			catch (...)
			{
				DVA_ASSERT_MSG(false, "XMP threw unknown exception");
			}
			return XMPText();
		}

		bool IsFolderBasedClip(const ASL::String& inClipPath)
		{
			try
			{
				SXMPFiles docMeta;
				bool docFileOpen = docMeta.OpenFile(
					reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inClipPath).c_str()),
					kXMP_UnknownFile,
					kXMPFiles_OpenForRead | kXMPFiles_OpenUseSmartHandler | kXMPFiles_OpenLimitedScanning);
				XMP_OptionBits formatFlags = 0;
				if (docMeta.GetFileInfo(0, 0, 0, &formatFlags))
				{
					return (formatFlags & kXMPFiles_FolderBasedFormat) != 0;
				}
			}
			catch (...)
			{
			}
			return false;
		}

		ASL::String GetXMPPath(const ASL::String& inClipPath)
		{
			std::vector<dvacore::StdString> metadataFiles;
			try
			{
				SXMPFiles::GetAssociatedResources(
					reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inClipPath).c_str()),
					&metadataFiles);
			}
			catch(...)
			{
				return ASL::String();
			}

			if (metadataFiles.empty()) // xmp SDK bug?
			{
				// Right now this result is as designed. SXMPFiles::GetAssociatedResources will only return additional resources.
				//DVA_ASSERT_MSG(0, "SXMPFiles::GetAssociatedResources get nothing without exception?! The clip path is: " << inClipPath);
				return inClipPath;
			}
			else // // embedded xmp or side-car xmp file
			{
				// XMP sdk will NOT guarantee xmp file path is the first one in result list
				BOOST_FOREACH (const dvacore::StdString& resourcePath, metadataFiles)
				{
					// [TODO] for spanned clip, there are might more than one xmp files, but right now let's skip it.
					ASL::String path = ASL::MakeString(resourcePath);
					if (dvacore::utility::CaseInsensitive::StringEquals(ASL::PathUtils::GetExtensionPart(path), ASL_STR(".xmp")))
					{
						return path;
					}
				}
			}

			return ASL::String();
		}

		ASL::PathnameList GetClipSupportedPathList(const ASL::String& inClipPath)
		{
			std::vector<dvacore::StdString> metadataFiles;
			try
			{
				SXMPFiles::GetAssociatedResources(
					reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inClipPath).c_str()),
					&metadataFiles);
			}
			catch(...)
			{
			}

			ASL::PathnameList pathList;
			ASL::String primaryClipPath = MZ::Utilities::NormalizePathWithoutUNC(inClipPath);
			bool includePrimaryClipPath = false;
			BOOST_FOREACH (const dvacore::StdString& filePath, metadataFiles)
			{
				ASL::String utf16Path = MZ::Utilities::NormalizePathWithoutUNC(ASL::MakeString(filePath));
				pathList.push_back(utf16Path);
				if (!includePrimaryClipPath && utf16Path == primaryClipPath)
				{
					includePrimaryClipPath = true;
				}
			}
			if (!includePrimaryClipPath)
			{
				pathList.push_back(primaryClipPath);
			}
			return pathList;
		}

		void MergePathList(
			ASL::PathnameList& ioResultList,
			const ASL::PathnameList& inAddedList,
			const ASL::PathnameList& inExceptList)
		{
			std::set<ASL::String> existNormalizedPaths;
			BOOST_FOREACH (const ASL::String& path, ioResultList)
			{
				existNormalizedPaths.insert(dvacore::utility::FileUtils::NormalizePathForStringComparison(ASL::PathUtils::ToNormalizedPath(path)));
			}
			BOOST_FOREACH (const ASL::String& path, inExceptList)
			{
				existNormalizedPaths.insert(dvacore::utility::FileUtils::NormalizePathForStringComparison(ASL::PathUtils::ToNormalizedPath(path)));
			}
			BOOST_FOREACH (const ASL::String& path, inAddedList)
			{
				ASL::String normalizedPath = dvacore::utility::FileUtils::NormalizePathForStringComparison(ASL::PathUtils::ToNormalizedPath(path));
				if (existNormalizedPaths.find(normalizedPath) == existNormalizedPaths.end())
				{
					existNormalizedPaths.insert(normalizedPath);
					ioResultList.push_back(path);
				}
			}
		}

		ASL::String ReplaceFilePartOfPath(const ASL::String& inPath, const ASL::String& inNewName)
		{
			ASL::String drivePart, directoryPart, fileName, extension;
			ASL::PathUtils::SplitPath(inPath, &drivePart, &directoryPart, &fileName, &extension);
			return ASL::PathUtils::MakePath(drivePart, directoryPart, inNewName, extension);
		}

		SuppressPeakFileGeneration::SuppressPeakFileGeneration()
		{
			mSuppressPeakFileGeneration = ML::GetSuppressPeakFileGeneration();
			ML::SuppressPeakFileGeneration(true);
		}
		SuppressPeakFileGeneration::~SuppressPeakFileGeneration()
		{
			ML::SuppressPeakFileGeneration(mSuppressPeakFileGeneration);
		}

		SuppressAudioConforming::SuppressAudioConforming()
		{
			mSuppressAudioConforming = ML::GetSuppressAudioConforming();
			ML::SuppressAudioConforming(true);
		}
		SuppressAudioConforming::~SuppressAudioConforming()
		{
			ML::SuppressAudioConforming(mSuppressAudioConforming);
		}

		bool EnoughDiskSpaceForUpdatingXMP(const ASL::String& inXMPFilePath, size_t inNewXMPSize)
		{
			return (ASL::PathUtils::GetFreeDriveSpace(ASL::PathUtils::GetFullDirectoryPart(inXMPFilePath)) > (ASL::UInt64)inNewXMPSize*4);
		}

		bool IsSideCarFileWritable(const ASL::String& inXMPFilePath)
		{
			// If the xmp file has been exist, check if it's writable.
			if (ASL::PathUtils::ExistsOnDisk(inXMPFilePath))
			{
				bool xmpFileWritable = !ASL::PathUtils::IsReadOnly(inXMPFilePath);
				if (!xmpFileWritable)
				{
					return false;
				}
			}
			// else check if we has permission to create file in its parent folder.
			else
			{
				const ASL::String parentFolder = ASL::PathUtils::GetFullDirectoryPart(inXMPFilePath);
				bool parentFolderWritable = !ASL::PathUtils::IsReadOnly(parentFolder);
				if (!parentFolderWritable)
				{
					return false;
				}
			}
			return true;
		}

		bool CanUpdateMediaFileXMP(const ASL::String& inPath, size_t inNewXMPSize, ASL::String* outErrorMessage)
		{
			// check offline first
			if (!ASL::PathUtils::ExistsOnDisk(inPath))
			{
				if (outErrorMessage != NULL)
				{
					(*outErrorMessage) = dvacore::ZString(
						"$$$/Prelude/MZ/Utilities/CanUpdateMediaFileXMP/OfflineMedia=The media @0 doesn't exist.",
						inPath);
				}
				return false;
			}

			// Check if the file support xmp.
			if (!ML::MetadataManager::PathSupportsXMP(inPath))
			{
				if (outErrorMessage != NULL)
				{
					(*outErrorMessage) = dvacore::ZString(
						"$$$/Prelude/MZ/Utilities/CanUpdateMediaFileXMP/NotSupportXMP=The file @0 doesn't support XMP.",
						inPath);
				}
				return false;
			}

			// check if xmp is writable
			ASL::String xmpPath = GetXMPPath(inPath);
			if (xmpPath.empty() || xmpPath == inPath)
			{
				xmpPath = inPath;
				if (ASL::PathUtils::IsReadOnly(inPath))
				{
					if (outErrorMessage != NULL)
					{
						(*outErrorMessage) = dvacore::ZString(
							"$$$/Prelude/MZ/Utilities/CanUpdateMediaFileXMP/MediaFileReadonly=The media file @0 isn't writable.",
							inPath);
					}
					return false;
				}
			}
			else if (!IsSideCarFileWritable(xmpPath))
			{
				if (outErrorMessage != NULL)
				{
					(*outErrorMessage) = dvacore::ZString(
						"$$$/Prelude/MZ/Utilities/CanUpdateMediaFileXMP/XMPFileReadonly=No permission to create or write the xmp file @0.",
						xmpPath);
				}
				return false;
			}

			// check disk space
			if (inNewXMPSize != 0 && !EnoughDiskSpaceForUpdatingXMP(xmpPath, inNewXMPSize))
			{
				if (outErrorMessage != NULL)
				{
					(*outErrorMessage) = dvacore::ZString(
						"$$$/Prelude/MZ/Utilities/CanUpdateMediaFileXMP/NoEnoughDiskSpace=No enough disk space for updating xmp file @0.",
						xmpPath);
				}
				return false;
			}

			return true;
		}

		ASL::String RenameSideCarFile(
			const ASL::String& inSideCarPath,
			const ASL::String& inOriginalClipName,
			const ASL::String& inNewClipName)
		{
			ASL::String newName = ASL::PathUtils::GetFilePart(inSideCarPath);
			dvacore::UTF16String::size_type subStrPos = newName.find(inOriginalClipName, 0);
			// all side car xmp files should has name begin with original clip file name.
			if (subStrPos != dvacore::UTF16String::npos && subStrPos == 0)
			{
				newName.replace(subStrPos, inOriginalClipName.length(), inNewClipName);
				return ReplaceFilePartOfPath(inSideCarPath, newName);
			}
			else
			{
				return inSideCarPath;
			}
		}

		void GetImportableFileFilters(ML::FileFilterMap& outFilters)
		{
			ML::ImportableFileTypeUtils::GetFileExtensionSets(outFilters);
			//ML::IMediaModuleVector importers;
			//ML::IImporterFactoryRef importerFactory(ASL::CreateClassInstanceRef(kImporterFactoryClassID));
			//importerFactory->GetAudioVideoImporters(importers);

			//ML::IMediaModuleVector importersAudioOnly;
			//importerFactory->GetAudioOnlyImporters(importersAudioOnly);
			//importers.insert(importers.begin(), importersAudioOnly.begin(), importersAudioOnly.end());

			//ASL::String extensionPrefix(dvacore::config::Localizer::Get()->GetLocalizedString("$$$/MediaCore/MediaLayer/Utils/ExtensionPrefix=*."));
			//ASL::String extensionSeparator(dvacore::config::Localizer::Get()->GetLocalizedString("$$$/MediaCore/MediaLayer/Utils/ExtensionSeparator=;"));

			//for (ML::IMediaModuleVector::const_iterator moduleIt = importers.begin();
			//	moduleIt != importers.end(); 
			//	++moduleIt)
			//{
			//	ML::FileDescVector fileTypes;
			//	(*moduleIt)->GetSupportedFileTypes(fileTypes);
			//	ML::FileDescVector::iterator end = fileTypes.end();
			//	for (ML::FileDescVector::iterator it = fileTypes.begin(); it != end; ++it)
			//	{
			//		ASL::SInt32 flags = (*it).mFlags;
			//		if ( !(flags & xfIsMovie || flags & xfIsSound) )
			//		{
			//			// we need the importer, but exclude the types we dont need
			//			continue;
			//		}

			//		ASL::String moduleExtensions = (*it).mExtensions;
			//		if (!moduleExtensions.empty())
			//		{
			//			ML::ExtensionSet extensions;
			//			ASL::Tokenizer<ASL::String> extensionTokenizer(moduleExtensions, extensionPrefix);
			//			while (extensionTokenizer.HasMoreTokens())
			//			{
			//				ASL::String extensionToAdd = extensionTokenizer.NextToken();
			//				if (!extensionToAdd.empty())
			//				{
			//					if (extensionToAdd.find(extensionSeparator) == extensionToAdd.size()-1)
			//					{
			//						extensionToAdd.erase(extensionToAdd.size()-1);
			//					}
			//					extensions.insert(extensionToAdd);
			//				}
			//			}

			//			// Delete the extension part, if it exists.
			//			ASL::String uiDescription = (*it).mDescription;
			//			ASL::String extensionSeparator = ASL_STR("(");
			//			const size_t pos = uiDescription.find_first_of(extensionSeparator);
			//			if (pos != ASL::String::npos)
			//			{
			//				uiDescription.erase(pos, uiDescription.size()-pos);
			//			}

			//			// If a filter descriptor with the same description string is already in the list, concat the
			//			// extension list to the existing set. Otherwise, add the filter descriptor.
			//			if (!uiDescription.empty() && !extensions.empty())
			//			{

			//				ML::FileFilterMap::iterator filterMapIter = outFilters.find(uiDescription);
			//				if (filterMapIter == outFilters.end())
			//				{
			//					ML::FilterDescriptor descriptor;
			//					descriptor.mExtensions = extensions;
			//					descriptor.mFileTypes.insert((*it).mFileType);
			//					outFilters[uiDescription] = descriptor;
			//				}
			//				else
			//				{
			//					(*filterMapIter).second.mExtensions.insert(extensions.begin(), extensions.end());
			//					(*filterMapIter).second.mFileTypes.insert((*it).mFileType);
			//				}
			//			}
			//		}
			//	}
			//}
		}

		void GetImportableFileDesc(
			ML::FileFilterMap const& inFileFilters, 
			ML::ExtensionSet& outExtensions,
			ML::FileTypes& outFileTypes)
		{
			BOOST_FOREACH(ML::FileFilterMap::value_type const& eachFileFilter, inFileFilters)
			{
				ML::FilterDescriptor const& filterDesc = eachFileFilter.second;
				outExtensions.insert(filterDesc.mExtensions.begin(), filterDesc.mExtensions.end());
				outFileTypes.insert(filterDesc.mFileTypes.begin(), filterDesc.mFileTypes.end());
			}
		}

		void AsyncCallFromMainThreadWithDelay(
			const boost::function<void()>& inFn,
			unsigned int inDelayInMilliseconds)
		{
			dvacore::threads::AsyncEventLoopAdaptorPtr mainThread = dvacore::threads::GetRegisteredMainThread();
			DVA_ASSERT_MSG(mainThread, "Main thread is not registered.");
			if (mainThread)
			{
				mainThread->CallAsynchronously(inFn, inDelayInMilliseconds);
			}
		}

		ASL::Result AddCuePointsToXMP(const BE::IProjectItemRef inProjectItem,
			SXMPMeta inXmpMetadata, 
			const MZ::CuePointVector& inCuePoints)
		{
			dvatemporalxmp::XMPDocOpsTemporal* docOpsPtr = new dvatemporalxmp::XMPDocOpsTemporal();
			if (docOpsPtr)
			{
				docOpsPtr->OpenXMP(&inXmpMetadata);

				dvatemporalxmp::XMPTrack* textTrack = MZ::GetTranscriptTrack(docOpsPtr, MZ::ProjectItemHelpers::GetProjectItemFrameRate(inProjectItem));
				if (textTrack)
				{
					dvacore::UTF16String speaker = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/MZ/SpeakerUnknown=Speaker Unknown");			// default value

					// Add each of the cue points to the XMP
					// The format is under section 5.2 on this page:  https://zerowing.corp.adobe.com/display/soundbooth/F32170
					MZ::CuePointVector::const_iterator iter, endIter = inCuePoints.end();
					for (iter = inCuePoints.begin() ; iter != endIter ; ++iter)
					{
						MZ::CuePointParamVector params = (*iter).params;

						dvamediatypes::TickTime durationTime;
						int confidence = dvametadataui::kUnknownConfidence;

						// First determine if this is a speaker change event
						bool speakerChange = MZ::IsSpeakerChanging(params);
						if (speakerChange)
						{
							speaker = MZ::GetSpeakerName(*iter, params);
						}
						else
						{
							// Now set the parameters
							durationTime = MZ::GetDuration(params);
							confidence = MZ::GetConfidence(params);

							// Add the marker
							dvametadataui::WordInfo		word(
								DVA_STR(""),					// Will we get a primary alternative?
								DVA_STR(""),					// Will we get a secondary alternative?
								(*iter).name,
								confidence,
								dvamediatypes::TickTime::TicksToTime((*iter).time.GetTicks()),
								durationTime,	
								speaker);

							textTrack->AddMarker(dvametadataui::WordToMarker(word));
						}
					}
				}

				docOpsPtr->TrackListChanged();				// Tag the track dirty
				docOpsPtr->NoteChange(						// Add a change history note
					reinterpret_cast<XMP_StringPtr>(MakePathChangePart(kTracksPath).c_str()));
				docOpsPtr->PrepareForSave("");				// Flush to the XMP
				delete docOpsPtr;
			}

			return ASL::kSuccess;
		}

		ASL::Result AssociateCuePointsToMasterClip(const BE::IProjectItemRef inProjectItem, const MZ::CuePointVector& inCuePoints)
		{
			PL::CottonwoodMarkerList newMarkerList;

			MZ::CuePointVector::const_iterator iter, endIter = inCuePoints.end();

			// default speaker name: Speaker Unknown
			dvacore::UTF16String speaker = dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/MZ/SpeakerUnknown=Speaker Unknown");

			for (iter = inCuePoints.begin() ; iter != endIter ; ++iter)
			{
				const MZ::CuePoint & cuePoint = *iter;

				MZ::CuePointParamVector params = cuePoint.params;

				// First determine if this is a speaker change event
				bool speakerChange = MZ::IsSpeakerChanging(params);
				if (speakerChange)
				{
					speaker = MZ::GetSpeakerName(cuePoint, params);
				}
				else
				{
					dvamediatypes::TickTime durationTime;
					int confidence = dvametadataui::kUnknownConfidence; // default confidence -1

					// Now set the parameters
					durationTime = MZ::GetDuration(params);
					confidence = MZ::GetConfidence(params);

					PL::CottonwoodMarker marker;
					marker.SetType(MarkerType::kSpeechMarkerType);
					marker.SetDuration(durationTime);
					marker.SetStartTime(dvamediatypes::TickTime::TicksToTime(cuePoint.time.GetTicks()));
					marker.SetName(cuePoint.name);
					marker.SetSpeaker(speaker);
					marker.SetProbability(dvacore::utility::NumberToString<int>(confidence));
					newMarkerList.push_back(marker);
				}
			}


			BE::IClipProjectItemRef clipProjItem(inProjectItem);
			ASL::AsyncCallFromMainThread(boost::bind(
				&PL::AssociateMarkers,
				newMarkerList, 
				clipProjItem->GetMasterClip(),
				false));

			return ASL::kSuccess;
		}

		ASL::Result AddContentAnalysisToXMP(
			SXMPMeta inXmpMetadata, 
			const MZ::CAPerformedVector& inCAPerformedVector)
		{
			if (inCAPerformedVector.size())
			{
				// Register any namespaces we'll need
				std::string registerPrefix;
				bool registeredNamespacePrefix = SXMPMeta::RegisterNamespace(kXMP_NS_CONTENT_ANALYSIS, kXMP_NS_CONTENT_ANALYSIS_Prefix, &registerPrefix);
				DVA_ASSERT_MSG(registeredNamespacePrefix == true, "Namespace Prefix doesn't match");

				XMP_DateTime currentDateTime;
				TXMP_STRING_TYPE currentDateTimeString;
				TXMPUtils<TXMP_STRING_TYPE>::CurrentDateTime(&currentDateTime);
				TXMPUtils<TXMP_STRING_TYPE>::ConvertFromDate(currentDateTime, &currentDateTimeString);

				MZ::CAPerformedVector::const_iterator iter, endIter = inCAPerformedVector.end();
				for (iter = inCAPerformedVector.begin() ; iter != endIter ; ++iter)
				{
					MZ::CAPerformed ca = *iter;
					TXMP_STRING_TYPE analyzerItemPath;

					try
					{
						inXmpMetadata.AppendArrayItem (
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Bag,
							kXMP_PropArrayIsOrdered,			// XMP_OptionBits
							0,
							kXMP_PropValueIsStruct);			// XMP_OptionBits

						SXMPUtils::ComposeArrayItemPath(
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Bag,
							kXMP_ArrayLastItem,
							&analyzerItemPath);

						// Set the analysis type
						dvacore::UTF8String caType8 = dvacore::utility::UTF16to8(ca.type);
						dvacore::StdString caType(caType8.begin(), caType8.end());
						inXmpMetadata.SetStructField(
							kXMP_NS_CONTENT_ANALYSIS,
							analyzerItemPath.c_str() ,
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Type,
							caType.c_str());

						// Set the version
						dvacore::UTF8String caVersion8 = dvacore::utility::UTF16to8(ca.version);
						dvacore::StdString caVersion(caVersion8.begin(),caVersion8.end());
						inXmpMetadata.SetStructField(
							kXMP_NS_CONTENT_ANALYSIS,
							analyzerItemPath.c_str() ,
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Version,
							caVersion.c_str());

						// Set the manufacturer
						dvacore::UTF8String caManufacturer8 = dvacore::utility::UTF16to8(ca.manufacturer);
						dvacore::StdString caManufacturer(caManufacturer8.begin(), caManufacturer8.end());
						inXmpMetadata.SetStructField(
							kXMP_NS_CONTENT_ANALYSIS,
							analyzerItemPath.c_str() ,
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Manufacturer,
							caManufacturer.c_str());

						// Set the current date/time
						inXmpMetadata.SetStructField(
							kXMP_NS_CONTENT_ANALYSIS,
							analyzerItemPath.c_str() ,
							kXMP_NS_CONTENT_ANALYSIS,
							kXMP_Property_Analyzer_Performed,
							currentDateTimeString);
					}
					catch(XMP_Error& inError)
					{
						XMP_Int32 errorCode = inError.GetID();
						dvacore::UTF16String errorString = dvacore::utility::UTF8to16(dvacore::utility::CharAsUTF8(inError.GetErrMsg()));
					}
				}
			}

			return ASL::kSuccess;
		}

		ASL::Result AddContentAnalysisToXMPToCoreMemoryCache(const BE::IProjectItemRef inProjectItem,
			const MZ::CAPerformedVector& inCAPerformedVector)
		{
			BE::IClipProjectItemRef clipProjItem(inProjectItem);
			ASL::String clipPath = BE::MasterClipUtils::GetMediaInstanceString(clipProjItem->GetMasterClip());
			PL::AssetMediaInfoPtr oldAssetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(clipPath, false);

			if(oldAssetMediaInfo)
			{
				XMPText xmpText = oldAssetMediaInfo->GetXMPString();
				SXMPMeta xmpMetadata;
				xmpMetadata.ParseFromBuffer(xmpText->c_str(), static_cast<XMP_StringLen>(xmpText->length()));

				AddContentAnalysisToXMP(xmpMetadata, inCAPerformedVector);

				ASL::StdString newXmpStr;
				xmpMetadata.SerializeToBuffer(&newXmpStr);
				XMPText newXmpText(new ASL::StdString(newXmpStr));


				ASL::String fileCreateTime;
				ASL::String fileModifyTime;

				fileCreateTime = SRLibrarySupport::GetFileCreateTime(oldAssetMediaInfo->GetMediaPath());
				fileModifyTime = SRLibrarySupport::GetFileModifyTime(oldAssetMediaInfo->GetMediaPath());

				AssetMediaInfoList assetMediaInfoList;
				AssetMediaInfoPtr newAssetMediaInfo = AssetMediaInfo::Create(
					oldAssetMediaInfo->GetAssetMediaType(),
					oldAssetMediaInfo->GetAssetMediaInfoGUID(),
					oldAssetMediaInfo->GetMediaPath(),
					newXmpText,
					oldAssetMediaInfo->GetFileContent(),
					oldAssetMediaInfo->GetCustomMetadata(),
					fileCreateTime,
					fileModifyTime,
					oldAssetMediaInfo->GetRateTimeBase(),
					oldAssetMediaInfo->GetRateNtsc(),
					oldAssetMediaInfo->GetAliasName(),
					oldAssetMediaInfo->GetNeedSavePreCheck());

				bool discardChanges = false;
				bool result = SRUtilitiesPrivate::PreCheckBeforeSendSaveMsg(clipPath, newAssetMediaInfo, discardChanges, true);

				if (result)
				{
					//	Refresh XMP data in core side
					SRProject::GetInstance()->GetAssetMediaInfoWrapper(clipPath)->RefreshMediaXMP(newAssetMediaInfo->GetXMPString());

					//	Send Save msg to keep file and xmp in sync.
					assetMediaInfoList.push_back(newAssetMediaInfo);
					SRProject::GetInstance()->GetAssetLibraryNotifier()->SaveAssetMediaInfo(
						ASL::Guid::CreateUnique(),
						ASL::Guid::CreateUnique(),
						assetMediaInfoList,
						PL::AssetItemPtr()); // No in/out marker change, so asset item is not necessary.
				}
			}

			return ASL::kSuccess;
		}

		/*
		**
		*/
		BE::IProjectItemRef CreateSequenceItemFromMasterClip(
			const BE::IMasterClipRef& inMasterClip,
			const BE::IProjectRef& inProject,
			const BE::IProjectItemRef& inContainingBin,
			MZ::SequenceAudioTrackRule inUseType,
			const ASL::String& inSequenceName/* = ASL_STR("")*/)
		{

			BE::ISequencePresetRef sequencePreset = BE::MasterClipUtils::GetContentType(inMasterClip) == BE::kContentType_Sequence
			? CreateSequencePresetFromSequence(BE::MasterClipUtils::GetSequence(inMasterClip), inProject, MZ::kSequenceAudioTrackRule_Logging == inUseType)
			: CreateSequencePresetFromMedia(inMasterClip, MZ::kSequenceAudioTrackRule_Logging == inUseType);

			return CreateSequenceItemFromPreset(
				inMasterClip,
				inProject,
				inContainingBin,
				sequencePreset,
				inUseType,
				inSequenceName);
		}

		void ExtendFolderPaths(const ASL::String& inPath, ASL::PathnameList& outSubFilePaths)
		{
			std::vector<ASL::String> containedPaths;
			try
			{
				ASL::Directory dir = ASL::Directory::ConstructDirectory(inPath);
				// The maximum number of files to be copied, transcoded and imported should be limited
				// because this API is a recursive call which could be never exit till 2 << 32.
				dir.GetContainedPaths(containedPaths, true, 2 << 16 );
			}
			catch (...)
			{
				// Construct directory failed, then do nothing.
				return;
			}

			BOOST_FOREACH (const ASL::String& path, containedPaths)
			{
				if (!ASL::PathUtils::IsDirectory(path) || ASL::PathUtils::IsDirectoryEmpty(path))
				{
					outSubFilePaths.push_back(path);
				}
			}
		}

		ASL::String GetRelativePath(const ASL::String& inRootPath, const ASL::String& inFullChildPath)
		{
			DVA_ASSERT_MSG(ASL::PathUtils::IsDirectory(inRootPath), "Get relative from a file path.");
			ASL::String fullChildPath(inFullChildPath);
			ASL::String relativePath;
			const bool isRelative = MZ::Utilities::ArePathsRelative(
				inRootPath,
				true,
				fullChildPath,
				ASL::PathUtils::IsDirectory(fullChildPath),
				relativePath);
			DVA_ASSERT_MSG(isRelative, "Selected path is not relative path of current selected root folder!");

			if (!ASL::PathUtils::IsDirectory(fullChildPath))
			{
				relativePath = ASL::PathUtils::AddTrailingSlash(relativePath) + ASL::PathUtils::GetFullFilePart(inFullChildPath);
			}

			return relativePath;
		}

		/*
		**
		*/
		ASL::String ConvertTimecodeToString(
			const dvamediatypes::TickTime& inTimeCode,
			const dvamediatypes::FrameRate& inFrameRate,
			bool inIsDropFrame)
		{
			ASL::Timecode timeCodeTemp(inTimeCode, inFrameRate, MZ::Utilities::GetTimeDisplayForFrameRate(inFrameRate, inIsDropFrame));
			timeCodeTemp.SetTime(inTimeCode);
			return timeCodeTemp.GetString();
		}	// end of PL::ConvertTimecodeToString

		/*
		** Get filename without extension
		** Input example: aa.com.cn.avi
		** Output:
		** ASL::PathUtils::GetFilePart		aa
		** PL::Utilities::GetFileNamePart	aa.com.cn
		*/
		ASL::String GetFileNamePart(const ASL::String& inFullPath)
		{
			ASL::String::size_type dotPos = inFullPath.find_last_of(ASL_STR("."));
			if (dotPos != ASL::String::npos)
			{
				return inFullPath.substr(0, dotPos);
			}
			return inFullPath;
		}

		/*
		**
		*/
		dvamediatypes::TimeDisplay ConvertStringToTimeDisplay(const ASL::String& inTimeDisplayString)
		{
			if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("24Timecode")))
			{
				return dvamediatypes::kTimeDisplay_24Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("25Timecode")))
			{
				return dvamediatypes::kTimeDisplay_25Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("2997DropTimecode")))
			{
				return dvamediatypes::kTimeDisplay_2997DropTimecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("2997NonDropTimecode")))
			{
				return dvamediatypes::kTimeDisplay_2997NonDropTimecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("30Timecode")))
			{
				return dvamediatypes::kTimeDisplay_30Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("50Timecode")))
			{
				return dvamediatypes::kTimeDisplay_50Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("5994DropTimecode")))
			{
				return dvamediatypes::kTimeDisplay_5994DropTimecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("5994NonDropTimecode")))
			{
				return dvamediatypes::kTimeDisplay_5994NonDropTimecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("60Timecode")))
			{
				return dvamediatypes::kTimeDisplay_60Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("Frames")))
			{
				return dvamediatypes::kTimeDisplay_Frames;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("23976Timecode")))
			{
				return dvamediatypes::kTimeDisplay_23976Timecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("16mmFeetFrames")))
			{
				return dvamediatypes::kTimeDisplay_16mmFeetFrames;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("35mmFeetFrames")))
			{
				return dvamediatypes::kTimeDisplay_35mmFeetFrames;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("AudioSamplesTimecode")))
			{
				return dvamediatypes::kTimeDisplay_AudioSamplesTimecode;
			}
			else if (dvacore::utility::StringEquals(inTimeDisplayString, ASL_STR("AudioMsTimecode")))
			{
				return dvamediatypes::kTimeDisplay_AudioMsTimecode;
			}

			DVA_ASSERT_MSG(0, "Invalid TimeDisplay String!!");
			return dvamediatypes::kTimeDisplay_Invalid;
		}

		/*
		**
		*/
		ASL::String ConvertTimeDisplayToString(dvamediatypes::TimeDisplay inTimeDisplay)
		{
			switch(inTimeDisplay)
			{
			case dvamediatypes::kTimeDisplay_24Timecode:
				return ASL_STR("24Timecode");
				break;
			case dvamediatypes::kTimeDisplay_25Timecode:
				return ASL_STR("25Timecode");
				break;
			case dvamediatypes::kTimeDisplay_2997DropTimecode:
				return ASL_STR("2997DropTimecode");
				break;
			case dvamediatypes::kTimeDisplay_2997NonDropTimecode:
				return ASL_STR("2997NonDropTimecode");
				break;
			case dvamediatypes::kTimeDisplay_30Timecode:
				return ASL_STR("30Timecode");
				break;
			case dvamediatypes::kTimeDisplay_50Timecode:
				return ASL_STR("50Timecode");
				break;
			case dvamediatypes::kTimeDisplay_5994DropTimecode:
				return ASL_STR("5994DropTimecode");
				break;
			case dvamediatypes::kTimeDisplay_5994NonDropTimecode:
				return ASL_STR("5994NonDropTimecode");
				break;
			case dvamediatypes::kTimeDisplay_60Timecode:
				return ASL_STR("60Timecode");
				break;
			case dvamediatypes::kTimeDisplay_Frames:
				return ASL_STR("Frames");
				break;
			case dvamediatypes::kTimeDisplay_23976Timecode:
				return ASL_STR("23976Timecode");
				break;
			case dvamediatypes::kTimeDisplay_16mmFeetFrames:
				return ASL_STR("16mmFeetFrames");
				break;
			case dvamediatypes::kTimeDisplay_35mmFeetFrames:
				return ASL_STR("35mmFeetFrames");
				break;
			case dvamediatypes::kTimeDisplay_AudioSamplesTimecode:
				return ASL_STR("AudioSamplesTimecode");
				break;
			case dvamediatypes::kTimeDisplay_AudioMsTimecode:
				return ASL_STR("AudioMsTimecode");
				break;
			case dvamediatypes::kTimeDisplay_Invalid:
			default:
				ASL_ASSERT_MSG(false, "Invalid TimeDisplay Format");
				return ASL_STR("InvalidTimecode");
				break;
			}
		}

		/*
		**
		*/
		dvamediatypes::FrameRate GetDefaultVideoFrameRate()
		{
			if (MZ::Locale::IsCurrentInputLocaleNTSC())
			{
				return dvamediatypes::kFrameRate_VideoNTSC;
			}
			else
			{
				return dvamediatypes::kFrameRate_VideoPAL;
			}
		}

		/*
		**
		*/
		XMPText GetXMPTextWithAltTimecodeField(XMPText inXMPText, const BE::IMasterClipRef& inMasterClip)
		{
			if (!inXMPText)
			{
				return XMPText();
			}

			if (!inMasterClip)
			{
				return inXMPText;
			}

			BE::IContentRef content = BE::MasterClipUtils::GetContent(inMasterClip, BE::kMediaType_Any, 0);
			if (!content)
			{
				return inXMPText;
			}
			dvamediatypes::TickTime mediaStart = content->GetOffset();

			dvamediatypes::FrameRate videoFrameRate;
			BE::IClipRef videoClip = inMasterClip->GetClip(BE::kMediaType_Video, 0);
			if (videoClip)
			{
				videoFrameRate = videoClip->GetFrameRate();
			}
			else
			{
				videoFrameRate = Utilities::GetDefaultVideoFrameRate();
			}

			bool isDropFrame = MZ::Utilities::IsDropFrame(inMasterClip);
			dvamediatypes::TimeDisplay videoTimeDisplay = 
				MZ::Utilities::GetTimeDisplayForFrameRate(videoFrameRate, isDropFrame);

			SXMPMeta xmpMetadata(inXMPText->c_str(), (XMP_StringLen)inXMPText->length());

			ASL::StdString timecodeString = 
				ASL::MakeStdString(Utilities::ConvertTimecodeToString(mediaStart, videoFrameRate, isDropFrame));
			ASL::StdString timeFormatString = 
				ASL::MakeStdString(Utilities::ConvertTimeDisplayToString(videoTimeDisplay));

			ASL::StdString existAltTimecodeString, existAltTimeFormatString;
			XMP_StringLen stringLen;
			xmpMetadata.GetStructField(kXMP_NS_DM, Utilities::kAltTimecode, kXMP_NS_DM, Utilities::kTimeValue, &existAltTimecodeString, &stringLen);
			xmpMetadata.GetStructField(kXMP_NS_DM, Utilities::kAltTimecode, kXMP_NS_DM, Utilities::kTimeFormat, &existAltTimeFormatString, &stringLen);
			if (existAltTimecodeString != timecodeString || existAltTimeFormatString != timeFormatString)
			{
				xmpMetadata.SetStructField(kXMP_NS_DM, kAltTimecode, kXMP_NS_DM, kTimeValue, timecodeString.c_str());
				xmpMetadata.SetStructField(kXMP_NS_DM, kAltTimecode, kXMP_NS_DM, kTimeFormat, timeFormatString.c_str());
			}

			ASL::StdString fileXMPBuffer;
			xmpMetadata.SerializeToBuffer(&fileXMPBuffer);
			return XMPText(new ASL::StdString(fileXMPBuffer));
		}

		bool AreFilePathsEquivalent(
			const dvacore::UTF16String& inFilePath1,
			const dvacore::UTF16String& inFilePath2)
		{
			try
			{
				if (ASL::CaseInsensitive::StringEquals(inFilePath1, inFilePath2))
				{
					return true;
				}
				boost::filesystem::path filePath1 = GetBoostFileSystemPath(inFilePath1);
				boost::filesystem::path filePath2 = GetBoostFileSystemPath(inFilePath2);

				return boost::filesystem::equivalent(filePath1, filePath2);
			}
			catch (std::exception& e)
			{
				DVA_ASSERT_MSG(false, "Failed to compare file paths due to an exception " << e.what());
			}

			return false;
		}



		/*
		**
		*/
		dvamediatypes::TickTime ConvertDayTimeToTick(ASL::Date::ExtendedTimeType const& inTime)
		{
			int seconds = inTime.tm_hour * 3600 + inTime.tm_min * 60 + inTime.tm_sec;
			return dvamediatypes::TickTime( double(seconds) );
		}

		bool IsXMPWritable(const ASL::String& inPath)
		{
			ASL::String clipPath = inPath;

			bool xmpPathWritable = false;
			bool supportXMP = true;
			if (UIF::IsEAMode())
			{
				xmpPathWritable = !PL::Utilities::IsAssetPathOffline(clipPath);
			}
			else
			{
				ASL::String xmpPath = Utilities::GetXMPPath(clipPath);
				if (xmpPath.empty())
				{
					xmpPath = clipPath;
				}
				
				if (AreFilePathsEquivalent(xmpPath, clipPath))
				{
					xmpPathWritable = !ASL::PathUtils::IsReadOnly(clipPath);
				}
				else
				{
					try
					{
						supportXMP = SXMPFiles::IsMetadataWritable(
							reinterpret_cast<const char*>(dvacore::utility::UTF16to8(clipPath).c_str()),
							&xmpPathWritable);
					}
					catch(...)
					{
						supportXMP = false;
					}
				}
			}
			return supportXMP && xmpPathWritable;
		}
		/*
		**
		*/
		ASL::String MakeTempPresetFilePath()
		{
			return ASL::File::MakeUniqueTempPath(
				ASL::PathUtils::AddTrailingSlash(ASL::PathUtils::GetTempDirectory()) + ASL_STR("PreludePreset.epr"));
		}

		/*
		**
		*/
		bool SavePresetToFile(EncoderHost::IPresetRef inPreset, const ASL::String& inPath)
		{
			const ASL::Result result = EncoderHost::FileBasedPresetHandler::WritePresetToFile(inPreset, inPath);
			return ASL::ResultSucceeded(result);
		}

		/*
		**
		*/
		ASL::String SavePresetToFile(EncoderHost::IPresetRef inPreset)
		{
			DVA_ASSERT(inPreset != NULL);
			ASL::String presetPath(PL::Utilities::MakeTempPresetFilePath());
			ASL::Result saveResult = PL::Utilities::SavePresetToFile(inPreset, presetPath);
			if (ASL::ResultFailed(saveResult))
			{
				presetPath.clear();
			}
			return presetPath;
		}

		EncoderHost::IPresetRef CreateCustomPresetForSequence(BE::ISequenceRef inSequence, ML::IExporterModuleRef inExportModule)
		{
			EncoderHost::IPresetRef presetRef;
			if (inSequence)
			{
				EncoderHost::IEncoderFactoryRef	encoderFactory(ASL::CreateClassInstanceRef(kEncoderFactoryClassID));

				EncoderHost::IEncoderRef encoder = encoderFactory->CreateEncoderForFormat(inSequence, MZ::GetProject(), inExportModule);
				encoder->SavePreset(presetRef);

				presetRef->SetPresetName(dvacore::config::Localizer::Get()->GetLocalizedString("$$$/Prelude/EncoderHost/CustomPresetName=Custom"));
			}
			return presetRef;
		}

		EncoderHost::IPresetRef CreateCustomPresetForMedia(const ASL::String& inMediaPath, ML::IExporterModuleRef inExportModule)
		{
			EncoderHost::IEncoderFactoryRef	encoderFactory(ASL::CreateClassInstanceRef(kEncoderFactoryClassID));

			BE::ISequenceRef sequence;
			encoderFactory->CreateSequenceFromFileMedia(inMediaPath, sequence);

			return CreateCustomPresetForSequence(sequence, inExportModule);
		}

		EncoderHost::IPresetRef CreateCustomPresetForAssetItem(const AssetItemPtr& inAssetItem, ML::IExporterModuleRef inExportModule)
		{
			if (inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_RoughCut)
			{
				SRRoughCutRef rc = SRRoughCut::Create(inAssetItem);
				rc->CreateSequence();
				return CreateCustomPresetForSequence(rc->GetBESequence(), inExportModule);
			}
			else if (inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_MasterClip ||
				inAssetItem->GetAssetMediaType() == PL::kAssetLibraryType_SubClip)
			{
				return CreateCustomPresetForMedia(inAssetItem->GetMediaPath(), inExportModule);
			}
			else
			{
				DVA_ASSERT_MSG(0, "Unsupported media type");
			}
			return EncoderHost::IPresetRef();
		}

		/*
		**
		*/
		bool IsMediaOffline(const BE::IMasterClipRef& inMasterClip)
		{
			// we only consider media-based masterclip here.
			BE::IMediaRef media = BE::MasterClipUtils::GetMedia(inMasterClip);
			return media ? media->IsOffline() : false;
		}

		dvamediatypes::FrameRate GetStillFrameRate()
		{
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);
			dvamediatypes::FrameRate frameRate = dvamediatypes::kFrameRate_VideoNTSC;
			bProp->GetValue(BE::kPrefsStillImagesDefaultFramerate, frameRate);
			return frameRate;
		}

		bool StillIsDropFrame()
		{
			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);
			bool isDropFrame = false;
			bProp->GetValue(BE::kPrefsStillImagesDefaultIsDropFrame, isDropFrame);
			return isDropFrame;
		}

		/*
		**
		*/
		dvamediatypes::FrameRate GetVideoFrameRate(const BE::IMasterClipRef& inMasterClip)
		{
			BE::IMediaRef media = BE::MasterClipUtils::GetDeprecatedMedia(inMasterClip);
			if (media)
			{
				BE::IClipRef videoClip = inMasterClip->GetClip(BE::kMediaType_Video, 0);
				if (videoClip)
				{
					return videoClip->GetFrameRate();
				}
			}

			return dvamediatypes::kFrameRate_Zero;
		}

		/*
		**
		*/
		dvamediatypes::TickTime GetMasterClipMaxDuration(const BE::IMasterClipRef& inMasterClip)
		{
			return (MZ::BEUtilities::IsMasterClipStill(inMasterClip) ? 
				inMasterClip->GetMaxTrimmedDuration(BE::kMediaType_Any) :
				inMasterClip->GetMaxCurrentUntrimmedDuration(BE::kMediaType_Any));
		}

		ASL::String GetDisplayStringOfFileSize(const dvacore::filesupport::FileSizeT& inFileSize)
		{
#ifdef DVA_OS_MAC
			const dvacore::filesupport::FileSizeT oneKB = 1000uLL;
			const dvacore::filesupport::FileSizeT oneMB = 1000000uLL;
			const dvacore::filesupport::FileSizeT oneGB = 1000000000uLL;
			const dvacore::filesupport::FileSizeT oneTB = 1000000000000uLL;
			const dvacore::filesupport::FileSizeT onePB = 1000000000000000uLL;
#else
			const dvacore::filesupport::FileSizeT oneKB = 1uLL << 10;
			const dvacore::filesupport::FileSizeT oneMB = 1uLL << 20;
			const dvacore::filesupport::FileSizeT oneGB = 1uLL << 30;
			const dvacore::filesupport::FileSizeT oneTB = 1uLL << 40;
			const dvacore::filesupport::FileSizeT onePB = 1uLL << 50;
#endif

			const char* stringTemplate = NULL;
			double numberValue = 0.0;

			if (inFileSize < oneMB)
			{
				stringTemplate = "$$$/Prelude/PLCore/FileSizeKB=@0 KB";
				numberValue = inFileSize/(double)oneKB;
			}
			else if (inFileSize < oneGB)
			{
				stringTemplate = "$$$/Prelude/PLCore/FileSizeMB=@0 MB";
				numberValue = inFileSize/(double)oneMB;
			}
			else if (inFileSize < oneTB)
			{
				stringTemplate = "$$$/Prelude/PLCore/FileSizeGB=@0 GB";
				numberValue = inFileSize/(double)oneGB;
			}
			else if (inFileSize < onePB)
			{
				stringTemplate = "$$$/Prelude/PLCore/FileSizeTB=@0 TB";
				numberValue = inFileSize/(double)oneTB;
			}
			else
			{
				stringTemplate = "$$$/Prelude/PLCore/FileSizePB=@0 PB";
				numberValue = inFileSize/(double)onePB;
			}

			std::ostringstream ss;
			ss << std::fixed << std::setprecision(2);
			ss << numberValue;
			std::string s = ss.str();
			
			return dvacore::ZString(
				stringTemplate,
				ASL::MakeString(s));
		}

		dvacore::filesupport::FileSizeT GetFileSize(const ASL::String& inPath)
		{
			try
			{
				return dvacore::filesupport::File(inPath).Size();
			}
			catch(...)
			{
				return (dvacore::filesupport::FileSizeT)0;
			}
		}


		dvamediatypes::TickTime TimecodeToTickTimeWithoutRoundingFrameRate(
			const ASL::String& inTimecodeString, 
			dvamediatypes::FrameRate inFrameRate, 
			dvamediatypes::TimeDisplay inTimeDisplay)
		{
			if (inTimeDisplay != dvamediatypes::kTimeDisplay_2997NonDropTimecode &&
				inTimeDisplay != dvamediatypes::kTimeDisplay_5994NonDropTimecode &&
				inTimeDisplay != dvamediatypes::kTimeDisplay_23976Timecode)
			{
				return dvamediatypes::TimecodeToTime(inTimecodeString, inFrameRate, inTimeDisplay);
			}

			ASL::UInt32 hours, minutes, seconds, frames;
			TimecodeStringToHMSF(inTimecodeString, false, hours, minutes, seconds, frames);

			ASL::UInt64 totalSeconds = hours * 3600 + minutes * 60 + seconds;
			return dvamediatypes::TickTime::TicksToTime(
				totalSeconds * dvamediatypes::kTicksPerSecond + frames * inFrameRate.GetTicksPerFrame());
		}

		ASL::String TickTimeToTimecodeWithoutRoundingFrameRate(
			dvamediatypes::TickTime inTime, 
			dvamediatypes::FrameRate inFrameRate, 
			dvamediatypes::TimeDisplay inTimeDisplay)
		{
			if (inTimeDisplay != dvamediatypes::kTimeDisplay_2997NonDropTimecode &&
				inTimeDisplay != dvamediatypes::kTimeDisplay_5994NonDropTimecode &&
				inTimeDisplay != dvamediatypes::kTimeDisplay_23976Timecode)
			{
				return dvamediatypes::TimeToTimecode(inTime, inFrameRate, inTimeDisplay);
			}

			ASL::UInt32 hours = 0, minutes = 0, seconds = 0, frames = 0;
			ASL::UInt64 totalSeconds = inTime.GetTicks() / dvamediatypes::kTicksPerSecond;
			ASL::UInt64 secondsLeft = totalSeconds;
			while (secondsLeft >= 3600)
			{
				++hours;
				secondsLeft -= 3600;
			}
			while (secondsLeft >= 60)
			{
				++minutes;
				secondsLeft -= 60;
			}
			seconds = secondsLeft;

			frames = (inTime.GetTicks() - totalSeconds * dvamediatypes::kTicksPerSecond) / inFrameRate.GetTicksPerFrame();

			ASL::String hourStr = ConvertToTimecodeSectionString(hours);
			ASL::String minuteStr = ConvertToTimecodeSectionString(minutes);
			ASL::String secondStr = ConvertToTimecodeSectionString(seconds);
			ASL::String frameStr = ConvertToTimecodeSectionString(frames);
			ASL::String delimiter = ASL_STR(":");

			return hourStr + delimiter + minuteStr + delimiter + secondStr + delimiter + frameStr;
		}

		dvaui::drawbot::ColorRGBA ConvertUINT32ToColorRGBA(std::uint32_t inUINT32ColorRGBA)
		{
			unsigned char red = static_cast<unsigned char>(inUINT32ColorRGBA) & 0xff;
			unsigned char green = static_cast<unsigned char>(inUINT32ColorRGBA >> 8) & 0xff;
			unsigned char blue = static_cast<unsigned char>(inUINT32ColorRGBA >> 16) & 0xff;
			unsigned char alpha = static_cast<unsigned char>(inUINT32ColorRGBA >> 24) & 0xff;

			return dvaui::drawbot::ColorRGBA(red, green, blue, alpha);
		}

		std::uint32_t ConvertColorRGBAToUINT32(const dvaui::drawbot::ColorRGBA& inColorRGBA)
		{
			unsigned char red = static_cast<unsigned char>(std::floor(inColorRGBA.Red() * 255.0f + 0.5f));
			unsigned char green = static_cast<unsigned char>(std::floor(inColorRGBA.Green() * 255.0f + 0.5f));
			unsigned char blue = static_cast<unsigned char>(std::floor(inColorRGBA.Blue() * 255.0f + 0.5f));
			unsigned char alpha = static_cast<unsigned char>(std::floor(inColorRGBA.Alpha() * 255.0f + 0.5f));

			std::uint32_t val_UInt32 = static_cast<std::uint32_t>(alpha << 24) |
										static_cast<std::uint32_t>(blue << 16) |
										static_cast<std::uint32_t>(green << 8) |
										static_cast<std::uint32_t>(red);

			return val_UInt32;
		}

		dvaui::drawbot::ColorRGBA ConvertStringToColorRGBA(const dvacore::StdString& inColorRGBAString)
		{
			// 1. string to uint32:
			std::uint32_t val_UInt32 = strtoul(inColorRGBAString.c_str(), NULL, 16);

			// 2. uint32 to RGBA as 0xaabbggrr
			return ConvertUINT32ToColorRGBA(val_UInt32);
		}

		dvacore::StdString ConvertColorRGBAToString(const dvaui::drawbot::ColorRGBA& inColorRGBA)
		{
			// 1. RGBA to uint32 as 0xaabbggrr
			std::uint32_t val_UInt32 = ConvertColorRGBAToUINT32(inColorRGBA);

			// 2. uint32 to string as hexadecimal format
			char buf[32];
			std::sprintf(buf, "0x%x", val_UInt32);
			return buf;
		}

		PL::CottonwoodMarkerPtr NewMarker(
			const PL::CottonwoodMarker& inTemplateMarker, 
			dvamediatypes::TickTime inPoint, 
			dvamediatypes::TickTime duration, 
			bool inIsSilent, 
			MarkerTypeMap& ioPreviousMarkerMap)
		{
			PL::CottonwoodMarkerPtr retMarkerPtr;

			PL::ISRPrimaryClipPlaybackRef primaryClipPlayback = 
				PL::ModulePicker::GetInstance()->GetCurrentModuleData();
			PL::ISRLoggingClipRef srLoggingClip(primaryClipPlayback);

			BE::ISequenceRef theSequence = 
				srLoggingClip ? primaryClipPlayback->GetBESequence() : BE::ISequenceRef();

			if (!theSequence)
			{
				DVA_ASSERT_MSG(0, "Should be a logging clip!!");
				return retMarkerPtr;
			}

			ML::IPlayerRef player;
			ASL::Result result = MZ::GetSequencePlayer(
				theSequence, MZ::GetProject(), ML::kPlaybackPreference_AudioVideo, player);

			// get time
			dvamediatypes::TickTime clipStart;
			dvamediatypes::TickTime clipDuration;
			dvamediatypes::TickTime	markerDuration;
			dvamediatypes::TickTime markerTime(player->GetCurrentPosition());

			PL::ISRMarkerSelectionRef markerSelectionRef(
				PL::ModulePicker::GetInstance()->GetCurrentModuleData());
			DVA_ASSERT(markerSelectionRef);

			PL::CottonwoodMarker newMarker(inTemplateMarker, true);
			dvacore::UTF16String newMarkerType(newMarker.GetType());

			dvacore::UTF16String speechStr(dvacore::utility::AsciiToUTF16("Speech"));
			if (speechStr == newMarkerType) 
			{
				if (newMarker.GetSpeaker().empty())
					newMarker.SetSpeaker(dvacore::utility::AsciiToUTF16("speaker"));
				if (newMarker.GetProbability().empty())
					newMarker.SetProbability(dvacore::utility::AsciiToUTF16("99"));
			}

			dvacore::UTF16String flvCuePointStr(dvacore::utility::AsciiToUTF16("FLVCuePoint"));
			if (flvCuePointStr == newMarkerType) 
			{
				newMarker.SetCuePointType(DVA_STR("Event"));
			}

			// find if a clip exists
			PL::ClipDurationMap				   clipDurationMap;
			PL::ClipDurationMap::TrackItemInfo trackIteminfo;
			clipDurationMap.SetSequence(theSequence);
			BE::IMasterClipRef currentMasterClip = clipDurationMap.GetClipStartAndDurationFromSequenceTime(markerTime, clipStart, clipDuration);
			newMarker.SetMarkerOwner(PL::ISRMarkerOwnerRef(srLoggingClip->GetSRClipItem()->GetSRMedia()));
			clipDurationMap.GetTrackItemInfoFromSequenceTime(markerTime, trackIteminfo);

			if (NULL == currentMasterClip)
			{
				DVA_ASSERT_MSG(false, "Healthy Assert: currentMasterClip should never be empty if it is open in timeline. Undo has to be disabled in this case!");
				return retMarkerPtr;
			}

			if (PL::MarkerType::kInOutMarkerType == newMarkerType && newMarker.GetName().empty())
			{
				PL::ISRMarkersRef markers = primaryClipPlayback->GetMarkers(currentMasterClip);
				PL::MarkerTrack const& subclipMarkers = markers->GetMarkerTrack(PL::MarkerType::kInOutMarkerType);
				newMarker.SetName(PL::Utilities::BuildSubClipName(subclipMarkers));
			}

			dvamediatypes::TickTime offsetTime = (dvamediatypes::kTime_Invalid == inPoint) ? 
													(markerTime - clipStart) :
													inPoint; 

			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);

			bool isPlaying = (player->GetCurrentPlayMode() == ML::kPlayMode_Playing);

			if (isPlaying)
			{
				ASL::SInt32 markerPrerollTime = 0;
				bProp->GetValue(PL::kPropertyKey_MarkerPreroll, markerPrerollTime);
				dvamediatypes::TickTime markerPrerollTickTime(static_cast<double> (markerPrerollTime));
				offsetTime = std::max(dvamediatypes::kTime_Zero, offsetTime - markerPrerollTickTime);
			}

			dvamediatypes::TickTime maxMarkerDuration = (dvamediatypes::kTime_Invalid == duration) ? 
														(clipDuration - offsetTime) : 
														duration;

			ASL::UInt32 setDuration = 0;
			BE::PropertyKey typeKeySetDuration = BE_MAKEPROPERTYKEY(PL::kPropertyKey_SetMarkerDurationByTypeBase.ToString() + newMarkerType);
			bProp->GetValue(typeKeySetDuration, setDuration);

			if (0 == setDuration)
			{
				markerDuration = maxMarkerDuration;
			}
			else if (1 == setDuration)
			{
				markerDuration = dvamediatypes::TickTime(5.0);
			}
			else if (2 == setDuration)
			{
				markerDuration = dvamediatypes::TickTime(10.0);
			}
			else if (3 == setDuration)
			{
				ASL::UInt32 duration = 2;
				BE::PropertyKey typeKeyDuration = BE_MAKEPROPERTYKEY(PL::kPropertyKey_MarkerDurationByTypeBase.ToString() + newMarkerType);
				bProp->GetValue(typeKeyDuration, duration);

				double durationDouble = static_cast<double>(duration);
				if (0 > durationDouble)
				{
					markerDuration = maxMarkerDuration;
				}
				else
				{
					dvamediatypes::TickTime defaultDuration(durationDouble);
					markerDuration = defaultDuration;
				}
			}
			if (markerDuration > maxMarkerDuration)
			{
				markerDuration = maxMarkerDuration;
			}

			if (markerDuration < dvamediatypes::kTime_Zero)
			{
				return retMarkerPtr;
			}

			newMarker.SetStartTime(offsetTime + trackIteminfo.mContentBoundaryStart);
			newMarker.SetDuration(markerDuration);

			//	If the pref is set and there is a single marker, set its out point to the new in point.
			bool closePreviousMarkerPref = false;
			bProp->GetValue(PL::kPropertyKey_CloseOnNewMarker, closePreviousMarkerPref);

			if (!isPlaying)
			{
				ioPreviousMarkerMap.clear();
			}

            PL::MarkerSet markerSelection = markerSelectionRef->GetMarkerSelection();
			if (closePreviousMarkerPref && isPlaying && markerSelection.size() == 1)
			{
				MarkerTypeMap::iterator pos = ioPreviousMarkerMap.find(newMarker.GetType());
				if (pos != ioPreviousMarkerMap.end())
				{
					//	Only close the new marker if the old marker has an out time before the CTI
					PL::ISRMarkerSelectionRef markerSelectionRef(
						PL::ModulePicker::GetInstance()->GetCurrentModuleData());
					DVA_ASSERT(markerSelectionRef);

					PL::CottonwoodMarker selectedMarker = *(markerSelection.begin());

					PL::ISRMarkersRef markers = primaryClipPlayback->GetMarkers(currentMasterClip);

					PL::CottonwoodMarker oldMarker;
					if (markers != NULL && markers->GetMarker(pos->second, oldMarker))
					{		
						if (oldMarker.GetStartTime() < offsetTime)
						{
							PL::CottonwoodMarker truncatedMarker = oldMarker;
							truncatedMarker.SetDuration(offsetTime - oldMarker.GetStartTime());
							PL::UpdateCottonwoodMarker(currentMasterClip, oldMarker, truncatedMarker);
						}
					}
				}
			}

			bool stopPlaybackPref = false;
			bProp->GetValue(PL::kPropertyKey_StopOnNewMarker, stopPlaybackPref);
			if (stopPlaybackPref)
			{
				player->StopPlayback();
				player->SetPosition(offsetTime + clipStart);
			}

			// assert no clip where there should be one
			DVA_ASSERT(currentMasterClip != NULL);
			if (currentMasterClip != NULL)
			{
				PL::AddCottonwoodMarker(newMarker, currentMasterClip, inIsSilent);

				if (isPlaying)
				{
					ioPreviousMarkerMap[newMarker.GetType()] = newMarker.GetGUID();
				}

				retMarkerPtr.reset(new PL::CottonwoodMarker(newMarker));
			}

			UIF::Headlights::LogEvent("Marker", "AddMarker", ASL::MakeStdString(newMarkerType).c_str());
			return retMarkerPtr;
		}

		/*
		**
		*/
		ASL::String GetInitRoughCutSaveFolder()
		{
			ASL::String initialFolder;
			BE::IPropertiesRef(BE::GetBackend())->GetValue(PL::kLastUsedRoughCutSaveDirKey, initialFolder);
			if (!EnsureDirectoryExist(initialFolder))
			{
				initialFolder.clear();
			}

			// If it is still empty, set user document folder as initial path
			if (initialFolder.empty())
			{
				initialFolder = ASL::PathUtils::CombinePaths(
					ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(ASL::kUserDocumentsDirectory)),
					ASL_STR("Rough Cuts"));
				// Make sure initial directory exist
				if (!EnsureDirectoryExist(initialFolder))
				{
					initialFolder.clear();
				}
			}

			return initialFolder;
		}

		ASL::String GetInitTagTemplateSaveFolder()
		{
			ASL::String initialFolder;
			BE::IPropertiesRef(BE::GetBackend())->GetValue(PL::kPropertyKey_LastTagTemplateDirectory, initialFolder);
			if (!EnsureDirectoryExist(initialFolder))
			{
				initialFolder.clear();
			}

			// If it is still empty, set user document folder as initial path
			if (initialFolder.empty())
			{
				initialFolder = ASL::PathUtils::CombinePaths(
					ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(ASL::kUserDocumentsDirectory)),
					ASL_STR("TagTemplates"));
				// Make sure initial directory exist
				if (!EnsureDirectoryExist(initialFolder))
				{
					initialFolder.clear();
				}
			}

			return initialFolder;
		}

		ASL::Result IsSequenceSafeToEditAsRoughCut(BE::ISequenceRef inSequence)
		{
			if (BE::IsMergedClip(inSequence))
			{
				return eIsMergedClip;
			}
			RoughCutVisitor visitor(inSequence);
			visitor.DoVisit();
			// Now we don't show anything for result. In future maybe we should return ASL result or something else.
			return visitor.GetResult();
		}

		bool IsMarkerInUsed(const ASL::Guid& inMarkerGuid)
		{
			BE::IProjectItemRef projectItem = MZ::BEUtilities::FindProjectItem(MZ::GetProject(), MZ::kPropertyKey_EASubclipID, inMarkerGuid.AsString());
			return (projectItem && BE::ProjectItemUtils::GetMasterClip(projectItem)->HasChildClipsInUse(BE::kMediaType_Any));
		}

		bool NothingSelectedInTimeline()
		{
			PL::ISRPrimaryClipPlaybackRef primaryClipPlayback = PL::ModulePicker::GetInstance()->GetCurrentModuleData();
			if (primaryClipPlayback == NULL)
			{
				return true;
			}

			if (primaryClipPlayback == PL::ModulePicker::GetInstance()->GetRoughCutClip())
			{
				if (primaryClipPlayback->GetBESequence())
				{
					BE::ITrackItemSelectionRef trackSelection = primaryClipPlayback->GetBESequence()->GetCurrentSelection();
					if (trackSelection != NULL)
					{
						BE::ITrackItemGroupRef currentGroup(trackSelection);
						ASL_ASSERT(currentGroup != NULL);
						if (currentGroup->GetItemCount() == 0 ||
							(currentGroup->GetEnd() - currentGroup->GetStart()) <= dvamediatypes::TickTime(0.0))
						{
							return true;
						}
					}
				}
			}
			return false;
		}
        
        /*
        **
        */
		PL::TrackTransitionMap BuildTransitionItems(
			const BE::ISequenceRef& inSequence,	
			BE::MediaType inType)
		{
			if (!inSequence)
			{
				return PL::TrackTransitionMap();
			}

			if (inType != BE::kMediaType_Video && inType != BE::kMediaType_Audio)
			{
				DVA_ASSERT_MSG(0, "Must be a specific type!");
				return PL::TrackTransitionMap();
			}

			PL::TrackTransitionMap trackTransitions;

			BE::ITrackGroupRef clipTrackGroup = inSequence->GetTrackGroup(inType);
			if (clipTrackGroup)
			{
				BE::TrackIndex trackCount = clipTrackGroup->GetClipTrackCount();
				for (BE::TrackIndex index = 0; index < trackCount; ++index)
				{
					BE::IClipTrackRef clipTrack = clipTrackGroup->GetClipTrack(index);
					BE::TrackItemVector transitionItems;
					clipTrack->GetTrackItems(BE::kTrackItemType_Transition, transitionItems);

					for (BE::TrackItemVector::iterator itemIter = transitionItems.begin();
						itemIter != transitionItems.end();
						++itemIter)
					{
						if ((*itemIter)->GetType() == BE::kTrackItemType_Transition)
						{
							// both video and audio will come into this
							PL::TransitionItemPtr transitionItem = 
								ConvertTransitionItem(inSequence, BE::ITransitionTrackItemRef(*itemIter));
							trackTransitions[index].insert(transitionItem);
						}
					}
				}
			}

			return trackTransitions;
		}

	} // Utilities
} // PL
