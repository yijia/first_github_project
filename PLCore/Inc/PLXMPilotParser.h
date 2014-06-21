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

#pragma once

#ifndef PLXMPILOTPARSER_H
#define PLXMPILOTPARSER_H

#ifndef PLXMPILOTUTILS_H
#include "PLXMPilotUtils.h"
#endif

namespace PL
{

	class XMPilotParser
	{
	public:
		bool Parse(const ASL::String& inPath);

		PL::NamespaceMetadataPtr GetAssignmentMetadata() { return mAssignmentMetadata; }
		PL::NamespaceMetadataPtr GetMemoMetadata() { return mMemoMetadata; }
		PL::XMPilotEssenseMarkerList GetEssenceMark() { return mEssenceMark; }

	private:
		bool Initialize(const ASL::String& inPath);

		bool ParsePlanningMetadata(const ASL::String& inPlanningFile);
		bool FindNRTMetadataForClip();
		bool ParseEssenceMark(const ASL::String& inNRTMetadataFile);

		bool ValidatePlanningFile(const ASL::String& inPlanningFile);

		ASL::String LoadXmlContent(const ASL::String& inFilePath);

		ASL::String mBaseDir; // base planning dir
		ASL::String mClipFileName;
		ASL::String mClipNRTMeta; // path to NonRealTimeMeta xml of clip
		ASL::String mMediaProPath; // path of MEDIAPRO.XML
		ASL::String mPlanningFile; // path to General\Sony\Planning\xxx.xml

		ASL::String mUmid;
		ASL::String mStatus;
		ASL::String mPath;

		PL::NamespaceMetadataPtr mAssignmentMetadata;
		PL::NamespaceMetadataPtr mMemoMetadata;
		PL::XMPilotEssenseMarkerList mEssenceMark;
	};

}

#endif
