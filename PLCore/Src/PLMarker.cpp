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

#include "ASLCoercion.h"
#include "ASLReferenceBridge.h"
#include "ASLDebug.h"
#include "ASLWeakReferenceCapable.h"
#include "ASLPathUtils.h"
#include "PLMarker.h"
#include "IPLMarkerOwner.h"
#include "PLMarkerTypeRegistry.h"
#include "PLUtilities.h"
#include "PLCompositeMarker.h"
#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProperties.h"
#include "dvamediatypes/FrameRate.h"
#include "dvatemporalxmp/XMPDocOpsTemporal.h"
#include <boost/foreach.hpp>
#include "dvacore/proplist/PropListUtils.h"
#include "UIFDVAConversionUtilities.h"

namespace
{
	const ASL::UInt32 kMaxBufferLength = 262144;

	const dvacore::UTF8String kMarker_ID = dvacore::utility::AsciiToUTF8("guid");
	const dvacore::UTF8String kMarker_Name = dvacore::utility::AsciiToUTF8("name");
	const dvacore::UTF8String kMarker_Type = dvacore::utility::AsciiToUTF8("type");
	const dvacore::UTF8String kMarker_Comment = dvacore::utility::AsciiToUTF8("comment");
	const dvacore::UTF8String kMarker_Location = dvacore::utility::AsciiToUTF8("location");
	const dvacore::UTF8String kMarker_Target = dvacore::utility::AsciiToUTF8("target");
	const dvacore::UTF8String kMarker_CuePointType = dvacore::utility::AsciiToUTF8("cuePointType");
	const dvacore::UTF8String kMarker_Speaker = dvacore::utility::AsciiToUTF8("speaker");
	const dvacore::UTF8String kMarker_Probability = dvacore::utility::AsciiToUTF8("probability");
	const dvacore::UTF8String kMarker_FrameRate = dvacore::utility::AsciiToUTF8("frameRate");
	const dvacore::UTF8String kMarker_StartTime = dvacore::utility::AsciiToUTF8("startTime");
	const dvacore::UTF8String kMarker_Duration = dvacore::utility::AsciiToUTF8("duration");
	const dvacore::UTF8String kMarker_CuePointParams = dvacore::utility::AsciiToUTF8("cuePointParams");
	const dvacore::UTF8String kMarker_TagList = dvacore::utility::AsciiToUTF8("tagList");
	const dvacore::UTF8String kMarker_TagGuid = dvacore::utility::AsciiToUTF8("tagGuid");
	const dvacore::UTF8String kMarker_TagName = dvacore::utility::AsciiToUTF8("tagName");
	const dvacore::UTF8String kMarker_TagPayload = dvacore::utility::AsciiToUTF8("tagDescription");
	const dvacore::UTF8String kMarker_TagColor = dvacore::utility::AsciiToUTF8("tagColor");
}

