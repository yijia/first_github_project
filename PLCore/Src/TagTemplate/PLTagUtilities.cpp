/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

//	Prefix
#include "Prefix.h"

//	Self
#include "TagTemplate/PLTagUtilities.h"

//	DVA
#include "dvacore/config/Localizer.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/filesupport/file/FileException.h"

// ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLStringCompare.h"
#include "ASLStationUtils.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLFile.h"
#include "ASLMixHashGuid.h"

//	boost
#include <boost/foreach.hpp>

// PL
#include "PLMessage.h"
#include "TagTemplate/PLTagTemplateInfo.h"
#include "TagTemplate/PLTagTemplateCollection.h"
#include "TagTemplate/IPLTagTemplate.h"

// PRM
#include "PRMApplicationTarget.h"

// UIF
#include "UIFWindowTypes.h"
#include "UIFWindowClass.h"
#include "UIFCommander.h"

static const char* const kCreateTagTemplateFailure = "$$$/Prelude/PLTagUtilities/FailedToCreateTemplateFile=Create tag template file failed.";

namespace PL
{

	namespace TagUtilities
	{

		/*
		**
		*/
		void UnregisterTagTemplate(const ASL::StringVector& inPathList)
		{
			PL::TagTemplateList removedList;
			ASL::String	tagTemplateID;
			PL::TagTemplateInfoPtr tagTemplate;

			BOOST_FOREACH(const ASL::String& eachPath, inPathList)
			{
				tagTemplateID = PL::MakeTagTemplateID(eachPath);
				tagTemplate = TagTemplateCollection::GetInstance()->FindTagTemplate(tagTemplateID);

				if (tagTemplate)
				{
					TagTemplateCollection::GetInstance()->RemoveTagTemplate(tagTemplate);
					removedList.push_back(tagTemplate);
				}
			}

			ASL::StationUtils::BroadcastMessage(
				GetTagTemplateStationID(),
				TagTemplateRemovedMessage(removedList));
		}

		/*
		**
		*/
		void RegisterTagTemplate(const ASL::StringVector& inPathList)
		{
			PL::TagTemplateList addedList;
			ASL::StringVector changedList;
			ASL::StringVector unfinishedList;
			bool exist = false;
			ASL::String	tagTemplateID;
			PL::TagTemplateInfoPtr tagTemplate;

			BOOST_FOREACH(const ASL::String& eachPath, inPathList)
			{
				exist = false;
				tagTemplateID = PL::MakeTagTemplateID(eachPath);
				tagTemplate = TagTemplateCollection::GetInstance()->FindTagTemplate(tagTemplateID);

				if (tagTemplate)
				{
					TagTemplateCollection::GetInstance()->RemoveTagTemplate(tagTemplate);
					exist = true;
				}

				tagTemplate = TagTemplateCollection::CreateTagTemplateItem(tagTemplateID, eachPath);

				if (tagTemplate && PL::LoadTagTemplate(eachPath))
				{
					TagTemplateCollection::GetInstance()->AddTagTemplate(tagTemplate);

					if (exist)
					{
						changedList.push_back(eachPath);
					}
					else
					{
						addedList.push_back(tagTemplate);
					}
				}
				else
				{
					unfinishedList.push_back(eachPath);
				}
			}

			if (!addedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					GetTagTemplateStationID(),
					TagTemplateAddedMessage(addedList));
			}

			if (!changedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					GetTagTemplateStationID(),
					TagTemplateChangedMessage(changedList));
				ASL::StationUtils::BroadcastMessage(
					GetTagTemplateStationID(),
					TagTemplateOrderChangedMessage());
			}

