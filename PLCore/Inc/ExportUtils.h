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

#ifndef EXPORTUTILS_H
#define EXPORTUTILS_H

// ASL
#ifndef ASLSTRING_H
#include "ASLString.h"
#endif

#ifndef ASLRESULT_H
#include "ASLResult.h"
#endif

#ifndef ASLFILE_H
#include "ASLFile.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

// Boost
#include "boost/function.hpp"

namespace PL
{

namespace ExportUtils
{

typedef std::pair<ASL::String, ASL::String> PathPair;
typedef std::vector<PathPair> PathPairVector;

// first: init top level name
// second: unique top level name
typedef std::map<ASL::String, ASL::String> TopLevelNameMap;

typedef ASL::String ExportMediaPath;
struct ExportItem
{
	ASL::PathnameList exportFilePaths; // This includes all stuffs for exporting
	ASL::String sourceStem;		// This is used for creating destination path

	// To fix bug #3215030
	// For every selected media, there's a topLevelName, and we can get uniqueTopLevelName from the topLevelName.
	// The uniqueTopLevelName is to avoid copying two sources to the same destination.
	// If we find that topLevelName of current item is different from its unique one, that means there's another
	// clip from different source but with the same topLevelName, so the item's topLevelName
	// should be renamed, and we also should modify the destination path for copy.

	// As in bug #3215030, there will a problem if only use a string to store uniqueTopLevelName:
	// For card spanned clip, it's different from most cases, if there's two cards: card1 and card2, and we 
	// select the clip from card1, then the topLevelName is for the selected clip and we will lose the 
	// unique topLevelName of the clip in card2... When export the items in card2, this will mislead us to 
	// rename all items in card2 because their topLevelName will be always different from card1.

	// To solve this bug, both "card1" and "card2" should be the topLevelName, so we use a map to record
	// the initial topLevelName and its renamed unique topLevelName, with different sourceStem, we can 
	// uniquely locate every topLevelName.
	TopLevelNameMap topLevelNameMap; // Just for two medias which have the same source names; for others this should be empty.
};

typedef std::map<ExportMediaPath, ExportItem> MeidaPathToExportItemMap;

/**
** CopyWithProgress are used only on mac.
**/ 
struct ExportProgressData
{
	boost::function<bool (ASL::UInt64)> UpdateProgress;
	ASL::UInt64 totalProgress;
	ASL::UInt64 currentProgress;
};

ASL::Result CopyWithProgress(
	const ASL::String& inSourcePath,
	const ASL::String& inDestinationPath,
	ASL::File::ASLCopyProgressFunc inProgressFunc,
	void* inProgressData);

/**
** Replace media path in project file. It is used when export a media to a new destination.
**/
void ReplaceMeidaPath(const ASL::String &inProjectFilePath,
	const ASL::String &inDestProjectDir,
	const PathPairVector &inMediaPaths);

/**
** The topLevelName equals the topLevel name of the path that is meidaPath subtracts sourceStem. 
** Case 1: meidaPath: "C:\Test\1.mp4"; sourceStem: "C:\Test", then topLevelName: 1.mp4
** Case 2: meidaPath: "C:\Users\test\Desktop\Test\XDCAMEX\BPAV\CLPR\130_0014_01\130_0014_01.MP4"
**			sourceStem: "C:\Users\test\Desktop\Test", then topLevelName: XDCAMEX
** Case 3: mediaPath: "C:\BPAV\CLPR\130_0014_01\130_0014_01.MP4"
**			sourceStem: "Volumes:", then topLevleName: C
*/
ASL::String GetTopLevelName(
	ASL::String const& inMeidaPath,
	ASL::String const& inSourceStem);

/**
** This function will erase the possible duplicated files for different medias in the same source folder. 
*/
ASL::Result BuildUniqueExportFilesMap(
	std::set<ASL::String> const& inExportMediaFiles,
	MeidaPathToExportItemMap& outMediaPathToExportItem,
	boost::function<bool (ASL::UInt64)> const& inUpdateProgressFn = boost::function<bool (ASL::UInt64)>());

/**
**
*/
ASL::String BuildUnqiueDestinationPath(
	ASL::String const& inMediaPath,
	ASL::String const& inDestRoot,
	ExportItem const& inExportItem);

/**
** For two medias with the same source name in different source, we will make their top level directory unique.
*/
void UniqueSameSourceNamePaths(
	MeidaPathToExportItemMap& ioMediaPathToExportItem, 
	std::vector<ASL::String>& outRenameMediaPath);

/**
**
*/
ASL::Result UniquneSameSourceNamePathsWithPrompt(MeidaPathToExportItemMap& ioMediaPathToExportItem);

/**
**
*/
const ASL::UInt32 nMaxWarningMessageNum = 10;

} //namespace ExportUtils

} //namespace PL

#endif 
