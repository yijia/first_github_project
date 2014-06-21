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


#ifndef PLMODULEPICKER_H
#define PLMODULEPICKER_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef MZCONSTANTS_H
#include "PLConstants.h"
#endif

#ifndef PLUTILITIES_h
#include "PLUtilities.h"
#endif

#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif

#ifndef IPLPRIMARYCLIPPLAYBACK_H
#include "IPLPrimaryClipPlayback.h"
#endif

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

namespace PL
{
	class ModulePicker
		:
		public ASL::Listener
	{
	public:
		typedef boost::shared_ptr<ModulePicker>	SharedPtr;

		/*
		**
		*/
		PL_EXPORT
		static void Initialize();

		/*
		**
		*/
		PL_EXPORT
		static void Terminate();

		/*
		**
		*/
		PL_EXPORT
		static ModulePicker::SharedPtr GetInstance();

		/*
		** Active AssetItem 
		*/
		PL_EXPORT
		bool ActiveModule(
					PL::AssetItemPtr const& inAssetItem,
					bool inNeedZoom = true);

		/*
		** 
		*/
		PL_EXPORT
		bool IsAssetItemCurrentOpened(ASL::String const& inOldMediaPath, PL::AssetItemPtr const& inAssetItem);

		/*
		**
		*/
		PL_EXPORT
		bool UpdateOpenedAssetItem(ASL::String const& inOldMediaPath, PL::AssetItemPtr const& inAssetItem);

		/*
		**
		*/
		PL_EXPORT
		bool ClearActiveAssetUndoStack(PL::AssetItemPtr const& inAssetItem);

		/*
		** Return Logging Clip
		*/
		PL_EXPORT
		ISRPrimaryClipPlaybackRef GetLoggingClip();

		/*
		** Return Rough Cut Clip
		*/
		PL_EXPORT
		ISRPrimaryClipPlaybackRef GetRoughCutClip();

		/*
		**	Get current Module data
		*/
		PL_EXPORT
		ISRPrimaryClipPlaybackRef GetCurrentModuleData();

		/*
		**	Close all Modules
		*/
		PL_EXPORT
		void CloseAllModuleData(bool inSuppressModeChangedMsg = false);

		/*
		**	Only Save & Close current module
		*/
		PL_EXPORT
		bool CloseActiveModule();

		/*
		**	Save all Modules. In default, we don't need to Prompt client
		*/
		PL_EXPORT
		UIF::MBResult::Type SaveAllModuleData(bool inIsSilent = true);

		/*
		**	Any data is dirty?
		*/
		PL_EXPORT
		bool ContainsDirtyData() const;

		/*
		**	Is auto save enabled?
		*/
		PL_EXPORT
		static bool IsAutoSaveEnabled();

		/*
		** Close Module by Path
		*/
		PL_EXPORT
		ISRPrimaryClipRef CloseModule(const dvacore::UTF16String& inMediaPath);

		PL_EXPORT
		~ModulePicker();

		/*
		**	Save Module data to a user specified xmp file
		*/
		PL_EXPORT
		UIF::MBResult::Type SaveModuleXMPDataAs();

		/*
		**	Save Module data to a user specified rough cut file
		*/
		PL_EXPORT
		UIF::MBResult::Type SaveModuleRoughCutDataAs();

		/*
		**  Get SREditingModule
		*/
		PL_EXPORT
		SREditingModule GetCurrentEditingModule();

		/*
		** Get PrimaryCLip by ModuleID
		*/
		PL_EXPORT
		ISRPrimaryClipRef GetPrimaryClip(SREditingModule inModuleID);

	private:

		/*
		**
		*/
		ModulePicker();

		/*
		**	Each AssetItem can specify which module we should open for.
		*/
		SREditingModule GetModuleTypeFromAssetItem(PL::AssetItemPtr const& inAssetItem);

		/*
		**	Get edited data by module ID
		*/
		ISRPrimaryClipPlaybackRef GetModuleData(SREditingModule inModuleID);

		/*
		**	Test if new workspace is same with current one.
		*/
		bool	IsSameModule(SREditingModule inModuleID);

		/*
		**	Test if new clip is same with current one.
		*/
		bool	IsSameClipPath(
					SREditingModule inModuleID, 
					PL::AssetItemPtr const& inAssetItem);

		/*
		**	Test if user cancel save operation.
		*/
		bool	IsUserCanceledSave(SREditingModule inModuleID);

		/*
		**	Active Module By ID and Open clipInfo on Timeline editor
		*/
		void ActiveModuleInternal(
					SREditingModule inModuleID, 
					PL::AssetItemPtr const& inAssetItem);

		/*
		**	Remove inPrimaryClip from cache, and update the UI if necessary
		*/
		void RemovePrimaryClipInternal(ISRPrimaryClipRef inPrimaryClip);

		/*
		**	Attach primaryClip in time line editor
		*/
		void AttachModuleData(ISRPrimaryClipRef inPrimaryClip);

		/*
		**	Detach primaryClip from time line editor
		*/
		void DetachModuleData(ISRPrimaryClipRef inPrimaryClip);

		/*
		**	Send Close Notification to client
		*/
		void SendCloseNotification(AssetItemPtr const& inCloseAssetItem);

		/*
		**  Send Open Notification to client
		*/
		void SendOpenNotification(AssetItemPtr const& inOpenAssetItem);

		/*
		**  Create a new primaryClip by AssetItem
		*/
		ISRPrimaryClipRef TakePrimaryClip(PL::AssetItemPtr const& inAssetItem);


		void ActivateAsset(PL::AssetItemPtr const& inAssetItem);


		ASL_MESSAGE_MAP_DECLARE();

	private:
		/*
		**
		*/
		void OnUndoStackChanged();
		void OnUndoStackCleared();

		/*
		**
		*/
		void OnMediaMetaDataMarkerNotInSync(ASL::String const& inMediaLocator);

	private:
	
		static ModulePicker::SharedPtr	sModulePicker;

		typedef std::map<SREditingModule, PL::AssetItemPtr> ModulePickerEntryMap;
		SREditingModule					mCurrentModuleID;
		ModulePickerEntryMap			mModulePickerEntrys;

		bool	mIsModuleOriginalDirty;
	};

	/**
	** This method helps determine if we're able to apply markers to the open item. This
	** should return false for Roughcuts and any clip that is offline, ReadOnly, or otherwise
	** unavailable to accept markers.
	*/
	PL_EXPORT
	bool CurrentMarkersAreEditable();
}


#endif
