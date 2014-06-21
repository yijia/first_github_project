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

// Prefix
#include "Prefix.h"

// ASL
#include "ASLFile.h"
#include "ASLResourceUtils.h"
#include "ASLStringUtils.h"
#include "ASLContract.h"
#include "ASLTimecode.h"
#include "ASLTimecode.h"

// Local
#include "PLExportMarkersAsCSVFile.h"
#include "PLExportMarkersType.h"
#include "PLUtilities.h"
#include "PLUtilitiesPrivate.h"

// BE
#include "BE/Marker/IMarker.h"

// MZ
#include "MZUtilities.h"

// ML
#include "SDKErrors.h"

// dvacore
#include "dvacore/config/Localizer.h"

// boost
#include "boost/foreach.hpp"

namespace PL
{
namespace
{
	const ASL::String tabDelimiter(ASL_STR("\t"));
	const char* const kFailToWriteToFile = "$$$/Prelude/PLCore/ExportMarkersAsCSVFileError=The @0.csv file can not be saved successfully.";
	const char* const kWriteToFileException = "$$$/Prelude/PLCore/ExportMarkersAsCSVFileException=The @0.csv file can not be saved successfully due to exception.";

	const ASL::String CSV_FILE_EXT = ASL_STR(".csv");
	const ASL::String EMPTY_MARKER_STR = ASL_STR("\t\t\t\t\t\t\t");

	typedef std::vector<ASL::String> MarkerRows;


	ASL::String EscapeSpecialChar(const ASL::String& inString)
	{
		ASL::String::size_type specialCharPos = inString.find_first_of(ASL_STR("\r\t\n"));
		if (specialCharPos != ASL::String::npos)
		{
			ASL::String tempString = inString;
			ASL::String quoteDelimiter = ASL_STR("\"");
			if (tempString.find_first_of(quoteDelimiter) != ASL::String::npos)
			{
				dvacore::utility::ReplaceAllStringInString(tempString, quoteDelimiter, ASL_STR("\"\""));
			}
			return quoteDelimiter + tempString + quoteDelimiter;
		}
		return inString;
	}

	/*
	**
	*/
	ASL::String BuildMarkerRow( const CottonwoodMarker& inMarkerItem, 
								const dvamediatypes::FrameRate& inFrameRate, 
								const dvamediatypes::TickTime& inMediaOffet,
								bool inIsDropFrame)
	{
		ASL::String markerRow;

		const ASL::String markerName = EscapeSpecialChar(inMarkerItem.GetName());
		const ASL::String markerComment = EscapeSpecialChar(inMarkerItem.GetComment());

		// Name
		markerRow += markerName;
		markerRow += tabDelimiter;
		// Description
		markerRow += markerComment;
		markerRow += tabDelimiter;

		// in timecode
		markerRow += PL::Utilities::ConvertTimecodeToString(inMediaOffet + inMarkerItem.GetStartTime(), inFrameRate, inIsDropFrame);
		markerRow += tabDelimiter;
		
		const dvamediatypes::TickTime outTimeCode = inMediaOffet + inMarkerItem.GetStartTime() + inMarkerItem.GetDuration();
		// out timecode
		markerRow += PL::Utilities::ConvertTimecodeToString(outTimeCode, inFrameRate, inIsDropFrame);
		markerRow += tabDelimiter;
		
		// during timecode
		markerRow += PL::Utilities::ConvertTimecodeToString(inMarkerItem.GetDuration(), inFrameRate, inIsDropFrame);
		markerRow += tabDelimiter;

		// marker Type
		ASL::String markerType = inMarkerItem.GetType();
		if (markerType == ASL_STR("InOut"))
		{
			markerType = ASL_STR("Subclip");
		}
		markerRow += markerType;
		markerRow += tabDelimiter;

		// marker tags
		markerRow += EscapeSpecialChar(PL::SRUtilitiesPrivate::ConvertMarkerTagsToString(inMarkerItem.GetTagParams()));
		markerRow += tabDelimiter;

		return markerRow;
	}

