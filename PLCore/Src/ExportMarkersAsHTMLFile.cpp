/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLTimeStamp.h"
#include "ASLClassFactory.h"
#include "ASLCriticalSection.h"
#include "ASLAsyncCallFromMainThread.h"

// Local
#include "PLExportMarkersAsHTMLFile.h"
#include "PLExportMarkersType.h"
#include "PLUtilitiesPrivate.h"

// BE
#include "BE/Marker/IMarker.h"

// dvacore
#include "dvacore/config/Localizer.h"
#include "dvacore/xml/XMLUtils.h"

// MZ
#include "MZUtilities.h"
#include "MZBEUtilities.h"
#include "MZProject.h"

// PL
#include "PLUtilities.h"
#include "PLBEUtilities.h"
#include "PLLibrarySupport.h"
#include "PLMarker.h"

// boost
#include "boost/regex.hpp"

namespace PL
{
namespace
{
	const ASL::StdString titleStartTagRegex("<title[^>]*?>");
	const ASL::StdString tableStartTagRegex("<table[^>]*?>");
	const ASL::StdString tableEndTagRegex("</table>");
	const ASL::StdString tableHeaderCellTagRegex("<th[^>]*?>");
	const ASL::StdString tableThreadCellTagRegex("<td[^>]*?>");
	const ASL::StdString tableRowTagRegex("(<tr[^>]*?>.*?</tr>)(.*)");

	const ASL::StdString assetNameRegex("\\{ASSETNAME\\}");
	const ASL::StdString assetParentBinPathRegex("\\{BIN\\}");
	const ASL::StdString markerThumbnailImageRegex("\\{THUMBNAILIMAGE\\}");
	const ASL::StdString markerNameRegex("\\{MARKERNAME\\}");
	const ASL::StdString markerCommentRegex("\\{DESCRIPTION\\}");
	const ASL::StdString markerInTimeRegex("\\{IN\\}");
	const ASL::StdString markerOutTimeRegex("\\{OUT\\}");
	const ASL::StdString markerDurationRegex("\\{DURATION\\}");
	const ASL::StdString markerTypeRegex("\\{MARKERTYPE\\}");
	const ASL::StdString assetMediaPathRegex("\\{FILEPATH\\}");
	const ASL::StdString markerTagsRegex("\\{TAGS\\}");

	const char* const sCreateFolderError = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/CreatFolderError=Fail to create folder @0.";
	const char* const sCreateFileError   = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/CreatFileError=Fail to create HTML file @0.";
	const char* const sWriteFileError    = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/WriteFileError=Fail to write HTML file @0.";
	const char* const sExportFileError   = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/ExportFileError=Fail to export thumbnail image file @0.";
	const char* const sTemplateMissing   = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/TemplateMissing=The HTML template @0 is missing.";
	const char* const sTemplateDamaged   = "$$$/Prelude/PLCore/ExportMarkersAsHTMLFile/TemplateDamaged=The HTML template @0 is damaged.";

	enum MediaType
	{
		kVideo,
		kAudio,
	};

	/*
	** store thumbnail general information
	** @var	mediaPath	the path of clip that the thumbnail is in
	** @var mediaType	type of clip, which is audio or video
	** @var imagePath	the dest path for exported thumbnail image
	** @var thumbnailPos	the position of thumbnail in current clip
	*/
	struct ThumbnailImageInfo 
	{
		ASL::String mediaPath;
		MediaType mediaType;
		ASL::String imagePath;
		dvamediatypes::TickTime thumbnailPos;
	};

	typedef boost::shared_ptr<ThumbnailImageInfo> ThumbnailImageInfoPtr;
	typedef std::vector<ThumbnailImageInfoPtr> ThumbnailImageInfoVec;

	/*
	** use to manage thumbnail exporting
	** @var	pendingCount	how many thumbnails are waiting to be exported
	** @var criticalSection	control access to pendingCount
	*/
	struct ThumbnailManagementInfo
	{
		ASL::UInt32 pendingCount;
		ASL::CriticalSection criticalSection;
	};

	typedef boost::shared_ptr<ThumbnailManagementInfo> ThumbnailManagementInfoPtr;

	/*
	** use to manage file exporting
	** @var	pendingCount	how many files are waiting to be exported
	** @var criticalSection	control access to pendingCount
	*/
	struct FileManagementInfo 
	{
		ASL::UInt32 pendingCount;
		ASL::CriticalSection criticalSection;
	};

	typedef boost::shared_ptr<FileManagementInfo> FileManagementInfoPtr;

	/*
	** use to store template table 
	** @var	header	table header string of template
	** @var rows	table rows with style
	*/
	class TemplateTableElement
	{
	public:
		void SetHeader(const ASL::StdString inHeader)
		{
			header = inHeader;
		};

		ASL::StdString GetHeader()
		{
			return header;
		};

		void AddRow(const ASL::StdString inRow)
		{
			rows.push_back(inRow);
		};
		
		ASL::StdString GetRow(const ASL::UInt32 inRowsCount)
		{
			return rows.at(inRowsCount%rows.size());
		};

		size_t GetRowsSize()
		{
			return rows.size();
		};

	private:	
		ASL::StdString header;
		std::vector<ASL::StdString> rows;
	};

	typedef boost::shared_ptr<TemplateTableElement> TemplateTableElementPtr;

