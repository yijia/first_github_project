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

#ifndef PLFACTORY_H
#define PLFACTORY_H

#ifndef BE_CLIP_IMASTERCLIPFWD_H
#include "BE/Clip/IMasterClipFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEMFWD_H
#include "BE/Project/IProjectItemFwd.h"
#endif

#ifndef BE_PROJECT_IPROJECTFWD_H
#include "BE/Project/IProjectFwd.h"
#endif

#ifndef BE_SEQUENCE_ITRACKITEMSELECTIONFWD_H
#include "BE/Sequence/ITrackItemSelectionFwd.h"
#endif

#ifndef PLCLIPDEFINITION_H
#include "PLClipDefinition.h"
#endif

#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef PLUNASSOCIATEDMETADATA_H
#include "PLUnassociatedMetadata.h"
#endif

namespace PL
{
	namespace SRFactory
	{
		/*
		** 
		*/
		BE::IProjectItemRef CreateEmptySequence(
				BE::IProjectRef const& inProject,
				BE::IProjectItemRef const& inContainingBin = BE::IProjectItemRef(),
				ASL::String const& inSequenceName = ASL_STR("Dummy Sequence"));

		/*
		**
		*/
		ISRPrimaryClipRef FindAndCreatePrimaryClip(PL::AssetItemPtr const& inAssetItem);

		/*
		**
		*/
		ISRPrimaryClipRef CreatePrimaryClip(PL::AssetItemPtr const& inAssetItem);

		/*
		** ID is used as key to register, search, read metadata.
		** Path is only used for UI display and cannot be trusted to read metadata
		** For local xmp, path equals to ID, for CC, ID is cached file path in local, path is url on cloud.
		*/
		SRUnassociatedMetadataRef CreateFileBasedSRUnassociatedMetadata(
			const ASL::String& inMetadataID,
			const ASL::String& inMetadataPath);

		/*
		**
		*/
		SRUnassociatedMetadataRef CreateSRUnassociatedMetadata(
			const ASL::String& inMetadataID,
			const ASL::String& inMetadataPath, 
			const XMPText& inXMPText);

	}
}

#endif