	inline void AddStringItem(ASL::String& ioString, const ASL::String& inItem)
	{
		ioString += inItem;
		ioString += tabDelimiter;
	}

	ASL::String BuildCSVHeader()
	{
		ASL::String csvHeader;
		const ASL::String assetName(dvacore::ZString(PL::ExportMarker::sAssetName));
		const ASL::String assetParentBinPath(dvacore::ZString(PL::ExportMarker::sAssetParentBinPath));
		const ASL::String markerName(dvacore::ZString(PL::ExportMarker::sMarkerName));
		const ASL::String markerComment(dvacore::ZString(PL::ExportMarker::sMarkerComment));
		const ASL::String markerInTime(dvacore::ZString(PL::ExportMarker::sMarkerInTime));
		const ASL::String markerOutTime(dvacore::ZString(PL::ExportMarker::sMarkerOutTime));
		const ASL::String markerDuration(dvacore::ZString(PL::ExportMarker::sMarkerDuration));
		const ASL::String markerType(dvacore::ZString(PL::ExportMarker::sMarkerType));
		const ASL::String markerTags(dvacore::ZString(PL::ExportMarker::sMarkerTags));
		const ASL::String assetMediaPath(dvacore::ZString(PL::ExportMarker::sAssetMediaPath));

		AddStringItem(csvHeader, assetName);
		AddStringItem(csvHeader, assetParentBinPath);
		AddStringItem(csvHeader, markerName);
		AddStringItem(csvHeader, markerComment);
		AddStringItem(csvHeader, markerInTime);
		AddStringItem(csvHeader, markerOutTime);
		AddStringItem(csvHeader, markerDuration);
		AddStringItem(csvHeader, markerType);
		AddStringItem(csvHeader, markerTags);
		AddStringItem(csvHeader, assetMediaPath);

		return csvHeader;
	}

	ASL::String GetValidFilePath(const ASL::String& inFilePath,
		ASL::String& ioFileName,
		const ASL::String& inFielExtension)
	{
		ASL::String validFullPath;
		if (ioFileName.empty())
		{
			ioFileName = ASL_STR("EmptyClipName");
		}

		validFullPath = inFilePath + ioFileName + inFielExtension;

		ASL::UInt32 number = 0;
		ASL::String tempFileName = ioFileName;
		while (ASL::PathUtils::ExistsOnDisk(validFullPath))
		{
			++number;
			tempFileName = ioFileName + ASL_STR("_") + ASL::Coercion<ASL::String>::Result(number);
			validFullPath = inFilePath + tempFileName + inFielExtension;
		}

		ioFileName = tempFileName;

		return validFullPath;
	}

	bool WriteCSVFile(const std::vector<ASL::String>& inMarkersLineVec, 
							 const ASL::String& inFilePath,
							 const ASL::String& inFileExtension)
	{
		typedef dvacore::oUTF16StringStream OutputStringStream;

		OutputStringStream outputStream;

		BOOST_FOREACH(const ASL::String& item, inMarkersLineVec)
		{
			outputStream << item << ASL_STR("\r\n");
		}

		ASL::Result result;
		ASL::File outputFile;

		try 
		{
			result = outputFile.Create(
				inFilePath,
				ASL::FileAccessFlags::kWrite,
				ASL::FileShareModeFlags::kNone,
				ASL::FileCreateDispositionFlags::kCreateAlways,
				ASL::FileAttributesFlags::kAttributeNormal);

			if (ASL::ResultSucceeded(result))
			{
				// Unicode BOM
				ASL::UInt16 unicodeIdentifier = 0xFEFF;
				ASL::UInt32 headToWrite = 2;
				ASL::UInt32 headsWritten = 0;
				result = outputFile.Write(&unicodeIdentifier, headToWrite, headsWritten);

				dvacore::UTF16String str = outputStream.str();
				ASL::UInt32 bytesToWrite = static_cast<ASL::UInt32>(str.size() * sizeof(dvacore::UTF16String::value_type));
				ASL::UInt32 bytesWritten = 0;
				result = outputFile.Write(str.data(), bytesToWrite, bytesWritten);
			}

			if (!ASL::ResultSucceeded(result))
			{
				ML::SDKErrors::SetSDKErrorString(dvacore::ZString(kFailToWriteToFile, inFilePath));
				return false;
			}
		}
		catch (...)
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(kWriteToFileException, inFilePath));
			return false;
		}

