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

#include "ASLStationRegistry.h"

#include "PLMediaMetadataHelper.h"


namespace PL
{

	MediaMetaDataHelper::MediaMetaDataHelper()
		:
		mStationID(ASL::StationRegistry::RegisterUniqueStation())
	{
	}

	MediaMetaDataHelper::~MediaMetaDataHelper()
	{
		ASL::StationRegistry::DisposeStation(mStationID);
	}

	// IMediaMetaData methods

	bool MediaMetaDataHelper::CanWriteTimecode() const
	{
		return true;
	}

	ASL::Result MediaMetaDataHelper::SetTimeInfo(
		const MF::BinaryData& inPrefs,
		const dvamediatypes::TickTime& inOriginalTime,
		const dvamediatypes::TickTime& inAlternateTime, 
		const ASL::String& inOriginalTapeName, 
		const ASL::String& inAlternateTapeName, 
		const ASL::String& inLogComment, 
		const dvamediatypes::FrameRate& inLogFrameRate,
		const bool inDropFrame)
	{
		return ASL::kSuccess;
	}

	dvamediatypes::TickTime MediaMetaDataHelper::GetOrigTime() const
	{
		ASL_ASSERT(false);
		return dvamediatypes::kTime_Zero;
	}

	dvamediatypes::TickTime MediaMetaDataHelper::GetAltTime() const
	{
		ASL_ASSERT(false);
		return dvamediatypes::kTime_Zero;
	}
	
	ASL::String MediaMetaDataHelper::GetLogComment() const
	{
		ASL_ASSERT(false);
		return ASL::String();
	}

	ASL::String MediaMetaDataHelper::GetOrigReel() const
	{
		ASL_ASSERT(false);
		return ASL::String();
	}

	ASL::String MediaMetaDataHelper::GetAltReel() const
	{
		ASL_ASSERT(false);
		return ASL::String();
	}

	bool MediaMetaDataHelper::GetIsDropFrame() const
	{
		ASL_ASSERT(false);
		return false;
	}
	dvamediatypes::PixelAspectRatio MediaMetaDataHelper::GetPixelAspectRatio() const
	{
		ASL_ASSERT(false);
		return dvamediatypes::kPixelAspectRatio_Any;
	}
	
	dvamediatypes::FieldType MediaMetaDataHelper::GetFieldType() const
	{
		ASL_ASSERT(false);
		return dvamediatypes::kFieldType_Progressive;
	}
	
	bool MediaMetaDataHelper::GetUseAlternateStart() const
	{
		ASL_ASSERT(false);
		return false;
	}

	ASL::Result MediaMetaDataHelper::GetMetadataState(dvacore::utility::Guid& outMetadataState, const dvacore::StdString& inPartID)
	{
		ASL_ASSERT(false);
		return ASL::kSuccess;
	}

	ASL::Result MediaMetaDataHelper::InjectMetadata(
		BE::IClipLoggingInfoRef inClipLogging,
		BE::ICaptureSettingsRef inCaptureSettings)
	{
		ASL_ASSERT(false);
		return ASL::kSuccess;
	}

	ASL::Result MediaMetaDataHelper::GetXMPCaptureProperties(
		ASL::String& outTapeName,
		ASL::String& outClipName,
		ASL::String& outDescription,
		ASL::String& outScene,
		ASL::String& outShot,
		ASL::String& outLogNote,
		ASL::String& outComment,
		ASL::String& outClient,
		ASL::Date& outShotDate,
		bool& outGood,
		ASL::String& outClipID)
	{
		ASL_ASSERT(false);
		return ASL::kSuccess;
	}

	ASL::Result MediaMetaDataHelper::UpdateXMPMetadataFromMedia(bool inCommitChangesToDisk)
	{
		ASL_ASSERT(false);
		return ASL::kSuccess;
	}

	BE::MediaSceneRecords MediaMetaDataHelper::GetMediaScenes()
	{
		ASL_ASSERT(false);
		return BE::MediaSceneRecords();
	}

	bool MediaMetaDataHelper::GetIsPreviewRenderNoAlphaDetected()
	{
		ASL_ASSERT(false);
		return false;
	}

	ASL::StationID MediaMetaDataHelper::GetStationID() const
	{
		return mStationID;
	}

	bool MediaMetaDataHelper::CanWriteXMPToFile() const
	{
		return true;
	}
}