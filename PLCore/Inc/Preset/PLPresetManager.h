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
#pragma once


#ifndef MZ_PLPRESETMANAGER_H
#define MZ_PLPRESETMANAGER_H

// ASL
#include "ASLListener.h"
#include "ASLStation.h"

// Local
#include "Preset/IPLPresetManager.h"
#include "Preset/AbstractPLPresetHandler.h"

namespace PL
{

/**
**
*/
ASL_DEFINE_CLASSREF(PLPresetManager, IPLPresetManager);

class PLPresetManager
	:
	public IPLPresetManager
{
	ASL_BASIC_CLASS_SUPPORT(PLPresetManager);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLPresetManager);
	ASL_QUERY_MAP_END
public:
	/**
	**
	*/
	PLPresetManager();

	/**
	**
	*/
	~PLPresetManager();

	/**
	**
	*/
	ASL::Result Initialize(
		const std::vector<dvacore::UTF16String>& inSystemPresetFolders, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory);

	/**
	**
	*/
	ASL::Result Initialize(
		const std::vector<dvacore::UTF16String>& inSystemPresetFolders,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory);

	/**
	**
	*/
	ASL::Result Initialize(
		const dvacore::UTF16String& inSystemPresetFolder, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory);

	/**
	**
	*/
	ASL::Result Initialize(
		const dvacore::UTF16String& inSystemPresetFolder,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory);

	/**
	**
	*/
	IPLPresetRef CreatePreset();

	/**
	**
	*/
	IPLPresets GetUserPresets(bool inUseCache = false);

	/**
	**
	*/
	IPLPresets GetSystemPresets();

	/**
	**	ReplaceAsUserPreset a given user preset with another one. Keeps the name and the storage location.
	**	Fails if a preset is not a changeable userpreset
	**	@param inOldPresetRef		The preset to delete/replace
	**	@param inNewPresetRef		The preset to replace with
	*/
	virtual ASL::Result ReplaceAsUserPreset(
		IPLPresetRef inOldPresetRef,
		IPLPresetRef inNewPresetRef,
		const dvacore::UTF16String& inNewPresetSavePath);
	
	/**
	**	Save a given preset as a user preset. Fails if a preset of the same 
	**	name already exists.
	**	@param inPresetRef		The preset in question
	**	@param ioPresetName		The name to save the preset with
	*/
	virtual ASL::Result SaveAsUserPreset(
		IPLPresetRef inPresetRef,
		const dvacore::UTF16String& inPresetName,
		const dvacore::UTF16String& inPresetComment,
		bool inOverwriteOnExist,
		const dvacore::UTF16String& inFileName);
	
	/**
	**	Import a given preset file. Fails if file does not exist.
	**	@param inPrestFileName	The Preset file name to be loaded
	*/
	virtual IPLPresetRef ImportPresetFromFile(
		const dvacore::UTF16String& inPresetFileName);

	/**
	**	Delete a given user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresetRef inPresetRef);	

	/**
	**	Delete multiple user presets.
	**	@param inPreset			The preset in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresets inPreset);

	/**
	**  Adds listener
	**  @param inListener is pointer to listener
	*/
	virtual void AddListener(ASL::Listener* inListener);
	
	/**
	**  Removes listener
	**  @param inListener is pointer to listener
	*/
	virtual void RemoveListener(ASL::Listener* inListener);

	/**
	**	Loads the placeholder preset
	*/
	virtual IPLPresetRef GetPlaceHolderPreset();

	/**
	** Saves a preset into the place holder preset.
	*/
	virtual ASL::Result SavePlaceHolderPreset(
		IPLPresetRef inPresetRef);

	/**
	** Return a fullpath to the user presets, can be empty if not a filebased one
	*/
	virtual dvacore::UTF16String GetUserPresetsLocation();
	
protected:

protected:
	std::auto_ptr<AbstractPLPresetHandler>	mPresetHandler;

	typedef std::vector<dvacore::UTF16String>	SystemPresetFoldersPathVector;
	SystemPresetFoldersPathVector			mSystemPresetFolders;

	ASL::StationID							mStationID;
	ASL::CriticalSection					m_rg;	//use resource guard to make class methods thread-safe

	PLPresetCategory						mPresetCategory;
};

}

#endif
