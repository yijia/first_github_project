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

#ifndef TAGTEMPLATE_H
#define TAGTEMPLATE_H


#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

#ifndef DVACORE_UTILITY_GUID_H
#include "dvacore/utility/Guid.h"
#endif

#ifndef DVACORE_GEOM_POINT_H
#include "dvacore/geom/Point.h"
#endif

#ifndef DVACORE_GEOM_RECT_H
#include "dvacore/geom/Rect.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef IPLTAGTEMPLATE_H
#include "TagTemplate/IPLTagTemplate.h"
#endif

#ifndef BOOST_PROPERTY_TREE_PTREE_HPP_INCLUDED
#include "boost/property_tree/ptree.hpp"
#endif

namespace PL
{

ASL_DEFINE_CLASSREF(TagTemplate, IPLTagTemplateReadonly);
// {107E9E74-0BCD-4EA9-BC0C-C9EC345B64DB}
ASL_DEFINE_IMPLID(TagTemplate, 0x107e9e74, 0xbcd, 0x4ea9, 0xbc, 0xc, 0xc9, 0xec, 0x34, 0x5b, 0x64, 0xdb);

class TagTemplate : public IPLTagTemplateReadonly, public IPLTagTemplateWritable, public IPLTagTemplateSearch
{
	ASL_STRONGWEAK_CLASS_SUPPORT(TagTemplate);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLTagTemplateReadonly)
		ASL_QUERY_ENTRY(IPLTagTemplateWritable)
		ASL_QUERY_ENTRY(IPLTagTemplateSearch)
		ASL_QUERY_ENTRY(TagTemplate)
	ASL_QUERY_MAP_END

public:
	TagTemplate();

	/**
	**
	*/
	virtual dvacore::utility::Guid GetGUID() const;

	/**
	**
	*/
	virtual ASL::String GetName() const;

	/**
	**
	*/
	virtual ASL::UInt32 GetRows() const;

	/**
	**
	*/
	virtual ASL::UInt32 GetColumns() const;

	/**
	**	base on pixel
	*/
	virtual dvacore::geom::Point32 GetSize() const;
	
	/**
	**	base on pixel
	*/
	virtual dvacore::geom::Point32 GetMargin() const;

	/**
	**
	*/
	virtual IPLTagCellReadonlyVector GetTagCells() const;

	/**
	**
	*/
	virtual void SetName(const ASL::String& inName);

	/**
	**
	*/
	virtual IPLTagCellReadonlyVector SetDimension(ASL::UInt32 inColumns, ASL::UInt32 inRows);

	/**
	**	base on pixel
	*/
	virtual void SetSize(const dvacore::geom::Point32& inSize);
	
	/**
	**	base on pixel
	*/
	virtual void SetMargin(const dvacore::geom::Point32& inMargin);

	/**
	**	If the position of cell can't be contained by current dimension, insert will fail.
	*/
	virtual bool InsertTagCell(IPLTagCellReadonlyRef inCell);

	/**
	**
	*/
	virtual void RemoveTagCell(IPLTagCellReadonlyRef inCell);

	virtual IPLTagCellReadonlyRef FindTagByGUID(const dvacore::utility::Guid& inGuid) const;

	virtual IPLTagCellReadonlyRef FindTagByPosition(const dvacore::geom::Point32& inPosition) const;

	IPLTagTemplateReadonlyRef Clone() const;

	void Set(IPLTagTemplateReadonlyRef inOriginal);

	enum FileType{kFileType_JSON};
	static
	PL::IPLTagTemplateWritableRef FromFile(const dvacore::UTF16String& inPath, FileType inType = kFileType_JSON);
	static
	PL::IPLTagTemplateWritableRef FromContent(const dvacore::UTF16String& inContent, FileType inType = kFileType_JSON);

	bool ToFile(const dvacore::UTF16String& inPath, FileType inType = kFileType_JSON);

	void CloneInit(const TagTemplate* inOriginal);

	virtual bool HasDuplicatedShortcuts() const;

	virtual bool HasDuplicatedShortcutsForTag(IPLTagCellReadonlyRef inCell, IPLTagCellReadonlyVector& outDuplicatedTagCells) const;

private:
	static
	PL::IPLTagTemplateWritableRef FromPTree(const boost::property_tree::wptree& inPTree);

	void ToPTree(boost::property_tree::wptree& outPTree);

private:
	typedef std::map<dvacore::utility::Guid, IPLTagCellReadonlyRef> TagCellMap;

	dvacore::utility::Guid		mGuid;
	dvacore::UTF16String		mName;
	dvacore::geom::Point32		mDimension;
	dvacore::geom::Point32		mSize;
	dvacore::geom::Point32		mMargin;
	TagCellMap					mTagCellMap;
};

} // namespace PL

#endif
