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


#ifndef PLTAGTEMPLATECOLLECTION_H
#define PLTAGTEMPLATECOLLECTION_H

// ASL
#include "ASLListener.h"
#include "ASLStation.h"

#ifndef PLMESSAGE_H
#include "PLMessage.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

// Local
#include "TagTemplate/PLTagTemplateInfo.h"

namespace PL
{
class TagTemplateCollection
{
public:

	typedef boost::shared_ptr<TagTemplateCollection>	SharedPtr;

	/**
	**
	*/
	~TagTemplateCollection();

	/**
	**
	*/
	ASL::StationID GetStationID();

	/*
	**
	*/
	PL_EXPORT
	static void Initialize();

	/*
	**
	*/
	PL_EXPORT
	static void Terminate();

	/*
	**
	*/
	PL_EXPORT
	TagTemplateInfoPtr FindTagTemplate(ASL::String inTagTemplateID);

	/*
	**
	*/
	PL_EXPORT
	TagTemplateInfoPtr FindTagTemplateByPath(ASL::String inTagTemplatePath);

	/*
	**
	*/
	PL_EXPORT
	TagTemplateInfoPtr FindSystemTagTemplateByPath(ASL::String inTagTemplatePath);

	/*
	**
	*/
	PL_EXPORT
	TagTemplateInfoPtr FindUserTagTemplateByPath(ASL::String inTagTemplatePath);

	/*
	**
	*/
	PL_EXPORT
	static TagTemplateInfoPtr CreateTagTemplateItem(
		ASL::String inTagTemplateID,
		ASL::String inTagTemplatePath);

	/**
	**
	*/
	PL_EXPORT
	ASL::Result AddTagTemplate(TagTemplateInfoPtr inTagTemplate);

	/**
	**
	*/
	PL_EXPORT
	TagTemplateList GetUserTagTemplates();

	/**
	**
	*/
	PL_EXPORT
	TagTemplateList GetSystemTagTemplates();


	/**
	**
	*/
	PL_EXPORT
	ASL::Result RemoveTagTemplate(dvacore::UTF16String inTagTemplate);

	/*
	**
	*/
	PL_EXPORT
	void SetSystemTagTemplatePath(dvacore::UTF16String inPath);

	/*
	**
	*/
	PL_EXPORT
	dvacore::UTF16String GetSystemTagTemplatePath();

	/**
	**
	*/
	PL_EXPORT
	ASL::Result RemoveTagTemplate(TagTemplateInfoPtr inTagTemplate);

	/**
	**
	*/
	PL_EXPORT
	ASL::Result UpdateTagTemplate();

	/*
	**	Singleton instance
	*/
	PL_EXPORT
	static TagTemplateCollection::SharedPtr GetInstance();

	/*
	**
	*/
	PL_EXPORT
	void LoadTagTemplateList();

	/*
	**
	*/
	PL_EXPORT
	void PersistUserTagTemplateList();

	/**
	**
	*/
	PL_EXPORT
	bool IsSystemTagTemplatePath(const ASL::String& inPath);

	/*
	**	Synchronization Lock
	*/
	static ASL::CriticalSection& GetLocker();

	/*
	**
	*/
	void ClearTagTemplateList();

	/*
	**
	*/
	bool IsDataReady();

	/*
	**
	*/
	void SetDataReady(bool inIsDataReady);

protected:

	dvacore::UTF16String							mSystemTagTemplatePath;
	ASL::StationID									mStationID;

private:

	/**
	**
	*/
	TagTemplateCollection();

	/*
	**
	*/
	void ImportSystemTagTemplates();

	/*
	**
	*/
	void ImportUserTagTemplates();

	static TagTemplateCollection::SharedPtr			sTagTemplateCollection;
	static ASL::CriticalSection						sCriticalSection;

	TagTemplateList									mSystemTagTemplates;
	TagTemplateList									mUserTagTemplates;

	bool											mDataIsReady;
};
}

#endif
