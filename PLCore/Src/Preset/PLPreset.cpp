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

//ASL
#include "ASLTypes.h"
#include "ASLClassFactory.h"

// DVA
#include "dvacore/config/Localizer.h"

// boost
#include "boost/foreach.hpp"

// Local
#include "Preset/PLPreset.h"
#include "Preset/IPLPresetManager.h"

namespace PL
{
	namespace 
	{
		// Common Key
		const ASL::UInt32	kPLPresetVersion = 1;
		const BE::Key		kPLPresetNameKey( BE_KEY("PLPresetName") );
		const BE::Key		kPLPresetIDKey( BE_KEY("PLPresetID") );

		const BE::Key		kPLPresetFieldCount( BE_KEY("PLFieldCount") );
		const BE::Key		kPLPresetFieldGroup(BE_KEY("PLFieldGroup"));
		const BE::Key		kPLPresetFieldItem( BE_KEY("PLFieldItem") );

		const dvacore::UTF16String kZStringSignature(DVA_STR("$" "$" "$"));

		const dvacore::UTF16String kPresetFieldTypeKey_AutoIncrease(DVA_STR("AutoIncrease"));
		const dvacore::UTF16String kPresetFieldTypeKey_UserDefinedAutoIncrease(DVA_STR("UserDefinedAutoIncrease"));
		const dvacore::UTF16String kPresetFieldTypeKey_CustomText(DVA_STR("CustomText"));
		const dvacore::UTF16String kPresetFieldTypeKey_CreationDate(DVA_STR("CreationDate"));
		const dvacore::UTF16String kPresetFieldTypeKey_IngestDate(DVA_STR("IngestDate"));
		const dvacore::UTF16String kPresetFieldTypeKey_CreationTime(DVA_STR("CreationTime"));
		const dvacore::UTF16String kPresetFieldTypeKey_IngestTime(DVA_STR("IngestTime"));
		const dvacore::UTF16String kPresetFieldTypeKey_FileName(DVA_STR("FileName"));

	}

	//////////////////////////////////////////////////////////////////////////
	// PLPresetField
	//////////////////////////////////////////////////////////////////////////

	PLPresetFieldType PresetField::GetType() const
	{
		if (0 == mKey.compare(kPresetFieldTypeKey_AutoIncrease))
			return kPLPresetFieldType_AutoIncrease;
		if (0 == mKey.compare(kPresetFieldTypeKey_UserDefinedAutoIncrease))
			return kPLPresetFieldType_UserDefinedAutoIncrease;
		if (0 == mKey.compare(kPresetFieldTypeKey_CustomText))
			return kPLPresetFieldType_CustomText;
		if (0 == mKey.compare(kPresetFieldTypeKey_CreationDate))
			return kPLPresetFieldType_CreationDate;
		if (0 == mKey.compare(kPresetFieldTypeKey_IngestDate))
			return kPLPresetFieldType_IngestDate;
		if (0 == mKey.compare(kPresetFieldTypeKey_CreationTime))
			return kPLPresetFieldType_CreationTime;
		if (0 == mKey.compare(kPresetFieldTypeKey_IngestTime))
			return kPLPresetFieldType_IngestTime;
		if (0 == mKey.compare(kPresetFieldTypeKey_FileName))
			return kPLPresetFieldType_Filename;

		return kPLPresetFieldType_UserDefined;
	}

	void PresetField::SetType(PLPresetFieldType inType)
	{
		switch (inType)
		{
		case kPLPresetFieldType_AutoIncrease:
			mKey = kPresetFieldTypeKey_AutoIncrease;
			break;
		case kPLPresetFieldType_UserDefinedAutoIncrease:
			mKey = kPresetFieldTypeKey_UserDefinedAutoIncrease;
			break;
		case kPLPresetFieldType_CustomText:
			mKey = kPresetFieldTypeKey_CustomText;
			break;
		case kPLPresetFieldType_CreationDate:
			mKey = kPresetFieldTypeKey_CreationDate;
			break;
		case kPLPresetFieldType_IngestDate:
			mKey = kPresetFieldTypeKey_IngestDate;
			break;
		case kPLPresetFieldType_CreationTime:
			mKey = kPresetFieldTypeKey_CreationTime;
			break;
		case kPLPresetFieldType_IngestTime:
			mKey = kPresetFieldTypeKey_IngestTime;
			break;
		case kPLPresetFieldType_Filename:
			mKey = kPresetFieldTypeKey_FileName;
			break;
		case kPLPresetFieldType_UserDefined: // nothing to do as we have put key into mKey
			break;
		default:
			DVA_ASSERT(false);
			break;
		}
	}

