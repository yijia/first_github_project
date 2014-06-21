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

//	MZ
#include "PLClipFinderUtils.h"
#include "PLUtilities.h"

// ASL
#include "ASLPathUtils.h"

// boost
#include "boost/foreach.hpp"

namespace PL
{
	namespace
	{
		/*
		**
		*/
		template <typename T>
		inline bool Castable(
			adobe::any_regular_t const& any)
		{
			return !(any.type_info() != adobe::type_info<typename adobe::promote<T>::type>());
		}


//		int ParserPathBatch(
//				MZ::IIngestJobItem::Batch_t& useBatchs,
//				bool const inImport,
//				PLMBC::Provider::Datum_t const& datum)
//		{
//			int result = 0;
//
//			if (Castable<PLMBC::Provider::Path_t>(datum))
//			{
//				PLMBC::Provider::Path_t const& path = datum.cast<PLMBC::Provider::Path_t>();
//				if (!path.empty())
//				{
//					ASL::String pathStr(ASL::MakeString(path));
//#if ASL_TARGET_OS_WIN
//					if (ASL::PathUtils::IsNormalizedPath(pathStr))
//					{
//						pathStr = ASL::PathUtils::Win::StripUNC(pathStr);
//					}
//#endif
//					
//					MZ::IIngestJobItem::BatchItem batchItem = MZ::IIngestJobItem::BatchItem(pathStr, inImport);
//					useBatchs.push_back(batchItem);
//					++result;
//					
//		
//				}
//			}
//			else if (Castable<PLMBC::Provider::DatumVector_t>(datum))
//			{
//				PLMBC::Provider::DatumVector_t const& datumVector = datum.cast<PLMBC::Provider::DatumVector_t>();
//				PLMBC::Provider::DatumVector_t::const_iterator i = datumVector.begin();
//				PLMBC::Provider::DatumVector_t::const_iterator const end = datumVector.end();
//
//				for (; i != end; ++i)
//				{
//					result += ParserPathBatch(useBatchs, inImport, *i);
//				}
//			}
//			return result;
//		}
//
		/*
		**
		*/
		bool ParserName(
					ASL::String& ClipName,
					PLMBC::Provider::Datum_t const& datum)
		{

			if (Castable<std::string>(datum))
			{
				std::string name = datum.cast<std::string>();
				if (!name.empty())
				{
					ClipName = ASL::MakeString(name);
					return true;
				}
			}
			return false;
		}

		/*
		**
		*/
		ASL::String GetSourceStem(ASL::String const & inFolderPath)
		{
			ASL::String sourceStem = inFolderPath;
#if ASL_TARGET_OS_WIN
			if (!sourceStem.empty() && ASL::PathUtils::IsNormalizedPath(sourceStem))
			{
				sourceStem = ASL::PathUtils::Win::StripUNC(sourceStem);
			}
#endif


			// [Note] If the files been transferred are logical clips, we need transfer their parent
			// directory too. "CombinePaths" can't be used here, because the wrong action on MAC
			ASL::String strDrive;
			ASL::String strParentDir;
			ASL::String strName;
			ASL::PathUtils::SplitPath(sourceStem, &strDrive, &strParentDir, &strName, 0);
			if (!strName.empty())
			{
				sourceStem = strDrive + strParentDir;
			}
			else
			{
				sourceStem = ASL::MakeString(PLMBC::kVolumeListPath);
			}

			if (ASL::PathUtils::HasTrailingSlash(sourceStem))
			{
				sourceStem = ASL::PathUtils::RemoveTrailingSlash(sourceStem);
			}

			return sourceStem;
		}

	}//	End namespace

/*
**
*/
void ExtractPaths(const PLMBC::Provider::Datum_t& inDatum, ASL::PathnameList& outPaths)
{
	PLMBC::Provider::Path_t extraPath;
	if (inDatum.cast(extraPath))
	{
		outPaths.push_back(ASL::MakeString(extraPath));
		return;
	}

	PLMBC::Provider::DatumVector_t datumVector;
	if (inDatum.cast(datumVector))
	{
		BOOST_FOREACH(PLMBC::Provider::DatumVector_t::value_type& datum, datumVector)
		{
			ExtractPaths(datum, outPaths);
		}
	}
}

/*
**
*/
void GetLogicalClipFromMediaPath(
				ASL::String const& inMediaPath,
				ASL::String& outSourceStem,
				ASL::PathnameList& outFileList,
				ASL::String& outClipName,
				ASL::String& outProviderID)
{
	ASL::String mediakey;
	ASL::String folderPath;

	PLMBC::Provider::Value_t value = PLMBC::GetRelatedFiles(
												inMediaPath,
												mediakey,
												folderPath,
												outProviderID);

	PLMBC::Provider::Value_t::const_iterator valueVideoPathIter = value.find(PLMBC::kPathField);
	PLMBC::Provider::Value_t::const_iterator valueExtraPathIter = value.find(PLMBC::kExtraPathField);
	PLMBC::Provider::Value_t::const_iterator valueNameiter = value.find(PLMBC::kNameField);

	ExtractPaths(valueVideoPathIter->second, outFileList);
	ExtractPaths(valueExtraPathIter->second, outFileList);

	// for file-based media, maybe there is side-car file which is needed to be put into extra path list too.
	if (!PL::Utilities::IsFolderBasedClip(inMediaPath))
	{
		ASL::PathnameList supportedList = PL::Utilities::GetClipSupportedPathList(inMediaPath);
		ASL::PathnameList exceptPathList;
		exceptPathList.push_back(inMediaPath);
		PL::Utilities::MergePathList(outFileList, supportedList, exceptPathList);
	}

	if (!ParserName(outClipName, valueNameiter->second))
	{
		outClipName = ASL::PathUtils::GetFullFilePart(inMediaPath);
	}

	// To fix bug #3186705, the providerID should not be empty.
	// For Win, com.adobe.Premiere.WinProviderDirectory means default provider
	// For Mac, com.adobe.Premiere.MacProviderDirectory means default provider
#if ASL_TARGET_OS_WIN
	ASL::String const& defaultProviderID = ASL_STR("com.adobe.Premiere.WinProviderDirectory");
#elif ASL_TARGET_OS_MAC
	ASL::String const& defaultProviderID = ASL_STR("com.adobe.Premiere.MacProviderDirectory");
#endif

	if (outProviderID.empty())
	{
		// This is a healthy assert and we keep the original code to get source stem, but outProviderID has no chance to be empty.
		DVA_ASSERT_MSG(0, "Empty providerID!");
		outSourceStem = folderPath;
	}
	else if (defaultProviderID == outProviderID)
	{
		outSourceStem = folderPath;
	}
	else
	{
		outSourceStem = GetSourceStem(folderPath);
	}
}

/*
**
*/
void GetLogicalClipNameFromMediaPath(
	ASL::String const& inMediaPath,
	ASL::String& outClipName)
{
	ASL::String mediakey;
	ASL::String folderPath;
	ASL::String providerID;

	PLMBC::Provider::Value_t value = PLMBC::GetRelatedFilesWithoutExtraFiles(
		inMediaPath,
		mediakey,
		folderPath,
		providerID);

	PLMBC::Provider::Value_t::const_iterator valueNameiter = value.find(PLMBC::kNameField);
	if (!ParserName(outClipName, valueNameiter->second))
	{
		outClipName = ASL::PathUtils::GetFullFilePart(inMediaPath);
	}
}

}