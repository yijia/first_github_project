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
#include "IngestMedia/PLIngestUtils.h"
#include "MZEncoderManager.h"
#include "MZBEUtilities.h"
#include "MZProject.h"
#include "MZMasterClip.h"
#include "MZSaveProject.h"
#include "IngestMedia/PLIngestMessageBox.h"
#include "MZUtilities.h"
#include "MZProjectActions.h"
#include "MZSequence.h"
#include "PLLibrarySupport.h"
#include "PLPrivateCreator.h"
#include "PLFactory.h"
#include "MZSequenceActions.h"
#include "PLSaveAsPremiereProject.h"

#include "PLUtilities.h"

//UIF
#include "UIFFocusSaver.h"
#include "UIFMessageBox.h"
#include "UIFWorkspace.h"

// BE
#include "BEBackend.h"
#include "BEMasterClip.h"
#include "BEResults.h"
#include "BEProperties.h"
#include "BE/Project/IClipProjectItem.h"

// ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLString.h"
#include "ASLFile.h"
#include "ASLContract.h"
#include "ASLBatchOperation.h"
#include "ASLStationUtils.h"
#include "ASLSleep.h"
#include "ASLStringCompare.h"
#include "ASLClassFactory.h"

// MBC
#include "Inc/MBCProvider.h"

// DVA
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/utility/StringCompare.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/debug/debug.h"
#include "dvacore/filesupport/file/file.h"
#include "dvaui/dialogs/UI_ProgressDialog.h"
#include "dvaworkspace/workspace/Workspace.h"
#include "dvaui/dialogs/UI_MessageBox.h"

// OS
#if ASL_TARGET_OS_WIN
#include "atlbase.h" 
#include "shlobj.h"
#else
#include "sys/stat.h"
#endif

// MD5
#include <adobe/md5.hpp>

namespace PL
{

	/*
	**
	*/
	extern const ASL::StationID kStation_IngestMedia(ASL_STATIONID("MZ.Station.IngestMedia"));


namespace
{
	CopyAction MapCopyExistOption2Action(const CopyExistOption& inOption)
	{
		switch(inOption)
		{
		case kExist_WarnUser:
			return kCopyAction_Copied;
			break;
		case kExist_Ignore:
			return kCopyAction_Ignored;
			break;
		case kExist_Replace:
			return kCopyAction_Replaced;
			break;
		default:
			return kCopyAction_Copied;
			break;
		}

		return kCopyAction_Copied;
	}

	VerifyFileResult VerifyFileSize(const ASL::String& inSrc, const ASL::String& inDest)
	{
		DVA_ASSERT(!inSrc.empty() && !inDest.empty());

		ASL::UInt64 srcSize = 0;
		ASL::UInt64 destSize = 0;
		if ( ASL::kSuccess != ASL::File::SizeOnDisk(inSrc, srcSize) || 
			ASL::kSuccess != ASL::File::SizeOnDisk(inDest, destSize) )
		{
			return kVerifyFileResult_FileBad;
		}

		if ( srcSize != destSize )
		{
			return kVerifyFileResult_SizeDifferent;
		}

		return kVerifyFileResult_Equal;
	}

	VerifyFileResult VerifyFileContent(const ASL::String& inSrc, const ASL::String& inDest)
	{
		DVA_ASSERT(!inSrc.empty() && !inDest.empty());

		VerifyFileResult result = kVerifyFileResult_Equal;

		result = VerifyFileSize(inSrc, inDest);
		if ( result != kVerifyFileResult_Equal )
		{
			return result;
		}

		// Buffer size is set to about 1M
		const ASL::UInt32 kBufferSize = 4096 * 25 * 10;
		ASL::File srcFile;
		ASL::File destFile;
		try
		{
			// [TO DO]: Need borrow the idea from Transfer Media component for buffer alignment
			int const kMemcmpMatch = 0;
			std::vector<unsigned char> srcBuf(kBufferSize);
			std::vector<unsigned char> destBuf(kBufferSize);
			unsigned char* srcBuffer = &srcBuf[0];
			unsigned char* destBuffer = &destBuf[0];

			if ( ASL::ResultSucceeded( ASL::File::Open(inSrc, ASL::FileAccessFlags::kRead, srcFile) ) && 
				ASL::ResultSucceeded( ASL::File::Open(inDest, ASL::FileAccessFlags::kRead, destFile) ) )
			{
				ASL::UInt32 srcReadLen = 0;
				ASL::UInt32 destReadLen = 0;
				while ( ASL::ResultSucceeded(srcFile.Read(srcBuffer, kBufferSize, srcReadLen)) && 
					ASL::ResultSucceeded(destFile.Read(destBuffer, kBufferSize, destReadLen)) )
				{
					if ( srcReadLen != destReadLen )
					{
						result = kVerifyFileResult_SizeDifferent;
						break;
					}

					bool isEof = false;
					if ( srcReadLen < kBufferSize || destReadLen < kBufferSize )
					{
						isEof = true;
					}

					if (std::memcmp(srcBuffer, destBuffer, srcReadLen) != kMemcmpMatch)
					{
						result = kVerifyFileResult_ContentDifferent;
						break;
					}

					srcReadLen = 0;
					destReadLen = 0;

					if ( isEof )
					{
						break;
					}
				}
			}
		}
		catch (...)
		{
			DVA_TRACE("MZ.IngestVerification", 5, "Exception happens during reading file from " << 
				inSrc << " or " << inDest);
			result = kVerifyFileResult_FileBad;
		}

		srcFile.Close();
		destFile.Close();

		return result;
	}

	static bool CalculateFileMD5(
		const ASL::String& inPath,
		adobe::md5_t::digest_t& outDigit)
	{
		if ( inPath.empty() || !ASL::PathUtils::ExistsOnDisk(inPath))
		{
			DVA_ASSERT_MSG(0, DVA_STR("Invalid file to calc MD5"));
			return false;
		}

		// 4k
		const ASL::UInt32 kBufferSize = 4096;

		ASL::File file;
		try
		{
			if ( ASL::ResultSucceeded(
				ASL::File::Open(inPath, ASL::FileAccessFlags::kRead, file) ) )
			{
				adobe::md5_t digit;
				char buffer[kBufferSize];
				memset(buffer, 0, kBufferSize * sizeof(char));
				const ASL::UInt32 kToReadLen = kBufferSize;
				ASL::UInt32 len = 0;
				while (ASL::ResultSucceeded(file.Read(buffer, kToReadLen, len)))
				{
					if ( len <= 0 )
					{
						break;
					}
					digit.update(buffer, len);
					memset(buffer, 0, kBufferSize * sizeof(char));
					len = 0;
				}
				file.Close();
				outDigit = digit.final();
			}
		}
		catch (...)
		{
			file.Close();

			DVA_ASSERT_MSG(0, DVA_STR("Exception happen when calculating MD5."));
			return false;
		}
		return true;
	}

	static VerifyFileResult VerifyFileMD5(
		const ASL::String& inFirstPath, 
		const ASL::String& inSecondPath,
        adobe::md5_t::digest_t& outFirstMD5Value,
        adobe::md5_t::digest_t& outSecondMD5Value)
	{
		if ( inFirstPath.empty() || 
			 inSecondPath.empty() || 
			 !ASL::PathUtils::ExistsOnDisk(inFirstPath) || 
			 !ASL::PathUtils::ExistsOnDisk(inSecondPath) )
		{
			return kVerifyFileResult_FileNotExist;
		}

		if ( CalculateFileMD5(inFirstPath, outFirstMD5Value) && CalculateFileMD5(inSecondPath, outSecondMD5Value) )
		{
			return outFirstMD5Value == outSecondMD5Value ? kVerifyFileResult_Equal : kVerifyFileResult_MD5Diff;
		}

		return kVerifyFileResult_FileBad;
	}
      
    static ASL::String GetMD5HexString(const adobe::md5_t::digest_t& inMD5Value)
    {
 		static const char hexDigits[17] = "0123456789ABCDEF";
		char buf[33];
		for (std::size_t i = 0; i < 16; i++)
		{
			buf[i * 2]   = hexDigits[(inMD5Value[i] >> 4) & 0xF];
			buf[i * 2 + 1] = hexDigits[inMD5Value[i] & 0xF];
		}
		buf[32] = '\0';

        return ASL::MakeString(buf);
    }

	ASL::String MakeCompareDescription(
		const ASL::String& inSrc, 
		const ASL::String& inDest,
		const ASL::String& inExtraInfo)
	{
		dvacore::UTF16String msg = dvacore::ZString(
			"$$$/Prelude/MZ/IngestMedia/MakeCompareDescription=Src: @0 , Dest: @1, Description: @2");
		msg = dvacore::utility::ReplaceInString(msg, inSrc, inDest, inExtraInfo);
		return msg;
	}

	ASL::String GetVerifyOptionString(const VerifyOption& inOption)
	{
		switch (inOption)
		{
		case kVerify_FileSize:
			return dvacore::ZString("$$$/Prelude/MZ/IngestMedia/VerifyOptionFileSize=Verify Size - ");
			break;
		case kVerify_FileContent:
			return dvacore::ZString("$$$/Prelude/MZ/IngestMedia/VerifyOptionFileContent=Verify Content - ");
			break;
		//case kVerify_FolderStruct:
		//	return dvacore::ZString("$$$/Prelude/MZ/IngestMedia/VerifyOptionFileStructure=Verify folder structure ");
		//	break;
		case kVerify_FileMD5:
			return dvacore::ZString("$$$/Prelude/MZ/IngestMedia/VerifyOptionMD5=Verify MD5 - ");
			break;

		default:
			break;
		}

		return dvacore::ZString("$$$/Prelude/MZ/IngestMedia/VerifyOptionUnknown=Unknown verify option ");
	}

	ASL::String GetCompareResultString(VerifyFileResult inResult)
	{
		dvacore::UTF16String msg;
		msg = dvacore::ZString(
			"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringUnknown=Unknown");
		switch (inResult)
		{
		case kVerifyFileResult_Equal:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileEqual=File is same");
			break;
		case kVerifyFileResult_ContentDifferent:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileContentDifferent=File content is different");
			break;
		case kVerifyFileResult_FileBad:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileBad=File is bad");
			break;
		case kVerifyFileResult_SizeDifferent:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileSizeDifferent=File size is different");
			break;
		case kVerifyFileResult_CountDifferent:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileCountDifferent=The number of files is different");
			break;
		case kVerifyFileResult_NameDifferent:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileNameDifferent=File name is different");
			break;
		case kVerifyFileResult_SrcNotExist:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileSrcNotExist=Source file does not exists");
			break;
		case kVerifyFileResult_DestNotExist:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileDestNotExist=Destination file does not exists");
			break;
		case kVerifyFileResult_Conflict:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileConflict=File name conflicts");
			break;
		case kVerifyFileResult_Cancel:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkFileCancel=File comparison is canceled");
			break;
		case kVerifyFileResult_Folder:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkBothFolder=Both are folders");
			break;
		case kVerifyFileResult_MD5Diff:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringkMD5Diff=MD5 is different");
			break;
		case kVerifyFileResult_FileNotExist:
			msg = dvacore::ZString(
				"$$$/Prelude/MZ/IngestMedia/GetCompareResultStringFileNotExit=File does not exist");
			break;
		default:
			break;
		}
		return msg;
	}

#if ASL_TARGET_OS_WIN
	/*
	** This helps produce a more helpful string for the ASL_TRACE when
	** regurgitating a GetLastError.
	*/
	ASL::String ErrorString(
		DWORD err)
	{
		LPVOID lpMsgBuf = 0;
		DWORD formatErr = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR) &lpMsgBuf,
			0, NULL);
	
