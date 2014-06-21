/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2012 Adobe Systems Incorporated		               */
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

#ifndef PLCLIPACTIONS_H
#define PLCLIPACTIONS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef BE_CLIP_IMASTERCLIP_H
#include "BE/Clip/IMasterClip.h"
#endif

#ifndef MZACTION_H
#include "MZAction.h"
#endif

namespace PL 
{
namespace ClipActions
{

/**
**	Set the logging description for a master clip.
**
**	@param	inMasterClip	the master clip to set
**	@param	inDescription	the new description
**	@param	inUndoableType	is undoable, we may not need undoable version for export
*/
PL_EXPORT
void SetMasterClipDescription(
		BE::IMasterClipRef inMasterClip,
		const ASL::String& inDescription,
		MZ::ActionUndoableFlags::Type inUndoableType);

}

};

#endif

