/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004-2011 Adobe Systems Incorporated                  */
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

#include "PLDragMarkerInfo.h"

//	ASL
#include "ASLCoercion.h"

// MZ
#include "PLXMLUtilities.h"

// dvacore
#include "dvacore/xml/XMLUtils.h"


namespace PL
{
	static const dvacore::UTF16String kDragMarkerInfoList		= DVA_STR("dragMarkerInfoList");
	static const dvacore::UTF16String kDragMarkerInfo			= DVA_STR("dragMarkerInfo");
	static const dvacore::UTF16String kMarkerGUID				= DVA_STR("markerGUID");
	static const dvacore::UTF16String kMediaPath				= DVA_STR("mediaPath");
	static const dvacore::UTF16String kIsZeroDuration				= DVA_STR("isZeroDuration");

	/*
	**
	*/
	void CreateAndSetNodeValue(
		DOMElement* inParent, 
		ASL::String const& inNodeName,
		ASL::String const& inNodeValue)
	{
		if (!inNodeValue.empty())
		{
			XMLUtils::AddChild(inParent, inNodeName.c_str(), inNodeValue.c_str());
		}
	}

	/*
	**
	*/
	void CreateAndSetNodeValue(
		DOMElement* inParent, 
		ASL::String const& inNodeName,
		bool const& inBool)
	{
		XMLUtils::AddChild(
			inParent, 
			inNodeName.c_str(),
			ASL::Coercion<ASL::String>::Result(inBool));
	}

	/*
	**
	*/
	void CreateAndSetNodeValue(
		DOMElement* inParent, 
		ASL::String const& inNodeName,
		dvamediatypes::TickTime const& inTickTime)
	{
		if (inTickTime != dvamediatypes::kTime_Invalid)
		{
			XMLUtils::AddChild(
				inParent, 
				inNodeName.c_str(),
				ASL::Coercion<ASL::String>::Result(inTickTime.GetTicks()).c_str());
		}
	}

	/*
	**
	*/
	ASL::String EncodeDragMarkerInfoList(
		DragMarkerInfoList const& inDragMarkerInfoList)
	{
		//	create DOMDocument
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = XMLUtilities::CreateDomDocument();

		//	create message root node
		DOMElement* rootElement = pDoc->createElement(kDragMarkerInfoList.c_str());
		pDoc->appendChild(rootElement);

		if (0 < inDragMarkerInfoList.size())
		{
			DragMarkerInfoList::const_iterator iter = inDragMarkerInfoList.begin();
			DragMarkerInfoList::const_iterator end = inDragMarkerInfoList.end();

			ASL::SInt32 index = 1;
			for (; iter != end ; ++iter, ++index)
			{
				//	dragMarkerInfo
				DOMElement* dragMarkerInfoElement 
					= XMLUtils::AddChild(rootElement, kDragMarkerInfo.c_str());

				//	markerGUID
				CreateAndSetNodeValue(
					dragMarkerInfoElement,
					kMarkerGUID,
					iter->mMarkerGUID.AsString());

				//	mediaPath
				CreateAndSetNodeValue(
					dragMarkerInfoElement,
					kMediaPath,
					iter->mMediaPath);

				//	isZeroDuration
				CreateAndSetNodeValue(
					dragMarkerInfoElement,
					kIsZeroDuration,
					iter->mIsZeroDuration);
			}
		}

		//  Convert Dom object to XML String
		ASL::String result = XMLUtilities::ConvertDomToXMLString(pDoc);

		pDoc->release();
		return result;
	}

	/*
	**
	*/
	bool DecodeDragMarkerInfoList(
		ASL::String const&	inXMLString,
		DragMarkerInfoList& outDragMarkerInfoList)
	{
		bool result(true);
		XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = NULL;

		result = XMLUtilities::GetDOMDocument(&parser, inXMLString, &pDoc);

		if (result)
		{
			DOMElement* rootElement = pDoc->getDocumentElement();
			result = (rootElement != NULL);
			if (result)
			{
				//	dragMarkerInfo
				const DOMNode* dragMarkerInfoNode =
					XMLUtils::FindChild(rootElement, kDragMarkerInfo.c_str());

				if (dragMarkerInfoNode)
				{
					do 
					{
						//	markerGUID
						ASL::String  markerGUID;
						XMLUtils::GetChildData(dragMarkerInfoNode, kMarkerGUID.c_str(), markerGUID);

						//	mediaPath
						ASL::String  mediaPath;
						XMLUtils::GetChildData(dragMarkerInfoNode, kMediaPath.c_str(), mediaPath);

						//	isZeroDuration
						bool  isZeroDuration;
						XMLUtils::GetChildData(dragMarkerInfoNode, kIsZeroDuration.c_str(), &isZeroDuration);

						DragMarkerInfo dragMarkerInfo(markerGUID.empty()? ASL::Guid():ASL::Guid(markerGUID), mediaPath, isZeroDuration);

						outDragMarkerInfoList.push_back(dragMarkerInfo);
					} while ( (dragMarkerInfoNode = XMLUtils::FindNextSibling(dragMarkerInfoNode, kDragMarkerInfo.c_str())) != NULL);
				}
			}
		}

		return result;
	}
}