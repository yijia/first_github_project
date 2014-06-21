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
#include "TagTemplate/TagTemplate.h"
#include "TagTemplate/TagCell.h"

#include "dvacore/utility/Coercion.h"

#include "boost/property_tree/json_parser.hpp"
#include "boost/foreach.hpp"

namespace
{
	inline std::wstring UTF16ToWString(const ASL::String& inUTF16Str)
	{
		return dvacore::utility::UTF16ToWstring(inUTF16Str);
	}

	inline ASL::String WStringToUTF16(const std::wstring& inWString)
	{
		return dvacore::utility::WcharToUTF16(inWString.c_str());
	}

	inline std::wstring AsciiToWString(const char* inStr)
	{
		return UTF16ToWString(ASL::MakeString(inStr));
	}

	const char* kTagTemplateVersion = "1.0";
	const std::wstring kKeyName_Version = AsciiToWString("version");
	const std::wstring kKeyName_Guid = AsciiToWString("guid");
	const std::wstring kKeyName_Name = AsciiToWString("name");
	const std::wstring kKeyName_DimensionRows = AsciiToWString("dimension.rows");
	const std::wstring kKeyName_DimensionColumns = AsciiToWString("dimension.columns");
	const std::wstring kKeyName_SizeWidth = AsciiToWString("size.width");
	const std::wstring kKeyName_SizeHeight = AsciiToWString("size.height");
	const std::wstring kKeyName_MarginWidth = AsciiToWString("margin.width");
	const std::wstring kKeyName_MarginHeight = AsciiToWString("margin.height");
	const std::wstring kKeyName_Tags = AsciiToWString("tags");
	const std::wstring kKeyName_Payload = AsciiToWString("payload");
	const std::wstring kKeyName_Duration = AsciiToWString("duration");
	const std::wstring kKeyName_ColorRed = AsciiToWString("color.red");
	const std::wstring kKeyName_ColorGreen = AsciiToWString("color.green");
	const std::wstring kKeyName_ColorBlue = AsciiToWString("color.blue");
	const std::wstring kKeyName_PositonLeftColumn = AsciiToWString("position.leftcolumn");
	const std::wstring kKeyName_PositonTopRow = AsciiToWString("position.toprow");
	const std::wstring kKeyName_PositonRows = AsciiToWString("position.rows");
	const std::wstring kKeyName_PositonColumns = AsciiToWString("position.columns");
	const std::wstring kKeyName_ShortcutVKey = AsciiToWString("shortcut_vkey");
	const std::wstring kKeyName_ShortcutCtrl = AsciiToWString("shortcut_control");
	const std::wstring kKeyName_ShortcutAlt = AsciiToWString("shortcut_alt");
	const std::wstring kKeyName_ShortcutShift = AsciiToWString("shortcut_shift");
	const std::wstring kKeyName_ShortcutMacCtrl = AsciiToWString("shortcut_macctrl");
}

