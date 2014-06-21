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

#include "Prefix.h"

#include "ASLClassFactory.h"
#include "ASLFile.h"
#include "ASLDirectory.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLCriticalSection.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"

#include "BEBackend.h"
#include "BEProperties.h"
#include "BESerializeable.h"
#include "BEXML.h"

#include "PLMarkerTemplate.h"
#include "PLMarkerTypeRegistry.h"
#include "PLUtilities.h"

#include "dvacore/config/Localizer.h"

namespace PL
{

namespace MarkerTemplate
{

namespace
{
	typedef std::map<dvacore::UTF16String, TemplateSet> CustomTemplates;
	CustomTemplates sCustomTemplates;

	typedef std::map<dvacore::UTF16String, dvacore::UTF16String> CustomTemplatesNameMap;
	CustomTemplatesNameMap sCustomTemplateSetNames;

	ASL::CriticalSection sCriticalSection;
	TemplateSetNames sTemplateSetNames;
	
	dvacore::UTF16String kDefaultEmptySet;
	dvacore::UTF16String kCustomSetName;
	dvacore::UTF16String sCurrentSetName;
	
	ASL::StationID sStationID;
	
	dvacore::UTF16String kTemplateSetExtension = ASL_STR(".markertemplateset");
	dvacore::UTF16String kCustomSetFileName = ASL_STR("Custom Template Set");
	dvacore::UTF16String kUserTemplateSetDirectory = ASL_STR("Marker Template Sets");

	//	Initial version
	const ASL::UInt32 kCurPointVersion_1 = 1;

	const ASL::UInt32 kCuePointVersion_Current = kCurPointVersion_1;

	const BE::Key kCuePointKey_Key(BE_KEY("Key"));
	const BE::Key kCuePointKey_Value(BE_KEY("Value"));

	ASL_DEFINE_CLASSREF(CuePointSerializer, BE::ISerializeable);
	ASL_DEFINE_IMPLID(CuePointSerializer, 0x9c0962b7, 0x338f, 0x4fa0, 0xa5, 0x5f, 0xfe, 0xe7, 0x68, 0x2b, 0xae, 0xc);

	const ASL::Guid kCuePointSerializerClassID("9F8DFA7C-3763-4732-BF29-262844177DBA");

	const BE::PropertyKey kPropertyKey_CurrentTemplateSetName(BE_PROPERTYKEY("mz.markertemplate.currenttemplatesetname"));
	/**
	**
	*/
	class CuePointSerializer
		:
		public BE::ISerializeable
	{
	public:
		ASL_BASIC_CLASS_SUPPORT(CuePointSerializer);

		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(ISerializeable)
			ASL_QUERY_ENTRY(CuePointSerializer)
		ASL_QUERY_MAP_END

		/**
		**
		*/
		CuePointSerializer()
			:
			mCuePoint(dvacore::UTF16String(), dvacore::UTF16String())
		{
		}

		/**
		**
		*/
		virtual void SerializeIn(
			ASL::UInt32 inVersion,
			BE::DataReader& inReader)
		{
			inReader.ReadValue(kCuePointKey_Key, mCuePoint.mKey);
			inReader.ReadValue(kCuePointKey_Value, mCuePoint.mValue);
		}

		/**
		**
		*/
		virtual void SerializeOut(
			BE::DataWriter& inWriter) const
		{
			inWriter.WriteValue(kCuePointKey_Key, mCuePoint.mKey);
			inWriter.WriteValue(kCuePointKey_Value, mCuePoint.mValue);
		}

		/**
		**
		*/
		virtual ASL::UInt32 GetVersion() const
		{
			return kCuePointVersion_Current;
		}

		/**
		**
		*/
		virtual ASL::Guid GetClassID() const
		{
			return kCuePointSerializerClassID;
		}

		/**
		**
		*/
		virtual BE::Key GetClassName() const
		{
			return BE_KEY("CuePointSerializer");
		}
		
		void SetCuePoint(
			const dvatemporalxmp::CustomMarkerParam& inCuePoint)
		{
			mCuePoint = inCuePoint;
		}
		
		dvatemporalxmp::CustomMarkerParam GetCuePoint()
		{
			return mCuePoint;
		}

	private:
		dvatemporalxmp::CustomMarkerParam mCuePoint;
	};

	ASL_DEFINE_CLASSREF(CuePointSerializer, BE::ISerializeable);

