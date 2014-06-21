/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2008 Adobe Systems Incorporated		               */
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

#ifndef PLXMPILOTUTILS_H
#define PLXMPILOTUTILS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef PLINGESTTYPES_H
#include "IngestMedia/PLIngestTypes.h"
#endif

#ifndef DVAMEDIATYPES_FRAMERATE_H
#include "dvamediatypes/FrameRate.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

namespace PL 
{
	// XMPilot is a new format from Sony. For more details, you can refer to wiki 
	// https://zerowing.corp.adobe.com/display/Prelude/XMPilot+Investigation.
	// This file describes the exported functions and data structures for XMPilot.

	typedef std::set<ASL::String> MediaPathSet;

	struct XMPilotEssenseMarker
	{
		ASL::Guid mGuid;
		ASL::String mName;
		std::int64_t mInPointFrame;
		std::int64_t mDurationFrame;
		XMPilotEssenseMarker(ASL::Guid const& inGuid, ASL::String const& inName, std::int64_t inInPoint, std::int64_t inDuration = 0)
			: mGuid(inGuid), mName(inName), mInPointFrame(inInPoint), mDurationFrame(inDuration) {};
	};
	typedef boost::shared_ptr<XMPilotEssenseMarker> XMPilotEssenseMarkerPtr;
	typedef std::list<XMPilotEssenseMarkerPtr> XMPilotEssenseMarkerList;

	PL_EXPORT
	extern const char* kXMPilotNamespace_Assign;
	PL_EXPORT
	extern const char* kXMPilotNamespacePrefix_Assign;
	PL_EXPORT
	extern const char* kXMPilotNamespace_Memo;
	PL_EXPORT
	extern const char* kXMPilotNamespacePrefix_Memo;

	PL_EXPORT
	bool ParseXMPilotXmlFile(
		const ASL::String& inPath, 
		PL::XMPilotEssenseMarkerList& outEssenseMarkerList,
		PL::NamespaceMetadataList& outNamespaceList);

	PL_EXPORT
	bool ApplyXMPilotEssenseMarker(
		ASL::StdString& ioXMPString, 
		PL::XMPilotEssenseMarkerList const& inEssenseMarkerList,
		dvamediatypes::FrameRate const& inMediaFrameRate);

	PL_EXPORT
	bool MergeXMPWithPlanningMetadata(XMPText ioXmp, PL::NamespaceMetadataList const& inNamespaceMetadataList);

} // namespace PLXMPILOTUTILS_H

#endif

