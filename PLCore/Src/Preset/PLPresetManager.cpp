/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
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

// ASL
#include "ASLClass.h"
#include "ASLFile.h"
#include "ASLStationUtils.h"
#include "ASLStationRegistry.h"
#include "ASLClassFactory.h"

// DVA
#include "dvacore/filesupport/file/File.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/filesupport/file/FileException.h"
#include "dvacore/debug/Debug.h"

#include "boost/foreach.hpp"

// Local
#include "Preset/PresetManagerMessages.h"
#include "Preset/PLPresetManagerStation.h"
#include "Preset/PLPresetManager.h"
#include "Preset/FileBasedPLPresetHandler.h"

namespace PL
{
/*
**
*/
PLPresetManager::PLPresetManager()
	:
	mStationID(GetPresetManagerStationID())
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
}

/*
**
*/
PLPresetManager::~PLPresetManager()
{
}

/*
**
*/
ASL::Result PLPresetManager::Initialize(
	const std::vector<dvacore::UTF16String>& inSystemPresetFolders, 
	const dvacore::UTF16String& inUserPresetFolder,
	const PLPresetCategory inCategory)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	std::auto_ptr<PLPresetHandlerFactory> presetHandlerFactory(
		new FileBasedPLPresetHandlerFactory(inUserPresetFolder));

	return Initialize(inSystemPresetFolders, presetHandlerFactory, inCategory);
}

/*
**
*/
ASL::Result PLPresetManager::Initialize(
	const std::vector<dvacore::UTF16String>& inSystemPresetFolders,
	std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
	const PLPresetCategory inCategory)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	mSystemPresetFolders = inSystemPresetFolders;
	mPresetHandler = inPresetHandlerFactory->GetPresetHandler(inCategory);
	mPresetCategory = inCategory;

	return ASL::kSuccess;
}

/*
**
*/
ASL::Result PLPresetManager::Initialize(
	const dvacore::UTF16String& inSystemPresetFolder, 
	const dvacore::UTF16String& inUserPresetFolder,
	const PLPresetCategory inCategory)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent thread
	std::auto_ptr<PLPresetHandlerFactory> presetHandlerFactory(
		new FileBasedPLPresetHandlerFactory(inUserPresetFolder));
	return Initialize(inSystemPresetFolder, presetHandlerFactory, inCategory);
}

/*
**
*/
ASL::Result PLPresetManager::Initialize(
	const dvacore::UTF16String& inSystemPresetFolder,
	std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
	const PLPresetCategory inCategory)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	mSystemPresetFolders.clear();
	mSystemPresetFolders.push_back(inSystemPresetFolder);
	mPresetHandler = inPresetHandlerFactory->GetPresetHandler(inCategory);
	mPresetCategory = inCategory;

	SystemPresetFoldersPathVector::iterator  iter ;
	for(iter = mSystemPresetFolders.begin(); iter!= mSystemPresetFolders.end(); iter++)
	{
		dvacore::filesupport::Dir systemPresetFolder(*iter);
		if(!systemPresetFolder.Exists())
		{
			DVA_TRACE("PLPresetManager::Initialize", 5, "System preset folder does not exist");
		}
	}
	return ASL::kSuccess;
}

IPLPresetRef PLPresetManager::CreatePreset()
{
	return mPresetHandler->CreatePreset();
}

/*
**
*/
IPLPresets PLPresetManager::GetUserPresets(bool inUseCache)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	return mPresetHandler->GetUserPresets(inUseCache);
}

/*
**
*/
IPLPresets PLPresetManager::GetSystemPresets()
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads

	class FilterSystemPresets 
		: 
		public FileBasedPLPresetHandler::PLPresetEnumerator
	{
	public:
		FilterSystemPresets(AbstractPLPresetHandler* inHandler)
			:
			mHandler(inHandler)
		{
		}

		virtual bool operator ()(IPLPresetRef inPreset)
		{
			return mHandler->AllowSystemPreset(inPreset);
		}

	protected:
		AbstractPLPresetHandler* mHandler;
	}
	systemPresetEnumerator(mPresetHandler.get());

	IPLPresets	presets = FileBasedPLPresetHandler::GetPresets(
		mSystemPresetFolders, 
		&systemPresetEnumerator,
		mPresetHandler->GetCategory(),
		kPLPresetType_SYSTEM,
		false,
		true);
	return presets;
}