	//	Initial version
	const ASL::UInt32 kMarkerVersion_1 = 1;
	
	const ASL::UInt32 kMarkerVersion_Current = kMarkerVersion_1;
	
	const BE::Key kMarkerKey_TemplateName(BE_KEY("Marker.TemplateName"));
	const BE::Key kMarkerKey_MarkerName(BE_KEY("Marker.Name"));
	const BE::Key kMarkerKey_MarkerType(BE_KEY("Marker.Type"));
	const BE::Key kMarkerKey_MarkerComment(BE_KEY("Marker.Comment"));
	const BE::Key kMarkerKey_MarkerLocation(BE_KEY("Marker.Location"));
	const BE::Key kMarkerKey_MarkerSpeaker(BE_KEY("Marker.Speaker"));
	const BE::Key kMarkerKey_MarkerProbability(BE_KEY("Marker.Probability"));
	const BE::Key kMarkerKey_MarkerTarget(BE_KEY("Marker.Target"));
	const BE::Key kMarkerKey_MarkerCuePointType(BE_KEY("Marker.CuePointType"));	
	const BE::Key kMarkerKey_MarkerCuePointCount(BE_KEY("Marker.CuePointCount"));
	const BE::Key kMarkerKey_MarkerCuePoint(BE_KEY("Marker.CuePoint"));

	ASL_DEFINE_CLASSREF(MarkerSerializer, BE::ISerializeable);
	ASL_DEFINE_IMPLID(MarkerSerializer, 0x72df654a, 0xe2c5, 0x4e4a, 0x92, 0x68, 0xad, 0x7c, 0x2a, 0xce, 0x18, 0x3);

	const ASL::Guid kMarkerSerializerClassID("0FD6D0DF-4F24-4b1c-BB9A-5CC1F9598F35");

	/**
	**
	*/
	class MarkerSerializer
		:
		public BE::ISerializeable
	{
	public:
		ASL_BASIC_CLASS_SUPPORT(MarkerSerializer);

		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(ISerializeable)
			ASL_QUERY_ENTRY(MarkerSerializer)
		ASL_QUERY_MAP_END

		/**
		**
		*/
		MarkerSerializer()
		{
		}

		/**
		**
		*/
		void SerializeIn(
			ASL::UInt32 inVersion,
			BE::DataReader& inReader)
		{
			inReader.ReadValue(kMarkerKey_TemplateName, mTemplate.mName);
			
			dvacore::UTF16String string;
			inReader.ReadValue(kMarkerKey_MarkerName, string);
			mTemplate.mMarker.SetName(string);
			
			inReader.ReadValue(kMarkerKey_MarkerType, string);
			mTemplate.mMarker.SetType(string);
			
			inReader.ReadValue(kMarkerKey_MarkerComment, string);
			mTemplate.mMarker.SetComment(string);
			
			inReader.ReadValue(kMarkerKey_MarkerLocation, string);
			mTemplate.mMarker.SetLocation(string);
			
			inReader.ReadValue(kMarkerKey_MarkerSpeaker, string);
			mTemplate.mMarker.SetSpeaker(string);
			
			inReader.ReadValue(kMarkerKey_MarkerProbability, string);
			mTemplate.mMarker.SetProbability(string);
			
			inReader.ReadValue(kMarkerKey_MarkerTarget, string);
			mTemplate.mMarker.SetTarget(string);
			
			inReader.ReadValue(kMarkerKey_MarkerCuePointType, string);
			mTemplate.mMarker.SetCuePointType(string);

			dvatemporalxmp::CustomMarkerParamList cuePointList;
			size_t numCuePoints;
			inReader.ReadValue(kMarkerKey_MarkerCuePointCount, numCuePoints);
			for (size_t i = 0; i < numCuePoints; ++i)
			{
				CuePointSerializer cuePointSerializer;
				inReader.ReadSubObject(BE::CreateKeyWithIndex(kMarkerKey_MarkerCuePoint, i + 1), &cuePointSerializer);
				cuePointList.push_back(cuePointSerializer.GetCuePoint());
			}
			mTemplate.mMarker.SetCuePointList(cuePointList);
			
		}

		/**
		**
		*/
		void SerializeOut(
			BE::DataWriter& inWriter) const
		{
			inWriter.WriteValue(kMarkerKey_TemplateName, mTemplate.mName);
			inWriter.WriteValue(kMarkerKey_MarkerName, mTemplate.mMarker.GetName());
			inWriter.WriteValue(kMarkerKey_MarkerType, mTemplate.mMarker.GetType());
			inWriter.WriteValue(kMarkerKey_MarkerComment, mTemplate.mMarker.GetComment());
			inWriter.WriteValue(kMarkerKey_MarkerLocation, mTemplate.mMarker.GetLocation());
			inWriter.WriteValue(kMarkerKey_MarkerSpeaker, mTemplate.mMarker.GetSpeaker());
			inWriter.WriteValue(kMarkerKey_MarkerProbability, mTemplate.mMarker.GetProbability());
			inWriter.WriteValue(kMarkerKey_MarkerTarget, mTemplate.mMarker.GetTarget());
			
			inWriter.WriteValue(kMarkerKey_MarkerCuePointType, mTemplate.mMarker.GetCuePointType());
			dvatemporalxmp::CustomMarkerParamList cuePointList = mTemplate.mMarker.GetCuePointList();
			inWriter.WriteValue(kMarkerKey_MarkerCuePointCount, cuePointList.size());
			for (size_t i = 0; i < cuePointList.size(); ++i)
			{
				CuePointSerializer cuePointSerializer;
				cuePointSerializer.SetCuePoint(cuePointList[i]);
				inWriter.WriteSubObject(BE::CreateKeyWithIndex(kMarkerKey_MarkerCuePoint, i + 1), &cuePointSerializer);
			}
		}

		/**
		**
		*/
		ASL::UInt32 GetVersion() const
		{
			return kMarkerVersion_Current;
		}

		/**
		**
		*/
		ASL::Guid GetClassID() const
		{
			return kMarkerSerializerClassID;
		}

		/**
		**
		*/
		BE::Key GetClassName() const
		{
			return BE_KEY("MarkerSerializer");
		}
		
		void GetTemplate(
			Template& outTemplate)
		{
			outTemplate = mTemplate;
		}
		
		void SetTemplate(
			const Template& inTemplate)
		{
			mTemplate = inTemplate;
		}

	private:
		Template mTemplate;
	};

