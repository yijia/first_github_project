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

#ifndef PLPROJECT_H
#define PLPROJECT_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

// ASL
#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLAPPLICATIONMESSAGES_H
#include "ASLApplicationMessages.h"
#endif

// BE
#ifndef BE_CLIP_IMASTERCLIPFWD_H
#include "BE/Clip/IMasterClipFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEMCONTAINER_H
#include "BE/Project/IProjectItemContainer.h"
#endif

// SR
#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif

#ifndef IPLMEDIA_H
#include "IPLMedia.h"
#endif

#ifndef PLUNASSOCIATEDMETADATA_H
#include "PLUnassociatedMetadata.h"
#endif

#ifndef PLCLIPDEFINITION_H
#include "PLClipDefinition.h"
#endif

#ifndef PLMESSAGE_H
#include "PLMessage.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLSTATIONREGISTRY_H
#include "ASLStationRegistry.h"
#endif

#ifndef ASLSTATIONUTILS_H
#include "ASLStationUtils.h"
#endif

#ifndef PLSELECTIONWATCHER_H
#include "PLSelectionWatcher.h"
#endif

// boost
#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef PLASSETSELECTIONMANAGER_H
#include "PLAssetSelectionManager.h"
#endif

#ifndef IPLASSETLIBRARYNOTIFIER_H
#include "IPLAssetLibraryNotifier.h"
#endif

#ifndef UIFDOCUMENTMANAGER_H
#include "UIFDocumentManager.h"
#endif

#ifndef MZCCMSERVICE_H
#include "MZCCMService.h"
#endif

#ifndef MZFILEQUIETER_H
#include "MZFileQuieter.h"
#endif

#ifndef BE_PROJECT_PROJECTITEMMESSAGES_H
#include "BE/Project/ProjectItemMessages.h"
#endif

namespace PL
{

	/*
	**
	*/
	class AssetMediaInfoWrapper
		:
		public ASL::Listener
	{
		ASL_MESSAGE_MAP_BEGIN(AssetMediaInfoWrapper)
			ASL_MESSAGE_HANDLER(MediaMetaDataOutOfSync, OnMediaMetaDataOutOfSync)
			ASL_MESSAGE_HANDLER(WriteXMPToDiskFinished, OnWriteXMPToDiskFinished)
			ASL_MESSAGE_HANDLER(UnregisterMediaMonitor, OnUnregisterSRMediaMonitor)
			ASL_MESSAGE_HANDLER(RegisterMediaMonitor, OnRegisterSRMediaMonitor)
		ASL_MESSAGE_MAP_END

	public:

		/*
		**
		*/
		AssetMediaInfoWrapper()
			:
		mStationID(ASL::StationRegistry::RegisterUniqueStation()),
		mOffline(false)
		{}

		/*
		**
		*/
		explicit AssetMediaInfoWrapper(AssetMediaInfoPtr const& inAssetMediaInfo);

		/*
		**
		*/
		AssetMediaInfoPtr GetAssetMediaInfo()const {return mAssetMediaInfo;}

		/*
		**
		*/
		void BackToCachedXMPData();

		/*
		**
		*/
		bool IsMediaOffline()
		{
			return mOffline;
		}

		/*
		**
		*/
		void SetAssetMediaInfoOffline(bool isOffline)
		{
			mOffline = isOffline;
		}

		/*
		** inXMPTest:  xmp data 
		** isSync:    Send MediaMetaDataChanged async or sync
		** checkDirty: Check if current file is dirty
		**
		*/
		PL_EXPORT
		void RefreshMediaXMP(XMPText const& inXMPTest, bool isSync = false, bool checkDirty = false);

		/*
		**
		*/
		ASL::StationID GetStationID()const {return mStationID;}

		/*
		**
		*/
		XMP_DateTime GetXMPLastModTime()const;


	private:

		/*
		**
		*/
		void UpdateMarkerState();

		/*
		**
		*/
		void SetAssetMediaInfo(AssetMediaInfoPtr const& inAssetMediaInfo)
		{
			mAssetMediaInfo = inAssetMediaInfo;
		}

		/*
		**
		*/
		void OnUnregisterSRMediaMonitor(ASL::String const& inMediaPath);

		/*
		**
		*/
		void OnRegisterSRMediaMonitor(ASL::String const& inMediaPath);

		/*
		**
		*/
		void UpdateXMPLastModTime();

		/*
		**
		*/
		void OnWriteXMPToDiskFinished(ASL::Result inResult, dvacore::UTF16String inMediaPath, const BE::RequestIDs& inRequestIDs);

		/*
		**
		*/
		void OnMediaMetaDataOutOfSync(ASL::PathnameList const& inPathList);

		/*
		**
		*/
		ASL::Result GetLatestMetadataState(const dvacore::StdString& inPartID, dvacore::StdString& outMetadataState);

		bool							mRedirectToXMPMonitor;
		XMP_DateTime					mXMPLastModifyTime;
		AssetMediaInfoPtr				mAssetMediaInfo;
		ASL::StationID					mStationID;
		bool							mOffline;
		dvacore::StdString				mMarkerState;

		friend class SRProject;
	};