		if (formatErr == 0)
		{
			// Uh oh.
			return ASL::String();
		}
	
		size_t msgLen = ::lstrlen((LPCWSTR)lpMsgBuf);
	
		// Worst case is msgLen+14.
		// 11 (for "-2147483648") + 2 (for ": ") + msgLen + 1 (terminator).
		// The extra 6 is just paranoia.
		std::vector<wchar_t> buffer(msgLen + 20);
	
		::wsprintf(&buffer[0], L"%d: %s", err, lpMsgBuf); 
		::LocalFree(lpMsgBuf);
		return ASL::String(&buffer[0]);
	}
#endif

/*
** Used to determine whether or not to use ASL::File::Copy or our own homebrew IncrementalCopy.
*/
ASL::UInt32 const k1Kilobyte = 1024;
ASL::UInt64 const kSingleCopyThreshold = ASL::UInt64(8) * k1Kilobyte * k1Kilobyte; // 8 MB is a WAG.  Adjust manually after empirical testing.
ASL::UInt32 const kIncrementalCopySize = 1 * k1Kilobyte * k1Kilobyte; // 1 MB is a WAG.  Adjust manually after empirical testing.
ASL::UInt64 const kIncrementalCopyThreshold = ASL::UInt64(2) * k1Kilobyte * k1Kilobyte * k1Kilobyte; // 2 GB is a WAG.  Adjust manually after empirical testing.

#if ASL_TARGET_OS_MAC

/*
**
*/
static
::OSStatus MakeRef(
	ASL::String const& inFileName,
	::FSRef& outFSRef)
{
	dvacore::UTF8String const& filename = dvacore::utility::UTF16to8(inFileName);
	::UInt8 const* filenamePosix = reinterpret_cast< ::UInt8 const*>(filename.c_str());
	return ::FSPathMakeRef(filenamePosix, &outFSRef, NULL);
}

/*
**
*/
static
bool operator == (
	::HFSUniStr255 const& lhs,
	::HFSUniStr255 const& rhs)
{
	if (lhs.length != rhs.length)
	{
		return false;
	}

	int const kMemcmpMatch = 0;
	// [NOTE] Handles zero-length compares correctly as a kMemcmpMatch, as desired.
	return std::memcmp(lhs.unicode, rhs.unicode, lhs.length * sizeof lhs.unicode[0]) == kMemcmpMatch;
}

/*
**
*/
static
bool operator != (
	::HFSUniStr255 const& lhs,
	::HFSUniStr255 const& rhs)
{
	return !(lhs == rhs);
}

/*
** Fork copy failure is silently ignored.  The data is important, the forks less so.
*/
static
void CopyFork(
	::FSRef inSourceFSRef,
	::FSRef inDestinationFSRef,
	::HFSUniStr255 inForkName,
	::SInt64 inForkSize)
{
	if (inForkSize <= 0)
	{
		// Ummm... no.
		return;
	}

	if (inForkSize > ::SInt64(kSingleCopyThreshold))
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! Surprisingly large fork inForkSize:" << inForkSize << " exceeds kSingleCopyThreshold:" << kSingleCopyThreshold << ". Problem suspected. Copy skipped.");
		return;
	}

	::FSIORefNum fsSourceIORefNum;
	::OSErr err = ::FSOpenFork(&inSourceFSRef, inForkName.length, inForkName.unicode, ::fsRdPerm, &fsSourceIORefNum);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSOpenFork on source failed. err:" << err);
		return;
	}

	::FSIORefNum fsDestinationIORefNum;
	err = ::FSOpenFork(&inDestinationFSRef, inForkName.length, inForkName.unicode, ::fsWrPerm, &fsDestinationIORefNum);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSOpenFork on destination failed. err:" << err);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	typedef char byte;
	::ByteCount actualCount;
	std::vector<byte> buffer(static_cast<std::vector<byte>::size_type>(inForkSize));
	err = ::FSReadFork(fsSourceIORefNum, ::fsFromStart, 0, static_cast< ::ByteCount>(inForkSize), &buffer[0], &actualCount);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSReadFork on source failed. err:" << err);
		::FSCloseFork(fsDestinationIORefNum);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	if (actualCount != inForkSize)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSReadFork on source inForkSize:" << inForkSize << " does not match actualCount:" << actualCount);
		::FSCloseFork(fsDestinationIORefNum);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	err = ::FSAllocateFork(fsDestinationIORefNum, ::kFSAllocDefaultFlags , ::fsFromStart, 0, inForkSize, NULL);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSAllocateFork inForkSize:" << inForkSize << " on destination failed. err:" << err);
		::FSCloseFork(fsDestinationIORefNum);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	err = ::FSWriteFork(fsDestinationIORefNum, ::fsFromStart, 0, static_cast< ::ByteCount>(inForkSize), &buffer[0], &actualCount);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSWriteFork on destination failed. err:" << err);
		::FSCloseFork(fsDestinationIORefNum);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	if (actualCount != inForkSize)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSWriteFork on destination inForkSize:" << inForkSize << " does not match actualCount:" << actualCount);
		::FSCloseFork(fsDestinationIORefNum);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	err = ::FSCloseFork(fsDestinationIORefNum);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSCloseFork on destination failed. err:" << err);
		::FSCloseFork(fsSourceIORefNum);
		return;
	}

	err = ::FSCloseFork(fsSourceIORefNum);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSCloseFork on source failed. err:" << err);
		return;
	}
}

/*
** Copies forks (except for the data fork).
*/
static
void CopyForks(
	ASL::String const& inSource,
	ASL::String const& inDestination)
{
	::HFSUniStr255 rsrcName = { };
	::OSErr err = ::FSGetResourceForkName(&rsrcName);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetResourceForkName err:" << err);
		return;
	}

	::HFSUniStr255 dataName = { }; // This fork we skip copying.
	err = ::FSGetDataForkName(&dataName);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetDataForkName err:" << err);
		return;
	}

	::FSRef sourceFSRef = { };
	::OSStatus status = MakeRef(inSource, sourceFSRef);

	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on inSource:" << inSource << " failed. status:" << status);
		return;
	}

	::FSRef destinationFSRef = { };
	status = MakeRef(inDestination, destinationFSRef);

	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on inDestination:" << inDestination << " failed. status:" << status);
		return;
	}

	::CatPositionRec catPositionRec = { };
	::HFSUniStr255 forkName = { };
	::SInt64 forkSize = 0;
	::UInt64 forkPhysicalSize = 0;

	do
	{
		err = ::FSIterateForks(&sourceFSRef, &catPositionRec, &forkName, &forkSize, &forkPhysicalSize);

		if (err == ::noErr
			&& forkSize > 0)
		{
			if (forkName == rsrcName)
			{
				CopyFork(sourceFSRef, destinationFSRef, forkName, forkSize);
			}
			else if (forkName != dataName)
			{
				// Apple does have some support for named forks.  But as of
				// 10.4 and 10.5, it is rather de-emphasized (understatement!).
				ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Unusual. A non-rsrc / non-data fork in source:" << inSource);
				CopyFork(sourceFSRef, destinationFSRef, forkName, forkSize);
			}
		}
	}
	while (err == ::noErr);
}

UInt16 GetPermissionMode(ASL::String const& inPath)
{
	::UInt16 mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	::FSRef sourceRef = { };
	::OSStatus status = MakeRef(inPath, sourceRef);
		
	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on source:" << inPath << " failed. status:" << status);
		return mode;
	}
		
	::FSCatalogInfoBitmap whichInfo = ::kFSCatInfoGettableInfo & ::kFSCatInfoSettableInfo;
	::FSCatalogInfo catalogInfo = { };
	::HFSUniStr255* ignoreName = NULL;
	::FSSpecPtr ignoreFSSpec = NULL;
	::FSRef* ignoreParent = NULL;
	::OSErr err = ::FSGetCatalogInfo(&sourceRef, whichInfo, &catalogInfo, ignoreName, ignoreFSSpec, ignoreParent);
		
	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetCatlogInfo on source:" << inPath << " failed. err:" << err);
		return mode;
	}
		
#ifdef __LP64__
	mode = catalogInfo.permissions.mode;
#else
	::FSPermissionInfo* permissions = static_cast< ::FSPermissionInfo*>(&catalogInfo.permissions[0]);
	mode = permissions->mode;
#endif
	return mode;
}
		
/*
**
*/
void SetPermissionMode(ASL::String const& inPath, ::UInt16 inMode)
{
	::FSRef fsref = { };
	::OSStatus status = MakeRef(inPath, fsref);
		
	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on destination:" << inPath << " failed. status:" << status);
		return;
	}
		
	::FSCatalogInfoBitmap whichInfo = ::kFSCatInfoGettableInfo & ::kFSCatInfoSettableInfo;
	::FSCatalogInfo catalogInfo = { };
	::HFSUniStr255* ignoreName = NULL;
	::FSSpecPtr ignoreFSSpec = NULL;
	::FSRef* ignoreParent = NULL;
	::OSErr err = ::FSGetCatalogInfo(&fsref, whichInfo, &catalogInfo, ignoreName, ignoreFSSpec, ignoreParent);
		
	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetCatlogInfo on source:" << inPath << " failed. err:" << err);
		return;
	}
		
#ifdef __LP64__
	catalogInfo.permissions.mode = inMode;
#else
	::FSPermissionInfo* permissions = static_cast< ::FSPermissionInfo*>(&catalogInfo.permissions[0]);
	permissions->mode = inMode;
#endif
	err = ::FSSetCatalogInfo(&fsref, whichInfo, &catalogInfo);
		
	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetCatlogInfo on destination:" << inPath << " failed. err:" << err);
		return;
	}
}
		
/*
**
*/
::UInt16 GetParentPermissionMode(ASL::String const& inPath)
{
	ASL::String drive, parentDir;
		
	//Trick, remove the last sub directory
	ASL::PathUtils::SplitPath(
								ASL::PathUtils::RemoveTrailingSlash(inPath),
								&drive,
								&parentDir,
								0,
								0);
	
	ASL::String const& parentDirFullPath = drive + parentDir;
	return GetPermissionMode(parentDirFullPath);
}
#endif

class ShowIngestMessageBoxInMainThread
{
public:
	ShowIngestMessageBoxInMainThread(
		IG_MessageBox::IngestButton& outReturnButton,
		const ASL::String& inWarningTitle,
		const ASL::String& inWarningMessage,
		const ASL::String& inCheckText,
		bool& inIsChecked,
		const IG_MessageBox::IngestButtons& inButtons)
		: mReturnButton(outReturnButton)
		, mWarningTitle(inWarningTitle)
		, mWarningMessage(inWarningMessage)
		, mCheckText(inCheckText)
		, mIsChecked(inIsChecked)
		, mButtons(inButtons)
	{
	}

