/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2003-2006 Adobe Systems Incorporated                  */
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


//ASL
#include "ASLClass.h"
#include "ASLClassFactory.h"
#include "ASLStringUtils.h"
#include "ASLpathUtils.h"

//Local
#include "Preset/PLPresetManagerFactory.h"
#include "Preset/PLPresetManager.h"
#include "Preset/AbstractPLPresetHandler.h"
#include "Preset/FileBasedPLPresetHandler.h"
#include "Preset/PLPreset.h"

//File Utils
#ifdef AME_OS_WIN32
#include <shlobj.h> //For knowing common dir flags.
#include "dvacore/filesupport/file/win/FileHelper.h"
#elif AME_OS_MAC
#include "dvacore/filesupport/file/mac/FileHelper.h"
#endif

#include "dvacore/filesupport/file/File.h"
#include "dvacore/config/Client.h"
#include "dvacore/config/Localizer.h"

namespace PL
{
	namespace
	{
		std::auto_ptr<PLPresetHandlerFactory> GetUserPresetHandlerFactory(const PLPresetCategory inCategory)
		{
			const dvacore::UTF16String& path = FileBasedPLPresetHandler::GetUserPresetFolderPath(inCategory);
			std::auto_ptr<PLPresetHandlerFactory> presetHandlerFactory(new FileBasedPLPresetHandlerFactory(path));
			return presetHandlerFactory;
		}
	}

PLPresetManagerFactory::PLPresetManagerFactory()
{
}

PLPresetManagerFactory::~PLPresetManagerFactory()
{
}

IPLPresetManagerRef PLPresetManagerFactory::CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory)
{
	PLPresetManagerRef presetManager( PLPresetManager::CreateClassRef() );
	
	presetManager->Initialize(inSystemPresetFolder, inUserPresetFolder, inCategory);
	
	return IPLPresetManagerRef(presetManager);
}

IPLPresetManagerRef PLPresetManagerFactory::CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory)
{
	PLPresetManagerRef presetManager( PLPresetManager::CreateClassRef() );
	
	presetManager->Initialize(inSystemPresetFolder, inPresetHandlerFactory, inCategory);
	
	return IPLPresetManagerRef(presetManager);
}

IPLPresetManagerRef PLPresetManagerFactory::CreatePresetManager(
	const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector, 
	const dvacore::UTF16String& inUserPresetFolder,
	const PLPresetCategory inCategory)
{
	PLPresetManagerRef presetManager( PLPresetManager::CreateClassRef() );

	presetManager->Initialize(inSystemPresetFoldersVector, inUserPresetFolder, inCategory);

	return IPLPresetManagerRef(presetManager);
}