	ASL_DEFINE_CLASSREF(MarkerSerializer, BE::ISerializeable);


	//	Initial version
	const ASL::UInt32 kTemplateSetVersion_1 = 1;

	const ASL::UInt32 kTemplateSetVersion_Current = kTemplateSetVersion_1;

	const BE::Key kTemplateSetKey_TemplateSet(BE_KEY("TemplateSet"));
	const BE::Key kTemplateSetKey_MarkerCount(BE_KEY("TemplateSet.MarkerCount"));
	const BE::Key kTemplateSetKey_Marker(BE_KEY("Marker"));
	const BE::Key kTemplateSetKey_TemplateSetName(BE_KEY("Name"));
	ASL_DEFINE_CLASSREF(TemplateSetSerializer, BE::ISerializeable);
	ASL_DEFINE_IMPLID(TemplateSetSerializer, 0x25277a10, 0x3fce, 0x4d1e, 0x95, 0xcb, 0x8d, 0xc, 0xd0, 0x15, 0xae, 0x7d);

	const ASL::Guid kTemplateSetSerializerClassID("25277A10-3FCE-4d1e-95CB-8D0CD015AE7D");

	/**
	**
	*/
	class TemplateSetSerializer
		:
		public BE::ISerializeable
	{
	public:
		ASL_BASIC_CLASS_SUPPORT(TemplateSetSerializer);

		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(ISerializeable)
			ASL_QUERY_ENTRY(TemplateSetSerializer)
		ASL_QUERY_MAP_END

		/**
		**
		*/
		TemplateSetSerializer()
		{
		}

		/**
		**
		*/
		virtual void SerializeIn(
			ASL::UInt32 inVersion,
			BE::DataReader& inReader)
		{
			ASL::UInt32 i;
				
			inReader.ReadValue(kTemplateSetKey_TemplateSetName, mName);

			ASL::UInt32 templateCount = 0;
			inReader.ReadValue(kTemplateSetKey_MarkerCount, templateCount);

			for (i = 0; i < templateCount; ++i)
			{
				MarkerSerializer markerSerializer;
				if (inReader.ReadSubObject(BE::CreateKeyWithIndex(kTemplateSetKey_Marker, i + 1), &markerSerializer))
				{
					Template templ;
					markerSerializer.GetTemplate(templ);
					mTemplateSet.push_back(templ);
				}
			}
		}

		/**
		**
		*/
		virtual void SerializeOut(
			BE::DataWriter& inWriter) const
		{
			inWriter.WriteValue(kTemplateSetKey_TemplateSetName, mName);
			inWriter.WriteValue(kTemplateSetKey_MarkerCount, mTemplateSet.size());
			for (size_t i = 0; i < mTemplateSet.size(); ++i)
			{
				MarkerSerializer markerSerializer;
				markerSerializer.SetTemplate(mTemplateSet[i]);
				inWriter.WriteSubObject(BE::CreateKeyWithIndex(kTemplateSetKey_Marker, i + 1), &markerSerializer);
			}
		}

		/**
		**
		*/
		virtual ASL::UInt32 GetVersion() const
		{
			return kTemplateSetVersion_Current;
		}

		/**
		**
		*/
		virtual ASL::Guid GetClassID() const
		{
			return kTemplateSetSerializerClassID;
		}

		/**
		**
		*/
		virtual BE::Key GetClassName() const
		{
			return BE_KEY("TemplateSetSerializer");
		}

		/**
		**
		*/
		void SetTemplateSetAndName(
			const TemplateSet& inTemplateSet,
			const dvacore::UTF16String& inName)
		{
			mTemplateSet = inTemplateSet;
			mName = inName;
		}

		/**
		**
		*/
		void GetTemplateSetAndName(
			TemplateSet& outTemplateSet,
			dvacore::UTF16String& outName)
		{
			outTemplateSet = mTemplateSet;
			outName = mName;
		}

	private:
		TemplateSet	mTemplateSet;
		dvacore::UTF16String mName;
	};

