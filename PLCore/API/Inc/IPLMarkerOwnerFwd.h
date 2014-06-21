/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2014 Adobe Systems Incorporated                       */
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

#ifndef IPLMARKEROWNERFWD_H
#define IPLMARKEROWNERFWD_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

namespace PL
{
    
    /**
     **	InterfaceRef & ID for IMarkerOwner.
     */
    ASL_DEFINE_IREF(ISRMarkerOwner);
    ASL_DEFINE_IID(ISRMarkerOwner, 0xbecd2f01, 0x557a, 0x460d, 0xa5, 0x44, 0x90, 0x75, 0x74, 0x30, 0xfb, 0x21)
} // namespace PL

#endif