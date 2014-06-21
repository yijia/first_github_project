/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2013 Adobe Systems Incorporated                       */
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

// local
#include "PLKeywordsAccessUtils.h"
#include "PLUtilities.h"

// dvacore
#include "dvacore/utility/Coercion.h"
#include "dvacore/utility/StringCompare.h"

// dvametadata


namespace PL
{

namespace KeywordsAccessUtils
{

	PL::TagParamMap GetTagParamsFromKeywordList(const dvatemporalxmp::XMPKeywordList& inKeywordList)
	{
		PL::TagParamMap tagParamMap;

		for (dvatemporalxmp::XMPKeywordList::const_iterator it = inKeywordList.begin();
			it != inKeywordList.end();
			it++)
		{
			PL::TagParamPtr tagParam(new PL::TagParam(*it));

			tagParamMap.insert(PL::TagParamMap::value_type(
				(*it).GetKeywordExtension().GetTagDescription().GetIndex(),
				tagParam));
		}

		return tagParamMap;
	}

	dvatemporalxmp::XMPKeywordList GetKeywordListFromTagParams(const PL::TagParamMap& inTagParams)
	{
		dvatemporalxmp::XMPKeywordList keywords;

		std::uint32_t index = 0;
		for (PL::TagParamMap::const_iterator it = inTagParams.begin();
			it != inTagParams.end();
			it++)
		{
			std::uint32_t argbColor = PL::Utilities::ConvertColorRGBAToUINT32((*it).second->GetColor());

			dvatemporalxmp::XMPDVATagDescription dvaTagDescription(
				ASL::MakeStdString((*it).second->GetPayload()), 
				argbColor, 
				index++);

			dvatemporalxmp::XMPKeywordExtension keywordExtension(dvaTagDescription);

			// save tag to xmpKeyword, we need to keep the guid to XMP
			dvatemporalxmp::XMPKeywordData keywordItem(
				ASL::MakeStdString((*it).second->GetName()), 
				keywordExtension,
				ASL::MakeStdString((*it).second->GetInstanceGuid().AsString()));

			keywords.push_back(keywordItem);
		}

		return keywords;
	}

}

}