	ASL_DEFINE_CLASSREF(TemplateSetSerializer, BE::ISerializeable);
	
	bool WriteTemplateSetToDisk(
		const TemplateSet& inTemplateSet,
		const dvacore::UTF16String& inName,
		const ASL::String& inPath)
	{
		bool result = false;
		BE::IXMLFileWriterRef theXMLWriter(ASL::CreateClassInstanceRef(BE::kXMLFileWriterClassID));
		BE::DataWriterPtr dataWriter(theXMLWriter->GetDataWriter());
		TemplateSetSerializer templateSetSerializer;
		templateSetSerializer.SetTemplateSetAndName(inTemplateSet, inName);
		dataWriter->WriteSubObject(kTemplateSetKey_TemplateSet, &templateSetSerializer);
		result = ASL::ResultSucceeded(theXMLWriter->WriteToFile(inPath));
		return result;
	}
	
	bool ReadTemplateFromDisk(
		TemplateSet& outTemplateSet,
		dvacore::UTF16String& outName,
		const ASL::String& inPath)
	{
		bool result = false;
		BE::IXMLFileReaderRef theXMLReader(ASL::CreateClassInstanceRef(BE::kXMLFileReaderClassID));
		ASL::Result aslResult(theXMLReader->ReadFromFile(inPath));
		if (ASL::ResultSucceeded(aslResult))
		{
			BE::DataReaderPtr dataReader(theXMLReader->GetDataReader());
			TemplateSetSerializer templateSetSerializer;
			if (dataReader->ReadSubObject(kTemplateSetKey_TemplateSet, &templateSetSerializer))
			{
				templateSetSerializer.GetTemplateSetAndName(outTemplateSet, outName);
				result = true;
			}
		}
		return result;
	}
	
	dvacore::UTF16String GetCustomSetPath()
	{
		dvacore::UTF16String settingsPath = ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(ASL::kUserSettingsDirectory));
		settingsPath = ASL::PathUtils::AddTrailingSlash(settingsPath);
		settingsPath += kCustomSetFileName;
		settingsPath += kTemplateSetExtension;
		return settingsPath;
	}
	
	dvacore::UTF16String GetUserTemplateSetDirectory()
	{
		dvacore::UTF16String settingsPath = ASL::DirectoryRegistry::FindDirectory(ASL::MakeString(ASL::kUserDocumentsDirectory));
		settingsPath = ASL::PathUtils::AddTrailingSlash(settingsPath);
		settingsPath += kUserTemplateSetDirectory;
		return settingsPath;
	}