	/*
	** Convert normal text to HTML text by encoding reserved character.
	*/
	ASL::String EncodeHTMLReservedCharacters(
		const ASL::String& inRowHTMLField)
	{
		ASL::String outProcessedHTMLField;
		dvacore::UTF8String processedHTMLFieldUtf8;
		dvacore::UTF8String tmpUTF8str = dvacore::utility::UTF16to8(inRowHTMLField);
		dvacore::UTF8String::const_iterator iter = tmpUTF8str.begin();

		while (iter != tmpUTF8str.end()) 
		{
			switch (*iter) 
			{
			case '<':
				processedHTMLFieldUtf8.append(dvacore::utility::AsciiToUTF8("&lt;"));
				break;
			case '>':
				processedHTMLFieldUtf8.append(dvacore::utility::AsciiToUTF8("&gt;"));			
				break;
			case '&':
				processedHTMLFieldUtf8.append(dvacore::utility::AsciiToUTF8("&amp;"));			
				break;
			case '\'':
				processedHTMLFieldUtf8.append(dvacore::utility::AsciiToUTF8("&apos;"));
				break;
			case '\"':
				processedHTMLFieldUtf8.append(dvacore::utility::AsciiToUTF8("&quot;"));
				break;
			default:
				processedHTMLFieldUtf8.push_back(*iter);
				break;
			}
			++iter;
		}

		outProcessedHTMLField = dvacore::utility::UTF8to16(processedHTMLFieldUtf8);
		return outProcessedHTMLField;		
	}	// end of EncodeHTMLReservedCharacter


	/*
	**	Return formated string based on current time, which is used for file/directory unique naming.
	**	So if current time return by posix_time is 
	**	2013-08-08T8:52:31,009148
	**	,it will return 
	**	2013-08-08-8-52-31-009148.
	*/
	ASL::String FormatCurrentTime()
	{
		std::string currentTimeStd = boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());

		size_t pos = currentTimeStd.find('T');  
		currentTimeStd.replace(pos, 1, std::string("-"));  
		currentTimeStd.replace(pos + 3, 1, std::string("-"));  
		currentTimeStd.replace(pos + 6, 1, std::string("-"));
		currentTimeStd.replace(pos + 9, 1, std::string("-"));

		ASL::String currentTime = ASL::MakeString(currentTimeStd);
		return currentTime;
	}	// end of FormatCurrentTime


	/*
	**
	*/
	void CheckFileExportProgress(
		FileManagementInfoPtr inFileManagementInfoPtr,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr)
	{
		inFileManagementInfoPtr->criticalSection.Lock();
	
		if (inFileManagementInfoPtr->pendingCount <= 0 ||
			--inFileManagementInfoPtr->pendingCount <= 0)
		{
			ASL::AsyncCallFromMainThread(boost::bind(
				inExportMarkersResultPtr->finishCallbackFn, 
				inExportMarkersResultPtr->allSaveSucceed, 
				inExportMarkersResultPtr->allItemsIsValid, 
				inExportMarkersResultPtr->exportFilePath));
		}

		inFileManagementInfoPtr->criticalSection.Unlock();
	}	// end of CheckFileExportProgress


	/*
	**
	*/
	void CheckThumbnailExportProgress(
		FileManagementInfoPtr inFileManagementInfoPtr,
		ThumbnailManagementInfoPtr inThumbnailManagementInfoPtr,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr)
	{
		inThumbnailManagementInfoPtr->criticalSection.Lock();

		if (inThumbnailManagementInfoPtr->pendingCount <= 0 ||
			--inThumbnailManagementInfoPtr->pendingCount <= 0)
		{
			CheckFileExportProgress(inFileManagementInfoPtr, inExportMarkersResultPtr);
		}

		inThumbnailManagementInfoPtr->criticalSection.Unlock();
	}	// end of CheckThumbnailExportProgress


