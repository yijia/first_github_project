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

//ASL
#include "ASLClassFactory.h"
#include "ASLFile.h"
#include "ASLPathUtils.h"
#include "ASLListener.h"
#include "ASLStation.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLTimer.h"

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif
#include "MFDirectoryChangeMonitor.h"

// boost
#include "boost/foreach.hpp"

// dva
#include "dvacore/utility/Coercion.h"
#include "dvacore/debug/DebugDatabase.h"

//BE
#include "BEXML.h"
#include "BE/Core/IXMLFileReader.h"
#include "BE/Core/IXMLFileWriter.h"
#include "BE/Core/ISerializeable.h"

// DVA
#include "dvacore/config/Client.h"
#include "dvacore/config/Localizer.h"
#include "dvacore/filesupport/file/File.h"
#include "dvacore/utility/FileUtils.h"
#include "dvaui/dialogs/SimpleDialogs.h"
#include "dvaui/dialogs/UI_MessageBox.h"
#include "dvacore/utility/StringUtils.h"
#include "dvaworkspace/workspace/Workspace.h"
#include "dvacore/filesupport/file/FileException.h"

// UIF
#include "UIFWorkspace.h"

// Self
#include "Preset/FileBasedPLPresetHandler.h"
#include "Preset/PLPreset.h"
#include "Preset/PLPresetCache.h"

// PRM
#include "PRMApplicationTarget.h"

#define ZPLPreset_Custom_Preset		"$$$/Prelude/Mezzanine/SilkRoad/Preset_Custom_Preset=Custom"

namespace PL
{
	
/*
**
*/
const dvacore::UTF16String AbstractPLPresetHandler::kRenamePresetFileExtensionOnly = DVA_STR("plrp"); // Prelude Preset (USE ALL LOWERCASE)
const dvacore::UTF16String AbstractPLPresetHandler::kRenamePresetFileExtension = 
					DVA_STR(".") + AbstractPLPresetHandler::kRenamePresetFileExtensionOnly; 

const dvacore::UTF16String AbstractPLPresetHandler::kMetadataPresetFileExtensionOnly = DVA_STR("plmp"); // Prelude Preset (USE ALL LOWERCASE)
const dvacore::UTF16String AbstractPLPresetHandler::kMetadataPresetFileExtension = 
					DVA_STR(".") + AbstractPLPresetHandler::kMetadataPresetFileExtensionOnly; 

namespace 
{
	dvacore::UTF16String GetPresetFileExtention(PLPresetCategory inCategory)
	{
		if ( inCategory == PLPreCategory_Rename )
		{
			return AbstractPLPresetHandler::kRenamePresetFileExtension;
		}
		else if ( inCategory == PLPreCategory_Metadata )
		{
			return AbstractPLPresetHandler::kMetadataPresetFileExtension;
		}

		return DVA_STR("");
	}

	dvacore::UTF16String GetPresetsFullPath(const dvacore::filesupport::Dir& inDir, bool inCreateNonExist = false)
	{
		// Get the user presets folder
		dvacore::filesupport::Dir userAdobeDir(inDir, DVA_STR("Adobe"));
		if (inCreateNonExist && !userAdobeDir.Exists()) 
		{
			userAdobeDir.Create();
		}

		dvacore::filesupport::Dir userPreludeDir(userAdobeDir, ASL::MakeString(PRM::GetApplicationName()));
		if (inCreateNonExist && !userPreludeDir.Exists()) 
		{
			userPreludeDir.Create();
		}

		dvacore::filesupport::Dir userPreludeVersionDir(userPreludeDir, ASL::MakeString(PRM::GetApplicationVersion()));
		if (inCreateNonExist && !userPreludeVersionDir.Exists()) 
		{
			userPreludeVersionDir.Create();
		}

		dvacore::filesupport::Dir userPresetsDir(userPreludeVersionDir, DVA_STR("Presets"));
		if (inCreateNonExist && !userPresetsDir.Exists()) 
		{
			userPresetsDir.Create();
		}

		return userPresetsDir.FullPath();
	}

