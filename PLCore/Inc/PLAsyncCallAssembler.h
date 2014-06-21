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

#ifndef PLASYNCCALLASSEMBLER_H
#define PLASYNCCALLASSEMBLER_H

// MZ
#ifndef MZIMPORTFAILURE_H
#include "MZImportFailure.h"
#endif

#ifndef PLLIBRARYSUPPORT_H
#include "PLLibrarySupport.h"
#endif

// ASL
#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLRESULT_H
#include "ASLResult.h"
#endif

// BE
#ifndef BEMASTERCLIP_H
#include "BEMasterClip.h"
#endif

namespace PL
{
namespace SRAsyncCallAssembler
{

typedef ASL::Result (*ImportXMPImplFn)(
				const ASL::String& inFilePath,
				MZ::ImportResultVector& outFilesFailure,
				PL::CustomMetadata& outCustomMetadata,
				XMPText outXMPBuffer);
				
typedef void (*RequestThumbnailImplFn)(
				ASL::String const& inFilePath,
				dvamediatypes::TickTime const& inClipTime,
				SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn);
				
void Initialize();

void Terminate();

bool NeedImportXMPInMainThread(const ASL::String& inPath);

ASL::Result ImportXMPInMainThread(
				const ASL::String& inFilePath,
				MZ::ImportResultVector& outFilesFailure,
				PL::CustomMetadata& outCustomMetadata,
				XMPText outXMPBuffer,
				ImportXMPImplFn inImportXMPImplFn);
				
bool NeedRequestThumbnailInMainThread(const ASL::String& inPath);

void RequestThumbnailInMainThread(
				const ASL::String& inFilePath,
				const dvamediatypes::TickTime& inClipTime,
				RequestThumbnailImplFn inRequestThumbnailImplFn,
				SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn);

}
}
#endif
