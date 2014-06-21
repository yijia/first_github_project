/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2008 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

#pragma once

#ifndef PLINGESTUTILITIES_H
#define PLINGESTUTILITIES_H

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLINGESTJOB_H
#include "IngestMedia/PLIngestJob.h"
#endif

#ifndef PLINGESTTYPES_H
#include "IngestMedia/PLIngestTypes.h"
#endif

// BE
#ifndef BEPROGRESS_H
#include "BEProgress.h"
#endif

// ASL
#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef MZENCODERMANAGER_H
#include "MZEncoderManager.h"
#endif

namespace PL
{

namespace IngestUtils
{

/**
**	Configurable data from config file
*/
struct IngestSettings
{
	IngestSettings()
		:
		mDisableAllIngestSettings(false),
		mDefaultSubFolderName(dvacore::UTF16String())
	{
	}

	bool					mDisableAllIngestSettings;
	dvacore::UTF16String	mDefaultSubFolderName;
};

/**
**
*/
enum CheckPathResult
{
	kSuccess,
	kFailed,
	kNeedUserConfirm
};

/**
**
*/
PL_EXPORT
bool IsDestAndSrcSame(
					  const dvacore::UTF16String& inSrcFolder, 
					  const ASL::PathnameList& inDestFolders);

/**
**
*/
PL_EXPORT
	bool IsDestAndSrcSame(
	const dvacore::UTF16String& inSrcPath, 
	const dvacore::UTF16String& inDestPath);

/**
**
*/
PL_EXPORT
CheckPathResult CheckTranferPaths(
		const ASL::PathnameList& inSourceFiles,
		const ASL::String& inPrimaryDestination,
		const ASL::PathnameList& inBackupDestinations,
		ASL::String& outErrorMsg);

/**
**
*/
PL_EXPORT
ASL::Result MakeDestinationThrSourceStem(
	ASL::String const& inSource, 
	ASL::String const& inSourceStem, 
	ASL::String const& inDestination,
	ASL::String const& inExtraSubDestFolder,
	ASL::String& outNewDestination);

/**
**
*/
PL_EXPORT
ASL::Result MakeDestinationThrSourceStem(
	ASL::String const& inSource, 
	ASL::String const& inSourceStem, 
	ASL::PathnameList const& inDestination,
	ASL::String const& inExtraSubDestFolder,
	ASL::PathnameList& outNewDestination);

/**
**
*/
//PL_EXPORT
//VerifyFileResult CompareTransferedFiles(
//	IIngestJobItemRef inIngestJobItem,
//	BE::IProgressRef inProgress,
//	std::list<ASL::String>& outCompareReport);

/**
**
*/
PL_EXPORT
ASL::String DateFormatter(
	wchar_t const* inFormat,
	ASL::Date const& inDate);


/**
** Create a Date-string with format "YYYY-DD-MMTHH:MM:SSZ"	
*/
PL_EXPORT
ASL::String CreateDateString(std::tm timeStruct);


/**
** Check the path doesn't have illegal characters and not empty.
*/
PL_EXPORT
bool IsValidPathName(const ASL::String& inPath);

/**
** Check the destination does exist and isn't read-only.
*/
PL_EXPORT
bool IsValidDestination(ASL::String const& inDestination, ASL::String& outErrorMsg);

/**
** Check the inString doesn't have illegalCharaters characters
*/
PL_EXPORT
bool IsValidString(const ASL::String& inString, const ASL::String & inIllegalCharaters);

/**
** Check if the path is a shortcut/alias/symbolic link
*/
PL_EXPORT
bool IsLinkFile(std::string const& inPath);

/**
** Check if the path is a shortcut/alias/symbolic link
*/
PL_EXPORT
bool IsLinkFile(ASL::String const& inPath);

/**
 ** Check if the path is a symbolic link
 */
PL_EXPORT
bool IsSymbolicLink(std::string const& inPath);
	
/**
 ** Check if the path is a symbolic link
 */
PL_EXPORT
bool IsSymbolicLink(ASL::String const& inPath);

/**
 **
 */
PL_EXPORT
ASL::String MakeAutoRenamedPath(const ASL::String& inPath);

/**
 **
 */
PL_EXPORT
bool PathsExist(const ASL::PathnameList& inPaths, ASL::String& outNotExistPath);

/**
 **
 */
PL_EXPORT
bool PathsAreWritable(const ASL::PathnameList& inPaths, ASL::String& outNotWritablePath);

/**
 **
 */
PL_EXPORT
bool NoDuplicatedPaths(const ASL::PathnameList& inPaths, ASL::String& outDuplicatedPath);

/**
 ** inAllowSame: If true, when parent folder is same with child will return true, else return false.
 */
PL_EXPORT
bool FolderContain(
	const ASL::String& inParentPath,
	const ASL::String& inChildPath,
	bool inAllowSame);

/**
 **
 */
PL_EXPORT
bool EnoughSpaceForCopy(
	const ASL::PathnameList& inSources,
	const ASL::PathnameList& inDestinations,
	ASL::String& outDstPath,
	ASL::UInt64& outNeededSize);

/**
 **	Copy file attributes to destination and remove read-only flag
 */
PL_EXPORT
void StampPathEntryInformation(
	ASL::String const& inSource,
	ASL::String const& inDestination);

/**
 ** Preprocess copy action. If need replace, then delete original file, if need rename, then make the renamed path.
 **	ioDestinationPath: If need renamed, will be filled with the renamed destination path.
 **	ioCopyAction: Return the rest copy action need to do when do copy operation. For example, if pass in kCopyAction_Replaced,
 **		then after preprocess deleted the original file, copy action will be filled with kCopyAction_Copied.
 ** Return result indicates whether preprocessing is successful.
 */
PL_EXPORT
ASL::Result CopyActionPreprocess(
	const ASL::String& inSourcePath,
	ASL::String& ioDestinationPath,
	CopyAction& ioCopyAction);

/**
 **	inSourcePath and inDestinationPath should have been normalized
 */
PL_EXPORT
ASL::Result CopySingleFile(const ASL::String& inSourcePath, const ASL::String& inDestinationPath);

/**
 ** inSourcePath and ioDestinationPath should have been normalized
 ** ioDestinationPath: if copy action is renamed, ioDestinationPath will be filled with final renamed destination.
 **	ioCopyAction: According to its input value to copy and fill its value with the final real copy action.
 */
PL_EXPORT
ASL::Result CopySingleFile(
	const ASL::String& inSourcePath,
	ASL::String& ioDestinationPath,
	CopyAction& ioCopyAction);

typedef boost::function<bool (ASL::Float32 inLastPercentDone, ASL::Float32 inPercentDone)> CopyProgressFxn;

/**
 ** inSourcePath and ioDestinationPath should have been normalized
 ** ioDestinationPath: if copy action is renamed, ioDestinationPath will be filled with final renamed destination.
 **	ioCopyAction: According to its input value to copy and fill its value with the final real copy action.
 */
PL_EXPORT
ASL::Result CopySingleFileWithProgress(
	ASL::String const& inSourcePath,
	ASL::String& ioDestination,
	CopyAction& ioCopyAction,
	const CopyProgressFxn& inProgressFxn);

PL_EXPORT
ASL::Result IncrementalCopyFiles(
	ASL::String const& inSourcePath,
	ASL::PathnameList const& inDestinations,
	const IngestUtils::CopyProgressFxn& inProgressFxn);

/**
 ** Smart copy file by its size. If it's small file, then copy with small mode, and large file with progress copy, huge file with incremental copy.
 ** inSourcePath and ioDestinationPath should have been normalized
 ** ioDestinationPath: if copy action is renamed, ioDestinationPath will be filled with final renamed destination.
 **	ioCopyAction: According to its input value to copy and fill its value with the final real copy action.
 ** [NOTE] Maybe we need a 1:n version smart copy function, then please implement it with IncrementalCopyFiles directly.
 */
PL_EXPORT
ASL::Result SmartCopyFileWithProgress(
	ASL::String const& inSourcePath,
	ASL::String& ioDestination,
	CopyAction& ioCopyAction,
	const CopyProgressFxn& inProgressFxn);

/**
 **	Return the human-readable string according to copy result if copy failed.
 */
PL_EXPORT
ASL::String ReportStringOfCopyResult(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	const ASL::Result& inResult);

/**
 ** Return the human-readable string according to copy action, maybe empty string if nothing happened.
 **	inDestinationPath: the destination path want to copy to.
 **	inRealDestinationPath: the destination path copied to. It may be different with inDestinationPath if user allow auto rename.
 */
PL_EXPORT
ASL::String ReportStringOfCopyAction(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	const ASL::String& inRealDestinationPath,
	const CopyAction& inCopyAction);

/**
 ** Only call this function when processing ingest dialog, NOT when pre-check.
 */
PL_EXPORT
bool GenerateFileCopyAction(
	const CopyExistOption& inSingleFileOption,
	CopyExistOption& ioJobOption,
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	CopyAction& outResultAction);

/**
 ** Only call this function when processing ingest dialog, NOT when pre-check.
 */
PL_EXPORT
bool GenerateFolderCopyAction(
	const CopyExistOption& inSingleFileOption,
	CopyExistOption& ioJobOption,
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	CopyAction& outResultAction);

/**
 ** If user canceled warning dialog, return false, else return true
 */
PL_EXPORT
bool WarnUserForExistPath(
	ASL::String const& inSrcPath,
	ASL::String const& inDestPath,
	bool inIsDirectory,
	int& outCopyExistOption,
	int& outApplyToAll,
	bool inAllowCancel,
	bool inIsconflictWithCopyingTask,
	bool inIsPrecheck);

PL_EXPORT
VerifyFileResult VerifyFile(
	const VerifyOption& inOption, 
	const ASL::String& inSrc, 
	const ASL::String& inDest,
	ASL::String& outResultMsg);

#ifndef DO_NOT_USE_AME
/*
**	PrepareMasterClipForTranscode
**
**	This method pops the shared Export Movie dialog box and makes the necessary
**	calls into the MediaLayer to compile and export the movie.
**
**	@param inType is either movie, frame or audio.
**	@param 
**	@param 
*/
PL_EXPORT
bool TranscodeMediaFile(
	BE::CompileSettingsType				inType,
	const dvacore::UTF16String&			inSourceClipPath,
	const dvacore::UTF16String&			inDestClipPath,
	const dvacore::UTF16String&			inDestClipName,
	const dvacore::UTF16String&			inPresetPath,
	const dvamediatypes::TickTime&		inStart,
	const dvamediatypes::TickTime&		inEnd,
	const MZ::RequesterID&				inRequestID,
	const MZ::JobID&					inJobID,
	dvacore::UTF16String&				outErrorInfo);

PL_EXPORT
bool TranscodeConcatenateFile(
	const RequesterID& inRequesterID,
	const JobID& inJobID,
	EncoderHost::IPresetRef inPreset,
	BE::IProjectRef inProject,
	BE::ISequenceRef inSequence,
	dvacore::UTF16String const& inRenderPath,
	dvacore::UTF16String const& inOutputName,
	IngestTask::PathToErrorInfoVec& outErrorInfos);

PL_EXPORT
bool SendSequenceToEncoderManager(
	const RequesterID& inRequesterID,
	const JobID& inJobID,
	EncoderHost::IPresetRef inPreset,
	BE::IProjectRef inProject,
	BE::ISequenceRef inSequence,
	dvacore::UTF16String const& inRenderPath,
	dvacore::UTF16String const& inOutputName);

#endif // DO_NOT_USE_AME

PL_EXPORT
bool CreateSequenceForConcatenateMedia(
	IngestTask::ClipSettingList const& inClipSettingList, 
	BE::IProjectRef& outProject, 
	BE::ISequenceRef& outSequence, 
	ASL::String const& inConcatenateName, 
	IngestTask::PathToErrorInfoVec& outErrorInfos);

PL_EXPORT
void TranscodeStatusToString(
				dvametadatadb::StatusType inStatus,
				TranscodeResult& outFailure);

} // IngestUtils

} // namespace PL

#endif