namespace PL
{

extern const BE::PropertyKey kPropertyKey_CloseOnNewMarker(BE_PROPERTYKEY("MZ.CloseOnNewMarker"));
extern const BE::PropertyKey kPropertyKey_StopOnNewMarker(BE_PROPERTYKEY("MZ.StopOnNewMarker"));
extern const BE::PropertyKey kPropertyKey_SwitchFocusOnDoubleClickMarker(BE_PROPERTYKEY("MZ.SwitchFocusOnDoubleClickMarker"));
extern const BE::PropertyKey kPropertyKey_SwitchUseTagColorMarker(BE_PROPERTYKEY("MZ.SwitchUseTagColorMarker"));
extern const BE::PropertyKey kPropertyKey_ShowRelatedPanelsOnOpenAsset(BE_PROPERTYKEY("MZ.ShowRelatedPanelsOnOpenAsset"));
extern const BE::PropertyKey kPropertyKey_SetMarkerDurationByTypeBase = BE_PROPERTYKEY("MZ.SetMarkerDuration.");
extern const BE::PropertyKey kPropertyKey_MarkerDurationByTypeBase = BE_PROPERTYKEY("MZ.MarkerDuration.");
extern const BE::PropertyKey kPropertyKey_MarkerPreroll = BE_PROPERTYKEY("MZ.MarkerPreroll");

////// TagParam
TagParam::TagParam()
	:
	mInstanceGuid(ASL::Guid::CreateUnique()),
	mColor(dvaui::drawbot::ColorRGBA(0.0f))
{
}

TagParam::TagParam(
	const ASL::String& inName, 
	const ASL::String& inPayload, 
	const dvaui::drawbot::ColorRGBA& inColor)
	:
	mInstanceGuid(ASL::Guid::CreateUnique()),
	mName(inName),
	mPayload(inPayload),
	mColor(inColor)
{
}

TagParam::TagParam(
	const dvatemporalxmp::XMPKeywordData& inXMPKeywordData)
	:
	mColor(dvaui::drawbot::ColorRGBA(0.0f))
{
	mInstanceGuid = ASL::Guid(inXMPKeywordData.GetGuid());

	mName = ASL::MakeString(inXMPKeywordData.GetName());

	mPayload = ASL::MakeString(inXMPKeywordData.GetKeywordExtension().GetTagDescription().GetPayload());

	mColor = PL::Utilities::ConvertUINT32ToColorRGBA(
		inXMPKeywordData.GetKeywordExtension().GetTagDescription().GetARGBColor());
}

TagParam::TagParam(const TagParam& inTagParam, bool inMakeUniqueInstance)
	:
	mName(inTagParam.mName),
	mPayload(inTagParam.mPayload),
	mColor(inTagParam.mColor)
{
	mInstanceGuid = inMakeUniqueInstance ? ASL::Guid::CreateUnique() : inTagParam.mInstanceGuid;
}

TagParam::~TagParam()
{

}

bool TagParam::operator ==(const TagParam& inTagParam)
{
	// maybe instance id is enough
	return (mInstanceGuid == inTagParam.mInstanceGuid &&
			mName == inTagParam.mName &&
			mPayload == inTagParam.mPayload &&
			mColor == inTagParam.mColor);
}

ASL::String TagParam::GetName() const
{
	return mName;
}

ASL::String TagParam::GetPayload() const
{
	return mPayload;
}

dvaui::drawbot::ColorRGBA TagParam::GetColor() const
{
	return mColor;
}

ASL::Guid TagParam::GetInstanceGuid() const
{
	return mInstanceGuid;
}

void TagParam::SerializeOut(ASL::OutputStringStream& outStream)
{
	ASL::String guidString = mInstanceGuid.AsString();
	outStream << guidString.size() << ASL::ENDL << guidString;

	outStream << mName.size() << ASL::ENDL << mName;

	outStream << mPayload.size() << ASL::ENDL << mPayload;

	ASL::String colorString = ASL::MakeString(Utilities::ConvertColorRGBAToString(mColor));
	outStream << colorString.size() << ASL::ENDL << colorString;
}

void TagParam::SerializeIn(ASL::InputStringStream& inStream)
{
	boost::shared_ptr<dvacore::UTF16Char> bufPtr(new dvacore::UTF16Char[kMaxBufferLength]);
	dvacore::UTF16Char* buf = bufPtr.get();

	dvacore::UTF16String line;
	size_t stringLength;

	std::getline(inStream, line);
	stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
	inStream.read(buf, stringLength);
	buf[stringLength] = 0;
	mInstanceGuid = ASL::Guid(ASL::String(buf));

	std::getline(inStream, line);
	stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
	inStream.read(buf, stringLength);
	buf[stringLength] = 0;
	mName = ASL::String(buf);

	std::getline(inStream, line);
	stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
	inStream.read(buf, stringLength);
	buf[stringLength] = 0;
	mPayload = ASL::String(buf);

	std::getline(inStream, line);
	stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
	inStream.read(buf, stringLength);
	buf[stringLength] = 0;
	dvacore::UTF16String colorString = ASL::String(buf);
	mColor = Utilities::ConvertStringToColorRGBA(ASL::MakeStdString(colorString));
}

void TagParam::ToProperty(dvacore::proplist::PropList* ioTagListPropList) const
{
	dvacore::proplist::PropList* tagSubList = ioTagListPropList->NewSubList(dvacore::utility::UTF16to8(mInstanceGuid.AsString()));
	tagSubList->SetValue(kMarker_TagName, dvacore::utility::UTF16to8(mName));
	tagSubList->SetValue(kMarker_TagPayload, dvacore::utility::UTF16to8(mPayload));
	ASL::String colorString = ASL::MakeString(Utilities::ConvertColorRGBAToString(mColor));
	tagSubList->SetValue(kMarker_TagColor, dvacore::utility::UTF16to8(colorString));
}

void TagParam::FromProperty(ASL::Guid const& inGuid, dvacore::proplist::PropList* inTagPropList)
{
	mInstanceGuid = inGuid;

	std::list<dvacore::UTF8String> tagParams;
	inTagPropList->GetPropertyNames(tagParams);
	std::list<dvacore::UTF8String>::iterator tagParamsIter = tagParams.begin();
	std::list<dvacore::UTF8String>::iterator tagParamsIterEnd = tagParams.end();

	for (; tagParamsIter != tagParamsIterEnd; ++tagParamsIter)
	{
		dvacore::UTF8String tagParamValueStr;
		inTagPropList->GetValue(*tagParamsIter, tagParamValueStr);
		dvacore::UTF16String tagParamValue = dvacore::utility::UTF8to16(tagParamValueStr);

		// Don't write out the Multiple Values string
		if (tagParamValue == PL::GetMultipleValuesString())
		{
			continue;
		}
		if (*tagParamsIter == kMarker_TagName)
		{
			mName = tagParamValue;
		}
		else if (*tagParamsIter == kMarker_TagPayload)
		{
			mPayload = tagParamValue;
		}
		else if (*tagParamsIter == kMarker_TagColor)
		{
			mColor = Utilities::ConvertStringToColorRGBA(ASL::MakeStdString(tagParamValue));
		}
	}
}

////// cottonwoodMarker
    
CottonwoodMarker::CottonwoodMarker()
    :
	mGUID(ASL::Guid::CreateUnique())
{
    
}
    
CottonwoodMarker::CottonwoodMarker(PL::SRMarkerOwnerWeakRef inMarkerOwner)
	:
	mGUID(ASL::Guid::CreateUnique()),
    mMarkerOwner(inMarkerOwner)
{
}

CottonwoodMarker::CottonwoodMarker(
	const CottonwoodMarker& inMarker,
	bool inMakeUnique)
{
	mGUID = inMakeUnique ? ASL::Guid::CreateUnique() : inMarker.mGUID;
	mStartTime = inMarker.mStartTime;
	mDuration = inMarker.mDuration;
	mType = inMarker.mType;
	mName = inMarker.mName;
	mComment = inMarker.mComment;
	mLocation = inMarker.mLocation;
	mTarget = inMarker.mTarget;
	mCuePointType = inMarker.mCuePointType;
	mCuePointList = inMarker.mCuePointList;
	mSpeaker = inMarker.mSpeaker;
	mProbability = inMarker.mProbability;
	mMarkerOwner = inMarker.mMarkerOwner;

	for (TagParamMap::const_iterator it = inMarker.GetTagParams().begin();
		it != inMarker.GetTagParams().end();
		it++)
	{
		TagParamPtr newInstanceTag(new TagParam(*((*it).second), inMakeUnique));
		AddTagParam(newInstanceTag);
	}
}

CottonwoodMarker::~CottonwoodMarker()
{
}

CottonwoodMarker& CottonwoodMarker::operator =(const CottonwoodMarker& inRHS)
{
	if (this == &inRHS)
	{
		return *this;
	}
	mGUID = inRHS.mGUID;
	mStartTime = inRHS.mStartTime;
	mDuration = inRHS.mDuration;
	mType = inRHS.mType;
	mName = inRHS.mName;
	mComment = inRHS.mComment;
	mLocation = inRHS.mLocation;
	mTarget = inRHS.mTarget;
	mCuePointType = inRHS.mCuePointType;
	mCuePointList = inRHS.mCuePointList;
	mSpeaker = inRHS.mSpeaker;
	mProbability = inRHS.mProbability;

	mTagParams.clear();
	for (TagParamMap::const_iterator it = inRHS.GetTagParams().begin();
		it != inRHS.GetTagParams().end();
		it++)
	{
		TagParamPtr newInstanceTag(new TagParam(*(it->second), false));
		AddTagParam(newInstanceTag);
	}

	mMarkerOwner = inRHS.mMarkerOwner;

	return *this;
}

void CottonwoodMarker::SerializeToString(
	const CottonwoodMarkerList& inMarkerList,
	dvacore::UTF16String& outString)
{
	if (!inMarkerList.empty())
	{
		ASL::OutputStringStream buffer;
		buffer << inMarkerList.size() << ASL::ENDL;
		dvamediatypes::TickTime earliestMarkerStart(dvamediatypes::kTime_Max);

		CottonwoodMarkerList::const_iterator iter = inMarkerList.begin();
		for ( ; iter != inMarkerList.end(); ++iter)
		{
			if (iter->GetStartTime() < earliestMarkerStart)
			{
				earliestMarkerStart = iter->GetStartTime();
			}
		}
		buffer << earliestMarkerStart.GetTicks() << ASL::ENDL;
				
		iter = inMarkerList.begin();
		for ( ; iter != inMarkerList.end(); ++iter)
		{
			ASL::String guid = iter->mGUID.AsString();
			buffer << guid.size() << ASL::ENDL << guid;
			buffer << iter->mStartTime.GetTicks() << ASL::ENDL;
			buffer << iter->mDuration.GetTicks() << ASL::ENDL;
			buffer << iter->mComment.size() << ASL::ENDL << iter->mComment;
			buffer << iter->mLocation.size() << ASL::ENDL << iter->mLocation;
			buffer << iter->mName.size() << ASL::ENDL << iter->mName;
			buffer << iter->mProbability.size() << ASL::ENDL << iter->mProbability;
			buffer << iter->mSpeaker.size() << ASL::ENDL << iter->mSpeaker;
			buffer << iter->mTarget.size() << ASL::ENDL << iter->mTarget;
			buffer << iter->mType.size() << ASL::ENDL << iter->mType;
			buffer << iter->mCuePointType.size() << ASL::ENDL << iter->mCuePointType;
			buffer << iter->mCuePointList.size() << ASL::ENDL;
			for (size_t i = 0; i < iter->mCuePointList.size(); ++i)
			{
				buffer << iter->mCuePointList[i].mKey.size() << ASL::ENDL << iter->mCuePointList[i].mKey;
				buffer << iter->mCuePointList[i].mValue.size() << ASL::ENDL << iter->mCuePointList[i].mValue;
			}

			// serialize out tag params
			buffer << iter->mTagParams.size() << ASL::ENDL;
			for (TagParamMap::const_iterator it = iter->mTagParams.begin();
				it != iter->mTagParams.end();
				it++)
			{
				(*it).second->SerializeOut(buffer);
			}
		}
		
		outString = buffer.str();
	}
}

void CottonwoodMarker::SerializeFromString(
	const dvacore::UTF16String& inString,
	const dvamediatypes::TickTime& inCurrentCTI,
	CottonwoodMarkerList& outMarkerList)
{
	ASL::InputStringStream buffer(inString);

	boost::shared_ptr<dvacore::UTF16Char> bufPtr(new dvacore::UTF16Char[kMaxBufferLength]);
	dvacore::UTF16Char* buf = bufPtr.get();

	dvacore::UTF16String line;

	std::getline(buffer, line);
	size_t count = ASL::Coercion<size_t>::Result(line);
	
	std::getline(buffer, line);
	dvamediatypes::TickTime originalOffset = dvamediatypes::TickTime::TicksToTime(ASL::Coercion<std::int64_t>::Result(line));
	dvamediatypes::TickTime markerStartOffset = inCurrentCTI - originalOffset;
	for (size_t i = 0; i < count; ++i)
	{
		CottonwoodMarker marker;
		size_t stringLength;
		
		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;

		try 
		{
			marker.mGUID = ASL::Guid(ASL::String(buf));
		}
		catch(...)
		{
			DVA_ASSERT_MSG(0, "Get an invalid Marker ID!");
		}

		std::getline(buffer, line);
		marker.mStartTime = dvamediatypes::TickTime::TicksToTime(ASL::Coercion<std::int64_t>::Result(line)) + markerStartOffset;
		
		std::getline(buffer, line);
		marker.mDuration = dvamediatypes::TickTime::TicksToTime(ASL::Coercion<std::int64_t>::Result(line));

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mComment = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mLocation = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mName = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mProbability = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mSpeaker = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mTarget = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mType = ASL::String(buf);

		std::getline(buffer, line);
		stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
		buffer.read(buf, stringLength);
		buf[stringLength] = 0;
		marker.mCuePointType = ASL::String(buf);
		
		std::getline(buffer, line);
		size_t cuePointCount = ASL::Coercion<size_t>::Result(line);
		
		for (size_t i = 0; i < cuePointCount; ++i)
		{
			std::getline(buffer, line);
			stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
			buffer.read(buf, stringLength);
			buf[stringLength] = 0;
			dvacore::UTF16String key = ASL::String(buf);

			std::getline(buffer, line);
			stringLength = std::min(ASL::Coercion<size_t>::Result(line), static_cast<size_t>(kMaxBufferLength - 1));
			buffer.read(buf, stringLength);
			buf[stringLength] = 0;
			dvacore::UTF16String value = ASL::String(buf);
        
			marker.mCuePointList.push_back(dvatemporalxmp::CustomMarkerParam(key, value));
		}

		std::getline(buffer, line);
		size_t tagParamsCount = ASL::Coercion<size_t>::Result(line);

		for (size_t i = 0; i < tagParamsCount; ++i)
		{
			TagParamPtr newTag(new TagParam());
			newTag->SerializeIn(buffer);
			marker.AddTagParam(newTag);
		}
		
		outMarkerList.push_back(marker);
	}
}

dvamediatypes::TickTime CottonwoodMarker::GetStartTime() const
{
	return mStartTime;
}

dvamediatypes::TickTime CottonwoodMarker::GetDuration() const
{
	return mDuration;
}

dvacore::UTF16String CottonwoodMarker::GetType() const
{
	return mType;
}

dvacore::UTF16String CottonwoodMarker::GetName() const
{
	return mName;
}

/**
**
*/
dvacore::UTF16String CottonwoodMarker::GetNameOrTheFirstKeyword() const
{
	if (!mName.empty())
	{
		return mName;
	}

	if (!mTagParams.empty())
	{
		return (*(mTagParams.begin())).second->GetName();
	}

	return dvacore::UTF16String();
}


dvacore::UTF16String CottonwoodMarker::GetSummary() const
{
	dvacore::UTF16String summary;

	dvacore::UTF16String commentStr = mComment;
	dvacore::UTF16String nameStr = GetName();
	dvacore::UTF16String theFirstTagName = mTagParams.empty() ? 
										dvacore::UTF16String() : 
										(*(mTagParams.begin())).second->GetName();

	// Remove spaces, tabs, and newlines from the beginning of the strings
	commentStr.erase(0, commentStr.find_first_not_of(DVA_STR(" \t\n\r")));
	nameStr.erase(0, nameStr.find_first_not_of(DVA_STR(" \t\n\r")));
	theFirstTagName.erase(0, theFirstTagName.find_first_not_of(DVA_STR(" \t\n\r")));


	// The priority we want the text to work for markers in the marker bar is as follows:
	//
	//	name empty (for all marker types):
	//	1. the first tag name
	//	2. comment
	//	3. marker type
	//
	//	non-empty name (subclip & speech marker)
	//	1. name
	//
	//	non-empty name (other marker types), *** this is only for compatibility with current behavior that
	//	for these types we would prefer comment than name, BUT if this, why we need to consider the first
	//	tag name when marker name is empty???
	//	1. comment
	//	2. name

	if (nameStr.empty())
	{
		if (!theFirstTagName.empty())
		{
			summary = theFirstTagName;
		}
		else
		{
			summary = !commentStr.empty() ? commentStr : MarkerType::GetDisplayNameForType(mType);
		}
	}
	else
	{
		if (mType == MarkerType::kInOutMarkerType ||
			mType == MarkerType::kSpeechMarkerType)
		{
			summary = nameStr;
		}
		else
		{
			summary = !commentStr.empty() ? commentStr : nameStr;
		}
	}

	return summary;
}

ASL::Guid CottonwoodMarker::GetGUID() const
{
	return mGUID;
}

void CottonwoodMarker::SetGUID(
	ASL::Guid inMarkerID)
{
	mGUID = inMarkerID;
}

void CottonwoodMarker::SetStartTime(
	const dvamediatypes::TickTime& inTime)
{
	ASL_ASSERT(inTime >= dvamediatypes::kTime_Zero);
	
	mStartTime = inTime;
}

void CottonwoodMarker::SetDuration(
	const dvamediatypes::TickTime& inTime)
{
	mDuration = inTime;
}

void CottonwoodMarker::SetType(
	const dvacore::UTF16String& inType)
{
	mType = inType;
}

void CottonwoodMarker::SetName(
	const dvacore::UTF16String& inName)
{
	mName = inName;
}

void CottonwoodMarker::SetComment(
	const dvacore::UTF16String& inComment)
{
	mComment = inComment;
}

void CottonwoodMarker::SetLocation(
	const dvacore::UTF16String& inLocation)
{
	mLocation = inLocation;
}

void CottonwoodMarker::SetTarget(
	const dvacore::UTF16String& inTarget)
{
	mTarget = inTarget;
}

void CottonwoodMarker::SetCuePointType(
	const dvacore::UTF16String& inCuePointType)
{
	mCuePointType = inCuePointType;
}

void CottonwoodMarker::SetCuePointList(
	const dvatemporalxmp::CustomMarkerParamList& inCuePointList)
{
	mCuePointList = inCuePointList;
}

void CottonwoodMarker::SetSpeaker(
	const dvacore::UTF16String& inSpeaker)
{
	mSpeaker = inSpeaker;
}

void CottonwoodMarker::SetProbability(
	const dvacore::UTF16String& inProbability)
{
	mProbability = inProbability;
}

dvacore::UTF16String CottonwoodMarker::GetComment() const
{
	return mComment;
}

dvacore::UTF16String CottonwoodMarker::GetLocation() const
{
	return mLocation;
}

dvacore::UTF16String CottonwoodMarker::GetTarget() const
{
	return mTarget;
}

dvacore::UTF16String CottonwoodMarker::GetCuePointType() const
{
	return mCuePointType;
}

const dvatemporalxmp::CustomMarkerParamList& CottonwoodMarker::GetCuePointList() const
{
	return mCuePointList;
}

dvacore::UTF16String CottonwoodMarker::GetSpeaker() const
{
	return mSpeaker;
}

dvacore::UTF16String CottonwoodMarker::GetProbability() const
{
	return mProbability;
}

bool CottonwoodMarker::operator ==(const CottonwoodMarker& inRHS) const
{
	bool result = true;

	if (mGUID != inRHS.mGUID ||
		mStartTime != inRHS.mStartTime ||
		mDuration != inRHS.mDuration ||
		mType != inRHS.mType ||
		mName != inRHS.mName ||
		mComment != inRHS.mComment ||
		mLocation != inRHS.mLocation ||
		mTarget != inRHS.mTarget ||
		mCuePointType != inRHS.mCuePointType ||
		mCuePointList.size() != inRHS.mCuePointList.size() ||
		// This will return false if the two cuePointLists have the same data, but in different orders
		mCuePointList != inRHS.mCuePointList ||
		mSpeaker != inRHS.mSpeaker ||
		mProbability != inRHS.mProbability ||
		!TagsEqual(inRHS))
	{
		result = false;
	}
	return result;
}

bool CottonwoodMarker::TagsEqual(const CottonwoodMarker& inMarker) const
{
	if (mTagParams.size() != inMarker.mTagParams.size())
	{
		return false;
	}

	TagParamMap::const_iterator itL = mTagParams.begin();
	TagParamMap::const_iterator itR = inMarker.mTagParams.begin();
	for (; itL != mTagParams.end(); itL++, itR++)
	{
		if (!(*((*itL).second) == *((*itR).second)))
		{
			return false;
		}
	}

	return true;
}

bool CottonwoodMarker::operator <(const CottonwoodMarker& inRHS) const
{
	return mGUID < inRHS.mGUID;
}

void CottonwoodMarker::AddTagParam(const TagParamPtr& inTagParam)
{
	if (mTagParams.empty())
	{
		mTagParams.insert(TagParamMap::value_type(0, inTagParam));
	}
	else
	{
		// User may do some delete operations, so the index of every item is not guaranteed to be sequential,
		//	so the new item's index should be incremental from the last item's index
		mTagParams.insert(TagParamMap::value_type((*mTagParams.rbegin()).first + 1, inTagParam));
	}
}

void CottonwoodMarker::RemoveTagParamByInstanceGuid(const ASL::Guid& inInstanceGuid)
{
	TagParamMap::iterator it = mTagParams.begin();
	for (; it != mTagParams.end(); it++)
	{
		if ((*it).second->GetInstanceGuid() == inInstanceGuid)
		{
			break;
		}
	}

	if (it != mTagParams.end())
	{
		mTagParams.erase(it);
	}
}

const TagParamMap& CottonwoodMarker::GetTagParams() const
{
	return mTagParams;
}

dvacore::UTF8String CottonwoodMarker::ToPropertyXML() const
{
	dvacore::proplist::PropList propertyList;

	propertyList.SetValue(kMarker_Type, dvacore::utility::UTF16to8(GetType()));
	propertyList.SetValue(kMarker_ID,dvacore::utility::UTF16to8(GetGUID().AsString()));
	propertyList.SetValue(kMarker_Name, dvacore::utility::UTF16to8(GetName()));
	propertyList.SetValue(kMarker_StartTime, dvacore::utility::UTF16to8(dvacore::utility::NumberToString(GetStartTime().GetTicks())));
	propertyList.SetValue(kMarker_Duration, dvacore::utility::UTF16to8(dvacore::utility::NumberToString(GetDuration().GetTicks())));
	propertyList.SetValue(kMarker_Comment, dvacore::utility::UTF16to8(GetComment()));
	propertyList.SetValue(kMarker_Location, dvacore::utility::UTF16to8(GetLocation()));
	propertyList.SetValue(kMarker_Target, dvacore::utility::UTF16to8(GetTarget()));
	propertyList.SetValue(kMarker_Speaker, dvacore::utility::UTF16to8(GetSpeaker()));
	propertyList.SetValue(kMarker_Probability, dvacore::utility::UTF16to8(GetProbability()));
	propertyList.SetValue(kMarker_CuePointType, dvacore::utility::UTF16to8(GetCuePointType()));

	// Loop through cue point params list and add key/value 
	// pairs to the property sub list
	dvacore::proplist::PropList* paramsSubList = propertyList.NewSubList(kMarker_CuePointParams);
	dvatemporalxmp::CustomMarkerParamList paramsList = GetCuePointList();
	dvatemporalxmp::CustomMarkerParamList::size_type i;
	dvacore::UTF16String key;
	dvacore::UTF16String value;

	for (i=0; i < paramsList.size(); i++)
	{
		key = paramsList[i].mKey;
		value = paramsList[i].mValue;

		paramsSubList->SetValue(dvacore::utility::UTF16to8(key), dvacore::utility::UTF16to8(value));
	}

	dvacore::UTF8String dataStr;
	propertyList.OutputAsXMLFragment(dataStr);
	propertyList.Clear();

	return dataStr;
}

dvacore::UTF16String CottonwoodMarker::ToPropertyXML(dvamediatypes::FrameRate const& inFrameRate) const
{
	dvacore::proplist::PropList propertyList;

	propertyList.SetValue(kMarker_Type, dvacore::utility::UTF16to8(GetType()));
	propertyList.SetValue(kMarker_ID,dvacore::utility::UTF16to8(GetGUID().AsString()));
	propertyList.SetValue(kMarker_Name, dvacore::utility::UTF16to8(GetName()));
	propertyList.SetValue(kMarker_StartTime, dvacore::utility::UTF16to8(dvacore::utility::NumberToString(mStartTime.ToFrame64(inFrameRate))));
	propertyList.SetValue(kMarker_Duration, dvacore::utility::UTF16to8(dvacore::utility::NumberToString(mDuration.ToFrame64(inFrameRate))));
	propertyList.SetValue(kMarker_Comment, dvacore::utility::UTF16to8(GetComment()));
	propertyList.SetValue(kMarker_Location, dvacore::utility::UTF16to8(GetLocation()));
	propertyList.SetValue(kMarker_Target, dvacore::utility::UTF16to8(GetTarget()));
	propertyList.SetValue(kMarker_Speaker, dvacore::utility::UTF16to8(GetSpeaker()));
	propertyList.SetValue(kMarker_Probability, dvacore::utility::UTF16to8(GetProbability()));
	propertyList.SetValue(kMarker_CuePointType, dvacore::utility::UTF16to8(GetCuePointType()));

	if (inFrameRate != dvamediatypes::kFrameRate_Zero)
	{
		dvamediatypes::RatioTime frameRateRatio = dvamediatypes::FrameRateToRatio(inFrameRate);
		ASL::String frameRateStr = DVA_STR("f") + dvacore::utility::NumberToString(frameRateRatio.numerator());
		if (frameRateRatio.denominator() != 1)
			frameRateStr += DVA_STR("s") + dvacore::utility::NumberToString(frameRateRatio.denominator());
		propertyList.SetValue(kMarker_FrameRate, dvacore::utility::UTF16to8(frameRateStr));
	}

	// Loop through cue point params list and add key/value 
	// pairs to the property sub list
	dvacore::proplist::PropList* paramsSubList = propertyList.NewSubList(kMarker_CuePointParams);
	for (dvatemporalxmp::CustomMarkerParamList::size_type i=0; i < mCuePointList.size(); ++i)
	{
		dvacore::UTF16String key = mCuePointList[i].mKey;
		dvacore::UTF16String value = mCuePointList[i].mValue;
		paramsSubList->SetValue(dvacore::utility::UTF16to8(key), dvacore::utility::UTF16to8(value));
	}

	// serialize tag params
	dvacore::proplist::PropList* tagListSubList = propertyList.NewSubList(kMarker_TagList);
	for (TagParamMap::const_iterator it = mTagParams.begin();
		it != mTagParams.end();
		it++)
	{
		it->second->ToProperty(tagListSubList);
	}

	dvacore::UTF8String dataStr;
	propertyList.OutputAsXMLFragment(dataStr);
	propertyList.Clear();

	return dvacore::utility::UTF8to16(dataStr);
}

bool CottonwoodMarker::FromPropertyXML(dvacore::UTF16String const& inPropertyXML, dvamediatypes::FrameRate const& inFrameRate)
{
	dvacore::UTF8String str = dvacore::utility::UTF16to8(inPropertyXML);
	dvacore::proplist::PropList markerProps;
	try 
	{
		dvacore::proplist::utils::ReadPropList(str, markerProps);
	}
	catch(...)
	{
		return false;
	}

	// Set standard marker fields
	std::list<dvacore::UTF8String> propertyNames;
	markerProps.GetPropertyNames(propertyNames);
	std::list<dvacore::UTF8String>::iterator namesIter = propertyNames.begin();
	std::list<dvacore::UTF8String>::iterator namesIterEnd = propertyNames.end();

	for (; namesIter != namesIterEnd; ++namesIter)
	{
		dvacore::UTF8String field = (*namesIter);

		// Set value, except for cuePointParams and tagList. That value will be handled separately.
		if (field != dvacore::utility::AsciiToUTF8("cuePointParams")
		&& field != dvacore::utility::AsciiToUTF8("tagList"))
		{
			dvacore::UTF8String valueStr;
			markerProps.GetValue(field, valueStr);
			dvacore::UTF16String value = dvacore::utility::UTF8to16(valueStr);

			// Don't write out the Multiple Values string
			if (value == PL::GetMultipleValuesString())
			{
				continue;
			}

			if (field == kMarker_ID)
			{
				SetGUID(ASL::Guid(value));
			}
			else if (field == kMarker_Type)
			{
				SetType(value);
				// Check if the type is in the Marker Type Registry.
				// If not, register the type
				MarkerTypeRegistry types = MarkerType::GetAllMarkerTypes();
				if (types.find(value) == types.end())
				{
					MarkerType::AddMarkerToRegistry(value, value, value, dvacore::UTF16String());
				}
			}
			else if (field == kMarker_Name)
			{
				SetName(value);
			}
			else if (field == kMarker_Comment)
			{
				SetComment(value);
			}
			else if (field == kMarker_Location)
			{
				SetLocation(value);
			}
			else if (field == kMarker_Target)
			{
				SetTarget(value);
			}
			else if (field == kMarker_Speaker)
			{
				SetSpeaker(value);
			}
			else if (field == kMarker_Probability)
			{
				SetProbability(value);
			}
			else if (field == kMarker_CuePointType)
			{
				SetCuePointType(value);
			}
			else if (field == kMarker_StartTime)
			{
				std::int64_t startTime;
				dvacore::utility::StringToNumber(startTime, value);
				SetStartTime(dvamediatypes::TickTime::TicksToTime(startTime * inFrameRate.GetTicksPerFrame()));
			}
			else if (field == kMarker_Duration)
			{
				std::int64_t duration;
				dvacore::utility::StringToNumber(duration, value);
				SetDuration(dvamediatypes::TickTime::TicksToTime(duration * inFrameRate.GetTicksPerFrame()));
			}
		}
		else if (field == kMarker_CuePointParams)
		{
			dvacore::proplist::PropList *newCuePointParams = markerProps.GetSubList(kMarker_CuePointParams);

			std::list<dvacore::UTF8String> paramNames;
			newCuePointParams->GetPropertyNames(paramNames);
			std::list<dvacore::UTF8String>::iterator paramsIter = paramNames.begin();
			std::list<dvacore::UTF8String>::iterator paramsIterEnd = paramNames.end();

			for (; paramsIter != paramsIterEnd; ++paramsIter)
			{
				dvacore::UTF8String	paramStr = (*paramsIter);
				dvacore::UTF8String paramValueStr;
				newCuePointParams->GetValue(paramStr, paramValueStr);
				dvacore::UTF16String param = dvacore::utility::UTF8to16(paramStr);
				dvacore::UTF16String paramValue = dvacore::utility::UTF8to16(paramValueStr);

				// Don't write out the Multiple Values string
				if (paramValue == PL::GetMultipleValuesString())
				{
					continue;
				}

				bool found = false;
				dvatemporalxmp::CustomMarkerParamList::iterator cuePointIter = mCuePointList.begin();
				for (; cuePointIter != mCuePointList.end(); cuePointIter++)
				{
					if ((*cuePointIter).mKey == param)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
                    dvatemporalxmp::CustomMarkerParam newParam(param, paramValue);
					mCuePointList.push_back(newParam);
				}
				else
				{
					if (paramValue.length() > 0)
					{
						(*cuePointIter).mValue = paramValue;
					}
					else
					{
						mCuePointList.erase(cuePointIter);
					}
				}
			}
		}
		else if (field == kMarker_TagList)
		{
			dvacore::proplist::PropList *newTagList = markerProps.GetSubList(kMarker_TagList);

			std::list<dvacore::UTF8String> tagIDs;
			newTagList->GetPropertyNames(tagIDs);
			std::list<dvacore::UTF8String>::iterator tagIDsIter = tagIDs.begin();
			std::list<dvacore::UTF8String>::iterator tagIDsIterEnd = tagIDs.end();

			for (; tagIDsIter != tagIDsIterEnd; ++tagIDsIter)
			{
				ASL::Guid tagID(dvacore::utility::UTF8to16(*tagIDsIter));
				TagParamPtr changedTag;
				PL::TagParamMap::iterator tagMapIter = mTagParams.begin();
				for (; tagMapIter != mTagParams.end(); ++tagMapIter)
				{
					if (tagMapIter->second->GetInstanceGuid() == tagID)
					{
						changedTag = tagMapIter->second;
						break;
					}
				}
				if (changedTag == NULL)
				{
					changedTag = TagParamPtr(new TagParam);
					AddTagParam(changedTag);
				}

				dvacore::proplist::PropList *newTag = newTagList->GetSubList(*tagIDsIter);
				changedTag->FromProperty(tagID, newTag);
			}
		}
	}
	markerProps.Clear();
	return true;
}
    
void CottonwoodMarker::SetMarkerOwner(PL::ISRMarkerOwnerRef inMarkerOwner)
{
    DVA_ASSERT(inMarkerOwner);
    mMarkerOwner = ASL::IWeakReferenceCapableRef(inMarkerOwner)->GetWeakReference();
}

PL::ISRMarkerOwnerRef CottonwoodMarker::GetMarkerOwner() const
{
    DVA_ASSERT(mMarkerOwner);
    if (mMarkerOwner)
    {
        ASL::ReferenceBridge bridge(mMarkerOwner);
        return PL::ISRMarkerOwnerRef(bridge.GetStrongReference());
    }
    
    return PL::ISRMarkerOwnerRef();
}


ASL::Color CottonwoodMarker::GetColor() const
{
	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef bProp(backend);

	// Populate controls
	bool isTagColorForMarker = true;
	bProp->GetValue(PL::kPropertyKey_SwitchUseTagColorMarker, isTagColorForMarker);

	if (isTagColorForMarker)
	{
		PL::TagParamMap paramMap = GetTagParams();
		TagParamPtr tag;

		// Get first valid tags
		for (TagParamMap::iterator it = paramMap.begin(); it != paramMap.end(); it++)
		{
			if(it->second)
			{
				tag = it->second;
				break;
			}
		}

		if (tag)
		{
			return UIF::DVAConversionUtilities::DVAColorRGBAtoASLColor(tag->GetColor());
		}
	}
	
	return PL::MarkerType::GetColorForType(GetType());
}

}
