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



#ifndef MZ_PLPRESETMANAGERFACTORY_H
#define MZ_PLPRESETMANAGERFACTORY_H

//ASL
#include "ASLClass.h"
#include "ASLStringUtils.h"

//Local
#include "Preset/IPLPresetManagerFactory.h"

namespace PL
{

class PLPresetManagerFactory;

ASL_DEFINE_CLASSREF(PLPresetManagerFactory, IPLPresetManagerFactory);
ASL_DEFINE_IMPLID(PLPresetManagerFactory, 0x634dac40, 0xd9d9, 0x4102, 0xa5, 0xca, 0xab, 0x33, 0x2d, 0x76, 0xb5, 0x3d);

class PLPresetManagerFactory
	:
	public IPLPresetManagerFactory
{
	ASL_SINGLETON_CLASS_SUPPORT(PLPresetManagerFactory);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(IPLPresetManagerFactory);
		ASL_QUERY_ENTRY(PLPresetManagerFactory)
	ASL_QUERY_MAP_END

public:
	PLPresetManagerFactory();

	virtual ~PLPresetManagerFactory();
	
	virtual IPLPresetManagerRef CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory);
	
	virtual IPLPresetManagerRef CreatePresetManager(
		const dvacore::UTF16String& inSystemPresetFolder,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory);

	virtual IPLPresetManagerRef CreatePresetManager(
		const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector, 
		const dvacore::UTF16String& inUserPresetFolder,
		const PLPresetCategory inCategory);

	virtual IPLPresetManagerRef CreatePresetManager(
		const std::vector<dvacore::UTF16String>& inSystemPresetFoldersVector,
		std::auto_ptr<PLPresetHandlerFactory> inPresetHandlerFactory,
		const PLPresetCategory inCategory);
};

} //namespace PL

#endif //MZ_PLPRESETMANAGERFACTORY_H