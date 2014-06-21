/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2008 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

// Prefix
#include "Prefix.h"

// Self
#include "IngestMedia/PLIngestJob.h"

// Local
#include "IngestMedia/PLIngestUtils.h"

// BE
#include "BEBackend.h"
#include "BEProject.h"
#include "BEProperties.h"

// ASL
#include "ASLPathUtils.h"
#include "ASLResults.h"
#include "ASLString.h"
#include "ASLAtomic.h"
#include "ASLClass.h"
#include "ASLClassFactory.h"
#include "ASLListener.h"
#include "ASLMessageMap.h"
#include "ASLPathUtils.h"
#include "ASLResourceUtils.h"
#include "ASLResults.h"
#include "ASLStringUtils.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLThreadedWorkQueue.h"
#include "ASLReferenceBroker.h"
#include "ASLSleep.h"
#include "ASLFile.h"
#include "ASLDirectory.h"

// DVA
#include "dvacore/config/Localizer.h"

// C++
#include <sstream>

namespace PL
{
/*
**
*/
void InitializeIngestMedia()
{
	ASL::StationRegistry::RegisterStation(PL::kStation_IngestMedia);
}

/*
**
*/
void ShutdownIngestMedia()
{
	// Force to shutdown TaskScheduler
	IngestTask::TaskScheduler::Shutdown(true);
	ASL::StationRegistry::DisposeStation(PL::kStation_IngestMedia);	
	IngestTask::TaskScheduler::ReleaseInstance();
}


ASL::String FormatConcatenateError(const IngestTask::PathToErrorInfoVec& inErrorInfos)
{
	ASL::String msg;
	BOOST_FOREACH(const IngestTask::PathToErrorInfoPair& eachErrorInfo, inErrorInfos)
	{
		// report task failure
		msg += dvacore::ZString(
			"$$$/Prelude/Mezzanine/IngestScheduler/ConcatenateTaskFailedDetail= concatenate \"@0\" failed. Reason: @1");
		msg = dvacore::utility::ReplaceInString(
			msg, 
			eachErrorInfo.first,
			eachErrorInfo.second);
	}

	return msg;
}

} // namespace PL
