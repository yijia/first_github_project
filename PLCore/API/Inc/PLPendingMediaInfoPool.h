/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2012 Adobe Systems Incorporated		               */
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

#ifndef PLMEDIAPENDINGPOOL_H
#define PLMEDIAPENDINGPOOL_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef BE_MEDIA_IMEDIAINFO_H
#include "BE/Media/IMediaInfo.h"
#endif

#include <boost/function.hpp>

namespace PL 
{

	typedef boost::function<void()> MediaOnlineNotification;

	/*
	**
	*/
	PL_EXPORT
	bool RegisterPendingMediaInfo(
			BE::IMediaInfoRef const& inMediaInfo, 
			MediaOnlineNotification const& inOnlineNotification = NULL);

};

#endif

