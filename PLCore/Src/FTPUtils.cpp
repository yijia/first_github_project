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

//	Prefix
#include "Prefix.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLStringUtils.h"
#include "ASLFile.h"
#include "ASLResults.h"
#include "ASLContract.h"
#include "ASLSleep.h"

// DVA
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/UnicodeCaseConversion.h"
#include "dvacore/threads/SharedThreads.h"

// local
#include "PLFTPUtils.h"
#include "MZUtilities.h"
#include "ExportUtils.h"

#if ASL_TARGET_OS_MAC
#include <sys/dirent.h>
#endif

namespace PL 
{
	namespace FTPUtils 
	{
		const ASL::UInt32 kMyBufferSize = 32768;

		static ASL::String AdjustServerName(ASL::String const& inServer)
		{
			return (inServer.substr(0, 6) != ASL_STR("ftp://"))
				? (ASL_STR("ftp://") + inServer)
				: inServer;
		}

		static void UpdateProgressIfNeeded(
			ExportUtils::ExportProgressData* inProgressData,
			ASL::UInt64 inFileSizeinBytes,
			ASL::UInt64 inTotalWrittenBytes)
		{
			if (inFileSizeinBytes == 0 || inProgressData == NULL || inProgressData->UpdateProgress == NULL)
			{
				return;
			}

			ASL::UInt64 latestProgress = inTotalWrittenBytes * inProgressData->totalProgress / inFileSizeinBytes;
			latestProgress = std::min(latestProgress, inProgressData->totalProgress);
			if (latestProgress > inProgressData->currentProgress)
			{
				inProgressData->UpdateProgress(latestProgress - inProgressData->currentProgress);
				inProgressData->currentProgress = latestProgress;
			}
		}

#if ASL_TARGET_OS_WIN
		static bool ValidFTPConnection(const FTPConnectionDataPtr& inFTPConnection)
		{
			return (inFTPConnection && inFTPConnection->hInternet && inFTPConnection->hSession);
		}

		// Suppose that separator in inDir is "/", 
		// because BuildUnqiueDestinationPathForFTP() forces to make it.
		static BOOL MakeRemoteDir(
			const FTPConnectionDataPtr& inFTPConnection,
			ASL::String const& inDir,
			bool inIsCreateIfNoDir)
		{
			BOOL ftpRes = TRUE;

// To fix bug #3520646, if FTP server is on Mac 10.7 or later, the default FTP directory is "Users/labuser",
// which is not the root, so we should not reset to root in this case. For FTP on win, current dir seems to be 
// the root.
// However, we should also need to reset the directory to default because everytime uploading or making dir
// will change current directory.

			// Reset to the root path
			FtpSetCurrentDirectoryA(inFTPConnection->hSession, "/");
			if (!inFTPConnection->mDefaultDir.empty() && 
				inFTPConnection->mDefaultDir != dvacore::utility::CharAsUTF8("/"))
			{
				if (!FtpSetCurrentDirectoryA(inFTPConnection->hSession, (LPCSTR)(inFTPConnection->mDefaultDir.c_str())))
				{
					// the default dir should exist, we should not fail to make it.
					return false;
				}
			}

			ASL::String dstPath = inDir;
			ASL::String::size_type slashPos = dstPath.find(L"/");
			while ( slashPos !=  ASL::String::npos )
			{
				ASL::String const& eachDir = dstPath.substr(0, slashPos);
				if (!eachDir.empty())
				{
					dvacore::UTF8String const& utf8eachDir = dvacore::utility::UTF16to8(eachDir);
					ftpRes = FtpSetCurrentDirectoryA(inFTPConnection->hSession, (LPCSTR)(utf8eachDir.c_str()));
					if (!ftpRes)
					{
						// this is used for directory listing, return if set dir fails,
						// which means dir does not exist.
						if (!inIsCreateIfNoDir)
						{
							return false;
						}

						ftpRes = FtpCreateDirectoryA(inFTPConnection->hSession, (LPCSTR)(utf8eachDir.c_str()));
						if (!ftpRes)
						{
							return false;
						}

						ftpRes = FtpSetCurrentDirectoryA(inFTPConnection->hSession, (LPCSTR)(utf8eachDir.c_str()));
						if (!ftpRes)
						{
							return false;
						}
					}
				}

				dstPath = dstPath.substr(slashPos + 1);
				slashPos = dstPath.find(L"/");
			}

			return ftpRes;
		}

		static bool IsFTPFileExist(ASL::String const& inFileName, WIN32_FIND_DATAA const& inData)
		{
			dvacore::UTF8String const& utf8CurFileName((dvacore::UTF8Char*)(inData.cFileName));
			ASL::String const& utf16CurFileName = dvacore::utility::UTF8to16(utf8CurFileName);

			return (inData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) && 
				dvacore::utility::LowerCase(utf16CurFileName) == dvacore::utility::LowerCase(inFileName);
		}

