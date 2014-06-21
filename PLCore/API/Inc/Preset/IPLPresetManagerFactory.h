/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2003-2006 Adobe Systems Incorporated                  */
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


#ifndef MZ_IPLPRESETMANAGERFACTORY_H
#define MZ_IPLPRESETMANAGERFACTORY_H

//ASL
#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

//Local
#include "Preset/IPLPresetManager.h"
#include "Preset/AbstractPLPresetHandler.h"

namespace PL
{

ASL_DEFINE_IID(IPLPresetManagerFactory, 0x53718821, 0xfb38, 0x41ba, 0xbd, 0xd2, 0x3d, 0x30, 0x5c, 0x91, 0x64, 0xfb);
ASL_DEFINE_IREF(IPLPresetManagerFactory);

ASL_INTERFACE IPLPresetManagerFactory: public ASLUnknown
{
public:
	virtual IPLPresetManagerRef CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory) = 0;
	
	virtual IPLPresetManagerRef CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory) = 0;

	virtual IPLPresetManagerRef CreatePresetManager(
		const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory)= 0;

	virtual IPLPresetManagerRef CreatePresetManager(
		const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory)= 0;

};

}

#endif //MZ_IPLPRESETMANAGERFACTORY_H