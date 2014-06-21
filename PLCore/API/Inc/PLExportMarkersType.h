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

#ifndef PLEXPORTMARKERSTYPE_H
#define PLEXPORTMARKERSTYPE_H

#pragma once

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#include "dvacore/config/Localizer.h"

namespace PL
{

	namespace ExportMarker
	{
		struct AssetBasicInfo
		{
			ASL::String					name;
			ASL::String					mediaPath;
			ASL::String					parentBinPath;
			bool						isDropFrame;
			dvamediatypes::FrameRate	frameRate;
			dvamediatypes::TickTime		mediaOffset;
		};

		typedef boost::shared_ptr<AssetBasicInfo> AssetBasicInfoPtr;
		typedef std::pair<AssetBasicInfoPtr,  CottonwoodMarkerList> AssetItemMarkersInfoPair;
		typedef std::vector<AssetItemMarkersInfoPair> AssetItemMarkersInfoPairVec;

		typedef boost::function<void(bool, bool, ASL::String)> ExportMarkersCallbackFn;

		/*
		** record final status and use to call finish dialog when all markers are exported 
		*/
		struct ExportMarkersResult
		{
			ExportMarkersCallbackFn finishCallbackFn;
			bool allSaveSucceed;
			bool allItemsIsValid;
			ASL::String exportFilePath;
		};

		typedef boost::shared_ptr<ExportMarkersResult> ExportMarkersResultPtr;

		const char* const sAssetName = "$$$/Prelude/PLCore/ExportMarkersAsFile/assetNameHeader=Asset Name";
		const char* const sAssetParentBinPath = "$$$/Prelude/PLCore/ExportMarkersAsFile/assetParentBinPathHeader=Bin";
		const char* const sMarkerThumbnailImage = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerThumbnailImageHeader=Thumbnail Image";
		const char* const sMarkerName = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerNameHeader=Marker Name";
		const char* const sMarkerComment = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerDescriptionHeader=Description";
		const char* const sMarkerInTime = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerInHeader=In";
		const char* const sMarkerOutTime = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerOutHeader=Out";
		const char* const sMarkerDuration = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerDurationHeader=Duration";
		const char* const sMarkerType = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerTypeHeader=Marker Type";
		const char* const sAssetMediaPath = "$$$/Prelude/PLCore/ExportMarkersAsFile/assetMediaPathHeader=File Path";
		const char* const sMarkerTags = "$$$/Prelude/PLCore/ExportMarkersAsFile/markerTags=Tags";
	}
}  // End PL

#endif