/*
 **
 */
ASL::Result PLPresetManager::ReplaceAsUserPreset(IPLPresetRef inOldPresetRef, IPLPresetRef inOutNewPresetRef, const dvacore::UTF16String& inNewPresetSavePath)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::Result	result	= mPresetHandler->ReplaceAsUserPreset(inOldPresetRef, inOutNewPresetRef, inNewPresetSavePath);

	if (ASL::ResultSucceeded(result))
	{
		ASL::StationUtils::BroadcastMessage(mStationID,	PresetListChangedMessage_ReplaceUserPreset(inOldPresetRef, inOutNewPresetRef));		
	}
	
	return result;
}
	
/*
**
*/
ASL::Result PLPresetManager::SaveAsUserPreset(
	IPLPresetRef inPresetRef,
	const dvacore::UTF16String& inPresetName,
	const dvacore::UTF16String& inPresetComment,
	bool inOverwriteOnExist,
	const dvacore::UTF16String& inFileName)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::Result	result = ASL::kSuccess;
	
	result = mPresetHandler->SaveAsUserPreset(
		inPresetRef,
		inPresetName,
		inPresetComment,
		inOverwriteOnExist,
		inFileName);
		
	if (ASL::ResultSucceeded(result))
	{
	}
	
	return result;
}

/**
**	Import a given preset file. Fails if file does not exist.
**	@param inPrestFileName	The Preset file name to be loaded
*/
IPLPresetRef PLPresetManager::ImportPresetFromFile(
	const dvacore::UTF16String& inPresetFileName)
{
	// Copy the preset to the user's folder first? Need more input.
	return FileBasedPLPresetHandler::ImportPreset(
		inPresetFileName,
		mPresetCategory);
}

/*
**
*/
ASL::Result PLPresetManager::DeleteUserPreset(
	IPLPresetRef inPresetRef)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::Result	result = ASL::eUnknown;
	
	if ( inPresetRef->GetPresetType() == kPLPresetType_SYSTEM )
	{
		return ePresetNoRightToDelete;
	}

	result = mPresetHandler->DeleteUserPreset(inPresetRef);
		
	if (ASL::ResultSucceeded(result))
	{
	}
	
	return result;
}

/*
**
*/
ASL::Result PLPresetManager::DeleteUserPreset(
	IPLPresets inPresets)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::Result	result = ASL::kSuccess;
	IPLPresets::iterator end = inPresets.end();
	for (IPLPresets::iterator it = inPresets.begin(); it != end; ++it)
	{
		result = mPresetHandler->DeleteUserPreset(*it);
		ASL_ASSERT_MSG(ASL::ResultSucceeded(result), "Error in deleting preset.");
	}

	if (ASL::ResultSucceeded(result) && inPresets.size())
	{
	}
	
	return result;
}

/*
**	Loads the placeholder preset
*/
IPLPresetRef PLPresetManager::GetPlaceHolderPreset()
{
	return IPLPresetRef();
}

/*
** Saves a preset into the place holder preset.
*/
ASL::Result PLPresetManager::SavePlaceHolderPreset(
	IPLPresetRef inPresetRef)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	return mPresetHandler->SavePlaceHolderPreset(inPresetRef);
}

/**
** Return a fullpath to the user presets, can be empty if not a filebased one
*/
dvacore::UTF16String PLPresetManager::GetUserPresetsLocation()
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	return mPresetHandler->GetUserPresetsLocation();
}
	
/*
**
*/
void PLPresetManager::AddListener(
	ASL::Listener* inListener)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::StationUtils::AddListener(mStationID, inListener);
}

/*
**
*/
void PLPresetManager::RemoveListener(
	ASL::Listener* inListener)
{
	ASL::CriticalSectionLock lock(m_rg); //serialize concurrent threads
	ASL::StationUtils::RemoveListener(mStationID, inListener);
}

}
