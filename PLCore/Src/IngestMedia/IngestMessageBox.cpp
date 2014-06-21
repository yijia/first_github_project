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

// Prefix
#include "Prefix.h"

// Local
#include "IngestMedia/PLIngestMessageBox.h"

// dva
#include "dvacore/config/Localizer.h"

namespace PL
{
namespace
{
	#define  INGEST_MERGE_BUTTON			dvaui::dialogs::UI_MessageBox::kButton_NoToAll
	#define  INGEST_OVERWRITE_BUTTON		dvaui::dialogs::UI_MessageBox::kButton_Yes

	const dvacore::UTF16String kBlankString(DVA_STR(""));

	/*
	**	Return the DVA button constant equivalents for the specified Ingest buttons.
	*/
	int GetDVAButtonsFromIngest(
		IG_MessageBox::IngestButtons inButtons)
	{
		int buttons = 0;

		if ((inButtons & IG_MessageBox::kIngestButton_Ok) > 0)
		{
			buttons |= dvaui::dialogs::UI_MessageBox::kButton_Ok;
		}

		if ((inButtons & IG_MessageBox::kIngestButton_Cancel) > 0)
		{
			buttons |= dvaui::dialogs::UI_MessageBox::kButton_Cancel;
		}

		if ((inButtons & IG_MessageBox::kIngestButton_Ignore) > 0)
		{
			buttons |= dvaui::dialogs::UI_MessageBox::kButton_Ignore;
		}

		if ((inButtons & IG_MessageBox::kIngestButton_Merge) > 0)
		{
			buttons |= INGEST_MERGE_BUTTON;
		}

		if ((inButtons & IG_MessageBox::kIngestButton_Overwrite) > 0)
		{
			buttons |= INGEST_OVERWRITE_BUTTON;
		}
	
		if ((inButtons & IG_MessageBox::kIngestButton_Retry) > 0)
		{
			buttons |= dvaui::dialogs::UI_MessageBox::kButton_Retry;
		}

		DVA_ASSERT(buttons != 0);
		
		return buttons;
	}

	/*
	**	Return the Ingest button constant equivalents for the specified DVA buttons.
	*/
	int GetIngestButtonsFromDVA(
		dvaui::dialogs::UI_MessageBox::Buttons inButtons)
	{
		int buttons = 0;

		if ((inButtons &dvaui::dialogs::UI_MessageBox::kButton_Ok) > 0)
		{
			buttons |=  IG_MessageBox::kIngestButton_Ok;
		}

		if ((inButtons & dvaui::dialogs::UI_MessageBox::kButton_Cancel) > 0)
		{
			buttons |= IG_MessageBox::kIngestButton_Cancel;
		}

		if ((inButtons & dvaui::dialogs::UI_MessageBox::kButton_Ignore) > 0)
		{
			buttons |= IG_MessageBox::kIngestButton_Ignore;
		}

		if ((inButtons & INGEST_MERGE_BUTTON) > 0)
		{
			buttons |= IG_MessageBox::kIngestButton_Merge;
		}

		if ((inButtons & INGEST_OVERWRITE_BUTTON) > 0)
		{
			buttons |= IG_MessageBox::kIngestButton_Overwrite;
		}
	
		if ((inButtons & dvaui::dialogs::UI_MessageBox::kButton_Retry) > 0)
		{
			buttons |= IG_MessageBox::kIngestButton_Retry;
		}

		DVA_ASSERT(buttons != 0);
		
		return buttons;
	}

	/*
	**	Return the dva constant for the specified icon type.
	*/
	dvaui::dialogs::UI_MessageBox::Icon GetDVAIcon(
		IG_MessageBox::IngestIcon inIcon)
	{
		dvaui::dialogs::UI_MessageBox::Icon icon;

		switch(inIcon)
		{
			case IG_MessageBox::kIcon_None:
				icon = dvaui::dialogs::UI_MessageBox::kIcon_None;
				break;
			case IG_MessageBox::kIngestIcon_Stop: 
				icon = dvaui::dialogs::UI_MessageBox::kIcon_Stop;
				break;
			case IG_MessageBox::kIngestIcon_Caution:
				icon = dvaui::dialogs::UI_MessageBox::kIcon_Caution;
				break;
			case IG_MessageBox::kIngestIcon_Note:
				icon = dvaui::dialogs::UI_MessageBox::kIcon_Note;
				break;
			default:
				icon = dvaui::dialogs::UI_MessageBox::kIcon_None;
		}
		return icon;
	}