	/*
	** add error info to events panel in main thread
	*/
	void ExportThumbnailImageFail(ASL::String imagePath)
	{
		ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sExportFileError, imagePath));
	}


	/*
	**
	*/
	void ExportThumbnailImageCallback(
		const ASL::String& inImageFullPath,
		MediaType inMediaType,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr,
		ThumbnailManagementInfoPtr inThumbnailManagementInfoPtr,
		FileManagementInfoPtr inFileManagementInfoPtr,
		const ASL::String& inThumbnailFileName,
		ASL::Result inResult)
	{

		if (ASL::PathUtils::ExistsOnDisk(inImageFullPath))
		{
			ASL::File::Delete(inImageFullPath);
		}

		ASL::Result result = ASL::kSuccess;
			
		/*
		**  Thumbnail image of audio is not dynamically generated and probably used by other components,
		**  so don't use Move to change its position and use Copy instead.
		*/
		switch (inMediaType)
		{
		case kVideo:
			result = ASL::File::Move(
				inThumbnailFileName, inImageFullPath);
			break;
		case kAudio:
			result = ASL::File::Copy(
				inThumbnailFileName, inImageFullPath);
			break;
		default:
			result = ASL::ResultFlags::kResultTypeFailure;
			break;
		}

		if (!ASL::ResultSucceeded(result))
		{
			inExportMarkersResultPtr->allSaveSucceed = false;
			ASL::AsyncCallFromMainThread(boost::bind(
				ExportThumbnailImageFail, 
				inImageFullPath));
		}

		CheckThumbnailExportProgress(inFileManagementInfoPtr, inThumbnailManagementInfoPtr, inExportMarkersResultPtr);
	}	// end of ExportThumbnailImageCallback


	/*
	**
	*/
	void ExportThumbnailImages(
		const ThumbnailImageInfoVec& inThumbnailImagesInfoVec,
		ThumbnailManagementInfoPtr inThumbnailManagementInfoPtr,
		FileManagementInfoPtr inFileManagementInfoPtr,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr)
	{

		if (inThumbnailImagesInfoVec.size() == 0)
		{
			CheckThumbnailExportProgress(inFileManagementInfoPtr, inThumbnailManagementInfoPtr, inExportMarkersResultPtr);
			return;
		}

		BOOST_FOREACH(ThumbnailImageInfoPtr thumbnailImageInfoItem, inThumbnailImagesInfoVec)
		{
			if (ASL::PathUtils::ExistsOnDisk(thumbnailImageInfoItem->mediaPath))
			{
				SRLibrarySupport::RequestThumbnailAsync(
					thumbnailImageInfoItem->mediaPath,
					thumbnailImageInfoItem->thumbnailPos,
					boost::bind<void>(
						ExportThumbnailImageCallback, 
						thumbnailImageInfoItem->imagePath, 
						thumbnailImageInfoItem->mediaType, 
						inExportMarkersResultPtr,
						inThumbnailManagementInfoPtr,
						inFileManagementInfoPtr,
						_1, _2));
			} 
			else
			{
				inExportMarkersResultPtr->allSaveSucceed = false;
				ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sExportFileError, thumbnailImageInfoItem->imagePath));
				CheckThumbnailExportProgress(inFileManagementInfoPtr, inThumbnailManagementInfoPtr, inExportMarkersResultPtr);
			}
		}

	}	// end of ExportAllThumbnailImages


	/*
	**
	*/
	ASL::String CreateThumbnailImageName(
		const ASL::String& inAssetName,
		const ASL::String& inMarkerName,
		const ASL::String& inAssetMediaPath,
		const dvamediatypes::TickTime inThumbnailPos,
		const ASL::String& inImageDirFullPath,
		const ASL::UInt32 inRowsCount,
		ThumbnailImageInfoVec& outThumbnailImagesInfoVec)
	{
		ASL::String currentTime = FormatCurrentTime();	
		ASL::String extension = ASL_STR(".jpeg");
		MediaType mediaType = kVideo;

		BE::IProjectItemRef projectItem = MZ::BEUtilities::GetProjectItem(MZ::GetProject(), inAssetMediaPath);
		if (MZ::BEUtilities::IsAudioOnlyProjectItem(projectItem))
		{
			extension = ASL_STR(".png");
			mediaType = kAudio;
		}

		ASL::String imageFilename = inAssetName 
							+ ASL_STR("_") 
							+ inMarkerName 
							+ ASL_STR("_")  
							+ currentTime
							+ ASL::Coercion<ASL::String>::Result(inRowsCount)
							+ extension;

		ASL::String imageFullPath = ASL::PathUtils::AddTrailingSlash(inImageDirFullPath);	
		imageFullPath += imageFilename;
	
		ThumbnailImageInfoPtr thumbnailImageInfoPtr(new ThumbnailImageInfo());
		thumbnailImageInfoPtr->imagePath = imageFullPath;
		thumbnailImageInfoPtr->mediaPath = inAssetMediaPath;
		thumbnailImageInfoPtr->mediaType = mediaType;
		thumbnailImageInfoPtr->thumbnailPos = inThumbnailPos;
		outThumbnailImagesInfoVec.push_back(thumbnailImageInfoPtr);

		return ASL_STR("./images/") + imageFilename;
	}	// end of CreateThumbnailImageName

	/*
	**
	*/	
	ASL::Result ConvertMarkerInfoToHTML(
		ASL::String& outMarkerRow,
		const CottonwoodMarker& inMarkerItem,
		const ASL::String& inAssetName,
		const ASL::String& inAssetParentBinPath,
		const ASL::String& inAssetMediaPath,
		const ASL::String& inImageDirFullPath,
		const dvamediatypes::FrameRate& inAssetFrameRate,
		const dvamediatypes::TickTime inAssetMediaOffset,
		const ASL::UInt32 inRowsCount,
		ThumbnailImageInfoVec& outThumbnailImagesInfoVec,
		bool inIsDropFrame,
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		ASL::Result result = ASL::kSuccess;
		ASL::StdString outMarkerRowStd = inTemplateTableElementPtr->GetRow(inRowsCount-1);
		ASL::String markerName = inMarkerItem.GetName();

		ASL::String assetName = inAssetName;
		assetName = EncodeHTMLReservedCharacters(assetName);

		ASL::String assetParentBinPath = inAssetParentBinPath;
		assetParentBinPath = EncodeHTMLReservedCharacters(assetParentBinPath);

		ASL::String markerThumbnailImage = CreateThumbnailImageName(
			inAssetName,
			markerName,
			inAssetMediaPath, 
			inMarkerItem.GetStartTime(),
			inImageDirFullPath,
			inRowsCount,
			outThumbnailImagesInfoVec);

		markerName = EncodeHTMLReservedCharacters(markerName);

		ASL::String markerComment = inMarkerItem.GetComment();
		markerComment = EncodeHTMLReservedCharacters(markerComment);

		const dvamediatypes::TickTime inTimeCode = inMarkerItem.GetStartTime() + inAssetMediaOffset;
		ASL::String markerInTime = PL::Utilities::ConvertTimecodeToString(inTimeCode, inAssetFrameRate, inIsDropFrame);
		markerInTime = EncodeHTMLReservedCharacters(markerInTime);

		const dvamediatypes::TickTime outTimeCode = inMarkerItem.GetStartTime() + inAssetMediaOffset + inMarkerItem.GetDuration();
		ASL::String markerOutTime = PL::Utilities::ConvertTimecodeToString(outTimeCode, inAssetFrameRate, inIsDropFrame);
		markerOutTime = EncodeHTMLReservedCharacters(markerOutTime);

		const dvamediatypes::TickTime duration = inMarkerItem.GetDuration();
		ASL::String markerDuration = PL::Utilities::ConvertTimecodeToString(duration, inAssetFrameRate, inIsDropFrame);
		markerDuration = EncodeHTMLReservedCharacters(markerDuration);;

		ASL::String markerType = inMarkerItem.GetType();	
		markerType = (markerType == ASL_STR("InOut")) ?
			ASL_STR("Subclip") :
			markerType;
		markerType = EncodeHTMLReservedCharacters(markerType);

		ASL::String assetMediaPath = inAssetMediaPath;
		assetMediaPath = EncodeHTMLReservedCharacters(assetMediaPath);

		TagParamMap markerTagParamMap = inMarkerItem.GetTagParams();
		ASL::String markerTags = PL::SRUtilitiesPrivate::ConvertMarkerTagsToString(markerTagParamMap);

		try
		{
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(assetNameRegex), ASL::MakeStdString(assetName), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(assetParentBinPathRegex), ASL::MakeStdString(assetParentBinPath), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerThumbnailImageRegex), ASL::MakeStdString(markerThumbnailImage), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerNameRegex), ASL::MakeStdString(markerName), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerCommentRegex), ASL::MakeStdString(markerComment), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerInTimeRegex), ASL::MakeStdString(markerInTime), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerOutTimeRegex), ASL::MakeStdString(markerOutTime), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerDurationRegex), ASL::MakeStdString(markerDuration), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerTypeRegex), ASL::MakeStdString(markerType), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(assetMediaPathRegex), ASL::MakeStdString(assetMediaPath), boost::regex_constants::format_literal);
			outMarkerRowStd = boost::regex_replace(outMarkerRowStd, 
				boost::regex(markerTagsRegex), ASL::MakeStdString(markerTags), boost::regex_constants::format_literal);
		}
		catch (boost::bad_expression bad) 
		{
			boost::regex_constants::error_type err = bad.code();
			std::ptrdiff_t diff = bad.position();
		}

		outMarkerRow = ASL::MakeString(outMarkerRowStd);
		return result;
	}	// end of ConvertMarkerInfoToHTML


	/*
	**
	*/
	ASL::String ConvertTableHeaderToHTML(
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		ASL::String headerRow;
		ASL::StdString headerRowStd = inTemplateTableElementPtr->GetHeader();

		ASL::String assetName(dvacore::ZString(PL::ExportMarker::sAssetName));
		ASL::String assetParentBinPath(dvacore::ZString(PL::ExportMarker::sAssetParentBinPath));
		ASL::String markerThumbnailImage(dvacore::ZString(PL::ExportMarker::sMarkerThumbnailImage));
		ASL::String markerName(dvacore::ZString(PL::ExportMarker::sMarkerName));
		ASL::String markerComment(dvacore::ZString(PL::ExportMarker::sMarkerComment));
		ASL::String markerInTime(dvacore::ZString(PL::ExportMarker::sMarkerInTime));
		ASL::String markerOutTime(dvacore::ZString(PL::ExportMarker::sMarkerOutTime));
		ASL::String markerDuration(dvacore::ZString(PL::ExportMarker::sMarkerDuration));
		ASL::String markerType(dvacore::ZString(PL::ExportMarker::sMarkerType));
		ASL::String assetMediaPath(dvacore::ZString(PL::ExportMarker::sAssetMediaPath));
		ASL::String markerTags(dvacore::ZString(PL::ExportMarker::sMarkerTags));

		assetName = EncodeHTMLReservedCharacters(assetName);
		assetParentBinPath = EncodeHTMLReservedCharacters(assetParentBinPath);
		markerThumbnailImage = EncodeHTMLReservedCharacters(markerThumbnailImage);
		markerName = EncodeHTMLReservedCharacters(markerName);
		markerComment = EncodeHTMLReservedCharacters(markerComment);
		markerInTime = EncodeHTMLReservedCharacters(markerInTime);
		markerOutTime = EncodeHTMLReservedCharacters(markerOutTime);
		markerDuration = EncodeHTMLReservedCharacters(markerDuration);
		markerType = EncodeHTMLReservedCharacters(markerType);
		assetMediaPath = EncodeHTMLReservedCharacters(assetMediaPath);
		markerTags = EncodeHTMLReservedCharacters(markerTags);

		try 
		{
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(assetNameRegex), ASL::MakeStdString(assetName), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(assetParentBinPathRegex), ASL::MakeStdString(assetParentBinPath), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerThumbnailImageRegex), ASL::MakeStdString(markerThumbnailImage), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerNameRegex), ASL::MakeStdString(markerName), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd,
				boost::regex(markerCommentRegex), ASL::MakeStdString(markerComment), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerInTimeRegex), ASL::MakeStdString(markerInTime), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerOutTimeRegex), ASL::MakeStdString(markerOutTime), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerDurationRegex), ASL::MakeStdString(markerDuration), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerTypeRegex), ASL::MakeStdString(markerType), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(assetMediaPathRegex), ASL::MakeStdString(assetMediaPath), boost::regex_constants::format_literal);
			headerRowStd = boost::regex_replace(headerRowStd, 
				boost::regex(markerTagsRegex), ASL::MakeStdString(markerTags), boost::regex_constants::format_literal);
		}
		catch (boost::bad_expression bad) 
		{
			boost::regex_constants::error_type err = bad.code();
			std::ptrdiff_t diff = bad.position();
		}

		headerRow = ASL::MakeString(headerRowStd);
		return headerRow;
	}	// end of ConvertTableHeaderToHTML


	/*
	**
	*/
	ASL::Result ConvertAssetInfoToHTML(
		const PL::ExportMarker::AssetItemMarkersInfoPair& inAssetItem,
		std::vector<ASL::String>& outMarkerListOfHTML,
		const ASL::String& inImageDirFullPath,
		ThumbnailImageInfoVec& outThumbnailImagesInfoVec,
		ASL::UInt32& ioRowsCount,
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		ASL::String markerRow; 
		ASL::Result result = ASL::kSuccess;

		dvamediatypes::FrameRate& assetFrameRate = inAssetItem.first->frameRate;
		bool isDropFrame = inAssetItem.first->isDropFrame;
		ASL::String assetName = inAssetItem.first->name;
		ASL::String assetParentBinPath = inAssetItem.first->parentBinPath;
		ASL::String assetMediaPath = inAssetItem.first->mediaPath;
		dvamediatypes::TickTime assetMediaOffset = inAssetItem.first->mediaOffset;

		BOOST_FOREACH(CottonwoodMarker markerItem, inAssetItem.second)
		{
			ioRowsCount++;
			markerRow = ASL_STR("");
			result = ConvertMarkerInfoToHTML(
								markerRow, 
								markerItem,
								assetName, 
								assetParentBinPath,
								assetMediaPath, 
								inImageDirFullPath,
								assetFrameRate,
								assetMediaOffset,
								ioRowsCount,
								outThumbnailImagesInfoVec,
								isDropFrame,
								inTemplateTableElementPtr
							);

			if (!ASL::ResultSucceeded(result))
				return result;

			outMarkerListOfHTML.push_back(markerRow);
		}

		return result;
	}	// end of ConvertAssetInfoToHTML
		

	ASL::Result ConvertBlankAssetInfoToHTML(
		const PL::ExportMarker::AssetItemMarkersInfoPair& inAssetItem,
		std::vector<ASL::String>& outMarkerListOfHTML,
		ASL::UInt32& ioRowsCount,
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		ASL::String assetRow = ASL_STR(""); 
		ASL::Result result = ASL::kSuccess;
		
		ioRowsCount++;

		ASL::StdString assetRowStd = inTemplateTableElementPtr->GetRow(ioRowsCount-1);
		ASL::String assetName = inAssetItem.first->name;
		ASL::String assetParentBinPath = inAssetItem.first->parentBinPath;

		assetName = EncodeHTMLReservedCharacters(assetName);
		assetParentBinPath = EncodeHTMLReservedCharacters(assetParentBinPath);

		try
		{
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(assetNameRegex), ASL::MakeStdString(assetName), boost::regex_constants::format_literal);
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(assetParentBinPathRegex), ASL::MakeStdString(assetParentBinPath), boost::regex_constants::format_literal);
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerThumbnailImageRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerNameRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerCommentRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerInTimeRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerOutTimeRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerDurationRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerTypeRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(assetMediaPathRegex), "");
			assetRowStd = boost::regex_replace(assetRowStd, boost::regex(markerTypeRegex), "");
		}
		catch (boost::bad_expression bad) 
		{
			boost::regex_constants::error_type err = bad.code();
			std::ptrdiff_t diff = bad.position();
		}

		assetRow = ASL::MakeString(assetRowStd);
		outMarkerListOfHTML.push_back(assetRow);

		return result;
	}	// end of ConvertBlankAssetInfoToHTML


	/*
	**
	*/
	ASL::Result  WriteHTMLFile(
		const std::vector<ASL::String>& markerListOfHTML,
		const ASL::String& inFileFullPath,
		const ASL::String& inTemplateFilePath,
		const ASL::String& inProjectName)
	{
		ASL::Result result;
		ASL::String linebreak = ASL_STR("\n\r");
		bool tableStartTagNotFound = true;
		bool tableEndTagNotFound = true;
		bool titleStartTagNotFound = true;

		// Read HTML template into a file stream
		std::ifstream templateFileStream(
			reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inTemplateFilePath).c_str()), 
			std::ios::in);
		
		// create HTML file
		ASL::File htmlFile;
		result = htmlFile.Create(
			inFileFullPath,
			ASL::FileAccessFlags::kWrite,
			ASL::FileShareModeFlags::kNone,
			ASL::FileCreateDispositionFlags::kCreateAlways,
			ASL::FileAttributesFlags::kAttributeNormal);
		if (!ASL::ResultSucceeded(result))
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sCreateFileError, inFileFullPath));
			return result;
		}

		// write HTML file
		ASL::UInt32 numberOfBytesWriten = 0;
		dvacore::UTF8String makerHTMLUtf8;
		ASL::String templateHTML;
		dvacore::UTF8String templateHTMLUtf8;
		ASL::StdString templateHTMLStd;
		dvacore::UTF8String inProjectNameUtf8;

		while (std::getline(templateFileStream, templateHTMLStd))
		{
			// write Rows of template into HTML file 
			if (tableStartTagNotFound || !tableEndTagNotFound) {
				templateHTML = ASL::MakeString(templateHTMLStd);
				templateHTMLUtf8 = dvacore::utility::UTF16to8(templateHTML + linebreak);
				result = htmlFile.Write(templateHTMLUtf8.data(), 
					static_cast<ASL::UInt32>(templateHTMLUtf8.size() * sizeof(dvacore::UTF8String::value_type)),
					numberOfBytesWriten);

				if (!ASL::ResultSucceeded(result))
				{
					ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sWriteFileError, inFileFullPath));
					return result;
				}
			}

			// write marker info 
			if (tableStartTagNotFound 
					&& boost::regex_search(templateHTMLStd, boost::regex(tableStartTagRegex, boost::regex::icase)))
			{
				tableStartTagNotFound = false;	
				BOOST_FOREACH(ASL::String markerHTML, markerListOfHTML)
				{
					makerHTMLUtf8 = dvacore::utility::UTF16to8(markerHTML + linebreak);
					result = htmlFile.Write(makerHTMLUtf8.data(), 
							static_cast<ASL::UInt32>(makerHTMLUtf8.size() * sizeof(dvacore::UTF8String::value_type)),
							numberOfBytesWriten);
					if (!ASL::ResultSucceeded(result))
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sWriteFileError, inFileFullPath));
						return result;
					}
				}
			}

			// write project name
			if (titleStartTagNotFound
					&& boost::regex_search(templateHTMLStd, boost::regex(titleStartTagRegex, boost::regex::icase)))
			{
				titleStartTagNotFound = false;	
				inProjectNameUtf8 = dvacore::utility::UTF16to8(inProjectName);
				result = htmlFile.Write(inProjectNameUtf8.data(), 
						static_cast<ASL::UInt32>(inProjectNameUtf8.size() * sizeof(dvacore::UTF8String::value_type)),
						numberOfBytesWriten);
				if (!ASL::ResultSucceeded(result))
				{
					ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sWriteFileError, inFileFullPath));
					return result;
				}
			}

			if (tableEndTagNotFound  
					&& boost::regex_search(templateHTMLStd, boost::regex(tableEndTagRegex, boost::regex::icase)))
			{
				tableEndTagNotFound = false;
				templateHTML = ASL::MakeString(templateHTMLStd);
				templateHTMLUtf8 = dvacore::utility::UTF16to8(templateHTML + linebreak);
				result = htmlFile.Write(templateHTMLUtf8.data(), 
					static_cast<ASL::UInt32>(templateHTMLUtf8.size() * sizeof(dvacore::UTF8String::value_type)),
					numberOfBytesWriten);

				if (!ASL::ResultSucceeded(result))
				{
					ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sWriteFileError, inFileFullPath));
					return result;
				}
			}
		}

		if (tableStartTagNotFound)
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sTemplateDamaged, inTemplateFilePath));
			return ASL::ResultFlags::kResultTypeFailure;
		}

		return result;
	}	// end of WriteHTMLFile


	/*
	**
	*/
	ASL::Result CreateFolder(
		ASL::String& outDirFullPath,
		ASL::String& outFileName,
		const ASL::String& inFilePath,
		const ASL::String& inFileName,
		const ASL::String& inProjectName)
	{
		ASL::Result result;
		ASL::UInt32 number = 1;
		ASL::String fileNum = ASL_STR("");

		outFileName = inFileName.empty() ?
					inProjectName :
					inFileName;
		do
		{
			outDirFullPath = ASL::PathUtils::AddTrailingSlash(inFilePath);	
			outDirFullPath += outFileName + fileNum;
 
			fileNum = ASL_STR("_") + ASL::Coercion<ASL::String>::Result(number);
			number++;
		} 
		while (ASL::PathUtils::ExistsOnDisk(outDirFullPath));	

		result = ASL::Directory::CreateOnDisk(outDirFullPath);
		if (!ASL::ResultSucceeded(result))
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sCreateFolderError, outDirFullPath));
			return result;
		}
		return result;
	}	// end of CreateFolder

	
	/*
	**
	*/
	ASL::Result CreateImageFolder(
		ASL::String& outImageDirFullPath,
		const ASL::String& inDirFullPath)
	{
		ASL::Result result;

		outImageDirFullPath = ASL::PathUtils::CombinePaths(inDirFullPath, ASL_STR("images"));
		result = ASL::Directory::CreateOnDisk(outImageDirFullPath);
		if (!ASL::ResultSucceeded(result))
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sCreateFolderError, outImageDirFullPath));
			return result;
		}

		return result;
	}	// end of CreateImageFolder


	ASL::Result ParseTemplate(
		const ASL::String& inTemplateFilePath,
		TemplateTableElementPtr outTemplateTableElementPtr)
	{
		ASL::Result result = ASL::kSuccess;
		bool tableStartTagFound = false;

		// Read HTML template into a file stream
		std::ifstream templateFileStream(
			reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inTemplateFilePath).c_str()), 
			std::ios::in);

		ASL::StdString templateHTMLLineStd;
		ASL::StdString templateHTMLTableStd;

		try
		{
			while (std::getline(templateFileStream, templateHTMLLineStd))
			{
				if (boost::regex_search(templateHTMLLineStd, boost::regex(tableStartTagRegex, boost::regex::icase)))
				{
					tableStartTagFound = true;
				}
			
				if (tableStartTagFound)
				{
					templateHTMLTableStd += templateHTMLLineStd;
				}

				if (boost::regex_search(templateHTMLLineStd, boost::regex(tableEndTagRegex, boost::regex::icase)))
				{
					break;
				}
			}

			ASL::StdString tableRow;
			boost::smatch matchResult;

			while (boost::regex_search(templateHTMLTableStd, matchResult, boost::regex(tableRowTagRegex, boost::regex::icase)))
			{
				tableRow = ASL::StdString(matchResult[1].first, matchResult[1].second);

				if (boost::regex_search(tableRow, boost::regex(tableHeaderCellTagRegex, boost::regex::icase)))
				{
					outTemplateTableElementPtr->SetHeader(tableRow);
				}	
				else if(boost::regex_search(tableRow, boost::regex(tableThreadCellTagRegex, boost::regex::icase)))
				{
					outTemplateTableElementPtr->AddRow(tableRow);
				}

				templateHTMLTableStd = ASL::StdString(matchResult[2].first, matchResult[2].second);
			}
		}
		catch (boost::bad_expression bad) 
		{
			boost::regex_constants::error_type err = bad.code();
			std::ptrdiff_t diff = bad.position();
		}

		if (outTemplateTableElementPtr->GetHeader() == "" || outTemplateTableElementPtr->GetRowsSize() == 0)
		{
			ML::SDKErrors::SetSDKErrorString(dvacore::ZString(sTemplateDamaged, inTemplateFilePath));
			return ASL::ResultFlags::kResultTypeFailure;
		}

		return result;
	}


	/*
	**
	*/
	ASL::Result GetTemplateFilePath(
		ASL::String& outTemplateFilePath)
	{	
		ASL::String theTemplateFileName = ASL_STR("default.tpl");

		ASL::String specialDirectoryID;

#if ASL_DEBUG
		specialDirectoryID = ASL::MakeString(ASL::kApplicationDirectory);
		outTemplateFilePath = ASL::PathUtils::ToNormalizedPath(ASL::DirectoryRegistry::FindDirectory(specialDirectoryID));

		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR(".."));
		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR(".."));
		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR(".."));
		if (ASL_TARGET_OS_MAC)
		{
			outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR(".."));
		}
		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR("Content"));
		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR("Settings"));
		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR("HTMLTemplate"));
	
		outTemplateFilePath = ASL::PathUtils::AddTrailingSlash(outTemplateFilePath);
		outTemplateFilePath += theTemplateFileName;
		if (ASL::PathUtils::ExistsOnDisk(outTemplateFilePath))
		{
			return ASL::kSuccess;
		}
		
		return ASL::ResultFlags::kResultTypeFailure;
