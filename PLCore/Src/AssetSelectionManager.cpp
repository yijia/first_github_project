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

#include "Prefix.h"

//	Local
#include "PLAssetSelectionManager.h"

#include "MZInfoStation.h"
#include "PLConstants.h"
#include "PLMessage.h"

//	SR
#include "IPLPrimaryClipPlayback.h"
#include "IPLRoughCut.h"
#include "PLProject.h"
#include "PLLibrarySupport.h"

// MZ
#include "MZUtilities.h"
#include "MZBEUtilities.h"

//	ASL
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLStringCompare.h"
#include "ASLPathUtils.h"
#include "ASLClassFactory.h"

//	BE

//	DVA
#include "dvacore/filesupport/file/file.h"
#include "dvacore/utility/StringUtils.h"

//	UIF
#include "UIFDocumentManager.h"


namespace PL
{
	namespace
	{
		bool sQuiet = false; 
	}

	AssetSelectionManagerQuieter::AssetSelectionManagerQuieter()
	{
		DVA_ASSERT(!sQuiet);
		sQuiet = true;
	}

	AssetSelectionManagerQuieter::~AssetSelectionManagerQuieter()
	{
		DVA_ASSERT(sQuiet);
		sQuiet = false;
	}

	ASL_MESSAGE_MAP_DEFINE(SRAssetSelectionManager)
		ASL_MESSAGE_HANDLER(PL::MediaMetaDataChanged, OnMediaMetaDataChanged)
		ASL_MESSAGE_HANDLER(PL::AssetItemNameChanged, OnAssetItemRename)	
		ASL_MESSAGE_HANDLER(PL::AssetItemInOutChanged, OnAssetItemInOutChanged)	
	ASL_MESSAGE_MAP_END

	/*
	**
	*/
	SRAssetSelectionManager::SRAssetSelectionManager()
	{
		ASL::StationUtils::AddListener(MZ::kStation_PreludeProject, this);
	}

	/*
	**
	*/
	SRAssetSelectionManager::~SRAssetSelectionManager()
	{
		Clear();
	}

