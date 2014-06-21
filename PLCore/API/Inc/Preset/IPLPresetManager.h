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

#ifndef MZ_IPLPRESETMANAGER_H
#define MZ_IPLPRESETMANAGER_H

// ASLFoundation
#include "ASLClass.h"
#include "ASLResult.h"
#include "ASLListener.h"

// DVA
#include "dvacore/config/UnicodeTypes.h"

// local
#include "Preset/IPLPreset.h"

namespace PL
{
/**
**
*/

ASL_DEFINE_IREF(IPLPresetManager);
ASL_DEFINE_IID(IPLPresetManager, 0x8d0674b8, 0xeee, 0x4deb, 0x83, 0x27, 0x82, 0x26, 0xb6, 0xd, 0xda, 0x64);

/**
**	Manages the preset management related operations
**	Boradcasts messages (see PresetManagerMessages.h)
*/
ASL_INTERFACE IPLPresetManager 
	: 
	public ASL::ASLUnknown
{
public:
	/**
	**	Create a new Preset
	*/
	virtual IPLPresetRef CreatePreset() = 0;

	/**
	**	Returns the list of user presets known to the system.
	*/
	virtual IPLPresets GetUserPresets(bool inUseCache = false) = 0;
	
	/**
	**  Saves a preset into the place holder preset.
	*/
	virtual IPLPresets GetSystemPresets() = 0;

	/**
	**	Returns the list of user presets known to the system.
	*/
	//virtual PresetProxyList GetUserProxyPresets() = 0;

	/**
	**  Saves a preset into the place holder preset.
	*/
	//virtual PresetProxyList GetSystemProxyPresets() = 0;

	/**
	**	ReplaceAsUserPreset a given user preset with another one. Keeps the name and the storage location.
	**	Fails if a preset is not a changeable userpreset
	**	@param inOldPresetRef		The preset to delete/replace
	**	@param inOutNewPresetRef	The preset to replace with
	**	@param inNewPresetSavePath	A poosible new used save path for this preset, othervise the exact same old name will be used
	*/
	virtual ASL::Result ReplaceAsUserPreset(
		IPLPresetRef inOldPresetRef,
		IPLPresetRef inOutNewPresetRef,
		const dvacore::UTF16String& inNewPresetSavePath = dvacore::UTF16String()) = 0;

	/**
	**	Save a given preset as a user preset. Fails if a preset of the same 
	**	name already exists.
	**	@param inPreset			The preset in question
	**	@param ioPresetName		The name to save the preset with
	*/
	virtual ASL::Result SaveAsUserPreset(
		IPLPresetRef inPreset,
		const dvacore::UTF16String& inPresetName,
		const dvacore::UTF16String& inPresetComments,
		bool inOverwriteOnExist = false,
		const dvacore::UTF16String& inFileName = dvacore::UTF16String()) = 0;

	/**
	**	Import a given preset file. Fails if file does not exist.
	**	@param inPrestFileName	The Preset file name to be loaded
	*/
	virtual IPLPresetRef ImportPresetFromFile(
		const dvacore::UTF16String& inPresetFileName) = 0;
	
	/**
	**	Delete a given user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresetRef inPreset) = 0;
				
	/**
	**	Delete multiple user presets.
	**	@param inPresetRef		The presets in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresets inPreset) = 0;

	/**
	**  Adds listener
	**  @param inListener is pointer to listener
	*/
	virtual void AddListener(
		ASL::Listener* inListener) = 0;
	
	/**
	**  Removes listener
	**  @param inListener is pointer to listener
	*/
	virtual void RemoveListener(
		ASL::Listener* inListener) = 0;	
		
	/**
	**	Loads the placeholder preset
	*/
	virtual IPLPresetRef GetPlaceHolderPreset() = 0;

	/**
	** Saves a preset into the place holder preset.
	*/
	virtual ASL::Result SavePlaceHolderPreset(
		IPLPresetRef inPresetRef) = 0;				

	/**
	** Return a fullpath to the user presets, can be empty if not a filebased one
	*/
	virtual dvacore::UTF16String GetUserPresetsLocation() = 0;
	
};

PL_EXPORT IPLPresetManagerRef InstantiatePLPresetManager(PLPresetCategory inCategory);
PL_EXPORT IPLPresetFieldRef CreatePresetField();
PL_EXPORT IPLPresetFieldRef CreatePresetField(PLPresetFieldType inType);
PL_EXPORT IPLPresetFieldRef CreatePresetField(const IPLPresetFieldRef inField);

PL_EXPORT dvacore::UTF16String GetRenamePresetFileExtension();
PL_EXPORT dvacore::UTF16String GetRenamePresetTypeString();

PL_EXPORT dvacore::UTF16String GetMetadataPresetFileExtension();
PL_EXPORT dvacore::UTF16String GetMetadataPresetTypeString();

PL_EXPORT std::vector<dvacore::UTF16String> GetPredefinedFieldValues(PLPresetFieldType inType);

}

#endif