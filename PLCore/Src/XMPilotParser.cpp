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

#include "Prefix.h"

// Local
#include "PLXMPilotParser.h"

//	MZ
#include "PLXMLUtilities.h"
#include "IPLMarkers.h"
#include "PLUtilities.h"
#include "PLProject.h"
#include "PLConstants.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLFile.h"
#include "ASLStationUtils.h"
#include "ASLMixHashGuid.h"

//  dva
#include "dvacore/utility/StringCompare.h"

namespace PL
{
	
namespace
{
	const ASL::String kMediaProfile = DVA_STR("MediaProfile");
	const ASL::String kContents = DVA_STR("Contents");
	const ASL::String kMaterial = DVA_STR("Material");
	const ASL::String kProxy = DVA_STR("Proxy");
	const ASL::String kRelevantInfo = DVA_STR("RelevantInfo");

	const ASL::String kUri = DVA_STR("uri");
	const ASL::String kUriType = DVA_STR("type");
	const ASL::String kUmid = DVA_STR("umid");
	const ASL::String kStatus = DVA_STR("status");
	const ASL::String kUmidRef = DVA_STR("umidRef");
	const ASL::String kXmlUri = DVA_STR("XML"); // uri type is XML

	const ASL::String kMXFExtension = DVA_STR(".MXF");

	const ASL::String kMediaProFile = DVA_STR("MEDIAPRO.XML");

	// special for planning metadata
	const ASL::String kGeneralDir = DVA_STR("General");
	const ASL::String kSonyDir = DVA_STR("Sony");
	const ASL::String kPlanningDir = DVA_STR("Planning");

	const ASL::String kPlanningMetadata = DVA_STR("PlanningMetadata");
	const ASL::String kProperties = DVA_STR("Properties");
	const ASL::String kPropertyId = DVA_STR("propertyId");
	const ASL::String kClass = DVA_STR("class");
	const ASL::String kClass_Original = DVA_STR("original");
	const ASL::String kClass_Modified = DVA_STR("modified");
	const ASL::String kPropertyId_Assign = DVA_STR("assignment");
	const ASL::String kPropertyId_Memo = DVA_STR("memo");
	const ASL::String kTitle = DVA_STR("Title");
	const ASL::String kMeta = DVA_STR("Meta");
	const ASL::String kName = DVA_STR("name");
	const ASL::String kContent = DVA_STR("content");
	const ASL::String kDescription = DVA_STR("Description");
	const ASL::String kType = DVA_STR("type");
	const ASL::String kMaterialGroup = DVA_STR("MaterialGroup");

	// special for EssenceMark
	const ASL::String kNonRealTimeMeta = DVA_STR("NonRealTimeMeta");
	const ASL::String kKlvPacketTable = DVA_STR("KlvPacketTable");
	const ASL::String kKlvPacket = DVA_STR("KlvPacket");

	const ASL::String kKey = DVA_STR("key");
	const ASL::String kFrameCount = DVA_STR("frameCount");
	const ASL::String kLengthValue = DVA_STR("lengthValue");
	const ASL::String kEssenceMarkKey_Ascii = DVA_STR("060E2B34010101050301020A02000000"); // special key for EssenceMark
	const ASL::String kEssenceMarkKey_Unicode = DVA_STR("060E2B34010101050301020A02010000"); // special key for EssenceMark

	ASL::String HandleWhiteSpaceInKey(const ASL::String& inStr)
	{
		ASL::String result = inStr;
		ASL::String space = DVA_STR(" ");
		ASL::String::size_type pos = result.find(space);
		while (pos != ASL::String::npos)
		{
			result = result.substr(0, pos) + result.substr(pos+1);
			pos = result.find(space);
		}
		return result;
	}

	ASL::String HandleCommaInValue(const ASL::String& inStr)
	{
		ASL::String result = inStr;
		boost::algorithm::replace_all(result, DVA_STR("\\,"), DVA_STR(","));
		return result;
	}

