/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2005 Adobe Systems Incorporated                       */
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

#ifndef PLEXPORT_H
#define PLEXPORT_H

#pragma once

#ifndef ASLCONFIG_H
#include "ASLConfig.h"
#endif

namespace PL 
{
	/**
	**	The PL_EXPORT macro is used to switch between dllexport 
	**	and dllimport linkage for Mezzanine. Internal to Mezzanine, 
	**	this macro is defined (in the precompiled header) as 
	**	__declspec(dllexport). The macro should not be defined for 
	**	external clients, and it will thus here be mapped to 
	**	__declspec(dllimport).
	**
	**	Note that this is NOT a general purpose import/export macro
	**	that other packages can use. It can only be used for the 
	**	PLCore package.
	*/
#ifndef PL_EXPORT
#if ASL_TARGET_OS_WIN
#define PL_EXPORT		__declspec(dllimport)
#elif ASL_TARGET_OS_MAC
	// __declspec(dllimport) is only required on windows
#define PL_EXPORT __attribute__((visibility("default")))
#else
#error Unsupported Platform
#endif
#endif

	//	Use this for exporting typeinfo for classes.
	//	Example for class Foo:
	//		class ASL_CLASS_EXPORT Foo { .... };
#ifndef PL_CLASS_EXPORT
#if ASL_TARGET_OS_WIN
	//	No op on Windows
#define PL_CLASS_EXPORT
#elif ASL_TARGET_OS_MAC
#define PL_CLASS_EXPORT __attribute__((visibility("default")))
#else
#error Unsupported Platform
#endif
#endif

}//namespace PL
#endif