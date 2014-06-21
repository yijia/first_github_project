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

#ifndef IPLTAGTEMPLATE_H
#define IPLTAGTEMPLATE_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
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

#ifndef IPLTAGCELL_H
#include "IPLTagCell.h"
#endif

namespace PL
{
typedef std::vector<IPLTagCellReadonlyRef> IPLTagCellReadonlyVector;
typedef std::vector<IPLTagCellWritableRef> IPLTagCellWritableVector;

// {B07D13FF-965B-4BE3-92ED-53F0C4691B8E}
ASL_DEFINE_IID(IPLTagTemplateReadonly, 0xb07d13ff, 0x965b, 0x4be3, 0x92, 0xed, 0x53, 0xf0, 0xc4, 0x69, 0x1b, 0x8e);
ASL_DEFINE_IREF(IPLTagTemplateReadonly);

ASL_INTERFACE IPLTagTemplateReadonly : public ASLUnknown
{
public:
	/**
	**
	*/
	virtual dvacore::utility::Guid GetGUID() const = 0;

	/**
	**
	*/
	virtual ASL::String GetName() const = 0;

	/**
	**
	*/
	virtual ASL::UInt32 GetRows() const = 0;

	/**
	**
	*/
	virtual ASL::UInt32 GetColumns() const = 0;

	/**
	**	base on pixel
	*/
	virtual dvacore::geom::Point32 GetSize() const = 0;
	
	/**
	**	base on pixel
	*/
	virtual dvacore::geom::Point32 GetMargin() const = 0;

	/**
	**
	*/
	virtual IPLTagCellReadonlyVector GetTagCells() const = 0;

	/**
	**	Clone will generate a new tag cell with same GUID and same content.
	*/
	virtual IPLTagTemplateReadonlyRef Clone() const = 0;

	/**
	**	Check duplicated shortcut existence
	*/
	virtual bool HasDuplicatedShortcuts() const = 0;

	/**
	**	Check whether duplicated tags with same shortcut existing for given tag
	*/
	virtual bool HasDuplicatedShortcutsForTag(IPLTagCellReadonlyRef inCell, IPLTagCellReadonlyVector& outDuplicatedTagCells) const = 0;
};

// {7EB2F048-A500-4128-A598-D76F9A90DA9C}
ASL_DEFINE_IID(IPLTagTemplateWritable, 0x7eb2f048, 0xa500, 0x4128, 0xa5, 0x98, 0xd7, 0x6f, 0x9a, 0x90, 0xda, 0x9c);
ASL_DEFINE_IREF(IPLTagTemplateWritable);

ASL_INTERFACE IPLTagTemplateWritable : public ASLUnknown
{
public:
	/**
	**
	*/
	virtual void SetName(const ASL::String& inName) = 0;

	/**
	**	Return all tag cells which have been removed from tag template.
	*/
	virtual IPLTagCellReadonlyVector SetDimension(ASL::UInt32 inColumns, ASL::UInt32 inRows) = 0;

	/**
	**	base on pixel
	*/
	virtual void SetSize(const dvacore::geom::Point32& inSize) = 0;
	
	/**
	**	base on pixel
	*/
	virtual void SetMargin(const dvacore::geom::Point32& inMargin) = 0;

	/**
	**	If the position of cell can't be contained by current dimension, insert will fail.
	*/
	virtual bool InsertTagCell(IPLTagCellReadonlyRef inCell) = 0;

	/**
	**
	*/
	virtual void RemoveTagCell(IPLTagCellReadonlyRef inCell) = 0;

	/**
	**	Set with same content, except GUID.
	*/
	virtual void Set(IPLTagTemplateReadonlyRef inOriginal) = 0;
};

// {2E4D9093-5FE6-48B2-9426-AE206DCDABEC}
ASL_DEFINE_IID(IPLTagTemplateSearch, 0x2e4d9093, 0x5fe6, 0x48b2, 0x94, 0x26, 0xae, 0x20, 0x6d, 0xcd, 0xab, 0xec);
ASL_DEFINE_IREF(IPLTagTemplateSearch);

ASL_INTERFACE IPLTagTemplateSearch : public ASLUnknown
{
public:
	/**
	**
	*/
	virtual IPLTagCellReadonlyRef FindTagByGUID(const dvacore::utility::Guid& inGuid) const = 0;

	/**
	**
	*/
	virtual IPLTagCellReadonlyRef FindTagByPosition(const dvacore::geom::Point32& inPosition) const = 0;
};

PL_EXPORT
IPLTagTemplateReadonlyRef CreateDefaultTagTemplate(const ASL::String& inName = ASL::String());

PL_EXPORT
IPLTagTemplateReadonlyRef LoadTagTemplate(const ASL::String& inPath);

PL_EXPORT
IPLTagTemplateReadonlyRef BuildTagTemplate(const ASL::String& inContent);

PL_EXPORT
bool SaveTagTemplate(const ASL::String& inPath, IPLTagTemplateReadonlyRef inTagTemplate);

} // namespace PL

#endif
