/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2013 Adobe Systems Incorporated                       */
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

#include "PLInitialize.h"

#include "PLEncoderPresetManager.h"

#include "Preset/PLPreset.h"
#include "Preset/PLPresetCache.h"

#include "IngestMedia/PLIngestJob.h"
#include "PLMarkerTemplate.h"
#include "PLModulePicker.h"
#include "PLWriteXMPToDiskCache.h"
#include "IPLMarkerSelection.h"

#include "PLMediaMonitorCache.h"
#include "PLAsyncCallAssembler.h"
#include "PLProject.h"

#include "TagTemplate/PLTagTemplateCollection.h"

#include "PLSequenceContextInfo.h"
#include "PLThreadUtils.h"

#include "PLEffectSupport.h"

namespace PL
{
	void Initialize()
	{
		PLPreset::Initialize();
		PLPresetCache::Initialize();

		// Init the MZ IngestMedia singleton.
		InitializeIngestMedia();
		SRAsyncCallAssembler::Initialize();

		WriteXMPToDiskCache::Initialize();

		MarkerType::Initialize();
		//Register pre-defined handlers
		MarkerType::RegisterMarkerTypes();

		SRMediaMonitor::Initialize();
		SRProject::Initialize();
		ModulePicker::Initialize();
		MarkerTemplate::Initialize();

		SequenceContextInfoSupport::Initialize();

		TagTemplateCollection::Initialize();

		PL::EffectSupport::Initialize();
	}

	void Terminate()
	{
		PLPreset::Terminate();
		PLPresetCache::Terminate();

		// watch-dog to ensure we will not leak this
		PreludePresetManager::ReleaseInstance();

		SRAsyncCallAssembler::Terminate();
		ShutdownIngestMedia();

		SequenceContextInfoSupport::Terminate();
		MarkerTemplate::Shutdown();
		ModulePicker::Terminate();
		SRProject::Terminate();
		SRMediaMonitor::Terminate();
		MarkerType::Shutdown();
		WriteXMPToDiskCache::Terminate();

		threads::TerminateAllExecutors();

		TagTemplateCollection::Terminate();

		PL::EffectSupport::Terminate();
	}
}