	ASL::String GetSubStrSeparatedByComma(ASL::String& ioStr)
	{
		ASL::String comma = DVA_STR(",");
		ASL::String::size_type pos = ioStr.find(comma);
		while (pos == 0) // start by ","
		{
			ioStr = ioStr.substr(1);
			pos = ioStr.find(comma);
		}

		ASL::String result;
		if (pos == ASL::String::npos)
		{
			result = ioStr;
			ioStr.clear();
		}
		else
		{
			result = ioStr.substr(0, pos);
			ioStr = ioStr.substr(pos+1);

			// we need handle "\,"
			if (result.substr(pos-1) == DVA_STR("\\"))
			{
				result += comma;
				result += GetSubStrSeparatedByComma(ioStr);
			}
		}
		return result;
	}

	void AddKeyValueToMap(const ASL::String& key, const ASL::String& value, PL::NamespaceMetadataValueMap& outMetadataMap, bool inIsOriginal)
	{
		if (!inIsOriginal)
		{
			outMetadataMap[key] = value;
		}
		else
		{
			PL::NamespaceMetadataValueMap::iterator it = outMetadataMap.find(key);
			if (it == outMetadataMap.end())
			{
				outMetadataMap[key] = value;
			}
		}
	}

	void ParseCompositeKeyValue(const ASL::String& key, const ASL::String& value, PL::NamespaceMetadataValueMap& outMetadataMap, bool inIsOriginal)
	{
		ASL::String compositeSymbol = DVA_STR("$^");
		ASL::String::size_type pos = key.find(compositeSymbol);
		if (pos == ASL::String::npos)
		{
			AddKeyValueToMap(HandleWhiteSpaceInKey(key), HandleCommaInValue(value), outMetadataMap, inIsOriginal);
		}
		else
		{
			ASL::String compositeKey = key.substr(pos + compositeSymbol.size());
			ASL::String compositValue = value;
			while(!compositeKey.empty())
			{
				ASL::String tempKey = GetSubStrSeparatedByComma(compositeKey);
				ASL::String tempValue = GetSubStrSeparatedByComma(compositValue);
				AddKeyValueToMap(HandleWhiteSpaceInKey(tempKey), HandleCommaInValue(tempValue), outMetadataMap, inIsOriginal);
			}
		}
	}

	void ParsePlanningMetadataKeyValue(const DOMNode* inNode, PL::NamespaceMetadataPtr& outNamespaceMetadata, bool inIsOriginal)
	{
		const DOMNodeList* children = inNode->getChildNodes();
		const int len = (int)children->getLength();
		for (int i = 0; i < len; ++i)
		{
			const DOMNode* child = children->item(i);
			if (child->getNodeType() == DOMNode::ELEMENT_NODE)
			{
				ASL::String nodeName = ASL::String(child->getNodeName());
				if (nodeName == kTitle)
				{
					const DOMNode* textNode = child->getFirstChild();
					ASL::String nodeValue;
					if (textNode != NULL)
					{
						const XMLCh* childData = textNode->getNodeValue();
						if (childData)
							nodeValue = ASL::String(childData);
					}
					AddKeyValueToMap(nodeName, nodeValue, outNamespaceMetadata->mNamespaceMetadataValueMap, inIsOriginal);
				}
				else if (nodeName == kMeta)
				{
					ASL::String name;
					XMLUtils::GetAttr(child, kName.c_str(), name);
					ASL::String content;
					XMLUtils::GetAttr(child, kContent.c_str(), content);
					ParseCompositeKeyValue(name, content, outNamespaceMetadata->mNamespaceMetadataValueMap, inIsOriginal);
				}
				else if (nodeName == kDescription)
				{
					ASL::String type;
					XMLUtils::GetAttr(child, kType.c_str(), type);
					ASL::String nodeValue;
					const DOMNode* textNode = child->getFirstChild();
					if (textNode != NULL)
					{
						const XMLCh* childData = textNode->getNodeValue();
						if (childData)
							nodeValue = ASL::String(childData);
					}
					type = HandleWhiteSpaceInKey(type);
					AddKeyValueToMap(type, nodeValue, outNamespaceMetadata->mNamespaceMetadataValueMap, inIsOriginal);
				}
			}
		}

	}

