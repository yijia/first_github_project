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

#ifndef PLUTILITIES_h
#define PLUTILITIES_h


#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif

#ifndef IPLROUGHCUT_H
#include "IPLRoughCut.h"
#endif

#ifndef PLCLIPDEFINITION_H
#include "PLClipDefinition.h"
#endif

#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

#ifndef IPLMARKEROWNER_H
#include "IPLMarkerOwner.h"
#endif

#ifndef PLUNASSOCIATEDMETADATA_H
#include "PLUnassociatedMetadata.h"
#endif

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef IPLASSETLIBRARYNOTIFIER_H
#include "IPLAssetLibraryNotifier.h"
#endif

#ifndef PLEXPORTCLIPITEM_H
#include "PLExportClipItem.h"
#endif

// UIF
#ifndef UIFMESSAGEBOX_H
#include "UIFMessageBox.h"
#endif

#ifndef MLIMPORTABLEFILETYPEUTILS_H
#include "MLImportableFileTypeUtils.h"
#endif

#ifndef MZSEQUENCE_H
#include "MZSequence.h"
#endif

#ifndef MZCUEPOINTEDITOR_H
#include "MZCuePointEditor.h"
#endif

#ifndef MZACTION_H
#include "MZAction.h"
#endif

#ifndef MZCONTENTANALYSIS_H
#include "MZContentAnalysis.h"
#endif

#ifndef DVAUI_DRAWBOT_DRAWBOTTYPES_H
#include "dvaui/drawbot/DrawbotTypes.h"
#endif

#ifndef PLINGESTUTILITIES_H
#include "IngestMedia/PLIngestUtils.h"
#endif

struct XMP_DateTime;

