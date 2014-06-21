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

#include "PLRoughCutUtils.h"
#include "PLRoughCut.h"
#include "PLProject.h"
#include "PLXMLUtilities.h"
#include "PLLibrarySupport.h"
#include "PLBEUtilities.h"

// MZ
#include "MZBEUtilities.h"

// BE
#include "BESequence.h"
#include "BEClip.h"
#include "BE/Sequence/IClipTrack.h"
#include "BE/Sequence/ITrackGroup.h"

// ASL
#include "ASLPathUtils.h"
#include "ASLResultSpaces.h"
#include "ASLResults.h"
#include "ASLFile.h"
#include "ASLCoercion.h"
#include "ASLFrameRate.h"
#include "ASLDiskUtils.h"

// DVA
#include "dvacore/debug/Debug.h"
#include "dvacore/config/Localizer.h"
#include "dvaui/dialogs/UI_MessageBox.h"
#include "UIFWorkspace.h"
#include "dvaworkspace/workspace/Workspace.h"
#include "dvacore/utility/StringCompare.h"

// MediaCore
#include "XMLUtils.h"
#include "xercesc/framework/MemBufInputSource.hpp"

namespace PL
{

namespace
{
	const dvacore::UTF16String kXMEMLTag		= DVA_STR("xmeml");
	const dvacore::UTF16String kNameTag			= DVA_STR("name");
	const dvacore::UTF16String kSequenceTag		= DVA_STR("sequence");
	const dvacore::UTF16String kDurationTag		= DVA_STR("duration");
	const dvacore::UTF16String kRateTag			= DVA_STR("rate");
	const dvacore::UTF16String kNTSCTag			= DVA_STR("ntsc");
	const dvacore::UTF16String kTimebaseTag		= DVA_STR("timebase");
	const dvacore::UTF16String kFileTag			= DVA_STR("file");
	const dvacore::UTF16String kPathURLTag		= DVA_STR("pathurl");
	const dvacore::UTF16String kMediaTag		= DVA_STR("media");
	const dvacore::UTF16String kVideoTag		= DVA_STR("video");
	const dvacore::UTF16String kAudioTag		= DVA_STR("audio");
	const dvacore::UTF16String kTrackTag		= DVA_STR("track");
	const dvacore::UTF16String kClipItemTag		= DVA_STR("clipitem");
	const dvacore::UTF16String kInPointTag		= DVA_STR("in");
	const dvacore::UTF16String kOutPointTag		= DVA_STR("out");
	const dvacore::UTF16String kStartPointTag	= DVA_STR("start");
	const dvacore::UTF16String kEndPointTag		= DVA_STR("end");
	const dvacore::UTF16String kEnabledTag		= DVA_STR("enabled");
	const dvacore::UTF16String kLockedTag		= DVA_STR("locked");

	// transition
	const dvacore::UTF16String kTransitionItemTag		= DVA_STR("transitionitem");
	const dvacore::UTF16String kAlignmentTag			= DVA_STR("alignment");
	const dvacore::UTF16String kCutPointTag				= DVA_STR("cutPoint");
	const dvacore::UTF16String kEffectTag				= DVA_STR("effect");
	const dvacore::UTF16String kEffectIDTag				= DVA_STR("effectid");
	const dvacore::UTF16String kEffectCategoryTag		= DVA_STR("effectcategory");
	const dvacore::UTF16String kEffectTypeTag			= DVA_STR("effecttype");
	const dvacore::UTF16String kMediaTypeTag			= DVA_STR("mediatype");
	const dvacore::UTF16String kWipeCodeTag				= DVA_STR("wipecode");
	const dvacore::UTF16String kWipeAccuracyTag			= DVA_STR("wipeaccuracy");
	const dvacore::UTF16String kStartRatioTag			= DVA_STR("startratio");
	const dvacore::UTF16String kEndRatioTag				= DVA_STR("endratio");
	const dvacore::UTF16String kReverseTag				= DVA_STR("reverse");
	const dvacore::UTF16String kParameterTag			= DVA_STR("parameter");
	const dvacore::UTF16String kParameterIDTag			= DVA_STR("parameterid");
	const dvacore::UTF16String kValueTag				= DVA_STR("value");
	const dvacore::UTF16String kAlphaTag				= DVA_STR("alpha");
	const dvacore::UTF16String kRedTag					= DVA_STR("red");
	const dvacore::UTF16String kGreenTag				= DVA_STR("green");
	const dvacore::UTF16String kBlueTag					= DVA_STR("blue");

	const dvacore::UTF16String kIDAttr			= DVA_STR("id");
	const dvacore::UTF16String kVersionAttr		= DVA_STR("version");

	const dvacore::UTF16String kRoughCutIDStr			= DVA_STR("1649EF72-725E-4d64-9E0B-484C53908136");
	const dvacore::UTF16String kXMLVersionStr			= DVA_STR("1.0");
	const dvacore::UTF16String kXMEMLDocTypeStr			= DVA_STR("xmeml");
	const dvacore::UTF16String kSequenceDurationStr		= DVA_STR("0");
	const dvacore::UTF16String kXMEMLVersionStr			= DVA_STR("4");
	const dvacore::UTF16String kTRUEStr					= DVA_STR("TRUE");
	const dvacore::UTF16String kFALSEStr				= DVA_STR("FALSE");

	const dvacore::UTF16String kDipToColorTransitionStr	= DVA_STR("Dip to Color Dissolve");
	const dvacore::UTF16String kDipToColorParamIDStr	= DVA_STR("dipcolor");
	const dvacore::UTF16String kDipToColorParamNameStr	= DVA_STR("Color");
	const dvacore::UTF16String kMediaTypeAudioStr		= DVA_STR("audio");
	const dvacore::UTF16String kMediaTypeVideoStr		= DVA_STR("video");
	const dvacore::UTF16String kEffectTypeTransitionStr	= DVA_STR("transition");

	const dvacore::UTF16String kCrossDissolveMatchName = ASL_STR("AE.ADBE Cross Dissolve New");
	const dvacore::UTF16String kDipToWhiteMatchName = ASL_STR("ADBE Dip To White");
	const dvacore::UTF16String kDipToBlackMatchName = ASL_STR("ADBE Dip To Black");
	const dvacore::UTF16String kConstantPowerMatchName = ASL_STR("Constant Power");
	const dvacore::UTF16String kConstantGainMatchName = ASL_STR("Constant Gain");

	const dvacore::UTF16String kCrossDissolveEffectName		= ASL_STR("Cross Dissolve");
	const dvacore::UTF16String kCrossDissolveEffectID		= ASL_STR("Cross Dissolve");
	const dvacore::UTF16String kDipColorEffectName			= ASL_STR("Dip to Color Dissolve");
	const dvacore::UTF16String kDipColorEffectID			= ASL_STR("Dip to Color Dissolve");
	const dvacore::UTF16String kDissolveEffectCategory		= ASL_STR("Dissolve");

	const dvacore::UTF16String kConstantPowerEffectName		= ASL_STR("Cross Fade (+3dB)");
	const dvacore::UTF16String kConstantPowerEffectID		= ASL_STR("KGAudioTransCrossFade3dB");
	const dvacore::UTF16String kConstantGainEffectName		= ASL_STR("Cross Fade ( 0dB)");
	const dvacore::UTF16String kConstantGainEffectID		= ASL_STR("KGAudioTransCrossFade0dB");


	template<typename T>
	inline ASL::String ToString(T inValue)
	{
		return ASL::Coercion<ASL::String>::Result(inValue);
	}

	inline  dvamediatypes::TickTime StringToTickTime(
		const ASL::String& inStr,
		const dvamediatypes::FrameRate& inFrameRate)
	{
		return dvamediatypes::TickTime(ASL::Coercion<std::int64_t>::Result(inStr), inFrameRate);
	}