	typedef boost::shared_ptr<AssetMediaInfoWrapper> AssetMediaInfoWrapperPtr;
	//	Key: MediaURL 
	//	Value: AssetMediaInfoWrapper
	typedef std::map<ASL::String, AssetMediaInfoWrapperPtr> AssetMediaInfoWrapperMap;

	/*
	**
	*/
	class SRProject
		:
		public ASL::Listener
 	{
	public:
		typedef boost::shared_ptr<SRProject>	SharedPtr;

		ASL_MESSAGE_MAP_BEGIN(SRProject)
			ASL_MESSAGE_HANDLER(BE::MediaMetaDataWriteCompleteMessage, OnMediaMetaDataWriteComplete)
			ASL_MESSAGE_HANDLER(PL::MediaMetaDataOutOfSync, OnMediaMetaDataNotInSync)
			ASL_MESSAGE_HANDLER(MZ::CreativeCloudSignedOut, OnCCMLogout)
			ASL_MESSAGE_HANDLER(BE::ProjectStructureChangedMessage, OnProjectStructureChanged)

			ASL_MESSAGE_HANDLER(ASL::ApplicationSuspendedMessage, OnAppSuspended)
			ASL_MESSAGE_HANDLER(ASL::ApplicationResumedMessage, OnAppResumed)
			ASL_MESSAGE_HANDLER(MZ::RefreshFilesMessage, OnRefreshFiles)
		ASL_MESSAGE_MAP_END

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
		**	Synchronization Lock
		*/
		static ASL::CriticalSection& GetLocker();

		/*
		**	Singleton instance
		*/
		PL_EXPORT
		static SRProject::SharedPtr GetInstance();

		/*
		**	inForceRegister: only Relink API use this param
		*/
		PL_EXPORT
		PL::AssetMediaInfoPtr RegisterAssetMediaInfo(
			const ASL::String& inAssetPath,
			const PL::AssetMediaInfoPtr& inAssetMediaInfo,
			bool inForceRegister = false,
			bool inRelink = false);

		/*
		**
		*/
		PL_EXPORT
		void RegisterAssetMediaInfoList(
			const PL::AssetItemList& inAssetItems,
			const PL::AssetMediaInfoList& inMediaInfoList,
			bool inForceRegister = false,
			bool inRelink = false);

		/*
		**
		*/
		PL_EXPORT
		SRSelectionWatcherPtr GetSelectionWatcher();

		/*
		**
		*/
		PL_EXPORT
		PL::AssetMediaInfoPtr GetAssetMediaInfo(const ASL::String& inMediaPath, bool inCreateDummy = true) const;

		/**
		**
		*/
		PL_EXPORT
		PL::AssetMediaInfoPtr FindAssetMediaInfo(const ASL::Guid& inMediaInfoID) const;

		/*
		**
		*/
		PL_EXPORT
		AssetMediaInfoWrapperPtr GetAssetMediaInfoWrapper(const ASL::String& inMediaPath) const;

		/**
		**
		*/
		PL_EXPORT
		AssetMediaInfoWrapperPtr FindAssetMediaInfoWrapper(const ASL::Guid& inMediaInfoID) const;

		/**
		**	This function should receive a physical file path, rather than guild string or something else.
		*/
		PL_EXPORT
		bool RefreshAssetMediaInfoByPath(const ASL::String& inFilePath, XMPText const& inXMPTest);

		/**
		**	
		*/
		PL_EXPORT
		bool RefreshAssetMediaInfoByID(const ASL::Guid& inMediaInfoID, XMPText const& inXMPTest);

		/*
		**
		*/
		PL_EXPORT
		ISRPrimaryClipRef GetPrimaryClip(const dvacore::UTF16String& inMediaPath) const;

		/*
		**
		*/
		PL_EXPORT
		void RefreshOfflineClips();

		/*
		**
		*/
		PL_EXPORT
		SRAssetSelectionManager::SharedPtr	GetAssetSelectionManager();


		/*
		**
		*/
		PL_EXPORT
		void AddSRMedia(
					ISRMediaRef inSRMedia);

		/*
		**
		*/
		PL_EXPORT
		PL::ISRMediaRef GetSRMedia(
			const ASL::String& inMasterClipPath) const;

		/*
		**
		*/
		PL_EXPORT
		PL::ISRMediaRef GetSRMediaByInstanceString(
			const ASL::String& inMediaInstanceString) const;

		/*
		**
		*/
		PL_EXPORT
		SRMediaSet GetSRMedias() const;
		
		/*
		**
		*/
		PL_EXPORT
		void AddUnassociatedMetadata(
			SRUnassociatedMetadataRef inSRUnassociatedMetadata);

		/*
		**
		*/
		PL_EXPORT
		void RemoveSRUnassociatedMetadata(
			SRUnassociatedMetadataRef inSRMedia);

		/*
		**
		*/
		PL_EXPORT
		bool RemoveAllSRUnassociatedMetadatas();

		/*
		**
		*/
		PL_EXPORT
		PL::SRUnassociatedMetadataRef FindSRUnassociatedMetadata(
			const ASL::String& inMetadataID) const;

		/*
		**
		*/
		PL_EXPORT
		const SRUnassociatedMetadataList& GetSRUnassociatedMetadatas() const;
		
		/*
		**
		*/
		PL_EXPORT
		void SetAssetLibraryNotifier(IAssetLibraryNotifier* inAssetLibraryNotifier);

		/*
		**
		*/
		PL_EXPORT
		IAssetLibraryNotifier* GetAssetLibraryNotifier();

		/*
		**
		*/
		PL_EXPORT
		void SetProject(BE::IProjectRef project);

		/*
		**
		*/
		PL_EXPORT
		void RemoveUnreferenceResources();

		/*
		** Debug purpose only
		*/
		PL_EXPORT
		void ValidProjectData();

	protected:
		SRProject();

		void OnAppSuspended();
		void OnAppResumed();
		void OnRefreshFiles();
		void RefreshFiles();


	public:
		~SRProject();
		
	private:

		/*
		**
		*/
		void  OnProjectStructureChanged(const BE::ProjectStructureChangedInfo& inChangedInfo);

		/*
		**
		*/
		void OnCCMLogout();

		/*
		**
		*/
		ISRMediaRef GetSRMedia(const SRMediaSet& inSRMedias, const ASL::String& inMasterClipPath) const;

		/*
		**
		*/
		bool RemoveAllSRMedias();

		/*
		**
		*/
		void RemoveSRMedia(SRMediaSet& ioSRMedias, ISRMediaRef inSRMedia);

		/*
		**
		*/
		void UnRegisterAssetMediaInfo(AssetMediaInfoWrapperMap& ioAssetMediaInfoWrapperMap, const ASL::String& inMediaPath);

		/*
		**
		*/
		void UnRegisterAllAssetMediaInfo();

		/*
		**
		*/
		void OnMediaMetaDataNotInSync(ASL::PathnameList const& inPathList);

		/*
		**
		*/
		void OnMediaMetaDataWriteComplete(
						ASL::Result inResult,	
						const BE::IMediaMetaDataRef& inMediaMetaData,
						const BE::RequestIDs& inRequestID);

		/*
		**
		*/
		void RemoveUnreferenceResourcesInternal();

		/*
		**
		*/
		void RemoveUnreferenceSRMedia(SRMediaSet& ioSRMedias, ASL::PathnameList const& inReservedMediaPathList);

		/*
		**
		*/
		void RemoveUnreferenceAssetMediaInfo(AssetMediaInfoWrapperMap& ioAssetMediaInfoMap, ASL::PathnameList const& inReservedMediaPathList);

		/*
		**
		*/
		void AddPrimaryClip(ISRPrimaryClipRef inPrimaryClip);

		/*
		**
		*/
		ISRPrimaryClipRef RemovePrimaryClip(const dvacore::UTF16String& inMediaPath);

		static SRProject::SharedPtr			sProject;
		static ASL::CriticalSection			sCriticalSection;

		SRMediaSet							mSRMedias;
		SRUnassociatedMetadataList			mSRUnassociatedMetadatas;
		SRPrimaryClipList					mSRPrimaryClipList;

		
		AssetMediaInfoWrapperMap			mAssetMediaInfoWrapperMap;

		SRAssetSelectionManager::SharedPtr	mAssetSelectionManager;		

		SRSelectionWatcherPtr				mSelectionWatcher;
		
		IAssetLibraryNotifier*				mAssetLibraryNotifier;

		dvacore::threads::AsyncThreadedExecutorPtr	mUnreferencedResourceRemovalExecutor;
		bool										mProjectResourceChanged;

		friend class ModulePicker;
	};

}

#endif
