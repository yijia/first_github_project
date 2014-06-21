/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2011 Adobe Systems Incorporated                       */
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

#ifndef MZWRITEXMPTODISKCACHE_H
#define MZWRITEXMPTODISKCACHE_H

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLRESULT_H
#include "ASLResult.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMACROS_H
#include "ASLMacros.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef BE_MEDIA_IMEDIAMETADATA_H
#include "BE/Media/IMediaMetaData.h"
#endif

namespace PL
{
	class WriteXMPToDiskCache
		:
		public ASL::Listener
	{
	public:
		typedef boost::shared_ptr<WriteXMPToDiskCache>	SharedPtr;

		ASL_MESSAGE_MAP_DECLARE();

		/*
		**
		*/
		PL_EXPORT
		static void Initialize();

		/*
		**
		*/
		PL_EXPORT
		static void Terminate();

		/*
		**
		*/
		PL_EXPORT
		void RestoreCachedXMP();

		/*
		**
		*/
		bool GetCachedXMPData(ASL::String const& inMediaPath, ASL::StdString& outXMPString);

		/*
		**
		*/
		ASL::Result RegisterWriting(
					ASL::String const& inMediaPath,
					ASL::Guid const& inAssetID,
					ASL::StdString const& inXMPString,
					ASL::SInt32 inRequestID,
					bool writeCacheFile = true);

		/*
		**
		*/
		ASL::Result UnregisterWriteRequest(
					ASL::String const& inMediaPath, 
					ASL::SInt32 inRequestID,
					ASL::Result inResult);

		/*
		**
		*/
		PL_EXPORT
		static WriteXMPToDiskCache::SharedPtr GetInstance();

		/*
		**
		*/
		~WriteXMPToDiskCache();

		/*
		**
		*/
		ASL::PathnameList GetCachedXMPMediaPathList();

		/*
		**
		*/
		PL_EXPORT
		bool ExistCache(ASL::String const& inMediaPath);

	private:

		/*
		**
		*/
		void OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String inMediaPath, const BE::RequestIDs& inRequestIDs);

		/*
		**
		*/
		bool ContainsRegisterRequestID(dvacore::UTF16String inMediaPath, const BE::RequestIDs& requestIDs);

		ASL_NO_DEFAULT(WriteXMPToDiskCache);
		ASL_NO_ASSIGNMENT(WriteXMPToDiskCache);
		ASL_NO_COPY(WriteXMPToDiskCache);

		ASL::StationID								mStationID;
		static WriteXMPToDiskCache::SharedPtr		sWriteXMPToDiskCache;


	};
}

#endif