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

#ifndef PLTAGUTILITIES_H
#define PLTAGUTILITIES_H

#ifndef PLTAGTEMPLATEINFO_H
#include "TagTemplate/PLTagTemplateInfo.h"
#endif

#ifndef IPLTAGTEMPLATE_H
#include "TagTemplate/IPLTagTemplate.h"
#endif

#include "PLUtilities.h"

#include "ASLStringUtils.h"

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

namespace PL
{
	namespace TagUtilities
	{

		/*
		**
		*/
		PL_EXPORT
		void UnregisterTagTemplate(const ASL::StringVector& inPathList);

		/*
		**	
		*/
		PL_EXPORT
		void  RegisterTagTemplate(const ASL::StringVector& inTagTemplateList);

		/*
		**	
		*/
		PL_EXPORT
		void RegisterCloudData(const PL::Utilities::CloudData& inCloudData);

		/*
		**	
		*/
		PL_EXPORT
		bool RegisterFromTagTemplateMessage(const PL::TagTemplateMessageInfoList& inTagTemplateMessageInfoList, PL::TagTemplateMessageOptionType inTagTemplateOption, PL::TagTemplateMessageResultList& outTagTemplateMessageResultList);

		/*
		**
		*/
		PL_EXPORT
		bool SaveTagTemplateAs(PL::IPLTagTemplateReadonlyRef inTagTemplate, const ASL::String& inNewPath, ASL::String& errorMessage);

		/*
		**
		*/
		PL_EXPORT
		bool NewTagTemplate(const ASL::String& inNewPath);

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetTagTemplateFileExtension();

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetTagTemplateFileTypeString();	

		/**
		** 
		*/
		PL_EXPORT
		dvacore::UTF16String GetSystemTagTemplateFolderPath(bool inCreateNonExist = false);

		/**
		** 
		*/
		PL_EXPORT
		ASL::StationID GetTagTemplateStationID();
	}
}

#endif
