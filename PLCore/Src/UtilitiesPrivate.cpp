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

//	XMP
#include "IncludeXMP.h"

//	SR
#include "PLUtilitiesPrivate.h"
#include "PLProject.h"
#include "PLModulePicker.h"

//	MZ
#include "MZMediaParams.h"
#include "MZAction.h"
#include "MZProject.h"
#include "MZActivation.h"
#include "MZSequence.h"
#include "MZProjectActions.h"
#include "MZProjectItemHelpers.h"
#include "PLUtilities.h"
#include "MZUtilities.h"
#include "PLWriteXMPToDiskCache.h"
#include "PLLibrarySupport.h"
#include "MZBEUtilities.h"

//	ASL
#include "ASLPathUtils.h"
#include "ASLClassFactory.h"

//	BE
#include "BEClip.h"
#include "BEMedia.h"
#include "BETrack.h"
#include "BE/Project/IClipProjectItem.h"
#include "BE/Content/IMediaContent.h"
#include "BE/Project/IProject.h"
#include "BE/Project/IProjectItemSelection.h"

//	ML
#include "MLMedia.h"
#include "ImporterHost.h"

//	DVA
#include "dvacore/config/Localizer.h"
#include "dvaui/ui/OS_UI.h"
#include "dvaui/dialogs/OS_NavigationDialog.h"
#include "dvaui/dialogs/OS_FileDialog.h"
#include "dvaui/dialogs/OS_FileSaveAsDialog.h"
#include "dvacore/utility/StringUtils.h"
#include "dvaworkspace/workspace/WorkspaceUtils.h"

//  UIF
#include "UIFPlayerControl.h"
#include "UIFPlatformTypes.h"
#include "UIFApplication.h"
#include "UIFDocumentManager.h"
#if ASL_TARGET_OS_WIN
#include "Win/UIFWinParentAndChildWindowDisabler.h"
#endif

#include "MLMetadataManager.h"

namespace PL
{
	namespace SRUtilitiesPrivate
	{
		const char* kDiscardButtonText = "$$$/Prelude/Mezzanine/SRLoggingClip/SaveReadOnlyOKText=Discard";

		/*
		**
		*/
		bool MediaFileXMPLastModTimeNotEqual(ASL::String const& inMediaPath)
		{
			XMP_DateTime	currentXMPModTime;
			XMP_FileFormat format = ML::MetadataManager::GetXMPFormat(inMediaPath);
			try
			{
				SXMPFiles::GetFileModDate(ASL::MakeStdString(inMediaPath).c_str(), &currentXMPModTime, &format, kXMPFiles_ForceGivenHandler);
			}
			catch(...)
			{
				return true;
			}

			DVA_ASSERT(SRProject::GetInstance()->GetAssetMediaInfoWrapper(inMediaPath));
		
			return !Utilities::IsDateTimeEqual(SRProject::GetInstance()->GetAssetMediaInfoWrapper(inMediaPath)->GetXMPLastModTime(), currentXMPModTime);
		}


		UIF::MBResult::Type PromptForSave(
			const ASL::String& inWarningCaption, 
			const ASL::String& inWarningMessage)
		{
			return UIF::MessageBox(
				inWarningMessage, 
				inWarningCaption, 
				UIF::MBFlag::kMBFlagYesNoCancel, 
				UIF::MBIcon::kMBIconNone,
				UIF::MBResult::kMBResultYes);
		}

		UIF::MBResult::Type WarnReadOnly(
			const ASL::String& inWarningCaption, 
			const ASL::String& inWarningMessage)
		{
			return UIF::MessageBox(
				inWarningMessage, 
				inWarningCaption, 
				UIF::MBFlag::kMBFlagOKCancel, 
				UIF::MBIcon::kMBIconWarning,
				UIF::MBResult::kMBResultCancel);
		}

		UIF::MBResult::Type PromptForError(
			const ASL::String& inErrorCaption, 
			const ASL::String& inErrorMessage)
		{
			return UIF::MessageBox(
				inErrorMessage, 
				inErrorCaption, 
				UIF::MBFlag::kMBFlagOK, 
				UIF::MBIcon::kMBIconError);
		}

