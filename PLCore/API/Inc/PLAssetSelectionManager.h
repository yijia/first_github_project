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

#ifndef PLASSETSELECTIONMANAGER_H
#define PLASSETSELECTIONMANAGER_H

//	ASL
#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

//	MZ
#ifndef MZACTIVATION_H
#include "MZActivation.h"
#endif

#ifndef PLASSETMEDIAINFOSELECTION_H
#include "PLAssetMediaInfoSelection.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif 

//	ASL
#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif 

// boost
#include "boost/shared_ptr.hpp"

namespace PL
{

	class AssetSelectionManagerQuieter
	{
	public:
		AssetSelectionManagerQuieter();
		~AssetSelectionManagerQuieter();
	};

	class SRAssetSelectionManager 
		:
		public ASL::Listener
	{
	public:
		typedef boost::shared_ptr<SRAssetSelectionManager>	SharedPtr;
		typedef std::set<ASL::String> MediaURLSet;

	public:
		SRAssetSelectionManager();
		~SRAssetSelectionManager();

	public:

		ASL_MESSAGE_MAP_DECLARE();

		/*
		**	Get Selected AssetItem list in library
		*/
		PL_EXPORT
		PL::AssetItemList const& GetSelectedAssetItemList() const;

		/*	
		**	Set Selected AssetItem list.
		**  When inForceUpdate is false, if the assetItem already exist, 
		**  the assetItem from param will not be used.
		*/
		PL_EXPORT
		void SetSelectedAssetItemList(const PL::AssetItemList& inAssetItemList, bool inForceUpdate = false);

		/*
		**	
		*/
		PL_EXPORT
		bool IsAssetItemCurrentSelected(const PL::AssetItemPtr& inAssetItem);

		/*
		**
		*/
		PL_EXPORT
		void UpdateSelectedAssetItem(const PL::AssetItemPtr& inAssetItem);

		/*	
		**	A tricky solution for the problem:
		**		when a dirty rough cut is saved, the selected asset item list 
		**		of the SelectionManager will not be updated.
		**	Therefore we decide to provide this interface for the rough cut saving code.
		*/
		PL_EXPORT
		void UpdateSelectedRCSubAssetItemList(const PL::AssetItemPtr& inAssetItem);

		/*
		**  This function can set selected file's XMP in library to metadata panel
		*/
		PL_EXPORT
		void TriggerLibraryMetaDataSelection();

		/*
		**  You can specify which files' XMP are displayed in metedata panel 
		**	except those selected in library
		*/
		PL_EXPORT
		void TriggerMetaDataSelection(MediaURLSet const& inMediaURLSet);

		/*
		**	Verify if specific media type is included in selected assetItems in library.
		*/
		PL_EXPORT
		bool LibrarySelectionContainAssetType(ASL::UInt64 inMediaType);

	protected:

		/*
		**
		*/
		void UpdateSelectedMediaURLSet(MediaURLSet const& inMediaURLSet);

		/*
		**
		*/
		void OnMediaMetaDataChanged(ASL::String const& inAssetMediaInfoID);

		/*
		**
		*/
		void OnAssetItemRename(ASL::String const& inMediaPath, ASL::String const& inNewName);

		/*
		**
		*/
		void CalcMediaLocatorAndUpdateInPanel(MediaURLSet const& inMediaURLSet);

		/*
		**
		*/
		void Clear();

		/*
		**
		*/
		void OnAssetItemInOutChanged(ASL::Guid inId, BE::IProjectItemRef inProjectItem);


	private:
		MediaURLSet							mSelectedMediaURLSet;

		//	AssetItem selected in library
		PL::AssetItemList					mSelectedAssetItemList;

		ASL::CriticalSection			mCriticalSection;
	};
}

#endif