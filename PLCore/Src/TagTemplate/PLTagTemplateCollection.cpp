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

// Local
#include "TagTemplate/PLTagTemplateCollection.h"

#include "TagTemplate/PLTagUtilities.h"

// MZ
#include "MZUtilities.h"

// ASL
#include "ASLClass.h"
#include "ASLFile.h"
#include "ASLStationUtils.h"
#include "ASLStationRegistry.h"
#include "ASLClassFactory.h"
#include "ASLStringCompare.h"

// DVA
#include "dvacore/filesupport/file/File.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/filesupport/file/FileException.h"
#include "dvacore/debug/Debug.h"

// BE
#include "BE/Core/Iproperties.h"
#include "BEBackend.h"

// boost
#include "boost/foreach.hpp"

namespace PL
{

namespace
{
	const BE::PropertyKey kPropertyKey_UserTagTemplateCount = BE_PROPERTYKEY("PLCore.TagTemplate.TagTemplateCollection.UserTagTemplateCount");
	const dvacore::UTF16String kPropertyKey_UserTagTemplateList = ASL_STR("PLCore.TagTemplate.TagTemplateCollection.UserTagTemplateList");

	/*
	**
	*/
	static BE::PropertyKey MakePropertyKey(const dvacore::UTF16String& inStr, ASL::UInt32 inIndex)
	{
		ASL::OutputStringStream stringStream;
		stringStream << inStr << inIndex;
		return BE_MAKEPROPERTYKEY(stringStream.str().c_str());
	}

}

ASL::CriticalSection TagTemplateCollection::sCriticalSection;
TagTemplateCollection::SharedPtr TagTemplateCollection::sTagTemplateCollection;

TagTemplateCollection::TagTemplateCollection()
	: mStationID(ASL::StationRegistry::UniqueStationID()),
	  mDataIsReady(false)
{

}

/*
**
*/
TagTemplateCollection::~TagTemplateCollection()
{

}

/*
**
*/
ASL::StationID TagTemplateCollection::GetStationID()
{
	return mStationID;
}

/*
**
*/
void TagTemplateCollection::Initialize()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	if (!sTagTemplateCollection)
	{
		sTagTemplateCollection.reset(new TagTemplateCollection());
		sTagTemplateCollection->LoadTagTemplateList();
	}

	ASL_ASSERT(sTagTemplateCollection);
	ASL::StationRegistry::RegisterStation(sTagTemplateCollection->GetStationID());
}

/*
**
*/
void TagTemplateCollection::Terminate()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	if (sTagTemplateCollection)
	{		
		sTagTemplateCollection->ClearTagTemplateList();
		sTagTemplateCollection.reset();
	}	
}

/**
**
*/
TagTemplateInfoPtr TagTemplateCollection::FindTagTemplate(ASL::String inTagTemplateID)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	BOOST_FOREACH(TagTemplateInfoPtr tagTemplate, mSystemTagTemplates)
	{
		if (ASL::CaseInsensitive::StringEquals(tagTemplate->GetTagTemplateID(), inTagTemplateID))
		{
			return tagTemplate;
		}
	}

	BOOST_FOREACH(TagTemplateInfoPtr tagTemplate, mUserTagTemplates)
	{
		if (ASL::CaseInsensitive::StringEquals(tagTemplate->GetTagTemplateID(), inTagTemplateID))
		{
			return tagTemplate;
		}
	}

	return TagTemplateInfoPtr();
}

TagTemplateInfoPtr TagTemplateCollection::FindSystemTagTemplateByPath(ASL::String inTagTemplatePath)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	BOOST_FOREACH(TagTemplateInfoPtr tagTemplate, mSystemTagTemplates)
	{
		if (ASL::CaseInsensitive::StringEquals(
			MZ::Utilities::NormalizePathToComparable(tagTemplate->GetTagTemplatePath()), 
			MZ::Utilities::NormalizePathToComparable(inTagTemplatePath)))
		{
			return tagTemplate;
		}
	}

	return TagTemplateInfoPtr();
}