		static dvaui::dialogs::FileDialogFilterInfo filterInfoFromExtensionAndWinFilter(
			const ASL::String& inExtension,
			const ASL::String& inWinFilterStr)
		{
			dvaui::dialogs::FileDialogFilterInfo result;

			result.extension = inExtension;
			ASL::String::size_type location = result.extension.find_first_of(ASL_STR("."));

			if (location != ASL::String::npos)
			{
				// Eliminate the '.' if there is one.
				result.extension = result.extension.substr(location + 1);
			}

			// the fileFilter is declared in a windows-specific fashion and the projectConverter API filter methods were
			// designed assuming windows. New dvaui::dialogs::OS_FileOpenDialog requires spliting the 'inWinFilterStr' apart now
			// for both platform.  The format for the windows dialog is to have the plain text description followed by a null, followed
			// by the filter, followed by 2 nulls.
			ASL::String::size_type nullPos(0);
			while (nullPos < inWinFilterStr.length() && inWinFilterStr[nullPos] != '\0')
			{
				++nullPos;
			}

			result.description = inWinFilterStr.substr(0, nullPos);

			// pass pointer to next character after nullPos so the string will stop at the first NULL character.
			// Using substr would get the trailing null char that is INSIDE inWinFilterStr also placed in 'filter' (bad!)
			ASL::String extensions(&(inWinFilterStr[nullPos+1]));

			// remove all occurences of "*." from the string
			dvacore::utility::ReplaceAllStringInString(extensions, ASL_STR("*."), ASL_STR(""));

			result.filter = extensions;

			return result;
		}

		bool AskSaveFilePath(	
			const ASL::String& inCaption,
			ASL::String& ioFullPath,
			const ASL::String& inExtension,
			const ASL::String& inFileFilter)
		{
			bool userHitOK = false;

			// Force any and all playback to stop.
			UIF::PlayerControl::StopAllPlayers();

			UIF::PlatformWindowPtr theOwner = UIF::Application::Instance()->GetMainFrame();

			// Kill input capture
			dvaui::ui::UI_NodeManager* nodeManager = dvaui::ui::UI_NodeManager::Get();
			if (nodeManager != NULL)
			{
				nodeManager->EndInputCapture(false);
			}
#if ASL_TARGET_OS_WIN
			// Must strip UNC , or it will become very slow on xp
			ioFullPath = ASL::PathUtils::Win::StripUNC(ASL::PathUtils::ToNormalizedPath(ioFullPath));
#endif

			// Convert the flags from Premiere flags to DVA flags
			dvaui::dialogs::OS_NavigationDialog::NavigationDialogFlags dvaFlags = 0;
			dvaFlags.Set(dvaui::dialogs::OS_NavigationDialog::NavigationDialogFlag_WarnOnOverwrite, true);
			dvaFlags.Set(dvaui::dialogs::OS_NavigationDialog::NavigationDialogFlag_CenterInParent, true);
//			dvaFlags.Set(dvaui::dialogs::OS_NavigationDialog::NavigationDialogFlag_PreserveSaveFileExtension, true);

			// Add extension fiters (all files without this extension hidden)
			dvaui::dialogs::FileDialogFilterInfoVec filterVec;
			dvaui::dialogs::FileDialogFilterInfo filterInfo(filterInfoFromExtensionAndWinFilter(inExtension, inFileFilter));
			filterVec.push_back(filterInfo);

			dvaui::dialogs::OS_FileSaveAsDialog fileSaveAsDialog(
				dvaui::ui::OS_Window::UI_GetWindow(theOwner), 
				filterVec, 
				0, 
				ASL::PathUtils::GetFullFilePart(ioFullPath), 
				dvaFlags);

			fileSaveAsDialog.SetWindowTitle(inCaption);

			if (!ioFullPath.empty())
			{
				fileSaveAsDialog.SetDefaultDirectory(ASL::PathUtils::GetDrivePart(ioFullPath) + ASL::PathUtils::GetDirectoryPart(ioFullPath));
			}

			// Make the dialog modal and save and restore the focused TabView
#if ASL_TARGET_OS_WIN
			UIF::Windows::ParentAndChildWindowDisabler windowDisabler(theOwner);
#endif

			//	Show the dialog
			fileSaveAsDialog.RunModal();

			// Set ioPath to the path chosen by the user if the user hit OK
			if (fileSaveAsDialog.GetResponse() == dvaui::dialogs::OS_NavigationDialog::responseOK)
			{
				ioFullPath = fileSaveAsDialog.GetFile().FullPath();
				userHitOK = true;
			}

			return userHitOK;
		}