	// copy from \Plugins\ProjectConverters\FCP\Src\Core_StringUtils.cpp
	// convert path to URL format
	std::string PathToURL(
		const std::string& inPath)
	{
		std::string result;
		std::string::const_iterator end = inPath.end();
		for (std::string::const_iterator it = inPath.begin(); it != end; ++it)
		{
			unsigned char ch = *it;
			if (ch == '\\')
			{
				result += "/";
			}
			else if (ch == ' ')
			{
				result += "%20";
			}
			else if ((ch > 127) || (ch <= 32) || (ch == '%') || (ch == ':'))
			{
				char buf[32];
				sprintf(buf, "%%%02x", ch);
				result += buf;
			}
			else
			{
				//	[???] other chars
				result += ch;
			}
		}
#ifdef WIN32
		return "file://localhost/" + result;
#else
		return "file://localhost" + result;
#endif
	}

	// copy from \Plugins\ProjectConverters\FCP\Src\Core_StringUtils.cpp
	// support URLToPath
	bool IsHex(char inC)
	{
		return ((inC >= '0') && (inC <= '9')) ||
			((inC >= 'a') && (inC <= 'f')) ||
			((inC >= 'A') && (inC <= 'F'));
	}

	// copy from \Plugins\ProjectConverters\FCP\Src\Core_StringUtils.cpp
	// support URLToPath
	char FromHex(char inC)
	{
		if ((inC >= '0') && (inC <= '9'))
		{
			return inC - '0';
		}
		if ((inC >= 'a') && (inC <= 'f'))
		{
			return 10 + (inC - 'a');
		}
		if ((inC >= 'A') && (inC <= 'F'))
		{
			return 10 + (inC - 'A');
		}
		return inC;
	}

	// copy from \Plugins\ProjectConverters\FCP\Src\Core_StringUtils.cpp
	// convert path to URL format path to local file system format
	std::string URLToPath(
		const std::string& inURL)
	{
		std::string result(inURL);

#ifdef WIN32
		std::string::size_type pos = result.find("file://localhost/");
#else
		std::string::size_type pos = result.find("file://localhost");
#endif

		if (pos != std::string::npos)
		{
#ifdef WIN32
			result = result.substr(17);
#else
			result = result.substr(16);
#endif
		}
		pos = 0;
		while (true)
		{
			pos = result.find("%", pos);
			if (pos == std::string::npos)
			{
				break;
			}
			std::string::size_type len = result.size();
			DVA_ASSERT(pos <= (len - 3));
			if (pos > (len - 3))
			{
				break;
			}
			char c1 = result[pos + 1];
			char c2 = result[pos + 2];
			DVA_ASSERT(IsHex(c1) && IsHex(c2));
			if (!IsHex(c1) || !IsHex(c2))
			{
				break;
			}
			char c = (FromHex(c1) << 4) + FromHex(c2);
			result = result.substr(0, pos) + c + result.substr(pos + 3);
			pos = pos + 1;
		}

#ifdef WIN32
		pos = 0;
		while (true)
		{
			pos = result.find("/", pos);
			if (pos == std::string::npos)
			{
				break;
			}
			result[pos] = '\\';
		}
#endif
		return result;
	}

	inline ASL::String URLPath(const ASL::String& inPath)
	{
		return ASL::MakeString(PathToURL(ASL::MakeStdString(inPath)));
	}

	inline ASL::String LocalPath(const ASL::String& inPath)
	{
		return ASL::MakeString(URLToPath(ASL::MakeStdString(inPath)));
	}


	/*
	**
	*/
	void AddTransitionItem(
		DOMElement* inTrackNode,
		const PL::TransitionItemPtr& inTransitionItem)
	{
		if (!inTrackNode || !inTransitionItem)
		{
			return;
		}

		DOMElement* transitionItemNode = XMLUtils::AddChild(inTrackNode, kTransitionItemTag.c_str());

		dvamediatypes::FrameRate frameRate = inTransitionItem->GetFrameRate();
		BE::MediaType mediaType = inTransitionItem->GetMediaType();

		std::int64_t startPoint = inTransitionItem->GetStart().ToFrame64(frameRate);
		XMLUtils::AddChild(transitionItemNode, kStartPointTag.c_str(), ToString(startPoint).c_str());
		std::int64_t endPoint = inTransitionItem->GetEnd().ToFrame64(frameRate);
		XMLUtils::AddChild(transitionItemNode, kEndPointTag.c_str(), ToString(endPoint).c_str());

		XMLUtils::AddChild(transitionItemNode, kAlignmentTag.c_str(), inTransitionItem->GetAlignmentTypeString());

		std::int64_t cutPoint = inTransitionItem->GetCutPoint().ToFrame64(frameRate);
		XMLUtils::AddChild(transitionItemNode, kCutPointTag.c_str(), ToString(cutPoint).c_str());

		ASL::String rateTimeBase;
		ASL::String rateNTSC;
		FCPRateToTimebaseAndNtsc(rateTimeBase, rateNTSC, frameRate);
		DOMElement* rateNode = XMLUtils::AddChild(transitionItemNode, kRateTag.c_str());
		XMLUtils::AddChild(rateNode, kTimebaseTag.c_str(), rateTimeBase.c_str());
		XMLUtils::AddChild(rateNode, kNTSCTag.c_str(), rateNTSC.c_str());

		ASL::String matchName = inTransitionItem->GetEffectMatchName();
		ASL::String name;
		ASL::String effectID;
		ASL::String	effectCategory;
		ASL::UInt32 wipeCode = 0;
		ASL::UInt32 wipeAccuracy = 100;
		ASL::UInt32	dipColorAlpha = 255;
		ASL::UInt32 dipColorRed = 0;
		ASL::UInt32 dipColorGreen = 0;
		ASL::UInt32 dipColorBlue = 0;

		if (inTransitionItem->GetMediaType() == BE::kMediaType_Video)
		{
			if (dvacore::utility::StringEquals(matchName, kCrossDissolveMatchName))
			{
				name = kCrossDissolveEffectName;
				effectID = kCrossDissolveEffectID;
				effectCategory = kDissolveEffectCategory;
				wipeCode = 0;
				wipeAccuracy = 100; 
			}
			else if (dvacore::utility::StringEquals(matchName, kDipToWhiteMatchName) ||
				dvacore::utility::StringEquals(matchName, kDipToBlackMatchName))
			{
				name = kDipColorEffectName;
				effectID = kDipColorEffectID;
				effectCategory = kDissolveEffectCategory;
				wipeCode = 0;
				wipeAccuracy = 100; 

				if (dvacore::utility::StringEquals(matchName, kDipToWhiteMatchName))
				{
					dipColorAlpha = 255;
					dipColorRed = 255;
					dipColorGreen = 255;
					dipColorBlue = 255;
				}
				else
				{
					dipColorAlpha = 255;
					dipColorRed = 0;
					dipColorGreen = 0;
					dipColorBlue = 0;
				}
			}
			else
			{
				DVA_ASSERT_MSG(0, "Unknown video transition match name!");
			}
		}
		else
		{
			if (dvacore::utility::StringEquals(matchName, kConstantPowerMatchName))
			{
				name = kConstantPowerEffectName;
				effectID = kConstantPowerEffectID;
				wipeCode = 0;
				wipeAccuracy = 100; 
			}
			else if (dvacore::utility::StringEquals(matchName, kConstantGainMatchName))
			{
				name = kConstantGainEffectName;
				effectID = kConstantGainEffectID;
				wipeCode = 0;
				wipeAccuracy = 100; 
			}
			else
			{
				DVA_ASSERT_MSG(0, "Unknown audio transition match name!");
			}
		}

		DOMElement* effectNode = XMLUtils::AddChild(transitionItemNode, kEffectTag.c_str());
		XMLUtils::AddChild(effectNode, kNameTag.c_str(), name.c_str());
		XMLUtils::AddChild(effectNode, kEffectIDTag.c_str(), effectID.c_str());
		if (mediaType == BE::kMediaType_Video)
		{
			XMLUtils::AddChild(effectNode, kEffectCategoryTag.c_str(), effectCategory.c_str());
		}
		XMLUtils::AddChild(effectNode, kEffectTypeTag.c_str(), kEffectTypeTransitionStr.c_str());
		ASL::String mediatypeString = mediaType == BE::kMediaType_Video ? kMediaTypeVideoStr : kMediaTypeAudioStr;
		XMLUtils::AddChild(effectNode, kMediaTypeTag.c_str(), mediatypeString.c_str());
		XMLUtils::AddChild(effectNode, kWipeCodeTag.c_str(), ToString(wipeCode).c_str());
		XMLUtils::AddChild(effectNode, kWipeAccuracyTag.c_str(), ToString(wipeAccuracy).c_str());
		XMLUtils::AddChild(effectNode, kStartRatioTag.c_str(), ToString(inTransitionItem->GetEffectStartRatio()).c_str());
		XMLUtils::AddChild(effectNode, kEndRatioTag.c_str(), ToString(inTransitionItem->GetEffectEndRatio()).c_str());
		XMLUtils::AddChild(
			effectNode, 
			kReverseTag.c_str(), 
			inTransitionItem->IsEffectReverse() ? kTRUEStr : kFALSEStr);

		if (mediaType == BE::kMediaType_Video &&
			dvacore::utility::StringEquals(effectID, kDipToColorTransitionStr))
		{
			DOMElement* parameterNode = XMLUtils::AddChild(effectNode, kParameterTag.c_str());
			XMLUtils::AddChild(parameterNode, kParameterIDTag.c_str(), kDipToColorParamIDStr.c_str());
			XMLUtils::AddChild(parameterNode, kNameTag.c_str(), kDipToColorParamNameStr.c_str());

			DOMElement* valueNode = XMLUtils::AddChild(parameterNode, kValueTag.c_str());
			XMLUtils::AddChild(valueNode, kAlphaTag.c_str(), ToString(dipColorAlpha).c_str());
			XMLUtils::AddChild(valueNode, kRedTag.c_str(), ToString(dipColorRed).c_str());
			XMLUtils::AddChild(valueNode, kGreenTag.c_str(), ToString(dipColorGreen).c_str());
			XMLUtils::AddChild(valueNode, kBlueTag.c_str(), ToString(dipColorBlue).c_str());
		}
	}

