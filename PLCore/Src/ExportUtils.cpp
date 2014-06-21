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

// Prefix
#include "Prefix.h"

// Self
#include "ExportUtils.h"

// ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLString.h"

// MBC
#include "Inc/MBCProvider.h"

// DVA
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/utility/NumberString.h"
#include "dvacore/debug/debug.h"
#include "dvacore/utility/UnicodeCaseConversion.h"

// MZ
#include "IngestMedia/PLIngestJob.h"
#include "PLClipFinderUtils.h"
#include "IngestMedia/PLIngestUtils.h"
#include "MZUtilities.h"

// UIF
#include "UIFMessageBox.h"

namespace PL
{

namespace ExportUtils
{

static const ASL::String kActualMeidaFilePathBeginTag = ASL_STR("<ActualMediaFilePath>");
static const ASL::String kActualMeidaFilePathEndTag = ASL_STR("</ActualMediaFilePath>");
static const ASL::String kRelativePathBeginTag = ASL_STR("<RelativePath>");
static const ASL::String kRelativePathEndTag = ASL_STR("</RelativePath>");

/*
** We cannot use ASL::PathUtils::RemoveTrailingSlash here because for ftp, we use 
** "/" as a slash with in windows and mac
*/
static ASL::String RemoveLastSlash(ASL::String const& inPath)
{
	ASL::String path = inPath;

	ASL::String slashes = ASL_STR("/\\");
	if (!path.empty() && slashes.find(*path.rbegin()) != ASL::String::npos)
	{
		path.erase(path.size() - 1);
	}

	return path;
}

/*
** <Note> In project file, ActualMediaFilePath and RelativePath are pair. RelativePath always
** follows AcautlMeidaFilePath. We also assume meidaFile and the projectFile have the same parent
** directory. For relativePath, just use the lastesActualMediaFilePath and projectFileContainer 
** to calculate the relativePath.
*/
static void ReplaceLineContent(
	ASL::String &inOutLine,
	const std::vector<ASL::String> &inXMLTags,
	const PathPairVector &inMediaPaths,
	const ASL::String &inProjectFileContainer)
{
	//Check the XML flag
	for (std::vector<ASL::String>::const_iterator it = inXMLTags.begin(); it != inXMLTags.end(); ++it)
	{	
		if (inOutLine.find(it->c_str()) == ASL::String::npos)
			continue;

		if (kRelativePathBeginTag == *it)
		{
			int beginIndex = inOutLine.find(kRelativePathBeginTag) + kRelativePathBeginTag.size();
			int endIndex = inOutLine.find(kRelativePathEndTag);

			ASL_ASSERT(beginIndex != ASL::String::npos);
			ASL_ASSERT(endIndex != ASL::String::npos);

			// we must compute the correct actual media path based on old relative path,
			// then get the new actual path from the old actual path
			ASL::String const& oldRelativePath = inOutLine.substr(beginIndex, endIndex - beginIndex);
			ASL::String const& oldActualMediaPath = MZ::Utilities::CombinePathsWithoutRelativeInfo(
								ASL::PathUtils::AddTrailingSlash(inProjectFileContainer), oldRelativePath);
			ASL::String newActualMediaPath;

			PathPairVector::const_iterator pathIterator;
			for (pathIterator = inMediaPaths.begin(); pathIterator != inMediaPaths.end(); ++pathIterator)
			{
#if ASL_TARGET_OS_WIN
				if (pathIterator->first.c_str() == oldActualMediaPath)
#else
					if (MZ::Utilities::AddHeadingSlash(pathIterator->first.c_str()) ==
						MZ::Utilities::AddHeadingSlash(oldActualMediaPath))
#endif
				{
					newActualMediaPath = pathIterator->second;
					break;
				}
			}

			if (newActualMediaPath.empty())
			{
				continue;
			}

			//ActualMediaFilePath and RelativePath must be pair.
			//MeidaFile and projectFile must have the same parent directory.
			ASL_ASSERT(newActualMediaPath.find(inProjectFileContainer) != ASL::String::npos);

			// Calculate the relativePath
			// If the projectFileContainer is empty(this only happens when exporting to through ftp),
			// we just add ./ for the relative path
			ASL::String relativePath = inProjectFileContainer.empty() ? 
									newActualMediaPath.insert(0,  ASL_STR("./")) :
									newActualMediaPath.replace(
										newActualMediaPath.find(inProjectFileContainer),
										inProjectFileContainer.size(),
										ASL_STR("."));

			inOutLine.replace(beginIndex, endIndex - beginIndex, relativePath);
		}
		else
		{
			PathPairVector::const_iterator pathIterator;
			for (pathIterator = inMediaPaths.begin(); pathIterator != inMediaPaths.end(); ++pathIterator)
			{
				int pathIndex = inOutLine.find(pathIterator->first.c_str());
				if (pathIndex != ASL::String::npos)
				{
					inOutLine.replace(pathIndex, pathIterator->first.size(), pathIterator->second);
					break;
				}
			}			
		}
	} // end Check XML tag
}

static void ReplaceContent(const ASL::String &inString,
	const std::vector<ASL::String> &inXMLTags,
	const PathPairVector &inMediaPaths,
	const ASL::String &inProjectFileContainer,
	ASL::String &outString)
{

	char delimiter = '\n';
	int startIndex = 0, endIndex = 0;

	while ((endIndex = inString.find(delimiter, startIndex)) != ASL::String::npos)
	{
		ASL::String line = inString.substr(startIndex, endIndex - startIndex + 1);
		ReplaceLineContent(line, inXMLTags, inMediaPaths, inProjectFileContainer);
		outString += line;
		startIndex = endIndex + 1;
	}

	//the LastLine
	if (startIndex != std::string::npos)
	{
		ASL::String line = inString.substr(startIndex);
		ReplaceLineContent(line, inXMLTags, inMediaPaths, inProjectFileContainer);
		outString += line;
	}
}

/*
**
*/
void ReplaceMeidaPath(const ASL::String &inProjectFilePath,
	const ASL::String &inDestProjectDir,
	const PathPairVector &inMediaPaths)
{
	ASL::String stringFileContent;

	//Get the content of the inFilePath
	{	
		ASL::File file;
		ASL::Result result = ASL::File::Open(inProjectFilePath, ASL::FileAccessFlags::kRead, file);

		DVA_ASSERT_MSG(ASL::ResultSucceeded(result), "File: " << inProjectFilePath << "unable to open.");

		std::vector<char> fileContents;
		ASL::UInt32 bytesRead = 0;
		std::size_t bytesOnDisk = static_cast<std::size_t>(file.SizeOnDisk());
		fileContents.resize(bytesOnDisk + 1);
		void* readIntoBuffer = &(fileContents[0]);
		result = file.Read(readIntoBuffer, static_cast<ASL::UInt32>(bytesOnDisk), bytesRead);

		// Add zero termination byte
		fileContents[bytesOnDisk] = 0; 

		stringFileContent = ASL::MakeString(&fileContents[0]);

		DVA_ASSERT_MSG(ASL::ResultSucceeded(result), "File: " << inProjectFilePath << " unable to read file on disk.");
	}

	std::vector<ASL::String> xmlToFindTags;
	xmlToFindTags.push_back(kActualMeidaFilePathBeginTag);
	xmlToFindTags.push_back(kRelativePathBeginTag);
	xmlToFindTags.push_back(ASL_STR("<FilePath>"));
	xmlToFindTags.push_back(ASL_STR("<ConformedAudioPath>"));

	ASL::String newStringFileContent;
	
	ASL::String projectFileContainer = RemoveLastSlash(inDestProjectDir);

	ReplaceContent(stringFileContent, xmlToFindTags, inMediaPaths, projectFileContainer, newStringFileContent);

	//Write newStringFileContent into an temp File
	ASL::String tmpName = ASL::PathUtils::AddTrailingSlash(ASL::PathUtils::GetTempDirectory()) + ASL_STR("tmp.file");
	ASL::String uniqueTmpFileName = ASL::PathUtils::MakeUniqueFilename(tmpName);

	{	
		ASL::File tmpFile;
		ASL::Result result = tmpFile.Create(
			uniqueTmpFileName,
			ASL::FileAccessFlags::kWrite,
			ASL::FileShareModeFlags::kNone,
			ASL::FileCreateDispositionFlags::kCreateNew,
			ASL::FileAttributesFlags::kFlagSequentialScan);

		DVA_ASSERT_MSG(ASL::ResultSucceeded(result), "File: " << uniqueTmpFileName << "unable to create.");

		dvacore::UTF8String string8 = dvacore::utility::UTF16to8(newStringFileContent);
		ASL::UInt32 numberOfBytesWritten;

		result = tmpFile.Write(string8.c_str(),
			string8.length(),
			numberOfBytesWritten);

		DVA_ASSERT_MSG(ASL::ResultSucceeded(result), "File: " << uniqueTmpFileName << " unable to write file.");
	}

// If dest file exists, ASL::Copy will fail on mac and ASL::Move will fail on win
#if ASL_TARGET_OS_WIN
	ASL::File::Copy(uniqueTmpFileName, inProjectFilePath);
	ASL::File::Delete(uniqueTmpFileName);
#else
	ASL::File::SwapTempFile(uniqueTmpFileName, inProjectFilePath);
#endif
}

/*
** Used to determine whether or not to use ASL::File::Copy or our own homebrew CopyWithProgress.
*/
static ASL::UInt32 const k1Kilobyte = 1024;
static ASL::UInt32 const kIncrementalCopySize = 1 * k1Kilobyte * k1Kilobyte; // 1 MB is a WAG.  Adjust manually after empirical testing.

ASL::Result CopyWithProgress(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	ASL::File::ASLCopyProgressFunc inProgressFunc,
	void* inProgressData)
{
	ASL::File source;
	ASL::Result sourceOpenResult = source.Create(
		inSourcePath,
		ASL::FileAccessFlags::kRead,
		ASL::FileShareModeFlags::kShareRead,
		ASL::FileCreateDispositionFlags::kOpenExisting,
		ASL::FileAttributesFlags::kFlagSequentialScan); // | ASL::FileAttributesFlags::kFlagNoBuffering

	if (ASL::ResultFailed(sourceOpenResult))
	{
		return sourceOpenResult;
	}

	if (!source.IsOpen())
	{
		return ASL::eUnknown;
	}

	ASL::File destination;
	ASL::Result destinationCreateResult = destination.Create(
		inDestinationPath,
		ASL::FileAccessFlags::kWrite,
		ASL::FileShareModeFlags::kNone,
		ASL::FileCreateDispositionFlags::kCreateAlways,
		ASL::FileAttributesFlags::kFlagSequentialScan); // | ASL::FileAttributesFlags::kFlagNoBuffering

	if (ASL::ResultFailed(destinationCreateResult))
	{
		return destinationCreateResult;
	}

	typedef char byte;
	ASL::UInt32 const kAlignmentSlough = 8 * k1Kilobyte; // [TRICKY] 8 KB to get alignment correct, for ASL::FileAttributesFlags::kFlagNoBuffering undocumented requirement.
	std::vector<byte> buffer(kIncrementalCopySize + kAlignmentSlough);
	byte* p = &buffer[0];
	ASL::UInt64 i = ASL::UInt64(p);
	i &= kAlignmentSlough - 1; // [TRICKY] relying on AND mask.  See Hacker's Delight by Henry S Warren, jr.

	if (i != 0)
	{
		p += kAlignmentSlough - i;
	}

	ASL::UInt32 chunk = kIncrementalCopySize;
	ASL::UInt64 totalBytes = source.SizeOnDisk();

	for (ASL::UInt64 destinationRemaining = totalBytes; destinationRemaining > 0; )
	{
		if (chunk > destinationRemaining)
		{
			chunk = ASL::UInt32(destinationRemaining);
		}

		ASL::UInt32 chunkRead = 0;
		source.Read(p, chunk, chunkRead);

		if (chunk != chunkRead)
		{
			return ASL::eUnknown;
		}

		ASL::UInt32 chunkWrite = 0;
		destination.Write(p, chunk, chunkWrite);

		if (chunk != chunkWrite)
		{
			return ASL::eUnknown;
		}

		destinationRemaining -= chunk;

		bool continueCopy = inProgressFunc((ASL::Float32)(totalBytes - destinationRemaining) / (ASL::Float32)totalBytes, inProgressData);

		if (!continueCopy)
		{
			return ASL::eUserCanceled;
		}
	}

	return ASL::kSuccess;
}

/*
**
*/
static ASL::String MakeUniqueTopLevelName(
	std::set<ASL::String> const &inNameSet, 
	const ASL::String &inName,
	bool inTopLevelIsDir)
{
	ASL::String uniqueName = inName;

	if (!uniqueName.empty())
	{
		for (std::uint32_t index = 1;
			inNameSet.find(uniqueName) != inNameSet.end();
			++index)
		{
			if (inTopLevelIsDir)
			{
				uniqueName = inName + ASL_STR("_") + dvacore::utility::NumberToString(index);
			}
			else
			{
				ASL::String filePart, fileExtension;
				ASL::PathUtils::SplitPath(inName, 0, 0, &filePart, &fileExtension);
				if (!fileExtension.empty())
				{
					uniqueName = filePart + ASL_STR("_") + dvacore::utility::NumberToString(index) + fileExtension;	
				}
				else
				{
					uniqueName = filePart + ASL_STR("_") + dvacore::utility::NumberToString(index);
				}
			}
		}
	}

	return uniqueName;
}

/*
** 
*/
ASL::String GetTopLevelName(
	ASL::String const& inMeidaPath,
	ASL::String const& inSourceStem)
{
	ASL::String topLevelName;
	if (inSourceStem != ASL::MakeString(PLMBC::kVolumeListPath))
	{
		ASL::String mediaDirectory = ASL::PathUtils::AddTrailingSlash(
			ASL::PathUtils::GetFullDirectoryPart(ASL::PathUtils::RemoveTrailingSlash(inMeidaPath)));
		ASL::String tmpSourceItem = ASL::PathUtils::AddTrailingSlash(inSourceStem);

		ASL_ASSERT(mediaDirectory.find(tmpSourceItem) != ASL::String::npos);

		ASL::String tmptopDirectory = mediaDirectory.substr(tmpSourceItem.length());
		topLevelName = tmptopDirectory.substr(0, tmptopDirectory.find(ASL::PathUtils::kPathSeparator));
		if (topLevelName.empty())
		{
			topLevelName = ASL::PathUtils::GetFullFilePart(inMeidaPath);
		}
	}
	else
	{
		topLevelName = ASL::PathUtils::GetDrivePart(inMeidaPath);
	}

	return topLevelName;
}

/*
**
*/
ASL::Result BuildUniqueExportFilesMap(
	std::set<ASL::String> const& inExportMediaFiles,
	MeidaPathToExportItemMap& outMediaPathToExportItem,
	boost::function<bool (ASL::UInt64)> const& inUpdateProgressFn/* = boost::function<bool (ASL::UInt64)>()*/)
{
	// Use exportFiles to gather all files and help to erase the possible duplicated files for different meidaFiles in the same source folder. 
	std::set<ASL::String> exportFiles;
	for (std::set<ASL::String>::const_iterator f = inExportMediaFiles.begin(), l = inExportMediaFiles.end(); f != l; ++f)
	{
		ASL::String const& mediaPath = *f;
		ExportItem exportItem;

		// Ignore all offLine files
		if (ASL::PathUtils::ExistsOnDisk(mediaPath))
		{
			// Parse the logical(folder) struct of the media
			ASL::PathnameList fileList;
			ASL::String clipName;
			ASL::String providerID;
			PL::GetLogicalClipFromMediaPath(mediaPath, exportItem.sourceStem, fileList, clipName, providerID);		

			ASL::PathnameList::const_iterator itr = fileList.begin();
			ASL::PathnameList::const_iterator end = fileList.end();

			// Check all related stuff
			for (; itr != end; ++itr)
			{
				ASL::String const& itemPath = ASL::PathUtils::RemoveTrailingSlash(*itr);

				// Make sure this itemPath is unique
				if (exportFiles.find(itemPath) == exportFiles.end())
				{
					exportFiles.insert(itemPath);
					exportItem.exportFilePaths.push_back(itemPath);
				}
			}

			// Add this exportItem into map
			DVA_ASSERT_MSG(!exportItem.exportFilePaths.empty(), "Empty export Files!");
			outMediaPathToExportItem.insert(std::make_pair(mediaPath, exportItem));
		} 

		if (inUpdateProgressFn)
		{
			//Use this to test whether user cancels the operation, it's equal to invoke GetAbort()
			//the implementation of ProgressImpl::UpdateProgress() make protection that if 0 is passed in, 
			//the function will return false and do nth.
			if (inUpdateProgressFn(0))
			{
				return ASL::eUserCanceled;
			}
			inUpdateProgressFn(1);
		}
	}

	return ASL::kSuccess;
}

/*
**
*/
ASL::String BuildUnqiueDestinationPath(
	ASL::String const& inMediaPath,
	ASL::String const& inDestRoot,
	ExportItem const& inExportItem)
{
	// Calculate destPath
	ASL::String itemDest;
	PL::IngestUtils::MakeDestinationThrSourceStem(inMediaPath, inExportItem.sourceStem, inDestRoot, ASL_STR(""), itemDest);
	ASL::String const& sourceTail = ASL::PathUtils::GetFullFilePart(inMediaPath);
	ASL::String destPath = ASL::PathUtils::AddTrailingSlash(itemDest) + sourceTail;

	// Use unique name
	ASL::String const& topLevelName =
		PL::ExportUtils::GetTopLevelName(inMediaPath, inExportItem.sourceStem);

	// Refer to the comments in definition of ExportItem, which is why to use a map for topLevelName
	TopLevelNameMap::const_iterator it = inExportItem.topLevelNameMap.find(topLevelName);
	if (it != inExportItem.topLevelNameMap.end())
	{
		ASL::String uniqueTopLevelName = (*it).second;
		if (!uniqueTopLevelName.empty() && uniqueTopLevelName != topLevelName)
		{
			destPath = destPath.replace(ASL::PathUtils::AddTrailingSlash(inDestRoot).length(),
				topLevelName.length(), 
				uniqueTopLevelName);
		}
	}

	return destPath;
}

/*
**
*/
void UniqueSameSourceNamePaths(
	MeidaPathToExportItemMap& ioMediaPathToExportItem, 
	std::set<ASL::String>& outRenameMediaPaths)
{
	std::map<ASL::String, ASL::String> mediaToTopNameMap;
	std::set<ASL::String> topLevelNames;
	
	for (MeidaPathToExportItemMap::iterator iter = ioMediaPathToExportItem.begin();
		iter != ioMediaPathToExportItem.end();
		++iter)
	{	
		ExportMediaPath const& mediaPath = iter->first; 
		ExportItem& exportItem = iter->second;

		ASL::String const& topLevelName = GetTopLevelName(mediaPath, exportItem.sourceStem);

		//To fix bug #3114448, sidecar of MXF file is strongly required to bring if export, so add this walk around.
		bool isMXFSingleMedia = dvacore::utility::LowerCase(ASL_STR(".mxf")) == dvacore::utility::LowerCase(ASL::PathUtils::GetExtensionPart(mediaPath));
		ASL_ASSERT_MSG(topLevelName != ASL::PathUtils::GetFullFilePart(mediaPath) || exportItem.exportFilePaths.size() == 1 || isMXFSingleMedia,
			"A media file have more than one related files!");

		ASL::PathnameList const& exportFilePaths = exportItem.exportFilePaths;
		ASL::PathnameList::const_iterator f = exportFilePaths.begin();
		ASL::PathnameList::const_iterator f_end = exportFilePaths.end();
		for ( ; f != f_end; f++)
		{
			ASL::String itemDest;
#if ASL_TARGET_OS_WIN
			ASL::String const& itemPath =  ASL::PathUtils::Win::StripUNC(ASL::PathUtils::ToNormalizedPath( ASL::PathUtils::RemoveTrailingSlash(*f) ));
#else
			ASL::String const& itemPath = ASL::PathUtils::RemoveTrailingSlash(*f);
#endif
			ASL_ASSERT(!itemPath.empty());
			if (itemPath.empty())
				continue;

			ASL::String const& itemTopLevelName = GetTopLevelName(itemPath, exportItem.sourceStem);

			if (!itemTopLevelName.empty())
			{	
				ASL::String const& mediaID = ASL::PathUtils::AddTrailingSlash(exportItem.sourceStem) + itemTopLevelName;
				ASL::String uniqueTopLevelName;

				// For the two medias from the same source folder struct
				if (mediaToTopNameMap.find(mediaID) != mediaToTopNameMap.end())
				{
					// For different clips from the same struct folder, we should set the top level folder
					// for every clip.
					exportItem.topLevelNameMap[itemTopLevelName] = mediaToTopNameMap[mediaID];
					if (itemTopLevelName != mediaToTopNameMap[mediaID])
					{
						outRenameMediaPaths.insert(mediaPath);
					}
				}
				else
				{
					uniqueTopLevelName =
						PL::ExportUtils::MakeUniqueTopLevelName(topLevelNames, itemTopLevelName, itemTopLevelName != ASL::PathUtils::GetFullFilePart(itemPath));
					topLevelNames.insert(uniqueTopLevelName);
					mediaToTopNameMap[mediaID] = uniqueTopLevelName;
					exportItem.topLevelNameMap[itemTopLevelName] = uniqueTopLevelName;

					// We reach here if two media's fristLevelDirectory are same
					if (itemTopLevelName != uniqueTopLevelName)
					{
						outRenameMediaPaths.insert(mediaPath);
					}
				}
			}
		}
	}
}

ASL::Result UniquneSameSourceNamePathsWithPrompt(MeidaPathToExportItemMap& ioMediaPathToExportItem)
{
	std::set<ASL::String> renameMeidaPaths;
	PL::ExportUtils::UniqueSameSourceNamePaths(ioMediaPathToExportItem, renameMeidaPaths);
	if (!renameMeidaPaths.empty())
	{
		ASL::String warnUserCaption = dvacore::ZString(
			"$$$/Prelude/Mezzanine/ExportUtils/ExportCaption=Prelude");

		ASL::String warnSameSourceNameMessage = dvacore::ZString(
			"$$$/Prelude/Mezzanine/ExportUtils/SameSourcePathName=The following files will be renamed automatically.\n@0\nDo you want to continue?");

		ASL::String mediaPaths;
		for (std::set<ASL::String>::const_iterator iter = renameMeidaPaths.begin();
			iter != renameMeidaPaths.end();
			++iter)
		{
			mediaPaths += *iter + ASL_STR("\n");
		}

		warnSameSourceNameMessage = dvacore::utility::ReplaceInString(warnSameSourceNameMessage, mediaPaths);

		UIF::MBResult::Type result = MZ::Utilities::PromptUIFMessageBoxFromMainThread(
															warnSameSourceNameMessage, 
															warnUserCaption, 
															UIF::MBFlag::kMBFlagYesNo, 
															UIF::MBIcon::kMBIconWarning);

		if ( result != UIF::MBResult::kMBResultYes )
			return ASL::eUserCanceled;
	}
	return ASL::kSuccess;
}

} //namespace ExportUtils

}// namespace PL