		dvamediatypes::TimeDisplay GetTimeDisplayForFrameRate(
			const dvamediatypes::FrameRate& inFrameRate,
			const bool inDropFrame)
		{
			dvamediatypes::TimeDisplay display(dvamediatypes::kTimeDisplay_2997DropTimecode);
			if (inFrameRate == dvamediatypes::kFrameRate_23976)
			{
				display = dvamediatypes::kTimeDisplay_23976Timecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_Film24)
			{
				display = dvamediatypes::kTimeDisplay_24Timecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoPAL)
			{
				display = dvamediatypes::kTimeDisplay_25Timecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoNTSC && inDropFrame)
			{
				display = dvamediatypes::kTimeDisplay_2997DropTimecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoNTSC && !inDropFrame)
			{
				display = dvamediatypes::kTimeDisplay_2997NonDropTimecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_Video30)
			{
				display = dvamediatypes::kTimeDisplay_30Timecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoPAL_HD)
			{
				display = dvamediatypes::kTimeDisplay_50Timecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoNTSC_HD && inDropFrame)
			{
				display = dvamediatypes::kTimeDisplay_5994DropTimecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_VideoNTSC_HD && !inDropFrame)
			{
				display = dvamediatypes::kTimeDisplay_5994NonDropTimecode;
			}
			else if (inFrameRate == dvamediatypes::kFrameRate_Video60)
			{
				display = dvamediatypes::kTimeDisplay_60Timecode;
			}

			return display;
		}

		/*
		**
		*/
		void OpenSequenceAndWrapOpenProjectItem(
			BE::ISequenceRef inSequence,
			BE::IProjectRef inProject,
			BE::IProjectItemRef inProjectItem,
			ASL::Guid inProjectViewID,
			bool inControlKey)
		{
			MZ::OpenSequence(inSequence, false);
			return MZ::OpenProjectItem(inProject, inProjectItem, inProjectViewID, inControlKey);
		}

		/**
		** Get filename of given file path (without extension)
		*/
		dvacore::UTF16String GetFileNamePart(dvacore::UTF16String filePath, bool withExtension)
		{
			dvacore::UTF16String fileName = ASL::PathUtils::GetFullFilePart(filePath);

			if (withExtension == false)
			{
				dvacore::UTF16String extension = ASL::PathUtils::GetExtensionPart(filePath);

				fileName = fileName.substr(0, fileName.length() - extension.length());
			}

			return fileName;
		}

		/*
		**
		*/
		ASL::String MakeNameUnique(ASL::String& inName, SRClipItems subClips)
		{
			ASL::String newName = inName;
			std::int64_t maxNum = -1;

			SRClipItems::iterator iter = subClips.begin();
			SRClipItems::iterator end = subClips.end();
			bool foundSame = false;

			for (; iter != end; ++iter)
			{
				dvacore::UTF16String::size_type pos = (*iter)->GetSubClipName().find(inName);
				if (pos == 0)
				{
					std::int64_t num = -1;
					ASL::String endString = (*iter)->GetSubClipName().substr(inName.length());
					if (endString.length() > 0)
					{
						if (endString[0] == '_')
						{
							// Found "already numbered" version => Check number
							endString = endString.substr(1);
							dvacore::utility::StringToNumber(num, endString);
						}
					}
					else
					{
						// Exact match:
						foundSame = true;
						num = 0;
					}

					if (num > maxNum)
					{
						maxNum = num;
					}
				}
			}

			if (maxNum >= 0 && foundSame)
			{
				newName = inName + ASL::MakeString("_") + dvacore::utility::NumberToString(maxNum + 1);
			}

			return newName;
		}



		/*
		**
		*/
		void GetMediaFromTrackItem(const BE::ITrackItemRef inTrackItem,
				BE::IMasterClipRef& outMasterClipRef,
				BE::IMediaRef& outMedia,
				BE::IClipRef& outClip)
		{
			if (NULL != inTrackItem)
			{
				BE::IClipTrackItemRef clipTrackItem(inTrackItem);
				BE::IChildClipRef subClip(clipTrackItem->GetChildClip());
				outClip = subClip->GetClip();
				outMasterClipRef = subClip->GetMasterClip();
				outMedia = BE::MasterClipUtils::GetDeprecatedMedia(outMasterClipRef);
			}
		}

		////////////////////////////////////following are new APIs for SilkRoad Begin/////////////////////////////////////////////////////	

		/*
		**
		*/
		void RemoveSequenceProjectItem(
			BE::IProjectRef inProject, 
			BE::IProjectItemRef inSequenceItem)
		{
			if (inSequenceItem != NULL && inProject != NULL)
			{
				// Remove this sequence from project
				BE::IProjectItemSelectionRef itemSelection(
					ASL::CreateClassInstanceRef(BE::kProjectItemSelectionClassID));
				itemSelection->AddItem(inSequenceItem);
				BE::IActionRef action = inProject->CreateDeleteItemsAction(itemSelection, false);
				MZ::ExecuteActionWithoutUndo(BE::IExecutorRef(inProject), action, true );
			}
		}

