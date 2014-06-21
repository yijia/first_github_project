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

// Self
#include "Workspace/PLWorkspace.h"

// DVA
#include "dvacore/utility/StringUtils.h"
#include "dvacore/config/Localizer.h"
#include "dvaworkspace/workspace/WorkspaceSerializer.h"
#include "dvaworkspace/workspace/WorkspaceUtils.h"

// ASL
#include "ASLDirectoryRegistry.h"
#include "ASLDirectories.h"
#include "ASLPathUtils.h"
#include "ASLMacros.h"

// UIF
#include "UIFInlineEditText.h"
#include "UIFApplication.h"

// PRM
#include "PRMApplicationTarget.h"

namespace PL
{

	//	WorkSpace IDs 
	const char* kLoggingViewWorkspaceStringID = "$$$/Prelude/Mezzanine/LoggingWorkspace=Logging";
	const char* kListViewWorkspaceStringID = "$$$/Prelude/Mezzanine/ListWorkspace=List";
	const char* kRoughCutWorkspaceStringID = "$$$/Prelude/Mezzanine/RoughCutWorkspace=Rough Cut";

	//	currently sequence workspace is not available
	const size_t kBuildInWorkspaceCount = 3;

	dvaworkspace::workspace::Manager* sDVAWSMgr = NULL;
	dvaworkspace::workspace::Workspace* sDVAWorkspace = NULL;
	PremBuiltInWorkspaceProvider* sDVABuiltInProvider(NULL);
	PremSystemWorkspaceProvider*	sDVASystemProvider(NULL);

	//	The XML filenames corresponding to each of the built-in workspace layouts
	const dvacore::UTF8Char* kLoggingTimeWorkspaceXMLName = dvacore::utility::AsciiToUTF8("LOGGINGTIMEWORKSPACELAYOUT");
	const dvacore::UTF8Char* kLoggingListWorkspaceXMLName = dvacore::utility::AsciiToUTF8("LOGGINGLISTWORKSPACELAYOUT");
	const dvacore::UTF8Char* kRoughCutWorkspaceXMLName = dvacore::utility::AsciiToUTF8("ROUGHCUTWORKSPACELAYOUT");

		
	//------------------------------------------------
	//------------------------------------------------
	//			System workspace
	//------------------------------------------------
	//------------------------------------------------
	PremSystemWorkspaceProvider::PremSystemWorkspaceProvider(const dvacore::UTF16String& inSystemFolder) 
		:
		mSystemFolder(inSystemFolder)
	{

	}

	PremSystemWorkspaceProvider::~PremSystemWorkspaceProvider() 
	{

	}

	dvacore::UTF16String PremSystemWorkspaceProvider::GetFolder() const 
	{ 
		return mSystemFolder; 
	}

	//------------------------------------------------
	//------------------------------------------------
	//			BuildIn workspace
	//------------------------------------------------
	//------------------------------------------------

	/*
	**
	*/
	PremBuiltInWorkspaceProvider::PremBuiltInWorkspaceProvider(const dvaui::utility::ResourceManager* inResManager)
		:
		mResourceManager(inResManager)
	{
		mLoggingViewWorkspaceName = dvacore::ZString(kLoggingViewWorkspaceStringID);
		mListViewWorkspaceName = dvacore::ZString(kListViewWorkspaceStringID);
		mRoughCutWorkspaceName = dvacore::ZString(kRoughCutWorkspaceStringID);
	}
	
	/*
	**
	*/
	bool PremBuiltInWorkspaceProvider::SupportsLocalizedWorkspaceNames()
	{
		return true;
	}
	
	/*
	**
	*/
	void PremBuiltInWorkspaceProvider::GetWorkspaceStringKeys(
		std::list<dvacore::UTF8String>& outStringKeys)
	{
		outStringKeys.resize(0);
		outStringKeys.push_back(dvacore::utility::AsciiToUTF8(kLoggingViewWorkspaceStringID));
		outStringKeys.push_back(dvacore::utility::AsciiToUTF8(kListViewWorkspaceStringID));
		outStringKeys.push_back(dvacore::utility::AsciiToUTF8(kRoughCutWorkspaceStringID));
		DVA_ASSERT(kBuildInWorkspaceCount == outStringKeys.size());
	}

	/*
	**	Return the user-visible names of the built-in workspace layouts
	*/
	void PremBuiltInWorkspaceProvider::GetWorkspaces(
		std::vector<dvacore::UTF16String>& outWorkspaces)
	{
		outWorkspaces.resize(0);
		outWorkspaces.push_back(mLoggingViewWorkspaceName);
		outWorkspaces.push_back(mListViewWorkspaceName);
		outWorkspaces.push_back(mRoughCutWorkspaceName);
		DVA_ASSERT(kBuildInWorkspaceCount == outWorkspaces.size());
	}