	/*
	**	
	*/
	void CommonShowNoteWithCheckbox(
				IG_MessageBox::SharedPtr messagebox,
				const dvacore::UTF16String& inCheckboxText,
				bool& ioCheckboxIsChecked)
	{
		messagebox->InitOptionalCheckbox(inCheckboxText, ioCheckboxIsChecked);

		messagebox->RunModal();

		if (messagebox->GetOptionalCheckbox())
			ioCheckboxIsChecked = messagebox->GetOptionalCheckbox()->GetValue() == dvaui::controls::kCheckboxValue_Checked;
	}

	/*
	**	
	*/
	void MapDvaButtonsToIngestButtons(
				IG_MessageBox::SharedPtr inMessagebox,
				IG_MessageBox::IngestButtons inButtons)
	{
		dvacore::UTF16String tempString; 
		dvaui::controls::UI_TextButton::SharedPtr buttonPtr;
		if ((inButtons & IG_MessageBox::kIngestButton_Merge) != 0)
		{
			tempString = dvacore::ZString("$$$/Prelude/Mezzanine/IngestMessageBox/Merge=Merge");
			buttonPtr = inMessagebox->GetSpecificButton(INGEST_MERGE_BUTTON);
			buttonPtr->SetText(tempString);
		}
		if ((inButtons & IG_MessageBox::kIngestButton_Overwrite) != 0)
		{
			tempString = dvacore::ZString("$$$/Prelude/Mezzanine/IngestMessageBox/Overwrite=Overwrite");
			buttonPtr = inMessagebox->GetSpecificButton(INGEST_OVERWRITE_BUTTON);
			buttonPtr->SetText(tempString);
		}
	}

}; // unnamed namespace


	/*
	**
	*/
	IG_MessageBox::IG_MessageBox(
				const dvaui::drawbot::SupplierInterface* inDrawSupplier, 
				const dvacore::UTF16String& inTitle, 
				const dvacore::UTF16String& inMessage,
				IngestButtons inButtons,
				IngestButton inDefaultButton,
				IngestIcon inIcon,
				dvaui::ui::UI_NodeSharedPtr inParent,
				std::uint32_t inWindowsStyleFlags,
				float	inMinDlogWidth,
				float	inMinDlogHeight)
				: dvaui::dialogs::UI_MessageBox(
										inDrawSupplier,
										inTitle,
										inMessage,
										static_cast<dvaui::dialogs::UI_MessageBox::Buttons>(GetDVAButtonsFromIngest(inButtons)),
										static_cast<dvaui::dialogs::UI_MessageBox::Button>(GetDVAButtonsFromIngest(inDefaultButton)),
										GetDVAIcon(inIcon),
										inParent,
										inWindowsStyleFlags,
										inMinDlogWidth,
										inMinDlogHeight)
	{
	}

	/*
	**
	*/
	IG_MessageBox::IG_MessageBox(
				const dvaui::drawbot::SupplierInterface* inDrawSupplier, 
				const dvacore::UTF16String& inTitle, 
				const dvacore::UTF16String& inMessage,
				IngestButtons inButtons,
				IngestButton inDefaultButton,
				IngestIcon inIcon,
				dvaui::ui::PlatformWindow inParent,
				std::uint32_t inWindowsStyleFlags,
				float	inMinDlogWidth,
				float	inMinDlogHeight)
				: dvaui::dialogs::UI_MessageBox(
										inDrawSupplier,
										inTitle,
										inMessage,
										static_cast<dvaui::dialogs::UI_MessageBox::Buttons>(GetDVAButtonsFromIngest(inButtons)),
										static_cast<dvaui::dialogs::UI_MessageBox::Button>(GetDVAButtonsFromIngest(inDefaultButton)),
										GetDVAIcon(inIcon),
										inParent,
										inWindowsStyleFlags,
										inMinDlogWidth,
										inMinDlogHeight)
	{
	}

