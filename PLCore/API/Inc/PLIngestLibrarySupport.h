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

#ifndef PLINGESTLIBRARYSUPPORT_H
#define PLINGESTLIBRARYSUPPORT_H


#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLMESSAGESTRUCT_H
#include "PLMessageStruct.h"
#endif

#ifndef AME_IPRESET_H
#include "IPreset.h"
#endif

namespace PL
{
	namespace IngestLibrarySupport
	{
		/**
		**	Used by transcode API b/w clients and core.
		*/
		PL_EXPORT
		void TranscodeRequest(
					const ASL::Guid& inTaskID, 
					const EncoderHost::IPresetRef& inPreset,
					const ASL::String& inOutputDirectory,
					const PL::TranscodeItemList& inTranscodeItemList);

		/**
		**	Used by concatenation API b/w clients and core.
		*/
		PL_EXPORT
		void ConcatenationRequest(
			const ASL::Guid& inTaskID, 
			const EncoderHost::IPresetRef& inPreset,
			const ASL::String& inOutputDirectory,
			const ASL::String& inOutputFileName,
			const PL::ConcatenationItemList& inConcatenatnionItemList);

		/**
		**	Cancel transcode task by TaskID
		*/
		PL_EXPORT
		void CancelTranscodeAsync(const ASL::Guid& inTaskID);

		/**
		**	Cancel concatenationm task by TaskID
		*/
		PL_EXPORT
		void CancelConcatenationAsync(const ASL::Guid& inTaskID);
	}
}


#endif
