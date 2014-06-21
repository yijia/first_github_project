/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
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

// ASL
#include "ASLClass.h"
#include "ASLStationUtilsManaged.h"

// local
#include "Preset/PLPresetManagerStation.h"
#include "Preset/PresetManagerMessages.h"

// dvacore
#include "dvacore/utility/StaticInstance.h"

namespace PL
{

	const ASL::StationID sPLPresetManagerStation (ASL_STATIONID("MZ.Station.PLPresetManagerStation"));
	DVA_POST_STATIC_INITIALIZED_INSTANCE(ASL::StationUtils::ManagedStation, PresetManagerStation, = sPLPresetManagerStation);

	ASL::StationID GetPresetManagerStationID()
	{
		return PresetManagerStation().GetStationID();
	}
	
} 
