/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2010 Adobe Systems Incorporated                       */
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

#include "PLMarkerTypeRegistry.h"

#include "dvacore/utility/StringCompare.h"
#include "dvacore/utility/StringUtils.h"
#include "dvacore/filesupport/file/File.h"
#include "dvacore/config/Localizer.h"

#include "ASLDirectoryRegistry.h"
#include "ASLEndianConverter.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLPathUtils.h"
#include "ASLStringCompare.h"

#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProperties.h"

#include "PLMarkerTemplate.h"

namespace PL
{

namespace 
{
	const BE::PropertyKey kMZMarkerColorByTypeBase = BE_PROPERTYKEY("mz.marker.color.");
	const BE::PropertyKey kMZMarkerEnabledByTypeBase = BE_PROPERTYKEY("mz.marker.enabled.");
	

	//	Default green for markers
	static const ASL::Color kDefaultMarkerColors[] = {
		ASL::Color(175, 139, 177), 
		ASL::Color(131, 137, 171),
	};

	const size_t kNumDefaultColors = sizeof(kDefaultMarkerColors)/sizeof(kDefaultMarkerColors[0]);
	static size_t sNotFoundColorIndex = 0;


	/*
	** convert ASL color to BGR (little-endian red, green, blue)
	*/
	ASL::UInt32 ASLColorToCOLORREF(
		const ASL::Color& inColor)
	{
		ASL::UInt32 outColor = 0;
		
		outColor |= inColor.Blue();
		outColor = outColor << 8;
		outColor |= inColor.Green();
		outColor = outColor << 8;
		outColor |= inColor.Red();
		
		return outColor;
	}
	
	/*
	** convert BGR (little-endian red, green, blue) to ASL color
	*/
	ASL::Color COLORREFToASLColor(ASL::UInt32 inColor)
	{
		ASL::Color outColor;
		
		ASL::UInt16 colorComponent;
		
		ASL_SWAP32(inColor);
		
		colorComponent = inColor & 0x000000FF;
		colorComponent |= colorComponent << 8;
		outColor.SetRed(static_cast<ASL::Color::ValueType>(colorComponent));
		
		colorComponent = (inColor & 0x0000FF00) >> 8;
		colorComponent |= colorComponent << 8;
		outColor.SetGreen(static_cast<ASL::Color::ValueType>(colorComponent));

		colorComponent = (inColor & 0x00FF0000) >> 16;
		colorComponent |= colorComponent << 8;
		outColor.SetBlue(static_cast<ASL::Color::ValueType>(colorComponent));
		
		return outColor;
	}
	
	MarkerTypeRegistry sMarkerTypeRegistry;
	MarkerTypeNameList sMarkerTypeList;
	ASL::StationID	   sStationID;
	ASL::CriticalSection sCriticalSection;
	typedef std::map<dvacore::UTF16String, dvacore::UTF16String> MarkerTypeDisplayNameMap;
	MarkerTypeDisplayNameMap sMarkerTypeDisplayNameMap;

	typedef	std::map<dvacore::UTF16String, ASL::Color> MarkerTypeDefaultNameColorMap;
	MarkerTypeDefaultNameColorMap sMarkerTypeDefaultNameColorMap;

} // private namespace

namespace MarkerType
{

	PL_EXPORT extern const dvacore::UTF16String kInOutMarkerType = ASL_STR("InOut");
	PL_EXPORT extern const dvacore::UTF16String kCommentMarkerType = ASL_STR("Comment");
	PL_EXPORT extern const dvacore::UTF16String kFLVCuePointMarkerType = ASL_STR("FLVCuePoint");
	PL_EXPORT extern const dvacore::UTF16String kWebLinkMarkerType = ASL_STR("WebLink");
	PL_EXPORT extern const dvacore::UTF16String kChapterMarkerType = ASL_STR("Chapter");
	PL_EXPORT extern const dvacore::UTF16String kSpeechMarkerType = ASL_STR("Speech");

void Initialize()
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	sStationID = ASL::StationRegistry::RegisterUniqueStation();

