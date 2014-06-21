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

#ifndef PLUNASSOCIATEDMETADATA_H
#define PLUNASSOCIATEDMETADATA_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef IPLUNASSOCIATEDMARKERS_H
#include "IPLUnassociatedMarkers.h"
#endif

// DVA
#ifndef DVACORE_UTILITY_SHAREDREFPTR_H
#include "dvacore/utility/SharedRefPtr.h"
#endif

// BE
#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

// ASL
#ifndef ASLTIMECODE_H
#include "ASLTimecode.h"
#endif

#ifndef ASLDATE_H
#include "ASLDate.h"
#endif

// PL
#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif


namespace PL
{

class SRUnassociatedMetadata;
DVA_UTIL_TYPEDEF_SHARED_REF_PTR(SRUnassociatedMetadata) SRUnassociatedMetadataRef;
typedef std::list<SRUnassociatedMetadataRef> SRUnassociatedMetadataList;
	
class SRUnassociatedMetadata : public dvacore::utility::RefCountedObject
{
private:
	SRUnassociatedMetadata();
	~SRUnassociatedMetadata();
	
public:
	
	DVA_UTIL_TYPEDEF_SHARED_REF_PTR(SRUnassociatedMetadata) SharedPtr;

	enum OffsetType
	{
		OffsetType_NoneOffset,
		OffsetType_Timecode,
		OffsetType_Date
	};
	
	/*
	**
	*/
	static SRUnassociatedMetadataRef Create(
		const ASL::String& inMetadataID,
		const ASL::String& inMetadataPath);

	/*
	**
	*/
	static SRUnassociatedMetadataRef Create(
		const ASL::String& inMetadataID, 
		const ASL::String& inMetadataPath,
		const XMPText& inXMPText);
	/*
	**
	*/
	void Initialize(ASL::String const& inMetadataID, const ASL::String& inMetadataPath);

	/*
	**
	*/
	void Initialize(ASL::String const& inMetadataID, const ASL::String& inMetadataPath, const XMPText& inXMPText);

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetMetadataID() const;

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetMetadataPath() const;

	/*
	**
	*/
	PL_EXPORT
	void SetAliasName(ASL::String const& inAliasName);

	/*
	**
	*/
	PL_EXPORT
	ASL::String GetAliasName();

	/*
	**
	*/
	PL_EXPORT
	void SetIsCreativeCloud(bool isCreativeCloud);

	/*
	**
	*/
	PL_EXPORT
	bool GetIsCreativeCloud();

	/*
	**
	*/
	PL_EXPORT
	PL::ISRUnassociatedMarkersRef GetMarkers();

	/*
	**
	*/
	PL_EXPORT
	OffsetType GetOffsetType() const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TickTime GetOffset() const;

	/*
	**
	*/
	PL_EXPORT
	dvamediatypes::TimeDisplay GetTimeDisplay() const;

private:

	/*
	**
	*/
	void ReadUnassociatedMetadataIntoMemory();

private:
	// Explanation for ID, path, AliasName:
	//	ID:	Every metadata should be registered by ID, and SRMarkers is read based on this ID, 
	//		this ID should *NOT* be shown in UI
	//	Path:	Where the metadata is put, in local case, path equals to ID, but for CC, path is the relative 
	//			URL to the CC root. Path will be only used for UI display and *CANNOT* be used for reading markers.
	//	AliasName:	Metadata name shown in UI.
	ASL::String				mMetadataID;
	ASL::String				mMetadataPath;
	//	[TRICKY] Current design of AliasName have to be unique, or else user may not choose 
	//	data with same alias name.
	ASL::String				mAliasName;

	PL::ISRUnassociatedMarkersRef       mMarkers;
	bool					mIsCreativeCloud;
	bool					mMarkerLoaded;

	// we use this field if PLL writes "timecodeOffset" or Prelude writes "altTimecode", 
	// this field has higher priority than "dayOffset".
	// If we cannot get a start timecode or dayOffset, we will apply markers on relative frame
	// Besides offset of tick count, we also need video time display to show a correct timecode string in UI
	OffsetType					mOffsetType;
	dvamediatypes::TickTime		mOffset;
	dvamediatypes::TimeDisplay	mTimeDisplay;
};

}

#endif // PLUNASSOCIATEDMETADATA_H