	bool PremBuiltInWorkspaceProvider::FindWorkspace(const dvacore::UTF16String& inWorkspaceName)
	{
		if ( inWorkspaceName != mLoggingViewWorkspaceName &&
			 inWorkspaceName != mListViewWorkspaceName &&
			 inWorkspaceName != mRoughCutWorkspaceName )
		{
			return false;
		}

		return true;
	}

	/*
	**
	*/
	std::auto_ptr<dvaworkspace::workspace::WorkspaceBuilder> PremBuiltInWorkspaceProvider::GetWorkspaceBuilder(
		const dvacore::UTF16String& inWorkspaceName,
		dvaworkspace::workspace::Workspace&	inWorkspace)
	{
		std::auto_ptr<dvaworkspace::workspace::WorkspaceBuilder> builder;

		if (mResourceManager != NULL)
		{
			dvacore::UTF8String resName;

			//	Map the user-visible workspace name to it's corresponding XML resource
			if (inWorkspaceName == mLoggingViewWorkspaceName)
			{
				resName = kLoggingTimeWorkspaceXMLName;
			}
			else if (inWorkspaceName == mListViewWorkspaceName)
			{
				resName = kLoggingListWorkspaceXMLName;
			}
			else if (inWorkspaceName == mRoughCutWorkspaceName)
			{
				resName = kRoughCutWorkspaceXMLName;
			}

			if (! resName.empty())
			{
				//	Load the xml for the requested layout
				const dvacore::UTF16String resType = dvacore::utility::AsciiToUTF16("xml");
				std::vector<char> xmlBuffer;

				mResourceManager->GetBinary(
					dvacore::utility::UTF8to16(resName), resType, &xmlBuffer);

				const dvacore::UTF8String xmlString(xmlBuffer.begin(), xmlBuffer.end());

				//	Create a builder and load the layout into it
				builder = std::auto_ptr<dvaworkspace::workspace::WorkspaceBuilder>(
					new dvaworkspace::workspace::WorkspaceBuilder(inWorkspace, true));
				dvacore::proplist::utils::ReadPropList(xmlString, builder->GetPropList());

				//	Set the main window size to the rect of the primary monitor
				dvacore::geom::Rect32 primaryMonitorRect;
				inWorkspace.DVA_GetMonitorInfo()->GetMonitorRect(0, primaryMonitorRect);
				dvaworkspace::workspace::SetMainWindowGlobalRect(
					builder->GetPropList(), primaryMonitorRect);
			}
		}

		DVA_ASSERT(builder.get() != NULL);

		return builder;
	}

	const dvacore::UTF16String& PremBuiltInWorkspaceProvider::GetDefaultWorkspaceName() const
	{
		return mLoggingViewWorkspaceName;
	}

	//------------------------------------------------------------------------

	dvacore::UTF16String GetSystemWorkspaceFolderPath()
	{
		dvacore::filesupport::Dir sharedDocsDir(dvacore::filesupport::Dir::SharedDocsDir());

		if ( !sharedDocsDir.Exists() )
		{
			return DVA_STR("");
		}

		dvacore::filesupport::Dir userAdobeDir(sharedDocsDir, DVA_STR("Adobe"));
		if (!userAdobeDir.Exists()) 
		{
			return DVA_STR("");
		}

		dvacore::filesupport::Dir userPreludeDir(userAdobeDir, DVA_STR("Prelude"));
		if (!userPreludeDir.Exists()) 
		{
			return DVA_STR("");
		}

		dvacore::UTF16String appVersion(ASL::MakeString(PRM::GetApplicationVersion()));
		dvacore::filesupport::Dir userPreludeVersionDir(userPreludeDir, appVersion);
		if (!userPreludeVersionDir.Exists()) 
		{
			return DVA_STR("");
		}

		dvacore::filesupport::Dir sysWorkspaceDir(userPreludeVersionDir, DVA_STR("Layouts"));
		if (!sysWorkspaceDir.Exists()) 
		{
			return DVA_STR("");
		}

		return sysWorkspaceDir.FullPath();
	}

