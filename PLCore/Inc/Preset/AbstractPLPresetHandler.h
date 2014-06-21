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

#ifndef MZ_ABSTRACTPLPRESETHANDLER_H
#define MZ_ABSTRACTPLPRESETHANDLER_H

#include "Preset/IPLPreset.h"

// STL
#include <memory>

namespace PL
{
/**
**
*/
class AbstractPLPresetHandler
{
public:

	virtual IPLPresetRef CreatePreset() = 0;

	/**
	**	Returns the list of user presets known to the system.
	*/
	virtual IPLPresets GetUserPresets(bool inUseCache = false) = 0;

	/**
	**	Return false if you do not want to list a user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual bool AllowUserPreset( 
		IPLPresetRef inPresetRef) = 0;

	/**
	**	Return false if you do not want to list a system preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual bool AllowSystemPreset( 
		IPLPresetRef inPresetRef) = 0;

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
	**	@param inPresetRef		The preset in question
	**	@param ioPresetName		The name to save the preset with
	**	@param inPresetComments	The comments to save the preset with
	*/
	virtual ASL::Result SaveAsUserPreset(
		IPLPresetRef inPresetRef,
		const dvacore::UTF16String& inPresetName,
		const dvacore::UTF16String& inPresetComments,
		bool inOverwriteOnExist = false,
		const dvacore::UTF16String& inFileName = dvacore::UTF16String()) = 0;

	/**
	**	Delete a given user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresetRef inPresetRef) = 0;

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

	/**
	** 
	*/
	virtual PLPresetCategory GetCategory() const = 0;

	virtual ~AbstractPLPresetHandler(){};

	
	/**
	** 
	*/
	static const dvacore::UTF16String kRenamePresetFileExtension;
	static const dvacore::UTF16String kRenamePresetFileExtensionOnly;

	static const dvacore::UTF16String kMetadataPresetFileExtension;
	static const dvacore::UTF16String kMetadataPresetFileExtensionOnly;
};

/**
**
*/
class PLPresetHandlerFactory
{
public:
	virtual ~PLPresetHandlerFactory(){};
	virtual std::auto_ptr<AbstractPLPresetHandler> GetPresetHandler(const PLPresetCategory inCategory) = 0;
};

}

#endif // MZ_ABSTRACTPLPRESETHANDLER_H