TagTemplateInfoPtr TagTemplateCollection::FindUserTagTemplateByPath(ASL::String inTagTemplatePath)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	BOOST_FOREACH(TagTemplateInfoPtr tagTemplate, mUserTagTemplates)
	{
		if (ASL::CaseInsensitive::StringEquals(
			MZ::Utilities::NormalizePathToComparable(tagTemplate->GetTagTemplatePath()),
			MZ::Utilities::NormalizePathToComparable(inTagTemplatePath)))
		{
			return tagTemplate;
		}
	}

	return TagTemplateInfoPtr();
}

TagTemplateInfoPtr TagTemplateCollection::FindTagTemplateByPath(ASL::String inTagTemplatePath)
{
	TagTemplateInfoPtr outTagTemplate = FindSystemTagTemplateByPath(inTagTemplatePath);
	if (outTagTemplate)
	{
		return outTagTemplate;
	}

	return FindUserTagTemplateByPath(inTagTemplatePath);
}

/**
**
*/
TagTemplateInfoPtr TagTemplateCollection::CreateTagTemplateItem(
	ASL::String inTagTemplateID,
	ASL::String inTagTemplatePath)
{
	TagTemplateInfoPtr tagTemplate;

	if (!ASL::PathUtils::ExistsOnDisk(inTagTemplatePath))
	{
		return tagTemplate;
	}

	bool readonly = ASL::PathUtils::IsReadOnly(inTagTemplatePath)
					|| GetInstance()->IsSystemTagTemplatePath(inTagTemplatePath);

	tagTemplate = TagTemplateInfo::Create(inTagTemplateID, inTagTemplatePath, readonly);

	return tagTemplate;
}

/**
**
*/
ASL::Result TagTemplateCollection::AddTagTemplate(TagTemplateInfoPtr inTagTemplate)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	if (FindTagTemplate(inTagTemplate->GetTagTemplateID()))
		return ASL::ResultFlags::kResultTypeFailure;

	if(IsSystemTagTemplatePath(inTagTemplate->GetTagTemplatePath()))
		mSystemTagTemplates.push_back(inTagTemplate);
	else
		mUserTagTemplates.push_back(inTagTemplate);
	return ASL::kSuccess;
}

/*
**
*/
void TagTemplateCollection::SetSystemTagTemplatePath(dvacore::UTF16String inPath)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	mSystemTagTemplatePath = ASL::PathUtils::AddTrailingSlash(inPath);
}

/*
**
*/
dvacore::UTF16String TagTemplateCollection::GetSystemTagTemplatePath()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	return mSystemTagTemplatePath;
}

/*
**
*/
TagTemplateList TagTemplateCollection::GetUserTagTemplates()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	return mUserTagTemplates;
}

/*
**
*/
TagTemplateList TagTemplateCollection::GetSystemTagTemplates()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	return mSystemTagTemplates;
}

/**
**
*/
ASL::Result TagTemplateCollection::RemoveTagTemplate(TagTemplateInfoPtr inTagTemplate)
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	TagTemplateList::iterator iter = std::find(
		mUserTagTemplates.begin(), 
		mUserTagTemplates.end(),
		inTagTemplate);

	if (iter != mUserTagTemplates.end())
	{
		mUserTagTemplates.erase(iter);
	}
	return ASL::kSuccess;
}

/**
**
*/
ASL::Result TagTemplateCollection::UpdateTagTemplate()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	return ASL::kSuccess;
}

/**
**
*/
TagTemplateCollection::SharedPtr TagTemplateCollection::GetInstance()
{
	return sTagTemplateCollection;
}

/*
**
*/
ASL::CriticalSection& TagTemplateCollection::GetLocker()
{
	return sCriticalSection;
}


/*
**
*/
void TagTemplateCollection::LoadTagTemplateList()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	if(!IsDataReady())
	{
		sTagTemplateCollection->SetSystemTagTemplatePath(TagUtilities::GetSystemTagTemplateFolderPath());
		ImportSystemTagTemplates();
		ImportUserTagTemplates();
		SetDataReady(true);
	}
}

