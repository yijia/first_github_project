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

#ifndef PLINGESTMESSAGEBOX_H
#define PLINGESTMESSAGEBOX_H

#pragma once

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef DVAUI_UI_MESSAGEBOX_H
#include "dvaui/dialogs/UI_MessageBox.h"
#endif

namespace PL
{
	class IG_MessageBox
		:
		public dvaui::dialogs::UI_MessageBox
	{
		typedef dvaui::dialogs::UI_MessageBox _inherited;
	public:
		DVA_UTIL_TYPEDEF_SHARED_REF_PTR(IG_MessageBox)	SharedPtr;

		enum IngestButton
		{
			kIngestButton_Ok	 	= 1 << 0,
			kIngestButton_Cancel 	= 1 << 1,
			kIngestButton_Ignore 	= 1 << 2,
			kIngestButton_Merge  	= 1 << 3,
			kIngestButton_Overwrite = 1 << 4,
			kIngestButton_Retry		= 1 << 5
		};
		typedef int IngestButtons;

		enum IngestIcon
		{
			kIngestIcon_None = 0,
			kIngestIcon_Stop,
			kIngestIcon_Caution,
			kIngestIcon_Note
		};


		PL_EXPORT IG_MessageBox(
			const dvaui::drawbot::SupplierInterface* inDrawSupplier, 
			const dvacore::UTF16String& inTitle, 
			const dvacore::UTF16String& inMessage, 
			IngestButtons inButtons,
			IngestButton inDefaultButton,
			IngestIcon inIcon,
			dvaui::ui::UI_NodeSharedPtr inParent = 0,
			std::uint32_t inWindowsStyleFlags = 0,
			float	inMinDlogWidth = 0.0f,
			float	inMinDlogHeight = 0.0f);

		PL_EXPORT IG_MessageBox(
			const dvaui::drawbot::SupplierInterface* inDrawSupplier, 
			const dvacore::UTF16String& inTitle, 
			const dvacore::UTF16String& inMessage, 
			IngestButtons inButtons,
			IngestButton inDefaultButton,
			IngestIcon inIcon,
			dvaui::ui::PlatformWindow,
			std::uint32_t inWindowsStyleFlags = 0,
			float	inMinDlogWidth = 0.0f,
			float	inMinDlogHeight = 0.0f);

	protected:

		/*
		**
		*/
		virtual void	InitSpecificButton(
					Buttons inButtonGroup, 
					Button inSpecificButton, 
					const dvacore::UTF16String& inString,
					dvaui::controls::SharedControlMessageFunction inMessageFunction);

	};

namespace IngestMessageBox
{
	/// Displays a note alert to the user.
	/** ShowNote Displays an alert that has the note icon, message, and Ok button. Returns when user clicks Ok.
	@param inTitle				The string for the dialog's title.
	@param inMessage			The message to present to the user.
	@param inCheckboxText		The text that is shown with the checkbox
	@param ioCheckboxIsChecked	IN: specifies whether the checkbox is pre-checked.  
								OUT:  specifies whether the checkbox was checked when the messagebox was dismissed.
	*/
	PL_EXPORT IG_MessageBox::IngestButton ShowNoteWithCheckbox(
						const dvaui::drawbot::SupplierInterface* inDrawSupplier,
						const dvacore::UTF16String& inTitle, 
						const dvacore::UTF16String& inMessage,
						const dvacore::UTF16String& inCheckboxText,
						bool& ioCheckboxIsChecked,
						dvaui::ui::UI_Node* inParent = 0,
						IG_MessageBox::IngestButtons inButtons = IG_MessageBox::kIngestButton_Ok,
						IG_MessageBox::IngestButton	 inDefaultButton = IG_MessageBox::kIngestButton_Ok,
						IG_MessageBox::IngestIcon	 inIcon = IG_MessageBox::kIngestIcon_Note);

	PL_EXPORT IG_MessageBox::IngestButton ShowNoteWithCheckbox(
						const dvaui::drawbot::SupplierInterface* inDrawSupplier,
						const dvacore::UTF16String& inTitle, 
						const dvacore::UTF16String& inMessage, 
						const dvacore::UTF16String& inCheckboxText,
						bool& ioCheckboxIsChecked,
						dvaui::ui::PlatformWindow inParent,
						IG_MessageBox::IngestButtons inButtons = IG_MessageBox::kIngestButton_Ok,
						IG_MessageBox::IngestButton	 inDefaultButton = IG_MessageBox::kIngestButton_Ok,
						IG_MessageBox::IngestIcon	 inIcon = IG_MessageBox::kIngestIcon_Note);

} // namespace IngestMessageBox
} // namespace PL

#endif