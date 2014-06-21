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

#ifndef MZ_PLPRESET_H
#define MZ_PLPRESET_H

#include "ASLFixedSizeEnum.h"
#include "ASLClass.h"

// Local
#include "Preset/IPLPresetFwd.h"
#include "Preset/IPLPreset.h"

// BE
#include "BE/Core/ISerializeable.h"
#include "BE/Core/ISerializeableDetails.h"
#include "BEDataReader.h"
#include "BEDataWriter.h"

namespace PL
{

ASL_DEFINE_CLASSREF(PLPreset, IPLPreset);
ASL_DEFINE_IMPLID(PLPreset, 0x51520e7c, 0x4068, 0x4ed7, 0x88, 0x3c, 0x98, 0x64, 0xbe, 0xe1, 0xd3, 0xaf);
const ASL::Guid	kPLPresetClassID("1D22040D-7486-4E41-BC5E-1561FA91AF18"); // This is different with the IMPLID

class PLPreset
	:
	public BE::ISerializeable,
	public IPLPreset
{
	ASL_BASIC_CLASS_SUPPORT(PLPreset);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLPreset)
		ASL_QUERY_ENTRY(BE::ISerializeable)
		ASL_QUERY_ENTRY(PLPreset)
	ASL_QUERY_MAP_END

	PLPreset();
	virtual ~PLPreset();

	virtual void					SetPresetName(const dvacore::UTF16String& inPresetName);
	virtual dvacore::UTF16String	GetPresetName(bool inResolveZString = true) const;

	//ISerializable
	virtual void				SerializeIn(
									ASL::UInt32 inVersion, 
									BE::DataReader& inReader);

	virtual void				SerializeOut(
									BE::DataWriter& inWriter) const;

	virtual void				SerializeToSink(
									BE::IDictionarySink& inSink) const;

	virtual ASL::UInt32			GetVersion() const;
	virtual ASL::Guid			GetClassID() const;
	virtual BE::Key				GetClassName() const;
	
	// Self 
	virtual IPLPresetRef		Clone(bool inCloneUniqueID = false);
	virtual IPLPresetRef		Create();
	virtual void				Reset();

	virtual bool				IsEditable() const { return mType != kPLPresetType_SYSTEM; }

	virtual void				SetFields(const IPLPresetFields& inFields) { mFields = inFields; }
	virtual IPLPresetFields		GetFields() const { return mFields; }

	virtual dvacore::UTF16String GetPresetFullPath() const;
	virtual void				 SetPresetFullPath(const dvacore::UTF16String& inFilePath);

	virtual PLPresetType		GetPresetType() const { return mType; }
	virtual void				SetPresetType(const PLPresetType& inType) { mType = inType; }

public:
	static dvacore::UTF16String ResolveZString(const dvacore::UTF16String& inZString);

	static void					Initialize();
	static void					Terminate();
	
private:
	dvacore::UTF16String		MakeZString(
									const dvacore::UTF16String& inEnglishString,
									const char* inTag)  const;

	dvacore::UTF16String		GenerateMatchName();

protected:
	ASL::Guid					mPresetID;
	dvacore::UTF16String		mPresetName;
	dvacore::UTF16String		mPresetFullPath;

	IPLPresetFields				mFields;	

	PLPresetType				mType;
};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Preset field
//----------------------------------------------------------------------
//----------------------------------------------------------------------

ASL_DEFINE_CLASSREF(PresetField, IPLPresetField);
ASL_DEFINE_IMPLID(PresetField, 0x38ef024d, 0x4bea, 0x4d29, 0x9f, 0x42, 0x9d, 0x5c, 0xb1, 0x5c, 0x19, 0xa9);
const ASL::Guid kPresetFieldClassID("CF37DDDD-B158-4D87-BEA2-6382DFDCF46D");

