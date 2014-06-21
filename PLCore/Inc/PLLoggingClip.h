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

#ifndef PLLOGGINGCLIP_H
#define PLLOGGINGCLIP_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BESEQUENCEFWD_H
#include "BESequenceFwd.h"
#endif

#ifndef ASLSTRONGWEAKCLASS_H
#include "ASLStrongWeakClass.h"
#endif

#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef PLMESSAGE_H
#include "PLMessage.h"
#endif

#ifndef IPLLOGGINGCLIP_H
#include "IPLLoggingClip.h"
#endif

#ifndef PLPRIVATECREATOR_H
#include "PLPrivateCreator.h"
#endif

#ifndef PLMARKERSELECTION_H
#include "PLMarkerSelection.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

namespace PL
{

/*
**
*/
ASL_DEFINE_CLASSREF(SRLoggingClip, ISRLoggingClip);
ASL_DEFINE_IMPLID(SRLoggingClip, 0xfea1788b, 0x7a11, 0x4ed7, 0xbc, 0x13, 0x9f, 0x1a, 0x46, 0xb, 0x12, 0xf8);

class SRLoggingClip 
	:
	public ISRLoggingClip,
	public ISRPrimaryClip,
	public ISRPrimaryClipPlayback,
	public SRMarkerSelection,
	public ASL::Listener
{

	/*
	**
	*/
	ASL_STRONGWEAK_CLASS_SUPPORT(SRLoggingClip);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ISRLoggingClip)
		ASL_QUERY_ENTRY(ISRPrimaryClip)
		ASL_QUERY_ENTRY(ISRPrimaryClipPlayback)
        ASL_QUERY_CHAIN(SRMarkerSelection)
	ASL_QUERY_MAP_END

	ASL_MESSAGE_MAP_DECLARE();
public:

	virtual BE::ISequenceRef GetBESequence();

	/*
	**
	*/
	virtual ASL::Color GetTrackItemLabelColor() const;

	/*
	**
	*/
	virtual bool Relink(
				PL::AssetItemPtr const& inRelinkAssetItem);
	
	/**
	**	Create LoggingClip by LibraryInfo
	*/
	static SRLoggingClipRef Create(PL::AssetItemPtr const& inAssetItem);

	/*
	**
	*/
	SRLoggingClip();
	
	/*
	**
	*/
	virtual ~SRLoggingClip();

	/*
	**
	*/
	virtual bool IsDirty() const;

	virtual void SetDirty(bool inIsDirty);

	/*
	**
	*/
	virtual UIF::MBResult::Type Save(bool isSilent, bool isCloseOrExit);

	/*
	**
	*/
	virtual UIF::MBResult::Type Save(bool isSilent, bool isCloseOrExit, ISRPrimaryClip::SaveCallBackFn const& inSaveCallBackFn);


	/*
	**
	*/
	virtual bool AttachToTimeLine();

	/*
	**
	*/
	virtual bool DetachFromTimeLine();

	/*
	**
	*/
	virtual bool CreateSequence();

	/*
	**
	*/
	virtual bool IsSupportMarkerEditing() const;

	/*
	**
	*/
	virtual bool IsSupportTimeLineEditing() const;

	/*
	**
	*/
	virtual bool IsLoggingClipForEA() const;

	/*
	**
	*/
	BE::IMasterClipRef GetMasterClip() const;

	virtual BE::IMasterClipRef GetSequenceMasterClip() const;

	/*
	**
	*/
	virtual PL::AssetItemPtr GetAssetItem() const;

	/*
	**
	*/
	virtual PL::AssetMediaInfoPtr GetAssetMediaInfo() const;

	/*
	**
	*/
	void SetEditTime(const dvamediatypes::TickTime& inEditTime);

	/*
	**
	*/
	dvamediatypes::TickTime GetEditTime() const;

	/*
	**
	*/
	void SetActiveSubClipBoundary(
		const dvamediatypes::TickTime& inStart,
		const dvamediatypes::TickTime& inEnd,
		const ASL::Guid& inID);

	virtual dvamediatypes::TickTime GetStartTime() const;
	virtual dvamediatypes::TickTime GetDuration() const;
	virtual void UpdateBoundary(const dvamediatypes::TickTime& inStartTime, const dvamediatypes::TickTime& inDuration);

	/*
	**
	*/
	virtual SRClipItemPtr GetSRClipItem() const;
	
	/*
	**
	*/
	virtual bool IsOffline() const;

	/*
	**
	*/
	virtual PL::ISRMarkersRef GetMarkers(BE::IMasterClipRef inMasterClip) const;

	/*
	**
	*/
	virtual SRMarkersList GetMarkersList() const;

	/*
	**
	*/
	virtual ASL::PathnameList GetReferencedMediaPath();

	/*
	**
	*/
	virtual dvacore::UTF16String GetName() const;
	
	/*
	**
	*/
	virtual ASL::String GetSubClipName(const BE::ITrackItemRef inTrackItem) const;

	/*
	**
	*/
	virtual ASL::Guid GetSubClipGUID(const BE::ITrackItemRef inTrackItem) const;

    /*
    **
    */
	virtual bool IsWritable() const;

private:

	/*
	**
	*/
	void OnSRMediaChanged(ISRMediaRef const& inSRMedia);

	/*
	**
	*/
	void LoadCachedXMPData();

	/*
	**
	*/
	bool SendSaveNotification(bool& outDiscardChanges, bool isCloseOrExit, ISRPrimaryClip::SaveCallBackFn const& inSaveCallBackFn);

	/*
	**
	*/
	void SelectClipItem();

	/*
	**	Init LoggingClip by LibraryInfo
	*/
	void Init(PL::AssetItemPtr const& inAssetItem);

	/*
	**
	*/
	void UpdateSelection();

	/*
	**
	*/
	void OnSilkRoadMarkerRenamed(PL::CottonwoodMarkerList const& inChangedMarkerList);

	/*
	**
	*/
	void OnShowTimecodeStatusChanged();

	/*
	**
	*/
	void OnMediaMetaDataChanged(ASL::String const& InMediaInfoID);

	/*
	**
	*/
	void OnAssetItemRename(ASL::String const& inMediaFile, ASL::String const& inNewName);

	/**
	**
	*/
	void OnUndoStackCleared();

private:

	// CTI location in parent for reference
	dvamediatypes::TickTime			mEditTime;
	dvamediatypes::TickTime			mActiveSubclipStart;
	dvamediatypes::TickTime			mActiveSubclipEnd;
	ASL::Guid						mActiveSubclipID;

	PL::AssetItemPtr				mAssetItem;
	SRClipItemPtr					mSRClipItem;
	BE::IProjectItemRef				mSequenceItem;

	// when auto-save succeeded, we remember undoable action count.
	// when save fails and need rollback, we undo/redo to reach same undoable action count.
	ASL::UInt32						mUndoableActionCountAfterAutoSave;
	// if user has choose Cancel at first time save failure dialog pop up, we will not pop up it again until close or exit.
	bool							mUserHasCanceledSaveFailure;

	ASL::CriticalSection		mCriticalSection;

	friend class SilkRoadPrivateCreator;
	friend class SRMarkerSelectionPrivateImpl;
};

} // namespace PL

#endif