	/*
	**	[NOTE] No trailing separators after the directory names (the
	**	dva workspace manager doesn't expect them).
	*/
	void GetWorkspaceDirs(
		dvacore::UTF16String& outUserWorkspacePath,
		dvacore::UTF16String& outArchivedWorkspacePath,
		dvacore::UTF16String& outSystemWorkspacePath,
		dvacore::UTF16String& outWorkspaceManagerConfigFilePath)
	{
		outSystemWorkspacePath = GetSystemWorkspaceFolderPath();

		outUserWorkspacePath = ASL::DirectoryRegistry::FindDirectory(
			ASL::MakeString(ASL::kUserLayoutsDirectory));

		bool dirExists = ASL::PathUtils::PreparePathForWrite(outUserWorkspacePath);
		DVA_ASSERT(dirExists);
		ASL_UNUSED(dirExists);

		outWorkspaceManagerConfigFilePath = 
			ASL::PathUtils::CombinePaths(outUserWorkspacePath, dvacore::utility::AsciiToUTF16("WorkspaceConfig.xml"));

		outUserWorkspacePath = ASL::PathUtils::RemoveTrailingSlash(outUserWorkspacePath);

		outArchivedWorkspacePath = ASL::DirectoryRegistry::FindDirectory(
			ASL::MakeString(ASL::kLayoutsDirectory));

		dirExists = ASL::PathUtils::PreparePathForWrite(outArchivedWorkspacePath);
		DVA_ASSERT(dirExists);
		ASL_UNUSED(dirExists);

		outArchivedWorkspacePath = ASL::PathUtils::RemoveTrailingSlash(outArchivedWorkspacePath);
	}

	bool EnableDeleteWorkspace()
	{
		if ( sDVAWSMgr && sDVABuiltInProvider )
		{
			const dvacore::UTF16String currWSName = sDVAWSMgr->GetCurrentWorkspace();
			std::vector<dvacore::UTF16String> buildInWorkspaces;
			sDVABuiltInProvider->GetWorkspaces(buildInWorkspaces);

			std::vector<dvacore::UTF16String> systemWorkspaces;
			if (sDVASystemProvider)
				sDVASystemProvider->GetWorkspaces(systemWorkspaces);

			std::map<dvacore::UTF16String, dvacore::UTF16String> tempMap;
			std::vector<dvacore::UTF16String>::iterator it;

			// Remove the duplicate workspace in system workspace and build-in workspace
			for (it = buildInWorkspaces.begin(); it != buildInWorkspaces.end(); ++it)
			{
				tempMap[*it] = DVA_STR("");
			}
			for (it = systemWorkspaces.begin(); it != systemWorkspaces.end(); ++it)
			{
				tempMap[*it] = DVA_STR("");
			}

			std::size_t cannotDeleteWorkspaceCount = tempMap.size();

			// If the current workspace is not in the Build-in and System workspace, 
			// it should be an user workspace and that cannot be deleted. So count should increase by 1.
			if ( !sDVABuiltInProvider->FindWorkspace(currWSName) && (!sDVASystemProvider || !sDVASystemProvider->FindWorkspace(currWSName)) )
			{
				cannotDeleteWorkspaceCount++;
			}

			return sDVAWSMgr->GetNumberWorkspaces() > cannotDeleteWorkspaceCount;
		}

		return false;
	}


	/**
	**	Do a quick check for common problems with the current workspace (e.g. missing
	**	data or no visible panels). Premiere workspaces appear to become corrupt occasionally,
	**	though we don't know why for sure yet.
	*/
	bool IsValidPremiereWorkspace(
		dvaworkspace::workspace::Workspace* inWorkspace)
	{
		if (inWorkspace == NULL)
		{
			return false;
		}

		dvaworkspace::workspace::TopLevelWindow* topLevelWindow =
			inWorkspace->GetMainTopLevelWindow();

		if (topLevelWindow == NULL)
		{
			return false;
		}

		dvaworkspace::workspace::Splitter* rootSplitter = dynamic_cast<dvaworkspace::workspace::Splitter*>(
			topLevelWindow->GetLayoutManager()->GetRootSplitter());

		if (rootSplitter == NULL)
		{
			return false;
		}

		if (! (rootSplitter->IsLeftOrTopSubtreeVisible() || rootSplitter->IsRightOrBottomSubtreeVisible()))
		{
			//	If neither subtree of the root splitter is visible then no windows are visible. This seems to be
			//  a common feature of corrupt workspaces and its hard to imagine a user intentionally saving such
			//  a workspace.
			return false;
		}

		return true;
	}