	void Process()
	{
		dvaworkspace::workspace::Workspace* dvaWorkspace = UIF::Workspace::Instance()->GetPtr();
		mReturnButton = IngestMessageBox::ShowNoteWithCheckbox(
			dvaWorkspace->GetDrawSupplier(), 
			mWarningTitle, 
			mWarningMessage,
			mCheckText,
			mIsChecked,
			dvaWorkspace->GetMainTopLevelWindow(),
			mButtons,
			IG_MessageBox::kIngestButton_Ignore,
			IG_MessageBox::kIngestIcon_Caution);
	}

private:
	IG_MessageBox::IngestButton&		mReturnButton;
	const ASL::String&					mWarningTitle;
	const ASL::String&					mWarningMessage;
	const ASL::String&					mCheckText;
	bool&								mIsChecked;
	const IG_MessageBox::IngestButtons&	mButtons;
};

} // anonymous namespace

namespace IngestUtils
{
/*
**
*/
static
ASL::String combineErrorMessage(
								ASL::String const& errMsg,
								ASL::String const& suggetMsg,
								ASL::String const& errFilePath = ASL_STR(""))
{
	ASL::String combineMsg = errMsg;
	combineMsg += dvacore::utility::GetLineEnding(dvacore::utility::LineEndingType_THIS_ONE);
	
	if (!errFilePath.empty())
	{
		combineMsg += ASL_STR("\"") + errFilePath + ASL_STR("\"");
		combineMsg += dvacore::utility::GetLineEnding(dvacore::utility::LineEndingType_THIS_ONE);
	}
	combineMsg += dvacore::utility::GetLineEnding(dvacore::utility::LineEndingType_THIS_ONE);
	combineMsg += suggetMsg;

	return combineMsg;
}

/*
** Unicode SPACE characters (including TAB).
*/
static
bool IsUniSpace(ASL::String::value_type ch)
{
	return ch == ' '
		|| ch == '\t'
		|| ch == 0x00A0
		|| ch == 0x2000
		|| ch == 0x2001
		|| ch == 0x2002
		|| ch == 0x2003
		|| ch == 0x2004
		|| ch == 0x2005
		|| ch == 0x2006
		|| ch == 0x2007
		|| ch == 0x2008
		|| ch == 0x2009
		|| ch == 0x200A
		|| ch == 0x200B
		|| ch == 0x202F
		|| ch == 0x205F
		|| ch == 0x3000;
}

/*
**
*/
static
bool IsFolder1ContainFolder2(
					  ASL::String const& inFolder1,
					  ASL::String const& inFolder2)
{
	ASL::String folder1, folder2;

#if ASL_TARGET_OS_MAC
	folder1 = MZ::Utilities::AddBootVolumePath(inFolder1);
	folder2 = MZ::Utilities::AddBootVolumePath(inFolder2);
#endif	
	
	folder1 = ASL::PathUtils::ToNormalizedPath(inFolder1);
	folder2 = ASL::PathUtils::ToNormalizedPath(inFolder2);
	
	return folder1 == folder2.substr(0, folder1.length());
}

/*
**
*/
static
bool Folder1IsChildOfFolder2(
						ASL::String const& inFolder1,
						ASL::String const& inFolder2)
{
	ASL::String folder1, folder2;
	
#if ASL_TARGET_OS_MAC
	folder1 = MZ::Utilities::AddBootVolumePath(inFolder1);
	folder2 = MZ::Utilities::AddBootVolumePath(inFolder2);
#else
	folder1 = inFolder1;
	folder2 = inFolder2;
#endif	

	ASL::String folder1Drive, folder1Dir;
	ASL::String folder1NoSlash = ASL::PathUtils::ToNormalizedPath(ASL::PathUtils::RemoveTrailingSlash(folder1));

	//Trick, remove the last sub directory
	ASL::PathUtils::SplitPath(
				folder1NoSlash,
				&folder1Drive,
				&folder1Dir,
				0,
				0);

	return (folder1Drive + folder1Dir) == ASL::PathUtils::ToNormalizedPath(ASL::PathUtils::AddTrailingSlash(folder2));
}

#if ASL_TARGET_OS_WIN
static ASL::String GetUniversalNameWin(const ASL::String& inFilePath)
{
	// Double check if the destination folder is a mapped driver
	// If yes, need get the universal name and compare it with source folder
	std::wstring winPath( dvacore::utility::UTF16ToWstring(inFilePath) );

	DWORD dwRetVal;

	WCHAR Buffer[1024];
	DWORD dwBufferLength = 1024;

	UNIVERSAL_NAME_INFO* unameinfo;

	unameinfo = (UNIVERSAL_NAME_INFO *)&Buffer;
	dwRetVal = WNetGetUniversalName(
		winPath.c_str(), 
		UNIVERSAL_NAME_INFO_LEVEL, 
		(LPVOID) unameinfo, 
		&dwBufferLength );

	// If getting Universal Name is successful, use the universal name.
	// Otherwise, return the original local path.
	if (dwRetVal == NO_ERROR && NULL != unameinfo) 
	{
		return dvacore::utility::WcharToUTF16(unameinfo->lpUniversalName);
	}
	return inFilePath;
}
#endif


#if ASL_TARGET_OS_MAC
static ASL::String GetUniversalNameMac(const ASL::String& inFilePath)
{
	dvacore::filesupport::File fileHandle = dvacore::filesupport::File(inFilePath);
	::FSRef resolvedFileRef;
	if (ASL::Mac::PathUtils::ConvertASLStringToFSRef(fileHandle.FullPath(), resolvedFileRef))
	{
		Boolean targetIsFolder;
		Boolean wasAlias;
		OSErr err = FSResolveAliasFile(&resolvedFileRef, true, &targetIsFolder, &wasAlias);
		const std::size_t len = 1024;
		std::vector<char> pathArray(len);
		::OSStatus status = ::FSRefMakePath(&resolvedFileRef, reinterpret_cast<UInt8*>(&(pathArray[0])), len - 1);
		if (status == ::noErr)
		{
			return dvacore::utility::AsciiToUTF16(&(pathArray[0]));
		}
	}		
	return inFilePath;
}
#endif

/**
**
*/
bool IsDestAndSrcSame(const dvacore::UTF16String& inSrcFolder, 
					  const ASL::PathnameList& inDestFolders) 
{
	bool isSame = false;
	ASL::PathnameList::const_iterator it;

	for (it = inDestFolders.begin(); it != inDestFolders.end(); ++it)
	{
#if ASL_TARGET_OS_WIN
		if ( inSrcFolder == GetUniversalNameWin(*it) )
#elif ASL_TARGET_OS_MAC
		if ( GetUniversalNameMac(inSrcFolder) == (*it) )
#endif
		{
			isSame = true;
			break;
		}
	}
	return isSame;
}

/**
**
*/
bool IsDestAndSrcSame(const dvacore::UTF16String& inSrcPath, 
	const dvacore::UTF16String& inDestPath)
{
	bool isSame = false;

#if ASL_TARGET_OS_WIN
	if ( dvacore::utility::CaseInsensitive::StringEquals(GetUniversalNameWin(inSrcPath),GetUniversalNameWin(inDestPath)) )
#elif ASL_TARGET_OS_MAC
	if ( dvacore::utility::CaseInsensitive::StringEquals(GetUniversalNameMac(inSrcPath),GetUniversalNameMac(inDestPath)) )
#endif
	{
		isSame = true;
	}

	return isSame;
}

bool FolderContain(
	const ASL::String& inParentPath,
	const ASL::String& inChildPath,
	bool inAllowSame)
{
#if ASL_TARGET_OS_MAC
	ASL::String parentPath = MZ::Utilities::AddBootVolumePath(inParentPath);
	ASL::String childPath = MZ::Utilities::AddBootVolumePath(inChildPath);
#else
	ASL::String parentPath(inParentPath);
	ASL::String childPath(inChildPath);
#endif

	ASL::String relativePath;
	bool isRelative = MZ::Utilities::ArePathsRelative(parentPath, true, childPath, true, relativePath);
	if (isRelative && !ASL::StringContains(relativePath, ASL_STR("..")))
	{
		return inAllowSame || relativePath != ASL_STR(".");
	}
	else
	{
		return false;
	}
}

/**
**
*/
bool IsValidDestination(ASL::String const& inDestination, ASL::String& outErrorMsg)
{
	using namespace dvacore::config;

	if (inDestination.empty() || !ASL::PathUtils::ExistsOnDisk(inDestination))
	{
		ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/DestinationIsNotExist=The specified Destination does not exist.");
		ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SpecifyDest=Please specify a new \"Destination\".");
		outErrorMsg = combineErrorMessage(errMsg, suggestMsg, inDestination);
		return false;
	}
	
	if (ASL::PathUtils::IsReadOnly(inDestination))
	{
		ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/DestinationIsLocked=The specified Destination is locked.");
		ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/UnLockOrRedirect=Please unlock the specified Destination or specify a new \"Destination\".");
		outErrorMsg = combineErrorMessage(errMsg, suggestMsg, inDestination);
		return false;
	}
	
	return true;
}


