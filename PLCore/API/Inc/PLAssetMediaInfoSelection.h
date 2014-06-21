/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2011 Adobe Systems Incorporated                       */
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

#ifndef PLASSETMEDIAINFOSELECTION_H
#define PLASSETMEDIAINFOSELECTION_H

#ifndef PLASSETMEDIAINFO_H
#include "PLAssetMediaInfo.h"
#endif

#ifndef PLMEDIAMETADATAHELPER_H
#include "PLMediaMetadataHelper.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef BEMEDIA_H
#include "BEMedia.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

namespace PL
{

	ASL_DEFINE_CLASSREF(AssetMediaInfoSelection, BE::IMediaMetaData);
	ASL_DEFINE_IMPLID(AssetMediaInfoSelection, 0xe3de65d, 0x20eb, 0x4d30, 0x87, 0xdd, 0xb3, 0x70, 0xa3, 0x4c, 0x9, 0xa7);

	typedef std::list<AssetMediaInfoSelectionRef> AssetMediaInfoSelectionList;
	ASL_DECLARE_MESSAGE_WITH_1_PARAM(LibraryClipSelectedMessage, AssetMediaInfoSelectionList);

	class AssetMediaInfoSelection
		:
		public PL::MediaMetaDataHelper,
		public ASL::Listener
	{
	public:

		/*
		**
		*/
		ASL_BASIC_CLASS_SUPPORT(AssetMediaInfoSelection);
		ASL_QUERY_MAP_BEGIN
			ASL_QUERY_ENTRY(AssetMediaInfoSelection)
			ASL_QUERY_ENTRY(BE::IMediaMetaData)
		ASL_QUERY_MAP_END


		ASL_MESSAGE_MAP_DECLARE();

		/*
		**
		*/
		PL_EXPORT
		AssetMediaInfoSelection();

		/*
		**
		*/
		PL_EXPORT
		~AssetMediaInfoSelection();

		/*
		**
		*/
		void Init(
				ASL::PathnameList const& inMediaURLList);

		/*
		**
		*/
		PL_EXPORT
		static AssetMediaInfoSelectionRef Create(
										ASL::PathnameList const& inMediaURLList);
	
		/*
		**
		*/
		PL_EXPORT
		ASL::String GetDisplayFilePath() const;

		/*
		**
		*/
		PL_EXPORT
		bool IsEAMediaInfo() const;

		/*
		**	IMediaMetaData methods
		*/
		virtual ASL::Result ReadMetaDatum(
								const MF::BinaryData& inPrefs,
								BE::MetaDataType inType,
								void* inID,
								MF::BinaryData& outDatum);

		/*
		**	IMediaMetaData methods
		*/
		virtual ASL::Result WriteMetaDatum(
								const MF::BinaryData& inPrefs,
								BE::MetaDataType inType,
								void* inID,
								const MF::BinaryData& inDatum,
								bool inAlwaysPostToUIThread = true,
								bool inRefreshFile = true,
								ASL::SInt32* outRequestID = NULL,
								const dvametadata::MetaDataChangeList& inChangeList = dvametadata::MetaDataChangeList());

		/*
		**
		*/
		virtual bool CanWriteXMPToFile() const;


	private:

		/*
		**
		*/
		ASL::Result SaveMetadata(MF::BinaryData const& inBinaryData);

	private:

		ASL::PathnameList		mMediaURLList;
	};
}

#endif