	/*
	**
	*/
	ASL::String GetAudioClipItemUniqueID(
		const ASL::String& inClipItemGUID,
		BE::MediaType inMediaType,
		BE::TrackIndex inTrackIndex)
	{
		if (inMediaType == BE::kMediaType_Video)
		{
			return inClipItemGUID;
		}

		return inClipItemGUID + DVA_STR("-A") + ToString(inTrackIndex);
	}

	/*
	**
	*/
	void AddClipItem(
		DOMElement* inTrackNode, 
		const BE::ITrackItemRef& inTrackItem, 
		const BE::ISequenceRef& inSequence, 
		const PL::SRClipItemPtr& inSRClipItem)
	{
		if (!inTrackNode || !inTrackItem || !inSequence)
		{
			return;
		}

		dvamediatypes::FrameRate frameRate = inSequence->GetVideoFrameRate();

		BE::IMasterClipRef masterClipRef = BE::IMasterClipRef();
		BE::IMediaRef media = BE::IMediaRef();
		BE::IClipRef clip = BE::IClipRef();
		MZ::BEUtilities::GetMediaFromTrackItem(inTrackItem, masterClipRef, media, clip);

		if (!clip || !media)
		{
			DVA_ASSERT_MSG(0, "cannot get BE data!!");
			return;
		}

		ASL::String subclipName, subclipGUID;
		if (inSRClipItem)
		{
			subclipName = inSRClipItem->GetSubClipName();	
			subclipGUID = inSRClipItem->GetSubClipGUID().AsString();	
		}

		ASL::String mediaPath = media->GetInstanceString();
		dvamediatypes::TickTime clipInPoint = clip->GetInPoint();
		dvamediatypes::TickTime itemDuration  = clip->GetTrimmedDuration();
		dvamediatypes::TickTime itemStartPoint = inTrackItem->GetStart();

		// clip item element
		DOMElement* clipItemNode = XMLUtils::AddChild(inTrackNode, kClipItemTag.c_str());

		subclipGUID = GetAudioClipItemUniqueID(
							subclipGUID, 
							inTrackItem->GetMediaType(), 
							inTrackItem->GetTrackIndex());
		clipItemNode->setAttribute(kIDAttr.c_str(), subclipGUID.c_str());

		XMLUtils::AddChild(clipItemNode, kNameTag.c_str(), subclipName);

		std::int64_t startPoint = itemStartPoint.ToFrame64(frameRate);
		XMLUtils::AddChild(clipItemNode, kStartPointTag.c_str(), ToString(startPoint).c_str());
		std::int64_t endPoint = (itemStartPoint + itemDuration).ToFrame64(frameRate);
		XMLUtils::AddChild(clipItemNode, kEndPointTag.c_str(), ToString(endPoint).c_str());

		std::int64_t inPoint = clipInPoint.ToFrame64(frameRate);
		XMLUtils::AddChild(clipItemNode, kInPointTag.c_str(), ToString(inPoint).c_str());
		std::int64_t outPoint = (clipInPoint + itemDuration).ToFrame64(frameRate);
		XMLUtils::AddChild(clipItemNode, kOutPointTag.c_str(), ToString(outPoint).c_str());

		XMLUtils::AddChild(clipItemNode, kFileTag.c_str())->setAttribute(kIDAttr.c_str(), mediaPath.c_str());

		// add link...
	}

	/*
	**
	*/
	void AddAudioTrack(
		DOMElement* inAudioNode,
		const BE::IClipTrackRef& inAudioClipTrack,
		const BE::ISequenceRef& inSequence,
		const PL::SRClipItems& inSRClipItems,
		const PL::TransitionItemSet& inTransitionItems)
	{
		if (!inAudioNode || !inAudioClipTrack || !inSequence)
		{
			return;
		}

		DOMElement* trackNode = XMLUtils::AddChild(inAudioNode, kTrackTag.c_str());
		XMLUtils::AddChild(trackNode, kEnabledTag.c_str(), kTRUEStr.c_str());
		XMLUtils::AddChild(trackNode, kLockedTag.c_str(), kFALSEStr.c_str());

		BE::TrackItemVector trackItems;
		inAudioClipTrack->GetTrackItems(BE::kTrackItemType_Clip, trackItems);

		for (BE::TrackItemVector::iterator trackIter = trackItems.begin();
			trackIter != trackItems.end();
			++trackIter)
		{
			BE::ITrackItemRef theTrackItem = *trackIter;
			if (theTrackItem->GetType() != BE::kTrackItemType_Clip)
			{
				continue;
			}

			BE::ITrackItemRef theFirstLinkedItem = 
				PL::BEUtilities::GetLinkedFirstTrackItem(inSequence, theTrackItem, BE::kMediaType_Any);

			if (theFirstLinkedItem)
			{
				PL::SRClipItemPtr srClipItem;
				SRClipItems::const_iterator iter = inSRClipItems.begin();
				SRClipItems::const_iterator end = inSRClipItems.end();
				for (; iter != end; ++iter)
				{
					if ((*iter)->GetTrackItem() == theFirstLinkedItem)
					{
						srClipItem = *iter;
					}
				}

				AddClipItem(trackNode, theTrackItem, inSequence, srClipItem);
			}
		}

		for (PL::TransitionItemSet::iterator iter = inTransitionItems.begin();
			iter != inTransitionItems.end();
			++iter)
		{
			AddTransitionItem(trackNode, *iter);
		}
	}

