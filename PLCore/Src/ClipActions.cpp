/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

//	Prefix
#include "Prefix.h"

//	Self
#include "PLClipActions.h"

// MZ
#include "MZClipActions.h"

// BE
#include "BE/Clip/IClipLoggingInfo.h"

// dvacore
#include "dvacore/config/Localizer.h"


namespace PL
{

namespace ClipActions
{

/*
**
*/
void SetMasterClipDescription(
	BE::IMasterClipRef inMasterClip,
	const ASL::String& inDescription,
	MZ::ActionUndoableFlags::Type inUndoableType/* = MZ::ActionUndoableFlags::kUndoable*/)
{
	ASL_REQUIRE(inMasterClip != NULL);

	BE::IClipLoggingInfoRef loggingInfo = inMasterClip->GetClipLoggingInfo();
	ASL_ASSERT(loggingInfo != 0);

	loggingInfo->SetDescription(inDescription);
	
	BE::IActionRef action = inMasterClip->CreateSetClipLoggingInfoAction(loggingInfo);

	if (inUndoableType == MZ::ActionUndoableFlags::kUndoable)
	{
		MZ::ExecuteAction(BE::IExecutorRef(inMasterClip),
			action,
			MZ::ClipActions::kAction_SetMasterClipDescription,
			dvacore::ZString("$$$/Prelude/MZ/SetMasterClipDescriptionAction=Set Description"));
	}
	else
	{
		MZ::ExecuteActionWithoutUndo(
			BE::IExecutorRef(inMasterClip),
			action,
			(inUndoableType == MZ::ActionUndoableFlags::kNotUndoableWantsNotification));
	}
}

} // namespace ClipActions

} // namespace PL