	// Checks for any marker types in the template set 
	// that are not registered with the marker type registry
	// and registers them
	void AddUnregisteredMarkerTypes(TemplateSet& templateSet)
	{	
		TemplateSet::iterator templateIter = templateSet.begin();
		dvacore::UTF16String markerType;
		PL::MarkerTypeRegistry markerTypes;

		for ( ; templateIter != templateSet.end(); templateIter++)
		{
			markerTypes = MarkerType::GetAllMarkerTypes();
			markerType = (templateIter->mMarker).GetType();
			if (markerTypes.find(markerType) == markerTypes.end())
			{
				MarkerType::AddMarkerToRegistry(markerType, markerType, markerType, dvacore::UTF16String());
			}
		}
	}
}

void Initialize()
{
	kDefaultEmptySet = dvacore::ZString("$$$/Prelude/MZ/MarkerTemplate/DefaultEmptySetName=All Default Markers");
	kCustomSetName = dvacore::ZString("$$$/Prelude/MZ/MarkerTemplate/CustomSetName=<custom>");
	sTemplateSetNames.push_back(kDefaultEmptySet);

	//  Create the default empy set
	RefreshDefaultMarkerTemplateSet();
	
	//	Look for a unsaved custom template set in the user settings directory and load it if found
	if (ASL::PathUtils::ExistsOnDisk(GetCustomSetPath()))
	{
		dvacore::UTF16String customName;
		TemplateSet customSet;
		if (ReadTemplateFromDisk(customSet, customName, GetCustomSetPath()))
		{
			sCustomTemplates[kCustomSetName] = customSet;
			sTemplateSetNames.push_back(kCustomSetName);

			AddUnregisteredMarkerTypes(customSet);
		}
	}
	
	//	Load any sets found in the user directory
	dvacore::UTF16String outPath = GetUserTemplateSetDirectory();
	if (ASL::PathUtils::ExistsOnDisk(outPath))
	{
		ASL::Directory directory = ASL::Directory::ConstructDirectory(outPath);
		ASL::PathnameList pathnames;
		directory.GetContainedFilePaths(pathnames, false);
		ASL::PathnameList::iterator userSetIter = pathnames.begin();
		for ( ; userSetIter != pathnames.end(); ++userSetIter)
		{
			if (ASL::PathUtils::GetExtensionPart(*userSetIter) == kTemplateSetExtension)
			{
				dvacore::UTF16String userName;
				TemplateSet userSet;		
				if (ReadTemplateFromDisk(userSet, userName, *userSetIter))
				{
					sCustomTemplates[userName] = userSet;
					sCustomTemplateSetNames[userName] = ASL::PathUtils::GetFilePart(*userSetIter);
					sTemplateSetNames.push_back(userName);

					AddUnregisteredMarkerTypes(userSet);
				}
			}
		}
	}

	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef bProp(backend);
	
	//	Get the stored property for the last set template. If the property is there and the set
	//	is loaded, then make it the active set.
	dvacore::UTF16String savedName;
	if (bProp->GetValue(kPropertyKey_CurrentTemplateSetName, savedName))
	{
		if (sCustomTemplates.find(savedName) != sCustomTemplates.end())
		{
			sCurrentSetName = savedName;
		}
		else
		{
			sCurrentSetName = kDefaultEmptySet;
		}
	}
	else
	{	
		sCurrentSetName = kDefaultEmptySet;
	}

	sStationID = ASL::StationRegistry::RegisterUniqueStation();
}

void Shutdown()
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	
	CustomTemplates::iterator iter = sCustomTemplates.find(kCustomSetName);
	if (iter != sCustomTemplates.end())
	{
		WriteTemplateSetToDisk(iter->second, kCustomSetName, GetCustomSetPath());
	}
	else
	{
		ASL::File::Delete(GetCustomSetPath());
	}

	ASL::StationRegistry::DisposeStation(sStationID);
	
	sCustomTemplates.clear();
	sTemplateSetNames.clear();
}

ASL::StationID GetStationID()
{
	return sStationID;
}

void GetCurrentTemplateSet(
	TemplateSet& outTemplateSet)
{
	ASL::CriticalSectionLock lock(sCriticalSection);

	CustomTemplates::iterator iter = sCustomTemplates.find(sCurrentSetName);
	if (iter != sCustomTemplates.end())
	{
		outTemplateSet = iter->second;
	}
	else
	{
		ASL_ASSERT(false);
	}
}

dvacore::UTF16String GetCurrentTemplateSetName()
{
	return sCurrentSetName;
}

bool IsCurrentTemplateSetDefault()
{
	return sCurrentSetName == kDefaultEmptySet;
}

