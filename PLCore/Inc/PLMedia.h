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

#ifndef PLMEDIA_H
#define PLMEDIA_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

// DVA
#ifndef DVACORE_UTILITY_SHAREDREFPTR_H
#include "dvacore/utility/SharedRefPtr.h"
#endif

// BE
#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BECLIPFWD_H
#include "BEClipFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEM_H
#include "BE/Project/IProjectItem.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

#ifndef BEORPHANITEM_H
#include "BEOrphanItem.h"
#endif 

#ifndef BE_MEDIA_IMEDIAFWD_H
#include "BE/Media/IMediaFwd.h"
#endif

// ASL
#ifndef ASLTIMECODE_H
#include "ASLTimecode.h"
#endif

// MZ
#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef PLMARKEROWNER_H
#include "PLMarkerOwner.h"
#endif

#ifndef IPLMEDIA_H
#include "IPLMedia.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

namespace PL
{

ASL_DEFINE_CLASSREF(SRMedia, ISRMarkerOwner);
ASL_DEFINE_IMPLID(SRMedia, 0xac4c9b2b, 0xcee5, 0x447d, 0xb8, 0x7b, 0xdb, 0x98, 0x2f, 0xbd, 0x00, 0x5c);
    
class SRMedia 
	:
    public ISRMedia,
    public SRMarkerOwner,
	public ASL::Listener
{
    ASL_STRONGWEAK_CLASS_SUPPORT(SRMedia);
    ASL_QUERY_MAP_BEGIN
    ASL_QUERY_ENTRY(ISRMedia)
    ASL_QUERY_CHAIN(SRMarkerOwner)
    ASL_QUERY_MAP_END
    
public:
	
	ASL_MESSAGE_MAP_DECLARE();

    /*
    **
    */
	SRMedia();
    
	/*
    **
    */
	~SRMedia();
    
	/*
	**
	*/
	static ISRMediaRef Create(ASL::String const& inClipFilePath);

	/*
	**
	*/
	BE::IMasterClipRef GetMasterClip() const;

	/*
	**
	*/
	ASL::String GetClipFilePath() const;

	/*
	**
	*/
	bool IsDirty() const;

	/**
	**
	*/
	bool IsWritable() const;

	/**
	**	Called if PL found save failure or read-only changes after resume.
	*/
	void UpdateWritableState(bool inIsWritable);

	/*
	**
	*/
	bool IsOffLine() const;

	/*
	**
	*/
	void SetDirty(bool inIsDirty);

	/*
	**
	*/
	void RefreshOfflineMedia();

	/*
	**
	*/
	virtual ASL::StationID GetStationID() const;

private:

	/*
	**
	*/
	void Initialize(ASL::String const& inClipFilePath);

	/*
	**
	*/
	void OnMasterClipClipChanged(BE::IMasterClipRef inMasterClip);

	/*
	**
	*/
	void OnMetadataSave();

	/*
	**
	*/
	void OnMediaMetaDataMarkerNotInSync(ASL::String const& inMediaLocator);

	/*
	**
	*/
	void OnMediaInfoChanged();

	void OnMediaChanged(
		const BE::IMediaRef& inMedia);

	/*
	**
	*/
	void TryCacheProjectItem() const;
	
private:
    static BE::IOrphanItemRef       sOrphanItems;
	ASL::String                     mClipFilePath;
	bool                            mIsDirty;
	bool							mIsWritable;
	ASL::StationID                  mStationID;

	//	In Local mode, This projectItem directly comes from Project
	//	In EA mode, This projectItem is cloned from ProjectItem of current project and their EAAssetID property is same
	mutable BE::IProjectItemRef     mProjectItem;
    friend class SRProject;
};

}

#endif