	/*
	**
	*/
	void AddAudio(
		DOMElement* inMediaNode, 
		const BE::ISequenceRef& inSequence, 
		const PL::SRClipItems& inSRClipItems)
	{
		if (!inMediaNode || !inSequence)
		{
			return;
		}

		DOMElement* audioNode = XMLUtils::AddChild(inMediaNode, kAudioTag.c_str());

		PL::TrackTransitionMap audioTransitions = 
			PL::Utilities::BuildTransitionItems(inSequence, BE::kMediaType_Audio);

		BE::ITrackGroupRef audioTrackGroup = inSequence->GetTrackGroup(BE::kMediaType_Audio);

		BE::TrackIndex audioTrackCount = audioTrackGroup->GetClipTrackCount();
		for (BE::TrackIndex trackIndex = 0;
			trackIndex < audioTrackCount; 
			++trackIndex)
		{
			BE::IClipTrackRef clipTrack = audioTrackGroup->GetClipTrack(trackIndex);

			PL::TransitionItemSet transitionItems;
			PL::TrackTransitionMap::iterator iter = audioTransitions.find(trackIndex);
			if (iter != audioTransitions.end())
			{
				transitionItems = (*iter).second;
			}

			AddAudioTrack(audioNode, clipTrack, inSequence, inSRClipItems, transitionItems);
		}
	}

	/*
	**
	*/
	void AddVideo(
		DOMElement* inMediaNode, 
		const BE::TrackItemVector& inTrackItems, 
		const BE::ISequenceRef& inSequence, 
		const PL::SRClipItems& inSRClipItems)
	{
		if (!inMediaNode || !inSequence)
		{
			return;
		}

		// only one video track
		DOMElement* videoNode = XMLUtils::AddChild(inMediaNode, kVideoTag.c_str());
		DOMElement* trackNode = XMLUtils::AddChild(videoNode, kTrackTag.c_str());
		XMLUtils::AddChild(trackNode, kEnabledTag.c_str(), kTRUEStr.c_str());
		XMLUtils::AddChild(trackNode, kLockedTag.c_str(), kFALSEStr.c_str());

		std::int64_t startPoint = 0;
		BE::TrackItemVector::const_iterator itr = inTrackItems.begin();
		BE::TrackItemVector::const_iterator end = inTrackItems.end();
		for (; itr != end; ++itr)
		{
			BE::ITrackItemRef aTrackItem(*itr);
			if (aTrackItem != NULL)
			{
				if (aTrackItem->GetType() == BE::kTrackItemType_Clip)
				{
					PL::SRClipItemPtr srClipItem;

					SRClipItems::const_iterator iter = inSRClipItems.begin();
					SRClipItems::const_iterator end = inSRClipItems.end();
					for (; iter != end; ++iter)
					{
						if ((*iter)->GetTrackItem() == aTrackItem)
						{
							srClipItem = *iter;
						}
					}

					AddClipItem(trackNode, aTrackItem, inSequence, srClipItem);
				}
			}
		}

		PL::TrackTransitionMap videoTransitions = 
			PL::Utilities::BuildTransitionItems(inSequence, BE::kMediaType_Video);
		if (!videoTransitions.empty())
		{
			for (PL::TransitionItemSet::iterator transitionIter = videoTransitions[0].begin();
				transitionIter != videoTransitions[0].end();
				++transitionIter)
			{
				AddTransitionItem(trackNode, *transitionIter);
			}
		}
	}

	/*
	**
	*/
	void AddFileElements(
		DOMElement* inSequenceNode,
		const BE::TrackItemVector& inTrackItems)
	{
		if (!inSequenceNode)
		{
			return;
		}

		std::set<ASL::String> existedFileSet;
		BE::TrackItemVector::const_iterator itr = inTrackItems.begin();
		BE::TrackItemVector::const_iterator end = inTrackItems.end();
		for (; itr != end; ++itr)
		{
			BE::ITrackItemRef aTrackItem(*itr);
			if (aTrackItem != NULL)
			{
				if (aTrackItem->GetType() == BE::kTrackItemType_Clip)
				{
					BE::IMasterClipRef masterClipRef = BE::IMasterClipRef();
					BE::IMediaRef media = BE::IMediaRef();
					BE::IClipRef clip = BE::IClipRef();
					MZ::BEUtilities::GetMediaFromTrackItem(aTrackItem, masterClipRef, media, clip);

					if (!media)
					{
						DVA_ASSERT_MSG(0, "cannot get BE data!!");
						continue;
					}

					// file element
					ASL::String mediaPath = media->GetInstanceString();
					if (existedFileSet.find(mediaPath) == existedFileSet.end())
					{
						existedFileSet.insert(mediaPath);
						DOMElement* fileNode = XMLUtils::AddChild(inSequenceNode, kFileTag.c_str());
						// use file path string as file ID
						fileNode->setAttribute(kIDAttr.c_str(), mediaPath.c_str());
						XMLUtils::AddChild(fileNode, kPathURLTag.c_str(), URLPath(mediaPath).c_str());
					}
				}
			}
		}
	}

	/*
	**
	*/
	void BuildRoughCutDOMDocFromSequence(
		const BE::ISequenceRef& inSequence, 
		const PL::SRClipItems& inSRClipItems, 
		const ASL::String& inName,
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outDoc)
	{
		if (!outDoc || !inSequence)
		{
			return;
		}

		// XML declaration
		outDoc->setXmlVersion(kXMLVersionStr.c_str());
		outDoc->appendChild(outDoc->createDocumentType(kXMEMLDocTypeStr.c_str()));

		// project content
		// xmeml element
		DOMElement* xmemlNode = outDoc->createElement(kXMEMLTag.c_str());
		// add XMEML node as root node
		outDoc->appendChild(xmemlNode);
		xmemlNode->setAttribute(kVersionAttr.c_str(), kXMEMLVersionStr.c_str());

		// sequence element
		DOMElement* sequenceNode = XMLUtils::AddChild(xmemlNode, kSequenceTag.c_str());
		sequenceNode->setAttribute(kIDAttr.c_str(), kRoughCutIDStr.c_str());
		XMLUtils::AddChild(sequenceNode, kNameTag.c_str(), inName.c_str());
		DOMElement* sequenceFrameRateNode = XMLUtils::AddChild(sequenceNode, kDurationTag.c_str(), DVA_STR("0").c_str());

		// sequence rate element
		ASL::String rateTimeBase, rateNTSC;
		dvamediatypes::FrameRate sequenceFrameRate = inSequence->GetVideoFrameRate();
		FCPRateToTimebaseAndNtsc(rateTimeBase, rateNTSC, sequenceFrameRate);
		DOMElement* rateNode = XMLUtils::AddChild(sequenceNode, kRateTag.c_str());
		XMLUtils::AddChild(rateNode, kTimebaseTag.c_str(), rateTimeBase.c_str());
		XMLUtils::AddChild(rateNode, kNTSCTag.c_str(), rateNTSC.c_str());

		BE::TrackItemVector trackItems = MZ::BEUtilities::GetTrackItemsFromTheFirstAvailableTrack(inSequence);

		AddFileElements(sequenceNode, trackItems);

		DOMElement* mediaNode = XMLUtils::AddChild(sequenceNode, kMediaTag.c_str());

		AddVideo(mediaNode, trackItems, inSequence, inSRClipItems);
		
		AddAudio(mediaNode, inSequence, inSRClipItems);
	}