		static bool FTPUploadWithProgress(
			const FTPConnectionDataPtr& inFTPConnection,
			const dvacore::UTF8String& inUTF8OutName,
			ExportUtils::ExportProgressData* inProgressData,
			const ASL::String& inSourcePath)
		{
			if (!ValidFTPConnection(inFTPConnection))
			{
				return false;
			}

			HANDLE hLocal = CreateFileW(
								inSourcePath.c_str(),
								GENERIC_READ,
								FILE_SHARE_READ,
								0,
								OPEN_EXISTING,
								0,
								NULL);
			DWORD error = GetLastError();
			if (hLocal == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			// As required in RFC-2640, FTP should support UTF8 encode
			// this handle must be closed before the next open for the same session
			HANDLE hFTPFile = FtpOpenFileA(
								inFTPConnection->hSession,
								(LPCSTR)inUTF8OutName.c_str(),
								GENERIC_WRITE,
								FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_NO_CACHE_WRITE,
								0);
			if (hFTPFile == INVALID_HANDLE_VALUE)
			{
				CloseHandle(hLocal);
				return false;
			}

			char* pBuf = NULL;
			try
			{
				pBuf = new char[kMyBufferSize];
			}
			catch(...)
			{
				CloseHandle(hLocal);
				InternetCloseHandle(hFTPFile);
				return false;
			}

			ASL::UInt64 fileSize;
			ASL::File::SizeOnDisk(inSourcePath, fileSize);
			bool success = true;
			DWORD dwRead = 0;
			ASL::UInt64 totalBytesWritten = 0;
			do 
			{
				if (ReadFile(hLocal, pBuf, kMyBufferSize, &dwRead, NULL))
				{
					DWORD dwWritten = 0;

					// If ReadFile() returns true and dwRead is 0, that means we reach the end of file
					if (dwRead > 0)
					{
						if (InternetWriteFile(hFTPFile, pBuf, dwRead, &dwWritten))
						{
							totalBytesWritten += (ASL::UInt64)dwWritten;
							UpdateProgressIfNeeded(inProgressData, fileSize, totalBytesWritten);
							if (inProgressData && inProgressData->UpdateProgress)
							{
								if (inProgressData->UpdateProgress(0))
								{
									break;
								}
							}
						}
						else
						{
							success = false;
						}
					}
				}
				else
				{
					success = false;
				}
			} while ( (dwRead == kMyBufferSize) && success );

			delete [] pBuf;
			InternetCloseHandle(hFTPFile);
			CloseHandle(hLocal);

			return success;
		}

		bool CreateFTPConnection(
			FTPConnectionDataPtr& outFTPConnection,
			ASL::String const& inServer,
			ASL::UInt32 inPort,
			ASL::String const& inRemoteDirectory,
			ASL::String const& inUser,
			ASL::String const& inPassword)
		{
			try
			{
				if (!outFTPConnection)
				{
					outFTPConnection = FTPConnectionDataPtr(new FTPConnectionData());
				}

				outFTPConnection->hInternet = InternetOpenW(L"Prelude FTP Export", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
				if(outFTPConnection->hInternet)
				{
					outFTPConnection->hSession = InternetConnectW(
						outFTPConnection->hInternet, 
						inServer.c_str(), 
						(INTERNET_PORT)inPort, 
						inUser.c_str(), 
						inPassword.c_str(), 
						INTERNET_SERVICE_FTP, 
						INTERNET_FLAG_PASSIVE, 
						0);
				}

				if (!outFTPConnection->hInternet || !outFTPConnection->hSession)
				{
					CloseFTPConncection(outFTPConnection);
					return false;
				}

				DWORD dirLen = MAX_PATH;
				CHAR szDefaultDir[MAX_PATH];
				FtpGetCurrentDirectoryA(outFTPConnection->hSession, szDefaultDir, &dirLen);
				outFTPConnection->mDefaultDir = dvacore::utility::CharAsUTF8(szDefaultDir);
			}
			catch(...)
			{
				CloseFTPConncection(outFTPConnection);
				return false;
			}

			return true;
		}

		void CloseFTPConncection(const FTPConnectionDataPtr& inFTPConnection)
		{
			if (!inFTPConnection)
			{
				return;
			}

			try
			{
				if (inFTPConnection->hSession)
				{
					InternetCloseHandle(inFTPConnection->hSession);
					inFTPConnection->hSession = NULL;
				}
				if (inFTPConnection->hInternet)
				{
					InternetCloseHandle(inFTPConnection->hInternet);
					inFTPConnection->hInternet = NULL;
				}
			}
			catch(...)
			{
				// Do nothing
			}
		}

		bool FTPMakeDir(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDir)
		{
			if (!ValidFTPConnection(inFTPConnection))
			{
				return false;
			}

			ASL::String remoteDir = MZ::Utilities::AddTrailingForwardSlash(inDir);
			return MakeRemoteDir(inFTPConnection, remoteDir, true) ? true : false;
		}

		bool FTPDelete(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDestPath)
		{
			if (!ValidFTPConnection(inFTPConnection) || inDestPath.empty())
			{
				return false;
			}
			if ( !MakeRemoteDir(inFTPConnection, inDestPath, false) )
			{
				return false;
			}

			ASL::String const& filePart = ASL::PathUtils::GetFullFilePart(inDestPath);
			dvacore::UTF8String utf8OutFile = dvacore::utility::UTF16to8(filePart);
			return FtpDeleteFileA(inFTPConnection->hSession, (LPCSTR)(utf8OutFile.c_str())) == TRUE
					? true : false;
		}

		bool FTPUpload(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDestPath,
			const ASL::String& inSourcePath,
			void* inProgressData)
		{
			if (!ValidFTPConnection(inFTPConnection) || inDestPath.empty() || inSourcePath.empty())
			{
				return false;
			}
			if ( !MakeRemoteDir(inFTPConnection, inDestPath, true) )
			{
				return false;
			}

			ASL::String const& filePart = ASL::PathUtils::GetFullFilePart(inDestPath);
			dvacore::UTF8String utf8OutFile = dvacore::utility::UTF16to8(filePart);

			bool success = true;
			if (inProgressData == NULL)
			{
				if (!FtpPutFileEx(
					inFTPConnection->hSession, inSourcePath.c_str(), 
					(LPCSTR)(utf8OutFile.c_str()), 
					FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 
					0))
				{
					success = false;
				}
			}
			else
			{
				success = FTPUploadWithProgress(
								inFTPConnection,
								utf8OutFile,
								(ExportUtils::ExportProgressData*)inProgressData,
								inSourcePath);
			}

			return success;
		}

		bool FTPExistOnDisk(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inPath,
			bool inIsDirectory)
		{
			if (!ValidFTPConnection(inFTPConnection) || inPath.empty())
			{
				return false;
			}

			if (inIsDirectory)
			{
				ASL::String remoteDir = MZ::Utilities::AddTrailingForwardSlash(inPath);
				return MakeRemoteDir(inFTPConnection, remoteDir, false) ? true : false;
			}

			// file exist
			// Navigate to the deepest directory
			if (MakeRemoteDir(inFTPConnection, inPath, false))
			{
				ASL::String const& filePart = ASL::PathUtils::GetFullFilePart(inPath);

				WIN32_FIND_DATAA fileData;
				memset(&fileData, 0, sizeof(WIN32_FIND_DATAA));

				// Firstly, try to find file name directly.
				// As MSDN said, the searched file name should not contain blank space,
				// however I test with FileZilla and Mac FTP server, file name including space
				// also can be found with this method.
				// So I perform directly find first, if it succeeds, check the file name and return;
				// if it fails (may because either file not exist or the file exist but file name contains
				// space and FtpFindFirstFileA works incorrectly for that), we should go through files
				// in the folder one by one and check the file existence.
				HINTERNET hfindDirectly = FtpFindFirstFileA(
					inFTPConnection->hSession, 
					(LPCSTR)((dvacore::utility::UTF16to8(filePart)).c_str()), 
					&fileData, 
					INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 
					0);
				if (hfindDirectly)
				{
					if (IsFTPFileExist(filePart, fileData))
					{
						InternetCloseHandle(hfindDirectly);
						return true;
					}
					InternetCloseHandle(hfindDirectly);
				}

				// If we cannot find the file directly, we go through the folder and find 
				// files one by one.
				{
					memset(&fileData, 0, sizeof(WIN32_FIND_DATAA));
					HINTERNET hfind = FtpFindFirstFileA(
						inFTPConnection->hSession, 
						NULL, 
						&fileData, 
						INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 
						0);
					if (IsFTPFileExist(filePart, fileData))
					{
						InternetCloseHandle(hfind);
						return true;
					}

					memset(&fileData, 0, sizeof(WIN32_FIND_DATAA));
					while (InternetFindNextFileA(hfind, &fileData))
					{
						if (IsFTPFileExist(filePart, fileData))
						{
							InternetCloseHandle(hfind);
							return true;
						}
						memset(&fileData, 0, sizeof(WIN32_FIND_DATAA));
					}

					// must close handle created by FtpFindFirstFile, 
					// otherwise there will be no new handle from the same session
					InternetCloseHandle(hfind);
				}
			}

			return false;
		}
#else
		static const CFOptionFlags kNetworkEvents = 
			kCFStreamEventOpenCompleted
			| kCFStreamEventHasBytesAvailable
			| kCFStreamEventEndEncountered
			| kCFStreamEventCanAcceptBytes
			| kCFStreamEventErrorOccurred;

		struct ListedFileInfo
		{
			dvacore::UTF8String mName;
			bool mIsDir;
		};
		typedef std::vector<ListedFileInfo> ListedFileInfoVec;

		typedef struct MyStreamInfo {
			CFWriteStreamRef  writeStream;
			CFReadStreamRef   readStream;
			CFDictionaryRef   proxyDict;
			ASL::UInt64       fileSizeinBytes;
			UInt32            totalBytesWritten;
			UInt32            leftOverByteCount;
			UInt32			  bufferSize;
			UInt8             buffer[kMyBufferSize];
			bool*			  pResult;
			ListedFileInfoVec* pFileInfos;
			ExportUtils::ExportProgressData* pProgressData;
		} MyStreamInfo;
		
		static bool ValidFTPConnection(const FTPConnectionDataPtr& inFTPConnection)
		{
			return (inFTPConnection && !inFTPConnection->mServerName.empty());
		}
		
		static bool IsFTPFileExist(ListedFileInfoVec const& inFileInfos, dvacore::UTF8String const inFileName, bool inIsDirectory)
		{
			ListedFileInfoVec::const_iterator it_end = inFileInfos.end();
			for (ListedFileInfoVec::const_iterator it = inFileInfos.begin(); it != it_end; it++)
			{
				bool foundDir = (*it).mIsDir;
				if ( (foundDir && inIsDirectory) || (!foundDir && !inIsDirectory) )
				{
					ASL::String const& foundName = dvacore::utility::UTF8to16((*it).mName);
					ASL::String const& sourceName = dvacore::utility::UTF8to16(inFileName);
					if (dvacore::utility::LowerCase(foundName) == dvacore::utility::LowerCase(sourceName))
					{
						return true;
					}
				}
			}
			return false;
		}
		
		static	bool
		MySetupStreamInfo(
			MyStreamInfo ** info, 
			CFReadStreamRef readStream, 
			CFWriteStreamRef writeStream, 
			bool* ioResult, 
			ListedFileInfoVec* ioFileInfos,
			ExportUtils::ExportProgressData* inProgressData,
			ASL::UInt64 inFileSize)
		{
			if (info == NULL) 
			{
				return false;
			}
			MyStreamInfo * streamInfo;
			
			streamInfo = reinterpret_cast<MyStreamInfo*>(malloc(sizeof(MyStreamInfo)));
			if (streamInfo == NULL) 
			{
				return false;
			}
			
			streamInfo->leftOverByteCount = 0;
			streamInfo->totalBytesWritten = 0;
			streamInfo->bufferSize = 1;
			streamInfo->readStream = readStream;
			streamInfo->writeStream = writeStream;
			streamInfo->pResult = ioResult;
			streamInfo->pFileInfos = ioFileInfos;
			streamInfo->pProgressData = inProgressData;
			streamInfo->fileSizeinBytes = inFileSize;
			
			/* Part of workaround for bug <rdar://problem/3745574>. CFFTPStream is crashing because it uses the proxyDict
			 after releasing it.  To workaround this bug, we simply retain a copy of the proxyDict in our MyStreamInfo struct
			 and then release it after closing the stream. */
			streamInfo->proxyDict = NULL;
			
			*info = streamInfo;
			
			return true;
		}
		
		static	void
		MyCleanupStreamInfo(MyStreamInfo * info)
		{
			if (info == NULL)
			{
				return;
			}
			
			if (info->readStream) {
				CFReadStreamUnscheduleFromRunLoop(info->readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
				(void) CFReadStreamSetClient(info->readStream, kCFStreamEventNone, NULL, NULL);
				
				/* CFReadStreamClose terminates the stream. */
				CFReadStreamClose(info->readStream);
				CFRelease(info->readStream);
			}
			
			if (info->writeStream) {
				CFWriteStreamUnscheduleFromRunLoop(info->writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
				(void) CFWriteStreamSetClient(info->writeStream, kCFStreamEventNone, NULL, NULL);
				
				/* CFWriteStreamClose terminates the stream. */
				CFWriteStreamClose(info->writeStream);
				CFRelease(info->writeStream);
			}
			
			/* Part of workaround for bug <rdar://problem/3745574>. CFFTPStream is crashing because it uses the proxyDict
			 after releasing it.  To workaround this bug, we simply retain a copy of the proxyDict and then release it
			 after closing the stream. */
			if (info->proxyDict) {
				CFRelease(info->proxyDict);
			}
			
			free(info);
		}
		
		// list directory
		/* GetListedFileOrDir lists a FTP directory entry, represented by a CFDictionary 
		 as returned by CFFTPCreateParsedResourceListing, as a single line of text, much like 
		 you'd get from "ls -l". */
		static void
		GetListedFileOrDir(CFDictionaryRef dictionary, ListedFileInfoVec& outFileInfos)
		{
			CFNumberRef           cfType;
			CFStringRef           cfName;
			char                  name[256];
			SInt32                type;
			
			if(dictionary == NULL)
			{
				return;
			}
			ListedFileInfo fileInfo;
			
			cfType = (CFNumberRef)CFDictionaryGetValue(dictionary, kCFFTPResourceType);
			if (cfType) 
			{
				CFNumberGetValue(cfType, kCFNumberSInt32Type, &type);
				
				if (type & DT_DIR)
				{
					fileInfo.mIsDir = true;
				}
				else// if (type & DT_REG)
				{
					// Treat other types as a file
					fileInfo.mIsDir = false;
				}
			}
			
			cfName = (CFStringRef)(CFDictionaryGetValue(dictionary, kCFFTPResourceName));
			if (cfName) 
			{				
				// No matter what kind of text encoding in FTP side, mac application will use mac roman encoding natively.
				// So we must consider that if we get strings from the dictionary.
				TextEncoding appTextEncoding = GetApplicationTextEncoding();
				CFStringGetCString(cfName, name, sizeof(name), appTextEncoding);
				
				// Note that we only support FTPs who support UTF8, which is required in RFC-2640,
				// otherwise, we cannot guarantee the name is decoded correctly.
				fileInfo.mName = dvacore::utility::CharAsUTF8(name);
			}
			outFileInfos.push_back(fileInfo);
		}
		
		/* MyDirectoryListingCallBack is the stream callback for the CFFTPStream during a directory 
		 list operation. Its main purpose is to read bytes off the FTP stream, which is returning bytes 
		 of the directory listing, parse them, and 'pretty' print the resulting directory entries. */
		static void
		MyDirectoryListingCallBack(CFReadStreamRef readStream, CFStreamEventType type, void * clientCallBackInfo)
		{
			MyStreamInfo     *info = (MyStreamInfo *)clientCallBackInfo;
			CFIndex          bytesRead;
			CFStreamError    error;
			CFDictionaryRef  parsedDict;
			
			bool* pResult = info->pResult;
			if (readStream == NULL || info == NULL || info->readStream != readStream)
			{
				if (pResult)
				{
					*pResult = false;
				}
				MyCleanupStreamInfo(info);
				CFRunLoopStop(CFRunLoopGetCurrent());
				return;
			}
			
			switch (type) 
			{
					
				case kCFStreamEventOpenCompleted:
					ASL_TRACE("MyDirectoryListingCallBack", 5, "Open Completed!");
					break;
				case kCFStreamEventHasBytesAvailable:
					
					/* When we get here, there are bytes to be read from the stream.  There are two cases:
					 either info->leftOverByteCount is zero, in which case we complete processed the last 
					 buffer full of data (or we're at the beginning of the listing), or 
					 info->leftOverByteCount is non-zero, in which case there are that many bytes at the 
					 start of info->buffer that were left over from the last time that we were called. 
					 By definition, any left over bytes were insufficient to form a complete directory 
					 entry.
					 
					 In both cases, we just read the next chunk of data from the directory listing stream 
					 and append it to our buffer.  We then process the buffer to see if it now contains 
					 any complete directory entries. */
					
					/* CFReadStreamRead will return the number of bytes read, or -1 if an error occurs
					 preventing any bytes from being read, or 0 if the stream's end was encountered. */
					bytesRead = CFReadStreamRead(info->readStream, info->buffer + info->leftOverByteCount, kMyBufferSize - info->leftOverByteCount);
					if (bytesRead > 0) 
					{
						const UInt8 *   nextByte;
						CFIndex         bytesRemaining;
						CFIndex         bytesConsumedThisTime;
						
						/* Parse directory entries from the buffer until we either run out of bytes 
						 or we stop making forward progress (indicating that the buffer does not have 
						 enough bytes of valid data to make a complete directory entry). */
						
						nextByte       = info->buffer;
						bytesRemaining = bytesRead + info->leftOverByteCount;
						do
						{                    
							
							/* CFFTPCreateParsedResourceListing parses a line of file or folder listing
							 of Unix format, and stores the extracted result in a CFDictionary. */
							bytesConsumedThisTime = CFFTPCreateParsedResourceListing(NULL, nextByte, bytesRemaining, &parsedDict);
							if (bytesConsumedThisTime > 0) 
							{
								
								/* It is possible for CFFTPCreateParsedResourceListing to return a positive number 
								 but not create a parse dictionary.  For example, if the end of the listing text 
								 contains stuff that can't be parsed, CFFTPCreateParsedResourceListing returns 
								 a positive number (to tell the calle that it's consumed the data), but doesn't 
								 create a parse dictionary (because it couldn't make sens of the data).
								 So, it's important that we only try to print parseDict if it's not NULL. */
								
								if (parsedDict != NULL) 
								{
									if (info->pFileInfos)
									{
										GetListedFileOrDir(parsedDict, *(info->pFileInfos));
									}
									CFRelease(parsedDict);
								}
								
								nextByte       += bytesConsumedThisTime;
								bytesRemaining -= bytesConsumedThisTime;
								
							} 
							else if (bytesConsumedThisTime == 0) 
							{
								/* This should never happen because we supply a pretty large buffer. 
								 Still, we handle it by leaving the loop, which leaves the remaining 
								 bytes in the buffer. */
							} 
							else if (bytesConsumedThisTime == -1) 
							{
								ASL_TRACE("MyDirectoryListingCallBack", 5, "CFFTPCreateParsedResourceListing parse failure");
								if (pResult)
								{
									*pResult = false;
								}
								MyCleanupStreamInfo(info);
								CFRunLoopStop(CFRunLoopGetCurrent());
							}
							
						} while ( (bytesRemaining > 0) && (bytesConsumedThisTime > 0) );
						
						/* If any bytes were left over, leave them in the buffer for next time. */
						if (bytesRemaining > 0) 
						{
							memmove(info->buffer, nextByte, bytesRemaining);                    
						}
						info->leftOverByteCount = bytesRemaining;
					} 
					else 
					{
						/* If bytesRead < 0, we've hit an error.  If bytesRead == 0, we've hit the end of the 
						 directory listing.  In either case, we do nothing, and rely on CF to call us with 
						 kCFStreamEventErrorOccurred or kCFStreamEventEndEncountered in order for us to do our 
						 clean up. */
					}
					break;
				case kCFStreamEventErrorOccurred:
				{
					error = CFReadStreamGetError(info->readStream);
					ASL_TRACE("MyDirectoryListingCallBack", 5, "CFReadStreamGetError returned " << error.domain << "," << error.error);
					if (pResult)
					{
						*pResult = false;
					}
					MyCleanupStreamInfo(info);
					CFRunLoopStop(CFRunLoopGetCurrent());
				}
					break;
				case kCFStreamEventEndEncountered:
				{
					ASL_TRACE("MyDirectoryListingCallBack", 5, "Listing Completed!");
					if (pResult)
					{
						*pResult = true;
					}
					MyCleanupStreamInfo(info);
					CFRunLoopStop(CFRunLoopGetCurrent());
				}
					break;
				default:
					ASL_TRACE("MyDirectoryListingCallBack", 5, "Received unexpected CFStream event " << type);
					break;
			}
			return;
		}
		
		// Combine ServerName(ASL::String: ftp://10.192.168.1) with the RemoteDir(UTF8String: test/), 
		// the result is UTF8String: ftp:://10.192.168.1/test/.
        static dvacore::UTF8String CombinePath(const ASL::String& inString1, const dvacore::UTF8String& inString2)
        {
            dvacore::UTF16String temp16Str1 = ASL::PathUtils::RemoveTrailingSlash(inString1);
            dvacore::UTF16String temp16Str2 = MZ::Utilities::AddHeadingSlash(dvacore::utility::UTF8to16(inString2));
            return dvacore::utility::UTF16to8(temp16Str1 + temp16Str2);
        }
        
		static CFReadStreamRef CreateReadStreamWithFTPInfo(
								const FTPConnectionDataPtr& inFtpConnection,
								const dvacore::UTF8String& inRemoteDir)
		{
			dvacore::UTF8String utf8FtpPath = CombinePath(inFtpConnection->mServerName, inRemoteDir);
			CFStringRef cfFTPPath = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(utf8FtpPath.c_str()), kCFStringEncodingUTF8);
			CFStringRef cfValidFTPPath = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfFTPPath, NULL, NULL, kCFStringEncodingUTF8);
			CFURLRef urlDestPath = CFURLCreateWithString(kCFAllocatorDefault, cfValidFTPPath, NULL);
            CFRelease(cfFTPPath);
            CFRelease(cfValidFTPPath);
			if (urlDestPath == NULL)
			{
				return NULL;
			}

			/* Create an FTP read stream for downloading operation from an FTP URL. */
			CFReadStreamRef readStream = CFReadStreamCreateWithFTPURL(kCFAllocatorDefault, urlDestPath);
            CFRelease(urlDestPath);
			if (readStream == NULL)
				return NULL;

			// To fix bug #3519034. If client is Mac 10.8 and server is on Mac, we must disable this property,
			// otherwise open stream will cause crash.
			CFReadStreamSetProperty(readStream, kCFStreamPropertyFTPAttemptPersistentConnection, kCFBooleanFalse);

			CFStringRef username = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFtpConnection->mUserName).c_str()), kCFStringEncodingUTF8);
			CFReadStreamSetProperty(readStream, kCFStreamPropertyFTPUserName, username);
			CFStringRef password = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFtpConnection->mPassword).c_str()), kCFStringEncodingUTF8);
			CFReadStreamSetProperty(readStream, kCFStreamPropertyFTPPassword, password);
			CFRelease(username);
			CFRelease(password);
            
            return readStream;
		}

		static CFWriteStreamRef CreateWriteStreamWithFTPInfo(
			const FTPConnectionDataPtr& inFtpConnection,
			const dvacore::UTF8String& inRemoteDir)
		{
			dvacore::UTF8String utf8FtpPath = CombinePath(inFtpConnection->mServerName, inRemoteDir);
			CFStringRef cfFTPPath = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(utf8FtpPath.c_str()), kCFStringEncodingUTF8);
			CFStringRef cfValidFTPPath = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfFTPPath, NULL, NULL, kCFStringEncodingUTF8);
			CFURLRef urlDestPath = CFURLCreateWithString(kCFAllocatorDefault, cfValidFTPPath, NULL);
            CFRelease(cfFTPPath);
            CFRelease(cfValidFTPPath);
			if (urlDestPath == NULL)
			{
				return NULL;
			}

			/* Create an FTP write stream for downloading operation from an FTP URL. */
			CFWriteStreamRef writeStream = CFWriteStreamCreateWithFTPURL(kCFAllocatorDefault, urlDestPath);
            CFRelease(urlDestPath);
			if (writeStream == NULL)
				return NULL;

			// To fix bug #3519034. If client is Mac 10.8 and server is on Mac, we must disable this property,
			// otherwise open stream will cause crash.
			CFWriteStreamSetProperty(writeStream, kCFStreamPropertyFTPAttemptPersistentConnection, kCFBooleanFalse);

			CFStringRef username = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFtpConnection->mUserName).c_str()), kCFStringEncodingUTF8);
			CFWriteStreamSetProperty(writeStream, kCFStreamPropertyFTPUserName, username);
			CFStringRef password = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFtpConnection->mPassword).c_str()), kCFStringEncodingUTF8);
			CFWriteStreamSetProperty(writeStream, kCFStreamPropertyFTPPassword, password);
			CFRelease(username);
			CFRelease(password);
            
            return writeStream;
		}

		/* MySimpleDirectoryListing implements the directory list command.  It sets up a MyStreamInfo 
		 'object' with the read stream being an FTP stream of the directory to list and with no 
		 write stream.  It then returns, and the real work happens asynchronously in the runloop.  
		 The function returns true if the stream setup succeeded, and false if it failed. */
		static bool
		MySimpleDirectoryListing(CFReadStreamRef inReadStream, bool* ioResult, ListedFileInfoVec& inFileInfos)
		{
			CFStreamClientContext  context = { 0, NULL, NULL, NULL, NULL };
			bool				   success = true;
			MyStreamInfo           *streamInfo;
			
			if (inReadStream == NULL)
			{
				return false;
			}
			
			/* Initialize our MyStreamInfo structure, which we use to store some information about the stream. */
			if (!MySetupStreamInfo(&streamInfo, inReadStream, NULL, ioResult, &inFileInfos, NULL, 0))
			{
				CFRelease(inReadStream);
				return false;
			}
			context.info = (void *)streamInfo;
			
			/* CFReadStreamSetClient registers a callback to hear about interesting events that occur on a stream. */
			success = CFReadStreamSetClient(inReadStream, kNetworkEvents, MyDirectoryListingCallBack, &context);
			if (success) 
			{
				/* Schedule a run loop on which the client can be notified about stream events.  The client
				 callback will be triggered via the run loop.  It's the caller's responsibility to ensure that
				 the run loop is running. */
				CFReadStreamScheduleWithRunLoop(inReadStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
				
				/* CFReadStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
				 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
				 listen to the run loop source to find out when the open completes and whether it was successful. */
				success = CFReadStreamOpen(inReadStream);
				if (success == false) 
				{
					MyCleanupStreamInfo(streamInfo);
				}
			} 
			else 
			{
				MyCleanupStreamInfo(streamInfo);
			}
			
			return success;
		}		
		
		static	void
		MyUploadCallBack(CFWriteStreamRef writeStream, CFStreamEventType type, void * clientCallBackInfo)
		{
			MyStreamInfo     *info = (MyStreamInfo *)clientCallBackInfo;
			CFIndex          totalBytesRead = 0, bytesRead, bytesWritten;
			CFStreamError    error;
			
			bool* pResult = info->pResult;
			if (writeStream == NULL || info == NULL || info->writeStream != writeStream)
			{
				if (pResult)
				{
					*pResult = false;
				}
				MyCleanupStreamInfo(info);
				CFRunLoopStop(CFRunLoopGetCurrent());
				return;
			}
			
			switch (type) 
			{
				case kCFStreamEventOpenCompleted:
					ASL_TRACE("MyUploadCallback",5,"Open complete");
					break;
				case kCFStreamEventCanAcceptBytes:
					do 
					{
						/* The first thing we do is check to see if there's some leftover data that we read
						 in a previous callback, which we were unable to upload for whatever reason. */
						if (info->leftOverByteCount > 0) 
						{
							bytesRead = info->leftOverByteCount;
						} 
						else 
						{
							/* CFReadStreamRead will return the number of bytes read, or -1 if an error occurs
							 preventing any bytes from being read, or 0 if the stream's end was encountered. */
							bytesRead = CFReadStreamRead(info->readStream, info->buffer, info->bufferSize);
							if (bytesRead < 0) 
							{
								ASL_TRACE("MyUploadCallback",5,"CFReadStream returned");
								if (pResult)
								{
									*pResult = false;
								}
								MyCleanupStreamInfo(info);
								CFRunLoopStop(CFRunLoopGetCurrent());
							}
							totalBytesRead += bytesRead;
                            ASL_TRACE("MyUploadCallback",7,"total read: " << totalBytesRead);
						}
						
						/* CFWriteStreamWrite returns the number of bytes successfully written, -1 if an error has
						 occurred, or 0 if the stream has been filled to capacity (for fixed-length streams).
						 If the stream is not full, this call will block until at least one byte is written. */
                        bytesWritten = CFWriteStreamWrite(info->writeStream, info->buffer, bytesRead);
						if (bytesWritten > 0)
						{
							info->totalBytesWritten += bytesWritten;
                            /* CFWriteStreamWrite seems to have bug when transfer small file.
                              Add Sleep 1 ms to work around */
                            ASL::Sleep(1);
                            ASL_TRACE("MyUploadCallback",6,"total write: " << info->totalBytesWritten);
							
							UpdateProgressIfNeeded(info->pProgressData, info->fileSizeinBytes, info->totalBytesWritten);
							if (info->pProgressData && info->pProgressData->UpdateProgress)
							{
								if (info->pProgressData->UpdateProgress(0))
								{
                                    ASL_TRACE("MyUploadCallback",5,"User canceled");
									if (pResult)
									{
										*pResult = true;
									}
									MyCleanupStreamInfo(info);
									CFRunLoopStop(CFRunLoopGetCurrent());
									break;
								}
							}

							/* If we couldn't upload all the data that we read, we temporarily store the data in our MyStreamInfo
							 context until our CFWriteStream callback is called again with a kCFStreamEventCanAcceptBytes event. */
							if (bytesWritten < bytesRead) 
							{
								info->leftOverByteCount = bytesRead - bytesWritten;
								memmove(info->buffer, info->buffer + bytesWritten, info->leftOverByteCount);
							} 
							else 
							{
								info->leftOverByteCount = 0;
							}
							if (1 == info->bufferSize)
							{
								/* For first write buffer size is made to one byte to ensure atleast two writes. */
								info->bufferSize = kMyBufferSize;
								/* If the file size is of few bytes and write permission is not there on the server
								 then kCFStreamEventEndEncountered event occurs before kCFStreamEventErrorOccurred.
								 that is why sleep of one second is added to get kCFStreamEventErrorOccurred event.
								 This is not a good and fullproof solution. */
								sleep(1);
							}
						} 
						else 
						{
							if (bytesWritten < 0) 
							{
								ASL_TRACE("MyUploadCallback",5,"CFWriteStreamWrite returned");
								if (pResult)
								{
									*pResult = false;
								}
							}
							
							break;
						}
						
					} while (CFWriteStreamCanAcceptBytes(info->writeStream));
					
					//ASL_TRACE("MyUploadCallback", 5, "Read "<< totalBytesRead << "bytes; Wrote "<< info->totalBytesWritten <<"bytes");
					break;
				case kCFStreamEventErrorOccurred:
				{
					error = CFWriteStreamGetError(info->writeStream);
					ASL_TRACE("MyUploadCallback", 5, "CFReadStreamGetError returned " << error.domain << "," << error.error);
					if (pResult)
					{
						*pResult = false;
					}
					MyCleanupStreamInfo(info);
					CFRunLoopStop(CFRunLoopGetCurrent());
				}
					break;
				case kCFStreamEventEndEncountered:
				{
					ASL_TRACE("MyUploadCallback", 5, "Upload complete");
					if (pResult)
					{
						*pResult = true;
					}
					MyCleanupStreamInfo(info);
					CFRunLoopStop(CFRunLoopGetCurrent());
				}
					break;
				default:
					ASL_TRACE("MyUploadCallback", 5, "Received unexpected CFStream event " << type);
					break;
			}
			return;
		}
		
		
		static bool CreateRemoteDir(CFWriteStreamRef inWriteStream, bool* ioResult)
		{
			CFStreamClientContext  context = { 0, NULL, NULL, NULL, NULL };
			MyStreamInfo *streamInfo;

			if (inWriteStream == NULL)
			{
				return false;
			}
            
			if (!MySetupStreamInfo(&streamInfo, NULL, inWriteStream, ioResult, NULL, NULL, 0))
			{
				CFRelease(inWriteStream);
				return false;
			}
			context.info = (void*)streamInfo;
			
			bool success = CFWriteStreamSetClient(inWriteStream, kNetworkEvents, MyUploadCallBack, &context);
			if (success) 
			{
				CFWriteStreamScheduleWithRunLoop(inWriteStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
                
				success = CFWriteStreamOpen(inWriteStream);
				if (!success)
				{
					MyCleanupStreamInfo(streamInfo);
				}
			}
			else
			{
				MyCleanupStreamInfo(streamInfo);
			}
			
			return success;
		}
		
		static	bool
		MySimpleUpload(
			CFWriteStreamRef inWriteStream,
			CFURLRef inLocalPath, 
			bool* ioResult,
			ExportUtils::ExportProgressData* inProgressData,
			ASL::UInt64 inFileSize)
		{
			CFReadStreamRef        readStream;
			CFStreamClientContext  context = { 0, NULL, NULL, NULL, NULL };
			bool				   success = false;
			MyStreamInfo           *streamInfo;
			
			if (inWriteStream == NULL || inLocalPath == NULL)
			{
				return false;
			}
			
			/* Create a CFReadStream from the local file being uploaded. */
			readStream = CFReadStreamCreateWithFile(kCFAllocatorDefault, inLocalPath);
			if (readStream == NULL) 
			{
				return false;
			}
            CFReadStreamSetProperty(readStream, kCFStreamPropertyFTPAttemptPersistentConnection, kCFBooleanFalse);
			ASL_TRACE("FTPUtils", 7, "SimpleUpload(), size " << inFileSize);
            
			/* Initialize our MyStreamInfo structure, which we use to store some information about the stream. */
			if (!MySetupStreamInfo(&streamInfo, readStream, inWriteStream, ioResult, NULL, inProgressData, inFileSize))
			{
				CFRelease(readStream);
				CFRelease(inWriteStream);
				return false;
			}
			context.info = (void *)streamInfo;
			
			/* CFReadStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
			 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
			 listen to the run loop source to find out when the open completes and whether it was successful. */
			success = CFReadStreamOpen(readStream);
			if (success) 
			{
				/* CFWriteStreamSetClient registers a callback to hear about interesting events that occur on a stream. */
				success = CFWriteStreamSetClient(inWriteStream, kNetworkEvents, MyUploadCallBack, &context);
				if (success) 
				{
					/* Schedule a run loop on which the client can be notified about stream events.  The client
					 callback will be triggered via the run loop.  It's the caller's responsibility to ensure that
					 the run loop is running. */
					CFWriteStreamScheduleWithRunLoop(inWriteStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
					
					/* CFWriteStreamOpen will return success/failure.  Opening a stream causes it to reserve all the
					 system resources it requires.  If the stream can open non-blocking, this will always return TRUE;
					 listen to the run loop source to find out when the open completes and whether it was successful. */
                    
					success = CFWriteStreamOpen(inWriteStream);
					if (success == false) 
					{
						MyCleanupStreamInfo(streamInfo);
					}
				} 
				else 
				{
					MyCleanupStreamInfo(streamInfo);
				}
			} 
			else 
			{
				MyCleanupStreamInfo(streamInfo);
			}
			
			return success;
		}
		
		static bool
		FTPFileExistInternal(const FTPConnectionDataPtr& inFTPConnection, dvacore::UTF8String const& inFTPPath, dvacore::UTF8String const& inFileName, bool inIsDirectory)
		{
			if (inFileName == dvacore::utility::CharAsUTF8("."))
			{
				// this is the case that check current folder.
				return true;
			}

			bool result = false;
			ListedFileInfoVec fileInfos;
			
			CFReadStreamRef readStream = CreateReadStreamWithFTPInfo(inFTPConnection, inFTPPath);
			if (readStream == NULL)
			{
				return false;
			}

			if (MySimpleDirectoryListing(readStream, &result, fileInfos))
			{
				CFRunLoopRun();
			}
			
			result = IsFTPFileExist(fileInfos, inFileName, inIsDirectory);
			
			return result;
		}
		
		/**
		 **  inFTPDestDir should not include file
		 **  isCreate indicates whether we create the folder if it does not exist or just navigate
		 **/
		static bool
		FTPMakeDirInternal(
			FTPConnectionDataPtr const& inFTPConnection,
			dvacore::UTF8String const& inFTPDestDir, 
			dvacore::UTF8String& outUTF8FTPPath, 
			bool isCreate)
		{
			bool result = true;
			dvacore::UTF8String utf8DstPath = MZ::Utilities::AddTrailingForwardSlash(inFTPDestDir);

			dvacore::UTF8String::size_type slashPos = utf8DstPath.find(dvacore::utility::CharAsUTF8("/"));
			while ( slashPos != dvacore::UTF8String::npos )
			{
				dvacore::UTF8String const& eachDir = utf8DstPath.substr(0, slashPos);
				if (!eachDir.empty())
				{
					if (!isCreate)
					{
						if (!FTPFileExistInternal(inFTPConnection, outUTF8FTPPath, eachDir, true))
						{
							return false;
						}
					}
					
					// create dir here
					outUTF8FTPPath += eachDir + dvacore::utility::CharAsUTF8("/");

					if (isCreate)
					{
						CFWriteStreamRef writeStream = CreateWriteStreamWithFTPInfo(inFTPConnection, outUTF8FTPPath);
						if (writeStream == NULL)
						{
							return false;
						}

						if ( CreateRemoteDir(writeStream, &result) )
						{
							CFRunLoopRun();
						}
					}
				}
				
				utf8DstPath = utf8DstPath.substr(slashPos + 1);
				slashPos = utf8DstPath.find(dvacore::utility::CharAsUTF8("/"));
			}

			return result;
		}
		
		static bool 
		FTPUploadInternal(
			FTPConnectionDataPtr const& inFTPConnection,
			dvacore::UTF8String const& inFileName, 
			dvacore::UTF8String const& inSourcePath, 
			ExportUtils::ExportProgressData* inProgressData,
			ASL::UInt64 inFileSize)
		{
			bool result = false;

			CFWriteStreamRef writeStream = CreateWriteStreamWithFTPInfo(inFTPConnection, inFileName);
			if (writeStream == NULL)
			{
				return false;
			}
			CFStringRef cfSourcePath = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(inSourcePath.c_str()), kCFStringEncodingUTF8);
			CFURLRef urlSourcePath = CFURLCreateWithFileSystemPath(NULL, cfSourcePath, kCFURLPOSIXPathStyle, true);
			if ( MySimpleUpload(writeStream, urlSourcePath, &result, inProgressData, inFileSize) )
			{
				CFRunLoopRun();
			}
			
			CFRelease(cfSourcePath);
			CFRelease(urlSourcePath);
			
			return result;
		}

		static bool 
			FTPDeleteInternal(
            FTPConnectionDataPtr const& inFTPConnection,
			dvacore::UTF8String const& inFTPPath,
			dvacore::UTF8String const& inFileName)
		{
            const int ftpStartLenth = 6;
            ASL::String ftpStart = inFTPConnection->mServerName.substr(0,ftpStartLenth);// Get the "ftp://" part
            CFStringRef cfFtpStart = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(ftpStart).c_str()), kCFStringEncodingUTF8);

            // in case there are special charactors in username or password, so we need encode the special charactors.
            CFStringRef cfUsername = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFTPConnection->mUserName).c_str()), kCFStringEncodingUTF8);
			CFStringRef cfValidUsername = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfUsername, NULL, (CFStringRef)@"!*'();:@&=+$,/?%#[]", kCFStringEncodingUTF8);
            CFStringRef cfPassword = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inFTPConnection->mPassword).c_str()), kCFStringEncodingUTF8);
			CFStringRef cfValidPassword = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfPassword, NULL, (CFStringRef)@"!*'();:@&=+$,/?%#[]", kCFStringEncodingUTF8);

            ASL::String addrPart = ASL_STR("@") + inFTPConnection->mServerName.substr(ftpStartLenth); // ip address
			CFStringRef cfAddrPart = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(addrPart).c_str()), kCFStringEncodingUTF8);
            
            CFStringRef cfFTPRoot = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@%@:%@%@"), cfFtpStart, cfValidUsername, cfValidPassword, cfAddrPart);// now it is "ftp://username:password@192.168.0.1

			CFRelease(cfFtpStart);
            CFRelease(cfUsername);
            CFRelease(cfValidUsername);
            CFRelease(cfPassword);
            CFRelease(cfValidPassword);
            CFRelease(cfAddrPart);
            
            if (!inFTPPath.empty())
            {
                ASL::String temp16Path = MZ::Utilities::AddHeadingSlash(dvacore::utility::UTF8to16(inFTPPath));
                temp16Path = ASL::PathUtils::RemoveTrailingSlash(temp16Path);
                CFStringRef cfFTPPath = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(dvacore::utility::UTF16to8(temp16Path).c_str()), kCFStringEncodingUTF8);
                CFStringRef cfValidFTPPath = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfFTPPath, NULL, NULL, kCFStringEncodingUTF8);
                CFStringRef cfFTPRootTemp = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@%@"), cfFTPRoot, cfValidFTPPath);// now it is "ftp://username:password@192.168.0.1/folder

                CFRelease(cfFTPRoot);
                CFRelease(cfFTPPath);
                CFRelease(cfValidFTPPath);
                cfFTPRoot = cfFTPRootTemp;
            }
            
			CFStringRef cfFileName = CFStringCreateWithCString(NULL, reinterpret_cast<const char*>(inFileName.c_str()), kCFStringEncodingUTF8);
			CFStringRef cfValidFileName = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, cfFileName, NULL, NULL, kCFStringEncodingUTF8);
            CFStringRef cfFTPFullPath = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@/%@"), cfFTPRoot, cfValidFileName);// now it is "ftp://username:password@192.168.0.1/folder/file
			CFURLRef uploadURL = CFURLCreateWithString(kCFAllocatorDefault, cfFTPFullPath, NULL);
			CFRelease(cfFTPFullPath);
			CFRelease(cfFileName);
            CFRelease(cfValidFileName);

			SInt32 deleteRes = -1;
			if (uploadURL)
			{
				// As saying in Mac reference, if delete a folder, only in the case that the folder is empty
				CFURLDestroyResource(uploadURL, &deleteRes);
				CFRelease(uploadURL);
			}

			return (deleteRes == 0) ? true : false;
		}

		bool CreateFTPConnection(
								 FTPConnectionDataPtr& outFTPConnection,
								 ASL::String const& inServer,
								 ASL::UInt32 inPort,
								 ASL::String const& inRemoteDirectory,
								 ASL::String const& inUser,
								 ASL::String const& inPassword)
		{
			try 
			{
				if (!outFTPConnection) 
				{
					outFTPConnection = FTPConnectionDataPtr(new FTPConnectionData());
				}
				
				outFTPConnection->mServerName = AdjustServerName(inServer);
				outFTPConnection->mPort = inPort;
				outFTPConnection->mRemoteDirectory = MZ::Utilities::AdjustToForwardSlash(inRemoteDirectory);
				outFTPConnection->mUserName = inUser;
				outFTPConnection->mPassword = inPassword;
			}
			catch (...) 
			{
				return false;
			}
			
			return true;
		}
		void CloseFTPConncection(const FTPConnectionDataPtr& inFTPConnection)
		{
			// Do nothing on mac
		}

		bool FTPDelete(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDestPath)
		{
			if (!ValidFTPConnection(inFTPConnection) || inDestPath.empty()) 
			{
				return false;
			}

			ASL::String const& remotePath = ASL::PathUtils::RemoveTrailingSlash(inDestPath);
			ASL::String const& navigateToHere = ASL::PathUtils::GetDirectoryPart(remotePath);
			ASL::String const& filePart = ASL::PathUtils::GetFullFilePart(remotePath);

			dvacore::UTF8String utf8FTPPath;			
			if ( FTPMakeDirInternal(inFTPConnection, dvacore::utility::UTF16to8(navigateToHere), utf8FTPPath, false) )
			{
				return FTPDeleteInternal(inFTPConnection, utf8FTPPath, dvacore::utility::UTF16to8(filePart));
			}

			return false;
		}
		
		bool FTPUpload(
					   const FTPConnectionDataPtr& inFTPConnection,
					   const ASL::String& inDestPath,
					   const ASL::String& inSourcePath,
					   void* inProgressData)
		{
			if (!ValidFTPConnection(inFTPConnection) || inDestPath.empty() || inSourcePath.empty()) 
			{
				return false;
			}
			bool result = false;
			
			dvacore::UTF8String utf8FTPPath;
			FTPMakeDirInternal(inFTPConnection, 
				dvacore::utility::UTF16to8(ASL::PathUtils::GetDirectoryPart(inDestPath)),
				utf8FTPPath,
				true);
			
			ASL::UInt64 fileSize;
			ASL::File::SizeOnDisk(inSourcePath, fileSize);
			result = FTPUploadInternal(inFTPConnection, 
									   utf8FTPPath + dvacore::utility::UTF16to8(ASL::PathUtils::GetFullFilePart(inDestPath)), 
									   dvacore::utility::UTF16to8(inSourcePath),
									   (ExportUtils::ExportProgressData*)inProgressData,
									   fileSize);
			
			return result;
		}
		
		bool FTPMakeDir(
						const FTPConnectionDataPtr& inFTPConnection,
						const ASL::String& inDir)
		{
			if (!ValidFTPConnection(inFTPConnection))
			{
				return false;
			}
			
			dvacore::UTF8String utf8FTPPath;
			// it may return false because either no permission or the folder(s) has existed
			// so generally we check the dir existence first.
			return FTPMakeDirInternal(inFTPConnection, 
				dvacore::utility::UTF16to8(inDir),
				utf8FTPPath,
				true);
		}
		
		bool FTPExistOnDisk(
							const FTPConnectionDataPtr& inFTPConnection,
							const ASL::String& inPath,
							bool inIsDirectory)
		{
			if (!ValidFTPConnection(inFTPConnection))
			{
				return false;
			}
			
			ASL::String const& remotePath = ASL::PathUtils::RemoveTrailingSlash(inPath);
			ASL::String const& navigateToHere = ASL::PathUtils::GetDirectoryPart(remotePath);
			ASL::String const& filePart = ASL::PathUtils::GetFullFilePart(remotePath);
			
			dvacore::UTF8String utf8FTPPath;			
			if ( FTPMakeDirInternal(inFTPConnection, dvacore::utility::UTF16to8(navigateToHere), utf8FTPPath, false) )
			{
				return FTPFileExistInternal(inFTPConnection, utf8FTPPath, dvacore::utility::UTF16to8(filePart), inIsDirectory);
			}
			
			return false;
		}
