/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2010 Adobe Systems Incorporated                       */
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

#ifndef IPLLOGGINGCLIP_H
#define IPLLOGGINGCLIP_H


#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef PLCLIPITEM_H
#include "PLClipItem.h"
#endif

namespace PL
{

ASL_DEFINE_IID(ISRLoggingClip, 0x135b70e, 0x8229, 0x437c, 0xaf, 0x71, 0x64, 0x76, 0xbe, 0x1d, 0xba, 0x91);
ASL_DEFINE_IREF(ISRLoggingClip);

ASL_INTERFACE ISRLoggingClip : public ASLUnknown
{
public:

	/*
	**
	*/
	virtual dvamediatypes::TickTime GetStartTime() const = 0;

	/*
	**
	*/
	virtual dvamediatypes::TickTime GetDuration() const = 0;

	/*
	**
	*/
	virtual SRClipItemPtr GetSRClipItem() const = 0;
	
	/*
	**
	*/
	virtual void UpdateBoundary(const dvamediatypes::TickTime& inStartTime, const dvamediatypes::TickTime& inDuration) = 0;

	/*
	**
	*/
	virtual bool IsLoggingClipForEA() const =0;
};

} // namespace PL

#endif
