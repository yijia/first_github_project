/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

//	Prefix
#include "Prefix.h"

//	Self
#include "PLXMLUtilities.h"
// MediaCore
#include "XMLUtils.h"
#include "xercesc/framework/MemBufInputSource.hpp"

XERCES_CPP_NAMESPACE_USE

namespace PL
{

namespace XMLUtilities
{
/*
**
*/
bool GetDOMDocument(
	XercesDOMParser* inParserPtr,
	ASL::String const& inXMLString,
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument** outdoc)
{
	XercesDOMParser::ValSchemes valScheme = XercesDOMParser::Val_Auto;
	bool doNamespaces = false;
	bool doSchema = false;
	bool schemaFullChecking = false;
	bool doCreate = false;
	bool retval = true;

	inParserPtr->setValidationScheme(valScheme);
	inParserPtr->setDoNamespaces(doNamespaces);
	inParserPtr->setDoSchema(doSchema);
	inParserPtr->setValidationSchemaFullChecking(schemaFullChecking);
	inParserPtr->setCreateEntityReferenceNodes(doCreate);

	std::string stdXmlString = ASL::MakeStdString(inXMLString);
	const XMLByte* pXmlByte = reinterpret_cast<const XMLByte*>(stdXmlString.c_str());
	XMLSize_t  xmlSize = stdXmlString.size();

	try
	{
		xercesc::MemBufInputSource xml_buf(
			pXmlByte, 
			xmlSize,
			"ACMessageXML(memory)");
		inParserPtr->parse(xml_buf);
		*outdoc = inParserPtr->getDocument();
	}
	catch (...)
	{
		retval = false;
	}

	return retval;
}

/*
**
*/
ASL::String ConvertDomToXMLString(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inDoc)
{
	XMLCh tempStr[100];
	XMLString::transcode("LS", tempStr, 99);
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(tempStr);
	DOMLSSerializer* theSerializer = static_cast<DOMImplementationLS*>(impl)->createLSSerializer();
	// set user specified end of line sequence and output encoding
	theSerializer->setNewLine(XMLString::transcode("\n"));
	XMLCh* tempRes = theSerializer->writeToString(inDoc);
	ASL::String result(tempRes);
	XMLString::release(&tempRes);
	theSerializer->release();
	return result;
}

/*
**
*/
XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* CreateDomDocument()
{
	DOMImplementation *pImpl = DOMImplementation::getImplementation();
	return pImpl->createDocument(NULL, NULL, NULL);
}
} // namespace XMLUtilities

} // namespace PL