/**
**
*/
CheckPathResult CheckTranferPaths(
		const ASL::PathnameList& inSourceFiles,
		const ASL::String& inPrimaryDestination,
		const ASL::PathnameList& inBackupDestinations,
		ASL::String& outErrorMsg)
{
	using namespace dvacore::config;

	if (inSourceFiles.empty())
	{
		ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/NoSelectSourceFiles=No selected Source File.");
		ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SelectSourceFile=Please specify a \"Source File\".");
		outErrorMsg = combineErrorMessage(errMsg, suggestMsg);
		return kFailed;
	}
	
	if(!IsValidDestination(inPrimaryDestination, outErrorMsg))
	{
		return kFailed;
	}

	std::uint64_t sourceSpaceNeeded = 0;
	for(ASL::PathnameList::const_iterator sourceItr = inSourceFiles.begin(); sourceItr != inSourceFiles.end(); ++sourceItr)
	{
		ASL::UInt64 outSize = 0;
		ASL::File::SizeOnDisk(*sourceItr, outSize);
		sourceSpaceNeeded += outSize;
	}

	bool hasEnoughSpace = true;
	std::uint64_t dstSize = 0;

	ASL::Result result = MZ::Utilities::GetDiskFreeSpace(inPrimaryDestination, dstSize);
	if ( !ASL::ResultSucceeded(result) || dstSize < sourceSpaceNeeded )
	{
		hasEnoughSpace = false;
	}

	for(ASL::PathnameList::const_iterator f = inBackupDestinations.begin(), l = inBackupDestinations.end(); f != l; ++f)
	{
		if(!IsValidDestination(*f, outErrorMsg))
		{
			return kFailed;
		}

		dstSize = 0;
		result = MZ::Utilities::GetDiskFreeSpace(*f, dstSize);

		if (!ASL::ResultSucceeded(result) || dstSize < sourceSpaceNeeded)
		{
			hasEnoughSpace = false;
		}
		
		if(std::find (f + 1, l, *f) != l)
		{
			ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/DuplicateBackup=Cannot transfer file to duplicate Backup Destinations.");
			ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SpecifyBackupDestination=Please specify new \"Backup Destinations\".");
			outErrorMsg = combineErrorMessage(errMsg, suggestMsg, *f);
			return kFailed;
		}

		if(inPrimaryDestination == *f)
		{
			ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/SamePrimaryBackup=The specified Primary Destination is same as Backup Destination.");
			ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SpecifyPrimaryBackupDestination=Please specify new \"Destinations\".");
			outErrorMsg = combineErrorMessage(errMsg, suggestMsg, inPrimaryDestination);
			return kFailed;
		}
	}

	for(ASL::PathnameList::const_iterator f = inSourceFiles.begin(), l = inSourceFiles.end(); f != l; ++f)
	{
		ASL::String const& sourcePath = ASL::PathUtils::AddTrailingSlash(*f);
		
		if(Folder1IsChildOfFolder2(sourcePath, inPrimaryDestination))
		{
			ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/CopyToParent=The specified Primary Destination is same as Source Destination.");
			ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SpecifyParentDestination=Please specify a new \"Primary Destination\".");
			outErrorMsg = combineErrorMessage(errMsg, suggestMsg, inPrimaryDestination);
			return kFailed;
		}
		
		if (ASL::PathUtils::IsDirectory(*f))
		{			
			if(IsFolder1ContainFolder2(sourcePath, inPrimaryDestination))
			{
				ASL::String const& errMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Failure/CopyToChild=The specified Primary Destination cannot be the Subfolder of Source Destination.");
				ASL::String const& suggestMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Frontend/IngestDialog/Failure/SpecifyChildDestination=Please specify a new \"Primary Destination\".");
				outErrorMsg = combineErrorMessage(errMsg, suggestMsg, inPrimaryDestination);
				return kFailed;
			}
		}
	}

	CheckPathResult checkResult = kSuccess;
	for (ASL::PathnameList::const_iterator f = inBackupDestinations.begin(), l = inBackupDestinations.end(); f != l; ++f)
	{
		if (IsFolder1ContainFolder2(inPrimaryDestination, *f))
		{
			ASL::String const& errorMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/PrimaryContainsBackup=Backup Destination \"@0\" is in Primary Destination.\n");
			outErrorMsg += dvacore::utility::ReplaceInString(errorMsg, *f);
			checkResult = kNeedUserConfirm;
		}
		else if (IsFolder1ContainFolder2(*f, inPrimaryDestination))
		{
			ASL::String const& errorMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/BackupContainsPrimary=Primary Destination is in Backup Destination \"@0\".\n");
			outErrorMsg += dvacore::utility::ReplaceInString(errorMsg, *f);
			checkResult = kNeedUserConfirm;
		}

		for (ASL::PathnameList::const_iterator f2 = f + 1, l2 = inBackupDestinations.end(); f2 != l2; ++f2)
		{
			if (IsFolder1ContainFolder2(*f, *f2))
			{
				ASL::String const& errorMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/BackupContainsBackup=Backup Destination \"@0\" is in Backup Destination \"@1\".\n");
				outErrorMsg += dvacore::utility::ReplaceInString(errorMsg, *f2, *f);
				checkResult = kNeedUserConfirm;
			}
			else if (IsFolder1ContainFolder2(*f2, *f))
			{
				ASL::String const& errorMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/BackupContainsBackup=Backup Destination \"@0\" is in Backup Destination \"@1\".\n");
				outErrorMsg += dvacore::utility::ReplaceInString(errorMsg, *f, *f2);
				checkResult = kNeedUserConfirm;
			}
		}
	}

	if (!hasEnoughSpace)
	{
		checkResult = kNeedUserConfirm;
		outErrorMsg = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/NotEnoughSpaceWarning=\nYou may not have enough disk space to complete this transfer, do you want to continue?");
	}
	else if(checkResult == kNeedUserConfirm)
	{
		outErrorMsg += Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/WantToContinue=\nSuch setting maybe unsafe for your data. Do you want to continue?");
	}

	return checkResult;
}

/**
**
*/
ASL::Result MakeDestinationThrSourceStem(
	ASL::String const& inSource, 
	ASL::String const& inSourceStem, 
	ASL::String const& inDestination,
	ASL::String const& inExtraSubDestFolder,
	ASL::String& outNewDestination)
{
	// [TRICKY] If the inSource is a directory, the functions in ASL::PathUtils will look 
	// the last sub-directory as a file due to the trailing slash was removed.	
	ASL::String const& tempSource = ASL::PathUtils::RemoveTrailingSlash(inSource);
	ASL::String const& tempDrive = ASL::PathUtils::GetDrivePart(tempSource);
	ASL::String const& tempSourceDir = ASL::PathUtils::GetDirectoryPart(tempSource);
	
	ASL::String const& sourceStem = ASL::PathUtils::RemoveTrailingSlash(inSourceStem);
	if (sourceStem != ASL::MakeString(PLMBC::kVolumeListPath))
	{
		ASL::String const& tempPath = tempDrive + tempSourceDir;

		if(sourceStem.length() <= tempPath.length())
		{
			if (inExtraSubDestFolder.empty())
			{
				outNewDestination = ASL::PathUtils::AddTrailingSlash(
											ASL::PathUtils::RemoveTrailingSlash(inDestination) + 
											tempPath.substr(sourceStem.length(), tempPath.length() - sourceStem.length()));
			}
			else
			{
				outNewDestination = ASL::PathUtils::AddTrailingSlash(
											ASL::PathUtils::AddTrailingSlash(inDestination) + 
											inExtraSubDestFolder +
											tempPath.substr(sourceStem.length(), tempPath.length() - sourceStem.length()));
			}
			return ASL::kSuccess;
		}
	}
	else // transfer a spanned card logical clip
	{
		ASL::String const& driveLetter = tempDrive.substr(0, 1);
		if (inExtraSubDestFolder.empty())
		{
			outNewDestination = ASL::PathUtils::AddTrailingSlash(
											ASL::PathUtils::AddTrailingSlash(inDestination) + 
											driveLetter + 
											tempSourceDir);
		}
		else
		{
			outNewDestination = ASL::PathUtils::AddTrailingSlash(
											ASL::PathUtils::AddTrailingSlash(inDestination) + 
											ASL::PathUtils::AddTrailingSlash(inExtraSubDestFolder) +
											driveLetter + 
											tempSourceDir);
		}
		return ASL::kSuccess;
	}
	
	return ASL::ePathNotFound;
}

/**
**
*/
ASL::Result MakeDestinationThrSourceStem(
	ASL::String const& inSource, 
	ASL::String const& inSourceStem, 
	ASL::PathnameList const& inDestination,
	ASL::String const& inExtraSubDestFolder,
	ASL::PathnameList& outNewDestination)
{
	ASL::Result result = ASL::kSuccess;
	for(ASL::PathnameList::const_iterator f = inDestination.begin(), l = inDestination.end(); f != l; ++f)
	{
		ASL::String tempDest;
		result = MakeDestinationThrSourceStem(inSource, inSourceStem, *f, inExtraSubDestFolder, tempDest);
		if(result != ASL::kSuccess)
			return result;
		outNewDestination.push_back(tempDest);
	}
	return result;
}

/**
**
*/
ASL::Result GetContainedFileAndDirectoryPaths(ASL::String const& inDirectory, ASL::PathnameList& outPathList)
{
	try {
		ASL::PathnameList pathList;
		ASL::DirectoryList dirList;
		ASL::Directory directory = ASL::Directory::ConstructDirectory(inDirectory);
		directory.GetContainedFileAndDirectoryPaths(pathList, dirList, true);
		for(ASL::DirectoryList::const_iterator f = dirList.begin(), l = dirList.end(); f != l; ++f)
		{
			outPathList.push_back(f->AsString());
		}
		outPathList.insert(outPathList.end(), pathList.begin(), pathList.end());
		return ASL::kSuccess;
	}
	catch (ASL::eNotADirectoryPath&)
	{
		return ASL::ePathNotFound;
	}
	catch (ASL::eDirectoryNoLongerValid&)
	{
		return ASL::ePathNotFound;
	}

}

/**
**
*/
//VerifyFileResult CompareTransferedFiles(
//	IIngestJobItemRef inIngestJobItem,
//	BE::IProgressRef inProgress,
//	std::list<ASL::String>& outCompareReport)
//{
//	StatusHandlerSharePtr handler(new StatusHandler());
//	DVA_ASSERT(inProgress != NULL);
//	handler->RegisterProgressHandler(inProgress);
//
//	Engine verifyEngine(inIngestJobItem->GetVerifyOption(), handler);
//	verifyEngine.Intialize();
//
//	ASL::String sourceStem;
//	if( inIngestJobItem->GetSourceStem() != ASL::MakeString(PLMBC::kVolumeListPath))
//	{
//		sourceStem = inIngestJobItem->GetSourceStem();
//	}
//
//	ASL::String primaryDestination = inIngestJobItem->GetPrimaryDestination();
//	ASL::String const& extraSubDestFolder = inIngestJobItem->GetExtraSubDestFolder();
//	if (extraSubDestFolder.length() > 0)
//	{
//		primaryDestination = ASL::PathUtils::AddTrailingSlash(primaryDestination) + extraSubDestFolder;
//	}
//
//	CopiedElementList const& copiedElementList = inIngestJobItem->GetCopiedElementList();
//
//	return verifyEngine.StartProcess(
//				ASL::PathUtils::AddTrailingSlash(sourceStem),
//				ASL::PathUtils::AddTrailingSlash(primaryDestination),
//				copiedElementList,
//				outCompareReport);
//}

/*
** The ASL::DateFormat works in wchar_t land.
** Support routine for DoCopy.
*/
ASL::String DateFormatter(
	wchar_t const* inFormat,
	ASL::Date const& inDate)
{
    DVA_ASSERT(inFormat != NULL);
	std::wostringstream stream;
	stream.imbue(ASL::Locale::Native());
	stream << ASL::DateFormat(inDate, inFormat);
	std::wstring const& outputStr = stream.str();

	// [TODO] Move dvacore::utility::WcharToUTF16 out of "ASLStringFwd.h" and
	// into "dvacore/utility/UnicodeUtilities.h".  Note that the namespace
	// in ASLStringFwd.h is dvacore::utility, anticipating the future.
	// [NOTE] Discuss on Drover-Dev first.  Loose end from Buffy era.  Concerns
	// expressed in the drover dev meeting way-back-when were:
	// + We should promote UTF16String usage, not wstring
	// + What's the wstring encoding?
	//   For 2-byte wchar_t, we don't KNOW that wstring is UTF-16
	//   For 4-byte wchar_t, we don't KNOW that wstring is UTF-32
	// + Using wstring to interop with an edge is an edge-thing, and that
	//   conversion utility function should be in the edge-thingy utilities, not
	//   in Drover with presumptions of UTF-16 (or UTF-32).

	return dvacore::utility::WcharToUTF16(outputStr.c_str());
}

