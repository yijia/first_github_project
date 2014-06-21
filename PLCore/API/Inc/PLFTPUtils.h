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

#ifndef PLFTPUTILITIES_H
#define PLFTPUTILITIES_H

// ASL
#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#if ASL_TARGET_OS_WIN
#include <Windows.h>
#include <WinInet.h>
#endif //ASL_TARGET_OS_WIN

namespace PL
{

namespace FTPUtils
{
#if ASL_TARGET_OS_WIN
	struct FTPConnectionData
	{
		FTPConnectionData() : 
		hInternet(NULL),
		hSession(NULL)
		{
		}

		HINTERNET hInternet;
		HINTERNET hSession;

		// For Mac FTP server on 10.7 or later, default dir is not the root, 
		// so we must cache this dir when setup FTP connection
		dvacore::UTF8String mDefaultDir;
	};
#else	// Data struct on mac
	struct FTPConnectionData
	{
		ASL::String mServerName;
		ASL::UInt32 mPort;
		ASL::String mRemoteDirectory;
		ASL::String mUserName;
		ASL::String mPassword;
	};	
#endif	//ASL_TARGET_OS_WIN

	typedef boost::shared_ptr<FTPConnectionData> FTPConnectionDataPtr;

	bool CreateFTPConnection(
		FTPConnectionDataPtr& outFTPConnection,
		ASL::String const& inServer,
		ASL::UInt32 inPort,
		ASL::String const& inRemoteDirectory,
		ASL::String const& inUser,
		ASL::String const& inPassword);
	void CloseFTPConncection(const FTPConnectionDataPtr& inFTPConnection);

	/**
	** inDestPath should be: RemoteDirectory + SubFolder + FileName, not include server URL
	*/
	bool FTPUpload(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDestPath,
			const ASL::String& inSourcePath,
			void* inProgressData);

	/**
	** inDir should be: RemoteDirectory + SubFolder + SomePath, not include server URL
	*/
	bool FTPMakeDir(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inDir);

	/**
	** inPath should be: RemoteDirectory + SubFolder + PathName(FileName), not include server URL
	*/
	bool FTPExistOnDisk(
			const FTPConnectionDataPtr& inFTPConnection,
			const ASL::String& inPath,
			bool inIsDirectory);

PL_EXPORT
	bool FTPTest(		
		ASL::String const& inServer,
		ASL::UInt32 inPort,
		ASL::String const& inRemoteDirectory,
		ASL::String const& inUser,
		ASL::String const& inPassword,
		ASL::String& outErrorMsg);

} // FTPUtils

} // namespace PL

#endif	//PLFTPUTILITIES_H
