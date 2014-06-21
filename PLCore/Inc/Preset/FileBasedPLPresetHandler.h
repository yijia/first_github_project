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


#ifndef MZ_FILEBASEDPLPRESETHANDLER_H
#define MZ_FILEBASEDPLPRESETHANDLER_H

// ASL
#include "ASLTypes.h"
#include "ASLListener.h"
#include "ASLStation.h"
#include "ASLThreadedWorkQueue.h"

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

// dva
#include "dvacore/config/UnicodeTypes.h"
#include "dvacore/filesupport/file/File.h"

// local
#include "Preset/AbstractPLPresetHandler.h"

namespace PL
{
const ASL::StationID kFileBasePLPresetHandlerWorkerStation (ASL_STATIONID("MZ.Station.FileBasePLPresetHandlerWorkerStation"));

/**
**	Sent when the filebase preset hander has cached presets of a certain folder.
**
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(PresetListCached, dvacore::UTF16String);

/*
** Sent when a preset has been added/deleted/modifed
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(PresetListModified);

class FileBasedPLPresetHandlerWorker;


/**
**
*/
class FileBasedPLPresetHandler
	:
	public AbstractPLPresetHandler
{
	friend class FileBasedPLPresetHandlerWorker;
public:
	/**
	**
	*/
	FileBasedPLPresetHandler(
		const dvacore::UTF16String& inUserPresetFolder,
		PLPresetCategory inCategory);

	/**
	**	The d'tor
	*/
	virtual ~FileBasedPLPresetHandler();

	/*
	**
	*/
	IPLPresetRef CreatePreset();

	/**
	**	Returns the list of user presets known to the system.
	*/
	virtual IPLPresets GetUserPresets(bool inUseCache = false);

	
	//virtual PresetProxyList GetUserProxyPresets();

	/**
	**	Return false if you do not want to list a user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual bool AllowUserPreset( 
		IPLPresetRef inPresetRef);

	/**
	**	Return false if you do not want to list a system preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual bool AllowSystemPreset( 
		IPLPresetRef inPresetRef);

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
		const dvacore::UTF16String& inNewPresetSavePath);
	
	/**
	**	Save a given preset as a user preset. Fails if a preset of the same 
	**	name already exists.
	**	@param inPresetRef		The preset in question
	**	@param inPresetName		The name to save the preset with
	**	@param inPresetComments	The comments to save the preset with
	*/
	virtual ASL::Result SaveAsUserPreset(
		IPLPresetRef inPresetRef,
		const dvacore::UTF16String& inPresetName,
		const dvacore::UTF16String& inPresetComments,
		bool inOverwriteOnExist,
		const dvacore::UTF16String& inFileName);

	/**
	**	Delete a given user preset.
	**	@param inPresetRef		The preset in question
	*/
	virtual ASL::Result DeleteUserPreset(
		IPLPresetRef inPresetRef);

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

	
public:
	/**
	**
	*/
	typedef class PLPresetEnumerator
	{
	public:
		virtual bool operator ()(IPLPresetRef inPreset){return true;}; 

		virtual ~PLPresetEnumerator(){}
	}
	PLPresetEnumerator;

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
	static void SetSystemPresetsFolder(
		const std::vector<dvacore::UTF16String>& inPresetFoldersVector,
		bool inRecurse = true);

	/**
	**
	*/
	static IPLPresets GetPresets(
		const dvacore::UTF16String& inPresetFolder,
		PLPresetEnumerator* inEnumerator,
		PLPresetCategory inCategory,
		PLPresetType inPresetType,
		bool inRecurseSubfolders,
		bool inUseCache = false);


	/**
	**
	*/
	static IPLPresets GetPresets(
		const std::vector<dvacore::UTF16String>& inPresetFoldersVector,
		PLPresetEnumerator* inEnumerator,
		PLPresetCategory inCategory,
		PLPresetType inPresetType,
		bool inRecurseSubfolders,
		bool inUseCache = false);

	/**
	**
	*/
	static IPLPresetRef ReadPresetFromFile(
		const dvacore::UTF16String& inPresetFile,
		PLPresetCategory inCategory,
		bool inUseCache = false);

	/**
	**
	*/
	static ASL::Result WritePresetToFile(
		IPLPresetRef inPreset,
		const dvacore::UTF16String& inPresetFile);

	/**
	 ** 
	 */
	static IPLPresetRef ImportPreset(
		const dvacore::UTF16String& inPresetFullPath, 
		const PLPresetCategory inCategory);

	/**
	 ** 
	 */
	static dvacore::UTF16String GetSystemPresetFolderPath(const PLPresetCategory inCategory);

	/**
	 ** 
	 */
	static dvacore::UTF16String GetUserPresetFolderPath(const PLPresetCategory inCategory);

	/**
	**  Adds listener
	**  @param inListener is pointer to listener
	*/
	//void AddListener(ASL::Listener* inListener)
	//{
	//	mStation.AddListener(inListener);
	//}

	/**
	**  Removes listener
	**  @param inListener is pointer to listener
	*/
	//void RemoveListener(ASL::Listener* inListener)
	//{
	//	mStation.RemoveListener(inListener);
	//}

protected:
	PLPresetCategory GetCategory() const;

	/**
	**
	*/
	dvacore::filesupport::File GetPlaceHolderPresetFile();

	/**
	**
	*/
	//static ASL::ThreadedWorkQueueRef GetWorkQueueRef();

protected:
	const dvacore::UTF16String			mUserPresetFolder;

	ASL::Station						mStation;
	static ASL::StationID				sStationID;

	PLPresetCategory					mCategory;
};

/**
**
*/
class FileBasedPLPresetHandlerFactory
	:
	public PLPresetHandlerFactory
{
public:
	FileBasedPLPresetHandlerFactory(
		const dvacore::UTF16String& inUserPresetFolder);

	virtual ~FileBasedPLPresetHandlerFactory();
	
	virtual std::auto_ptr<AbstractPLPresetHandler> GetPresetHandler(const PLPresetCategory inCategory);

protected:
	dvacore::UTF16String		mUserPresetFolder;
};

}

#endif
