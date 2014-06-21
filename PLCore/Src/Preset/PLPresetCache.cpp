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

#include <queue>
#include "boost/foreach.hpp"

// Local
#include "Preset/PLPresetCache.h"
#include "Preset/FileBasedPLPresetHandler.h"

// ASL
#include "ASLClassFactory.h"
#include "ASLResourceUtils.h"
#include "ASLLanguageIDs.h"

// DVA
#include "dvacore/filesupport/file/SafeSave.h"
#include "dvacore/filesupport/file/FileException.h"

// Backend
#include "BE/Core/IXMLFileReader.h"
#include "BE/Core/IXMLFileWriter.h"
#include "BEXML.h"
#include "BEProperties.h"
#include "BEBackend.h"

namespace PL
{
PLPresetCache*	PLPresetCache::sPLPresetCache = NULL;

bool operator<(const PLPresetMapKey& inArg1, const PLPresetMapKey& inArg2)
{
	if(inArg1.mDirPath != inArg2.mDirPath)
		return inArg1.mDirPath < inArg2.mDirPath;
	if(inArg1.mFileType != inArg2.mFileType)
		return inArg1.mFileType < inArg2.mFileType;
	return inArg1.mClassID < inArg2.mClassID;
}

PLPresetCache& PLPresetCache::Get()
{
	if(!sPLPresetCache)
	{
		sPLPresetCache = new PLPresetCache;
	}
	DVA_ASSERT(sPLPresetCache);
	return *sPLPresetCache;
}

void PLPresetCache::Initialize()
{
}

void PLPresetCache::Terminate()
{
	if(sPLPresetCache)
	{
		delete sPLPresetCache;
		sPLPresetCache = NULL;
	}
}

IPLPresets PLPresetCache::GetPresets(
	const dvacore::UTF16String& inDirPath,
	const PLPresetCategory inCategory,
	const PLPresetType inType,
	bool inRecursive)
{
	PLPresetMapKey key;
	key.mDirPath = inDirPath;
	key.mFileType = inCategory;

	if(mPresetMap.find(key) != mPresetMap.end())
		return mPresetMap[key];

	dvacore::filesupport::Dir rootDir(inDirPath);
	if(mSearchedPaths.find(rootDir.FullPath()) != mSearchedPaths.end())
		return IPLPresets(); //We have already searched this path, don't search again.

	class GenericFilter
		: 
		public FileBasedPLPresetHandler::PLPresetEnumerator
	{
	public:
		virtual bool operator ()(IPLPresetRef inPreset)
		{
			return true;
		}
	} genericFilter;

	std::queue<dvacore::filesupport::Dir> dirQueue;
	dirQueue.push(rootDir);
	
	while(!dirQueue.empty())
	{
		dvacore::filesupport::Dir currentDir = dirQueue.front();
		dirQueue.pop();

		if (!currentDir.Exists())
		{
			continue;
		}
		
		IPLPresets presets = 
			FileBasedPLPresetHandler::GetPresets(currentDir.FullPath(), &genericFilter, inCategory, inType, false, false);

		mSearchedPaths.insert(currentDir.FullPath());

		//We have cached all the presets irrespective of the exportermodule. Add them to the appropriate key.
		BOOST_FOREACH(IPLPresetRef thePreset, presets)
		{
			PLPresetMapKey currentKey;
			currentKey.mDirPath = inDirPath;
			currentKey.mFileType = inCategory;
			//currentKey.mClassID = thePreset->GetExporterClassID();
			mPresetMap[currentKey].push_back(thePreset); //Add the preset to its proper place in the map.
		}

		if(inRecursive)
		{
			for(dvacore::filesupport::Dir::DirIterator dirIt = currentDir.BeginSubdirs(false);
				dirIt != currentDir.EndSubdirs(); 
				++dirIt)
			{
				dirQueue.push(*dirIt);
			}
		}
	}
	
	return mPresetMap[key];
}



static const dvacore::UTF16String	kPLPresetCacheFileName(DVA_STR("PLPresetCache.xml"));
static const BE::Key				kPLPresetLanguage(BE_MAKEKEY("PLPresetLanguage"));
static const BE::Key				kPLDirectoryPathKey(BE_MAKEKEY("PLDirectoryPath"));

static const BE::Key				kMapKey(BE_MAKEKEY("Key"));

PLPresetCache::PLPresetCache()
{
	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef properties(backend);

	ASL::UInt16 langID = ASL_MAKELANGID(ASL::kLanguage_English, ASL::kSubLanguage_English_US);
	if (!properties->GetValue(kPLPresetLanguage, langID))
	{
		return;
	}

	if(langID != ASL::GetLanguageID()) 
	{
		return;
	}
	
	// Read from cache
	BE::IXMLFileReaderRef fileReader(ASL::CreateClassInstanceRef(BE::kXMLFileReaderClassID));
	dvacore::filesupport::File cacheFile(dvacore::filesupport::Dir::PrefDir(), kPLPresetCacheFileName);
	if (cacheFile.Exists())
	{
		if ( ASL::ResultSucceeded(fileReader->ReadFromFile(cacheFile.FullPath())) )
		{
			BE::DataReaderPtr rootObj = fileReader->GetDataReader();
			ASL::UInt32 currentIndex = 0;
			BE::DataReaderPtr currentDirReader;
			while (true)
			{
				currentDirReader = rootObj->GetNestedData(currentIndex);
				if (!currentDirReader)
				{
					break;
				}
				
				dvacore::UTF16String dirPath;
				currentDirReader->ReadValue(kPLDirectoryPathKey, dirPath);
				ASL:: UInt32 currentPresetIndex = 0;
				BE::DataReaderPtr currentPresetReader;
				while (true)
				{
					currentPresetReader = currentDirReader->GetNestedData(currentPresetIndex);
					if (!currentPresetReader)
					{
						break;
					}
					
					//ProxyPreset	proxyPreset;
					//proxyPreset.SerializeIn(currentPresetReader->GetVersion(), *currentPresetReader);
					//dvacore::UTF16String	filePath(proxyPreset.GetPresetFilePath());

					//mDirToFilePresetProxyMap[dirPath].insert(FileToProxyPresetMap::value_type(filePath, proxyPreset));
					++currentPresetIndex;
				}

				++currentIndex;
			}
		}
	}
}

PLPresetCache::~PLPresetCache()
{
	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef properties(backend);

	properties->SetValue(kPLPresetLanguage, ASL::GetLanguageID(), true);
	
	// Write back to cache
	BE::IXMLFileWriterRef fileWriter(ASL::CreateClassInstanceRef(BE::kXMLFileWriterClassID));
	dvacore::filesupport::File cacheFile(dvacore::filesupport::Dir::PrefDir(), kPLPresetCacheFileName);
	dvacore::filesupport::SafeSave safeSaveCache(cacheFile);

	BE::DataWriterPtr rootObj = fileWriter->GetDataWriter();
	ASL::UInt32 currentIndex = 0;
	//BOOST_FOREACH (DirToFilePresetProxyMap::value_type entry, mDirToFilePresetProxyMap)
	//{
	//	BE::DataWriterPtr dirWriter = rootObj->CreateNestedData(kMapKey, currentIndex);
	//	dirWriter->WriteValue(kDirectoryPathKey, entry.first);

	//	ASL::UInt32 currentPresetInfoIndex = 0;
	//	BOOST_FOREACH (FileToProxyPresetMap::value_type presetProxyEntry, mDirToFilePresetProxyMap[entry.first])
	//	{
	//		BE::DataWriterPtr presetProxyWriter = dirWriter->CreateNestedData(kMapKey, currentPresetInfoIndex);
	//		presetProxyEntry.second.SerializeOut(*presetProxyWriter);
	//		++currentPresetInfoIndex;
	//	}
	//	
	//	++currentIndex;
	//}

	fileWriter->WriteToFile(safeSaveCache.GetTempFile().FullPath());
	try
	{
		safeSaveCache.Commit();	// atomically commits a change to guard against file corruption
	}
	catch (dvacore::filesupport::file_exception&)
	{
		// Ignore errors if we can't serialize the system preset cache so that we can continue with the exit sequence.
	}
}

std::vector<dvacore::filesupport::File>	PLPresetCache::GetAllFilesInDir(
	dvacore::filesupport::Dir inDir,
	bool inRecursive)
{
	std::vector<dvacore::filesupport::File> fileList;

	std::queue<dvacore::filesupport::Dir> dirQueue;
	dirQueue.push(inDir);

	while(!dirQueue.empty())
	{
		dvacore::filesupport::Dir currentDir = dirQueue.front();
		dirQueue.pop();

		if (!currentDir.Exists())
		{
			continue;
		}
		
		dvacore::filesupport::Dir::FileIterator endFileIt = currentDir.EndFiles();
		for (dvacore::filesupport::Dir::FileIterator fileIt = currentDir.BeginFiles(false);
			fileIt != endFileIt;
			++fileIt)
		{
			fileList.push_back(*fileIt);
		}

		if(inRecursive)
		{
			dvacore::filesupport::Dir::DirIterator endIt = currentDir.EndSubdirs();
			for(dvacore::filesupport::Dir::DirIterator dirIt = currentDir.BeginSubdirs(false);
				dirIt != endIt; 
				++dirIt)
			{
				dirQueue.push(*dirIt);
			}
		}
	}

	return fileList;
}

IPLPresetRef PLPresetCache::GetPreset(
	const dvacore::UTF16String& inPresetFile)
{
	IPLPresetRef preset;
	ASL::String normalizedPath = ASL::PathUtils::ToNormalizedPath(inPresetFile);
	ASL::FileTime modificationTime = 0;
	ASL::File::GetLastModificationTime(normalizedPath, modificationTime);

	{
		ASL::CriticalSectionLock lock(mPresetCriticalSection);
		PresetCacheMap::iterator it = mPresetCache.find(normalizedPath);
		if (it != mPresetCache.end())
		{
			if (modificationTime != it->second.mModificationTime)
			{
				mPresetCache.erase(it);
			}
			else
			{
				preset = it->second.mPreset;
			}
		}
	}

	return preset;
}

void PLPresetCache::CachePreset(
	const dvacore::UTF16String& inPresetFile,
	const IPLPresetRef& inPreset)
{
	bool cloneUniqueID = true;
	ASL::String normalizedPath = ASL::PathUtils::ToNormalizedPath(inPresetFile);

	ASL::FileTime modificationTime = 0;
	if (ASL::ResultSucceeded(ASL::File::GetLastModificationTime(normalizedPath, modificationTime)))
	{
		ASL::CriticalSectionLock lock(mPresetCriticalSection);
		PLPresetRecord record = { modificationTime, inPreset };
		mPresetCache[normalizedPath] = record;
	}
}

}