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

#ifndef IPLTAGCELL_H
#define IPLTAGCELL_H

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

#ifndef DVAUI_DRAWBOT_DRAWBOTTYPES_H
#include "dvaui/drawbot/DrawbotTypes.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef UIFKEYSPEC_H
#include "UIFKeySpec.h"
#endif

namespace PL
{
enum TagCellPositionType {kTagCellPositionType_Cells};

// {6569F36B-D631-40B7-99BF-FCD159D9FA21}
ASL_DEFINE_IID(IPLTagCellReadonly, 0x6569f36b, 0xd631, 0x40b7, 0x99, 0xbf, 0xfc, 0xd1, 0x59, 0xd9, 0xfa, 0x21);
ASL_DEFINE_IREF(IPLTagCellReadonly);

ASL_INTERFACE IPLTagCellReadonly : public ASLUnknown
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
	virtual ASL::String GetPayload() const = 0;

	/**
	**
	*/
	virtual dvaui::drawbot::ColorRGBA GetColor() const = 0;
	
	/**
	**	kTagCellPositionType_Cells: rect.left means the included left cell column, rect.top means the included top row
	**								rect.width means included columns, rect.height means included rows
	*/
	virtual TagCellPositionType GetPositionType() const = 0;

	/**
	**
	*/
	virtual dvacore::geom::Rect32 GetPosition() const = 0;

	/**
	**
	*/
	virtual UIF::KeySpec GetShortcut() const =0;

	/**
	**	Clone will generate a new tag cell with same GUID and same content.
	*/
	virtual IPLTagCellReadonlyRef Clone() const = 0;

	/**
	**
	*/
	virtual std::size_t GetDuration() const = 0;
};

// {97CBD263-855B-4060-A91F-8217E2F08A93}
ASL_DEFINE_IID(IPLTagCellWritable, 0x97cbd263, 0x855b, 0x4060, 0xa9, 0x1f, 0x82, 0x17, 0xe2, 0xf0, 0x8a, 0x93);
ASL_DEFINE_IREF(IPLTagCellWritable);

ASL_INTERFACE IPLTagCellWritable : public ASLUnknown
{
public:
	/**
	**
	*/
	virtual void SetName(const ASL::String& inName) = 0;

	/**
	**
	*/
	virtual void SetPayload(const ASL::String& inPayload) = 0;

	/**
	**
	*/
	virtual void SetColor(const dvaui::drawbot::ColorRGBA& inColor) = 0;
	
	/**
	**
	*/
	virtual void SetPositionType(TagCellPositionType inType) = 0;

	/**
	**
	*/
	virtual void SetPosition(const dvacore::geom::Rect32& inRect) = 0;

	/**
	**
	*/
	virtual void SetShortcut(const UIF::KeySpec& inKey) = 0;

	/**
	**	Set with same content, except GUID.
	*/
	virtual void Set(IPLTagCellReadonlyRef inOriginal) = 0;

	/**
	**
	*/
	virtual void SetDuration(std::size_t inDuration) = 0;
};

/**
**	Default tag cell has (0,0,0,0) position, black color, empty name and payload, empty shortcut.
*/
PL_EXPORT
IPLTagCellWritableRef CreateDefaultTagCell();
} // namespace PL

#endif