		return true;
	}

 } // namespace

 bool SaveSingleFile(const PL::ExportMarker::AssetItemMarkersInfoPairVec& inItemMarkersInfo,
					 const ASL::String& inCSVHeader, 
					 const ASL::String& inFilePath,
					 ASL::String& ioFileName )
 {
	 DVA_ASSERT(!inCSVHeader.empty());

	 ASL::String markerRow;
	 ASL::String assetName;
	 ASL::String assetParentBinPath;
	 ASL::String assetFilePath;

	 MarkerRows rows;
	 rows.push_back(inCSVHeader);

	 BOOST_FOREACH(const ExportMarker::AssetItemMarkersInfoPair& item, inItemMarkersInfo)
	 {
		 assetName = EscapeSpecialChar(item.first->name) + tabDelimiter;
		 assetParentBinPath = EscapeSpecialChar(item.first->parentBinPath) + tabDelimiter;
		 assetFilePath = EscapeSpecialChar(item.first->mediaPath);

		 // have markers in an asset
		 if (item.second.size() != 0)
		 {
			 BOOST_FOREACH(const CottonwoodMarker& marker, item.second)
			 {
				 markerRow = BuildMarkerRow(
					 marker, 
					 item.first->frameRate, 
					 item.first->mediaOffset, 
					 item.first->isDropFrame);
				 markerRow = assetName + assetParentBinPath + markerRow + assetFilePath;
				 rows.push_back(markerRow);
			 }
		 }
		 else
		 {
			 markerRow = assetName + assetParentBinPath + EMPTY_MARKER_STR + assetFilePath;
			 rows.push_back(markerRow);
		 }
	 } 

	 ASL::String validFullPath = GetValidFilePath(inFilePath, ioFileName, CSV_FILE_EXT);
	 return  WriteCSVFile(rows, validFullPath, CSV_FILE_EXT);
 }

 bool SaveMutlipeFile(const PL::ExportMarker::AssetItemMarkersInfoPairVec& inItemMarkersInfo,
	 const ASL::String& inCSVHeader, 
	 const ASL::String& inFilePath,
	 ASL::String& ioFileName)
 {
	 bool saveAllSuccess = true;
	 BOOST_FOREACH(const ExportMarker::AssetItemMarkersInfoPair& pairItem, inItemMarkersInfo)
	 {
		 PL::ExportMarker::AssetItemMarkersInfoPairVec tempVec;
		 tempVec.push_back(pairItem);
		 ioFileName = PL::Utilities::GetFileNamePart(pairItem.first->name);
		 saveAllSuccess &= SaveSingleFile(tempVec, inCSVHeader, inFilePath, ioFileName);
	 }

	 return saveAllSuccess;
 }

 bool SaveAsCSVFile(const PL::ExportMarker::AssetItemMarkersInfoPairVec& inItemMarkersInfoPairVec,
					const ASL::String& inFilePath,
					bool inSaveAsSingleFile,
					ASL::String& ioFileName)
 {
	 const ASL::String& csvHeader = BuildCSVHeader();

	 return inSaveAsSingleFile ? 
			SaveSingleFile(inItemMarkersInfoPairVec, csvHeader, inFilePath, ioFileName) :
			SaveMutlipeFile(inItemMarkersInfoPairVec, csvHeader, inFilePath, ioFileName);
 }

} // namespace PL