	/*
	**
	*/
	void BuildRoughCutDOMDocFromAssetItems(
		AssetMediaInfoPtr const& inAssetMediaInfo, 
		AssetItemPtr const& inAssetItem,
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outDoc)
	{
		if (!inAssetMediaInfo || !inAssetItem || !outDoc)
		{
			return;
		}

		// XML declaration
		outDoc->setXmlVersion(kXMLVersionStr.c_str());
		outDoc->appendChild(outDoc->createDocumentType(kXMEMLDocTypeStr.c_str()));

		// project content
		// xmeml element
		DOMElement* xmemlNode = outDoc->createElement(kXMEMLTag.c_str());
		// add XMEML node as root node
		outDoc->appendChild(xmemlNode);
		xmemlNode->setAttribute(kVersionAttr.c_str(), kXMEMLVersionStr.c_str());

		// sequence element
		DOMElement* sequenceNode = XMLUtils::AddChild(xmemlNode, kSequenceTag.c_str());
		sequenceNode->setAttribute(kIDAttr.c_str(), kRoughCutIDStr.c_str());
		XMLUtils::AddChild(sequenceNode, kNameTag.c_str(), ASL::PathUtils::GetFilePart(inAssetMediaInfo->GetMediaPath()).c_str());
		DOMElement* sequenceDurationNode = XMLUtils::AddChild(sequenceNode, kDurationTag.c_str(), DVA_STR("0").c_str());

		// sequence rate element
		ASL::String rateTimeBase = inAssetMediaInfo->GetRateTimeBase();
		ASL::String rateNTSC = inAssetMediaInfo->GetRateNtsc();
		DOMElement* rateNode = XMLUtils::AddChild(sequenceNode, kRateTag.c_str());
		XMLUtils::AddChild(rateNode, kTimebaseTag.c_str(), rateTimeBase.c_str());
		XMLUtils::AddChild(rateNode, kNTSCTag.c_str(), rateNTSC.c_str());

		// media/video/track element
		DOMElement* mediaNode = XMLUtils::AddChild(sequenceNode, kMediaTag.c_str());
		DOMElement* videoNode = XMLUtils::AddChild(mediaNode, kVideoTag.c_str());
		DOMElement* trackNode = XMLUtils::AddChild(videoNode, kTrackTag.c_str());
		XMLUtils::AddChild(trackNode, kEnabledTag.c_str(), kTRUEStr.c_str());
		XMLUtils::AddChild(trackNode, kLockedTag.c_str(), kFALSEStr.c_str());

		std::int64_t startPoint = 0;
		std::set<ASL::String> existedFileSet;
		AssetItemList subItemList = inAssetItem->GetSubAssetItemList();
		for (AssetItemList::iterator it=subItemList.begin(); it!=subItemList.end(); it++)
		{
			ASL::String clipItemGUID = (*it)->GetAssetClipItemGuid().AsString();
			ASL::String clipItemPath = (*it)->GetMediaPath();
			ASL::String clipItemName = (*it)->GetAssetName();
			dvamediatypes::TickTime clipDuration = (*it)->GetDuration();
			dvamediatypes::TickTime clipStartTime = (*it)->GetInPoint();

			// file element
			if (existedFileSet.find(clipItemPath) == existedFileSet.end())
			{
				existedFileSet.insert(clipItemPath);
				DOMElement* fileNode = XMLUtils::AddChild(sequenceNode, kFileTag.c_str());
				// use file path string as file ID
				fileNode->setAttribute(kIDAttr.c_str(), clipItemPath.c_str());
				XMLUtils::AddChild(fileNode, kPathURLTag.c_str(), URLPath(clipItemPath).c_str());
			}

			dvamediatypes::FrameRate sequenceFrameRate = TimebaseAndNtscToFCPRate(rateTimeBase, rateNTSC);
			// clip item element
			DOMElement* clipItemNode = XMLUtils::AddChild(trackNode, kClipItemTag.c_str());
			clipItemNode->setAttribute(kIDAttr.c_str(), clipItemGUID.c_str());
			XMLUtils::AddChild(clipItemNode, kNameTag.c_str(), clipItemName);
			XMLUtils::AddChild(clipItemNode, kStartPointTag.c_str(), ToString(startPoint).c_str());
			startPoint += clipDuration.ToFrame64(sequenceFrameRate);
			XMLUtils::AddChild(clipItemNode, kEndPointTag.c_str(), ToString(startPoint).c_str());
			std::int64_t inPoint = clipStartTime.ToFrame64(sequenceFrameRate);
			XMLUtils::AddChild(clipItemNode, kInPointTag.c_str(), ToString(inPoint).c_str());
			std::int64_t outPoint = (clipStartTime + clipDuration).ToFrame64(sequenceFrameRate);
			XMLUtils::AddChild(clipItemNode, kOutPointTag.c_str(), ToString(outPoint).c_str());
			XMLUtils::AddChild(clipItemNode, kFileTag.c_str())->setAttribute(kIDAttr.c_str(), clipItemPath.c_str());
		}
	}

	/*
	**
	*/
	void BuildRoughCutDOMDocForEmptyFile(
		const ASL::String& inName,
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outDoc)
	{
		if(!outDoc)
		{
			return;
		}

		// XML declaration
		outDoc->setXmlVersion(kXMLVersionStr.c_str());
		outDoc->appendChild(outDoc->createDocumentType(kXMEMLDocTypeStr.c_str()));

		// project content
		// xmeml element
		DOMElement* xmemlNode = outDoc->createElement(kXMEMLTag.c_str());
		// add XMEML node as root node
		outDoc->appendChild(xmemlNode);
		xmemlNode->setAttribute(kVersionAttr.c_str(), kXMEMLVersionStr.c_str());

		// sequence element
		DOMElement* sequenceNode = XMLUtils::AddChild(xmemlNode, kSequenceTag.c_str());
		sequenceNode->setAttribute(kIDAttr.c_str(), kRoughCutIDStr.c_str());
		XMLUtils::AddChild(sequenceNode, kNameTag.c_str(), inName.c_str());
		DOMElement* sequenceFrameRateNode = XMLUtils::AddChild(sequenceNode, kDurationTag.c_str(), DVA_STR("0").c_str());

		// sequence rate element
		DOMElement* rateNode = XMLUtils::AddChild(sequenceNode, kRateTag.c_str());
		ASL::String rateTimeBase;
		ASL::String rateNtsc;
		FCPRateToTimebaseAndNtsc(rateTimeBase, rateNtsc);
		XMLUtils::AddChild(rateNode, kTimebaseTag.c_str(), rateTimeBase.c_str());
		XMLUtils::AddChild(rateNode, kNTSCTag.c_str(), rateNtsc.c_str());

		// media/video/track element
		DOMElement* mediaNode = XMLUtils::AddChild(sequenceNode, kMediaTag.c_str());
		DOMElement* videoNode = XMLUtils::AddChild(mediaNode, kVideoTag.c_str());
		DOMElement* trackNode = XMLUtils::AddChild(videoNode, kTrackTag.c_str());
		XMLUtils::AddChild(trackNode, kEnabledTag.c_str(), kTRUEStr.c_str());
		XMLUtils::AddChild(trackNode, kLockedTag.c_str(), kFALSEStr.c_str());
	}