	IPLPresetFieldRef PresetField::Create(PLPresetFieldType inType)
	{
		PresetFieldRef field = CreateClassRef();
		switch (inType)
		{
		case kPLPresetFieldType_AutoIncrease:
			field->SetKey(kPresetFieldTypeKey_AutoIncrease);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_UserDefinedAutoIncrease:
			field->SetKey(kPresetFieldTypeKey_UserDefinedAutoIncrease);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_CustomText:
			field->SetKey(kPresetFieldTypeKey_CustomText);
			// leave value as empty
			break;
		case kPLPresetFieldType_CreationDate:
			field->SetKey(kPresetFieldTypeKey_CreationDate);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_IngestDate:
			field->SetKey(kPresetFieldTypeKey_IngestDate);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_CreationTime:
			field->SetKey(kPresetFieldTypeKey_CreationTime);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_IngestTime:
			field->SetKey(kPresetFieldTypeKey_IngestTime);
			field->SetTemplateValue(GetPredefinedFieldValues(inType).at(0));
			break;
		case kPLPresetFieldType_Filename:
			field->SetKey(kPresetFieldTypeKey_FileName);
			// leave value to be empty
			break;
		case kPLPresetFieldType_UserDefined:
			break;
		default:
			DVA_ASSERT(false);
			break;
		}

		return IPLPresetFieldRef(field);
	}