#endif

		specialDirectoryID = ASL::MakeString(ASL::kNonLocalizedSettingsDirectory);
		outTemplateFilePath = ASL::PathUtils::ToNormalizedPath(ASL::DirectoryRegistry::FindDirectory(specialDirectoryID));

		outTemplateFilePath = ASL::PathUtils::CombinePaths(outTemplateFilePath, ASL_STR("HTMLTemplate"));
		outTemplateFilePath = ASL::PathUtils::AddTrailingSlash(outTemplateFilePath);	
		outTemplateFilePath += theTemplateFileName;

		if (ASL::PathUtils::ExistsOnDisk(outTemplateFilePath))
		{
			return ASL::kSuccess;
		}

		return ASL::ResultFlags::kResultTypeFailure;
	}


	/*
	**
	*/
	ASL::Result SaveAsSingleHTMLFile(
		const PL::ExportMarker::AssetItemMarkersInfoPairVec& inAssetItemMarkersInfoPairVec,
		const ASL::String& inDirFullPath,
		const ASL::String& inFileName,
		const ASL::String& inTemplateFilePath,
		const ASL::String& inProjectName,
		FileManagementInfoPtr inFileManagementInfoPtr,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr,
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		ASL::Result result = ASL::kSuccess;
		std::vector<ASL::String> markerListOfHTML;
		ASL::String fileFullPath = ASL::PathUtils::AddTrailingSlash(inDirFullPath);	
		fileFullPath +=  inFileName + ASL_STR(".html");
		ASL::UInt32 RowsCount = 0;
		ThumbnailImageInfoVec thumbnailImagesInfoVec;
		ThumbnailManagementInfoPtr thumbnailManagementInfoPtr(new ThumbnailManagementInfo());
		thumbnailManagementInfoPtr->pendingCount = 0;

		ASL::String imageDirFullPath;
		result = CreateImageFolder(imageDirFullPath, inDirFullPath);
		if (!ASL::ResultSucceeded(result))
			return result;
		
		const ASL::String tableHeader = ConvertTableHeaderToHTML(inTemplateTableElementPtr);
		markerListOfHTML.push_back(tableHeader);

		BOOST_FOREACH(PL::ExportMarker::AssetItemMarkersInfoPair assetItem, inAssetItemMarkersInfoPairVec)
		{	
			if (assetItem.second.size() == 0) 
			{
				result = ConvertBlankAssetInfoToHTML(assetItem, markerListOfHTML, RowsCount, inTemplateTableElementPtr);
			}
			else
			{
				result = ConvertAssetInfoToHTML(assetItem, markerListOfHTML, imageDirFullPath, thumbnailImagesInfoVec, RowsCount, inTemplateTableElementPtr);
			}

			if (!ASL::ResultSucceeded(result))
				return result;
		}

		result = WriteHTMLFile(markerListOfHTML, fileFullPath, inTemplateFilePath, inProjectName);

		if (!ASL::ResultSucceeded(result))
			return result;

		thumbnailManagementInfoPtr->pendingCount = static_cast<ASL::UInt32>(thumbnailImagesInfoVec.size());
		ExportThumbnailImages(thumbnailImagesInfoVec, thumbnailManagementInfoPtr, inFileManagementInfoPtr, inExportMarkersResultPtr);

		return result;
	}	// end of SaveAsSingleHTMLFile


	/*
	**
	*/
	void SaveAsMultiHTMLFiles(
		const PL::ExportMarker::AssetItemMarkersInfoPairVec& inAssetItemMarkersInfoPairVec,
		const ASL::String& inDirFullPath,
		const ASL::String& inFileName,
		const ASL::String& inTemplateFilePath,
		const ASL::String& inProjectName,
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr,
		TemplateTableElementPtr inTemplateTableElementPtr)
	{
		PL::ExportMarker::AssetItemMarkersInfoPairVec childAssetItemMarkersInfoPairVec; 
		ASL::String assetName;
		ASL::String childDirFullPath;
		ASL::String childFileName;
		ASL::Result result = ASL::kSuccess;
		FileManagementInfoPtr fileManagementInfoPtr(new FileManagementInfo());
		fileManagementInfoPtr->pendingCount = static_cast<ASL::UInt32>(inAssetItemMarkersInfoPairVec.size());

		if (fileManagementInfoPtr->pendingCount == 0)
		{
			CheckFileExportProgress(fileManagementInfoPtr, inExportMarkersResultPtr);
		}

		BOOST_FOREACH(PL::ExportMarker::AssetItemMarkersInfoPair assetItem, inAssetItemMarkersInfoPairVec)
		{			
			childAssetItemMarkersInfoPairVec.clear();
			childAssetItemMarkersInfoPairVec.push_back(assetItem);

			assetName = assetItem.first->name;
			result = CreateFolder(childDirFullPath, childFileName, inDirFullPath, assetName, inProjectName);
			if (!ASL::ResultSucceeded(result))
			{
				inExportMarkersResultPtr->allSaveSucceed = false;
				CheckFileExportProgress(fileManagementInfoPtr, inExportMarkersResultPtr);
				continue;
			}
			
			result = SaveAsSingleHTMLFile(
				childAssetItemMarkersInfoPairVec, 
				childDirFullPath, 
				childFileName, 
				inTemplateFilePath,
				inProjectName,
				fileManagementInfoPtr,
				inExportMarkersResultPtr,
				inTemplateTableElementPtr);

			if (!ASL::ResultSucceeded(result))
			{
				inExportMarkersResultPtr->allSaveSucceed = false;
				CheckFileExportProgress(fileManagementInfoPtr, inExportMarkersResultPtr);
			}
		}
	}	// end of SaveAsMultiHTMLFiles

}

	/*
	**
	*/
	void SaveAsHTMLFile(
		PL::ExportMarker::ExportMarkersResultPtr inExportMarkersResultPtr,
		const PL::ExportMarker::AssetItemMarkersInfoPairVec& inAssetItemMarkersInfoPairVec, 
		const ASL::String& inFilePath,
		const ASL::String& inFileName,
		const ASL::String& inProjectName,
		bool inSaveAsSingleFile)
	{
		ASL::String dirFullPath;
		ASL::String fileName;
		ASL::String templateFilePath;
		ASL::String imageDirFullPath;
		ASL::Result result;
		TemplateTableElementPtr templateTableElementPtr(new TemplateTableElement());

		result = GetTemplateFilePath(templateFilePath);

		if (!ASL::ResultSucceeded(result))
		{	
			ASL::AsyncCallFromMainThread(boost::bind(
				inExportMarkersResultPtr->finishCallbackFn, 
				false,
				false,
				ASL_STR("")));
			return;
		}

		result = ParseTemplate(templateFilePath, templateTableElementPtr);

		if (!ASL::ResultSucceeded(result))
		{
			ASL::AsyncCallFromMainThread(boost::bind(
				inExportMarkersResultPtr->finishCallbackFn, 
				false,
				false,
				ASL_STR("")));
			return;
		}

		result = CreateFolder(dirFullPath, fileName, inFilePath, inFileName, inProjectName);
		if (!ASL::ResultSucceeded(result))
		{
			ASL::AsyncCallFromMainThread(boost::bind(
				inExportMarkersResultPtr->finishCallbackFn, 
				false,
				false,
				ASL_STR("")));
			return;
		}

		inExportMarkersResultPtr->exportFilePath = dirFullPath;

		if (inSaveAsSingleFile)
		{
			FileManagementInfoPtr fileManagementInfoPtr(new FileManagementInfo());
			fileManagementInfoPtr->pendingCount = 1;

			result = SaveAsSingleHTMLFile(
							inAssetItemMarkersInfoPairVec, 
							dirFullPath, 
							fileName, 
							templateFilePath,
							inProjectName,
							fileManagementInfoPtr,
							inExportMarkersResultPtr,
							templateTableElementPtr
						);

			if (!ASL::ResultSucceeded(result))
			{
				inExportMarkersResultPtr->allSaveSucceed = false;
				CheckFileExportProgress(fileManagementInfoPtr, inExportMarkersResultPtr);
			}
		}
		else
		{
			SaveAsMultiHTMLFiles(
							inAssetItemMarkersInfoPairVec,
							dirFullPath,
							fileName,
							templateFilePath,
							inProjectName,
							inExportMarkersResultPtr,
							templateTableElementPtr
						);
		}	

	}	// end of SaveAsHTML

}	// end of PL