	std::uint16_t HexStrToNum(ASL::String& ioString, bool isAscii)
	{
		int step = isAscii ? 2 : 4;
		ASL::String length = ioString.substr(0, step);
		ASL::InputStringStream stream(length);
		std::int32_t lengNum;
		stream >> std::hex >> lengNum;
		ioString = ioString.substr(step);
		return (std::uint16_t)lengNum;
	}

	dvacore::UTF16String decodeAscii(ASL::String& ioString)
	{
		dvacore::UTF8String str;
		while (ioString.size() > 1)
		{
			dvacore::UTF8Char utf8Char = (std::uint8_t)HexStrToNum(ioString, true);
			str += utf8Char;
		}
		return dvacore::utility::UTF8to16(str);
	}

	dvacore::UTF16String decodeUnicode(ASL::String& ioString)
	{
		dvacore::UTF16String str;
		while (ioString.size() > 3)
		{
			dvacore::UTF16Char utf16Char = HexStrToNum(ioString, false);
			str += utf16Char;
		}
		return str;
	}

}//private namespace
	
/**
 * inPath	path of .XMF file
 *
 * suppose inPath = d:\Prelude\PMwithHiRes\Planning\Clip\Office_00001.MXF
 * then baseDir = d:\Prelude\PMwithHiRes\Planning\
 */
bool XMPilotParser::Initialize(const ASL::String& inPath)
{
	ASL::String drive, directory, file, ext;
	ASL::PathUtils::SplitPath(inPath, &drive, &directory, &file, &ext);

	// check the file extension
	if (!dvacore::utility::CaseInsensitive::StringEquals(ext,kMXFExtension))
		return false;

	ASL::String parentDir;
	parentDir = drive + directory;
	if (ASL::PathUtils::HasTrailingSlash(parentDir))
	{
		parentDir = ASL::PathUtils::RemoveTrailingSlash(parentDir);
	}
	ASL::String::size_type lastSlashPos = parentDir.find_last_of(ASL::PathUtils::kPathSeparator);
	if (lastSlashPos == ASL::String::npos)
		return false;
	mBaseDir = parentDir.substr(0, lastSlashPos + 1);

	mMediaProPath = mBaseDir + kMediaProFile; // path of MEDIAPRO.XML
	mClipFileName = DVA_STR("./Clip/") + file + ext;

	if (!FindNRTMetadataForClip())
		return false;

	// get the planning metadata file
	mPlanningFile = DVA_STR("");
	dvacore::filesupport::Dir planningDir(mBaseDir + kGeneralDir + ASL::PathUtils::kPathSeparator + kSonyDir + ASL::PathUtils::kPathSeparator + kPlanningDir);
	if (planningDir.Exists())
	{
		dvacore::filesupport::Dir::FileIterator planningFile = planningDir.BeginFiles(false);
		while (planningFile != planningDir.EndFiles()) // there may be more than one file there
		{
			if (ValidatePlanningFile((*planningFile).FullPath())) {
				mPlanningFile = (*planningFile).FullPath();
				break;
			}

			planningFile++;
		}
	}
	// look for NRT metadata file
	return true;
}

ASL::String XMPilotParser::LoadXmlContent(const ASL::String& inFilePath)
{
	ASL::File xmlFile;
	if(ASL::ResultSucceeded(xmlFile.Create(
		inFilePath,
		ASL::FileAccessFlags::kRead,
		ASL::FileShareModeFlags::kNone,
		ASL::FileCreateDispositionFlags::kOpenExisting,
		ASL::FileAttributesFlags::kAttributeNormal)))
	{
		ASL::UInt64 byteSize = xmlFile.SizeOnDisk();
		if (byteSize < ASL::UInt32(-1))
		{
			ASL::UInt32 readSize = static_cast<ASL::UInt32>(byteSize);
			std::vector<dvacore::UTF8Char> buffer(readSize + 1);
			ASL::UInt32 numberOfBytesRead = 0;
			if (ASL::ResultSucceeded(xmlFile.Read(&buffer[0], readSize, numberOfBytesRead)))
			{
				return ASL::MakeString(&buffer[0]);
			}
		}
		else
		{
			DVA_ASSERT_MSG(0, "Too huge xml file whose size is more than 2^32 bytes.");
		}
	}

	return ASL::String();
}

bool XMPilotParser::ValidatePlanningFile(const ASL::String& inPlanningFile)
{
	ASL::String strContent = LoadXmlContent(inPlanningFile);

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = NULL;

	bool result = XMLUtilities::GetDOMDocument(&parser, strContent, &pDoc);
	if (result)
	{
		DOMElement* rootElement = pDoc->getDocumentElement();
		if (rootElement == NULL || kPlanningMetadata != rootElement->getNodeName())
			return false;

		const DOMNode* node = NULL;

		node = XMLUtils::FindChild(rootElement, kMaterialGroup.c_str());
		if (node == NULL)
			return false;

		node = XMLUtils::FindChild(node, kMaterial.c_str());
		while (true)
		{
			if (node == NULL)
				return false;

			ASL::String umid;
			XMLUtils::GetAttr(node, kUmidRef.c_str(), umid);
			if (umid == mUmid)
				return true;

			node = XMLUtils::FindNextSibling(node, kMaterial.c_str());
		}
	}

	return false;
}

bool XMPilotParser::FindNRTMetadataForClip()
{
	ASL::String strContent = LoadXmlContent(mMediaProPath);

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = NULL;

	bool result = XMLUtilities::GetDOMDocument(&parser, strContent, &pDoc);
	if (!result)
		return false;

	DOMElement* rootElement = pDoc->getDocumentElement();
	const DOMNode* node = NULL;

	// check schema
	if (rootElement == NULL || kMediaProfile != rootElement->getNodeName())
		return false;

	node = XMLUtils::FindChild(rootElement, kContents.c_str());
	if (node == NULL)
		return false;

	node = XMLUtils::FindChild(node, kMaterial.c_str());

	ASL::String uriStr, uriType;
	while (true)
	{
		if (node == NULL)
			return false;

		XMLUtils::GetAttr(node, kUri.c_str(), uriStr);

		if (dvacore::utility::CaseInsensitive::StringEquals(mClipFileName, uriStr))// found <Material> for clip
		{
			XMLUtils::GetAttr(node, kUmid.c_str(), mUmid);
			DVA_ASSERT(!mUmid.empty());
			if (mUmid.empty())
				return false;

			XMLUtils::GetAttr(node, kStatus.c_str(), mStatus);
			DVA_ASSERT(!mStatus.empty());
			if (mStatus.empty())
				mStatus = DVA_STR("none");

			// read RelevantInfo
			node = XMLUtils::FindChild(node, kRelevantInfo.c_str());
			while (true)
			{
				if (node == NULL)
					return false;

				XMLUtils::GetAttr(node, kUriType.c_str(), uriType);
				if (dvacore::utility::CaseInsensitive::StringEquals(kXmlUri, uriType))
				{
					XMLUtils::GetAttr(node, kUri.c_str(), mClipNRTMeta);
					break;
				}

				node = XMLUtils::FindNextSibling(node, kRelevantInfo.c_str());
			}

			break;
		}

		node = XMLUtils::FindNextSibling(node, kMaterial.c_str());
	}

	DVA_ASSERT(!mClipNRTMeta.empty());
	if (mClipNRTMeta.empty())
		return false;

	ASL::String::size_type firstSlashPos = mClipNRTMeta.find_first_of(DVA_STR("./"));
	if (firstSlashPos == 0) {
#if ASL_TARGET_OS_WIN
		boost::algorithm::replace_all(mClipNRTMeta, DVA_STR("/"), DVA_STR("\\"));
#endif
		mClipNRTMeta = mBaseDir + mClipNRTMeta.substr(firstSlashPos + 2, mClipNRTMeta.length() - firstSlashPos - 2);
	}

	return true;
}

bool XMPilotParser::Parse(const ASL::String& inPath)
{
	bool ret = false;
	ret = Initialize(inPath);
	if (!ret) return false;

	if (!mPlanningFile.empty())
	{
		ParsePlanningMetadata(mPlanningFile);
	}

	ret = ParseEssenceMark(mClipNRTMeta);
	if (!ret) return false;

	return true;
}

bool XMPilotParser::ParsePlanningMetadata(const ASL::String& inPlanningFile)
{
	ASL::String strContent = LoadXmlContent(inPlanningFile);

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = NULL;

	bool result = XMLUtilities::GetDOMDocument(&parser, strContent, &pDoc);
	if (!result)
		return false;

	DOMElement* rootElement = pDoc->getDocumentElement();
	const DOMNode* node = NULL;

	if (rootElement == NULL || kPlanningMetadata != rootElement->getNodeName())
		return false;

	mAssignmentMetadata = PL::NamespaceMetadataPtr(new PL::NamespaceMetadata);
	mMemoMetadata = PL::NamespaceMetadataPtr(new PL::NamespaceMetadata);
	mAssignmentMetadata->mNamespaceUri = dvacore::utility::AsciiToUTF16(PL::kXMPilotNamespace_Assign);
	mAssignmentMetadata->mNamespacePrefix = dvacore::utility::AsciiToUTF16(PL::kXMPilotNamespacePrefix_Assign);
	mMemoMetadata->mNamespaceUri = dvacore::utility::AsciiToUTF16(PL::kXMPilotNamespace_Memo);
	mMemoMetadata->mNamespacePrefix = dvacore::utility::AsciiToUTF16(PL::kXMPilotNamespacePrefix_Memo);
	mMemoMetadata->mNamespaceMetadataValueMap[kStatus] = mStatus;
	node = XMLUtils::FindChild(rootElement, kProperties.c_str());
	while (true)
	{
		if (node == NULL)
			break;

		ASL::String propertyId;
		XMLUtils::GetAttr(node, kPropertyId.c_str(), propertyId);
		ASL::String classType;
		XMLUtils::GetAttr(node, kClass.c_str(), classType);
		bool isOriginalClass = false;
		if (classType == kClass_Original)
			isOriginalClass = true;
		else if (classType == kClass_Modified)
			isOriginalClass = false;
		else
			DVA_ASSERT(0);

		if (propertyId == kPropertyId_Assign)
		{
			ParsePlanningMetadataKeyValue(node, mAssignmentMetadata, isOriginalClass);
		}
		else if (propertyId == kPropertyId_Memo)
		{
			ParsePlanningMetadataKeyValue(node, mMemoMetadata, isOriginalClass);
		}

		node = XMLUtils::FindNextSibling(node, kProperties.c_str());
	}

	node = XMLUtils::FindChild(rootElement, kMaterialGroup.c_str());
	if (node == NULL)
		return true;

	node = XMLUtils::FindChild(node, kMaterial.c_str());
	while (true)
	{
		if (node == NULL)
			break;

		ASL::String umid;
		XMLUtils::GetAttr(node, kUmidRef.c_str(), umid);
		if (umid == mUmid)
		{
			ParsePlanningMetadataKeyValue(node, mMemoMetadata, false);
			break;
		}

		node = XMLUtils::FindNextSibling(node, kMaterial.c_str());
	}

	return true;
}

bool XMPilotParser::ParseEssenceMark(const ASL::String& inNRTMetadataFile)
{
	ASL::String strContent = LoadXmlContent(inNRTMetadataFile);

	XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser parser;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDoc = NULL;

	bool result = XMLUtilities::GetDOMDocument(&parser, strContent, &pDoc);
	if (!result)
		return false;

	DOMElement* rootElement = pDoc->getDocumentElement();
	const DOMNode* node = NULL;

	if (rootElement == NULL || !dvacore::utility::CaseInsensitive::StringEquals(kNonRealTimeMeta, ASL::MakeString(rootElement->getNodeName())))
		return false;

	node = XMLUtils::FindChild(rootElement, kKlvPacketTable.c_str());
	if (node == NULL)
		return true;

	node = XMLUtils::FindChild(node, kKlvPacket.c_str());
	while (true)
	{
		if (node == NULL)
			return true;

		ASL::String keyStr;
		XMLUtils::GetAttr(node, kKey.c_str(), keyStr);
		bool encodeAscii = true;
		if (dvacore::utility::CaseInsensitive::StringEquals(keyStr, kEssenceMarkKey_Ascii))
			encodeAscii = true;//ascii
		else if (dvacore::utility::CaseInsensitive::StringEquals(keyStr, kEssenceMarkKey_Unicode))
			encodeAscii = false; //unicode
		else
			DVA_ASSERT(0);

		{
			ASL::String frameCount;
			XMLUtils::GetAttr(node, kFrameCount.c_str(), frameCount);
			ASL::String lengthValue;
			XMLUtils::GetAttr(node, kLengthValue.c_str(), lengthValue);

			ASL::MixHashGuid hash("EssenseMarker");
			hash.MixInValue(frameCount);
			hash.MixInValue(lengthValue);

			std::int64_t frameNum = ASL::Coercion<std::int64_t>::Result(frameCount);

			dvacore::UTF16String name;
			if (lengthValue.size() >= 2)
			{
				std::uint16_t lengNum = HexStrToNum(lengthValue, true);
				DVA_ASSERT(lengNum*2 == lengthValue.size());

				name = encodeAscii ? decodeAscii(lengthValue) : decodeUnicode(lengthValue);
			}
			PL::XMPilotEssenseMarkerPtr essenseMarker(new PL::XMPilotEssenseMarker(hash.ToGuid(), name, frameNum));
			mEssenceMark.push_back(essenseMarker);
		}

		node = XMLUtils::FindNextSibling(node, kKlvPacket.c_str());
	}
	return true;
}

// below are the exported functions
bool ApplyXMPilotEssenseMarker(ASL::StdString& ioXMPString, 
	PL::XMPilotEssenseMarkerList const& inEssenseMarkerList, 
	dvamediatypes::FrameRate const& inMediaFrameRate)
{
	PL::TrackTypes trackTypes;
	PL::MarkerSet markersExist;
	PL::Utilities::BuildMarkersFromXMPString(ioXMPString, trackTypes, markersExist);

	PL::MarkerSet markersToAdd;
	BOOST_FOREACH(const PL::XMPilotEssenseMarkerPtr& essenseMarker, inEssenseMarkerList)
	{
		bool duplicated = false;
		dvamediatypes::TickTime startTime(essenseMarker->mInPointFrame, inMediaFrameRate);
		dvamediatypes::TickTime duration(essenseMarker->mDurationFrame, inMediaFrameRate);
		BOOST_FOREACH(const PL::CottonwoodMarker& existMarker, markersExist)
		{
			//if (essenseMarker->mName == existMarker.GetName()
			//	&& startTime == existMarker.GetStartTime()
			//	&& duration == existMarker.GetDuration()
			//	&& existMarker.GetType() == DVA_STR("Comment"))
			if (essenseMarker->mGuid == existMarker.GetGUID())
			{
				duplicated = true;
				break;
			}
		}
		if (!duplicated)
		{
			PL::CottonwoodMarker marker;
			marker.SetType(DVA_STR("Comment"));
			marker.SetName(essenseMarker->mName);
			marker.SetDuration(duration);
			marker.SetStartTime(startTime);
			marker.SetGUID(essenseMarker->mGuid);
			markersToAdd.insert(marker);
		}
	}

	if (!markersToAdd.empty())
	{
		markersToAdd.insert(markersExist.begin(), markersExist.end());
		PL::Utilities::BuildXMPStringFromMarkers(ioXMPString, markersToAdd, trackTypes, inMediaFrameRate);
		return true;
	}
	return false;
}

const char* kXMPilotNamespace_Assign = "http://xmlns.sony.net/pro/metadata/planningmetadata/assignment/";
const char* kXMPilotNamespace_Memo = "http://xmlns.sony.net/pro/metadata/planningmetadata/memo/";
const char* kXMPilotNamespacePrefix_Assign = "xmpPMAssign:";
const char* kXMPilotNamespacePrefix_Memo = "xmpPMMemo:";

bool ParseXMPilotXmlFile(
	const ASL::String& inPath, 
	PL::XMPilotEssenseMarkerList& outEssenseMarkerList,
	PL::NamespaceMetadataList& outNamespaceList)
{
	XMPilotParser parser;
	if (parser.Parse(inPath))
	{
		outEssenseMarkerList = parser.GetEssenceMark();
		if (parser.GetAssignmentMetadata() != NULL)
			outNamespaceList.push_back(parser.GetAssignmentMetadata());
		if (parser.GetMemoMetadata() != NULL)
			outNamespaceList.push_back(parser.GetMemoMetadata());
		return true;
	}
	return false;
}

bool MergeXMPWithPlanningMetadata(XMPText ioXmp, PL::NamespaceMetadataList const& inNamespaceMetadataList)
{
	SXMPMeta xmpMetaData = SXMPMeta(ioXmp->c_str(), ioXmp->size());
	bool result = false;

	// namespace
	NamespaceMetadataList::const_iterator namespaceIt = inNamespaceMetadataList.begin();
	NamespaceMetadataList::const_iterator namespaceEnd = inNamespaceMetadataList.end();
	for (; namespaceIt != namespaceEnd; ++namespaceIt)
	{
		NamespaceMetadataPtr namespaceMetadata = *namespaceIt;

		if (namespaceMetadata->mNamespaceUri.empty() || namespaceMetadata->mNamespacePrefix.empty())
			continue;
		ASL::StdString namespaceURI = ASL::MakeStdString(namespaceMetadata->mNamespaceUri);
		ASL::StdString namespacePrefix = ASL::MakeStdString(namespaceMetadata->mNamespacePrefix);

		ASL::StdString registerPrefix;
		SXMPMeta::RegisterNamespace(namespaceURI.c_str(), namespacePrefix.c_str(), &registerPrefix);

		NamespaceMetadataValueMap::const_iterator mapIt = namespaceMetadata->mNamespaceMetadataValueMap.begin();
		NamespaceMetadataValueMap::const_iterator mapEnd = namespaceMetadata->mNamespaceMetadataValueMap.end();
		for (; mapIt != mapEnd; ++mapIt)
		{
			ASL::StdString propName = ASL::MakeStdString(mapIt->first);
			ASL::StdString propValue = ASL::MakeStdString(mapIt->second);

			if (!propValue.empty() && !xmpMetaData.DoesPropertyExist(namespaceURI.c_str(), propName.c_str()))
			{
				xmpMetaData.SetProperty(namespaceURI.c_str(), propName.c_str(), propValue.c_str());
				result = true;
			}
		}
	}

	if (result)
		xmpMetaData.SerializeToBuffer(ioXmp.get(), kXMP_OmitPacketWrapper);
	return result;
}

}