/*
**
*/
void TagTemplateCollection::ImportSystemTagTemplates()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	dvacore::filesupport::Dir systemTagTemplateFolder(mSystemTagTemplatePath);

	if (!systemTagTemplateFolder.Exists())
	{
		DVA_TRACE("TagTemplateCollection::Initialize", 5, "System tag template folder does not exist");
	}
	else
	{
		TagTemplateInfoPtr newTagTemplate;
		for (dvacore::filesupport::Dir::FileIterator file = systemTagTemplateFolder.BeginFiles(false);
			file != systemTagTemplateFolder.EndFiles();
			file++)
		{
			dvacore::UTF16String filePath((*file).FullPath());
			dvacore::UTF16String fileExtension = (*file).GetExtension();

			if (ASL::CaseInsensitive::StringEquals(fileExtension, DVA_STR(".json"))
				|| ASL::CaseInsensitive::StringEquals(fileExtension, DVA_STR(".xml")))
			{
				newTagTemplate = CreateTagTemplateItem(MakeTagTemplateID(filePath), filePath);

				if (newTagTemplate)
				{
					mSystemTagTemplates.push_back(newTagTemplate);
				}
			}
		}
	}
}

/*
**
*/
void TagTemplateCollection::ImportUserTagTemplates()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef backendProps(backend);

	ASL::UInt32 templateCount = 0;
	dvacore::UTF16String templatePath;

	if (backendProps->GetValue(kPropertyKey_UserTagTemplateCount, templateCount))
	{
		TagTemplateInfoPtr newTagTemplate;
		for (ASL::UInt32 index = 1; index <= templateCount; ++index)
		{
			if (backendProps->GetValue(MakePropertyKey(kPropertyKey_UserTagTemplateList, index), templatePath))
			{
				newTagTemplate = CreateTagTemplateItem(MakeTagTemplateID(templatePath), templatePath);

				if (newTagTemplate)
				{
					mUserTagTemplates.push_back(newTagTemplate);
				}
			}
		}
	}
}

/**
**
*/
void TagTemplateCollection::PersistUserTagTemplateList()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());

	BE::IBackendRef backend = BE::GetBackend();
	BE::IPropertiesRef backendProps(backend);

	ASL::UInt32 templateCount = 0;

	if (backendProps->GetValue(kPropertyKey_UserTagTemplateCount, templateCount))
	{
		for (ASL::UInt32 index = 1; index <= templateCount; ++index)
		{
			BE::PropertyKey propKey = MakePropertyKey(kPropertyKey_UserTagTemplateList, index);
			if (backendProps->PropertyExists(propKey))
			{
				backendProps->ClearValue(propKey);
			}
		}
	}

	templateCount = 0;
	BOOST_FOREACH(TagTemplateInfoPtr tagTemplate, mUserTagTemplates)
	{		
		templateCount++;
		backendProps->SetValue(MakePropertyKey(kPropertyKey_UserTagTemplateList, templateCount), tagTemplate->GetTagTemplatePath(), BE::kPersistent);
	}

	backendProps->SetValue(kPropertyKey_UserTagTemplateCount, templateCount, BE::kPersistent);
}

/**
**
*/
void TagTemplateCollection::ClearTagTemplateList()
{
	ASL::CriticalSectionLock lock(TagTemplateCollection::GetLocker());
	
	mSystemTagTemplates.clear();
	mUserTagTemplates.clear();
}


/*
**
*/
bool TagTemplateCollection::IsDataReady()
{
	return mDataIsReady;
}


/*
**
*/
void TagTemplateCollection::SetDataReady(bool inDataIsReady)
{
	mDataIsReady = inDataIsReady;
}

bool TagTemplateCollection::IsSystemTagTemplatePath(const ASL::String& inPath)
{
	return MZ::Utilities::NormalizePathToComparable(mSystemTagTemplatePath)
		== MZ::Utilities::NormalizePathToComparable(ASL::PathUtils::GetFullDirectoryPart(inPath));
}

}