class PresetField :
	public BE::ISerializeable,
	public IPLPresetField
{
	ASL_BASIC_CLASS_SUPPORT(PresetField);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(BE::ISerializeable)
		ASL_QUERY_ENTRY(IPLPresetField)
		ASL_QUERY_ENTRY(PresetField)
	ASL_QUERY_MAP_END

public:
	// clone a field for editing, only TemplateValue is used
	// as we need update value according to TemplateValue when we save
	static IPLPresetFieldRef Create(
		const dvacore::UTF16String& inKey, 
		const dvacore::UTF16String& inTemplateValue,
		const bool inMandatory = true)
	{
		PresetFieldRef field = CreateClassRef();
		field->mKey = inKey;
		field->mValue = inTemplateValue;
		field->mTemplateValue = inTemplateValue;
		field->mMandatory = inMandatory;
		return IPLPresetFieldRef(field);
	}

	// exact clone of a field, used to clone a field for saving
	// otherwise, SaveAs will share field between presets
	static IPLPresetFieldRef Create(const IPLPresetFieldRef inField)
	{
		if (!inField)
			return IPLPresetFieldRef();

		PresetFieldRef field = CreateClassRef();
		field->mKey = inField->GetKey();
		field->mValue = inField->GetValue();
		field->mTemplateValue = inField->GetTemplateValue();
		field->mMandatory = inField->GetMandatoryFlag();
		return IPLPresetFieldRef(field);
	}

	static IPLPresetFieldRef Create()
	{
		PresetFieldRef field = CreateClassRef();
		return IPLPresetFieldRef(field);
	}

	static IPLPresetFieldRef Create(PLPresetFieldType inType);

	PresetField() :
		mKey(DVA_STR("")),
		mMandatory(false),
		mTemplateValue(DVA_STR("")),
		mValue(DVA_STR(""))
	{
	}

	virtual ~PresetField() {}

	virtual dvacore::UTF16String GetKey() const { return mKey; }
	virtual dvacore::UTF16String GetValue() const {	return mValue; }
	virtual bool GetMandatoryFlag() const { return mMandatory; }
	virtual dvacore::UTF16String GetTemplateValue() const { return mTemplateValue; }

	virtual void SetKey(const dvacore::UTF16String& inKey) { mKey = inKey; }
	virtual void SetValue(const dvacore::UTF16String& inValue) { mValue = inValue; }
	virtual void SetMandatoryFlag(bool inValue) { mMandatory = inValue; }
	virtual void SetTemplateValue(const dvacore::UTF16String& inValue)
	{
		mTemplateValue = inValue;
		mValue = inValue;
	}
	
	virtual void SetType(PLPresetFieldType inType);
	virtual PLPresetFieldType GetType() const;

	// ISerializeable
	virtual ASL::UInt32 GetVersion() const { return 1; }
	virtual ASL::Guid GetClassID() const { return kPresetFieldClassID; }
	virtual BE::Key GetClassName() const { return BE_KEY("PresetField"); }

	virtual void SerializeIn(ASL::UInt32 inVersion, BE::DataReader& inReader)
	{
		inReader.ReadValue(BE_KEY("key"), mKey);
		inReader.ReadValue(BE_KEY("value"), mTemplateValue);
		inReader.ReadValue(BE_KEY("mandatory"), mMandatory);

		ValidateValue();

		mValue = mTemplateValue; // temporary solution: force sync between them
	}

	virtual void SerializeOut(BE::DataWriter& inWriter) const
	{
		inWriter.WriteValue(BE_KEY("key"), mKey);
		inWriter.WriteValue(BE_KEY("value"), mTemplateValue);
		inWriter.WriteValue(BE_KEY("mandatory"), mMandatory);
	}

	bool operator==(const PresetField& inRHS) const
	{
		return	(mKey == inRHS.mKey) &&
			(mValue == inRHS.mValue);
	}

private:
	void ValidateValue();

	dvacore::UTF16String		mValue; // the value on UI
	dvacore::UTF16String		mKey;
	bool						mMandatory;

	dvacore::UTF16String		mTemplateValue; // the value which will be saved to template
};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Preset Rename
//----------------------------------------------------------------------
//----------------------------------------------------------------------

ASL_DEFINE_CLASSREF(PLPresetRename, IPLPresetRename);
ASL_DEFINE_IMPLID(PLPresetRename, 0x98a83a29, 0xc252, 0x454c, 0xb3, 0x3b, 0x9e, 0xbb, 0x35, 0x9, 0xba, 0x11);
const ASL::Guid		kPLPresetRenameClassID("90FAA153-D562-4F30-807A-D28A4D1513F8");

class PLPresetRename 
	: 
	public PLPreset,
	public IPLPresetRename
{
	ASL_BASIC_CLASS_SUPPORT(PLPresetRename);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLPreset)
		ASL_QUERY_ENTRY(IPLPresetRename)
		ASL_QUERY_ENTRY(BE::ISerializeable)
		ASL_QUERY_ENTRY(PLPresetRename)
	ASL_QUERY_MAP_END

public:
	PLPresetRename();
	virtual ~PLPresetRename() {}

	virtual void SerializeOut(BE::DataWriter& inWriter) const;
	virtual void SerializeIn(ASL::UInt32 inVersion, BE::DataReader& inReader);
	
	virtual ASL::Guid GetClassID() const;
	virtual BE::Key GetClassName() const;
};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Preset Metadata
//----------------------------------------------------------------------
//----------------------------------------------------------------------
ASL_DEFINE_CLASSREF(PLPresetMetadata, IPLPresetMetadata);
ASL_DEFINE_IMPLID(PLPresetMetadata, 0xb5817771, 0xdc5c, 0x4139, 0x93, 0xb9, 0xbf, 0x37, 0x37, 0xe2, 0x26, 0xb4);
const ASL::Guid		kPLPresetMetadataClassID("92A656D8-6BD2-4E0F-8823-B4F4CE0879C9");

class PLPresetMetadata 
	: 
	public PLPreset,
	public IPLPresetMetadata
{
	ASL_BASIC_CLASS_SUPPORT(PLPresetMetadata);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLPreset)
		ASL_QUERY_ENTRY(IPLPresetMetadata)
		ASL_QUERY_ENTRY(BE::ISerializeable)
		ASL_QUERY_ENTRY(PLPresetMetadata)
	ASL_QUERY_MAP_END

public:
	PLPresetMetadata() {}
	virtual ~PLPresetMetadata() {}

	virtual void SerializeOut(BE::DataWriter& inWriter) const;
	virtual void SerializeIn(ASL::UInt32 inVersion, BE::DataReader& inReader);

	virtual ASL::Guid GetClassID() const;
	virtual BE::Key GetClassName() const;
};

}

#endif 