namespace PL
{
	// Steps to extend the RoughCutVisitor to handle new things that can go wrong:
	// 1. Define a new error code for the thing that can go wrong
	// 2. Determine the correct typeto inspect to look for the new problem, add a Visit() overload for that type
	// 3. Add a test for that type in VisitObject
	ASL_DEFINE_RESULT_SPACE(kRoughCutInspectionResults, 130);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eTooManyNonEmptyVideoTracks, 1);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsTransition, 2);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsNestedSequence, 3);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eIsMergedClip, 4);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsTitle, 5);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsSynthetic, 6);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsAdjustmentLayer, 7);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsMisalignedTrackItems, 8);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsMixedTrackItems, 9);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsGapsBetweenSubClips, 10);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eSubClipTrackItemsNotCompleteness, 11);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsCrossSubClipLinkOrGroup, 12);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsLockedTrack, 13);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eUnsafeSpeedSetting, 14);
	ASL_DEFINE_ERROR(kRoughCutInspectionResults, eUnsafeSequenceStructure, 15);
    ASL_DEFINE_ERROR(kRoughCutInspectionResults, eContainsMoreThanOneNonEmptyVideoTrack, 16);
    ASL_DEFINE_ERROR(kRoughCutInspectionResults, eAudioTrackClipItemIsNotAligned, 17);

	//	Module enum
	enum SREditingModule
	{
		SRUnknownModule,
		SRLoggingModule,
		SRRoughCutModule,

		// Following enum haven't been implemented
		SRRoughCutSubclipModule
	};

	typedef std::set<ASL::String> ASLStringSet;
	typedef std::map<dvacore::UTF16String, ASL::Guid> MarkerTypeMap;


	namespace Utilities
	{
		PL_EXPORT extern const char* kTimeValue;
		PL_EXPORT extern const char* kTimeFormat;
		PL_EXPORT extern const char* kStartTimecode;
		PL_EXPORT extern const char* kAltTimecode;

		typedef std::map<dvacore::UTF16String, dvacore::UTF16String> CloudData;
		PL_EXPORT
		void RegisterCloudData(const CloudData& inCloudData);

		/*
		**	
		*/
		PL_EXPORT
		void RegisterUnassociatedMetadata(
				const ASL::String& inMetadataID,
				const ASL::String& inMetadataPath = ASL::String(), 
				const ASL::String& inName = ASL::String(), 
				bool isCreativeCloud = false);

		/*
		**	
		*/
		PL_EXPORT
		void RegisterUnassociatedMetadata(const PL::UnassociatedMetadataList& inUnassociatedMetadataList);

		/*
		**	
		*/
		PL_EXPORT
		void UnregisterUnassociatedMetadata(const PL::UnassociatedMetadataList& inUnassociatedMetadataList);

		/*
		**
		*/
		PL_EXPORT
		void BuildClipItemFromLibraryInfo(
						PL::AssetItemList & ioAsseteItemList, 
						SRClipItems& ioClipItems);

		/*
		**
		*/
		PL_EXPORT
		void SwitchToLoggingModule(
						PL::ISRRoughCutRef inRoughCut, 
						BE::ITrackItemSelectionRef inTrackItemSelection);


		/**
		**
		*/ 
		PL_EXPORT
		bool IsAssetItemOffline(const AssetItemPtr& inAssetItem);

		/**
		**
		*/ 
		PL_EXPORT
		bool IsAssetPathOffline(ASL::String const& assetPath);

		/*
		**
		*/
		PL_EXPORT
		void SwitchToLoggingModule(
						const ASL::String& inMediaPath,
						const dvamediatypes::TickTime& inEditTime,
						const dvamediatypes::TickTime& inStart,
						const dvamediatypes::TickTime& inDuration);

		/*
		**
		*/
		PL_EXPORT
		PL::ClipType GetClipType(const PL::ISRPrimaryClipPlaybackRef inPrimaryClipPlayback);

		/*
		**	Get sub-clip's name from the given clip-type track item
		*/
		PL_EXPORT
		ASL::String GetClipName(BE::ITrackItemRef inTrackItem);

		/*
		**	
		*/
		PL_EXPORT
		SREditingModule GetCurrentEditingModule();

		/*
		**
		*/
		PL_EXPORT
		ASL::Result SaveSelectedAssetItems(bool isSilent = false);

		/*
		**
		*/
		PL_EXPORT
		bool PathSupportsXMP(
			const ASL::String& inMediaPath);
		/*
		**
		*/
		PL_EXPORT
		void BuildXMPStringFromMarkers(
							ASL::StdString& ioXMPString, 
							PL::MarkerSet const& inMarkers, 
							PL::TrackTypes const& inTrackTypes,
							dvamediatypes::FrameRate const& inMediaFrameRate);

		/*
		**
		*/
		PL_EXPORT
		void BuildMarkersFromXMPString(
							ASL::StdString const& inXMPString, 
							PL::TrackTypes& outTrackTypes,
							PL::MarkerSet& outMarkers,
                            PL::ISRMarkerOwnerRef inMarkerOwner = PL::ISRMarkerOwnerRef());

		/*
		**
		*/
		PL_EXPORT
		void MergeTemporalMarkers(
							ASL::StdString const& inNewXMPString, 
							ASL::StdString const& inOldXMPString, 
							ASL::StdString& outMergedString);

		/*
		**
		*/
		PL_EXPORT
		bool IsNewerMetaData(
							ASL::StdString const& inNewMediaInfo, 
							ASL::StdString const& inOldMediaInfo);

		/*
		**
		*/
		PL_EXPORT
		bool IsDateTimeEqual(
					const XMP_DateTime& newXMPTime,
					const XMP_DateTime& oldXMPTime);

		/*
		**
		*/
		PL_EXPORT
		dvacore::UTF16String BuildSubClipName(PL::MarkerTrack const& inSubclipMarkers);

		/*
		**
		*/
		PL_EXPORT
		bool ExtractElementValue(
							const ASL::String& inSrc,
							const ASL::String& inTagName,
							ASL::String& outValue,
							ASL::String* outTheRestString = NULL);

		/*
		** Copy from SharedView/SettingUtilities
		*/
		PL_EXPORT
		ASL::String EncodeNameCell(const ASL::String& inPath);

		/*
		**
		*/
		PL_EXPORT
		ASL::String DecodeNameCell(const ASL::String& inPath);

		/*
		** Restore Project cache location to default place
		*/
		PL_EXPORT
		ASL::String RestoreProjectCacheFilesLocationToDefault();

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetProjectCacheFilesLocation();

		/*
		**
		*/
		PL_EXPORT
		int GetProjectCacheFilesLongestReservedDays();

		/*
		**
		*/
		PL_EXPORT
		int GetProjectCacheFilesMaximumNumbers();

		/*
		**
		*/
		PL_EXPORT
		void CleanProjectCacheFiles();

		/*
		**
		*/
		PL_EXPORT
		bool NeedEnableMediaCollection();

		/*
		**
		*/
		PL_EXPORT
		IngestUtils::IngestSettings GetConfigurableIngestSettings();

		/*
		**
		*/
		PL_EXPORT
		void SaveStartMode(bool inIsNative);

		/*
		**
		*/
		PL_EXPORT
		void MoveProjectCacheFilesToNewDestination(ASL::String const& inFolderPath);

		/*
		**
		*/
		PL_EXPORT
		bool IsAssetItemsAppending();

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetClipDurationRegardlessBoundaries(BE::IMasterClipRef const& inMasterClip);

		/*
		**
		*/
		PL_EXPORT
		void SetAssetItemsAppending(bool isAppending);

		/*
		**
		*/
		PL_EXPORT
		PL::CustomMetadata GetCustomMedadata(BE::IMasterClipRef const& inMasterClip);

		/*
		**
		*/
		PL_EXPORT
		ASL::String CreateCustomMetadataStr(PL::CustomMetadata const& inCustomMetadata);

		/*
		**
		*/
		PL_EXPORT
		dvatemporalxmp::XMPTrack* GetTrack(
							const dvacore::UTF16String& inTrackType,
							dvatemporalxmp::XMPDocOpsTemporal* inDocOps);

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetMaxMarkerOutPoint(BE::IMasterClipRef inMasterClip);

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetMinMarkerInPoint(BE::IMasterClipRef inMasterClip);

		/*
		** Restore CCM download location to default place
		*/
		PL_EXPORT
		ASL::String RestoreCCMDownLoadLocationToDefault();

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetCCMDownLoadLocation();

		/*
		**
		*/
		PL_EXPORT
		bool WriteMarkers(
			dvatemporalxmp::XMPDocOpsTemporal& inDocOps,
			const dvacore::UTF16String& inTrackType, 
			const dvacore::UTF16String& inTrackName, 
			PL::MarkerTrack& inMarkerTrack,
			dvamediatypes::FrameRate const& inMediaFrameRate);

		/*
		** In EA mode, we save keywords in xmp but trust other information from projectitem for subclip
		**	so here we update all inout markers but keep their keywords unchanged
		*/
		PL_EXPORT
		bool UpdateInOutMarkersInEAMode(			
			dvatemporalxmp::XMPDocOpsTemporal& inDocOps,
			const dvacore::UTF16String& inTrackType, 
			const dvacore::UTF16String& inTrackName, 
			PL::MarkerTrack& inMarkerTrack,
			dvamediatypes::FrameRate const& inMediaFrameRate);


		/*
		**
		*/
		PL_EXPORT
		bool IsXMPStemEqual(const XMPText& inNewXMPData, const XMPText& inOldXMPData);

		/*
		**
		*/
		PL_EXPORT
		ASL::String ConvertProviderIDToFormat(ASL::String const& inProviderID);

		// Moved from MZUtilities.h


		PL_EXPORT
			ASL::String GetMetadataCreateDate(XMPText const& inXMP);

		//ASL::DateToString will always return English date even if in other language systems on Mac,
		//So this function will return date format: MM/DD/YY, HH:MM:SS, no matter under which language, win or mac
		PL_EXPORT
			ASL::String ASLDateToString(ASL::Date const& inDate);

		/**
		**	Recursive get all sub file and empty folder paths. inPath should be a folder path.
		**	NOTE: the max list size is limited to 2 << 16. All no-empty folders will not be contained.
		*/
		PL_EXPORT
			void ExtendFolderPaths(const ASL::String& inPath, ASL::PathnameList& outSubFilePaths);

		PL_EXPORT
			ASL::String GetRelativePath(const ASL::String& inRootPath, const ASL::String& inFullChildPath);


		PL_EXPORT
			bool ExtractDateFromXMP(XMPText const& inXMP, 
			const char* const inNamespace, 
			const char* const inProperty,
			ASL::Date::ExtendedTimeType& outDate);

		PL_EXPORT
		ASL::Date XMPDateTimeToASLDate(const XMP_DateTime& inXMPTime);

		PL_EXPORT
		ASL::Date GetFileCreationDate(const ASL::String& inFilePath);

		//  Date String format:
		//   YYYY
		//   YYYY-MM
		//   YYYY-MM-DD
		//   YYYY-MM-DDThh:mmTZD
		//   YYYY-MM-DDThh:mm:ssTZD
		//   YYYY-MM-DDThh:mm:ss.sTZD
		PL_EXPORT
			ASL::Date::ExtendedTimeType ASLStringToDate(ASL::String const& inDateStr);

		PL_EXPORT
			void RegisterDBMetadataProvider();

		PL_EXPORT
			void UnregisterDBMetadataProvider();

		PL_EXPORT
			void RegisterDBMetadataProviderForPath(ASL::String const& inPath);


		/**
		**	[NOTE] PL should NOT call this function. It's only used by temp code for debugging.
		*/
		PL_EXPORT
		XMPText ReadXMPFromPhysicalFile(const ASL::String& inFilePath);

		PL_EXPORT
		bool IsFolderBasedClip(const ASL::String& inClipPath);

		/**
		**	If embedded xmp file, return original clip file path, if there is side-car file, return it.
		**	If there are any error, return empty string. Note that the returned path maybe not exist.
		*/
		PL_EXPORT
			ASL::String GetXMPPath(const ASL::String& inClipPath);

		/**
		**	All files needed for clip render and get its metadata are included, the inClipPath will be included too.
		**	All paths returned should have been normalized.
		*/
		PL_EXPORT
			ASL::PathnameList GetClipSupportedPathList(const ASL::String& inClipPath);

		/**
		**	Add all path in inAddedList into ioResultList, except it has been exist in inExceptList or ioResultList.
		*/
		PL_EXPORT
			void MergePathList(
			ASL::PathnameList& ioResultList,
			const ASL::PathnameList& inAddedList,
			const ASL::PathnameList& inExceptList);

		PL_EXPORT
			ASL::String ReplaceFilePartOfPath(const ASL::String& inPath, const ASL::String& inNewName);

		class PL_CLASS_EXPORT SuppressPeakFileGeneration
		{
		public:
			PL_EXPORT
				SuppressPeakFileGeneration();

			PL_EXPORT
				~SuppressPeakFileGeneration();

		private:
			bool mSuppressPeakFileGeneration;
		};

		class PL_CLASS_EXPORT SuppressAudioConforming
		{
		public:
			PL_EXPORT
				SuppressAudioConforming();

			PL_EXPORT
				~SuppressAudioConforming();

		private:
			bool mSuppressAudioConforming;
		};

		/**
		**	
		*/
		bool EnoughDiskSpaceForUpdatingXMP(const ASL::String& inPath, size_t inNewXMPSize);

		/**
		**	inXMPFilePath should be a side-car xmp file, rather than if it exist.
		*/
		PL_EXPORT
			bool IsSideCarFileWritable(const ASL::String& inXMPFilePath);

		/**
		**	Check if a media file's xmp can be updated. It might fail if xmp file is not writable or disk space is not enough.
		**	If inNewXMPSize is not zero, will check disk space to ensure new xmp can be updated.
		*/
		PL_EXPORT
			bool CanUpdateMediaFileXMP(const ASL::String& inPath, size_t inNewXMPSize, ASL::String* outErrorMessage = NULL);

		/**
		**	
		*/
		PL_EXPORT
			ASL::String RenameSideCarFile(const ASL::String& inSideCarPath,	const ASL::String& inOriginalClipName, const ASL::String& inNewClipName);

		/**
		**	
		*/
		PL_EXPORT
			void GetImportableFileFilters(ML::FileFilterMap& outFilters);

		/**
		**	
		*/
		PL_EXPORT
			void GetImportableFileDesc(
			ML::FileFilterMap const& inFileFilters, 
			ML::ExtensionSet& outExtensions,
			ML::FileTypes& outFileTypes);

		PL_EXPORT
			void AsyncCallFromMainThreadWithDelay(
			const boost::function<void()>& inFn,
			unsigned int inDelayInMilliseconds);

		// Moved from MZSequence.h
		/**
		**	Creates a Rough Cut sequence for local editing, export to Ppro or AME with the same settings as the specified masterclip,
		**	with a valid editing mode & preview settings. It does not insert the master clip.
		*/
		PL_EXPORT
			BE::IProjectItemRef CreateSequenceItemFromMasterClip(
			const BE::IMasterClipRef& inMasterClip,
			const BE::IProjectRef& inProject,
			const BE::IProjectItemRef& inContainingBin,
			MZ::SequenceAudioTrackRule inUseType,
			const ASL::String& inSequenceName = ASL_STR(""));

		// Moved from MZContentAnalysis.h
		/*
		** Add Cue points to xmp
		*/
		PL_EXPORT
			ASL::Result AddCuePointsToXMP(const BE::IProjectItemRef inProjectItem,
			SXMPMeta inXmpMetadata, 
			const MZ::CuePointVector& inCuePoints);

		/*
		** Associate Cue points to xmp
		*/
		PL_EXPORT
			ASL::Result  AssociateCuePointsToMasterClip(const BE::IProjectItemRef inProjectItem, 
			const MZ::CuePointVector& inCuePoints);

		/*
		** Add content result to xmp
		*/
		PL_EXPORT
			ASL::Result AddContentAnalysisToXMP(
			SXMPMeta inXmpMetadata, 
			const MZ::CAPerformedVector& inCAPerformedVector);

		/*
		** Add content result to core in memory cache
		*/
		PL_EXPORT
			ASL::Result AddContentAnalysisToXMPToCoreMemoryCache(const BE::IProjectItemRef inProjectItem,
			const MZ::CAPerformedVector& inCAPerformedVector);

		/*
		** Convert Timecode object To string
		*/
		PL_EXPORT
			ASL::String ConvertTimecodeToString(
			const dvamediatypes::TickTime& inTimeCode,
			const dvamediatypes::FrameRate& inFrameRate,
			bool isDropFrame);

		/*
		** Get filename without extension
		** Input example: aa.com.cn.avi
		** Output:
		** ASL::PathUtils::GetFilePart		aa
		** PL::Utilities::GetFileNamePart	aa.com.cn
		*/
		PL_EXPORT
			ASL::String GetFileNamePart(const ASL::String& inFullPath);
		
		/*
		** 
		*/
		PL_EXPORT
		dvamediatypes::TimeDisplay ConvertStringToTimeDisplay(const ASL::String& inTimeDisplayString);

		/*
		** 
		*/
		PL_EXPORT
		ASL::String ConvertTimeDisplayToString(dvamediatypes::TimeDisplay inTimeDisplay);

		/*
		** 
		*/
		PL_EXPORT 
		dvamediatypes::FrameRate GetDefaultVideoFrameRate();

		/*
		** 
		*/
		PL_EXPORT
		XMPText GetXMPTextWithAltTimecodeField(XMPText inXMPText, const BE::IMasterClipRef& inMasterClip);

		/**
		**	only calculate hour, minute and second, no year, month and day.
		*/
		PL_EXPORT
		dvamediatypes::TickTime ConvertDayTimeToTick(ASL::Date::ExtendedTimeType const& inTime);

		/**
		**	Determine if a file's xmp is writable. For local mode, through XMP sdk SXMPFiles::IsMetadataWritable.
		**	For EA mode, always return true.
		*/
		PL_EXPORT
		bool IsXMPWritable(const ASL::String& inPath);

		/*
		**
		*/
		PL_EXPORT
		bool AreFilePathsEquivalent(
			const dvacore::UTF16String& inFilePath1,
			const dvacore::UTF16String& inFilePath2);

		/*
		** 
		*/
		PL_EXPORT
		ASL::String MakeTempPresetFilePath();

		/*
		** 
		*/
		PL_EXPORT
		bool SavePresetToFile(EncoderHost::IPresetRef inPreset, const ASL::String& inPath);

		/*
		** 
		*/
		PL_EXPORT
		ASL::String SavePresetToFile(EncoderHost::IPresetRef inPreset);

		/*
		** 
		*/
		PL_EXPORT
		EncoderHost::IPresetRef CreateCustomPresetForSequence(BE::ISequenceRef inSequence, ML::IExporterModuleRef inExportModule);

		/*
		** 
		*/
		PL_EXPORT
		EncoderHost::IPresetRef CreateCustomPresetForMedia(const ASL::String& inMediaPath, ML::IExporterModuleRef inExportModule);

		/*
		** 
		*/
		PL_EXPORT
		EncoderHost::IPresetRef CreateCustomPresetForAssetItem(const AssetItemPtr& inAssetItem, ML::IExporterModuleRef inExportModule);

		/**
		**
		*/
		PL_EXPORT
		bool IsMediaOffline(const BE::IMasterClipRef& inMasterClip);

		/**
		**	Because still image doesn't own a fixed framerate, read preference result.
		*/
		PL_EXPORT
		dvamediatypes::FrameRate GetStillFrameRate();

		/**
		**	Because still image doesn't own a fixed framerate, read preference result.
		*/
		PL_EXPORT
		bool StillIsDropFrame();

		/*
		**
		*/
		PL_EXPORT
		dvamediatypes::FrameRate GetVideoFrameRate(const BE::IMasterClipRef& inMasterClip);

		/*
		** Trimmed or Untrimmed, Trimmed for still image because we only care its in/out
		**	Untrimmed for other types of MasterClip
		*/
		PL_EXPORT
		dvamediatypes::TickTime GetMasterClipMaxDuration(const BE::IMasterClipRef& inMasterClip);

		PL_EXPORT
		ASL::String GetDisplayStringOfFileSize(const dvacore::filesupport::FileSizeT& inFileSize);

		/**
		**	If any exeption is catched, return 0. If need more info about error,
		**	please use dvacore::filesupport::File::Size directly and catch exception by yourself.
		*/
		PL_EXPORT
		dvacore::filesupport::FileSizeT GetFileSize(const ASL::String& inPath);

		/**
		**	Correct version of dvamediatypes::TimecodeToTime
		**	The same as dvamediatypes version in most cases only except for 29.97NonDrop, 59.94NonDrop and 23.976,
		**	we never round frame rate and get correct transformation b/w tick time and timecode
		*/
		PL_EXPORT
		dvamediatypes::TickTime TimecodeToTickTimeWithoutRoundingFrameRate(
			const ASL::String& inTimecodeString,
			dvamediatypes::FrameRate inFrameRate,
			dvamediatypes::TimeDisplay inTimeDisplay);

		/**
		**	Correct version of dvamediatypes::TimeToTimecode
		*/
		PL_EXPORT
		ASL::String TickTimeToTimecodeWithoutRoundingFrameRate(
			dvamediatypes::TickTime inTime,
			dvamediatypes::FrameRate inFrameRate,
			dvamediatypes::TimeDisplay inTimeDisplay);

		/**
		**	convert the color as 0xaabbggrr
		*/
		PL_EXPORT
		dvaui::drawbot::ColorRGBA ConvertStringToColorRGBA(const dvacore::StdString& inColorRGBAString);

		/**
		**	read the string as format of 0xaabbggrr for color
		*/
		PL_EXPORT
		dvacore::StdString ConvertColorRGBAToString(const dvaui::drawbot::ColorRGBA& inColorRGBA);

		/**
		**	
		*/
		PL_EXPORT
		dvaui::drawbot::ColorRGBA ConvertUINT32ToColorRGBA(std::uint32_t inUINT32ColorRGBA);

		/**
		**	
		*/
		PL_EXPORT
		std::uint32_t ConvertColorRGBAToUINT32(const dvaui::drawbot::ColorRGBA& inColorRGBA);

		/**
		**	Add a new marker to current logging clip
		**
		**	@param	inTemplateMarker:	template of the new marker
		**	@param	inPoint:			where to add the marker, set it to invalid to use current player position
		**								as default
		**	@param	duration:			how long should the marker be, set it to invalid to use the longest value
		**								to the end of clip by default
		**	@param	inIsSilent:			if we want some special UI display, currently Timeline listen to this
		**								value to decide whether to show HUD
		*/
		PL_EXPORT
		PL::CottonwoodMarkerPtr NewMarker(
			const PL::CottonwoodMarker& inTemplateMarker,
			dvamediatypes::TickTime inPoint,
			dvamediatypes::TickTime duration,
			bool inIsSilent,
			MarkerTypeMap& ioPreviousMarkerMap);

		PL_EXPORT
		ASL::String GetInitRoughCutSaveFolder();
		PL_EXPORT
		ASL::String GetInitTagTemplateSaveFolder();

		PL_EXPORT
		ASL::Result IsSequenceSafeToEditAsRoughCut(BE::ISequenceRef inSequence);

		/**
		** If the marker is in/out marker and its related sub clip has been used by any sequences, retur true. Else return false.
		*/
		PL_EXPORT
		bool IsMarkerInUsed(const ASL::Guid& inMarkerGuid);

        /*
        **
        */
		PL_EXPORT
		bool NothingSelectedInTimeline();
        
		/*
        **
        */
		PL_EXPORT
		PL::TrackTransitionMap BuildTransitionItems(
			const BE::ISequenceRef& inSequence,	
			BE::MediaType inType);
	}
}

#endif
