/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2008 Adobe Systems Incorporated                       */
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


#ifndef MZ_PLPRESET_CACHE_H
#define MZ_PLPRESET_CACHE_H

#include <set>

// ASL
#include "ASLTypes.h"
#include "ASLFile.h"
#include "ASLCriticalSection.h"

// DVA
#include "dvacore/filesupport/file/File.h"

// Local
#include "Preset/IPLPreset.h"

namespace PL
{

struct PLPresetMapKey
{
	dvacore::UTF16String	mDirPath;
	ASL::UInt32				mFileType;
	ASL::UInt32				mClassID;
};

struct PLPresetRecord
{
	ASL::FileTime mModificationTime;
	IPLPresetRef mPreset;
};

class PLPresetCache
{
public:
	static PLPresetCache& Get();

	static void Initialize();
	static void Terminate();

	IPLPresets GetPresets(
		const dvacore::UTF16String& inDirPath,
		const PLPresetCategory inCategory,
		const PLPresetType inType,
		bool inRecursive);

	//PresetProxyList GetProxyPresets(
	//	const dvacore::UTF16String& inDirPath);

	IPLPresetRef GetPreset(
		const dvacore::UTF16String& inPresetFile);

	void CachePreset(
		const dvacore::UTF16String& inPresetFile,
		const IPLPresetRef& inPreset);

private:
	PLPresetCache();
	~PLPresetCache();

	std::vector<dvacore::filesupport::File>		GetAllFilesInDir(
		dvacore::filesupport::Dir inDir,
		bool inRecursive);

	static PLPresetCache*						sPLPresetCache;
	std::map<PLPresetMapKey, IPLPresets>			mPresetMap;
	std::set<dvacore::UTF16String>				mSearchedPaths;

	// Disk based preset cache
	//typedef	std::map<dvacore::UTF16String, ProxyPreset>					FileToProxyPresetMap;		// map from preset file path to its info.
	//typedef std::map<dvacore::UTF16String, FileToProxyPresetMap>		DirToFilePresetProxyMap;	// map from dir path to the above map.
	typedef std::map<dvacore::UTF16String, PLPresetRecord>				PresetCacheMap;

	//DirToFilePresetProxyMap				mDirToFilePresetProxyMap;				
	std::set<dvacore::UTF16String>		mSearchedDirs;												// List of dirs that we have searched atleast once.
	PresetCacheMap						mPresetCache;

	ASL::CriticalSection				mPresetCriticalSection;
};

}

#endif