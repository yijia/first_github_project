/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2011 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

#pragma once

#ifndef PLASSETMEDIAINFO_H
#define PLASSETMEDIAINFO_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

// ASL
#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef ASLDATE_H
#include "ASLDate.h"
#endif

// dva
#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

// boost
#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

namespace PL
{

	#ifndef XMPText
	typedef boost::shared_ptr<ASL::StdString> XMPText;
	#endif

	#ifndef XMPTextVector
	typedef std::vector<XMPText> XMPTextVector;
	#endif
	
	/*
	**
	*/
	enum AssetMediaType
	{
		kAssetLibraryType_Unset		 = 0,
		kAssetLibraryType_MasterClip,
		kAssetLibraryType_SubClip,
		kAssetLibraryType_RoughCut,
		kAssetLibraryType_RCSubClip,
	};


	class AssetMediaInfo;
	typedef boost::shared_ptr<AssetMediaInfo> AssetMediaInfoPtr;
	typedef std::map<ASL::String, AssetMediaInfoPtr> AssetMediaInfoMap;
	typedef std::list<AssetMediaInfoPtr> AssetMediaInfoList;
	
	/*
	**
	*/
	class AssetMediaInfo
	{
	public:

		/*
		**
		*/
		PL_EXPORT
		static AssetMediaInfoPtr Create(
							AssetMediaType		inType,
							ASL::Guid	const&	inGuid,
							ASL::String	const&	inMediaPath,
							XMPText				inXMP,
							ASL::String const&  inFileContent = ASL::String(),
							ASL::String	const&	inCustomMetadata = ASL::String(),
							ASL::String const&  inCreateTime = ASL::String(),
							ASL::String const&  inLastModifyTime = ASL::String(),
							ASL::String const&  inTimeBase = ASL::String(),
							ASL::String const&  inNtsc = ASL::String(),
							ASL::String const&  inAliasName = ASL::String(),
							bool inNeedSavePreCheck = false);

		/*
		**
		*/
		PL_EXPORT
		static AssetMediaInfoPtr CreateMasterClipMediaInfo(
							ASL::Guid	const&	inGuid,
							ASL::String	const&	inMediaPath,
							ASL::String const&  inAliasName,
							XMPText				inXMP,
							ASL::String	const&	inCustomMetadata = ASL::String(),
							ASL::String const&  inCreateTime = ASL::String(),
							ASL::String const&  inLastModifyTime = ASL::String());

		PL_EXPORT
		static AssetMediaInfoPtr CreateRoughCutMediaInfo(
			ASL::Guid	const&	inGuid,
			ASL::String	const&	inMediaPath,
			ASL::String const&  inFileContent = ASL::String(),
			ASL::String const&  inCreateTime = ASL::String(),
			ASL::String const&  inLastModifyTime = ASL::String(),
			ASL::String const&  inTimeBase = ASL::String(),
			ASL::String const&  inNtsc = ASL::String());

		/*
		**
		*/
		PL_EXPORT
		~AssetMediaInfo();

		/*
		**
		*/
		PL_EXPORT
		AssetMediaType GetAssetMediaType() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetMediaPath() const;

		/*
		**
		*/
		PL_EXPORT
		XMPText GetXMPString() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetCustomMetadata() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetFileContent() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::Guid GetAssetMediaInfoGUID() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetCreateTime() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetLastModifyTime() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetRateTimeBase() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetRateNtsc() const;

		/*
		**
		*/
		PL_EXPORT
		ASL::String GetAliasName() const;

		/*
		**
		*/
		PL_EXPORT
		void SetAliasName(const ASL::String& inNewAliasName);

		/*
		**
		*/
		PL_EXPORT
		bool GetNeedSavePreCheck() const;

		/*
		**
		*/
		PL_EXPORT
		void SetNeedSavePreCheck(bool isNeedSavePreCheck);

		/*
		**
		*/
		PL_EXPORT
		void SetForceLoadFromLocalDisk(ASL::SInt32 inForceLoadFromLocalDisk);

		/*
		**
		*/
		PL_EXPORT
		ASL::SInt32	GetForceLoadFromLocalDisk() const;

		/*
		**
		*/
		PL_EXPORT
		static bool IsAssetMediaInfoSameGuid(
			AssetMediaInfoPtr const& inLeftMediaInfo,
			AssetMediaInfoPtr const& inRightMediaInfo);

	private:

		/*
		**
		*/
		AssetMediaInfo(	
					AssetMediaType		inType,
					ASL::Guid	const&	inGuid,
					ASL::String	const&	inMediaPath,
					XMPText				inXMP,
					ASL::String const&  inFileContent = ASL::String(),
					ASL::String	const&	inCustomMetadata = ASL::String(),
					ASL::String const&  inCreateTime = ASL::String(),
					ASL::String const&  inLastModifyTime = ASL::String(),
					ASL::String const&  inTimeBase = ASL::String(),
					ASL::String const&  inNtsc = ASL::String(),
					ASL::String const&  inAliasName = ASL::String(),
					bool				inNeedSavePreCheck = false,
					ASL::SInt32			inForceLoadFromLocalDisk = -1);
	private:
		AssetMediaType			mType;

		//	AssetMediaInfoGUID, It is not necessary that it is equal to masterClip ID.
		ASL::Guid				mAssetMediaInfoGUID;

		//	AssetMediaInfo's MediaLocator 
		//  Local mode it is file path
		//	EA mode it is MediaLocator
		ASL::String				mMediaPath;

		//	XMP String Data
		XMPText					mXMP;

		//	CustomMetadata String
		ASL::String				mCustomMetadata;

		//	File Data mainly for RC, reserve for logging clip.
		ASL::String				mFileContent;

		//	File Create time
		ASL::String				mCreateTime;

		//	Last Modification time
		ASL::String				mLastModificationTime;

		//	timebase
		ASL::String				mRateTimeBase;

		//  ntsc
		ASL::String				mRateNtsc;

		// alias Name (currently only available  for master clip)
		ASL::String				mAliasName;

		// forceLoadFromLocalDisk (should only be used by our client, not core side)
		ASL::SInt32				mForceLoadFromLocalDisk;

		// [TRICKY] We need check if save action can be performed successfully before send save notification 
		//  for client which want to save xmp to local disk. Need figure out a better solution for this after CS6.
		bool					mNeedSavePreCheck;
	};
}

#endif