	sMarkerTypeDefaultNameColorMap[kInOutMarkerType] = ASL::Color(41, 103, 178);
	sMarkerTypeDefaultNameColorMap[kCommentMarkerType] = ASL::Color(113, 134, 55);
	sMarkerTypeDefaultNameColorMap[kFLVCuePointMarkerType] = ASL::Color(208, 161, 43); 
	sMarkerTypeDefaultNameColorMap[kWebLinkMarkerType] = ASL::Color(233, 111, 36);
	sMarkerTypeDefaultNameColorMap[kChapterMarkerType] = ASL::Color(210, 44, 54);
	sMarkerTypeDefaultNameColorMap[kSpeechMarkerType] = ASL::Color(184, 65, 123);
}

void Shutdown()
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	ASL::StationRegistry::DisposeStation(sStationID);
}

bool IsCustomMarkerType(const ASL::String& inMarkerType)
{
	bool isCustomMarkerType = true;

	if(ASL::StringEquals(inMarkerType,kInOutMarkerType))
		isCustomMarkerType = false;
	else if(ASL::StringEquals(inMarkerType,kCommentMarkerType))
		isCustomMarkerType = false;
	else if(ASL::StringEquals(inMarkerType,kFLVCuePointMarkerType))
		isCustomMarkerType = false;
	else if(ASL::StringEquals(inMarkerType,kWebLinkMarkerType))
		isCustomMarkerType = false;
	else if(ASL::StringEquals(inMarkerType,kChapterMarkerType))
		isCustomMarkerType = false;
	else if(ASL::StringEquals(inMarkerType,kSpeechMarkerType))
		isCustomMarkerType = false;

	return isCustomMarkerType;
}

dvacore::UTF16String GetCustomMarkerIDStr()
{
	return dvacore::utility::AsciiToUTF16("_MarkerTypeHandler");
}

ASL::StationID GetStationID()
{
	return sStationID;
}

void AddMarkerToRegistry(
	const dvacore::UTF16String& inMarkerName,
	const dvacore::UTF16String& inMarkerDisplayName,
	const dvacore::UTF16String& inMarkerHandlerName,
	const dvacore::UTF16String& swfPath)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef bProp(backend);

	//	Add markers to the registry.
	BE::PropertyKey typeKey = BE_MAKEPROPERTYKEY(kMZMarkerColorByTypeBase.ToString() + inMarkerName);
	ASL::UInt32 storedColor;
	if (!bProp->GetValue(typeKey, storedColor))
	{
		ASL::Color defaultColor;
		MarkerTypeDefaultNameColorMap::const_iterator iter = sMarkerTypeDefaultNameColorMap.find(inMarkerName);
		if (iter != sMarkerTypeDefaultNameColorMap.end())
		{
				defaultColor = iter->second;
		}
		else
		{
			defaultColor = kDefaultMarkerColors[sNotFoundColorIndex];
			sNotFoundColorIndex = std::min(sNotFoundColorIndex + 1, static_cast<size_t>(kNumDefaultColors-1));
		}
		storedColor = ASLColorToCOLORREF(defaultColor);
	}
	ASL::Color aslColor(COLORREFToASLColor(storedColor));
	
	if (!inMarkerDisplayName.empty())
	{
		sMarkerTypeDisplayNameMap[inMarkerName] = inMarkerDisplayName;
	}
	else
	{
		sMarkerTypeDisplayNameMap[inMarkerName] = inMarkerName;
	}

	MarkerTypeHandlerInfo info = {inMarkerHandlerName, swfPath, 0, aslColor, true};
	RegisterMarkerType(inMarkerName, info);
}