IPLPresetManagerRef PLPresetManagerFactory::CreatePresetManager(
	const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector,
	std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
	const PLPresetCategory inCategory)
{
	PLPresetManagerRef presetManager( PLPresetManager::CreateClassRef() );

	presetManager->Initialize(inSystemPresetFoldersVector, inPresetHandlerFactory, inCategory);

	return IPLPresetManagerRef(presetManager);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------
//					Export Functions
//-----------------------------------------------------------------
//-----------------------------------------------------------------
IPLPresetManagerRef InstantiatePLPresetManager(PLPresetCategory inCategory)
{
	IPLPresetManagerFactoryRef presetManagerFactory(PLPresetManagerFactory::CreateClassRef());
	DVA_ASSERT(presetManagerFactory);
	std::vector<dvacore::UTF16String> systemPresetFolders;
	const dvacore::UTF16String& sharedPresetPath = FileBasedPLPresetHandler::GetSystemPresetFolderPath(inCategory);
	if ( !sharedPresetPath.empty() )
	{
		systemPresetFolders.push_back(sharedPresetPath);
	}

	return IPLPresetManagerRef(presetManagerFactory->CreatePresetManager(
											systemPresetFolders,
											GetUserPresetHandlerFactory(inCategory), 
											inCategory));
}

IPLPresetFieldRef CreatePresetField()
{
	return PresetField::Create();
}

IPLPresetFieldRef CreatePresetField(PLPresetFieldType inType)
{
	return PresetField::Create(inType);
}

IPLPresetFieldRef CreatePresetField(const IPLPresetFieldRef inField)
{
	return PresetField::Create(inField);
}

dvacore::UTF16String GetRenamePresetFileExtension() { return AbstractPLPresetHandler::kRenamePresetFileExtensionOnly; } 
dvacore::UTF16String GetRenamePresetTypeString()
{
	dvacore::UTF16String typeString = 
		dvacore::ZString("$$$/Prelude/Mezzanine/RenamePresetType=Adobe Prelude Rename Preset (*.plrp)");
	typeString.push_back('\0');
	typeString.append(ASL_STR("*.plrp"));
	typeString.push_back('\0');
	return typeString;
}

dvacore::UTF16String GetMetadataPresetFileExtension() { return AbstractPLPresetHandler::kMetadataPresetFileExtensionOnly; } 
dvacore::UTF16String GetMetadataPresetTypeString()
{
	dvacore::UTF16String typeString = 
		dvacore::ZString("$$$/Prelude/Mezzanine/MetadataPresetType=Adobe Prelude Metadata Preset (*.plmp)");
	typeString.push_back('\0');
	typeString.append(ASL_STR("*.plmp"));
	typeString.push_back('\0');
	return typeString;
}

static std::map<PLPresetFieldType, std::vector<dvacore::UTF16String> > sPredefinedFieldValues;

void InitializePredefinedFieldValues()
{
	// AutoIncrease
	dvacore::UTF16String const& autoIncStr1 = DVA_STR("1");
	dvacore::UTF16String const& autoIncStr01 = DVA_STR("01");
	dvacore::UTF16String const& autoIncStr001 = DVA_STR("001");
	dvacore::UTF16String const& autoIncStr0001 = DVA_STR("0001");

	std::vector<dvacore::UTF16String> autoIncreaseValues;
	autoIncreaseValues.push_back(autoIncStr1);
	autoIncreaseValues.push_back(autoIncStr01);
	autoIncreaseValues.push_back(autoIncStr001);
	autoIncreaseValues.push_back(autoIncStr0001);
	sPredefinedFieldValues[kPLPresetFieldType_AutoIncrease] = autoIncreaseValues;

	// User Defined AutoIncrease
	sPredefinedFieldValues[kPLPresetFieldType_UserDefinedAutoIncrease] = autoIncreaseValues;

	// CustomText

	// Date
	dvacore::UTF16String const& yyyymmdd = DVA_STR("%Y%m%d");
	dvacore::UTF16String const& yymmdd = DVA_STR("%y%m%d");
	dvacore::UTF16String const& mmddyyyy = DVA_STR("%m%d%Y");
	dvacore::UTF16String const& mmddyy = DVA_STR("%m%d%y");
	dvacore::UTF16String const& ddmmyyyy = DVA_STR("%d%m%Y");
	dvacore::UTF16String const& ddmmyy = DVA_STR("%d%m%y");

	std::vector<dvacore::UTF16String> dateValues;
	dateValues.push_back(yyyymmdd);
	dateValues.push_back(yymmdd);
	dateValues.push_back(mmddyyyy);
	dateValues.push_back(mmddyy);
	dateValues.push_back(ddmmyyyy);
	dateValues.push_back(ddmmyy);
	sPredefinedFieldValues[kPLPresetFieldType_CreationDate] = dateValues;
	sPredefinedFieldValues[kPLPresetFieldType_IngestDate] = dateValues;

	// Time
	dvacore::UTF16String const& hhmmss = DVA_STR("%H-%M-%S");

	std::vector<dvacore::UTF16String> timeValues;
	timeValues.push_back(hhmmss);
	
	sPredefinedFieldValues[kPLPresetFieldType_CreationTime] = timeValues;
	sPredefinedFieldValues[kPLPresetFieldType_IngestTime] = timeValues;

	// FileName
}

std::vector<dvacore::UTF16String> GetPredefinedFieldValues(PLPresetFieldType inType)
{
	if (sPredefinedFieldValues.empty())
		InitializePredefinedFieldValues();

	return sPredefinedFieldValues[inType];
}

} //namespace PL