	/*
	**
	*/
	ASL::Result GetTransitionItem(
		DOMNode* inTransitionItemNode,
		const dvamediatypes::FrameRate& inFrameRate,
		BE::MediaType inMediaType,
		BE::TrackIndex inTrackIndex,
		PL::TransitionItemPtr& outTransitionItem)
	{
		if (!inTransitionItemNode)
		{
			return ASL::eUnknown;
		}

		ASL::String startStr;
		ASL::String endStr;
		ASL::String alignmentStr;
		ASL::String cutPointStr;
		ASL::String effectID;

		XMLUtils::GetChildData(inTransitionItemNode, kStartPointTag.c_str(), startStr);
		XMLUtils::GetChildData(inTransitionItemNode, kEndPointTag.c_str(), endStr);
		dvamediatypes::TickTime startPoint = StringToTickTime(startStr, inFrameRate);
		dvamediatypes::TickTime endPoint = StringToTickTime(endStr, inFrameRate);

		XMLUtils::GetChildData(inTransitionItemNode, kAlignmentTag.c_str(), alignmentStr);

		XMLUtils::GetChildData(inTransitionItemNode, kCutPointTag.c_str(), cutPointStr);
		dvamediatypes::TickTime cutPoint = StringToTickTime(cutPointStr, inFrameRate);

		// effect node
		DOMNode* effectNode = XMLUtils::FindChild(inTransitionItemNode, kEffectTag.c_str());
		if (!effectNode)
		{
			DVA_ASSERT_MSG(0, "Cannot get effect node");
			return ASL::eUnknown;
		}
		XMLUtils::GetChildData(effectNode, kEffectIDTag.c_str(), effectID);
		ASL::String transitionMatchName;
		if (inMediaType == BE::kMediaType_Video)
		{
			if (dvacore::utility::StringEquals(effectID, kCrossDissolveEffectID))
			{
				transitionMatchName = kCrossDissolveMatchName;
			}
			else if (dvacore::utility::StringEquals(effectID, kDipColorEffectID))
			{
				DOMNode* paramNode = XMLUtils::FindChild(effectNode, kParameterTag.c_str());
				if (!paramNode)
				{
					DVA_ASSERT_MSG(0, "Cannot get param node");
					return ASL::eUnknown;
				}

				DOMNode* valueNode = XMLUtils::FindChild(paramNode, kValueTag.c_str());
				int redVal = 0;
				XMLUtils::GetChildData(valueNode, kRedTag.c_str(), &redVal);
				if (redVal == 255)
				{
					transitionMatchName = kDipToWhiteMatchName;
				}
				else
				{
					transitionMatchName = kDipToBlackMatchName;
				}
			}
			else
			{
				DVA_ASSERT(0);
			}
		}
		else
		{
			if (dvacore::utility::StringEquals(effectID, kConstantPowerEffectID))
			{
				transitionMatchName = kConstantPowerMatchName;
			}
			else if (dvacore::utility::StringEquals(effectID, kConstantGainEffectID))
			{
				transitionMatchName = kConstantGainMatchName;
			}
			else
			{
				DVA_ASSERT(0);
			}
		}

		float startRatio;
		float endRatio;
		bool isReverse;
		XMLUtils::GetChildData(effectNode, kStartRatioTag.c_str(), &startRatio);
		XMLUtils::GetChildData(effectNode, kEndRatioTag.c_str(), &endRatio);
		XMLUtils::GetChildData(effectNode, kReverseTag.c_str(), &isReverse);

		outTransitionItem = PL::TransitionItemPtr(new PL::TransitionItem(
			startPoint,
			endPoint,
			inFrameRate,
			alignmentStr,
			cutPoint,
			inTrackIndex,
			transitionMatchName,
			inMediaType,
			startRatio,
			endRatio,
			isReverse));

		return ASL::kSuccess;
	}

	/*
	**
	*/
	ASL::Result GetClipItem(
		DOMNode* inClipItemNode,
		const std::map<ASL::String, ASL::String>& inFileIdToPathMap,
		const dvamediatypes::FrameRate& inFrameRate,
		const ASL::Guid& inRoughCutID,
		PL::AssetItemPtr& outSubAssetItem)
	{
		if (!inClipItemNode)
		{
			return ASL::eUnknown;
		}

		ASL::String clipItemIDStr;
		ASL::String nameStr;
		ASL::String inPointStr;
		ASL::String outPointStr;
		ASL::String fileIDStr;

		XMLUtils::GetAttr(inClipItemNode, kIDAttr.c_str(), clipItemIDStr);
		DOMNode* fileNode = XMLUtils::FindChild(inClipItemNode, kFileTag.c_str());
		if(fileNode != NULL)
		{		
			XMLUtils::GetAttr(fileNode, kIDAttr.c_str(), fileIDStr);
		}
		XMLUtils::GetChildData(inClipItemNode, kNameTag.c_str(), nameStr);
		XMLUtils::GetChildData(inClipItemNode, kInPointTag.c_str(), inPointStr);
		XMLUtils::GetChildData(inClipItemNode, kOutPointTag.c_str(), outPointStr);

		// Check the data of clipItem
		std::map<ASL::String, ASL::String>::const_iterator fileIDIter = 
			inFileIdToPathMap.find(fileIDStr);
		if (fileIDStr.empty() || 
			inPointStr.empty() || 
			outPointStr.empty() || 
			fileIDIter == inFileIdToPathMap.end())
		{
			DVA_ASSERT(0);
			return ASL::eUnknown;
		}

		dvamediatypes::TickTime clipStart = StringToTickTime(inPointStr, inFrameRate);
		dvamediatypes::TickTime clipDuration = 
			StringToTickTime(outPointStr, inFrameRate) - clipStart;

		outSubAssetItem = PL::AssetItemPtr(new PL::AssetItem(
			PL::kAssetLibraryType_RCSubClip,
			(*fileIDIter).second,
			ASL::Guid(clipItemIDStr),
			inRoughCutID,
			ASL::Guid::CreateUnique(),
			nameStr, 
			DVA_STR(""),
			clipStart,
			clipDuration,
			dvamediatypes::kTime_Invalid,
			dvamediatypes::kTime_Invalid,
			ASL::Guid(clipItemIDStr)));

		return ASL::kSuccess;
	}

	/*
	**
	*/
	ASL::Result GetVideo(
		DOMNode* inMediaNode,
		const std::map<ASL::String, ASL::String>& inFileIdToPathMap,
		const dvamediatypes::FrameRate& inFrameRate,
		const ASL::Guid& inRoughCutID,
		PL::TrackTransitionMap& outVideoTrackTransitions,
		PL::AssetItemList& outSubAssetItems)
	{
		if (!inMediaNode)
		{
			return ASL::eUnknown;
		}

		DOMNode* videoNode = XMLUtils::FindChild(inMediaNode, kVideoTag.c_str());
		if(videoNode != NULL)
		{
			// only one video track, get directly
			DOMNode* trackNode = XMLUtils::FindChild(videoNode, kTrackTag.c_str());
			if(trackNode != NULL)
			{
				// Get clip item info and fill RawRoughCutData
				DOMNode* clipItemNode = XMLUtils::FindChild(trackNode, kClipItemTag.c_str());
				while (clipItemNode != NULL)
				{
					PL::AssetItemPtr subAssetItem;
					GetClipItem(
						clipItemNode,
						inFileIdToPathMap,
						inFrameRate,
						inRoughCutID,
						subAssetItem);
					outSubAssetItems.push_back(subAssetItem);

					clipItemNode = XMLUtils::FindNextSibling(clipItemNode, kClipItemTag.c_str());
				}

				// get transitions
				DOMNode* transitionItemNode = XMLUtils::FindChild(trackNode, kTransitionItemTag.c_str());
				while (transitionItemNode != NULL)
				{
					PL::TransitionItemPtr videoTransition;
					if (ASL::ResultSucceeded(GetTransitionItem(
						transitionItemNode, 
						inFrameRate,
						BE::kMediaType_Video,
						0, 
						videoTransition)))
					{
						outVideoTrackTransitions[0].insert(videoTransition);
					}

					transitionItemNode = XMLUtils::FindNextSibling(transitionItemNode, kTransitionItemTag.c_str());
				}
			}
		}

		return ASL::kSuccess;
	}

	/*
	**
	*/
	ASL::Result GetAudio(
		DOMNode* inMediaNode,
		PL::TrackTransitionMap& outAudioTrackTransitions,
		const dvamediatypes::FrameRate& inFrameRate)
	{
		if (!inMediaNode)
		{
			return ASL::eUnknown;
		}

		DOMNode* audioNode = XMLUtils::FindChild(inMediaNode, kAudioTag.c_str());
		if(audioNode != NULL)
		{
			BE::TrackIndex trackIndex =  0;
			DOMNode* trackNode = XMLUtils::FindChild(audioNode, kTrackTag.c_str());
			while (trackNode != NULL)
			{
				// get transitions
				DOMNode* transitionItemNode = XMLUtils::FindChild(trackNode, kTransitionItemTag.c_str());
				while (transitionItemNode != NULL)
				{
					PL::TransitionItemPtr audioTransition;
					if (ASL::ResultSucceeded(GetTransitionItem(
						transitionItemNode, 
						inFrameRate,
						BE::kMediaType_Audio,
						trackIndex, 
						audioTransition)))
					outAudioTrackTransitions[trackIndex].insert(audioTransition);

					transitionItemNode = XMLUtils::FindNextSibling(transitionItemNode, kTransitionItemTag.c_str());
				}

				trackNode = XMLUtils::FindNextSibling(trackNode, kTrackTag.c_str());
				++trackIndex;
			}
		}

		return ASL::kSuccess;
	}

