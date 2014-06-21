/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2012 Adobe Systems Incorporated                       */
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

#ifndef MZ_WORKSPACE_H
#define MZ_WORKSPACE_H

// Local
#include "PLExport.h"

// DVA 
#include "dvacore/utility/StringUtils.h"
#include "dvaworkspace/workspace/Workspace.h"
#include "dvaworkspace/workspace/WorkspacesManager.h"
#include "dvacore/proplist/PropList.h"
#include "dvacore/proplist/PropListUtils.h"
#include "dvacore/utility/FileUtils.h"
#include "dvaui/utility/ResourceUtils.h"
#include "dvaworkspace/workspace/WorkspaceBuilder.h"
#include "dvaworkspace/workspace/WorkspaceMenuHelper.h"

// ASL
#include "ASLAutoPtr.h"


namespace PL
{
	//	WorkSpace IDs 
	PL_EXPORT extern const char* kLoggingViewWorkspaceStringID;
	PL_EXPORT extern const char* kListViewWorkspaceStringID;
	PL_EXPORT extern const char* kRoughCutWorkspaceStringID;

	//	currently sequence workspace is not available
	PL_EXPORT extern const size_t	kBuildInWorkspaceCount;

//------------------------------------------------
//------------------------------------------------
//			System workspace
//------------------------------------------------
//------------------------------------------------
class PL_CLASS_EXPORT PremSystemWorkspaceProvider 
	:
	public dvaworkspace::workspace::SystemWorkspaceProviderBase
{
public:
	PL_EXPORT
	PremSystemWorkspaceProvider(const dvacore::UTF16String& inSystemFolder);
	virtual ~PremSystemWorkspaceProvider();

	PL_EXPORT
	virtual dvacore::UTF16String GetFolder() const ;

private:
	dvacore::UTF16String mSystemFolder;
};

//------------------------------------------------
//------------------------------------------------
//			BuildIn workspace
//------------------------------------------------
//------------------------------------------------

/*
**
*/
class PL_CLASS_EXPORT PremBuiltInWorkspaceProvider
	:
	public dvaworkspace::workspace::BuiltInWorkspaceProviderBase
{
public:
	/*
	**
	*/
	PL_EXPORT
	PremBuiltInWorkspaceProvider(const dvaui::utility::ResourceManager* inResManager);
	
	/*
	**
	*/
	PL_EXPORT
	virtual bool SupportsLocalizedWorkspaceNames();
	
	/*
	**
	*/
	PL_EXPORT
	virtual void GetWorkspaceStringKeys(
		std::list<dvacore::UTF8String>& outStringKeys);

	/*
	**	Return the user-visible names of the built-in workspace layouts
	*/
	PL_EXPORT
	virtual void GetWorkspaces(
		std::vector<dvacore::UTF16String>& outWorkspaces);

	PL_EXPORT
	bool FindWorkspace(const dvacore::UTF16String& inWorkspaceName);

	/*
	**
	*/
	PL_EXPORT
	virtual std::auto_ptr<dvaworkspace::workspace::WorkspaceBuilder> GetWorkspaceBuilder(
		const dvacore::UTF16String& inWorkspaceName,
		dvaworkspace::workspace::Workspace&	inWorkspace);

	PL_EXPORT
	const dvacore::UTF16String& GetDefaultWorkspaceName() const;

private:
	dvacore::UTF16String mLoggingViewWorkspaceName;
	dvacore::UTF16String mListViewWorkspaceName;
	dvacore::UTF16String mRoughCutWorkspaceName;

	const dvaui::utility::ResourceManager* mResourceManager;
};

PL_EXPORT dvacore::UTF16String GetSystemWorkspaceFolderPath();
PL_EXPORT void GetWorkspaceDirs(
				dvacore::UTF16String& outUserWorkspacePath,
				dvacore::UTF16String& outArchivedWorkspacePath,
				dvacore::UTF16String& outSystemWorkspacePath,
				dvacore::UTF16String& outWorkspaceManagerConfigFilePath);

PL_EXPORT extern dvaworkspace::workspace::Manager* sDVAWSMgr;
PL_EXPORT extern dvaworkspace::workspace::Workspace* sDVAWorkspace;
PL_EXPORT extern PremBuiltInWorkspaceProvider*	sDVABuiltInProvider;
PL_EXPORT extern PremSystemWorkspaceProvider*	sDVASystemProvider;

PL_EXPORT bool EnableDeleteWorkspace();


PL_EXPORT bool SetCurrentWorkspace(
				const dvacore::UTF16String& inWorkspaceName);

PL_EXPORT bool RevertWorkspace(
				const dvacore::UTF16String& inWorkspaceName);

PL_EXPORT bool IsValidPremiereWorkspace(
				dvaworkspace::workspace::Workspace* inWorkspace);

} // namespace PL

#endif
