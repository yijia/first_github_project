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
#include "TagTemplate/TagCell.h"

namespace PL
{
	TagCell::TagCell()
		: mGuid(dvacore::utility::Guid::CreateUnique())
		, mColor(dvaui::drawbot::ColorRGBA(0.f, 0.f, 0.f, 0.f))
		, mPositionType(kTagCellPositionType_Cells)
		, mShortcut(UIF::KeySpec())
		, mDuration(1)
	{

	}

	dvacore::utility::Guid TagCell::GetGUID() const
	{
		return mGuid;
	}

	ASL::String TagCell::GetName() const
	{
		return mName;
	}

	ASL::String TagCell::GetPayload() const
	{
		return mPayload;
	}

	dvaui::drawbot::ColorRGBA TagCell::GetColor() const
	{
		return mColor;
	}

	TagCellPositionType TagCell::GetPositionType() const
	{
		return kTagCellPositionType_Cells;
	}

	dvacore::geom::Rect32 TagCell::GetPosition() const
	{
		return mPosition;
	}

	UIF::KeySpec TagCell::GetShortcut() const
	{
		return mShortcut;
	}

	std::size_t TagCell::GetDuration() const
	{
		return mDuration;
	}

	IPLTagCellReadonlyRef TagCell::Clone() const
	{
		TagCellRef cloneInstance = CreateClassRef();
		cloneInstance->CloneInit(this);
		return IPLTagCellReadonlyRef(cloneInstance);
	}

	void TagCell::CloneInit(const TagCell* inOriginal)
	{
		Set(IPLTagCellReadonlyRef(inOriginal));
		// keep same GUID
		mGuid = inOriginal->mGuid;
	}

	void TagCell::SetPosition( const dvacore::geom::Rect32& inPosition )
	{
		mPosition = inPosition;
	}

	void TagCell::SetColor( const dvaui::drawbot::ColorRGBA& inColor )
	{
		mColor = inColor;
	}

	void TagCell::SetName( const ASL::String& inName )
	{
		mName = inName;
	}

	void TagCell::SetPayload( const ASL::String& inPayload )
	{
		mPayload = inPayload;
	}

	void TagCell::SetPositionType( TagCellPositionType inType )
	{
		mPositionType = inType;
	}

	void TagCell::SetShortcut( const UIF::KeySpec& inKey )
	{
		mShortcut = inKey;
	}

	void TagCell::Set(IPLTagCellReadonlyRef inOriginal)
	{
		if (inOriginal && IPLTagCellReadonlyRef(this) != inOriginal)
		{
			mPositionType = inOriginal->GetPositionType();
			mPosition = inOriginal->GetPosition();
			mColor = inOriginal->GetColor();
			mName = inOriginal->GetName();
			mPayload = inOriginal->GetPayload();
			mDuration = inOriginal->GetDuration();
			mShortcut = inOriginal->GetShortcut();
		}
	}

	void TagCell::SetGUID( const dvacore::utility::Guid& inGuid )
	{
		mGuid = inGuid;
	}

	IPLTagCellWritableRef CreateDefaultTagCell()
	{
		return IPLTagCellWritableRef(TagCell::CreateClassRef());
	}

	void TagCell::SetDuration(std::size_t inDuration)
	{
		mDuration = inDuration;
	}

} // namespace PL