void RegisterMarkerTypes()
{
	// location of marker type handler swf's for the marker panel
	static const char* const kMarkerTypesDirectory	= "marker.types";

	dvacore::UTF16String path = ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(kMarkerTypesDirectory));
	dvacore::filesupport::Dir markerTypesDir(path);

	//	Always register the subclip marker no matter what.
	AddMarkerToRegistry(kInOutMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Subclip=Subclip"),
		kInOutMarkerType, dvacore::UTF16String());

	bool installAllDefaults = true;
	if (markerTypesDir.Exists())
	{
		dvacore::UTF16String defaultMarkerTypesPath = path;
		defaultMarkerTypesPath = ASL::PathUtils::AddTrailingSlash(defaultMarkerTypesPath);
		defaultMarkerTypesPath += ASL_STR("Default Marker List.txt");
		
		if (ASL::PathUtils::ExistsOnDisk(defaultMarkerTypesPath))
		{
			ASL::InputFileStream defaultMarkerTypesStream;
#if ASL_TARGET_OS_WIN
			// Use the TCHAR 'W' Unicode UCS2 way for Windows...
			defaultMarkerTypesStream.open(defaultMarkerTypesPath.c_str(), ASL::InputFileStream::in);
#else
			// Use the UTF8 POSIX way for all other platforms...
			defaultMarkerTypesStream.open(ASL::MakeStdString(defaultMarkerTypesPath).c_str(), ASL::InputFileStream::in);
#endif
			dvacore::UTF8Char buffer[1024];
			do
			{
				defaultMarkerTypesStream.getline(reinterpret_cast<char *>(buffer), 1024);
				dvacore::UTF16String markerItem(dvacore::utility::UTF8to16(dvacore::UTF8String(buffer)));
				if (!markerItem.empty())
				{
					if (markerItem == kCommentMarkerType)
					{
						AddMarkerToRegistry(kCommentMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Comment=Comment"),
							kCommentMarkerType, dvacore::UTF16String());
					}
					else if (markerItem == kFLVCuePointMarkerType)
					{
						AddMarkerToRegistry(kFLVCuePointMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/FLVCuePoint=Flash Cue Point"),
							kFLVCuePointMarkerType, dvacore::UTF16String());
					}
					else if (markerItem == kChapterMarkerType)
					{
						AddMarkerToRegistry(kChapterMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Chapter=Chapter"),
							kChapterMarkerType, dvacore::UTF16String());
					}
					else if (markerItem == kWebLinkMarkerType)
					{
						AddMarkerToRegistry(kWebLinkMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Weblink=Web Link"),
							kWebLinkMarkerType, dvacore::UTF16String());
					}
					else if (markerItem == kSpeechMarkerType)
					{
						AddMarkerToRegistry(kSpeechMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Speech=Speech Transcription"),
							kSpeechMarkerType, dvacore::UTF16String());
					}
				}
			}
			while (defaultMarkerTypesStream.good());
			
			installAllDefaults = false;	
		}
	}
	
	if (installAllDefaults)
	{	
		// register standard markers 
		AddMarkerToRegistry(kCommentMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Comment=Comment"),
			kCommentMarkerType, dvacore::UTF16String());
		AddMarkerToRegistry(kFLVCuePointMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/FLVCuePoint=Flash Cue Point"),
			kFLVCuePointMarkerType, dvacore::UTF16String());
		AddMarkerToRegistry(kChapterMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Chapter=Chapter"),
			kChapterMarkerType, dvacore::UTF16String());
		AddMarkerToRegistry(kWebLinkMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Weblink=Web Link"),
			kWebLinkMarkerType, dvacore::UTF16String());
		AddMarkerToRegistry(kSpeechMarkerType, dvacore::ZString("$$$/Prelude/MZ/Marker/Speech=Speech Transcription"),
			kSpeechMarkerType, dvacore::UTF16String());
	}
}

void RegisterMarkerType(dvacore::UTF16String markerType, MarkerTypeHandlerInfo& inInfo)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	bool isNewMarker = (sMarkerTypeRegistry.find(markerType) == sMarkerTypeRegistry.end());
	ASL::AddOrUpdateValue(sMarkerTypeRegistry, markerType, inInfo);

	if (isNewMarker)
	{
		sMarkerTypeList.push_back(markerType);

		// Refresh default template set
		PL::MarkerTemplate::RefreshDefaultMarkerTemplateSet();
	}
	ASL::StationUtils::PostMessageToUIThread(sStationID, MarkerTypeRegistryChanged());
}

void UnregisterMarkerType(dvacore::UTF16String markerType)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	MarkerTypeRegistry::iterator it = sMarkerTypeRegistry.find(markerType);
	if (it == sMarkerTypeRegistry.end())
		return;

	sMarkerTypeRegistry.erase(it);

	MarkerTypeNameList::iterator iter = sMarkerTypeList.begin();
	for (; iter != sMarkerTypeList.end(); ++iter)
	{
		if (*iter == markerType)
		{
			sMarkerTypeList.erase(iter);
			// Refresh default template set
			PL::MarkerTemplate::RefreshDefaultMarkerTemplateSet();
			break;
		}
	}

	ASL::StationUtils::PostMessageToUIThread(sStationID, MarkerTypeRegistryChanged());
}