/*
*	Create a Date-string with format "YYYY-DD-MMTHH:MM:SSZ"	
*/
ASL::String CreateDateString(
	std::tm timeStruct)
{
	ASL::String timeString = dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_year + 1900)) + ASL::MakeString("-");
	if (timeStruct.tm_mon < 9)
	{
		timeString += ASL::MakeString("0");
	}
	timeString += dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_mon + 1)) + ASL::MakeString("-");
	if (timeStruct.tm_mday < 10)
	{
		timeString += ASL::MakeString("0");
	}
	timeString += dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_mday)) +  ASL::MakeString("T");
	if (timeStruct.tm_hour < 10)
	{
		timeString += ASL::MakeString("0");
	}
	timeString += dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_hour)) + ASL::MakeString(":");
	if (timeStruct.tm_min < 10)
	{
		timeString += ASL::MakeString("0");
	}
	timeString += dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_min)) + ASL::MakeString(":");
	if (timeStruct.tm_sec < 10)
	{
		timeString += ASL::MakeString("0");
	}
	timeString += dvacore::utility::NumberToString(std::int64_t(timeStruct.tm_sec)) + ASL::MakeString("Z");

	return timeString;
}


/**
**
*/
bool IsValidPathName(const ASL::String& inPath)
{
	static const ASL::String kIllegalPathCharacters(ASL_STR(":*?\"<>|")); // These are not correct for OS X.
	static const ASL::String::size_type kMaxPathLength = 256;

	if (inPath.empty())
	{
		return false;
	}

	if (inPath.size() > kMaxPathLength)
	{
		return false;
	}

	ASL::String::size_type position(inPath.find_first_of(kIllegalPathCharacters));
	if (position != ASL::String::npos)
	{
		return false;
	}

	//	No invalid character found
	ASL::String::size_type size = inPath.size();
	//	Make sure there is at least one non-blank character.
	for (ASL::String::size_type index = 0; index < size; ++index)
	{
		if (!IsUniSpace(inPath[index]))
		{
			return true;
		}
	}

	return false;
}
	
/**
**
*/
bool IsValidString(const ASL::String& inString, const ASL::String & inIllegalCharaters)
{
	if (inString.empty())
	{
		return true;
	}
	
	ASL::String::size_type position(inString.find_first_of(inIllegalCharaters));
	if (position != ASL::String::npos)
	{
		return false;
	}
	
	return true;
}

/**
**
*/
bool IsLinkFile(ASL::String const& inPath)
{
	return IsLinkFile(ASL::MakeStdString(inPath));
}

/**
**
*/
bool IsLinkFile(std::string const& inPath)
{
#if ASL_TARGET_OS_WIN
	ATL::CComPtr<IShellLink> ipShellLink;
	HRESULT hRes = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&ipShellLink);

	if (FAILED(hRes))
	{
		return false;
	}

	ATL::CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);
	std::vector<wchar_t> wBuffer(32768); // Maximum UNC path length plus 1 for NUL terminator.
	MultiByteToWideChar(CP_UTF8, 0, inPath.c_str(), -1, &wBuffer[0], static_cast<int>(wBuffer.size()));
	hRes = ipPersistFile->Load(&wBuffer[0], STGM_READ);

	if (FAILED(hRes))
	{
		return false;
	}

	std::vector<wchar_t> wLinkBuffer(32768); // Maximum UNC path length, plus 1 for NUL terminator.
	hRes = ipShellLink->GetPath(&wLinkBuffer[0], static_cast<int>(wLinkBuffer.size()), NULL, SLGP_UNCPRIORITY);

	if (hRes == S_OK)
	{
		return true;
	}

	return false;
#else
	::FSRef ref;
	::OSStatus err = ::FSPathMakeRef((UInt8*)inPath.c_str(), &ref, NULL);
	if (err != ::noErr)
		return false;

	::Boolean aliasFileFlag, folderFlag;
	err = ::FSIsAliasFile (&ref, &aliasFileFlag, &folderFlag);
	if (err == ::noErr && aliasFileFlag == TRUE)
		return true;

	return IsSymbolicLink(inPath);

#endif

}

/**
**
*/
bool IsSymbolicLink(std::string const& inPath)
{
#if ASL_TARGET_OS_MAC
	struct stat buf;
	int err = lstat(inPath.c_str(), &buf);
	return (err == 0 && S_ISLNK(buf.st_mode));
#else
	return false;
#endif
}
	
/**
**
*/
bool IsSymbolicLink(ASL::String const& inPath)
{
	return IsSymbolicLink(ASL::MakeStdString(inPath));
}

ASL::String MakeAutoRenamedPath(const ASL::String& inPath)
{
	char postfix[4] = {'-', '\0', '\0', '\0'};
	ASL::String drive, directory, file, ext;

	ASL::PathUtils::SplitPath(inPath, &drive, &directory, &file, &ext);
	ASL::String fullPathWithoutExt = drive + directory + file;

	for (char i = 'a'; i <= 'z'; ++i)
	{
		postfix[1] = i;			
		ASL::String const& newDestFile = fullPathWithoutExt + dvacore::utility::AsciiToUTF16(postfix) + ext;
		if (newDestFile.empty() || !ASL::PathUtils::ExistsOnDisk(newDestFile))
		{
			return newDestFile;
		}
	}

	for (char i = 'a'; i <= 'z'; ++i)
	{
		postfix[1] = i;			
		for (char j = '0'; j <= '9'; ++j)
		{
			postfix[2] = j;			
			ASL::String const& newDestFile = fullPathWithoutExt + dvacore::utility::AsciiToUTF16(postfix) + ext;
			if (newDestFile.empty() || !ASL::PathUtils::ExistsOnDisk(newDestFile))
			{
				return newDestFile;
			}
		}
	}

	// If don't get one, use GUID to generate unique path
	return ASL::File::MakeUniqueTempPath(inPath);
}

bool PathsExist(const ASL::PathnameList& inPaths, ASL::String& outNotExistPath)
{
	BOOST_FOREACH (const ASL::String& path, inPaths)
	{
		if (path.empty() || !ASL::PathUtils::ExistsOnDisk(path))
		{
			outNotExistPath = path;
			return false;
		}
	}
	return true;
}

bool PathsAreWritable(const ASL::PathnameList& inPaths, ASL::String& outNotWritablePath)
{
	BOOST_FOREACH (const ASL::String& path, inPaths)
	{
		if (path.empty() || !ASL::PathUtils::ExistsOnDisk(path) || ASL::PathUtils::IsReadOnly(path))
		{
			outNotWritablePath = path;
			return false;
		}
	}
	return true;
}

bool NoDuplicatedPaths(const ASL::PathnameList& inPaths, ASL::String& outDuplicatedPath)
{
	for (size_t i = 0, count = inPaths.size(); i < count; ++i)
	{
		for (size_t j = i + 1; j < count; ++j)
		{
			if (PL::IngestUtils::IsDestAndSrcSame(inPaths[i], inPaths[j]))
			{
				outDuplicatedPath = inPaths[i];
				return false;
			}
		}
	}
	return true;
}

bool EnoughSpaceForCopy(const ASL::PathnameList& inSources, const ASL::PathnameList& inDestinations, ASL::String& outDstPath, ASL::UInt64& outNeededSize)
{
	ASL::UInt64 sourceSpaceNeeded = 0;
	ASL::UInt64 outSize = 0;
	BOOST_FOREACH (const ASL::String& path, inSources)
	{
		if (!ASL::PathUtils::IsDirectory(path))
		{
			ASL::File::SizeOnDisk(path, outSize);
			sourceSpaceNeeded += outSize;
		}
		else
		{
			ASL::PathnameList pathList;
			ASL::Directory sourceDirectory = ASL::Directory::ConstructDirectory(path);
			sourceDirectory.GetContainedFilePaths(pathList, true);
			BOOST_FOREACH (const dvacore::UTF16String& filepath, pathList)
			{
				ASL::File::SizeOnDisk(filepath, outSize);
				sourceSpaceNeeded += outSize;
			}
		}
	}

	BOOST_FOREACH (const ASL::String& path, inDestinations)
	{
		std::uint64_t dstSize = 0;
		ASL::Result result = MZ::Utilities::GetDiskFreeSpace(path, dstSize);

		if ( !ASL::ResultSucceeded(result) || dstSize < sourceSpaceNeeded )
		{
			outNeededSize = sourceSpaceNeeded - dstSize;
			outDstPath = path;
			return false;
		}
	}

	return true;
}

void StampPathEntryInformation(
	ASL::String const& inSource,
	ASL::String const& inDestination)
{
#if ASL_TARGET_OS_WIN

	WIN32_FILE_ATTRIBUTE_DATA fileAttr = { };
	BOOL success = ::GetFileAttributesExW(inSource.c_str(), GetFileExInfoStandard, &fileAttr);

	if (success == FALSE)
	{
		DWORD err = ::GetLastError();
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! GetFileAttributesExW err:" << err << "(" << ErrorString(err) << ")");
		return;
	}

	// W#1785225 - we want the copied file to be read-write-able.
	// Here we mask out the READONLY attribute.  (We're not worried about r/w permission bits.)
	fileAttr.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
	fileAttr.dwFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;

	::SetFileAttributesW(inDestination.c_str(), fileAttr.dwFileAttributes);

	if (success == FALSE)
	{
		DWORD err = GetLastError();
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! SetFileAttributesW err:" << err << "(" << ErrorString(err) << ")");
		return;
	}

#elif ASL_TARGET_OS_MAC

	::UInt16 parentPermissionMode = GetParentPermissionMode(inDestination);
	parentPermissionMode |= S_IWUSR;

	::FSRef sourceRef = { };
	::OSStatus status = MakeRef(inSource, sourceRef);

	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on source:" << inSource << " failed. status:" << status);
		return;
	}

	::FSRef destinationRef = { };
	status = MakeRef(inDestination, destinationRef);

	if (status != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! MakeRef on destination:" << inDestination << " failed. status:" << status);
		return;
	}

	::FSCatalogInfoBitmap whichInfo = ::kFSCatInfoGettableInfo & ::kFSCatInfoSettableInfo;
	::FSCatalogInfo catalogInfo = { };
	::HFSUniStr255* ignoreName = NULL;
	::FSSpecPtr ignoreFSSpec = NULL;
	::FSRef* ignoreParent = NULL;
	::OSErr err = ::FSGetCatalogInfo(&sourceRef, whichInfo, &catalogInfo, ignoreName, ignoreFSSpec, ignoreParent);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetCatlogInfo on source:" << inSource << " failed. err:" << err);
		return;
	}

	// Inherit permission mode from parent folder.
#ifdef __LP64__
	catalogInfo.permissions.mode = parentPermissionMode;
#else
	::FSPermissionInfo* permissions = static_cast< ::FSPermissionInfo*>(&catalogInfo.permissions[0]);
	permissions->mode = parentPermissionMode;
#endif
	catalogInfo.nodeFlags &= ~::kFSNodeLockedMask;

	err = ::FSSetCatalogInfo(&destinationRef, kFSCatInfoNodeFlags, &catalogInfo);
	if (err != ::noErr)
	{
		return;
	}
	
	// Should change permission first to make sure no get error afpAccessDenied
	err = ::FSSetCatalogInfo(&destinationRef, kFSCatInfoPermissions, &catalogInfo);
	if (err != ::noErr)
	{
		return;
	}
	
	err = ::FSSetCatalogInfo(&destinationRef, whichInfo, &catalogInfo);

	if (err != ::noErr)
	{
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Alert! FSGetCatlogInfo on destination:" << inDestination << " failed. err:" << err);
		return;
	}