bool IsCurrentTemplateSetSaved()
{
	return sCurrentSetName != kCustomSetName;
}

void GetTemplateSetNames(
	TemplateSetNames& outTemplateSetNames)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	outTemplateSetNames = sTemplateSetNames;
}

void SetCurrentTemplateSetName(
	const dvacore::UTF16String& inNewName)
{
	if (inNewName != sCurrentSetName)
	{
		if (sCustomTemplates.find(inNewName) != sCustomTemplates.end())
		{
			sCurrentSetName = inNewName;

			BE::IBackendRef backend = BE::GetBackend();
			BE::IPropertiesRef bProp(backend);
			
			bProp->SetValue(kPropertyKey_CurrentTemplateSetName, sCurrentSetName, true);
			ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateChanged());
		}
	}
}

bool SaveMarkerToCurrentTemplateSet(
	const CottonwoodMarker& inMarker,
	const dvacore::UTF16String& inName,
	bool inForceOverwrite)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	
	TemplateSet templateSet = sCustomTemplates[sCurrentSetName];
	
	TemplateSet::iterator iter = templateSet.begin();
	bool nameFound(false);
	for ( ; iter != templateSet.end(); ++iter)
	{
		if (iter->mName == inName)
		{
			if (!inForceOverwrite)
			{
				return false;
			}
			else
			{
				iter->mMarker = inMarker;
				nameFound = true;
				break;
			}
		}
	}
	
	if (!nameFound)
	{
		Template templ;
		templ.mMarker = inMarker;
		templ.mName = inName;
		templateSet.push_back(templ);
	}
	
	if (sCurrentSetName != kCustomSetName)
	{
		sCurrentSetName = kCustomSetName;
		
		//	If we have to add the custom item, then send that the list changed.
		if (sCustomTemplates.find(kCustomSetName) == sCustomTemplates.end())
		{
			sTemplateSetNames.insert(sTemplateSetNames.begin() + 1, kCustomSetName);
			ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateSetListChanged());
		}
	}
	
	sCustomTemplates[sCurrentSetName] = templateSet;

	ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateChanged());
	

	return true;
}

void DeleteMarkerFromCurrentSet(
	const dvacore::UTF16String& inName)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	
	TemplateSet templateSet = sCustomTemplates[sCurrentSetName];
	
	TemplateSet::iterator iter = templateSet.begin();
	bool nameFound(false);
	for ( ; iter != templateSet.end(); ++iter)
	{
		if (iter->mName == inName)
		{
			templateSet.erase(iter);
			break;
		}
	}
	
	if (sCurrentSetName != kCustomSetName)
	{
		sCurrentSetName = kCustomSetName;
		
		//	If we have to add the custom item, then send that the list changed.
		if (sCustomTemplates.find(kCustomSetName) == sCustomTemplates.end())
		{
			sTemplateSetNames.insert(sTemplateSetNames.begin() + 1, kCustomSetName);
			ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateSetListChanged());
		}
	}
	
	sCustomTemplates[sCurrentSetName] = templateSet;

	ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateChanged());
}

void MoveMarkerInCurrentSet(
	size_t inOldIndex,
	size_t inNewIndex)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	
	TemplateSet templateSet = sCustomTemplates[sCurrentSetName];
	TemplateSet::iterator iter = templateSet.begin() + inOldIndex;

	Template templ = *iter;
	templateSet.erase(iter);
	
	templateSet.insert(templateSet.begin() + inNewIndex, templ);
	
	if (sCurrentSetName != kCustomSetName)
	{
		sCurrentSetName = kCustomSetName;
		
		//	If we have to add the custom item, then send that the list changed.
		if (sCustomTemplates.find(kCustomSetName) == sCustomTemplates.end())
		{
			sTemplateSetNames.insert(sTemplateSetNames.begin() + 1, kCustomSetName);
			ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateSetListChanged());
		}
	}
	
	sCustomTemplates[sCurrentSetName] = templateSet;

	ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateChanged());
}

