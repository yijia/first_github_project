/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
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


#ifndef MZ_PLPRESETMANAGERMESSAGES_H
#define MZ_PLPRESETMANAGERMESSAGES_H

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef MZ_IPLPRESETFWD_H
#include "Preset/IPLPresetFwd.h"
#endif

namespace PL
{
PL_EXPORT extern ASL::StationID	GetPresetManagerStationID();
/**
**	Sent when the user preset list has changed (presets have been added/deleted).
**
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(PresetListChangedMessage, ASL::UInt32, ASL::UInt32);

/**
**	Sent when the user preset list has changed by replacing a preset
**	1. ref = old Preset ref (deleted)
**	2. ref = new Preset ref	(added/replaced)
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(PresetListChangedMessage_ReplaceUserPreset, IPLPresetRef, IPLPresetRef);

}

#endif