	/*
	**
	*/
	ASL::Result LoadAssetItemsFromXmemlNode(
		DOMElement* inXmemlNode,
		const ASL::Guid& inRoughCutID,
		const ASL::String& inRCFilePath,
		PL::AssetMediaInfoPtr& outAssetMediaInfo,
		PL::AssetItemPtr& outAssetItem)
	{
		DVA_ASSERT(inXmemlNode != NULL);
		if (inXmemlNode == NULL)
		{
			return ASL::eUnknown;
		}

		DOMNode* sequenceNode = XMLUtils::FindChild(inXmemlNode, kSequenceTag.c_str());
		if (sequenceNode != NULL)
		{
			// Get sequence Rate
			dvamediatypes::FrameRate sequenceRate = dvamediatypes::kFrameRate_Zero;
			DOMNode* sequenceRateNode = XMLUtils::FindChild(sequenceNode, kRateTag.c_str());
			ASL::String frameRateStr;
			ASL::String NtscStr;
			if(sequenceRateNode != NULL)
			{
				XMLUtils::GetChildData(sequenceRateNode, kTimebaseTag.c_str(), frameRateStr);
				XMLUtils::GetChildData(sequenceRateNode, kNTSCTag.c_str(), NtscStr);
				sequenceRate = TimebaseAndNtscToFCPRate(frameRateStr, NtscStr);
			}

			// Build file ID and path map
			DOMNode* fileNode = XMLUtils::FindChild(sequenceNode, kFileTag.c_str());
			std::map<ASL::String, ASL::String> fileIdToPathMap;
			ASL::String fileIDStr;
			ASL::String filePath;
			while (fileNode != NULL)
			{
				XMLUtils::GetAttr(fileNode, kIDAttr.c_str(), fileIDStr);
				XMLUtils::GetChildData(fileNode, kPathURLTag.c_str(), filePath);

				// Check the validation of fileID and filePath
				ASL_ASSERT(!fileIDStr.empty() && !filePath.empty());
				if (fileIDStr.empty() || filePath.empty())
				{
					return ASL::eUnknown;
				}

				fileIdToPathMap[fileIDStr] = LocalPath(filePath);
				fileNode = XMLUtils::FindNextSibling(fileNode, kFileTag.c_str());
			}

			PL::AssetItemList subAssetItemList;
			PL::TrackTransitionMap videoTransitions;
			PL::TrackTransitionMap audioTransitions;

			// Get clipItems
			DOMNode* mediaNode = XMLUtils::FindChild(sequenceNode, kMediaTag.c_str());
			if(mediaNode != NULL)
			{
				GetVideo(
					mediaNode,
					fileIdToPathMap,
					sequenceRate,
					inRoughCutID,
					videoTransitions,
					subAssetItemList);

				GetAudio(
					mediaNode,
					audioTransitions,
					sequenceRate);
			}

			outAssetItem = PL::AssetItemPtr(new PL::AssetItem(
				PL::kAssetLibraryType_RoughCut,
				inRCFilePath,
				inRoughCutID,
				inRoughCutID,
				inRoughCutID,
				ASL::PathUtils::GetFilePart(inRCFilePath)));
			outAssetItem->SetSubAssetItemList(subAssetItemList);
			outAssetItem->SetTrackTransitions(videoTransitions, BE::kMediaType_Video);
			outAssetItem->SetTrackTransitions(audioTransitions, BE::kMediaType_Audio);

			outAssetMediaInfo = PL::AssetMediaInfo::CreateRoughCutMediaInfo(
				inRoughCutID,
				inRCFilePath,
				PL::LoadRoughCut(inRCFilePath),
				PL::SRLibrarySupport::GetFileCreateTime(inRCFilePath),
				PL::SRLibrarySupport::GetFileModifyTime(inRCFilePath),
				frameRateStr,
				NtscStr);

			return ASL::kSuccess;
		}

		return ASL::eUnknown;
	}
}

ASL::Result CreateEmptyFile(const ASL::String& inPath)
{
	ASL::File tempXMLFile;
	ASL::Result result = tempXMLFile.Create(
		inPath,
		ASL::FileAccessFlags::kWrite,
		ASL::FileShareModeFlags::kNone,
		ASL::FileCreateDispositionFlags::kCreateAlways,
		ASL::FileAttributesFlags::kAttributeNormal);
	tempXMLFile.Close();
	return result;
}

void FCPRateToTimebaseAndNtsc(
	ASL::String& outTimebase,
	ASL::String& outIsNtsc,
	dvamediatypes::FrameRate inFCPRate)
{
	outIsNtsc = kTRUEStr;
	dvamediatypes::FrameRate timebase;
	if (inFCPRate == ASL::kFrameRate_23976)
		timebase = ASL::kFrameRate_Film24;
	else if (inFCPRate == ASL::kFrameRate_VideoNTSC)
		timebase = ASL::kFrameRate_Video30;
	else if (inFCPRate == ASL::kFrameRate_VideoNTSC_HD)
		timebase = ASL::kFrameRate_Video60;
	else
	{
		outIsNtsc = kFALSEStr;
		timebase = inFCPRate;
	}
	outTimebase = ToString(timebase);
}

dvamediatypes::FrameRate TimebaseAndNtscToFCPRate(
	const ASL::String& inTimebase,
	const ASL::String& inIsNtsc)
{
	dvamediatypes::FrameRate frameRate(ASL::Coercion<double>::Result(inTimebase));

	if (inIsNtsc != kTRUEStr)
		return frameRate;

	if (frameRate == ASL::kFrameRate_Film24)
		return ASL::kFrameRate_23976;
	else if (frameRate == ASL::kFrameRate_Video30)
		return ASL::kFrameRate_VideoNTSC;
	else if (frameRate == ASL::kFrameRate_Video60)
		return ASL::kFrameRate_VideoNTSC_HD;
	else
		return frameRate;
}

ASL::Result CreateEmptyRoughCutFile(const ASL::String& inFilePath)
{
	ASL::String roughcutContent = CreateEmptyRoughCutContent(ASL::PathUtils::GetFilePart(inFilePath));
	return SaveRoughCut(inFilePath, roughcutContent);
}

bool IsRoughCutFileExtension(
	const dvacore::UTF16String& inFullFileName)
{
	return (ASL::PathUtils::IsValidPath(inFullFileName) && ASL::PathUtils::FileHasExtension(inFullFileName, ASL_STR(".arcut")));
}

bool IsValidRoughCutFile(
	const dvacore::UTF16String& inFullFileName)
{
	if (IsRoughCutFileExtension(inFullFileName))
	{
		try
		{
			XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
			XMLUtils::CreateDOM(&parser, inFullFileName.c_str(), &doc);
			if(doc != NULL)
			{
				XERCES_CPP_NAMESPACE_QUALIFIER DOMDocumentType* docType = doc->getDoctype();
				DOMElement* xmemlNode = doc->getDocumentElement();
				if (docType != NULL && xmemlNode != NULL && ASL::MakeString(docType->getName()) == kXMEMLDocTypeStr)
				{
					ASL::String xmemlVersionStr;
					XMLUtils::GetAttr(xmemlNode, kVersionAttr.c_str(), xmemlVersionStr);
					if (xmemlVersionStr == kXMEMLVersionStr)
					{
						DOMNode* sequenceNode = XMLUtils::FindChild(xmemlNode, kSequenceTag.c_str());
						if (sequenceNode != NULL)
						{		
							ASL::String roughCutIDStr;
							XMLUtils::GetAttr(sequenceNode, kIDAttr.c_str(), roughCutIDStr);
							if (roughCutIDStr == kRoughCutIDStr)
							{
								return true;
							}
						}
					}
				}
			}
		}
		catch (...)
		{
			return false;
		}
	}
	return false;
}

ASL::Result SaveRoughCut(
	const ASL::String& inFilePath,
	const ASL::String& inContent)
{
	if(!ASL::PathUtils::IsValidPath(inFilePath))
	{
		return ASL::ePathNotFound;
	}

	bool isTryAgain = false;
	ASL::Result result = ASL::eUnknown;

	do
	{
		ASL::String tempFilePath = ASL::File::MakeUniqueTempPath(inFilePath);
		ASL::UInt64 neededSize = inContent.size();
		isTryAgain = false;
		result = ASL::eUnknown;

		{
			ASL::File rcFile;
			if(ASL::ResultSucceeded(result = rcFile.Create(
				tempFilePath,
				ASL::FileAccessFlags::kWrite,
				ASL::FileShareModeFlags::kNone,
				ASL::FileCreateDispositionFlags::kCreateAlways,
				ASL::FileAttributesFlags::kAttributeNormal)))
			{
				ASL::UInt32 numberOfBytesWritten;
				dvacore::UTF8String contentUTF8 =  dvacore::utility::UTF16to8(inContent);
				result = rcFile.Write(contentUTF8.c_str(), static_cast<ASL::UInt32>(contentUTF8.size()), numberOfBytesWritten);
			}
			rcFile.Close();
		}

		if (ASL::ResultSucceeded(result))
		{
			if (!ASL::PathUtils::ExistsOnDisk(inFilePath))
			{
				result = CreateEmptyFile(inFilePath);
			}
			if (ASL::ResultSucceeded(result))
			{
				if (!ASL::PathUtils::IsReadOnly(inFilePath))
				{
					result = ASL::File::SwapTempFile(tempFilePath, inFilePath);
				}
				else
				{
					result = ASL::eAccessIsDenied;
				}
			}
			else
			{
				neededSize *= 2;
			}
		}

		if (!ASL::ResultSucceeded(result))
		{
			ASL::String title = dvacore::ZString("$$$/Prelude/Mezzanine/RoughCut/SaveRoughCutErrorTitle=Save Rough Cut");
			ASL::String suggestStr;
			switch(result)
			{
			case ASL::eDiskFull:
				{
					suggestStr = dvacore::ZString(
						"$$$/Prelude/Mezzanine/RoughCut/DiskIsFull=There is not enough space on @0. You need about @1  to save this rough cut.",
						ASL::PathUtils::GetFullDirectoryPart(inFilePath),
						ASL::DiskUtils::MakeDiskSpaceString(neededSize));
					break;
				}
			case ASL::eAccessIsDenied:
				{
					suggestStr = dvacore::ZString(
						"$$$/Prelude/Mezzanine/RoughCut/AccessIsDenied=The file @0 cannot be saved - it is write-protected.",
						ASL::PathUtils::GetFullFilePart(inFilePath));
					break;
				}
			case ASL::eFileInUse:
				{
					suggestStr = dvacore::ZString(
						"$$$/Prelude/Mezzanine/RoughCut/FileInUse=The file @0 cannot be saved, because it is being used by some process.",
						ASL::PathUtils::GetFullFilePart(inFilePath));
					break;
				}
			default:
				{
					suggestStr = dvacore::ZString(
						"$$$/Prelude/Mezzanine/RoughCut/OtherSaveFailed=The file @0 cannot be saved. Please verify there is enough space to save the file and that you have proper write access permissions for the file.",
						ASL::PathUtils::GetFullFilePart(inFilePath));
					break;
				}

			}

			dvaui::dialogs::UI_MessageBox::Button response  = 
				dvaui::dialogs::messagebox::Messagebox(
				UIF::Workspace::Instance()->GetPtr()->GetDrawSupplier(),
				title,
				suggestStr,
				dvaui::dialogs::UI_MessageBox::kButton_Retry | dvaui::dialogs::UI_MessageBox::kButton_Cancel,
				dvaui::dialogs::UI_MessageBox::kButton_Cancel,
				dvaui::dialogs::UI_MessageBox::kIcon_Caution,
				UIF::Workspace::Instance()->GetPtr()->GetMainTopLevelWindow());
			if (response == dvaui::dialogs::UI_MessageBox::kButton_Retry)
			{
				isTryAgain = true;
			}
		}		

		if (!ASL::ResultSucceeded(result) && ASL::PathUtils::ExistsOnDisk(tempFilePath))
		{
			ASL::File::Delete(tempFilePath);
		}

	} while (isTryAgain);

	return result;
}

ASL::String LoadRoughCut(const ASL::String& inFilePath)
{
	ASL::File rcFile;
	if(ASL::ResultSucceeded(rcFile.Create(
		inFilePath,
		ASL::FileAccessFlags::kRead,
		ASL::FileShareModeFlags::kNone,
		ASL::FileCreateDispositionFlags::kOpenExisting,
		ASL::FileAttributesFlags::kAttributeNormal)))
	{
		ASL::UInt64 byteSize = rcFile.SizeOnDisk();
		if (byteSize < ASL::UInt32(-1))
		{
			ASL::UInt32 readSize = static_cast<ASL::UInt32>(byteSize);
			std::vector<dvacore::UTF8Char> buffer(readSize + 1);
			ASL::UInt32 numberOfBytesRead = 0;
			if (ASL::ResultSucceeded(rcFile.Read(&buffer[0], readSize, numberOfBytesRead)))
			{
				return ASL::MakeString(&buffer[0]);
			}
		}
		else
		{
			DVA_ASSERT_MSG(0, "Too huge rough cut file which size is more than 2^32 bytes.");
		}
	}
	return ASL::String();
}

ASL::String BuildRoughCutContent(
	const BE::ISequenceRef& inSequence,
	const PL::SRClipItems& inSRClipItems,
	const ASL::String& inName)
{
	//	sequence -> DOM -> XML
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = XMLUtilities::CreateDomDocument();
	BuildRoughCutDOMDocFromSequence(
		inSequence,
		inSRClipItems,
		inName,
		pDoc);

	ASL::String fileContent = XMLUtilities::ConvertDomToXMLString(pDoc);
	pDoc->release();

	return fileContent;
}
ASL::String BuildRoughCutContent(
	AssetMediaInfoPtr const& inAssetMediaInfo, 
	AssetItemPtr const& inAssetItem)
{
	if (!inAssetMediaInfo)
	{
		return ASL::String();
	}

	ASL::String fileContent = inAssetMediaInfo->GetFileContent();
	if (!fileContent.empty())
	{
		return fileContent;
	}

	if (!inAssetItem)
	{
		return ASL::String();
	}

	//	assetItem -> DOM -> XML
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = XMLUtilities::CreateDomDocument();
	BuildRoughCutDOMDocFromAssetItems(
		inAssetMediaInfo,
		inAssetItem,
		pDoc);

	fileContent = XMLUtilities::ConvertDomToXMLString(pDoc);
	pDoc->release();

	return fileContent;
}

ASL::String CreateEmptyRoughCutContent(const ASL::String& inName)
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = XMLUtilities::CreateDomDocument();
	BuildRoughCutDOMDocForEmptyFile(inName, pDoc);

	ASL::String fileContent = XMLUtilities::ConvertDomToXMLString(pDoc);
	pDoc->release();
	return fileContent;
}

ASL::Result LoadRoughCut(
	const ASL::String& inFilePath,
	const ASL::Guid& inRoughCutID,
	PL::AssetMediaInfoPtr& outAssetMediaInfo,
	PL::AssetItemPtr& outAssetItem)
{
	if (!IsValidRoughCutFile(inFilePath))
	{
		return  ASL::ePathNotFound;
	}

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
	XMLUtils::CreateDOM(&parser, inFilePath.c_str(), &doc);

	if (doc != NULL)
	{
		DOMElement* xmemlNode = doc->getDocumentElement();
		if (xmemlNode != NULL)
		{
			return LoadAssetItemsFromXmemlNode(
				xmemlNode,
				inRoughCutID,
				inFilePath,
				outAssetMediaInfo,
				outAssetItem);
		}
	}

	return ASL::eUnknown;
}

} // namespace PL