	/*
	**
	*/
	void SRAssetSelectionManager::OnAssetItemRename(ASL::String const& inMediaPath, ASL::String const& inNewName)
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		BOOST_FOREACH(PL::AssetItemPtr& assetItem, mSelectedAssetItemList)
		{
			if (MZ::Utilities::NormalizePathWithoutUNC(assetItem->GetMediaPath()) == MZ::Utilities::NormalizePathWithoutUNC(inMediaPath))
			{
				if (inNewName != assetItem->GetAssetName())
				{
					assetItem->SetAssetName(inNewName);
				}
			}
		}
	}

	/*
	**
	*/
	void SRAssetSelectionManager::OnMediaMetaDataChanged(ASL::String const& inAssetMediaInfoID)
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		if (!sQuiet)
		{
			if (UIF::IsEAMode())
			{
				BOOST_FOREACH(PL::AssetItemPtr const& assetItem, mSelectedAssetItemList)
				{
					if (inAssetMediaInfoID == assetItem->GetAssetMediaInfoGUID().AsString())
					{
						CalcMediaLocatorAndUpdateInPanel(mSelectedMediaURLSet);
						break;
					}
				}
			}
			else
			{
				if (mSelectedMediaURLSet.find(inAssetMediaInfoID) != mSelectedMediaURLSet.end())
				{
					CalcMediaLocatorAndUpdateInPanel(mSelectedMediaURLSet);
				}
			}
		}

		if (!UIF::IsEAMode())
		{
			BOOST_FOREACH(PL::AssetItemPtr& assetItem, mSelectedAssetItemList)
			{
				if (assetItem->GetAssetMediaType() == kAssetLibraryType_MasterClip &&
					SRProject::GetInstance()->GetAssetMediaInfo(assetItem->GetMediaPath())->GetMediaPath() == inAssetMediaInfoID)
				{
					ASL::String newName;
					SRLibrarySupport::GetNameFromXMP(SRProject::GetInstance()->GetAssetMediaInfo(assetItem->GetMediaPath())->GetXMPString(),newName);

					if (!newName.empty() && newName != assetItem->GetAssetName())
					{
						assetItem->SetAssetName(newName);
						//	Ok, we got new name
						PL::AssetMediaInfoPtr mediaInfo = PL::SRProject::GetInstance()->GetAssetMediaInfo(
							assetItem->GetMediaPath(), false);
						if (mediaInfo)
						{
							mediaInfo->SetAliasName(newName);
						}
					}
				}
			}
		}
	}

	/*
	**
	*/
	PL::AssetItemList const& SRAssetSelectionManager::GetSelectedAssetItemList() const
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		return mSelectedAssetItemList;
	}

	/*
	**
	*/
	void SRAssetSelectionManager::Clear()
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		mSelectedMediaURLSet.clear();
		mSelectedAssetItemList.clear();
	}

	/*
	**
	*/
	void SRAssetSelectionManager::SetSelectedAssetItemList(
							const PL::AssetItemList& inAssetItemList, bool inForceUpdate)
	{
		{
			ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

			if (inForceUpdate)
			{
				mSelectedAssetItemList = inAssetItemList;
			}
			else
			{
				typedef std::map<ASL::Guid, AssetItemPtr> GuidToAssetItemMap;
				GuidToAssetItemMap guidToAssetItemMap;
				BOOST_FOREACH(AssetItemPtr& assetItem, mSelectedAssetItemList)
				{
					guidToAssetItemMap.insert(GuidToAssetItemMap::value_type(assetItem->GetAssetGUID(), assetItem));
				}

				AssetItemList resultAssetItemList;
				BOOST_FOREACH(const AssetItemPtr& newAssetItem, inAssetItemList)
				{
					GuidToAssetItemMap::iterator it = guidToAssetItemMap.find(newAssetItem->GetAssetGUID());
					if (it != guidToAssetItemMap.end())
					{
						resultAssetItemList.push_back(it->second);
					}
					else
					{
						resultAssetItemList.push_back(newAssetItem);
					}
				}
				mSelectedAssetItemList = resultAssetItemList;
			}
		}

		SRProject::GetInstance()->RemoveUnreferenceResources();
	}

	/*
	**
	*/
	bool SRAssetSelectionManager::IsAssetItemCurrentSelected(const PL::AssetItemPtr& inAssetItem)
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		bool result(false);
		BOOST_FOREACH(PL::AssetItemPtr& eachAssetItem, mSelectedAssetItemList)
		{
			if (inAssetItem->GetAssetGUID() == eachAssetItem->GetAssetGUID())
			{
				result = true;
			}
		}

		return result;
	}

	void SRAssetSelectionManager::UpdateSelectedAssetItem(const PL::AssetItemPtr& inAssetItem)
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		BOOST_FOREACH(PL::AssetItemPtr& eachAssetItem, mSelectedAssetItemList)
		{
			if (inAssetItem->GetAssetGUID() == eachAssetItem->GetAssetGUID())
			{
				eachAssetItem = inAssetItem;
				break;
			}
		}
	}

	/*	
	**	
	*/
	void SRAssetSelectionManager::UpdateSelectedRCSubAssetItemList(const PL::AssetItemPtr& inAssetItem)
	{
		{
			ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

			BOOST_FOREACH(PL::AssetItemPtr& eachOldAssetItem, mSelectedAssetItemList)
			{
				if (inAssetItem->GetAssetMediaType() == kAssetLibraryType_RoughCut &&
					eachOldAssetItem->GetAssetMediaType() == kAssetLibraryType_RoughCut &&
					inAssetItem->GetAssetGUID() == eachOldAssetItem->GetAssetGUID())
				{
					eachOldAssetItem->SetSubAssetItemList(inAssetItem->GetSubAssetItemList());
				}
			}
		}

		SRProject::GetInstance()->RemoveUnreferenceResources();
	}


	/*
	**
	*/
	bool SRAssetSelectionManager::LibrarySelectionContainAssetType(ASL::UInt64 inMediaType)
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		bool find = false;
		PL::AssetItemList::const_iterator iter = mSelectedAssetItemList.begin();
		PL::AssetItemList::const_iterator end = mSelectedAssetItemList.end();

		for ( ; iter != end; ++iter )
		{
			if ( (*iter)->GetAssetMediaType() & inMediaType)
			{
				find = true;
				break;
			}
		}

		return find;
	}

	/*
	**
	*/
	void SRAssetSelectionManager::UpdateSelectedMediaURLSet(MediaURLSet const& inMediaURLSet)
	{
		MediaURLSet toRemove;
		set_difference(mSelectedMediaURLSet.begin(), mSelectedMediaURLSet.end(), inMediaURLSet.begin(), inMediaURLSet.end(),
			std::inserter(toRemove, toRemove.begin()));

		MediaURLSet toAdd;
		set_difference(inMediaURLSet.begin(), inMediaURLSet.end(), mSelectedMediaURLSet.begin(), mSelectedMediaURLSet.end(),
			std::inserter(toAdd, toAdd.begin()));

		MediaURLSet::const_iterator iter = toRemove.begin();
		MediaURLSet::const_iterator end = toRemove.end();
		for (; iter != end; ++iter)
		{
			if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(*iter))
			{
				ASL::StationUtils::RemoveListener(
					SRProject::GetInstance()->GetAssetMediaInfoWrapper(*iter)->GetStationID(),
					this);
			}
		}

		mSelectedMediaURLSet = inMediaURLSet;

		iter = toAdd.begin();
		end = toAdd.end();

		for (; iter != end; ++iter)
		{
			if (SRProject::GetInstance()->GetAssetMediaInfoWrapper(*iter))
			{
				ASL::StationUtils::AddListener(
					SRProject::GetInstance()->GetAssetMediaInfoWrapper(*iter)->GetStationID(),
					this);
			}
		}

	}

	/*
	**
	*/
	void SRAssetSelectionManager::TriggerMetaDataSelection(MediaURLSet const& inMediaURLSet)
	{
		if (mSelectedMediaURLSet != inMediaURLSet)
		{
			UpdateSelectedMediaURLSet(inMediaURLSet);

			CalcMediaLocatorAndUpdateInPanel(inMediaURLSet);
		}
	}

	/*
	**
	*/
	void SRAssetSelectionManager::CalcMediaLocatorAndUpdateInPanel(MediaURLSet const& inMediaURLSet)
	{
		typedef std::map<ASL::String, ASL::PathnameList> MediaLocatorMediaURLMap;
		MediaLocatorMediaURLMap mediaLocatorMediaURLMap;
		BOOST_FOREACH(ASL::String const& mediaURL, inMediaURLSet)
		{
			ASL::String mediaLocator;
			PL::AssetMediaInfoPtr assetMediaInfo = SRProject::GetInstance()->GetAssetMediaInfo(mediaURL);

			if (MZ::Utilities::IsEAMediaPath(mediaURL))
			{
				mediaLocator = assetMediaInfo->GetAssetMediaInfoGUID().AsString();
			}
			else
			{
				mediaLocator = assetMediaInfo->GetMediaPath();
			}

			mediaLocatorMediaURLMap[mediaLocator].push_back(mediaURL);
		}


		PL::AssetMediaInfoSelectionList tempMediaInfoSelectionList;
		BOOST_FOREACH(MediaLocatorMediaURLMap::value_type const& pair, mediaLocatorMediaURLMap)
		{		
			PL::AssetMediaInfoSelectionRef mediaInfoSelection(AssetMediaInfoSelection::Create(pair.second));
			tempMediaInfoSelectionList.push_back(mediaInfoSelection);
		}

		ASL::StationUtils::PostMessageToUIThread(
			MZ::kStation_Info, 
			PL::LibraryClipSelectedMessage(tempMediaInfoSelectionList),
			true);
	}

	/*
	**
	*/
	void SRAssetSelectionManager::TriggerLibraryMetaDataSelection()
	{
		ASL::CriticalSectionLock lock(mCriticalSection); //serialize concurrent threads

		PL::AssetItemList::const_iterator iter = mSelectedAssetItemList.begin();
		PL::AssetItemList::const_iterator end = mSelectedAssetItemList.end();
		MediaURLSet mediaURLSet;
		for (; iter != end ; ++iter)
		{
			mediaURLSet.insert((*iter)->GetMediaPath());
		}

		TriggerMetaDataSelection(mediaURLSet);
	}

	/*
	**
	*/
	void SRAssetSelectionManager::OnAssetItemInOutChanged(ASL::Guid inId, BE::IProjectItemRef inProjectItem)
	{
		PL::AssetItemList::const_iterator iter = mSelectedAssetItemList.begin();
		PL::AssetItemList::const_iterator end = mSelectedAssetItemList.end();
		MediaURLSet mediaURLSet;
		for (; iter != end ; ++iter)
		{
			if ( (*iter)->GetAssetGUID() == inId )
			{
				BE::IMasterClipRef masterClip = MZ::BEUtilities::GetMasteClipFromProjectitem(inProjectItem);
				(*iter)->SetCustomInPoint(masterClip->GetMinInPoint(BE::kMediaType_Any));
				(*iter)->SetCustomOutPoint(masterClip->GetMaxOutPoint(BE::kMediaType_Any));
				break;
			}
		}
	}
	
}
