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

#ifndef PLMEDIAMONITORCACHE_H
#define PLMEDIAMONITORCACHE_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef ASLMACROS_H
#include "ASLMacros.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef BE_MEDIA_IMEDIAMETADATA_H
#include "BE/Media/IMediaMetaData.h"
#endif

// boost
#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#define TXMP_STRING_TYPE	dvacore::StdString

#if ASL_TARGET_OS_MAC
#define MAC_ENV 1
#endif
#if ASL_TARGET_OS_WIN
#define WIN_ENV 1
#endif

#ifndef XMP_INCLUDE_XMPFILES
#define XMP_INCLUDE_XMPFILES 1
#endif

#include "XMP.hpp"


namespace PL
{
	class SRMediaMonitor
		:
		public ASL::Listener
	{
		ASL_MESSAGE_MAP_DECLARE();

	public:
		typedef boost::shared_ptr<SRMediaMonitor>	SharedPtr;

		typedef std::set<ASL::String> MediaFileMonitorSet;

		/**
		**
		*/
		static void Initialize();

		/**
		**
		*/
		static void Terminate();

		/**
		**
		*/
		PL_EXPORT
		static SRMediaMonitor::SharedPtr GetInstance();

		/**
		**
		*/
		PL_EXPORT
		bool RegisterMonitorMediaXMP(ASL::String const& inMediaPath);

		/**
		**
		*/
		PL_EXPORT
		bool UnRegisterMonitorMediaXMP(ASL::String const& inMediaPath);

		/**
		**
		*/
		PL_EXPORT
		void RegisterMonitorFilePath(ASL::String const& inMediaPath);

		/**
		**
		*/
		PL_EXPORT
		void UnRegisterMonitorFilePath(ASL::String const& inMediaPath);

		/**
		**
		*/
		PL_EXPORT
		void Refresh(ASL::PathnameList const& inExclusivePathList = ASL::PathnameList());

		/**
		**
		*/
		PL_EXPORT
		MediaFileMonitorSet GetMediaFileMonitorSet() const;

		/**
		**
		*/
		bool UpdateXMPModTime(ASL::String const& inMediaPath);

		/**
		**
		*/
		bool GetXMPModTime(ASL::String const& inMediaPath, XMP_DateTime& outXMPDate);

		/**
		**
		*/
		bool IsFileMonitored(ASL::String const& inMediaPath) const;

		/**
		**
		*/
		bool RefreshXMPIfChanged(ASL::String const& inMediaPath);

		/**
		**
		*/
		~SRMediaMonitor();

	private:

		/**
		**
		*/
		void RefreshFiles(ASL::PathnameList& outOfflinePathList);

		/**
		**
		*/
		void RefreshMediaXMP(ASL::PathnameList const& inExclusivePathList = ASL::PathnameList());

		/**
		**
		*/
		void OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String const& inMediaParh, BE::RequestIDs);


		ASL_NO_DEFAULT(SRMediaMonitor);
		ASL_NO_ASSIGNMENT(SRMediaMonitor);
		ASL_NO_COPY(SRMediaMonitor);

		static SRMediaMonitor::SharedPtr		sSRMediaMonitor;

	};

	/**
	**	
	*/
	void RefreshBackEndFileCache(const ASL::String& inPath);
}

#endif