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

#pragma once

#ifndef PLCLIPITEM_H
#define PLCLIPITEM_H

// BE
#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BECLIPFWD_H
#include "BEClipFwd.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

// ASL
#ifndef ASLTIMECODE_H
#include "ASLTimecode.h"
#endif

#ifndef ASLOBJECTPTR_H
#include "ASLObjectPtr.h"
#endif

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef IPLMEDIA_H
#include "IPLMedia.h"
#endif


namespace PL
{

class SRClipItem;
	
typedef boost::shared_ptr<SRClipItem> SRClipItemPtr;
typedef std::list<SRClipItemPtr> SRClipItems;

class SRClipItem
{
public:

	/*
	**
	*/
	PL_EXPORT
	SRClipItem(PL::AssetItemPtr const& inAssetItem);

	/*
	**
	*/
	PL_EXPORT
	SRClipItem(ISRMediaRef inSRMedia, AssetItemPtr const& inAssetItem);

	/*
	**
	*/
	PL_EXPORT
	~SRClipItem();

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetStartTime(bool ignoreHardBoundaries = false) const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetDuration() const;

	/*
	**
	*/
	PL_EXPORT
	ISRMediaRef GetSRMedia() const;

	/*
	**
	*/
	PL_EXPORT
	PL::AssetItemPtr GetAssetItem() const;

	/*
	**
	*/
	PL_EXPORT
	BE::IMasterClipRef GetMasterClip() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::Guid GetSubClipGUID() const;

	/*
	**
	*/
	PL_EXPORT
	void SetSubClipGUID(ASL::Guid inGuid);

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetSubClipName() const;

	/*
	**
	*/
	PL_EXPORT
	BE::ITrackItemRef GetTrackItem() const;

	/*
	**
	*/
	PL_EXPORT
	void SetTrackItem(BE::ITrackItemRef inTrackItem);

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetOriginalClipPath() const;

private:
	ISRMediaRef						mSRMedia;

	PL::AssetItemPtr				mAssetItem;

	BE::ITrackItemRef				mTrackItem;
	ASL::Guid						mGUID;
};

}

#endif