ASL::Result SaveCustomMarkerTemplateSet(
	const dvacore::UTF16String& inSetName,
	bool inForceOverwrite)
{
	ASL::CriticalSectionLock lock(sCriticalSection);
	
	TemplateSet templateSet = sCustomTemplates[sCurrentSetName];
	dvacore::UTF16String setNameGuid;

	CustomTemplatesNameMap::const_iterator nameMapIter = sCustomTemplateSetNames.find(inSetName);
	if (nameMapIter != sCustomTemplateSetNames.end())
	{
		if (!inForceOverwrite)
		{
			return ASL::eFileAlreadyExists;
		}

		setNameGuid = nameMapIter->second;		
	} else {
		setNameGuid = ASL::Guid::CreateUnique().AsString();
	}

	dvacore::UTF16String outPath = GetUserTemplateSetDirectory();
	ASL::Directory::CreateOnDisk(outPath, true);
	outPath = ASL::PathUtils::AddTrailingSlash(outPath);
	outPath += setNameGuid;
	outPath += kTemplateSetExtension;
	
	if (WriteTemplateSetToDisk(templateSet, inSetName, outPath))
	{
		sCustomTemplates[inSetName] = templateSet;
		sCustomTemplateSetNames[inSetName] = setNameGuid;

		if (inSetName != sCurrentSetName)
		{
			bool nameReplaced = false;
			if (inSetName != kDefaultEmptySet)
			{
				sCustomTemplates.erase(sCurrentSetName);
				sCustomTemplateSetNames.erase(sCurrentSetName);

				TemplateSetNames::iterator iter = sTemplateSetNames.begin();
				//	If the current item is <custom>, then delete it.
				if (sCurrentSetName == kCustomSetName)
				{
					for ( ; iter != sTemplateSetNames.end(); ++iter)
					{
						if (*iter == sCurrentSetName)
						{
							sTemplateSetNames.erase(iter);
							break;
						}
					}
				}
				//	If the new item is already in the list, don't add it.
				iter = sTemplateSetNames.begin();
				for ( ; iter != sTemplateSetNames.end(); ++iter)
				{
					if (*iter == inSetName)
					{
						nameReplaced = true;
						break;
					}
				}
			}
			if (!nameReplaced)
			{
				sTemplateSetNames.push_back(inSetName);
			}
			SetCurrentTemplateSetName(inSetName);
		}
		ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateSetListChanged());
		return ASL::kSuccess;
	}

	return ASL::eUnknown;
}

bool IsSameAsDefaultName(const dvacore::UTF16String& inSetName)
{
	return inSetName == kDefaultEmptySet; 
}
void DeleteMarkerTemplateSet(
	const dvacore::UTF16String& inSetName)
{
	dvacore::UTF16String path = GetUserTemplateSetDirectory();
	path = ASL::PathUtils::AddTrailingSlash(path);

	CustomTemplatesNameMap::const_iterator nameMapIter = sCustomTemplateSetNames.find(inSetName);
	if (nameMapIter != sCustomTemplateSetNames.end())
	{
		path += nameMapIter->second;
		path += kTemplateSetExtension;

		if (ASL::PathUtils::ExistsOnDisk(path))
		{
			ASL::File::Delete(path);

			if (sCurrentSetName == inSetName)
			{
				SetCurrentTemplateSetName(kDefaultEmptySet);
			}

			sCustomTemplates.erase(inSetName);
			sCustomTemplateSetNames.erase(inSetName);
			TemplateSetNames::iterator iter = sTemplateSetNames.begin();
			for ( ; iter != sTemplateSetNames.end(); ++iter)
			{
				if (*iter == inSetName)
				{
					sTemplateSetNames.erase(iter);
					break;
				}
			}
			ASL::StationUtils::BroadcastMessage(sStationID, MarkerTemplateSetListChanged());
		}
	}
}

void RefreshDefaultMarkerTemplateSet()
{
	CustomTemplates::iterator defaultTemplate = sCustomTemplates.find(kDefaultEmptySet);
	if (defaultTemplate != sCustomTemplates.end())
	{
		sCustomTemplates.erase(defaultTemplate);
	}

	const PL::MarkerTypeNameList& types = MarkerType::GetSortedMarkerTypes();
	PL::MarkerTypeNameList::const_iterator typesIter = types.begin();

	TemplateSet set;
	
	for ( ; typesIter != types.end(); ++typesIter)
	{
        Template templ;
        templ.mName = MarkerType::GetDisplayNameForType(*typesIter);
        templ.mMarker.SetType(*typesIter);
        set.push_back(templ);
	}
	
	sCustomTemplates[kDefaultEmptySet] = set;
}

