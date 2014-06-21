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

#ifndef PLTAGTEMPLATEINFO_H
#define PLTAGTEMPLATEINFO_H

// ASLFoundation
#include "ASLClass.h"
#include "ASLResult.h"
#include "ASLListener.h"

// DVA
#include "dvacore/config/UnicodeTypes.h"

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif


namespace PL
{
struct TagTemplateBasicData
{
	TagTemplateBasicData(const ASL::String& inTagTemplateID, const ASL::String& inTagTemplatePath)
		:	
		mTagTemplateID(inTagTemplateID), 
		mTagTemplatePath(inTagTemplatePath)
		{};

	ASL::String	mTagTemplateID;
	ASL::String	mTagTemplatePath;
};
typedef boost::shared_ptr<TagTemplateBasicData> TagTemplateBasicDataPtr;
typedef std::list<TagTemplateBasicDataPtr> TagTemplateBasicDataList;

class TagTemplateInfo;

typedef boost::shared_ptr<TagTemplateInfo> TagTemplateInfoPtr;
typedef std::vector<TagTemplateInfoPtr> TagTemplateList;

class TagTemplateInfo
{

public:

	/*
	**
	*/
	TagTemplateInfo();

	/*
	**
	*/
	~TagTemplateInfo();

	/*
	**
	*/
	PL_EXPORT
	void Initialize(
		const ASL::String& inTagTemplateID,
		const ASL::String& inTagTemplatePath);

	/*
	**
	*/
	PL_EXPORT
	dvacore::UTF16String GetTagTemplateID() const;

	/*
	**
	*/
	PL_EXPORT
	void SetTagTemplateID(const dvacore::UTF16String& inTagTemplateID);

	/*
	**
	*/
	PL_EXPORT
	dvacore::UTF16String GetTagTemplatePath() const;

	/*
	**
	*/
	PL_EXPORT
	void SetTagTemlatePath(const dvacore::UTF16String& inTagTemplatePath);

	/*
	**
	*/
	PL_EXPORT
	dvacore::UTF16String GetTagTemplateName() const;

	/*
	**
	*/
	PL_EXPORT
	void OnTagTemplateNameChanged() const;

	/*
	**
	*/
	PL_EXPORT
	bool IsReadOnly() const;


	/*
	**
	*/
	PL_EXPORT
	void SetReadOnly(bool inIsReadOnly);

	/*
	**
	*/
	PL_EXPORT
	static TagTemplateInfoPtr Create(		
		const ASL::String& inTagTemplateID,
		const ASL::String& inTagTemplatePath,
		bool inReadOnly);

private:
	dvacore::UTF16String					mTagTemplateID;
	dvacore::UTF16String					mTagTemplatePath;
	mutable dvacore::UTF16String			mTagTemplateName;
	bool									mIsReadOnly;
};

/**
** 
*/
PL_EXPORT
	dvacore::UTF16String MakeTagTemplateID(dvacore::UTF16String inTagTemplatePath);
} // namespace PL

#endif