void RegisterCustomMarkerType(const dvacore::UTF16String& inLibraryFullPath)
{
	dvacore::filesupport::File libraryFile(inLibraryFullPath);
	if (!libraryFile.Exists())
		return;

	dvacore::UTF16String markerTypeStr = libraryFile.GetTitle();
	dvacore::UTF16String extension = libraryFile.GetExtension();

	if (dvacore::utility::CaseInsensitive::StringEquals(extension, ASL_STR(".swf")))
	{

		dvacore::UTF16String::size_type index = markerTypeStr.find(GetCustomMarkerIDStr());

		if (index != dvacore::UTF16String::npos && index != 0)
		{
			dvacore::UTF16String truncatedMarkerType(markerTypeStr.substr(0,index));	

			// Set display name for marker if there is a config file available
			dvacore::UTF16String markerDisplayName = truncatedMarkerType;
			bool configFileFound = false;

			dvacore::filesupport::Dir::FileIterator configFileIter = libraryFile.Parent().BeginFiles(false);
			dvacore::filesupport::Dir::FileIterator configFileEndIter = libraryFile.Parent().EndFiles();

			for ( ; !configFileFound && configFileIter != configFileEndIter; configFileIter++)
			{
				dvacore::UTF16String configTitle = (*configFileIter).GetTitle();

				if (dvacore::utility::CaseInsensitive::StringEquals(configTitle, ASL_STR("configuration")))
				{
					std::ifstream configFile;
#if defined(DVA_OS_WIN)
					configFile.open((*configFileIter).FullPath().c_str(), std::ios::binary);
#elif defined(DVA_OS_MAC)
					configFile.open(reinterpret_cast<const char*>(dvacore::utility::UTF16to8((*configFileIter).FullPath()).c_str()), std::ios::binary);
#endif

					dvacore::UTF16String kDisplayNameString = DVA_STR("display.name");
					dvacore::StdString configData;
					dvacore::UTF16String configStr;

					while (!configFile.eof())
					{
						std::getline(configFile, configData);
						configStr = dvacore::utility::AsciiToUTF16(configData.c_str());
						dvacore::UTF16String::size_type displayNameIndex = configStr.find(kDisplayNameString);

						if (displayNameIndex != dvacore::UTF16String::npos)
						{
							dvacore::UTF16String::size_type equalsIndex = configStr.find(DVA_STR("="));
							if (equalsIndex != dvacore::UTF16String::npos)
							{
								markerDisplayName = configStr.substr(equalsIndex + 1);
								dvacore::utility::Trim(markerDisplayName);
							}
						}
					}

					configFile.close();
					configFileFound = true;
				}
			}

			AddMarkerToRegistry(truncatedMarkerType, markerDisplayName, markerTypeStr, inLibraryFullPath);
		}
	}
}

const MarkerTypeRegistry GetAllMarkerTypes()
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	return sMarkerTypeRegistry;
}

const MarkerTypeNameList GetSortedMarkerTypes()
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	return sMarkerTypeList;
}

ASL::Color GetColorForType(
	const dvacore::UTF16String& inType)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	MarkerTypeRegistry::iterator iter = sMarkerTypeRegistry.find(inType);
	ASL_ASSERT(iter != sMarkerTypeRegistry.end());
	if (iter == sMarkerTypeRegistry.end())
	{
		return kDefaultMarkerColors[kNumDefaultColors-1];
	}
	return iter->second.mColor;
}

bool SetColorForType(
	const dvacore::UTF16String& inType,
	const ASL::Color& inColor)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	bool changed = false;
	MarkerTypeRegistry::iterator iter = sMarkerTypeRegistry.find(inType);
	ASL_ASSERT(iter != sMarkerTypeRegistry.end());
	if (iter->second.mColor != inColor)
	{
		changed = true;
		iter->second.mColor = inColor;

		BE::IBackendRef backend = BE::GetBackend();
		BE::IPropertiesRef bProp(backend);
		
		BE::PropertyKey typeKey = BE_MAKEPROPERTYKEY(kMZMarkerColorByTypeBase.ToString() + inType);
		bProp->SetValue(typeKey, ASLColorToCOLORREF(inColor), BE::kPersistent);
	}
	return changed;
}

dvacore::UTF16String GetDisplayNameForType( 
	const dvacore::UTF16String& inType)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	return sMarkerTypeDisplayNameMap[inType];
}

}

} //namespace PL
