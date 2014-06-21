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

#ifndef PLROUGHCUT_H
#define PLROUGHCUT_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
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

#ifndef IPLROUGHCUT_H
#include "IPLRoughCut.h"
#endif

#ifndef PLROUGHCUTUTILS_H
#include "PLRoughCutUtils.h"
#endif

#ifndef PLPRIVATECREATOR_H
#include "PLPrivateCreator.h"
#endif

#ifndef IPLMARKERS_H
#include "IPLMarkers.h"
#endif

#ifndef PLMARKERSELECTION_H
#include "PLMarkerSelection.h"
#endif


namespace PL
{

/*
**
*/
ASL_DEFINE_CLASSREF(SRRoughCut, ISRRoughCut);
ASL_DEFINE_IMPLID(SRRoughCut, 0xd061088e, 0x7151, 0x422a, 0x89, 0x1c, 0x30, 0xe1, 0x1d, 0x49, 0x3a, 0x5f);

class SRRoughCut 
	:
	public ISRRoughCut,
	public ISRPrimaryClip,
	public ISRPrimaryClipPlayback,
	public SRMarkerSelection,
	public ASL::Listener
{
	/*
	**
	*/
	ASL_STRONGWEAK_CLASS_SUPPORT(SRRoughCut);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ISRRoughCut)
		ASL_QUERY_ENTRY(ISRPrimaryClip)
		ASL_QUERY_ENTRY(ISRPrimaryClipPlayback)
		ASL_QUERY_CHAIN(SRMarkerSelection)
	ASL_QUERY_MAP_END

	ASL_MESSAGE_MAP_DECLARE();
public:

	/*
	**
	*/
	virtual BE::ISequenceRef GetBESequence();

	/*
	**
	*/
	virtual bool TestIfRoughCutChanged();

	/*
	**
	*/
	virtual ASL::Color GetTrackItemLabelColor() const;

	/*
	**
	*/
	virtual bool RemoveClipItem(const BE::ITrackItemRef inTrackItem);

	/*
	**
	*/
	virtual bool RelinkClipItem(SRClipItems& ioRelinkedClipItems);
	
	/*
	**	
	*/
	static SRRoughCutRef Create(
							PL::AssetItemPtr const& inAssetItem);

	/*
	**
	*/
	virtual ASL::PathnameList GetReferencedMediaPath();

	/*
	**	
	*/
	virtual bool AppendClipItems(SRClipItems& ioClipItems);

	/*
	**
	*/
	virtual void RefreshRoughCut(PL::AssetItemPtr const& inRCAssetItem);

	/*
	**
	*/
	virtual bool AddClipItems(
					SRClipItems& ioClipItems, 
					dvamediatypes::TickTime inInsertTime,
					dvamediatypes::TickTime* outChangedDuration);

	/*
	**
	*/
	virtual bool RemoveSelectedTrackItem(bool inDoRipple, bool inAlignToVideo);

	/*
	**
	*/
	SRRoughCut();

	/*
	**
	*/
	~SRRoughCut();

	/*
	**
	*/
	virtual bool IsDirty() const;

	/*
	**
	*/
	virtual bool Relink(
		PL::AssetItemPtr const& inRelinkAssetItem);

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
	dvacore::UTF16String GetName() const;

	/*
	**
	*/
	virtual void SetDirty(bool inIsDirty);

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
	virtual BE::IMasterClipRef GetSequenceMasterClip() const;

	/*
	**
	*/
	PL::AssetMediaInfoPtr GetAssetMediaInfo() const;

	/*
	**
	*/
	virtual SRClipItems GetClipItems() const;

	/*
	**
	*/
	virtual dvacore::UTF16String GetFilePath()const;

	/*
	**
	*/
	virtual void ResetRoughtClipItemFromSequence();

	virtual bool IsWritable() const;

	virtual bool IsOffline() const;

	/**
	**	Called if PL found save failure or read-only changes after resume.
	*/
	void UpdateWritableState(bool inIsWritable);

private:

	/*
	**
	*/
	void OnAssetItemRename(ASL::String const& inMediaLocator, ASL::String const& inNewName);

	/*
	**
	*/
	PL::AssetItemPtr BuildRCAssetItem();

	/*
	**	
	*/
	void Init(PL::AssetItemPtr const& inAssetItem);

	/*
	**
	*/
    PL::ISRMarkersRef GetMarkers(BE::IMasterClipRef inMasterClip) const;
	
	/*
	**
	*/
	virtual PL::AssetItemPtr GetAssetItem() const;
	/*
	**
	*/
	virtual SRMarkersList GetMarkersList() const;
	
	/*
	**
	*/
	virtual ASL::String GetSubClipName(const BE::ITrackItemRef inTrackItem) const;

	/*
	**
	*/
	void StopPlayer();

	/*
	**
	*/
	virtual ASL::Guid GetSubClipGUID(const BE::ITrackItemRef inTrackItem) const;

	/*
	**
	*/
	virtual SRClipItemPtr GetSRClipItem(const BE::ITrackItemRef inTrackItem) const;

	/*
	**	
	*/
	virtual bool IsDirtyChild(BE::IMasterClipRef inMasterClip) const;

	/*
	**
	*/
	virtual bool Publish();

	/*
	**
	*/
	ISRMediaRef GetSRMedia(BE::IMasterClipRef inMasterClip) const;
	
	/*
	**
	*/
	void Load();

	/*
	**
	*/
	void GetDeletingMarkers(const BE::IMasterClipRef inMasterClip, MarkerSet& outMarkers) const;

	/*
	**
	*/
	bool FindMarker(const CottonwoodMarker& inMarker) const;

	/*
	**
	*/
	dvacore::UTF16String GetRoughCutName() const;

	/*
	**
	*/
	void OnShowTimecodeStatusChanged();

	void DiscardUnsavedChanges();

	/*
	**
	*/
	void OnUndoStackCleared();

	/*
	**
	*/
	bool IsRoughCutForEA() const;

	/*
	** Helper function to insert a SRClipItem into a SRClipItems at a specified position
	*/
	void ReverseInsertClipItem(SRClipItems::iterator& ioPosition, const SRClipItems::value_type& inClipItem);

	/*
	**
	*/
	void UpdateAssetItem();

private:
	bool					mIsDirty;
	PL::AssetItemPtr		mAssetItem;
	dvamediatypes::TickTime	mEditTime;

	BE::IProjectItemRef		mSequenceItem;
	SRClipItems				mClipItems;

	// This is used to track all the deleted items. We need it for undo operation.
	SRClipItems				mTrashItems;

	bool					mIsAttached;
	bool					mIsWritable;

	// when auto-save succeeded, we remember undoable action count.
	// when save fails and need rollback, we undo/redo to reach same undoable action count.
	ASL::UInt32						mUndoableActionCountAfterAutoSave;
	// if user has choose Cancel at first time save failure dialog pop up, we will not pop up it again until close or exit.
	bool							mUserHasCanceledSaveFailure;

	ASL::CriticalSection		mCriticalSection;
	
	friend class SilkRoadPrivateCreator;
};

} // namespace PL

#endif // #ifndef PLROUGHCUT_H
