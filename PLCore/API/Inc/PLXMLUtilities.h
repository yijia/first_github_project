/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2011 Adobe Systems Incorporated                       */
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

#ifndef PLXMLUTILITIES_H
#define PLXMLUTILITIES_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef XMLUTILS_H
#include "XMLUtils.h"
#endif

#ifndef XERCESC_INCLUDE_GUARD_MEMBUFINPUTSOURCE_HPP
#include "xercesc/framework/MemBufInputSource.hpp"
#endif

namespace PL
{

namespace XMLUtilities
{
/**
**
*/ 
PL_EXPORT
bool GetDOMDocument(
	XercesDOMParser* inParserPtr,
	ASL::String const& inXMLString,
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument** outdoc);

/**
**
*/ 
PL_EXPORT
ASL::String ConvertDomToXMLString(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inDoc);
/**
**
*/
PL_EXPORT
XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* CreateDomDocument();
} // namespace XMLUtilites

} // namespace PL

#endif