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

#ifndef IPLROUGHCUT_H
#define IPLROUGHCUT_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef BE_SEQUENCE_ITRACKITEM_H
#include "BE/Sequence/ITrackItem.h"
#endif

namespace PL
{

ASL_DEFINE_IREF(ISRRoughCut);
ASL_DEFINE_IID(ISRRoughCut, 0xa9fad902, 0xafdb, 0x471e, 0xae, 0x41, 0xc2, 0xc2, 0xc6, 0xe2, 0xb8, 0x23);

ASL_INTERFACE ISRRoughCut : public ASLUnknown
{
	/*
	** 
	*/
	virtual SRClipItems GetClipItems() const = 0;

	/*
	**
	*/
	virtual bool AppendClipItems(SRClipItems& ioClipItems) =0;

	/*
	**
	*/
	virtual bool AddClipItems(SRClipItems& inClipItems, dvamediatypes::TickTime inInsertTime, dvamediatypes::TickTime* outChangedDuration) =0;

	/*
	**
	*/
	virtual bool RemoveClipItem(const BE::ITrackItemRef inTrackItem) =0;

	/*
	**
	*/
	virtual bool RelinkClipItem(SRClipItems& inRelinkedClipItems) =0;

	/*
	**
	*/
	virtual SRClipItemPtr GetSRClipItem(const BE::ITrackItemRef inTrackItem) const = 0;

	/*
	**	
	*/
	virtual dvacore::UTF16String GetFilePath()const = 0;

	/*
	**	
	*/
	virtual bool TestIfRoughCutChanged() = 0;

	/*
	**	
	*/
	virtual bool IsDirtyChild(BE::IMasterClipRef inMasterClip) const = 0;

	/*
	**
	*/
	virtual void ResetRoughtClipItemFromSequence() = 0;

	/*
	**
	*/
	virtual bool Publish() = 0;

	/*
	**
	*/
	virtual bool RemoveSelectedTrackItem(bool inDoRipple, bool inAlignToVideo) = 0;

	/*
	**
	*/
	virtual void RefreshRoughCut(PL::AssetItemPtr const& inRCAssetItem) = 0;

};

} // namespace PL

#endif