	void PresetField::ValidateValue()
	{
		PLPresetFieldType fieldType = GetType();
		if (fieldType == kPLPresetFieldType_UserDefined)
			return;

		std::vector<dvacore::UTF16String> values = GetPredefinedFieldValues(fieldType);
		if (!values.empty())
		{
			// force value to be the first predefined value if an invalid value is loaded
			if (std::find(values.begin(), values.end(), mTemplateValue) == values.end())
				mTemplateValue = values.front();
		}
	}

//////////////////////////////////////////////////////////////////////////
// PLPreset
//////////////////////////////////////////////////////////////////////////

void PLPreset::Initialize()
{
	ASL_REGISTER_CLASS(kPresetFieldClassID, PresetField);	
}

void PLPreset::Terminate()
{
}

PLPreset::PLPreset() :
	mPresetID(ASL::Guid::CreateUnique())
{
}

PLPreset::~PLPreset()
{
}


void PLPreset::SetPresetName(
	const dvacore::UTF16String& inPresetName)
{
	mPresetName = inPresetName;
}

dvacore::UTF16String PLPreset::GetPresetName(bool inResolveZString) const
{
	return inResolveZString ? ResolveZString(mPresetName) : mPresetName;
}

IPLPresetRef PLPreset::Create()
{
	return IPLPresetRef();
}

void PLPreset::Reset()
{
	
}

IPLPresetRef PLPreset::Clone(bool inCloneUniqueID)
{
	PLPresetRef clonePreset	= PLPreset::CreateClassRef();
	clonePreset->mPresetName = mPresetName;
	clonePreset->mType = kPLPresetType_USER; // For safety. It's impossible to clone a system preset in current design.

	BOOST_FOREACH(IPLPresetFields::value_type field, mFields)
	{
		IPLPresetFieldRef clonedField = PresetField::Create(field->GetKey(), field->GetTemplateValue(), field->GetMandatoryFlag());
		clonePreset->mFields.push_back(clonedField);
	}

	if (inCloneUniqueID)
		clonePreset->mPresetID = mPresetID;

	return IPLPresetRef(clonePreset);
}

// static
dvacore::UTF16String PLPreset::ResolveZString(const dvacore::UTF16String& inZString)
{
	if(inZString.find(kZStringSignature) != dvacore::UTF16String::npos)
	{
		dvacore::UTF16String inputString;
		if(inZString.length()>=2 && 
			(((inZString[0]=='(') && (inZString[inZString.length()-1]==')'))
				|| ((inZString[0]=='\'') && (inZString[inZString.length()-1]=='\''))
				|| ((inZString[0]=='"') && (inZString[inZString.length()-1]=='"'))))
				
		{
			inputString = inZString.substr(1, inZString.length()-2);
		}
		else
			inputString = inZString;

		dvacore::UTF16String resolvedString =
			dvacore::config::Localizer::Get()->GetLocalizedString(
			reinterpret_cast<const char*>(dvacore::utility::UTF16to8(inputString).c_str()));

		if(resolvedString.find(kZStringSignature) == dvacore::UTF16String::npos)
			return resolvedString;
		else
			return dvacore::UTF16String();
	}
	else
		return inZString;
}

dvacore::UTF16String PLPreset::MakeZString(
	const dvacore::UTF16String& inEnglishString,
	const char* inTag) const
{
	if(inEnglishString.find(kZStringSignature) == dvacore::UTF16String::npos)
	{
		return	DVA_STR("(") + kZStringSignature + DVA_STR("/Prelude/Mezzanine/Preset/") +
			mPresetID.AsString() + DVA_STR("/") + dvacore::utility::AsciiToUTF16(inTag) + 
			DVA_STR("=") + inEnglishString + DVA_STR(")");
	}
	else
		return inEnglishString;

}


void PLPreset::SerializeIn(
	ASL::UInt32 inVersion, 
	BE::DataReader& inReader)
{
}

void PLPreset::SerializeOut(
	BE::DataWriter& inWriter) const
{
}

void PLPreset::SerializeToSink(
	BE::IDictionarySink& inSink) const
{

}

ASL::UInt32 PLPreset::GetVersion() const
{
	return kPLPresetVersion;
}

ASL::Guid PLPreset::GetClassID() const
{
	return kPLPresetClassID;
}

BE::Key PLPreset::GetClassName() const
{
	return BE_KEY("PLPreset");
}

dvacore::UTF16String PLPreset::GetPresetFullPath() const
{
	return mPresetFullPath;
}

void PLPreset::SetPresetFullPath(const dvacore::UTF16String& inFilePath)
{
	mPresetFullPath = inFilePath;
}

//-----------------------------------------------------------
//-----------------------Rename Preset-----------------------
//-----------------------------------------------------------
PLPresetRename::PLPresetRename()
{
}

void PLPresetRename::SerializeIn(ASL::UInt32 inVersion, BE::DataReader& inReader)
{
	ASL::SInt32 count = 0;
	mFields.clear();

	inReader.ReadValue(kPLPresetNameKey,				mPresetName);
	inReader.ReadValue(kPLPresetIDKey,					mPresetID);

	inReader.ReadValue(kPLPresetFieldCount,				count);
	inReader.ReadItems(kPLPresetFieldGroup,				mFields);

	DVA_ASSERT(count == mFields.size());
}


void PLPresetRename::SerializeOut(
	BE::DataWriter& inWriter) const
{
	inWriter.WriteValue(kPLPresetNameKey,				mPresetName);
	inWriter.WriteValue(kPLPresetIDKey,					mPresetID);

	inWriter.WriteValue(kPLPresetFieldCount,			mFields.size());
	inWriter.WriteItems(kPLPresetFieldGroup, kPLPresetFieldItem, mFields);
}

ASL::Guid PLPresetRename::GetClassID() const
{
	return kPLPresetRenameClassID;
}

BE::Key PLPresetRename::GetClassName() const
{
	return BE_KEY("PLPresetRename");
}

//-----------------------------------------------------------
//-----------------------Metadata Preset-----------------------
//-----------------------------------------------------------

void PLPresetMetadata::SerializeIn(ASL::UInt32 inVersion, BE::DataReader& inReader)
{
	ASL::SInt32 count = 0;
	mFields.clear();

	inReader.ReadValue(kPLPresetNameKey,				mPresetName);
	inReader.ReadValue(kPLPresetIDKey,					mPresetID);

	inReader.ReadValue(kPLPresetFieldCount,				count);
	inReader.ReadItems(kPLPresetFieldGroup,				mFields);

	DVA_ASSERT(count == mFields.size());
}


void PLPresetMetadata::SerializeOut(
	BE::DataWriter& inWriter) const
{
	inWriter.WriteValue(kPLPresetNameKey,				mPresetName);
	inWriter.WriteValue(kPLPresetIDKey,					mPresetID);

	inWriter.WriteValue(kPLPresetFieldCount,			mFields.size());
	inWriter.WriteItems(kPLPresetFieldGroup, kPLPresetFieldItem, mFields);
}

ASL::Guid PLPresetMetadata::GetClassID() const
{
	return kPLPresetMetadataClassID;
}

BE::Key PLPresetMetadata::GetClassName() const
{
	return BE_KEY("PLPresetMetadata");
}

}