#else
#error Unsupported platform.
#endif
}

/*	inSourcePath and inDestinationPath should have been normalized
*/
ASL::Result CopySingleFile(const ASL::String& inSourcePath, const ASL::String& inDestinationPath)
{
	// ASL::File::Copy overwrite exist destination file always, so check if destination file exist first.
	if (!inDestinationPath.empty() && ASL::PathUtils::ExistsOnDisk(inDestinationPath))
	{
		return ASL::eFileAlreadyExists;
	}
	else
	{
		return ASL::File::Copy(inSourcePath, inDestinationPath);
	}
}

ASL::String ReportStringOfCopyResult(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	const ASL::Result& inResult)
{
	const char* resultStr = NULL;
	switch(inResult)
	{
	case ASL::eFileNotFound:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/FileNotFound=Can't find file when copying file \"@0\" to \"@1\".";
		break;

	case ASL::ePathNotFound:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/PathNotFound=Can't find path when copying \"@0\" to \"@1\".";
		break;

	case ASL::eDiskFull:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/DiskFull=Disk full detected when copying file \"@0\" to \"@1\".";
		break;

	case ASL::eAccessIsDenied:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/AccessIsDenied=Access denied when copying file \"@0\" to \"@1\".";
		break;

	case ASL::eFileInUse:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/FileInUse=File in use when copying file \"@0\" to \"@1\".";
		break;

	case ASL::eFileAlreadyExists:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/FileAlreadyExists=File already exists when copying file \"@0\" to \"@1\".";
		break;

	default:
		if ( ASL::ResultSucceeded(inResult) )
		{
			resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/CopySucceed=Copy file \"@0\" to \"@1\" finished.";
		}
		else
		{
			resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyFileResultMessage/CopyFailed=Copy file \"@0\" to \"@1\" failed for unknown reasons.";
		}
		break;
	}

	return resultStr == NULL ? 
		ASL::String() : 
		dvacore::utility::ReplaceInString(dvacore::ZString(resultStr), inSourcePath, inDestinationPath);
}

/*	inDestinationPath: the destination path want to copy to.
**	inRealDestinationPath: the destination path copied to. It may be different with inDestinationPath if user allow auto rename.
*/
ASL::String ReportStringOfCopyAction(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	const ASL::String& inRealDestinationPath,
	const CopyAction& inCopyAction)
{
	const char* resultStr = NULL;
	switch (inCopyAction)
	{
	case kCopyAction_Ignored:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyActionMessage/Ignored=Ignore copying file \"@0\" to \"@1\".";
		break;
	case kCopyAction_Replaced:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyActionMessage/Replaced=Replace existing file when copying file \"@0\" to \"@1\".";
		break;
	case kCopyAction_Renamed:
		resultStr = "$$$/Prelude/Mezzanine/IngestMedia/CopyActionMessage/Renamed=When copy file \"@0\" to \"@2\" instead of \"@1\".";
		break;
	}
	return resultStr == NULL ? ASL::String() : dvacore::utility::ReplaceInString(
		dvacore::ZString(resultStr),
		inSourcePath,
		inDestinationPath,
		inRealDestinationPath);
}


ASL::Result CopyActionPreprocess(
	const ASL::String& inSourcePath,
	ASL::String& ioDestinationPath,
	CopyAction& ioCopyAction)
{
	if (ioCopyAction == kCopyAction_Replaced)
	{
		ioCopyAction = kCopyAction_Copied;
		if (!ioDestinationPath.empty() && ASL::PathUtils::ExistsOnDisk(ioDestinationPath))
		{
			// When the destination file is in use, ASL::File::Delete() will not fail.
			// So we need check if the file is in use before calling Delete().
			ASL::File file;
			ASL::Result result = file.Create(
				ioDestinationPath,
				ASL::FileAccessFlags::kWrite,
				ASL::FileShareModeFlags::kShareWrite,
				ASL::FileCreateDispositionFlags::kOpenExisting,
				ASL::FileAttributesFlags::kAttributeNormal | ASL::FileAttributesFlags::kFlagUseSmallBuffers);
			file.Close();
			if (ASL::ResultFailed(result))
			{
				ioCopyAction = kCopyAction_NoFurtherAction;
				return result;
			}

			result = ASL::File::Delete(ioDestinationPath);
			if (ASL::ResultFailed(result))
			{
				ioCopyAction = kCopyAction_NoFurtherAction;
				return result;
			}
		}
	}
	else if (ioCopyAction == kCopyAction_Renamed)
	{
		ioCopyAction = kCopyAction_Copied;
		if (!ioDestinationPath.empty() && ASL::PathUtils::ExistsOnDisk(ioDestinationPath))
		{
			ioDestinationPath = MakeAutoRenamedPath(ioDestinationPath);
		}
	}
	return ASL::kSuccess;
}

/*	inSourcePath and inDestinationPath should have been normalized
**	ioCopyAction: According to its input value to copy and fill its value with the final real copy action.
*/
ASL::Result CopySingleFile(
	const ASL::String& inSourcePath,
	ASL::String& ioDestinationPath,
	CopyAction& ioCopyAction)
{
	ASL::Result copyResult = CopyActionPreprocess(inSourcePath, ioDestinationPath, ioCopyAction);
	if (ASL::ResultFailed(copyResult) || ioCopyAction == kCopyAction_Ignored)
	{
		return copyResult;
	}

	DVA_ASSERT_MSG(ioCopyAction == kCopyAction_Copied, "Preprocess copy return unknown value.");
	copyResult = CopySingleFile(inSourcePath, ioDestinationPath);

	if (ASL::ResultFailed(copyResult))
	{
		ioCopyAction = kCopyAction_NoFurtherAction;
	}

	return copyResult;
}

struct CopyProgressData
{
	CopyProgressData(const CopyProgressFxn& inProgressUpdateFxn)
		:
		onProgressUpdateFxn(inProgressUpdateFxn),
		lastPercentDone(0)
	{
	}

	ASL::Float32 lastPercentDone;
	const CopyProgressFxn& onProgressUpdateFxn;
};

static
bool ASLCopyProgressCallback(ASL::Float32 inPercentDone, void* inProgressData)
{
	CopyProgressData* p = (CopyProgressData*)inProgressData;
	ASL::Float32 lastPercentDone = p->lastPercentDone;
	p->lastPercentDone = inPercentDone;
	// Default return true to make copy continue.
	return (p->onProgressUpdateFxn != NULL) ? p->onProgressUpdateFxn(lastPercentDone, inPercentDone) : true;
}

ASL::Result CopySingleFileWithProgress(
	ASL::String const& inSourcePath,
	ASL::String& ioDestination,
	CopyAction& ioCopyAction,
	const CopyProgressFxn& inProgressFxn)
{
	ASL::Result result = IngestUtils::CopyActionPreprocess(inSourcePath, ioDestination, ioCopyAction);
	if (ASL::ResultFailed(result) || ioCopyAction == kCopyAction_Ignored)
	{
		return result;
	}

	CopyProgressData copyProgressData(inProgressFxn);
	result = ASL::File::CopyWithProgress(inSourcePath, ioDestination, ASLCopyProgressCallback, &copyProgressData);
	if (ASL::ResultSucceeded(result))
	{
		// [TODO] (Mac) Copy resource fork here.
#if ASL_TARGET_OS_MAC
		CopyForks(inSourcePath, ioDestination);	
#endif
	}
	else
	{
		ioCopyAction = kCopyAction_Ignored;
	}
	return result;
}

ASL::Result IncrementalCopyFiles(
	ASL::String const& inSourcePath,
	ASL::PathnameList const& inDestinations,
	const IngestUtils::CopyProgressFxn& inProgressFxn)
{
	ASL::File source;

	// [NOTE] Not using ASL::FileAttributesFlags::kFlagNoBuffering, because it
	// invariably fails with GetLastError of 87 (ERROR_INVALID_PARAMETER).
	// There must be more caveats to using it than having an 8 KB aligned buffer.
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
		ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "Never happen failure!  source.IsOpen is false.");
		return ASL::eUnknown;
	}

	std::vector<ASL::ObjectPtr<ASL::File> > destinations;
	for (std::size_t i = 0; i < inDestinations.size(); ++i)
	{
		ASL::String destFile = inDestinations[i];

		// [NOTE] Not using ASL::FileAttributesFlags::kFlagNoBuffering, because it
		// invariably fails with GetLastError of 87 (ERROR_INVALID_PARAMETER).
		// There must be more caveats to using it than having an 8 KB aligned buffer.
		ASL::ObjectPtr<ASL::File> filePtr(new ASL::File);
		ASL::Result destinationCreateResult = filePtr->Create(
			destFile,
			ASL::FileAccessFlags::kWrite,
			ASL::FileShareModeFlags::kNone,
			ASL::FileCreateDispositionFlags::kCreateNew,
			ASL::FileAttributesFlags::kFlagSequentialScan); // | ASL::FileAttributesFlags::kFlagNoBuffering

		if (ASL::ResultFailed(destinationCreateResult))
		{
			return destinationCreateResult;
		}

		destinations.push_back(filePtr);
	}

	if(destinations.size() == 0)
		return ASL::kSuccess;

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

	ASL::UInt64 destinationSize = source.SizeOnDisk();
	ASL::UInt64 destinationRemaining = destinationSize;
	ASL::UInt32 chunk = kIncrementalCopySize;

	ASL::Float32 lastPercentDone = 0;
	ASL::UInt64 totalChunkRead = 0;
	while (destinationRemaining > 0)
	{
		if (chunk > destinationRemaining)
		{
			chunk = ASL::UInt32(destinationRemaining);
		}

		ASL::UInt32 chunkRead = 0;
		source.Read(p, chunk, chunkRead);

		if (chunk != chunkRead)
		{
			ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "source.Read() requested read:" << chunk << " but only got:" << chunkRead);
			return ASL::eUnknown;
		}

		for(std::size_t i = 0; i < destinations.size(); ++i)
		{
			ASL::UInt32 chunkWrite = 0;
			destinations[i]->Write(p, chunk, chunkWrite);

			if (chunk != chunkWrite)
			{
				ASL_TRACE("MZ.IngestMediaQueueRequest", 5, "destination.Write() requested write:" << chunk << " but only wrote:" << chunkWrite);
				return ASL::eUnknown;
			}
		}

		destinationRemaining -= chunk;
		totalChunkRead += chunk;

		ASL::Float32 percentDone = ASL::Float32((double)totalChunkRead / (double)destinationSize);
		bool continueCopy = inProgressFxn(lastPercentDone, percentDone);
		lastPercentDone = percentDone;

		if (!continueCopy)
		{
			return ASL::eUserCanceled;
		}
	}

	// [TODO] (Mac) Copy resource fork here.
