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

#pragma once

#ifndef IPLPRIMARYCLIP_H
#define IPLPRIMARYCLIP_H

// BE
#ifndef BE_CLIP_IMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

// ASL
#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

// local

#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

#ifndef UIFMESSAGEBOX_H
#include "UIFMessageBox.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

namespace PL
{

ASL_DEFINE_IREF(ISRPrimaryClip);
ASL_DEFINE_IID(ISRPrimaryClip, 0xe898fd2f, 0x1ec4, 0x457a, 0x9e, 0x92, 0x8d, 0x45, 0x3, 0xfc, 0xe0, 0xff);

typedef std::list<ISRPrimaryClipRef> SRPrimaryClipList;

	
ASL_INTERFACE ISRPrimaryClip : public ASLUnknown
{
public:

	typedef boost::function<void(PL::AssetItemPtr const&, PL::AssetMediaInfoPtr const&, ASL::Result, ASL::String const&)> SaveCallBackFn;

	/*
	**
	*/
	virtual PL::AssetItemPtr GetAssetItem() const =0;

	/*
	**
	*/
	virtual PL::AssetMediaInfoPtr GetAssetMediaInfo() const =0;

	/*
	**
	*/
	virtual bool Relink(
				PL::AssetItemPtr const& inRelinkAssetItem) = 0;
	
	/*
	**
	*/
	virtual UIF::MBResult::Type Save(bool isSilent, bool isCloseOrExit) =0;

	/*
	**
	*/
	virtual UIF::MBResult::Type Save(bool isSilent, bool isCloseOrExit, SaveCallBackFn const& inSaveCallBackFn) =0;

	/*
	**
	*/
	virtual ASL::PathnameList GetReferencedMediaPath() = 0;
	
	/*
	**
	*/
	virtual bool IsDirty() const =0;
	
	/*
	** [TRICKY] Please be careful to use this method. It's a temporary situation for dirty flag clear when all actions are undone.
	**
	*/
	virtual void SetDirty(bool inIsDirty) =0;

	/*
	**
	*/
	virtual bool AttachToTimeLine() =0;

	/*
	**
	*/
	virtual bool DetachFromTimeLine() =0;
	
	/*
	**
	*/
	virtual bool CreateSequence() =0;	
	
	/*
	**
	*/
	virtual dvacore::UTF16String GetName() const = 0;
	
	/**
	**
	*/
	virtual bool IsWritable() const = 0;

	/**
	**
	*/
	virtual bool IsOffline() const = 0;
};

} // namespace PL

#endif
