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

#pragma once

#ifndef	PLFTPSETTINGSDATA_H
#define PLFTPSETTINGSDATA_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#include <boost/shared_ptr.hpp>

namespace PL
{

class FTPSettingsData
{

public:

	typedef boost::shared_ptr<FTPSettingsData> SharedPtr;

	/*
	** Setter and Getter methods
	*/
	PL_EXPORT
	ASL::String GetServerName() const 
	{ 
		return mServerName; 
	}

	PL_EXPORT
	void SetServerName(const ASL::String& inServerName) 
	{
		mServerName = inServerName; 
	}

	PL_EXPORT
	ASL::UInt32 GetServerPort() const 
	{
		return mServerPort; 
	}

	PL_EXPORT
	void SetServerPort(ASL::UInt32 inServerPort)
	{ 
		mServerPort = inServerPort;
	}

	PL_EXPORT
	ASL::String GetRemoteDirectory() const 
	{ 
		return mRemoteDirectory;
	}

	PL_EXPORT
	void SetRemoteDirectory(const ASL::String& inRemoteDirectory)
	{
		mRemoteDirectory = inRemoteDirectory;
	}

	PL_EXPORT
	ASL::String GetUserName() const
	{ 
		return mUserName;
	}

	PL_EXPORT
	void SetUserName(const ASL::String& inUserName) 
	{ 
		mUserName = inUserName; 
	}

	PL_EXPORT
	ASL::String GetPassword() const
	{ 
		return mPassword; 
	}

	PL_EXPORT
	void SetPassword(const ASL::String& inPassword)
	{ 
		mPassword = inPassword;
	}

	PL_EXPORT
	ASL::UInt32 GetRetriesNumber() const
	{ 
		return 0; 
	}

	PL_EXPORT
	void SetRetriesNumber(ASL::UInt32 inRetriesNumber)
	{
		mRetriesNumber = inRetriesNumber;
	}

private:

	ASL::String mServerName;

	ASL::UInt32 mServerPort;

	ASL::String mRemoteDirectory;

	ASL::String mUserName;

	ASL::String mPassword;

	ASL::UInt32 mRetriesNumber;
};

} // namespace PL

#endif // PLFTPSETTINGSDATA_H