#if ASL_TARGET_OS_MAC

	for(ASL::PathnameList::size_type i = 0; i < inDestinations.size(); ++i)
	{
		CopyForks(inSourcePath, inDestinations[i]);
	}

#endif

	return ASL::kSuccess;
}

ASL::Result SmartCopyFileWithProgress(
	ASL::String const& inSourcePath,
	ASL::String& ioDestination,
	CopyAction& ioCopyAction,
	const CopyProgressFxn& inProgressFxn)
{
	ASL::UInt64 sourceSize = 0;
	ASL::Result result = ASL::File::SizeOnDisk(inSourcePath, sourceSize);
	if (ASL::ResultFailed(result))
	{
		ioCopyAction = kCopyAction_Ignored;
		return result;
	}

	if (sourceSize <= kSingleCopyThreshold)
	{
		// copy with small mode
		result = CopySingleFile(inSourcePath, ioDestination, ioCopyAction);
		inProgressFxn(0.0f, 1.0f);
	}
	else if (sourceSize > kIncrementalCopyThreshold)
	{
		// copy with incremental mode
		result = IngestUtils::CopyActionPreprocess(inSourcePath, ioDestination, ioCopyAction);
		if (ASL::ResultFailed(result) || ioCopyAction == kCopyAction_Ignored)
		{
			return result;
		}

		ASL::PathnameList destinationList;
		destinationList.push_back(ioDestination);
		result = IncrementalCopyFiles(inSourcePath, destinationList, inProgressFxn);

		if (ASL::ResultFailed(result))
		{
			ioCopyAction = kCopyAction_Ignored;
		}
	}
	else
	{
		result = CopySingleFileWithProgress(inSourcePath, ioDestination, ioCopyAction, inProgressFxn);
	}

	if (ASL::ResultSucceeded(result))
	{
		StampPathEntryInformation(inSourcePath, ioDestination);
	}
	return result;
}



bool GenerateFileCopyAction(
	const CopyExistOption& inSingleFileOption,
	CopyExistOption& ioJobOption,
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	CopyAction& outResultAction)
{
	CopyAction action = kCopyAction_Copied;

	if (inDestinationPath.empty())
	{
		action = kCopyAction_Ignored;
	}
	else
	{
		// The single file option will overwrite the job level option
		CopyExistOption option = inSingleFileOption;
		if (option == kExist_WarnUser)
		{
			option = ioJobOption;
		}

		if (ASL::PathUtils::ExistsOnDisk(inDestinationPath))
		{
			if (option == kExist_WarnUser)
			{
				// Need pop us dialog for users to select an action
				int applyToAll = 0;
				int copyExistOption = (int)option;
				bool canContinue = WarnUserForExistPath(
					inSourcePath, 
					inDestinationPath, 
					false, // isn't directory
					copyExistOption, 
					applyToAll,
					true, // allow cancel
					false, // not in copy list
					false); // not pre-check
				if (!canContinue)
				{
					// user cancel the operation
					return false;
				}

				if ( copyExistOption != (int)kExist_WarnUser && applyToAll == 1 )
				{
					ioJobOption = (CopyExistOption)copyExistOption;
				}
				option = (CopyExistOption)copyExistOption;
			}
		}
		action = MapCopyExistOption2Action(option);
	}

	outResultAction = action;
	return true;
}


bool GenerateFolderCopyAction(
	const CopyExistOption& inSingleFileOption,
	CopyExistOption& ioJobOption,
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	CopyAction& outResultAction)
{
	CopyAction action = kCopyAction_Copied;

	// If the destination file is not on disk, just copy it
	// Otherwise, need to figure out the right copy action
	if ( !inDestinationPath.empty() && ASL::PathUtils::ExistsOnDisk(inDestinationPath) )
	{
		// The single file option will overwrite the job level option
		CopyExistOption option = inSingleFileOption;
		if (option == kExist_WarnUser)
		{
			option = ioJobOption;
		}

		if ( option == kExist_WarnUser )
		{
			// [TODO] Need pop us dialog for users to select an action
			// Need pop us dialog for users to select an action
			int applyToAll = 0;
			int copyExistOption = (int)option;
			bool canContinue = WarnUserForExistPath(
				inSourcePath, 
				inDestinationPath, 
				true,  // is directory
				copyExistOption, 
				applyToAll,
				true, // allow cancel
				false, // not in copy list
				false); // not pre-check
			if (!canContinue)
			{
				// user cancel the operation
				return false;
			}

			if ( copyExistOption != (int)kExist_WarnUser && applyToAll == 1 )
			{
				ioJobOption = (CopyExistOption)copyExistOption;
			}
			option = (CopyExistOption)copyExistOption;
		}

		action = MapCopyExistOption2Action(option);
	}

	outResultAction = action;
	return true;
}

bool WarnUserForExistPath(
	ASL::String const& inSrcPath,
	ASL::String const& inDestPath,
	bool inIsDirectory,
	int& outCopyExistOption,
	int& outApplyToAll,
	bool inAllowCancel,
	bool inIsconflictWithCopyingTask,
	bool inIsPrecheck)
{
	using namespace dvacore::config;
	using namespace dvaui::dialogs;

	dvacore::UTF16String warningTitle;
	dvacore::UTF16String warningMessage;
	IG_MessageBox::IngestButtons buttons = IG_MessageBox::kIngestButton_Ignore;
	if (inAllowCancel)
	{
		buttons |= IG_MessageBox::kIngestButton_Cancel;
	}
	if (inIsDirectory)
	{
		buttons |= IG_MessageBox::kIngestButton_Merge;
		warningTitle = Localizer::Get()->GetLocalizedString("$$$/Prelude/Mezzanine/IngestMedia/Warning/DirectoryExistTitle=Directory already exists");
		if (!inIsconflictWithCopyingTask)
		{
			warningMessage = Localizer::Get()->GetLocalizedString(
				"$$$/Prelude/Mezzanine/IngestMedia/Warning/DirectoryExistMessage=Directory \"@0\" already exists.@1Do you want to merge this directory with \"@2\"?");
		}
		else
		{
			warningMessage = Localizer::Get()->GetLocalizedString(
				"$$$/Prelude/Mezzanine/IngestMedia/Warning/DirectoryIsCreatingMessage=Directory \"@0\" will be created during ingest.@1Do you want to merge this directory with \"@2\"?");
		}
	}
	else
	{
		buttons |= IG_MessageBox::kIngestButton_Overwrite;
		warningTitle = dvacore::ZString("$$$/Prelude/Mezzanine/IngestMedia/Warning/FileExistTitle=File already exists");
		if (!inIsconflictWithCopyingTask)
		{
			warningMessage = Localizer::Get()->GetLocalizedString(
				"$$$/Prelude/Mezzanine/IngestMedia/Warning/FileExistMessage=File \"@0\" already exists.@1Do you want to overwrite this file with \"@2\"?");
		}
		else
		{
			warningMessage = Localizer::Get()->GetLocalizedString(
				"$$$/Prelude/Mezzanine/IngestMedia/Warning/FileCreatingMessage=File \"@0\" will be created during ingest.@1Do you want to overwrite this file with \"@2\"?");
		}
	}

	dvacore::UTF16String const& lineEnding = dvacore::utility::GetLineEnding(dvacore::utility::LineEndingType_THIS_ONE);
	warningMessage = dvacore::utility::ReplaceInString(warningMessage, inDestPath, lineEnding, inSrcPath);
	warningMessage += lineEnding;
	warningMessage += dvacore::ZString(
		inIsPrecheck ? "$$$/Prelude/Mezzanine/IngestMedia/Warning/PrecheckCancelBtnIntruduction=Press \"Cancel\" button to go back to the ingest dialog."
					 : "$$$/Prelude/Mezzanine/IngestMedia/Warning/RunningCancelBtnIntruduction=Press \"Cancel\" button to cancel all ongoing ingest tasks.");

	bool isChecked = false;
	dvacore::UTF16String checkText = Localizer::Get()->GetLocalizedString(
		"$$$/Prelude/Mezzanine/IngestMedia/CheckBoxText=Apply to All");

	IG_MessageBox::IngestButton button;
	ShowIngestMessageBoxInMainThread helper(button, warningTitle, warningMessage, checkText, isChecked, buttons);
	MZ::Utilities::SyncCallFromMainThread(boost::bind(&ShowIngestMessageBoxInMainThread::Process, &helper));

	outApplyToAll = isChecked ? 1 : 0;

	bool continueProcess = true;
	if (button == IG_MessageBox::kIngestButton_Overwrite || button == IG_MessageBox::kIngestButton_Merge)
		outCopyExistOption = kExist_Replace;
	else if (button == IG_MessageBox::kIngestButton_Ignore)
		outCopyExistOption = kExist_Ignore;
	else
	{
		DVA_ASSERT_MSG(inAllowCancel, "Don't allow cancel, but receive canceled result?");
		// user select cancel, then return false
		continueProcess = false;
	}

	return continueProcess;
}

VerifyFileResult VerifyFile(
	const VerifyOption& inOption, 
	const ASL::String& inSrc, 
	const ASL::String& inDest,
	ASL::String& outResultMsg)
{
	const ASL::String& optionStr = GetVerifyOptionString(inOption);
	VerifyFileResult result = kVerifyFileResult_Equal;

	if ( inSrc.empty() || 
		 inDest.empty() || 
		 !ASL::PathUtils::ExistsOnDisk(inSrc) || 
		 !ASL::PathUtils::ExistsOnDisk(inDest))
	{
		result = kVerifyFileResult_FileNotExist;
		outResultMsg = MakeCompareDescription(
			inSrc, 
			inDest, 
			optionStr +	GetCompareResultString(result));
		return result;
	}
    
    adobe::md5_t::digest_t srcMD5Value;
    adobe::md5_t::digest_t destMD5Value;

	switch (inOption)
	{
	case kVerify_FileSize:
		result = VerifyFileSize(inSrc, inDest);
		break;
	case kVerify_FileContent: 
		result = VerifyFileContent(inSrc, inDest);
		break;
	case kVerify_FileMD5:
		result = VerifyFileMD5(inSrc, inDest, srcMD5Value, destMD5Value);
	//case kVerify_FolderStruct: 
	//	break;
	default:
		break;
	}

	outResultMsg = MakeCompareDescription(
		inSrc, 
		inDest, 
		optionStr +	GetCompareResultString(result));

	if ( kVerify_FileMD5 == inOption )
	{
		dvacore::UTF16String detailFormat = dvacore::ZString("$$$/Prelude/MZ/IngestMedia/MD5Details=(MD5 value of source file: @0; MD5 value of dest file: @1)");
		dvacore::UTF16String detailMessage = dvacore::utility::ReplaceInString(detailFormat, GetMD5HexString(srcMD5Value), GetMD5HexString(destMD5Value));

		outResultMsg += DVA_STR(" ") + detailMessage;
	}
    
	return result;
}

