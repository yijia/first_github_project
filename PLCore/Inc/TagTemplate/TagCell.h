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

#ifndef TAGCELL_H
#define TAGCELL_H

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

#ifndef IPLTAGCELL_H
#include "TagTemplate/IPLTagCell.h"
#endif

namespace PL
{

ASL_DEFINE_CLASSREF(TagCell, IPLTagCellReadonly);
// {B0E3CB4B-5C44-4269-8959-82BC51DF92C9}
ASL_DEFINE_IMPLID(TagCell, 0xb0e3cb4b, 0x5c44, 0x4269, 0x89, 0x59, 0x82, 0xbc, 0x51, 0xdf, 0x92, 0xc9);

class TagCell : public IPLTagCellReadonly, public IPLTagCellWritable
{
	ASL_STRONGWEAK_CLASS_SUPPORT(TagCell);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLTagCellReadonly)
		ASL_QUERY_ENTRY(IPLTagCellWritable)
		ASL_QUERY_ENTRY(TagCell)
	ASL_QUERY_MAP_END

public:
	TagCell();

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
	virtual ASL::String GetPayload() const;

	/**
	**
	*/
	virtual dvaui::drawbot::ColorRGBA GetColor() const;
	
	/**
	**
	*/
	virtual TagCellPositionType GetPositionType() const;

	/**
	**
	*/
	virtual dvacore::geom::Rect32 GetPosition() const;

	/**
	**
	*/
	virtual UIF::KeySpec GetShortcut() const;

	/**
	**
	*/
	virtual std::size_t GetDuration() const;


	/**
	**
	*/
	IPLTagCellReadonlyRef Clone() const;

	/**
	**
	*/
	virtual void SetName(const ASL::String& inName);

	/**
	**
	*/
	virtual void SetPayload(const ASL::String& inPayload);

	/**
	**
	*/
	virtual void SetColor(const dvaui::drawbot::ColorRGBA& inColor);
	
	/**
	**
	*/
	virtual void SetPositionType(TagCellPositionType inType);

	/**
	**
	*/
	virtual void SetPosition(const dvacore::geom::Rect32& inRect);

	/**
	**
	*/
	virtual void SetShortcut(const UIF::KeySpec& inKey);

	virtual void Set(IPLTagCellReadonlyRef inOriginal);

	/**
	**	Only used by serialization.
	*/
	void SetGUID(const dvacore::utility::Guid& inGuid);

	/**
	**
	*/
	virtual void SetDuration(std::size_t inDuration);

	void CloneInit(const TagCell* inOriginal);

private:
	dvacore::utility::Guid		mGuid;
	TagCellPositionType			mPositionType;
	dvacore::geom::Rect32		mPosition;
	dvaui::drawbot::ColorRGBA	mColor;
	ASL::String					mName;
	ASL::String					mPayload;
	std::size_t					mDuration; // Format is frames
	UIF::KeySpec				mShortcut;
};

} // namespace PL

#endif