namespace PL
{

IPLTagTemplateReadonlyRef CreateDefaultTagTemplate(const ASL::String& inName)
{
	auto newTagTemplate = TagTemplate::CreateClassRef();
	newTagTemplate->SetName(inName);
	newTagTemplate->SetSize(dvacore::geom::Point32(100, 100));
	newTagTemplate->SetDimension(5, 6);
	return IPLTagTemplateReadonlyRef(newTagTemplate);
}

IPLTagTemplateReadonlyRef LoadTagTemplate(const ASL::String& inPath)
{
	return IPLTagTemplateReadonlyRef(TagTemplate::FromFile(inPath));
}

IPLTagTemplateReadonlyRef BuildTagTemplate(const ASL::String& inContent)
{
	return IPLTagTemplateReadonlyRef(TagTemplate::FromContent(inContent));
}

bool SaveTagTemplate(const ASL::String& inPath, IPLTagTemplateReadonlyRef inTagTemplate)
{
	TagTemplateRef tagTemplate(inTagTemplate);
	return tagTemplate ? tagTemplate->ToFile(inPath) : false;
}

TagTemplate::TagTemplate()
	: mGuid(dvacore::utility::Guid::CreateUnique())
{

}

dvacore::utility::Guid TagTemplate::GetGUID() const
{
	return mGuid;
}

ASL::String TagTemplate::GetName() const
{
	return mName;
}

ASL::UInt32 TagTemplate::GetRows() const
{
	return mDimension.y;
}

ASL::UInt32 TagTemplate::GetColumns() const
{
	return mDimension.x;
}

dvacore::geom::Point32 TagTemplate::GetSize() const
{
	return mSize;
}

dvacore::geom::Point32 TagTemplate::GetMargin() const
{
	return mMargin;
}

IPLTagCellReadonlyVector TagTemplate::GetTagCells() const
{
	IPLTagCellReadonlyVector vector;
	BOOST_FOREACH (const auto& idTagCellPair, mTagCellMap)
	{
		vector.push_back(idTagCellPair.second);
	}
	return vector;
}

void TagTemplate::SetName( const ASL::String& inName )
{
	mName = inName;
}

IPLTagCellReadonlyVector TagTemplate::SetDimension( ASL::UInt32 inColumns, ASL::UInt32 inRows )
{
	mDimension.Set(inColumns, inRows);
	dvacore::geom::Rect32 wholeArea(dvacore::geom::Point32(0, 0), mDimension);
	std::vector<dvacore::utility::Guid> toBeRemovedIDs;
	BOOST_FOREACH (const auto& idTagCellPair, mTagCellMap)
	{
		if (!wholeArea.ContainsInclusive(idTagCellPair.second->GetPosition()))
			toBeRemovedIDs.push_back(idTagCellPair.first);
	}

	IPLTagCellReadonlyVector removedTags;
	BOOST_FOREACH (const auto& id, toBeRemovedIDs)
	{
		removedTags.push_back(mTagCellMap[id]);
		mTagCellMap.erase(id);
	}

	return removedTags;
}

void TagTemplate::SetSize( const dvacore::geom::Point32& inSize )
{
	mSize = inSize;
}

void TagTemplate::SetMargin( const dvacore::geom::Point32& inMargin )
{
	mMargin = inMargin;
}

bool TagTemplate::InsertTagCell( IPLTagCellReadonlyRef inCell )
{
	if (inCell == NULL)
		return false;

	const auto& position = inCell->GetPosition();
	if (dvacore::geom::Rect32(dvacore::geom::Point32(0, 0), mDimension).ContainsInclusive(position))
	{
		mTagCellMap[inCell->GetGUID()] = inCell;
		return true;
	}
	return false;
}

void TagTemplate::RemoveTagCell( IPLTagCellReadonlyRef inCell )
{
	if (inCell != NULL)
	{
		mTagCellMap.erase(inCell->GetGUID());
	}
}

IPLTagCellReadonlyRef TagTemplate::FindTagByGUID(const dvacore::utility::Guid& inGuid) const
{
	auto iter = mTagCellMap.find(inGuid);
	return iter != mTagCellMap.end() ? iter->second : IPLTagCellReadonlyRef();
}

IPLTagCellReadonlyRef TagTemplate::FindTagByPosition(const dvacore::geom::Point32& inPosition) const
{
	BOOST_FOREACH (auto& idTagPair, mTagCellMap)
	{
		if (idTagPair.second->GetPosition().Contains(inPosition))
			return idTagPair.second;
	}
	return IPLTagCellReadonlyRef();
}

IPLTagTemplateReadonlyRef TagTemplate::Clone() const
{
	TagTemplateRef cloneInstance = CreateClassRef();
	cloneInstance->CloneInit(this);
	return IPLTagTemplateReadonlyRef(cloneInstance);
}

void TagTemplate::CloneInit(const TagTemplate* inOriginal)
{
	Set(IPLTagTemplateReadonlyRef(inOriginal));
	mGuid = inOriginal->mGuid;
}

void TagTemplate::Set(IPLTagTemplateReadonlyRef inOriginal)
{
	if (inOriginal && IPLTagTemplateReadonlyRef(this) != inOriginal)
	{
		mGuid = inOriginal->GetGUID();
		mName = inOriginal->GetName();
		mDimension.Set(inOriginal->GetColumns(), inOriginal->GetRows());
		mSize = inOriginal->GetSize();
		mMargin = inOriginal->GetMargin();
		mTagCellMap.clear();
		IPLTagCellReadonlyVector cells = inOriginal->GetTagCells();
		BOOST_FOREACH (const auto& cell, cells)
		{
			mTagCellMap[cell->GetGUID()] = cell->Clone();
		}
	}
}

PL::IPLTagTemplateWritableRef TagTemplate::FromFile(const dvacore::UTF16String& inPath, FileType inType)
{
	boost::property_tree::wptree propertyTree;
	DVA_ASSERT_MSG(inType == kFileType_JSON, "only support json format right now!");
	try
	{
		// Bug fix 3704993
		// Cannot pass file name directly to read_json because read_json only accept 
		// std::string as the file name which is not acceptable for double byte file name (such as Chinese)
		// so we need to generate stream with double byte file name firstly
		// Note: try to convert UTF16 file name to UTF8 and store it into std::string 
		// is not a good idea here, since Windows use GB2312 (by default) to show the file name 
		// in this way all UTF8 file name will be messy code (this is why the bug only reproduce on Win, 
		// since Linux & Mac use UTF8 file name)
		// Furthemore since file name created as UTF8, so search it by file name which is in UTF16
		// return incorrect result, this is also the root cause of bug 3704993
#ifdef DVA_OS_WIN
		std::basic_ifstream<boost::property_tree::wptree::key_type::value_type>
            stream(inPath.c_str());
#else
		std::basic_ifstream<boost::property_tree::wptree::key_type::value_type>
			stream(ASL::MakeStdString(inPath).c_str());
#endif
		boost::property_tree::read_json(stream, propertyTree);
		return FromPTree(propertyTree);
	}
	catch(...)
	{
		return PL::IPLTagTemplateWritableRef();
	}
}

PL::IPLTagTemplateWritableRef TagTemplate::FromContent(const dvacore::UTF16String& inContent, FileType inType)
{
	boost::property_tree::wptree propertyTree;
	DVA_ASSERT_MSG(inType == kFileType_JSON, "only support json format right now!");
	try
	{
		std::wstring contentStr = dvacore::utility::UTF16ToWstring(inContent);
		std::basic_istringstream<boost::property_tree::wptree::key_type::value_type> stream(contentStr.c_str());
		boost::property_tree::read_json(stream, propertyTree);
		return FromPTree(propertyTree);
	}
	catch(...)
	{
		return PL::IPLTagTemplateWritableRef();
	}
}

bool TagTemplate::ToFile(const dvacore::UTF16String& inPath, FileType inType)
{
	boost::property_tree::wptree propertyTree;
	ToPTree(propertyTree);
	DVA_ASSERT_MSG(inType == kFileType_JSON, "only support json format right now!");
	try
	{
		// Bug fix 3704993
		// See detail comments in FromFile function
#ifdef DVA_OS_WIN
		std::basic_ofstream<boost::property_tree::wptree::key_type::value_type>
            stream(inPath.c_str());
#else
		std::basic_ofstream<boost::property_tree::wptree::key_type::value_type>
			stream(ASL::MakeStdString(inPath).c_str());
#endif
		boost::property_tree::write_json(stream, propertyTree);
		return true;
	}
	catch(...)
	{
		return false;
	}
}

PL::IPLTagTemplateWritableRef TagTemplate::FromPTree(const boost::property_tree::wptree& inPTree)
{
	ASL::String version = WStringToUTF16(inPTree.get<std::wstring>(kKeyName_Version));
	if (version != ASL::MakeString(kTagTemplateVersion))
	{
		DVA_ASSERT_MSG(0, "Unknown tag template version! " << version);
		return PL::IPLTagTemplateWritableRef();
	}

	PL::TagTemplateRef tagTemplate = TagTemplate::CreateClassRef();
	tagTemplate->mGuid =  dvacore::utility::Coercion<dvacore::utility::Guid>::Result(
		WStringToUTF16(inPTree.get<std::wstring>(kKeyName_Guid)));

	auto optionalName = inPTree.get_optional<std::wstring>(kKeyName_Name);
	if (optionalName)
		tagTemplate->SetName(WStringToUTF16(*optionalName));

	bool needCalcDimension = false;
	auto optionalDimensionRows = inPTree.get_optional<ASL::UInt32>(kKeyName_DimensionRows);
	auto optionalDimensionColumns = inPTree.get_optional<ASL::UInt32>(kKeyName_DimensionColumns);
	if (optionalDimensionRows && optionalDimensionColumns)
		tagTemplate->SetDimension(*optionalDimensionColumns, *optionalDimensionRows);
	else
		needCalcDimension = true;

	auto optionalSizeWidth = inPTree.get_optional<std::int32_t>(kKeyName_SizeWidth);
	auto optionalSizeHeight = inPTree.get_optional<std::int32_t>(kKeyName_SizeHeight);
	if (optionalSizeWidth && optionalSizeHeight)
		tagTemplate->SetSize(dvacore::geom::Point32(*optionalSizeWidth, *optionalSizeHeight));

	auto optionalMarginWidth = inPTree.get_optional<std::int32_t>(kKeyName_MarginWidth);
	auto optionalMarginHeight = inPTree.get_optional<std::int32_t>(kKeyName_MarginHeight);
	if (optionalMarginWidth && optionalMarginHeight)
		tagTemplate->SetMargin(dvacore::geom::Point32(*optionalMarginWidth, *optionalMarginHeight));

	auto tags = inPTree.get_child_optional(kKeyName_Tags);
	if (tags)
	{
		ASL::UInt32 minRows = 0u;
		ASL::UInt32 minColumns = 0u;
		BOOST_FOREACH (const auto& keyValuePair, *tags)
		{
			TagCellRef tagCell = TagCell::CreateClassRef();
			auto& tagPTree = keyValuePair.second;
			tagCell->SetGUID(dvacore::utility::Coercion<dvacore::utility::Guid>::Result(
				WStringToUTF16(tagPTree.get<std::wstring>(kKeyName_Guid))));
			// Should after SetGUID
			tagTemplate->InsertTagCell(IPLTagCellReadonlyRef(tagCell));

			auto optionalName = tagPTree.get_optional<std::wstring>(kKeyName_Name);
			if (optionalName)
				tagCell->SetName(WStringToUTF16(*optionalName));

			auto optionalShortcut_vkey = tagPTree.get_optional<ASL::UInt32>(kKeyName_ShortcutVKey);
			auto optionalShortcut_ctrl = tagPTree.get_optional<bool>(kKeyName_ShortcutCtrl);
			auto optionalShortcut_alt = tagPTree.get_optional<bool>(kKeyName_ShortcutAlt);
			auto optionalShortcut_shift = tagPTree.get_optional<bool>(kKeyName_ShortcutShift);
#if ASL_TARGET_OS_MAC
			auto optionalShortcut_macctrl = tagPTree.get_optional<bool>(kKeyName_ShortcutMacCtrl);
			if (optionalShortcut_vkey && optionalShortcut_ctrl && optionalShortcut_alt && optionalShortcut_shift && optionalShortcut_macctrl)
				tagCell->SetShortcut(UIF::KeySpec(*optionalShortcut_vkey, *optionalShortcut_ctrl, *optionalShortcut_alt, *optionalShortcut_shift, *optionalShortcut_macctrl));
#else
			if (optionalShortcut_vkey && optionalShortcut_ctrl && optionalShortcut_alt && optionalShortcut_shift)
				tagCell->SetShortcut(UIF::KeySpec(*optionalShortcut_vkey, *optionalShortcut_ctrl, *optionalShortcut_alt, *optionalShortcut_shift));
#endif
			
				

			auto optionalPayload = tagPTree.get_optional<std::wstring>(kKeyName_Payload);
			if (optionalPayload)
				tagCell->SetPayload(WStringToUTF16(*optionalPayload));
			
			auto duration = tagPTree.get_optional<std::wstring>(kKeyName_Duration);
			if (duration)
				tagCell->SetDuration(dvacore::utility::Coercion<std::size_t>::Result(WStringToUTF16(*duration)));
			
			auto optionalColorRed = tagPTree.get_optional<float>(kKeyName_ColorRed);
			auto optionalColorGreen = tagPTree.get_optional<float>(kKeyName_ColorGreen);
			auto optionalColorBlue = tagPTree.get_optional<float>(kKeyName_ColorBlue);
			if (optionalColorRed && optionalColorGreen && optionalColorBlue)
				tagCell->SetColor(dvaui::drawbot::ColorRGBA(*optionalColorRed, *optionalColorGreen, *optionalColorBlue));

			dvacore::geom::Rect32 position;
			auto optionalPosLeftColumn = tagPTree.get_optional<std::int32_t>(kKeyName_PositonLeftColumn);
			if (optionalPosLeftColumn)
				position.SetLeft(*optionalPosLeftColumn);
			auto optionalPosTowRow = tagPTree.get_optional<std::int32_t>(kKeyName_PositonTopRow);
			if (optionalPosTowRow)
				position.SetTop(*optionalPosTowRow);
			auto optionalPosRows = tagPTree.get_optional<std::int32_t>(kKeyName_PositonRows);
			position.SetHeight(optionalPosRows ? *optionalPosRows : 1);
			auto optionalPosColumns = tagPTree.get_optional<std::int32_t>(kKeyName_PositonColumns);
			position.SetWidth(optionalPosColumns ? *optionalPosColumns : 1);
			if (!position.Empty())
			{
				minRows = std::max(minRows, (ASL::UInt32)position.Bottom());
				minColumns = std::max(minColumns, (ASL::UInt32)position.Right());
				tagCell->SetPosition(position);
			}
		}

		if (needCalcDimension)
			tagTemplate->SetDimension(minColumns, minRows);
	}

	return PL::IPLTagTemplateWritableRef(tagTemplate);
}

/*
**
*/
void TagTemplate::ToPTree(boost::property_tree::wptree& outPTree)
{
	outPTree.put(kKeyName_Version, kTagTemplateVersion);
	outPTree.put(kKeyName_Guid, UTF16ToWString(GetGUID().AsString()));
	outPTree.put(kKeyName_Name, UTF16ToWString(GetName()));
	outPTree.put(kKeyName_DimensionRows, GetRows());
	outPTree.put(kKeyName_DimensionColumns, GetColumns());
	outPTree.put(kKeyName_SizeWidth, GetSize().x);
	outPTree.put(kKeyName_SizeHeight, GetSize().y);
	outPTree.put(kKeyName_MarginWidth, GetMargin().x);
	outPTree.put(kKeyName_MarginHeight, GetMargin().y);

	boost::property_tree::wptree tags;
	PL::IPLTagCellReadonlyVector tagCells = GetTagCells();
	BOOST_FOREACH(PL::IPLTagCellReadonlyRef cell, tagCells)
	{
		boost::property_tree::wptree tag;
		tag.put(kKeyName_Guid, UTF16ToWString(cell->GetGUID().AsString()));
		tag.put(kKeyName_Name, UTF16ToWString(cell->GetName()));
		tag.put(kKeyName_Payload, UTF16ToWString(cell->GetPayload()));
		tag.put(kKeyName_Duration, UTF16ToWString(dvacore::utility::Coercion<dvacore::UTF16String>::Result(cell->GetDuration())));
		tag.put(kKeyName_ColorRed, cell->GetColor().Red());
		tag.put(kKeyName_ColorGreen, cell->GetColor().Green());
		tag.put(kKeyName_ColorBlue, cell->GetColor().Blue());
		tag.put(kKeyName_PositonLeftColumn, cell->GetPosition().Left());
		tag.put(kKeyName_PositonTopRow, cell->GetPosition().Top());
		tag.put(kKeyName_PositonRows, cell->GetPosition().Height());
		tag.put(kKeyName_PositonColumns, cell->GetPosition().Width());
		if (cell->GetShortcut().HasShortcut())
		{
			tag.put(kKeyName_ShortcutVKey, cell->GetShortcut().GetKey());
			tag.put(kKeyName_ShortcutCtrl, cell->GetShortcut().Control());
			tag.put(kKeyName_ShortcutAlt, cell->GetShortcut().Alt());
			tag.put(kKeyName_ShortcutShift, cell->GetShortcut().Shift());
#if ASL_TARGET_OS_MAC
			tag.put(kKeyName_ShortcutMacCtrl, cell->GetShortcut().MacControl());
#endif
		}

		tags.push_back(std::make_pair(std::wstring(), tag));
	}
	outPTree.put_child(kKeyName_Tags, tags);
}

bool TagTemplate::HasDuplicatedShortcuts() const
{
	PL::IPLTagCellReadonlyVector tagCells = GetTagCells();
	IPLTagCellReadonlyVector duplicatedTagCells;
	BOOST_FOREACH(PL::IPLTagCellReadonlyRef cell, tagCells)
	{
		if (HasDuplicatedShortcutsForTag(cell, duplicatedTagCells))
		{
			return true;
		}
	}

	return false;
}

bool TagTemplate::HasDuplicatedShortcutsForTag(IPLTagCellReadonlyRef inCell, IPLTagCellReadonlyVector& outDuplicatedTagCells) const
{
	bool hasDupShortcuts = false;
	UIF::KeySpec shortcut = inCell->GetShortcut();
	PL::IPLTagCellReadonlyVector tagCells = GetTagCells();
	BOOST_FOREACH(PL::IPLTagCellReadonlyRef cell, tagCells)
	{
		if (shortcut.HasShortcut() && shortcut == cell->GetShortcut() && inCell->GetPosition() != cell->GetPosition())
		{
			hasDupShortcuts = true;
			outDuplicatedTagCells.push_back(cell);
		}
	}

	if (hasDupShortcuts)
	{
		outDuplicatedTagCells.push_back(inCell);
	}

	return hasDupShortcuts;
}

} // namespace PL
