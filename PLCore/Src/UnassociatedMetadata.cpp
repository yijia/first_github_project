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
#include "PLMessage.h"
#include "PLUtilities.h"
#include "PLUnassociatedMarkers.h"
#include "PLUnassociatedMetadata.h"
#include "PLFactory.h"
#include "PLProject.h"
#include "PLMarkers.h"
#include "PLConstants.h"

//  MZ
#include "MZUtilities.h"

#include "boost/bind.hpp"


//	DVA
#include "dvaui/ui/OS_UI.h"
#include "dvamediatypes/Timecode.h"
#include "dvamediatypes/TimeDisplayUtils.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLFile.h"
#include "ASLStationUtils.h"

namespace PL
{

namespace
{

const char* const kNSLiveLogging = "http://ns.adobe.com/xmp/liveLogging/";
const char* const kPropertyDayOffset = "dayOffset";
const char* const kPropertyTimecodeOffset = "timecodeOffset";

/*
**
*/
bool ReadUnassociatedMetadata(ASL::String const& inXMPPath, SXMPMeta& outXMPMeta)
{
	// read the serialized XMP from a sidecar file
	std::vector<char> charBuffer;
	ASL::File xmpFile;
	bool result(false);
	if (ASL::ResultSucceeded(ASL::File::Open(inXMPPath, ASL::FileAccessFlags::kRead, xmpFile)))
	{
		ASL::UInt32 bytesRead = 0;
		std::size_t bytesOnDisk = static_cast<std::size_t>(xmpFile.SizeOnDisk());
		if (bytesOnDisk == 0)
		{
			return false;
		}

		charBuffer.resize(bytesOnDisk);
		if(ASL::ResultSucceeded(xmpFile.Read(&charBuffer[0], static_cast<ASL::UInt32>(bytesOnDisk), bytesRead)))
		{
			try
			{
				outXMPMeta.ParseFromBuffer(&charBuffer[0], bytesRead);
				result = true;
			}
			catch (XMP_Error& p)
			{
				ASL_UNUSED(p);
				result = false;
			}
		}
	}

	return result;
}

bool ReadAltTimecode(
	SXMPMeta inXMPMeta, 
	dvamediatypes::TickTime& outStartTime, 
	dvamediatypes::TimeDisplay& outVideoTimeDisplay)
{
	bool hasActualStartTime = false;

	// Try to get "timecodeOffset" to see if it is from PLL
	std::string startTimeString;
	dvamediatypes::FrameRate frameRate;
	XMP_StringLen stringLen;
	try
	{
		hasActualStartTime = 
			inXMPMeta.GetProperty(kNSLiveLogging, kPropertyTimecodeOffset, &startTimeString, &stringLen);
		if (hasActualStartTime)
		{
			dvatemporalxmp::XMPDocOpsTemporal docOps;
			docOps.OpenXMP(&inXMPMeta);
			dvatemporalxmp::XMPDocOpsTemporal::XMPTrackListIter it = docOps.TrackListIteratorBegin();
			if (it != docOps.TrackListIteratorEnd())
			{
				dvatemporalxmp::XMPTrack* xmpTrack = &*it;

				dvatemporalxmp::DVATime numerator, denominator;
				numerator = xmpTrack->GetFrameRateNumerator();
				denominator = xmpTrack->GetFrameRateDenominator();

				if (numerator != kXMP_FrameRate_DefaultNumerator || 
					denominator != kXMP_FrameRate_DefaultDenominator)
				{
					frameRate = dvamediatypes::FrameRate(numerator * 1.0 / denominator);

					ASL::String timeString(ASL::MakeString(startTimeString));
					bool isDropFrame = (timeString.find(ASL_STR(";")) != ASL::String::npos);
					outVideoTimeDisplay = dvamediatypes::GetTimeDisplayForFrameRate(frameRate, isDropFrame, false);
		
					dvamediatypes::FrameRate newFrameRate = 
						dvamediatypes::GetFrameRateForTimeDisplay(outVideoTimeDisplay);
					if (newFrameRate != frameRate)
					{
						// LiveLogger is using a non-standard frame rate;
						outVideoTimeDisplay = dvamediatypes::kTimeDisplay_25Timecode;
						newFrameRate = dvamediatypes::GetFrameRateForTimeDisplay(outVideoTimeDisplay);
					}

					outStartTime = PL::Utilities::TimecodeToTickTimeWithoutRoundingFrameRate(
						ASL::MakeString(startTimeString.c_str()), 
						newFrameRate,
						outVideoTimeDisplay);
					return true;
				}
			}
			else
			{
				return false;
			}
		}
	}
	catch(...)
	{

	}

	// If it is not from PLL, try to get "altTimecode"
	std::string altStartTimeString, timeFormatString;
	hasActualStartTime = 
		inXMPMeta.GetStructField(kXMP_NS_DM, Utilities::kAltTimecode, kXMP_NS_DM, Utilities::kTimeValue, &altStartTimeString, &stringLen);
	if (hasActualStartTime)
	{
		inXMPMeta.GetStructField(kXMP_NS_DM, Utilities::kAltTimecode, kXMP_NS_DM, Utilities::kTimeFormat, &timeFormatString, &stringLen);
		dvamediatypes::TimeDisplay timeDisplay = Utilities::ConvertStringToTimeDisplay(ASL::MakeString(timeFormatString.c_str()));
		if (timeDisplay != dvamediatypes::kTimeDisplay_Invalid)
		{
			outVideoTimeDisplay = timeDisplay;
			outStartTime = PL::Utilities::TimecodeToTickTimeWithoutRoundingFrameRate(
				ASL::MakeString(altStartTimeString.c_str()), 
				dvamediatypes::GetFrameRateForTimeDisplay(timeDisplay, dvamediatypes::kFrameRate_VideoPAL),
				timeDisplay);

			return true;
		}
	}

	return false;
}

}

/*
**
*/
SRUnassociatedMetadata::SRUnassociatedMetadata()
	:
	mIsCreativeCloud(false),
	mMarkerLoaded(false),
	mOffsetType(OffsetType_NoneOffset),
	mOffset(dvamediatypes::kTime_Zero),
	mTimeDisplay(dvamediatypes::kTimeDisplay_Invalid)
{
}

/*
**
*/
SRUnassociatedMetadata::~SRUnassociatedMetadata()
{
}

/*
**
*/
SRUnassociatedMetadataRef SRUnassociatedMetadata::Create(
	ASL::String const& inMetadataID,
	const ASL::String& inMetadataPath)
{
	ASL_ASSERT(!inMetadataID.empty());
	if (!inMetadataID.empty())
	{
		SRUnassociatedMetadataRef srUnassociatedMetadata(new SRUnassociatedMetadata());
		srUnassociatedMetadata->Initialize(inMetadataID, inMetadataPath);
		return srUnassociatedMetadata;
	}
	return SRUnassociatedMetadataRef();
}
	
/*
**
*/
SRUnassociatedMetadataRef SRUnassociatedMetadata::Create(
	const ASL::String& inMetadataID, 
	const ASL::String& inMetadataPath,
	const XMPText& inXMPText)
{
	ASL_ASSERT(!inMetadataID.empty() && inXMPText);
	if (!inMetadataID.empty() &&
		inXMPText)
	{
		SRUnassociatedMetadataRef srUnassociatedMetadata(new SRUnassociatedMetadata());
		srUnassociatedMetadata->Initialize(inMetadataID, inMetadataPath, inXMPText);
		return srUnassociatedMetadata;
	}
	return SRUnassociatedMetadataRef();
}

/*
**
*/
void SRUnassociatedMetadata::Initialize(
	ASL::String const& inMetadataID,
	const ASL::String& inMetadataPath)
{
	mMetadataID = inMetadataID;

	mMetadataPath = inMetadataPath;
#if ASL_TARGET_OS_WIN
	if (ASL::PathUtils::IsNormalizedPath(mMetadataPath))
	{
		mMetadataPath = ASL::PathUtils::Win::StripUNC(mMetadataPath);
	}
#endif

	mMarkers = SRUnassociatedMarkers::Create(inMetadataID);
}

/*
**
*/
void SRUnassociatedMetadata::SetIsCreativeCloud(bool isCreativeCloud)
{
	mIsCreativeCloud = isCreativeCloud;
}

/*
**
*/
bool SRUnassociatedMetadata::GetIsCreativeCloud()
{
	return mIsCreativeCloud;
}

/*
**
*/
void SRUnassociatedMetadata::Initialize(
	ASL::String const& inMetadataID, 
	const ASL::String& inMetadataPath,
	const XMPText& inXMPText)
{
	mMetadataID = inMetadataID;

	mMetadataPath = inMetadataPath;
#if ASL_TARGET_OS_WIN
	if (ASL::PathUtils::IsNormalizedPath(mMetadataPath))
	{
		mMetadataPath = ASL::PathUtils::Win::StripUNC(mMetadataPath);
	}
#endif

	mMarkers = SRUnassociatedMarkers::Create(mMetadataID);

	// The unassociated metadata may has actual start time or dayoffset to calculate the absolute applied position
	//	If the xmp is from Prelude, there should be "altTimecode"
	//	If the xmp is from PLL, there should be "timecodeOffset" to indicate a start timecode, or there should be
	//	"dayOffset" to indicate Time of Day instead of Ticks.
	SXMPMeta xmpMetadata(inXMPText->c_str(), (XMP_StringLen)inXMPText->length());

	dvamediatypes::TickTime startTimecode;
	dvamediatypes::TimeDisplay videoTimeDisplay;
	bool hasActualStartTime = ReadAltTimecode(xmpMetadata, startTimecode, videoTimeDisplay);
	if (hasActualStartTime)
	{
		mOffsetType = OffsetType_Timecode;
		mOffset = startTimecode;
		mTimeDisplay = videoTimeDisplay;
	}
	else
	{
		ASL::Date::ExtendedTimeType dayOffset;
		bool hasDayOffset = 
			Utilities::ExtractDateFromXMP(inXMPText, kNSLiveLogging, kPropertyDayOffset, dayOffset);
		if (hasDayOffset)
		{
			mOffsetType = OffsetType_Date;
			mOffset = Utilities::ConvertDayTimeToTick(dayOffset);
		}
	}

	mMarkers->BuildMarkersFromXMPString(inXMPText, true);
	mMarkerLoaded = true;
}

/*
**
*/
void SRUnassociatedMetadata::ReadUnassociatedMetadataIntoMemory()
{
	SXMPMeta xmpMeta;
	if (ReadUnassociatedMetadata(mMetadataID, xmpMeta))
	{
		ASL::StdString fileXMPBuffer;
		xmpMeta.SerializeToBuffer(&fileXMPBuffer);
		XMPText xmpText(new ASL::StdString(fileXMPBuffer));

		// The unassociated metadata may has actual start time or dayoffset to calculate the absolute applied position
		//	If the xmp is from Prelude, there should be "altTimecode"
		//	If the xmp is from PLL, there should be "timecodeOffset" to indicate a start timecode, or there should be
		//	"dayOffset" to indicate Time of Day instead of Ticks.
		dvamediatypes::TickTime startTimecode;
		dvamediatypes::TimeDisplay videoTimeDisplay;
		bool hasActualStartTime = ReadAltTimecode(xmpMeta, startTimecode, videoTimeDisplay);
		if (hasActualStartTime)
		{
			mOffsetType = OffsetType_Timecode;
			mOffset = startTimecode;
			mTimeDisplay = videoTimeDisplay;
		}
		else
		{
			ASL::Date::ExtendedTimeType dayOffset;
			bool hasDayOffset = 
				Utilities::ExtractDateFromXMP(xmpText, kNSLiveLogging, kPropertyDayOffset, dayOffset);
			if (hasDayOffset)
			{
				mOffsetType = OffsetType_Date;
				mOffset = Utilities::ConvertDayTimeToTick(dayOffset);
			}
		}

		mMarkers->BuildMarkersFromXMPString(xmpText, true);
	}
}

/*
**
*/
ASL::String SRUnassociatedMetadata::GetMetadataPath() const
{
	return mMetadataPath;
}

/*
**
*/
ASL::String SRUnassociatedMetadata::GetMetadataID() const
{
	return mMetadataID;
}

/*
**
*/
ASL::String SRUnassociatedMetadata::GetAliasName()
{
	if (mAliasName.empty())
	{
		return mMetadataPath;
	}

	return mAliasName;
}

/*
**
*/
PL::ISRUnassociatedMarkersRef SRUnassociatedMetadata::GetMarkers()
{
	if (!mMarkerLoaded)
	{
		mMarkerLoaded = true;
		ReadUnassociatedMetadataIntoMemory();
	}
	return mMarkers;
}

/*
**
*/
void SRUnassociatedMetadata::SetAliasName(ASL::String const& inAliasName)
{
	mAliasName = inAliasName;
}

/*
**
*/
SRUnassociatedMetadata::OffsetType SRUnassociatedMetadata::GetOffsetType() const
{
	return mOffsetType;
}

/*
**
*/
dvamediatypes::TickTime SRUnassociatedMetadata::GetOffset() const
{
	return mOffset;
}

/*
**
*/
dvamediatypes::TimeDisplay SRUnassociatedMetadata::GetTimeDisplay() const
{
	return mTimeDisplay;
}

}