		/*
		**
		*/
		bool PreCheckBeforeSendSaveMsg(
			const ASL::String& inClipURL,
			AssetMediaInfoPtr const& newAssetMediaInfo,
			bool& outDiscardChanges,
			bool disableDialog)
		{
			outDiscardChanges = false;
			if (MZ::Utilities::IsEAMediaPath(inClipURL))
			{
				return true;
			}

			ASL::String filePath = inClipURL;

			bool result(true);
			// check off-line 
			const dvacore::UTF16String tipsForNextOptions = dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/SaveFailedNextOptionsTips=All new changes will be discarded.");
			AssetMediaInfoWrapperPtr mediaInfoPtr = SRProject::GetInstance()->GetAssetMediaInfoWrapper(filePath);
			if ((NULL != mediaInfoPtr && mediaInfoPtr->IsMediaOffline()) || 
				!ASL::PathUtils::ExistsOnDisk(filePath))
			{
				if (!disableDialog)
				{
					const ASL::String& warningCaption = 
						dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/WarnAssetOfflineCaption=Media Off-line");
					const dvacore::UTF16String& errorMsg = dvacore::ZString(
						"$$$/Prelude/Mezzanine/SRLoggingClip/WarnAssetOffline=The media you are attempting to save is off-line.");
					UIF::MessageBox(
						errorMsg + DVA_STR("\n") + tipsForNextOptions,
						warningCaption,
						UIF::MBIcon::kMBIconError);
					outDiscardChanges = true;
				}

				// post error event
				const dvacore::UTF16String& errorMsg = dvacore::ZString(
					"$$$/Prelude/Mezzanine/SRLoggingClip/WarnAssetOfflineStatic=The file @0 cannot be saved, because it is off-line.", ASL::PathUtils::GetFullFilePart(filePath.c_str()));
				ML::SDKErrors::SetSDKErrorString(errorMsg);

				result = false;
			}
			//	Only needed if XMP has been saved into media file
			else if(newAssetMediaInfo->GetNeedSavePreCheck())
			{
				DVA_ASSERT(ASL::PathUtils::ExistsOnDisk(filePath));

				////	check Read-only
				bool xmpPathWritable = PL::Utilities::IsXMPWritable(filePath);

				if (!xmpPathWritable)
				{
					ASL::String xmpPath = Utilities::GetXMPPath(filePath);
					if (xmpPath.empty())
					{
						xmpPath = filePath;
					}

					const ASL::String caption = dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/SaveReadOnly=Save File failed");
					const dvacore::UTF16String content = dvacore::ZString(
						"$$$/Prelude/Mezzanine/SRLoggingClip/AccessIsDenied=The file @0 cannot be saved, because it is READ ONLY.",
						ASL::PathUtils::GetFullFilePart(xmpPath.c_str()));
					if (!disableDialog)
					{
						UIF::MessageBox(
							content + DVA_STR("\n") + tipsForNextOptions,
							caption,
							UIF::MBIcon::kMBIconError);
						outDiscardChanges = true;
					}
					
					ML::SDKErrors::SetSDKErrorString(content);
					
					result = false;
				}
				//	check xmp modtime
				//	If there is pending xmp write, we don't need to check xmp mod time here
				//	Performance optimization.
				else if(!WriteXMPToDiskCache::GetInstance()->ExistCache(filePath) && MediaFileXMPLastModTimeNotEqual(filePath)) 
				{
					XMPText xmpTest(new ASL::StdString());
					ASL::String outErrorInfo;
					if (ASL::ResultSucceeded(SRLibrarySupport::ReadXMPFromFile(filePath, xmpTest, outErrorInfo)))
					{
						//	If the "Stem" XMP data is exacly same as cached one, we see it as match.
						if (!Utilities::IsXMPStemEqual(xmpTest, mediaInfoPtr->GetAssetMediaInfo()->GetXMPString()))
						{
							if (!disableDialog)
							{
								const ASL::String caption = dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/SaveNoEuqalXMPModTime=Save File");
								const dvacore::UTF16String content = 
									dvacore::utility::ReplaceInString(dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/XMPLastModTimeNotEqual=The file @0's XMP data has been changed by another application. Do you want to do overwrite?"), ASL::PathUtils::GetFullFilePart(filePath.c_str()));
								result  = 
									(UIF::MessageBox(content, caption, UIF::MBFlag::kMBFlagOKCancel, UIF::MBIcon::kMBIconAsterisk) == UIF::MBResult::kMBResultOK);
							}
							else
							{
								const dvacore::UTF16String content = 
									dvacore::utility::ReplaceInString(dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/XMPLastModTimeNotEqualStatic=The file @0 cannot be saved because it's XMP data has been changed by another application."), ASL::PathUtils::GetFullFilePart(filePath.c_str()));
								ML::SDKErrors::SetSDKErrorString(content);
								result = false;
							}
						}
					}
				}

				// Auto-save shold not warning for possible failure, instead of it, when we receive save failures with space limit, error will be reported.
				if (!ModulePicker::IsAutoSaveEnabled() && result && !Utilities::EnoughDiskSpaceForUpdatingXMP(filePath, newAssetMediaInfo->GetXMPString()->length()))
				{
					if (!disableDialog)
					{
						const ASL::String caption = dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/SaveDiskSpaceNotEnough=Save File");
						const dvacore::UTF16String content = 
							dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/DiskSpaceNotEnough=Not enough available disk space for this update. Do you want to continue?");
						result  = 
							(UIF::MessageBox(content, caption, UIF::MBFlag::kMBFlagOKCancel, UIF::MBIcon::kMBIconAsterisk) == UIF::MBResult::kMBResultOK);
					}
					else
					{
						const dvacore::UTF16String content = 
							dvacore::ZString("$$$/Prelude/Mezzanine/SRLoggingClip/DiskSpaceNotEnoughStatic=Not enough available disk space to save file @0.", ASL::PathUtils::GetFullFilePart(filePath.c_str()));
						ML::SDKErrors::SetSDKErrorString(content);
						result = false;
					}
					
				}
			}

			return result;
		}

		/*
		**
		*/
		bool RemoveSequence(const ISRPrimaryClipPlaybackRef inPrimaryClipPlayback)
		{
			bool result = true;
			BE::IProjectRef project = MZ::GetProject();
			if ( inPrimaryClipPlayback != NULL)
			{
				BE::ISequenceRef sequence = inPrimaryClipPlayback->GetBESequence();
				BE::IProjectItemRef sequenceItem = 
					MZ::ProjectItemHelpers::GetProjectItemForSequence(
						BE::IProjectItemContainerRef(project->GetRootProjectItem()), 
						sequence);

				RemoveSequenceProjectItem(project, sequenceItem);
			}

			return result;
		}

		/*
		**
		*/
		dvaui::dialogs::UI_MessageBox::Button SimpleMessageBoxWithCancel(
			const dvacore::UTF16String& inDialogTitle,
			const dvacore::UTF16String& inMessage,
			const dvacore::UTF16String& inOkText,
			const dvaui::dialogs::UI_MessageBox::Icon inIcon,
			const dvaui::dialogs::UI_MessageBox::Button inDefaultButton,
			const dvacore::UTF16String& inCancelText)
		{
			dvaworkspace::workspace::TopLevelWindow* window = dvaworkspace::workspace::WorkspaceUtils::FindMainTopLevelWindow();
			DVA_ASSERT(window);
			if (!window)
			{
				return dvaui::dialogs::UI_MessageBox::kButton_Cancel;
			}

			dvaworkspace::workspace::Workspace* workspace = window->GetWorkspace();

			DVA_ASSERT_MSG(workspace, "Workspace cannot be found. Unable to show message box.");
			if (!workspace)
			{
				return dvaui::dialogs::UI_MessageBox::kButton_Cancel;
			}

			dvacore::UTF16String cancelText = inCancelText.empty() ? dvacore::ZString("$$$/EACL/SimpleMessageBoxCancel=Cancel") : inCancelText;

			dvaui::dialogs::UI_MessageBox::Button result = dvaui::dialogs::messagebox::Messagebox(
				workspace->GetDrawSupplier(),
				inDialogTitle,
				inMessage,
				window,
				inOkText,			// inOkText
				cancelText,			// inCancelText
				dvacore::UTF16String(),
				dvacore::UTF16String(),
				dvacore::UTF16String(),			// inCheckboxText
				NULL,							// ioCheckboxIsChecked
				inDefaultButton,	// inDefaultButton
				inIcon);

			return result;	
		}

		ASL::String ConvertMarkerTagsToString(const TagParamMap inTagParamMap, const ASL::String& inSeparatorStr)
		{
			ASL::String markerTags;

			BOOST_FOREACH(const auto& indexParamPair, inTagParamMap)
			{
				TagParamPtr eachTag = indexParamPair.second;
				if (eachTag)
					markerTags += eachTag->GetName() + inSeparatorStr;
			}

			if (!markerTags.empty() && markerTags.size() >= inSeparatorStr.size())
				markerTags.erase(markerTags.size() - inSeparatorStr.size());
			return markerTags;
		}
	} 
} // End MZ
