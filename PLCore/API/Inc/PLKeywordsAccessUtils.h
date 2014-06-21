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

#ifndef PLKEYWORDSACCESSUTILS_H
#define PLKEYWORDSACCESSUTILS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef INCLUDEXMP_H
#include "IncludeXMP.h"
#endif

#ifndef DVATEMPORALXMP_XMPKEYWORDDATA_H
#include "dvatemporalxmp/XMPKeywordData.h"
#endif

namespace PL
{

namespace KeywordsAccessUtils
{
	PL_EXPORT
	PL::TagParamMap GetTagParamsFromKeywordList(const dvatemporalxmp::XMPKeywordList& inKeywordList);

	PL_EXPORT
	dvatemporalxmp::XMPKeywordList GetKeywordListFromTagParams(const PL::TagParamMap& inTagParams);
}

}

#endif