	/*
	**	Revert a workspace to its canonical state. If the workspace is one of the built-ins
	**	and its archived version appears to be corrupt, this method will revert it to its
	**	original "factory default" setting.
	*/
	bool RevertWorkspace(
		const dvacore::UTF16String& inWorkspaceName)
	{
		DVA_ASSERT(sDVAWSMgr != NULL);

		//[TODO]Huasheng
		UIF::InlineEditText::Unfocus();
		sDVAWSMgr->RevertWorkspace(inWorkspaceName);

		bool isValidWorkspace = IsValidPremiereWorkspace(sDVAWorkspace);

		if (! isValidWorkspace)
		{
			DVA_ASSERT_MSG(false, "Workspace still invalid after call to RevertWorkspace()");

			std::vector<dvacore::UTF16String> workspaceNames;
			sDVABuiltInProvider->GetWorkspaces(workspaceNames);

			if (std::find(workspaceNames.begin(), workspaceNames.end(), inWorkspaceName) != workspaceNames.end())
			{
				//	This workspace has the same name as a built-in workspace, but is invalid.
				//	Revert to the original "factory installed" definition of the workspace with that name.
				std::auto_ptr<dvaworkspace::workspace::WorkspaceBuilder> builder =
					sDVABuiltInProvider->GetWorkspaceBuilder(inWorkspaceName, *sDVAWorkspace);

				if (builder->UsePropList()) {
					const bool sizeMainframe = false;
					const bool restoreMaximized = true;
					const bool forceConstraints = false;
					const bool wasRestored = dvaworkspace::workspace::RestoreWorkspace(
						*sDVAWorkspace, builder->GetPropList(), sizeMainframe, restoreMaximized, forceConstraints);
					DVA_ASSERT(wasRestored);

					isValidWorkspace = wasRestored && IsValidPremiereWorkspace(sDVAWorkspace);
				}
				else
				{
					builder->BuildWorkspace();
				}

			}
		}

		return isValidWorkspace;
	}

	/**
	**	Switch to the workspace with the specified name and set the
	**	keyboard/commander focus to the frontmost tab panel of the
	**	root frame if no tab panel currently has focus. In a typical Premiere
	**	workspace this will be the project panel, but that depends completely
	**	on layout of the individual workspace.
	*/
	bool SetCurrentWorkspace(
		const dvacore::UTF16String& inWorkspaceName)
	{
		DVA_ASSERT(! inWorkspaceName.empty());
		DVA_ASSERT(sDVAWSMgr != NULL);

		std::uint32_t lastFocusedTabID = 0;
		bool hadFocusedTab = false;
		dvaworkspace::workspace::TabPanel* lastFocusedTab =
			sDVAWorkspace->GetFocusedTabPanel();

		if (lastFocusedTab != NULL)
		{
			hadFocusedTab = true;
			lastFocusedTabID = lastFocusedTab->GetID();
		}

		const bool wasLoaded = sDVAWSMgr->SetCurrentWorkspace(inWorkspaceName);
		bool isValidWorkspace = (wasLoaded && IsValidPremiereWorkspace(sDVAWorkspace));

		if (isValidWorkspace)
		{
			dvaworkspace::workspace::TabPanel* newFocusPanel = NULL;

			if (hadFocusedTab)
			{
				//	Try to find the tab that had focus before we changed
				//	workspaces and give it focus again (we can't rely on a
				//	TabPanel* to be valid across workspace loads so we use
				//	a tab panel ID).
				newFocusPanel = dvaworkspace::workspace::WorkspaceUtils::FindTabFromTabPanelID(
					*sDVAWorkspace, lastFocusedTabID);
			}

			if (newFocusPanel == NULL)
			{
				dvaworkspace::workspace::TopLevelWindow* topLevelWindow =
					sDVAWorkspace->GetMainTopLevelWindow();
				DVA_ASSERT(topLevelWindow != NULL);

				//	Couldn't find an appropriate tab panel -- find the front most tab of the
				//	root frame.
				if (topLevelWindow != NULL)
				{
					dvaworkspace::workspace::WorkspaceFrame* rootFrame =
						topLevelWindow->GetRootFrame();

					if (rootFrame != NULL)
					{
						newFocusPanel = rootFrame->GetFrontTabPanel();
					}
				}
			}

			if (newFocusPanel == NULL)
			{
				// No top-level window or root frame -- presumably this workspace has no open windows
				UIF::Commander::SwitchTarget(UIF::Application::Instance()->GetRootCommander());
			}
			else
			{
				sDVAWorkspace->SetFocusedTabPanel(newFocusPanel);
			}
		}
		else
		{
			// The current workspace is screwy -- try to restore a backup
			isValidWorkspace = RevertWorkspace(inWorkspaceName);
		}

		return isValidWorkspace;
	}

}// MZ
