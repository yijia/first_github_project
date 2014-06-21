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

// MZ
#include "MZUtilities.h"

// DVA
#include "dvacore/filesupport/file/File.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/filesupport/file/FileException.h"
#include "dvacore/debug/Debug.h"

#include "boost/foreach.hpp"

// Local
#include "TagTemplate/PLTagTemplateInfo.h"
#include "TagTemplate/IPLTagTemplate.h"
#include "TagTemplate/PLTagUtilities.h"
#include "PLMessage.h"

namespace PL
{
	/*
	**
	*/
	TagTemplateInfo::TagTemplateInfo(){}

	/*
	**
	*/
	TagTemplateInfo::~TagTemplateInfo(){}

	/*
	**
	*/
	void TagTemplateInfo::Initialize(
		const ASL::String& inTagTemplateID,
		const ASL::String& inTagTemplatePath)
	{
		mTagTemplateID		= inTagTemplateID;
		mTagTemplatePath	= inTagTemplatePath;
	}

	/*
	**
	*/
	dvacore::UTF16String TagTemplateInfo::GetTagTemplateID() const
	{
		return mTagTemplateID;
	}

	/*
	**
	*/	
	void TagTemplateInfo::SetTagTemplateID(const dvacore::UTF16String& inTagTemplateID)
	{
		mTagTemplateID = inTagTemplateID;
	}

	/*
	**
	*/
	dvacore::UTF16String TagTemplateInfo::GetTagTemplatePath() const
	{
		return mTagTemplatePath;
	}

	/*
	**
	*/	
	void TagTemplateInfo::SetTagTemlatePath(const dvacore::UTF16String& inTagTemplatePath)
	{
		mTagTemplatePath = inTagTemplatePath;
	}

	/*
	**
	*/
	dvacore::UTF16String TagTemplateInfo::GetTagTemplateName() const
	{
		if (mTagTemplateName.empty())
		{
			OnTagTemplateNameChanged();
		}
		return mTagTemplateName;
	}

	/*
	**
	*/
	void TagTemplateInfo::OnTagTemplateNameChanged() const
	{
		auto originalName = mTagTemplateName;
		auto tagTemplate = PL::LoadTagTemplate(mTagTemplatePath);
		if (tagTemplate != NULL)
		{
			mTagTemplateName = tagTemplate->GetName();
		}
		if (mTagTemplateName.empty())
			mTagTemplateName = ASL::PathUtils::GetFilePart(mTagTemplatePath);
	}

	/*
	**
	*/
	bool TagTemplateInfo::IsReadOnly() const
	{
		return mIsReadOnly;
	}

	/*
	**
	*/
	void TagTemplateInfo::SetReadOnly(bool inIsReadOnly)
	{
		mIsReadOnly = inIsReadOnly;
	}

	/*
	**
	*/
	TagTemplateInfoPtr TagTemplateInfo::Create(		
		const ASL::String& inTagTemplateID,
		const ASL::String& inTagTemplatePath,
		bool inReadOnly)
	{
		ASL_ASSERT(!inTagTemplateID.empty());

		TagTemplateInfoPtr tagTemplate(new TagTemplateInfo());

		if (!inTagTemplateID.empty() && tagTemplate != NULL)
		{
			tagTemplate->Initialize(inTagTemplateID, inTagTemplatePath);
			tagTemplate->SetReadOnly(inReadOnly);
		}

		return tagTemplate;
	}

	/*
	** Since ID of every tag template should be unique, we use path as ID presently.
	*/
	dvacore::UTF16String MakeTagTemplateID(dvacore::UTF16String inTagTemplatePath)
	{
		return MZ::Utilities::NormalizePathToComparable(inTagTemplatePath);
	}
}