			if (!unfinishedList.empty())
			{
				ASL::StationUtils::BroadcastMessage(
					GetTagTemplateStationID(),
					ImportTagTemplateErrorMessage(unfinishedList));
			}
		}

		/*
		**
		*/
		void RegisterCloudData(const PL::Utilities::CloudData& inCloudData)
		{
			ASL::StringVector pathList;
			BOOST_FOREACH(PL::Utilities::CloudData::value_type pair, inCloudData)
			{
				pathList.push_back(pair.first);
			}

			RegisterTagTemplate(pathList);
		}

		/*
		**
		*/
		bool RegisterFromTagTemplateMessage(const PL::TagTemplateMessageInfoList& inTagTemplateMessageInfoList, PL::TagTemplateMessageOptionType inTagTemplateOption, PL::TagTemplateMessageResultList& outTagTemplateMessageResultList)
		{
			// Open "tag template" panel automatically
			UIF::WindowClassName umClassName = UIF::kTagTemplateWindowTypeID;
			UIF::CommandID umCommandID = UIF::GetWindowClassCommandID(umClassName);
			UIF::Commander* target = UIF::Commander::GetTarget();
			if (target != NULL)
			{
				UIF::Command command(umCommandID);
				target->PostCommand(command);
			}

			ASL::StringVector pathList;
			bool confirmToOverwrite = inTagTemplateOption != PL::kTagTemplateOptionType_Overwrite;
			bool overwrite = inTagTemplateOption == PL::kTagTemplateOptionType_Overwrite;
			BOOST_FOREACH (const PL::TagTemplateMessageInfoPtr& tagTemplateMessageInfo, inTagTemplateMessageInfoList)
			{
				PL::TagTemplateMessageResultPtr tagTemplateResult(new PL::TagTemplateMessageResult(tagTemplateMessageInfo->mTagTemplateID, ASL::ResultFlags::kResultTypeFailure));
				IPLTagTemplateReadonlyRef tagTemplate = BuildTagTemplate(tagTemplateMessageInfo->mTagTemplateContent);
				if (tagTemplate != NULL)
				{
					if (tagTemplate->GetName().empty())
					{
						IPLTagTemplateWritableRef(tagTemplate)->SetName(ASL::PathUtils::GetFilePart(tagTemplateMessageInfo->mTagTemplateID));
					}
					ASL::MixHashGuid guid(ASL::MakeStdString(tagTemplateMessageInfo->mTagTemplateID).c_str());
					ASL::String fileName = ASL::PathUtils::GetFilePart(tagTemplateMessageInfo->mTagTemplateID) + guid.ToGuid().AsString() + DVA_STR(".json");
					ASL::String filePath = ASL::PathUtils::CombinePaths(PL::Utilities::GetInitTagTemplateSaveFolder(), fileName);
					if (confirmToOverwrite && ASL::PathUtils::ExistsOnDisk(filePath))
					{
						ASL::String dialogTitle = dvacore::ZString("$$$/Prelude/PLCore/TagUtilities/TagTemplateMessage=Tag Template");
						ASL::String dialogText = 
							dvacore::ZString("$$$/Prelude/PLCore/TagUtilities/DuplicateTemplate=One or more tag templates have been sent before, to overwrite may cause data loss, do you want to continue?");
						if (UIF::MBResult::kMBResultYes == UIF::MessageBox(
							dialogText, 
							dialogTitle, 
							UIF::MBFlag::kMBFlagYesNo, 
							UIF::MBIcon::kMBIconWarning))
						{
							overwrite = true;
						}
						confirmToOverwrite = true;
					}
					if (overwrite || !ASL::PathUtils::ExistsOnDisk(filePath))
					{
						if (PL::SaveTagTemplate(filePath, tagTemplate))
						{
							pathList.push_back(filePath);
							tagTemplateResult->mResult = ASL::kSuccess;
						}
					}
				}
				if (ASL::ResultFailed(tagTemplateResult->mResult))
				{
					ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/TagUtilities/FailedTemplate=Failed to add @0 to tag template panel", tagTemplateMessageInfo->mTagTemplateID));
				}
				outTagTemplateMessageResultList.push_back(tagTemplateResult);
			}
			RegisterTagTemplate(pathList);
			return true;
		}

		/*
		**
		*/
		bool SaveTagTemplateAs(PL::IPLTagTemplateReadonlyRef inTagTemplate, const ASL::String& inNewPath, ASL::String& errorMessage)
		{
			try
			{
				bool saveSuccess = PL::SaveTagTemplate(inNewPath, inTagTemplate);

				ASL::String newID = MakeTagTemplateID(inNewPath);
				PL::TagTemplateInfoPtr tagTemplate = TagTemplateCollection::CreateTagTemplateItem(newID, inNewPath);

				if (saveSuccess && tagTemplate)
				{
					// try to remove the existing one before we add
					bool exist = false;

					PL::TagTemplateInfoPtr oldTagTemplate = 
						TagTemplateCollection::GetInstance()->FindTagTemplate(MakeTagTemplateID(newID));

					if (oldTagTemplate)
					{
						TagTemplateCollection::GetInstance()->RemoveTagTemplate(oldTagTemplate);
						exist = true;
					}

					// add the new one
					TagTemplateCollection::GetInstance()->AddTagTemplate(tagTemplate);

					if (exist)
					{
						ASL::StationUtils::BroadcastMessage(
							GetTagTemplateStationID(),
							TagTemplateOverwrittenMessage(tagTemplate));
					}
					else
					{
						ASL::StationUtils::BroadcastMessage(
							GetTagTemplateStationID(),
							TagTemplateCreatedMessage(tagTemplate));
					}

					return true;
				}

				errorMessage = dvacore::ZString(kCreateTagTemplateFailure);
			}
			catch (dvacore::filesupport::file_exception& e)
			{
				DVA_ASSERT_MSG(false, e.what());

				errorMessage = dvacore::ZString(kCreateTagTemplateFailure);
			}
			return false;
		}


		/*
		**
		*/
		bool NewTagTemplate(const ASL::String& inNewPath)
		{
			ASL::String newID = MakeTagTemplateID(inNewPath);
			bool exist = false;

			PL::TagTemplateInfoPtr tagTemplate = 
				TagTemplateCollection::GetInstance()->FindTagTemplate(MakeTagTemplateID(newID));

			if (tagTemplate)
			{
				TagTemplateCollection::GetInstance()->RemoveTagTemplate(tagTemplate);
				exist = true;
			}

			if (PL::SaveTagTemplate(inNewPath, PL::CreateDefaultTagTemplate(ASL::PathUtils::GetFilePart(inNewPath))))
			{
				tagTemplate = TagTemplateCollection::CreateTagTemplateItem(newID, inNewPath);

				if (tagTemplate)
				{
					TagTemplateCollection::GetInstance()->AddTagTemplate(tagTemplate);

					if (exist)
					{
						ASL::StationUtils::BroadcastMessage(
							GetTagTemplateStationID(),
							TagTemplateOverwrittenMessage(tagTemplate));
					}
					else
					{
						ASL::StationUtils::BroadcastMessage(
							GetTagTemplateStationID(),
							TagTemplateCreatedMessage(tagTemplate));
					}

					ASL::StationUtils::BroadcastMessage(
						GetTagTemplateStationID(),
						EditTagTemplateMessage(inNewPath, PL::IPLTagTemplateReadonlyRef()));

					return true;
				}
			}

			ASL::StationUtils::BroadcastMessage(
				GetTagTemplateStationID(),
				NewTagTemplateErrorMessage(inNewPath));
			
			return false;
		}

		/*
		**
		*/
		ASL::String GetTagTemplateFileExtension()
		{
			ASL::String extensionString = ASL_STR("json");

			return extensionString;
		}

		/*
		**
		*/
		ASL::String GetTagTemplateFileTypeString()
		{
			ASL::String typeString;
			ASL::String	jsonString = dvacore::ZString("$$$/Prelude/PLCore/TagTemplateFileTypeJSON=Adobe Tag Template (*.json)");
			jsonString.push_back('\0');
			jsonString.append(ASL_STR("*.json"));
			jsonString.push_back('\0');
			typeString.append(jsonString);

			/*
			// We don't support xml-based tag template currently.
			ASL::String	xmlString = dvacore::ZString("$$$/Prelude/PLCore/TagTemplateFileTypeXML=Adobe Tag Template (*.xml)");
			xmlString.push_back('\0');
			xmlString.append(ASL_STR("*.xml"));
			xmlString.push_back('\0');
			typeString.append(xmlString);
			*/

			return typeString;
		}

		/**
		** 
		*/
		dvacore::UTF16String GetSystemTagTemplateFolderPath(bool inCreateNonExist)
		{
			dvacore::filesupport::Dir sharedDocsDir(dvacore::filesupport::Dir::SharedDocsDir());
			DVA_ASSERT(sharedDocsDir.Exists());

			if ( !sharedDocsDir.Exists() )
			{
				return DVA_STR("");
			}

			// Get the user tag template folder
			dvacore::filesupport::Dir userAdobeDir(sharedDocsDir, DVA_STR("Adobe"));
			if (inCreateNonExist && !userAdobeDir.Exists()) 
			{
				userAdobeDir.Create();
			}

			dvacore::filesupport::Dir userPreludeDir(userAdobeDir, DVA_STR("Prelude"));
			if (inCreateNonExist && !userPreludeDir.Exists()) 
			{
				userPreludeDir.Create();
			}

			dvacore::filesupport::Dir userPreludeVersionDir(userPreludeDir, ASL::MakeString(PRM::GetApplicationVersion()));
			if (inCreateNonExist && !userPreludeVersionDir.Exists()) 
			{
				userPreludeVersionDir.Create();
			}

			dvacore::filesupport::Dir userTagTemplatesDir(userPreludeVersionDir, DVA_STR("TagTemplates"));
			if (inCreateNonExist && !userTagTemplatesDir.Exists()) 
			{
				userTagTemplatesDir.Create();
			}

			return userTagTemplatesDir.FullPath();
		}

		/**
		** 
		*/
		ASL::StationID GetTagTemplateStationID()
		{
			return TagTemplateCollection::GetInstance()->GetStationID();
		}

	} // namespace TagUtilities

} // namespace PL