	dvacore::UTF16String GetPresetCategoryString(const PLPresetCategory inCategory)
	{
		dvacore::UTF16String subPath;
		switch (inCategory)
		{
		case PLPreCategory_Rename:
			subPath = DVA_STR("Rename");
			break;
		case PLPreCategory_Metadata:
			subPath = DVA_STR("Metadata");
			break;
		default:
			break;
		}
		return subPath;
	}
}

/*
**	Statics
*/
ASL::StationID				FileBasedPLPresetHandler::sStationID(kFileBasePLPresetHandlerWorkerStation);
/*
**
*/
FileBasedPLPresetHandler::FileBasedPLPresetHandler(
	const dvacore::UTF16String& inUserPresetFolder,
	PLPresetCategory inCategory)
	:
	mUserPresetFolder(inUserPresetFolder),
	mStation(sStationID),
	mCategory(inCategory)
{
	dvacore::filesupport::Dir userPresetFolder(mUserPresetFolder);

	// Create the folder if it does not exist
	if (!userPresetFolder.Exists())
	{
		userPresetFolder.Create();
	}
}

/*
**
*/
FileBasedPLPresetHandler::~FileBasedPLPresetHandler()
{
}

/*
**
*/
IPLPresetRef FileBasedPLPresetHandler::CreatePreset()
{
	IPLPresetRef preset; 
	switch (mCategory)
	{
	case PLPreCategory_Rename:
		preset = IPLPresetRef(PLPresetRename::CreateClassRef());
		break;
	case PLPreCategory_Metadata:
		preset = IPLPresetRef(PLPresetMetadata::CreateClassRef());
		break;
	default:
		break;
	}

	return preset;
}

/*
**	Returns the list of user presets known to the system.
*/
IPLPresets FileBasedPLPresetHandler::GetUserPresets(bool inUseCache)
{
	class FilterUserPresets : public PLPresetEnumerator
	{
	public:
		FilterUserPresets(AbstractPLPresetHandler* inHandler)
			:
		mHandler(inHandler)
		{
		}

		virtual bool operator ()(IPLPresetRef inPreset)
		{
			return mHandler->AllowUserPreset(inPreset);
		}

	protected:
		AbstractPLPresetHandler* mHandler;
	}
	userPresetEnumerator(this);

	IPLPresets	presets = GetPresets(mUserPresetFolder, &userPresetEnumerator, mCategory, kPLPresetType_USER, false, inUseCache);

	return presets;
}


/*
**	Return false if you do not want to list a user preset.
**	@param inPresetRef		The preset in question
*/
bool FileBasedPLPresetHandler::AllowUserPreset( 
	IPLPresetRef inPresetRef)
{
	// default implementation allows everything
	return true;
}

/*
**	Return false if you do not want to list a system preset.
**	@param inPresetRef		The preset in question
*/
bool FileBasedPLPresetHandler::AllowSystemPreset( 
	IPLPresetRef inPresetRef)
{
	// default implementation allows everything
	return true;
}

/*
**	Replace a given user preset with another one.
**	@param inOldPresetRef		The preset to replace
**	@param inOutNewPresetRef	The preset with the new data
*/
ASL::Result FileBasedPLPresetHandler::ReplaceAsUserPreset(
	IPLPresetRef inOldPresetRef, 
	IPLPresetRef inOutNewPresetRef, 
	const dvacore::UTF16String& inNewPresetSavePath)
{
	ASL::Result					result = ePresetNotSupported;
	// get the filename
	//IPresetPrivateRef			pvtPreset(inOldPresetRef);
//	dvacore::filesupport::File	presetFile(pvtPreset->GetPrivateData());
//
//	// the source file must be in the preset folder
//	if (presetFile.Parent() == dvacore::filesupport::Dir(mUserPresetFolder))
//	{
//		dvacore::UTF16String	newPresetSavePath(presetFile.FullPath());
//		// The destination too
//		if (!inOutNewPresetRef->GetPresetName().empty() && !inNewPresetSavePath.empty() && dvacore::filesupport::File(inNewPresetSavePath).Parent() == dvacore::filesupport::Dir(mUserPresetFolder))
//		{
//			// a valid preset name and a new valid file destination path inside of the user preset folder
//			newPresetSavePath	= inNewPresetSavePath;
//			inOutNewPresetRef->SetResetToSaveAsUserPreset(inNewPresetSavePath);
//		}
//		else
//		{
//			// prepair the new Preset with the Name and the private data
//			inOutNewPresetRef->SetPresetName(inOldPresetRef->GetPresetName());
//		}
//
//		ASL::Result deleteResult	= DeleteUserPreset(inOldPresetRef);
//		ASL_ASSERT(ASL::ResultSucceeded(deleteResult) || ((deleteResult == eFileNotFound) && inNewPresetSavePath == presetFile.FullPath()));
//		
//		ASL::Result	writeResult = WritePresetToFile(inOutNewPresetRef, newPresetSavePath);
//		if (ASL::ResultSucceeded(writeResult))
//		{
//#ifndef DISABLE_PRESETDB
//			writeResult = GetDBRef()->AddPresetFileToDb(newPresetSavePath, kPLPresetType_USER);
//#endif
//			result	= ASL::kSuccess;
//		}
//		else
//		{
//			// try to restore the old one
//			ASL::Result	writeResult = WritePresetToFile(inOutNewPresetRef, newPresetSavePath);
//			if (ASL::ResultSucceeded(writeResult))
//			{
//#ifndef DISABLE_PRESETDB
//				writeResult = GetDBRef()->AddPresetFileToDb(newPresetSavePath, kPLPresetType_USER);
//#endif
//			}
//		}
//	}

	return result;
}

/*
**	Save a given user preset. Fails if a preset of the same 
**	name already exists.
**	@param inPresetRef		The preset in question
*/
ASL::Result FileBasedPLPresetHandler::SaveAsUserPreset(
	IPLPresetRef inPresetRef,
	const dvacore::UTF16String& inPresetName,
	const dvacore::UTF16String& inPresetComment,
	bool inOverwriteOnExist,
	const dvacore::UTF16String& inFileName)
{
	dvacore::UTF16String newPresetFileName(inFileName.empty() ? inPresetName : inFileName);
	dvacore::utility::FileUtils::ReplaceIllegalFileNameCharacters(newPresetFileName);
	ASL::Result result = ePresetFileAlreadyExists;
	dvacore::filesupport::File newPresetFile(
		mUserPresetFolder, 
		newPresetFileName + GetPresetFileExtention(mCategory) );
	
	if (!newPresetFile.Exists() || inOverwriteOnExist)
	{
		result = WritePresetToFile(inPresetRef, newPresetFile.FullPath());
		if (ASL::ResultSucceeded(result))
		{
			inPresetRef->SetPresetFullPath(newPresetFile.FullPath());
		}
	}
	
	return result;
}

/*
**	Delete a given user preset.
**	@param inPresetRef		The preset in question
*/
ASL::Result FileBasedPLPresetHandler::DeleteUserPreset(
	IPLPresetRef inPresetRef)
{
	ASL::Result	ret = ASL::kSuccess;
	dvacore::filesupport::File	presetFile(inPresetRef->GetPresetFullPath());
	if (presetFile.Parent() == dvacore::filesupport::Dir(mUserPresetFolder) && presetFile.Exists())
	{
		presetFile.Delete();
	}
	
	return ret;
}

/**
**	Loads the placeholder preset
*/
IPLPresetRef FileBasedPLPresetHandler::GetPlaceHolderPreset()
{
	dvacore::filesupport::File placeHolderPreset(GetPlaceHolderPresetFile());
	IPLPresetRef preset(ReadPresetFromFile(placeHolderPreset.FullPath(), mCategory));
	return preset;
}


/**
** Saves a preset into the place holder preset.
*/
ASL::Result FileBasedPLPresetHandler::SavePlaceHolderPreset(
	IPLPresetRef inPresetRef)
{
	dvacore::filesupport::File placeHolderPreset(GetPlaceHolderPresetFile());
	PLPresetRef editablePreset(inPresetRef);
	editablePreset->SetPresetName(dvacore::config::Localizer::Get()->GetLocalizedString(ZPLPreset_Custom_Preset));
	//if(editablePreset->GetPresetComments().size() == 0)
	//{
	//	editablePreset->SetPresetComments(dvacore::config::Localizer::Get()->GetLocalizedString(ZPLPreset_Custom_Preset));
	//}
	return WritePresetToFile(
		inPresetRef,
		placeHolderPreset.FullPath());
}

/**
** Return a fullpath to the user presets, can be empty if not a filebased one
*/
dvacore::UTF16String FileBasedPLPresetHandler::GetUserPresetsLocation()
{
	return mUserPresetFolder;
}
	
/*
**
*/
PLPresetCategory FileBasedPLPresetHandler::GetCategory() const
{
	return mCategory;
}

/*
**
*/
dvacore::filesupport::File FileBasedPLPresetHandler::GetPlaceHolderPresetFile()
{
	dvacore::filesupport::Dir prefsDir(dvacore::filesupport::Dir::PrefDir());
	dvacore::UTF16String fileName(DVA_STR("Placeholder Preset"));

	DVA_ASSERT(dvacore::filesupport::Dir::PrefDir().Exists());

	fileName += GetPresetFileExtention(mCategory);

	return dvacore::filesupport::File(prefsDir, fileName);
}

/**
**
*/
IPLPresets FileBasedPLPresetHandler::GetPresets(
	const dvacore::UTF16String& inPresetFolder,
	PLPresetEnumerator* inEnumerator,
	PLPresetCategory inCategory,
	PLPresetType inPresetType,
	bool inRecurseSubfolders,
	bool inUseCache)
{
	dvacore::filesupport::Dir presetFolder(inPresetFolder);
	IPLPresets	presets;

	// [ToDo] the preset DB is not implemented. Needs refer to exporter preset implementation.
	bool shouldEnablePresetDB = false;
	if (!shouldEnablePresetDB)
	{
		ASL::Timer	elapsedTimer;

		elapsedTimer.Start();
		if (presetFolder.Exists())
		{
			for (dvacore::filesupport::Dir::FileIterator file = presetFolder.BeginFiles(false);
				file != presetFolder.EndFiles();
				file++)
			{
				dvacore::UTF16String filePath((*file).FullPath());
				dvacore::UTF16String fileExtension = (*file).GetExtension();

				if ( fileExtension == GetPresetFileExtention(inCategory) )
				{
					IPLPresetRef preset;
					if (inUseCache)
					{
						preset = PLPresetCache::Get().GetPreset((*file).FullPath());
					}

					if ( !preset )
					{
						preset = ReadPresetFromFile(filePath, inCategory, inUseCache);	
						if (preset)
						{
							preset->SetPresetType(inPresetType);
						}
					}

					bool allowPreset = false;
					if(!preset)
						continue;
					if(!(*inEnumerator)(preset))
						continue;

					// [TODO] The preset right management needs be considered. This can refer to export preset implementation
					allowPreset = true;
					if (allowPreset)
					{
						presets.push_back(preset);
					}
				}
			}
		}

	
		elapsedTimer.Stop();

		double elapsedSeconds = elapsedTimer.LapSeconds();
		dvacore::UTF16String traceString = DVA_STR("Searching IPLPresets the old way: ") + 
			inPresetFolder + DVA_STR(" (Time taken (seconds) = ") + 
			dvacore::utility::Coercion<dvacore::UTF16String>::Result(elapsedSeconds) + 
			DVA_STR(")");

		DVA_TRACE_ALWAYS("Mezzanine-PLPreset", 6, traceString);

		return presets;
	}

	return IPLPresets();
}

/**
**
*/
IPLPresets FileBasedPLPresetHandler::GetPresets(
	const std::vector<dvacore::UTF16String>& inPresetFoldersVector,
	PLPresetEnumerator* inEnumerator,
	PLPresetCategory inCategory,
	PLPresetType inPresetType,
	bool inRecurseSubfolders,
	bool inUseCache)
{
	std::vector<dvacore::UTF16String>::const_iterator it;
	IPLPresets	presets;
	for(it = inPresetFoldersVector.begin(); it!=inPresetFoldersVector.end(); it++)
	{
		dvacore::filesupport::Dir presetFolder(*it);
		if (presetFolder.Exists())
		{
			IPLPresets currentPresetSet = GetPresets(presetFolder.FullPath(), inEnumerator, inCategory, inPresetType, inRecurseSubfolders, inUseCache);
			presets.insert(presets.end(), currentPresetSet.begin(), currentPresetSet.end());
		}
	}

	return presets;
}

/**
** 
*/
dvacore::UTF16String FileBasedPLPresetHandler::GetSystemPresetFolderPath(const PLPresetCategory inCategory)
{
	dvacore::filesupport::Dir sharedDocsDir(dvacore::filesupport::Dir::SharedDocsDir());
	DVA_ASSERT(sharedDocsDir.Exists());

	if ( !sharedDocsDir.Exists() )
	{
		return DVA_STR("");
	}

	dvacore::UTF16String sharedPresetPath(ASL::PathUtils::CombinePaths(
		GetPresetsFullPath(sharedDocsDir), 
		GetPresetCategoryString(inCategory)));

	return sharedPresetPath;
}

/**
** 
*/
dvacore::UTF16String FileBasedPLPresetHandler::GetUserPresetFolderPath(const PLPresetCategory inCategory)
{
	dvacore::filesupport::Dir userDocsDir(dvacore::filesupport::Dir::UserDocsDir());
	DVA_ASSERT(userDocsDir.Exists());

	if ( !userDocsDir.Exists() )
	{
		userDocsDir.Create();
	}

	dvacore::UTF16String userPresetPath(ASL::PathUtils::CombinePaths(
		GetPresetsFullPath(userDocsDir, true), 
		GetPresetCategoryString(inCategory)));

	return userPresetPath;
}

/*
**
*/
IPLPresetRef FileBasedPLPresetHandler::ImportPreset(
	const dvacore::UTF16String& inPresetFullPath, 
	const PLPresetCategory inCategory)
{
	IPLPresetRef preset = IPLPresetRef();

	const dvacore::UTF16String& sourceExt = ASL::PathUtils::GetExtensionPart(inPresetFullPath);

	if ( sourceExt != GetPresetFileExtention(inCategory) )
	{
		dvaui::dialogs::UI_MessageBox::Button button = dvaui::dialogs::messagebox::Messagebox(
			UIF::Workspace::Instance()->GetPtr()->GetDrawSupplier(),
			dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ImportPreset=Import Preset"),
			dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ImportPresetInvalidFileExt=The selected file extension does not match the preset extension."),
			dvaui::dialogs::UI_MessageBox::kButton_Ok,
			dvaui::dialogs::UI_MessageBox::kButton_Ok,
			dvaui::dialogs::UI_MessageBox::kIcon_Stop,
			UIF::Workspace::Instance()->GetPtr()->GetMainTopLevelWindow());

		return preset;
	}

	const dvacore::UTF16String& userPath = GetUserPresetFolderPath(inCategory);
	const dvacore::UTF16String& sourceFileName = ASL::PathUtils::GetFullFilePart(inPresetFullPath);
	const dvacore::UTF16String& destFileName = ASL::PathUtils::CombinePaths(userPath, sourceFileName);

	if ( ASL::PathUtils::ExistsOnDisk(destFileName) )
	{
		dvaui::dialogs::UI_MessageBox::Button button = dvaui::dialogs::messagebox::Messagebox(
			UIF::Workspace::Instance()->GetPtr()->GetDrawSupplier(),
			dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ImportPreset=Import Preset"),
			dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ImportPresetError=A preset file with the same name already exists. Do you want to overwrite it?"),
			dvaui::dialogs::UI_MessageBox::kButton_Yes | dvaui::dialogs::UI_MessageBox::kButton_No,
			dvaui::dialogs::UI_MessageBox::kButton_No,
			dvaui::dialogs::UI_MessageBox::kIcon_Stop,
			UIF::Workspace::Instance()->GetPtr()->GetMainTopLevelWindow());

		if ( button == dvaui::dialogs::UI_MessageBox::kButton_No )
		{
			return preset;
		}
	}

	try
	{
		if ( dvacore::utility::FileUtils::NormalizePathForStringComparison(ASL::PathUtils::ToNormalizedPath(inPresetFullPath)) != 
			 dvacore::utility::FileUtils::NormalizePathForStringComparison(ASL::PathUtils::ToNormalizedPath(destFileName)) )
		{
			dvacore::utility::FileUtils::Copy(inPresetFullPath, destFileName); // don't use ASL::File::Copy as it will not overwrite on Mac
		}

		bool isOnDisk = ASL::PathUtils::ExistsOnDisk(destFileName);
		DVA_ASSERT(isOnDisk);
		if ( isOnDisk )
		{
			preset = FileBasedPLPresetHandler::ReadPresetFromFile(destFileName, inCategory, false);
		}
	}
	catch (dvacore::filesupport::file_exception& e)
	{
		DVA_ASSERT_MSG(false, e.what());;
	}
	if (preset == NULL && ASL::PathUtils::ExistsOnDisk(destFileName))
	{
		ASL::File::Delete(destFileName);
	}

	return preset;
}

/*
**
*/
IPLPresetRef FileBasedPLPresetHandler::ReadPresetFromFile(
	const dvacore::UTF16String& inPresetFile,
	PLPresetCategory inCategory,
	bool inUseCache)
{
	IPLPresetRef preset;

	if(!inPresetFile.empty())
	{
		if ( inUseCache )
		{
			preset = PLPresetCache::Get().GetPreset(inPresetFile);
		}

		if (!preset)
		{
			BE::IXMLFileReaderRef presetReader(ASL::CreateClassInstanceRef(BE::kXMLFileReaderClassID));
			
			if(ASL::ResultSucceeded(presetReader->ReadFromFile(inPresetFile)))//File exists
			{
				switch (inCategory)
				{
				case PLPreCategory_Rename: 
					preset = IPLPresetRef(PLPresetRename::CreateClassRef());
					break;

				case PLPreCategory_Metadata:
					preset = IPLPresetRef(PLPresetMetadata::CreateClassRef());
					break;

				default:
					preset = IPLPresetRef(PLPreset::CreateClassRef());
					break;
				}
				BE::ISerializeableRef(preset)->SerializeIn(
					BE::ISerializeableRef(preset)->GetVersion(),
					*presetReader->GetDataReader());
			}
			
			if (preset)
			{
				preset->SetPresetFullPath(inPresetFile);
				PLPresetCache::Get().CachePreset(inPresetFile, preset);
			}
			else
			{
				dvaui::dialogs::messagebox::Messagebox(
					UIF::Workspace::Instance()->GetPtr()->GetDrawSupplier(),
					dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ReadPreset=Import Preset"),
					dvacore::ZString("$$$/Prelude/Mezzanine/Failure/ReadPresetInvalidPresetFile=The file @0 is not a valid preset file.", inPresetFile),
					dvaui::dialogs::UI_MessageBox::kButton_Ok,
					dvaui::dialogs::UI_MessageBox::kButton_Ok,
					dvaui::dialogs::UI_MessageBox::kIcon_Stop,
					UIF::Workspace::Instance()->GetPtr()->GetMainTopLevelWindow());
			}
		}
	}

	return preset;
}

/*
**
*/
ASL::Result FileBasedPLPresetHandler::WritePresetToFile(
	IPLPresetRef inPreset,
	const dvacore::UTF16String& inPresetFile)
{
	BE::IXMLFileWriterRef presetWriter(ASL::CreateClassInstanceRef(BE::kXMLFileWriterClassID));
	
	BE::ISerializeableRef(inPreset)->SerializeOut(
		*presetWriter->GetDataWriter());
		
	ASL::Result result = presetWriter->WriteToFile(inPresetFile);
	if (ASL::ResultSucceeded(result))
	{
		PLPresetCache::Get().CachePreset(inPresetFile, inPreset);
	}
	return result;
}

/*
**
*/
void FileBasedPLPresetHandler::SetSystemPresetsFolder(
	const std::vector<dvacore::UTF16String>& inPresetFoldersVector,
	bool inRecurse)
{
}

/*
**
*/
void FileBasedPLPresetHandler::Initialize()
{
	if(!ASL::StationRegistry::FindStation(sStationID))
	{
		ASL::StationRegistry::RegisterStation(sStationID);
	}
}

/*
**
*/
void FileBasedPLPresetHandler::Terminate()
{
	ASL::StationRegistry::DisposeStation(sStationID);
}

/*
**
*/
FileBasedPLPresetHandlerFactory::FileBasedPLPresetHandlerFactory(
	const dvacore::UTF16String& inUserPresetFolder)
	:
	mUserPresetFolder(inUserPresetFolder)
{
}

/*
**
*/
FileBasedPLPresetHandlerFactory::~FileBasedPLPresetHandlerFactory()
{
}

/*
**
*/
std::auto_ptr<AbstractPLPresetHandler> FileBasedPLPresetHandlerFactory::GetPresetHandler(const PLPresetCategory inCategory)
{
	switch (inCategory)
	{
	case PLPreCategory_Rename:
		return std::auto_ptr<AbstractPLPresetHandler>(new FileBasedPLPresetHandler(mUserPresetFolder, inCategory));
		break;

	case PLPreCategory_Metadata:
		return std::auto_ptr<AbstractPLPresetHandler>(new FileBasedPLPresetHandler(mUserPresetFolder, inCategory));
		break;

	default:
		break;
	}
	return std::auto_ptr<AbstractPLPresetHandler>(NULL);
}

}
