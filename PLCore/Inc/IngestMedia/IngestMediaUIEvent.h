/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2010 Adobe Systems Incorporated
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

#pragma once

#ifndef INGESTMEDIAUIEVENT_H
#define INGESTMEDIAUIEVENT_H

// MZ
#ifndef PLINGESTJOB_H
#include "IngestMedia/PLIngestJob.h"
#endif

namespace PL
{

class IngestMediaUIEvent
{

public:
	virtual ~IngestMediaUIEvent(){}

	virtual CopyExistOption WarnUserForExistPath(
								ASL::String const& inSrcPath, 
								ASL::String const& inDestPath, 
								bool inIsDirectory, 
								bool& outApplyToAll) = 0;
};

} // namespace PL

#endif // INGESTMEDIAUIEVENT_H