void SerializeTemplateToXMLString(
	Template inTemplate,
	dvacore::UTF16String& outString)
{
	outString = ASL::MakeString("");
	AddTagString(kMarkerKey_TemplateName.ToString(), inTemplate.mName, outString);
	AddTagString(kMarkerKey_MarkerType.ToString(), inTemplate.mMarker.GetType(), outString);
	AddTagString(kMarkerKey_MarkerComment.ToString(), inTemplate.mMarker.GetComment(), outString);
	AddTagString(kMarkerKey_MarkerLocation.ToString(), inTemplate.mMarker.GetLocation(), outString);
	AddTagString(kMarkerKey_MarkerSpeaker.ToString(), inTemplate.mMarker.GetSpeaker(), outString);
	AddTagString(kMarkerKey_MarkerProbability.ToString(), inTemplate.mMarker.GetProbability(), outString);
	AddTagString(kMarkerKey_MarkerTarget.ToString(), inTemplate.mMarker.GetTarget(), outString);
	AddTagString(kMarkerKey_MarkerCuePointType.ToString(), inTemplate.mMarker.GetCuePointType(), outString);
	dvatemporalxmp::CustomMarkerParamList cuePointList = inTemplate.mMarker.GetCuePointList();
	std::int64_t numCuePoints = cuePointList.size();
	dvacore::UTF16String numCuePointsString = dvacore::utility::NumberToString(numCuePoints);
	AddTagString(kMarkerKey_MarkerCuePointCount.ToString(), numCuePointsString, outString);

	for (std::int64_t i = 0; i < numCuePoints; i++)
	{
		dvacore::UTF16String cuePointTagString = BE::CreateKeyWithIndex(kMarkerKey_MarkerCuePoint, i + 1).ToString();
		outString += ASL::MakeString("<") + cuePointTagString + ASL::MakeString(">");
		AddTagString(kCuePointKey_Key.ToString(), cuePointList[i].mKey, outString);
		AddTagString(kCuePointKey_Value.ToString(), cuePointList[i].mValue, outString);
		outString += ASL::MakeString("</") + cuePointTagString + ASL::MakeString(">");
	}
}

void AddTagString(
	const dvacore::UTF16String& inTagName,
	const dvacore::UTF16String& inValue,
	dvacore::UTF16String& outString)
{
	outString += ASL::MakeString("<") + inTagName + ASL::MakeString(">");
	outString += inValue;
	outString += ASL::MakeString("</") + inTagName + ASL::MakeString(">");
}

void SerializeTemplateFromXMLString(
	const dvacore::UTF16String& inString,
	Template& outTemplate)
{

	PL::Utilities::ExtractElementValue(inString, kMarkerKey_TemplateName.ToString(), outTemplate.mName);
	
	dvacore::UTF16String value;
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_TemplateName.ToString(), value);
	outTemplate.mMarker.SetName(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerType.ToString(), value);
	outTemplate.mMarker.SetType(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerComment.ToString(), value);
	outTemplate.mMarker.SetComment(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerLocation.ToString(), value);
	outTemplate.mMarker.SetLocation(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerSpeaker.ToString(), value);
	outTemplate.mMarker.SetSpeaker(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerProbability.ToString(), value);
	outTemplate.mMarker.SetProbability(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerTarget.ToString(), value);
	outTemplate.mMarker.SetTarget(value);
	
	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerCuePointType.ToString(), value);
	outTemplate.mMarker.SetCuePointType(value);

	PL::Utilities::ExtractElementValue(inString, kMarkerKey_MarkerCuePointCount.ToString(), value);
	std::int64_t numCuePoints;
	dvacore::utility::StringToNumber(numCuePoints, value);

	dvatemporalxmp::CustomMarkerParamList cuePointList;
	for (std::int64_t i = 0; i < numCuePoints; i++)
	{
		dvacore::UTF16String cuePointString;
		PL::Utilities::ExtractElementValue(inString, BE::CreateKeyWithIndex(kMarkerKey_MarkerCuePoint, i + 1).ToString(), cuePointString);
		dvacore::UTF16String key;
		PL::Utilities::ExtractElementValue(cuePointString, kCuePointKey_Key.ToString(), key);
		dvacore::UTF16String value;
		PL::Utilities::ExtractElementValue(cuePointString, kCuePointKey_Value.ToString(), value);
		dvatemporalxmp::CustomMarkerParam cuePoint(key, value);
		cuePointList.push_back(cuePoint);
	}
	outTemplate.mMarker.SetCuePointList(cuePointList);
}

}

}