#ifndef DO_NOT_USE_AME
// Copy from PrepareSequenceForExport() in ExportMovie.cpp
void PrepareSequenceForExport(
	const BE::IProjectRef& inProject,
	const BE::ISequenceRef& inSequence,
	BE::ISequenceRef& outClonedSequence,
	BE::IProjectRef& outClonedProject)
{
	ASL_REQUIRE(inProject != NULL);
	ASL_REQUIRE(inSequence != NULL);		

	ASL::Result result = ASL::kSuccess;

	try
	{
		// first copy the sequence and project to the output params in case the user aborts
		outClonedSequence = inSequence;
		outClonedProject = inProject;
		//	Create the operation.
		MZ::IPrepareSequenceForExportOperationRef operation(
			MZ::EncoderManager::CreatePrepareSequenceForExportOperation(inProject, inSequence));

		operation->StartSynchronous();
		{
			//	The operation was not canceled. Get the true result
			//	from the operation itself.
			result = operation->GetResult(outClonedSequence, outClonedProject);
			if (result == BE::eOperationCanceled)
			{
				result = ASL::eUserCanceled;
			}
		}
	}
	catch (std::bad_alloc&)
	{
		result = ASL::eInsufficientMemory;
	}

	// note that we are ignoring the result currently, since presumably the operation
	// left things in an exportable state.
	// Leaving the skeleton of a "cancel" implementation or other error handling in case need it later
}

ASL::Result SaveProjectAs(
	const BE::IProjectRef& inProject,
	const dvacore::UTF16String& inFilePath)
{
	ASL_REQUIRE(inProject != NULL);
	ASL_REQUIRE(!inFilePath.empty());		

	ASL::Result result = ASL::kSuccess;

	try
	{
		//	Create the operation.
		// We want to save the file path to BE.Project
		const bool saveACopy = false;
		MZ::ISaveProjectOperationRef operation(
			MZ::CreateSaveProjectOperation(inProject, inFilePath, saveACopy, false));

		operation->StartSynchronous();
		result = operation->GetResult();
		if (result == BE::eOperationCanceled)
		{
			result = ASL::eUserCanceled;
		}
	}
	catch (std::bad_alloc&)
	{
		result = ASL::eInsufficientMemory;
	}

	return result;
}

/* Reference HSL::PrepareMasterClipForTranscode
*/
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
	dvacore::UTF16String&				outErrorInfo)
{
	ASL_REQUIRE(inType != BE::kCompileSettingsType_Custom);
	//ASL_REQUIRE(inMasterClip != NULL);
	if ( inSourceClipPath.empty() || !ASL::PathUtils::ExistsOnDisk(inSourceClipPath) )
	{
		DVA_ASSERT_MSG(0, "Source clip does not exist!");
		outErrorInfo = dvacore::ZString(
			"$$$/Prelude/Mezzanine/IngestUtils/TranscodeSrcFail=Source clip does not exist!");
		return false;
	}

	if ( inPresetPath.empty() || !ASL::PathUtils::ExistsOnDisk(inPresetPath) )
	{
		DVA_ASSERT_MSG(0, "Preset path is invalid!");
		outErrorInfo = dvacore::ZString(
			"$$$/Prelude/Mezzanine/IngestUtils/TranscodePresetFail=Preset path is invalid!");
		return false;
	}

	BE::CompileSettingsType compileSettingsType = inType;

	// The encoderManager is the interface we use to render out to AME
	MZ::EncoderManager* encoderManager = MZ::EncoderManager::Instance();
	if (encoderManager)
	{		
		const BE::IProjectRef theProject(MZ::GetProject());

		DVA_ASSERT(!inDestClipPath.empty());
		ASL::String inDstDir(ASL::PathUtils::GetFullDirectoryPart(inDestClipPath));
		bool isEnsureDirectoryExists = dvacore::utility::FileUtils::EnsureDirectoryExists(inDstDir);
		DVA_ASSERT(isEnsureDirectoryExists);

		UIF::FocusSaver focusSaver;

		ASL::String fullFilePath(ASL::PathUtils::AddTrailingSlash(inDestClipPath));
		if (inDestClipName.empty())
		{
			// AME client app has to give the full file name to AME for transcoding.
			// If the extension of destination file is different with the real file created by AME, 
			// AME can correct and build the correct and unique file name before transcoding. 
			// Please refer to function Transcoder::SetOutputFileName in Transcoder.cpp in AME or Premiere
			fullFilePath += ASL::PathUtils::GetFullFilePart(inSourceClipPath);
		}
		else
		{
			fullFilePath += inDestClipName;
		}

		encoderManager->AsyncAddMasterClip(
			inRequestID,
			inJobID,
			theProject,
			inSourceClipPath,
			inPresetPath,
			BE::IMasterClipRef(),
			fullFilePath,
			inStart,
			inEnd,
			true,
			true);

		return true;
	}

	outErrorInfo = dvacore::ZString(
		"$$$/Prelude/Mezzanine/IngestUtils/TranscodeEncodeManagerFail=Adobe Prelude received a memory error while it was creating Encoder Manager!");
	return false;
}

bool SendSequenceToEncoderManager(
	const RequesterID& inRequesterID,
	const JobID& inJobID,
	EncoderHost::IPresetRef inPreset,
	BE::IProjectRef inProject,
	BE::ISequenceRef inSequence,
	dvacore::UTF16String const& inRenderPath,
	dvacore::UTF16String const& inOutputName)
{
	MZ::EncoderManager* encoderManager = MZ::EncoderManager::Instance();
	if (encoderManager)
	{		
		DVA_ASSERT(!inRenderPath.empty());
		ASL::String inDstDir(ASL::PathUtils::GetFullDirectoryPart(inRenderPath));
		bool isEnsureDirectoryExists = dvacore::utility::FileUtils::EnsureDirectoryExists(inDstDir);
		DVA_ASSERT(isEnsureDirectoryExists);

		UIF::FocusSaver focusSaver;

		dvamediatypes::TickTime tempFrameTime = dvamediatypes::TickTime::TicksToTime(0);
		dvacore::UTF16String renderPath = ASL::PathUtils::AddTrailingSlash(inRenderPath) + inOutputName;
		encoderManager->AddSequence(
			inRequesterID,
			inJobID,
			inProject,
			BE::IProjectRef(),
			inPreset,
			inSequence,
			tempFrameTime,
			renderPath,
			MZ::ExportMediaEncodeType_Timeline,
			SaveProjectAs,
			PrepareSequenceForExport);

		return true;
	}

	return false;
}

bool TranscodeConcatenateFile(
	const RequesterID& inRequesterID,
	const JobID& inJobID,
	EncoderHost::IPresetRef inPreset,
	BE::IProjectRef inProject,
	BE::ISequenceRef inSequence,
	dvacore::UTF16String const& inRenderPath,
	dvacore::UTF16String const& inOutputName,
	IngestTask::PathToErrorInfoVec& outErrorInfos)
{
	if (SendSequenceToEncoderManager(
			inRequesterID,
			inJobID,
			inPreset,
			inProject,
			inSequence,
			inRenderPath,
			inOutputName))
	{
		return true;
	}

	ASL::String const& errorInfo = dvacore::ZString(
		"$$$/Prelude/Mezzanine/IngestUtils/TranscodeEncodeManagerFail=Adobe Prelude received a memory error while it was creating Encoder Manager!");
	outErrorInfos.push_back(make_pair(inOutputName, errorInfo));

	return false;
}

#endif // DO_NOT_USE_AME

bool CreateSequenceForConcatenateMedia(
	IngestTask::ClipSettingList const& inClipSettingList, 
	BE::IProjectRef& outProject, 
	BE::ISequenceRef& outSequence, 
	ASL::String const& inConcatenateName, 
	IngestTask::PathToErrorInfoVec& outErrorInfos)
{
	Utilities::SuppressPeakFileGeneration suppressPeakFile;
	Utilities::SuppressAudioConforming suppressAudioConform;

	SRClipItems srClipItems;
	AssetItemList subAssetItems;

	IngestTask::ClipSettingList::const_iterator itr_end = inClipSettingList.end();
	for (IngestTask::ClipSettingList::const_iterator itr = inClipSettingList.begin(); itr != itr_end; itr++)
	{
		if (((*itr).mInPoint != dvamediatypes::kTime_Invalid) && ((*itr).mOutPoint != dvamediatypes::kTime_Invalid))
		{
			AssetItemPtr eachItem = AssetItemPtr(new AssetItem(				
				kAssetLibraryType_SubClip,
				(*itr).mFileName,
				ASL::Guid(),
				ASL::Guid(),
				ASL::Guid(),
				ASL::PathUtils::GetFilePart((*itr).mFileName),
				ASL::String(),
				(*itr).mInPoint,
				(*itr).mOutPoint - (*itr).mInPoint));
			subAssetItems.push_back(eachItem);
		}
		else
		{
			AssetItemPtr eachItem = AssetItemPtr(new AssetItem(				
				kAssetLibraryType_MasterClip,
				(*itr).mFileName,
				ASL::Guid(),
				ASL::Guid(),
				ASL::Guid(),
				ASL::PathUtils::GetFilePart((*itr).mFileName),
				ASL::String(),
				dvamediatypes::kTime_Invalid,
				dvamediatypes::kTime_Invalid));
			subAssetItems.push_back(eachItem);
		}
	}

	BE::IProjectSettingsRef projectSettings(ASL::CreateClassInstanceRef(BE::kProjectSettingsClassID));
	BE::IPropertiesRef projectProperties(BE::GetBackend());
	projectProperties->GetOpaque(BE::kPrefsDefaultProjectSettings, projectSettings);

	//<note>The default project settings is empty. So we use a dummy video frameRate value to avoid PPro crash.
	projectSettings->GetCaptureSettings()->SetVideoFrameRate(dvamediatypes::kFrameRate_VideoNTSC);

	//	Create an empty project to pass to premiere during copy and paste
	outProject = BE::GetBackend()->CreateProject(ASL_STR("Concatenation"), projectSettings);

	ASL::String errorInfo, failedMediaName;
	outSequence = CreateBESequenceFromRCSubClips(
						subAssetItems,
						outProject,
						inConcatenateName,
						errorInfo,
						failedMediaName,
						true,
						MZ::kSequenceAudioTrackRule_ExportToAME);

	if (!outSequence)
	{
		outErrorInfos.push_back(make_pair(failedMediaName, errorInfo));
		return false;
	}

	return true;
}

void TranscodeStatusToString(
	dvametadatadb::StatusType inStatus,
	TranscodeResult& outFailure)
{
	outFailure.first	= inStatus;
	switch (inStatus)
	{
	case dvametadatadb::kStatusType_Queued:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeQueued=Task is queued");
		break;
	case dvametadatadb::kStatusType_Processing:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeProcessing=Task is processing");
		break;
	case dvametadatadb::kStatusType_Complete:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeComplete=Task is complete");
		break;
	case dvametadatadb::kStatusType_Skipped:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeSkipped=Task is skipped");
		break;
	case dvametadatadb::kStatusType_Stopped:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeStopped=Task is stopped");
		break;
	case dvametadatadb::kStatusType_Paused:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypePaused=Task is paused");
		break;
	case dvametadatadb::kStatusType_NotFound:
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/StatusTypeNotFound=Task is not found");
		break;
	default :
		outFailure.second	= dvacore::ZString(
			"$$$/Prelude/Mezzanine/UnknownEncoderError=Unknown error");
		break;
	}		
}

} //IngestUtils

} // namespace PL
