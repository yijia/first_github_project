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

#ifndef PLENCODERPRESETMANAGER_H
#define PLENCODERPRESETMANAGER_H

#pragma once

// BE
#ifndef BESEQUENCE_H
#include "BESequence.h"
#endif

//	AME
#ifndef DO_NOT_USE_AME

#ifndef AME_IENCODERFACTORY_H
#include "IEncoderFactory.h"
#endif

#ifndef AME_IENCODER_H
#include "IEncoder.h"
#endif

#ifndef AME_IPRESET_H
#include "IPreset.h"
#endif

#ifndef AME_IPRESETMANAGER_H
#include "IPresetManager.h"
#endif

#endif // DO_NOT_USE_AME

// MZ
#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

namespace PL
{

class PreludePresetManager
{
public:

	/*
	**
	*/
	PL_EXPORT
	static PreludePresetManager& GetInstance();

	/*
	**
	*/
	PL_EXPORT
	static void ReleaseInstance();

	/*
	**
	*/
	PL_EXPORT
	const EncoderHost::IEncoderFactory::EncoderList& GetEncoderList() const;

	/*
	**
	*/
	PL_EXPORT
	void LoadPresets(
		EncoderHost::IEncoderFactory::IExporterModuleRef inExporterModule,
		EncoderHost::Presets& outSystemPresets,
		EncoderHost::Presets& outUserPresets);


protected:
	PreludePresetManager();
	~PreludePresetManager();

	/*
	**
	*/
	EncoderHost::IPresetManagerRef CreatePresetManager(
		EncoderHost::IEncoderFactory::IExporterModuleRef inExporterModule) const;

private:

	EncoderHost::IEncoderFactory::EncoderList			mEncoderList;

	std::auto_ptr<EncoderHost::AbstractHostConfig>		mHostConfig;
};

} // namespace PL

#endif // PLENCODERPRESETMANAGER_H
