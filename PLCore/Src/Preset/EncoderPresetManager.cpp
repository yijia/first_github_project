/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004-11 Adobe Systems Incorporated                    */
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

// ASL
#include "ASLClassFactory.h"
#include "ASLPathUtils.h"
#include "ASLFile.h"

// Self
#include "PLEncoderPresetManager.h"

//	AME
#ifndef DO_NOT_USE_AME
#include "EncoderHostInit.h"
//#include "IEncoderFactory.h"
//#include "IEncoder.h"
//#include "SettingsUIInit.h"
//#include "IAMESettingsDlg.h"
//#include "SettingsUICallbacks.h"
#include "MZEncoderManager.h"

// EncoderHost
#include "config/FileBasedPresetHandler.h"
//#include "IPreset.h"
//#include "IPresetManager.h"
#include "IPostEncoderFactory.h"
#include "MCHelperFunctions.h"
#include "config/AbstractHostConfig.h"
#include "MLExporterModuleFwd.h"

#endif // DO_NOT_USE_AME


// dvacore
#include "dvacore/config/Localizer.h"
#include "dvacore/filesupport/file/File.h"
#include "dvacore/filesupport/file/CommonDirs.h"
#include "dvacore/utility/FileUtils.h"
#include "dvacore/threads/SharedThreads.h"
#if ASL_TARGET_OS_WIN
#include "dvacore/utility/FileUtils_WIN.h"
#include "dvacore/utility/OS_Utils_WIN.h"
#elif ASL_TARGET_OS_MAC
#include "dvacore/filesupport/file/Mac/FileHelper.h"
#endif
#include "dvacore/utility/FileUtils.h"

// boost
#include "boost/foreach.hpp"

namespace PL
{

//[Prelude ToDo] Need to put this to global Initialize and Terminate function
static PreludePresetManager* managerPtr = NULL;

/*
**
*/
PreludePresetManager& PreludePresetManager::GetInstance()
{
	if (managerPtr == NULL)
	{
		managerPtr = new PreludePresetManager();
	}
	return *managerPtr;
}

/*
**
*/
void PreludePresetManager::ReleaseInstance()
{
	if (managerPtr != NULL)
	{
		delete managerPtr;
		managerPtr = NULL;
	}
}

/*
**
*/
PreludePresetManager::PreludePresetManager()
{
	EncoderHost::IEncoderFactoryRef	encoderFactory(ASL::CreateClassInstanceRef(kEncoderFactoryClassID));
	if ( encoderFactory != NULL )
	{
		EncoderHost::IEncoderFactory::EncoderList tempEncoderList = encoderFactory->GetEncoderList();
		BOOST_FOREACH(const ML::IExporterModuleRef& exporter, tempEncoderList)
		{
			if (!exporter->HideInUI() && exporter->GetName() != DVA_STR("AS-11"))//disable AS-11 in Prelude.
			{
				mEncoderList.push_back(exporter);
			}
		}
	}

	std::auto_ptr<EncoderHost::AbstractHostConfigFactory> configFactory (new MZ::HostConfigFactory());
	mHostConfig = configFactory->GetHostConfig();
}

/*
**
*/
PreludePresetManager::~PreludePresetManager()
{

}

/*
**
*/
const EncoderHost::IEncoderFactory::EncoderList& PreludePresetManager::GetEncoderList() const
{
	return mEncoderList;
}

/*
**
*/
void PreludePresetManager::LoadPresets(
	EncoderHost::IEncoderFactory::IExporterModuleRef inExporterModule,
	EncoderHost::Presets& outSystemPresets,
	EncoderHost::Presets& outUserPresets) 
{
	EncoderHost::IPresetManagerRef presetManager = CreatePresetManager(inExporterModule);
	DVA_ASSERT(presetManager != NULL);
	if (presetManager != NULL)
	{
		outSystemPresets = presetManager->GetSystemPresets();
		outUserPresets = presetManager->GetUserPresets();
	}
}

/*
**
*/
EncoderHost::IPresetManagerRef PreludePresetManager::CreatePresetManager(
	EncoderHost::IEncoderFactory::IExporterModuleRef inExporterModule) const
{
	EncoderHost::IPresetManagerRef presetManager;
	DVA_ASSERT(inExporterModule != NULL && mHostConfig.get() != NULL );
	if ( inExporterModule != NULL && mHostConfig.get() != NULL )
	{
		presetManager = EncoderHost::InstantiatePresetManager(inExporterModule, mHostConfig.get());
	}

	return presetManager;
}

} // namespace PL