	/*
	**
	*/
	void IG_MessageBox::InitSpecificButton(
							Buttons inButtonGroup, 
							Button inSpecificButton, 
							const dvacore::UTF16String& inString,
							dvaui::controls::SharedControlMessageFunction inMessageFunction)
	{
		if ((inButtonGroup & inSpecificButton) != 0)
		{
			dvacore::UTF16String tempString = inString;

			if (inSpecificButton == INGEST_MERGE_BUTTON)
			{
				tempString = dvacore::ZString("$$$/Prelude/Mezzanine/IngestMessageBox/Merge=Merge");
			}
			else if (inSpecificButton == INGEST_OVERWRITE_BUTTON)
			{
				tempString = dvacore::ZString("$$$/Prelude/Mezzanine/IngestMessageBox/Overwrite=Overwrite");
			}
			
			_inherited::InitSpecificButton(
				inButtonGroup,
				inSpecificButton,
				tempString,
				inMessageFunction);
		}
	}

namespace IngestMessageBox
{

/*
**
*/
IG_MessageBox::IngestButton ShowNoteWithCheckbox(
		const dvaui::drawbot::SupplierInterface* inDrawSupplier,
		const dvacore::UTF16String& inTitle, 
		const dvacore::UTF16String& inMessage,
		const dvacore::UTF16String& inCheckboxText,
		bool& ioCheckboxIsChecked,
		dvaui::ui::UI_Node* inParent,
		IG_MessageBox::IngestButtons inButtons,
		IG_MessageBox::IngestButton	 inDefaultButton,
		IG_MessageBox::IngestIcon	 inIcon)
	{
		dvacore::UTF16String title = inTitle;
#ifdef DVA_OS_MAC
		title = kBlankString;
#endif
		IG_MessageBox::SharedPtr messagebox = new IG_MessageBox(
			inDrawSupplier, 
			title, 
			inMessage,
			inButtons,
			inDefaultButton,
			inIcon,
			inParent,
			// According to Adobe UI Guidelines, alerts on Windows should not have a close button
			// in the upper right corner.  This means we have to explicitly specify the Windows style.
#ifdef DVA_OS_WIN
			WS_POPUP | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | DS_MODALFRAME | WS_BORDER);
#elif defined (DVA_OS_MAC)
			0);
#endif
	
		MapDvaButtonsToIngestButtons(messagebox, inButtons);
		CommonShowNoteWithCheckbox(messagebox, inCheckboxText, ioCheckboxIsChecked);

		return static_cast<IG_MessageBox::IngestButton>(GetIngestButtonsFromDVA(messagebox->GetResponse()));
	}

/*
**
*/
IG_MessageBox::IngestButton ShowNoteWithCheckbox(
		const dvaui::drawbot::SupplierInterface* inDrawSupplier,
		const dvacore::UTF16String& inTitle, 
		const dvacore::UTF16String& inMessage, 
		const dvacore::UTF16String& inCheckboxText,
		bool& ioCheckboxIsChecked,
		dvaui::ui::PlatformWindow inParent,
		IG_MessageBox::IngestButtons inButtons,
		IG_MessageBox::IngestButton	 inDefaultButton,
		IG_MessageBox::IngestIcon	 inIcon)
	{
		dvacore::UTF16String title = inTitle;
		// Adobe UI guidelines say that alerts on the Mac should have no text in the title area
#ifdef DVA_OS_MAC
		title = kBlankString;
#endif

		IG_MessageBox::SharedPtr messagebox = new IG_MessageBox(
			inDrawSupplier, 
			title, 
			inMessage,
			inButtons,
			inDefaultButton,
			inIcon,
			inParent,

			// According to Adobe UI Guidelines, alerts on Windows should not have a close button
			// in the upper right corner.  This means we have to explicitly specify the Windows style.
#ifdef DVA_OS_WIN
			WS_POPUP | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | DS_MODALFRAME | WS_BORDER);
#elif defined (DVA_OS_MAC)
			0);
#endif

		MapDvaButtonsToIngestButtons(messagebox, inButtons);
		CommonShowNoteWithCheckbox(messagebox, inCheckboxText, ioCheckboxIsChecked);

		return static_cast<IG_MessageBox::IngestButton>(GetIngestButtonsFromDVA(messagebox->GetResponse()));
	}
};

}; // namespace PL

