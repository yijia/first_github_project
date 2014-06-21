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

#ifndef PLROUGHCUTUTILS_H
#define PLROUGHCUTUTILS_H
#pragma once

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef DVACORE_CONFIG_UNICODETYPES_H
#include "dvacore/config/UnicodeTypes.h"
#endif

#ifndef DVAMEDIATYPES_FRAMERATE_H
#include "dvamediatypes/FrameRate.h"
#endif

#ifndef BESEQUENCEFWD_H
#include "BESequenceFwd.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

namespace PL
{

	PL_EXPORT
	ASL::Result CreateEmptyRoughCutFile(const ASL::String& inFilePath);

	PL_EXPORT
	ASL::String CreateEmptyRoughCutContent(const ASL::String& inName);

	PL_EXPORT
	bool IsValidRoughCutFile(
		const dvacore::UTF16String& inFullFileName);

	PL_EXPORT
	bool IsRoughCutFileExtension(
		const dvacore::UTF16String& inFullFileName);

	PL_EXPORT
	ASL::Result SaveRoughCut(
		const ASL::String& inFilePath,
		const ASL::String& inContent);

	PL_EXPORT
	void FCPRateToTimebaseAndNtsc(
		ASL::String& outTimebase,
		ASL::String& outIsNtsc,
		dvamediatypes::FrameRate inFCPRate = dvamediatypes::FrameRate());

	PL_EXPORT
	dvamediatypes::FrameRate TimebaseAndNtscToFCPRate(
		const ASL::String& inTimebase,
		const ASL::String& inIsNtsc);

	PL_EXPORT
	ASL::Result CreateEmptyFile(const ASL::String& inPath);

	PL_EXPORT
	ASL::String BuildRoughCutContent(
		const BE::ISequenceRef& inSequence,
		const PL::SRClipItems& inSRClipItems,
		const ASL::String& inName);

	PL_EXPORT
	ASL::String BuildRoughCutContent(
		AssetMediaInfoPtr const& inAssetMediaInfo, 
		AssetItemPtr const& inAssetItem);

	PL_EXPORT 
	ASL::Result LoadRoughCut(
		const ASL::String& inFilePath,
		const ASL::Guid& inRoughCutID,
		PL::AssetMediaInfoPtr& outAssetMediaInfo,
		PL::AssetItemPtr& outAssetItem);

	PL_EXPORT
	ASL::String LoadRoughCut(const ASL::String& inFilePath);

} // namespace PL

#endif // PLROUGHCUTUTILS_H
