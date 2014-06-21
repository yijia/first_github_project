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

#ifndef MZ_IPLPRESET_H
#define MZ_IPLPRESET_H

//ASL
#include "ASLInterface.h"
#include "ASLTypes.h"
#include "ASLRect.h"

#include "dvacore/config/UnicodeTypes.h"

#include "Preset/IPLPresetFwd.h"

// STL
#include <vector>

namespace PL
{

ASL_INTERFACE IPLPresetField : public ASLUnknown
{
public:
	virtual dvacore::UTF16String GetKey() const = 0;
	virtual dvacore::UTF16String GetValue() const = 0;
	virtual bool GetMandatoryFlag() const = 0;
	virtual dvacore::UTF16String GetTemplateValue() const = 0;

	virtual void SetKey(const dvacore::UTF16String& inKey) = 0;
	virtual void SetValue(const dvacore::UTF16String& inValue) = 0;
	virtual void SetMandatoryFlag(bool inValue) = 0;
	virtual void SetTemplateValue(const dvacore::UTF16String& inValue) = 0;

	virtual void SetType(PLPresetFieldType inType) = 0;
	virtual PLPresetFieldType GetType() const = 0;
};

ASL_INTERFACE IPLPreset : public ASLUnknown
{
public:
	virtual dvacore::UTF16String GetPresetName(bool inResolveZString = true) const = 0;
	virtual void SetPresetName(const dvacore::UTF16String& inPresetName) = 0;

	virtual IPLPresetRef Create() = 0;
	virtual void Reset() = 0;
	virtual IPLPresetRef Clone(bool inCloneUniqueID = false) = 0;

	virtual bool IsEditable() const = 0;

	virtual void SetFields(const IPLPresetFields& inFields) = 0;
	virtual IPLPresetFields		 GetFields() const = 0;

	virtual dvacore::UTF16String GetPresetFullPath() const = 0;
	virtual void SetPresetFullPath(const dvacore::UTF16String& inFilePath) = 0;

	virtual PLPresetType GetPresetType() const = 0;
	virtual void SetPresetType(const PLPresetType& inType) = 0;
};

ASL_DEFINE_IREF(IPLPresetRename);
ASL_DEFINE_IID(IPLPresetRename, 0xcf3097a4, 0x65d7, 0x460d, 0x9c, 0xde, 0x4f, 0xf4, 0x55, 0x5d, 0xd, 0x91);

ASL_INTERFACE IPLPresetRename : public ASLUnknown
{
public:
};

ASL_DEFINE_IREF(IPLPresetMetadata);
ASL_DEFINE_IID(IPLPresetMetadata, 0x8fc245f4, 0x121a, 0x40e7, 0xa7, 0xf1, 0x7b, 0x9f, 0x8b, 0x14, 0x7a, 0xc6);

ASL_INTERFACE IPLPresetMetadata : public ASLUnknown
{
public:
};

}

#endif
