/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2008 Adobe Systems Incorporated                       */
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

#ifndef MZ_IPLPRESETFWD_H
#define MZ_IPLPRESETFWD_H

#include "ASLInterface.h"
#include <vector>

#include "dvacore/config/PreConfig.h"

#include "PLExport.h"

namespace PL
{

	ASL_DEFINE_IREF(IPLPreset);
	ASL_DEFINE_IID(IPLPreset, 0xa0f81f5f, 0xfd2f, 0x479b, 0xa4, 0x8c, 0xb5, 0x8f, 0x6a, 0x79, 0x41, 0xa4);

	ASL_DEFINE_IREF(IPLPresetField);
	ASL_DEFINE_IID(IPLPresetField, 0xfcafd6e5, 0x2cfb, 0x4b6f, 0x84, 0x34, 0xbe, 0x44, 0xf5, 0x85, 0xfc, 0xa5);

	typedef std::vector<IPLPresetFieldRef> IPLPresetFields;


	/**
	**	A list of presets
	*/
	typedef std::vector<IPLPresetRef> IPLPresets;

	typedef enum _PLPresetCategory 
	{
		PLPreCategory_Rename = 0,
		PLPreCategory_Metadata,
		PLPreCategory_end
	} PLPresetCategory;

	typedef enum _PLPresetType
	{
		kPLPresetType_USER,
		kPLPresetType_SYSTEM,
		kPLPresetTYpe_PLACEHOLDER
	} PLPresetType;

	typedef enum _PLPresetFieldType
	{
		kPLPresetFieldTYpe_Unknown = -1,
		kPLPresetFieldType_AutoIncrease = 0,
		kPLPresetFieldType_CustomText,
		kPLPresetFieldType_CreationDate,
		kPLPresetFieldType_IngestDate,
		kPLPresetFieldType_CreationTime,
		kPLPresetFieldType_IngestTime,
		kPLPresetFieldType_Filename,
		kPLPresetFieldType_UserDefined,
		kPLPresetFieldType_UserDefinedAutoIncrease,
		kPLPresetFieldTYpe_End
	} PLPresetFieldType;

	//PR == Preset
	ASL_DEFINE_RESULT_SPACE(kPRResults, 100);

	/// Preset related errors
	ASL_DEFINE_ERROR(kPRResults, ePresetFileAlreadyExists, 450);
	ASL_DEFINE_ERROR(kPRResults, ePresetLoadFailed, 451);
	ASL_DEFINE_ERROR(kPRResults, ePresetNotSupported, 452);
	ASL_DEFINE_ERROR(kPRResults, ePresetNoRightToDelete, 453);
}

#include "dvacore/config/PostConfig.h"

#endif