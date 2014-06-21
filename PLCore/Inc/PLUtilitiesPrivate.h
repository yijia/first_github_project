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

#ifndef PLUTILS_H
#define PLUTILS_H

// UIF
#ifndef UIFMESSAGEBOX_H
#include "UIFMessageBox.h"
#endif

// BE
#ifndef BEMASTERCLIP_H
#include "BEMasterClip.h"
#endif

// DVA
#ifndef DVAMEDIATYPES_TIMEDISPLAY_H
#include "dvamediatypes/TimeDisplay.h"
#endif
#ifndef BE_PROJECT_IPROJECTITEMFWD_H
#include "BE/Project/IProjectItemFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTFWD_H
#include "BE/Project/IProjectFwd.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef PLROUGHCUT_H
#include "PLRoughCut.h"
#endif

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef DVAUI_UI_MESSAGEBOX_H
#include "dvaui/dialogs/UI_MessageBox.h"
#endif

namespace PL
{
	namespace SRUtilitiesPrivate
	{
		extern const char* kDiscardButtonText;

		UIF::MBResult::Type PromptForSave(
			const ASL::String& inWarningCaption, 
			const ASL::String& inWarningMessage);

		UIF::MBResult::Type WarnReadOnly(
			const ASL::String& inWarningCaption, 
			const ASL::String& inWarningMessage);

		UIF::MBResult::Type PromptForError(
			const ASL::String& inErrorCaption, 
			const ASL::String& inErrorMessage);

		bool AskSaveFilePath(	
			const ASL::String& inCaption,
			ASL::String& ioFullPath,
			const ASL::String& inExtension,
			const ASL::String& inFileFilter);

		dvamediatypes::TimeDisplay GetTimeDisplayForFrameRate(
			const dvamediatypes::FrameRate& inFrameRate,
			const bool inDropFrame);
		void OpenSequenceAndWrapOpenProjectItem(
			BE::ISequenceRef inSequence,
			BE::IProjectRef inProject,
			BE::IProjectItemRef inProjectItem,
			ASL::Guid inProjectViewID,
			bool inControlKey);

		/**
		** Get filename of given file path (without extension)
		*/
		dvacore::UTF16String GetFileNamePart(dvacore::UTF16String filePath, bool withExtension);

		/**
		*	Check if the name is already used as mName in one of the CottonwoodSubclips in the given list
		*	If so, the name with "_#" (# denotes the next "free" number) is returned, else it is returned unchanged
		*/
		dvacore::UTF16String MakeNameUnique(ASL::String& inName, SRClipItems subClips);

		/*
		**
		*/
		dvamediatypes::FrameRate GetTrackItemFrameRate(BE::ITrackItemRef aTrackItem);

		/*
		**
		*/
		dvacore::UTF16String GetMediaPathFromMasterClip(const BE::IMasterClipRef inMasterClip);

		/*
		**
		*/
		void GetMediaFromTrackItem(const BE::ITrackItemRef inTrackItem,
										BE::IMasterClipRef& outMasterClipRef,
										BE::IMediaRef& outMedia,
										BE::IClipRef& outClip);


		////////////////////////////////////following are new APIs for SilkRoad Begin/////////////////////////////////////////////////////

		/*
		** 
		*/
		void RemoveSequenceProjectItem(
			BE::IProjectRef inProject, 
			BE::IProjectItemRef inSequenceItem);

		/*
		** 
		*/
		bool RemoveSequence(const PL::ISRPrimaryClipPlaybackRef inPrimaryClipPlayback);

		/*
		** if disableDialog true, error msg will go to event panel.
		*/
		bool PreCheckBeforeSendSaveMsg(
			const ASL::String& inClipURL,
			AssetMediaInfoPtr const& newAssetMediaInfo,
			bool& outDiscardChanges,
			bool disableDialog = false);

		dvaui::dialogs::UI_MessageBox::Button SimpleMessageBoxWithCancel(
			const dvacore::UTF16String& inDialogTitle,
			const dvacore::UTF16String& inMessage,
			const dvacore::UTF16String& inOkText,
			const dvaui::dialogs::UI_MessageBox::Icon inIcon,
			const dvaui::dialogs::UI_MessageBox::Button inDefaultButton,
			const dvacore::UTF16String& inCancelText = dvacore::UTF16String());

		ASL::String ConvertMarkerTagsToString(const TagParamMap inTagParamMap, const ASL::String& inSeparatorStr = ASL_STR("\n"));
	}
}

#endif