#endif	//ASL_TARGET_OS_WIN

		bool FTPTestInternal(		
			ASL::String const& inServer,
			ASL::UInt32 inPort,
			ASL::String const& inRemoteDirectory,
			ASL::String const& inUser,
			ASL::String const& inPassword,
			ASL::String& outErrorMsg)
		{
			FTPConnectionDataPtr ftpConnection;
			if ( !CreateFTPConnection(
				ftpConnection,
				inServer,
				inPort,
				inRemoteDirectory,
				inUser,
				inPassword) )
			{
				outErrorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/FTPUtils/Failure/FTPGenericError=Cannot connect to the remote server. Please check the settings.");
				return false;
			}

			ASL::String const& tmpName = ASL::PathUtils::AddTrailingSlash(ASL::PathUtils::GetTempDirectory()) + ASL_STR("ftpTest");
			ASL::String const& uniqueName = ASL::PathUtils::MakeUniqueFilename(tmpName);
			ASL::StdString const& uniqueNameStd =  ASL::MakeStdString(uniqueName);
			FILE* ftpfile = fopen(uniqueNameStd.c_str(), "w");
			char* simpleText = "Test for Prelude FTP export.\n";
			fwrite(simpleText, strlen(simpleText), 1, ftpfile);
			fclose(ftpfile); /* close the local file */

			bool result = false;
			if ( FTPUpload(
				ftpConnection,
				ASL::PathUtils::GetFullFilePart(uniqueName),
				uniqueName,
				NULL))
			{
				if (FTPDelete(ftpConnection, ASL::PathUtils::GetFullFilePart(uniqueName)))
				{
					outErrorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/FTPUtils/Failure/TestOK=The connection has been successfully verified.");
					result = true;
				}
				else
				{
					outErrorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/FTPUtils/Failure/FTPTestFileDeleteError=Test delete failed. Please check the user's delete permissions.");
				}
			}
			else
			{
				if (FTPExistOnDisk(ftpConnection, ASL::PathUtils::GetFullFilePart(uniqueName), false))
				{
					outErrorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/FTPUtils/Failure/FTPTestFileReplaceError=Test overwrite failed. Please check the user's write and delete permissions.");
				}
				else
				{
					outErrorMsg = dvacore::ZString("$$$/Prelude/Mezzanine/FTPUtils/Failure/FTPGenericError=Cannot connect to the remote server. Please check the settings.");
				}
			}

			ASL::File::Delete(uniqueName);
			CloseFTPConncection(ftpConnection);
			return result;
		}


		class FTPTestHelper
		{
		public:
			FTPTestHelper(
				ASL::String const& inServer,
				ASL::UInt32 inPort,
				ASL::String const& inRemoteDirectory,
				ASL::String const& inUser,
				ASL::String const& inPassword)
				:
				mServer(inServer),
				mPort(inPort),
				mRemoteDirectory(inRemoteDirectory),
				mUser(inUser),
				mPassword(inPassword),
				mResult(false)
			{
				ASL::AtomicExchange(mIsProcessing, 0);
			}

			void Start()
			{
				mResult = FTPTestInternal(
					mServer,
					mPort,
					mRemoteDirectory,
					mUser,
					mPassword,
					mErrorMsg);

				ASL::AtomicExchange(mIsProcessing, 0);
			}

			void DoTest()
			{
				ASL::AtomicExchange(mIsProcessing, 1);

				dvacore::threads::AsyncExecutorPtr threadedWorkQueue = dvacore::threads::CreateIOBoundExecutor();
				if (threadedWorkQueue)
				{
					threadedWorkQueue->CallAsynchronously(boost::bind(&FTPTestHelper::Start, this));

					while (ASL::AtomicRead(mIsProcessing) != 0)
					{
						ASL::Sleep(10);
					}
				}
			}

			bool GetResult(ASL::String& outErrorMsg) const
			{
				outErrorMsg = mErrorMsg;
				return mResult;
			}

		private:
			ASL::String			mServer;
			ASL::UInt32			mPort;
			ASL::String			mRemoteDirectory;
			ASL::String			mUser;
			ASL::String			mPassword;

			ASL::String			mErrorMsg;
			ASL::AtomicInt		mIsProcessing;
			bool				mResult;
		};


		bool FTPTest(		
			ASL::String const& inServer,
			ASL::UInt32 inPort,
			ASL::String const& inRemoteDirectory,
			ASL::String const& inUser,
			ASL::String const& inPassword,
			ASL::String& outErrorMsg)
		{
#if ASL_TARGET_OS_WIN
			return FTPTestInternal(
					inServer,
					inPort,
					inRemoteDirectory,
					inUser,
					inPassword,
					outErrorMsg);
#else
			FTPTestHelper testHelper(
							inServer,
							inPort,
							inRemoteDirectory,
							inUser,
							inPassword);

			testHelper.DoTest();
			return testHelper.GetResult(outErrorMsg);
#endif
		}

	}
}
