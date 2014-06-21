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

#pragma once

#ifndef PLMESSAGE_H
#define PLMESSAGE_H

#ifndef BEMASTERCLIPFWD_H
#include "BEMasterClipFwd.h"
#endif

#ifndef BECLIPFWD_H
#include "BEClipFwd.h"
#endif

#ifndef BETRACKFWD_H
#include "BETrackFwd.h"
#endif

#ifndef MZTRACKITEMSELECTION_H
#include "MZTrackItemSelection.h"
#endif


#ifndef BE_SEQUENCE_ISEQUENCEFWD_H
#include "BE/Sequence/ISequenceFwd.h"
#endif

// MZ
#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

// ASL
#ifndef ASLCRITICALSECTION_H
#include "ASLCriticalSection.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLDIRECTORY_H
#include "ASLDirectory.h"
#endif

#ifndef ASLSTRINGVECTOR_H
#include "ASLStringVector.h"
#endif

#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

//local
#ifndef IPLROUGHCUT_H
#include "IPLRoughCut.h"
#endif

#ifndef BE_MEDIA_IMEDIAMETADATA_H
#include "BE/Media/IMediaMetaData.h"
#endif

#ifndef PLMESSAGESTRUCT_H
#include "PLMessageStruct.h"
#endif

#ifndef BE_PROJECT_IPROJECTITEMSELECTION_H
#include "BE/Project/IProjectItemSelection.h"
#endif

#ifndef BEPROJECT_H
#include "BEProject.h"
#endif

#ifndef UIFMULTICONTEXTHANDLER_H
#include "UIFMultiContextHandler.h"
#endif

#ifndef PLUNASSOCIATEDMETADATA_H
#include "PLUnassociatedMetadata.h"
#endif

#ifndef PLTAGTEMPLATEINFO_H
#include "TagTemplate/PLTagTemplateInfo.h"
#endif

#ifndef IPLTAGTEMPLATE_H
#include "TagTemplate/IPLTagTemplate.h"
#endif

namespace PL
{

ASL_DECLARE_MESSAGE_WITH_0_PARAM(SubclipInOutChanged);

/**
**	Sent when there is a change to the markers
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(SilkRoadMarkerChanged, PL::CottonwoodMarkerList);
/**
**	bool: If we want to add marker silently without special UI change, 
**	Handlers can do some work based on this value, such as displaying HUD
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(SilkRoadMarkerAdded, PL::CottonwoodMarkerList, bool);
ASL_DECLARE_MESSAGE_WITH_1_PARAM(SilkRoadMarkerDeleted, PL::CottonwoodMarkerList);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(PLStaticMetadataChanged);
ASL_DECLARE_MESSAGE_WITH_1_PARAM(SilkRoadSubClipAppended, BE::ISequenceRef);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(PreludeEditModeChanged);
ASL_DECLARE_MESSAGE_WITH_3_PARAM(WriteXMPToDiskFinished, ASL::Result, dvacore::UTF16String, BE::RequestIDs);

ASL_DECLARE_MESSAGE_WITH_0_PARAM(SilkRoadMediaDirtyChanged);
ASL_DECLARE_MESSAGE_WITH_0_PARAM(SilkRoadRoughCutDirtyChanged);
ASL_DECLARE_MESSAGE_WITH_1_PARAM(SilkRoadRoughCutStructureChanged, BE::ISequenceRef);
ASL_DECLARE_MESSAGE_WITH_0_PARAM(SilkRoadLoggingClipRelinked);
ASL_DECLARE_MESSAGE_WITH_0_PARAM(SilkRoadRoughCutRelinked);


ASL_DECLARE_MESSAGE_WITH_2_PARAM(SelectRoughCutClips, BE::ISequenceRef, BE::ITrackItemSelectionRef);

/**
**	Sent when there is a selection change for markers
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(MarkerSelectionChanged);

/**
**	
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(CleanProjectCacheFilesMessage);

/**
**	
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(MoveProjectCacheFilesToNewDestinationMessage, ASL::String);

/**
**	Send when a marker is open from project panel
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(
	ZoomToActiveRange, 
	dvamediatypes::TickTime,			// Start Time
	dvamediatypes::TickTime);			// Duration

/**
**	Sent if we want to open Ingest Dialog
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(OpenIngestDialogMessage, ASL::String);

/**
**	Sent if we want to use send to Ppro with right click menu
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(SendToPremiereProMessage);


/**
**	Sent if we want to exit Application
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(ExitApplicationMessage, ASL::Result);

/**
**	Sent Unload Library Message
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(UnloadLibraryMessage, bool, ASL::Result);

/**
**	Sent if media has been changed from Other APP like Premiere.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(MediaMetaDataOutOfSync, ASL::PathnameList);

/**
**	Sent when file has been registerred into SRMediaMonitor
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(RegisterMediaMonitor, ASL::String);

/**
**	Sent when file has been un-registerred from SRMediaMonitor
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(UnregisterMediaMonitor, ASL::String);


/**
**	Sent if media's XMP's marker part Not in sync.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(MediaMetaDataMarkerNotInSync, ASL::String);


/**
**	Sent if media's XMP's marker part changed.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(MediaMetaDataMarkerChanged, ASL::String);

/**
**	Sent if media's XMP been changed.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(MediaMetaDataChanged, ASL::String);

/** 
**	Sent to begin process of importing unassociated metadata file from disk
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(ImportUnassociatedMetadataFileMessage);

/** 
**	Sent after the process of importing unassociated metadata file from disk is complete
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(UnassociatedMetadataChangedMessage, PL::SRUnassociatedMetadataList);

/**
**	Sent when unassociated metadatas are registered to SRProject
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(UnassociatedMetadataAddedMessage, PL::SRUnassociatedMetadataList);

/**
** Sent when unassociated metadatas are unregistered from SRProject
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(UnassociatedMetadataRemovedMessage, PL::SRUnassociatedMetadataList);


/**
** When an asset item In/Out is changed, send this message
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(AssetItemInOutChanged, ASL::Guid, BE::IProjectItemRef );

/** 
**	When primaryclip  alias name is changed, broadcast it.
**	param 1: ASL::String primary clip media path 
**	param 2: ASL::String new name
**	param 3: ASL::String the old name
*/
ASL_DECLARE_MESSAGE_WITH_3_PARAM(PrimaryClipRenamedMessage, ASL::String, ASL::String, ASL::String);

/**
**	Sent when apply unassociated metadata to timeline
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(ApplyUnassociatedMetadataMessage, UnassociatedMetadataList, ApplyUnassociatedMetadataPositionType);


/**
**  Sent when appending assetitems
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(AssetsItemAppendingStateChanged);

/**
**  Sent when deleting rcsubclips from roughcut
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(RCSubClipDeletingMessage);

/**
**  Sent when the whole SequenceContextInfoMap is rebuilt.
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(SequenceContextInfoMapChanged);


/**
**  Sent only for EA case
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(AssetItemNameChanged, ASL::String, ASL::String);

/**
**  Sent when one SequenceContextInfo changed
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(SequenceContextInfoChanged, UIF::MultiContextHandler::ContextID);

/**
**  Sent when current primary clip (logging clip or RC) has been changed between read-only and writable status.
**	The bool parameter means new savable state, true means savable.
**	This message should be post to SRProject::GetStationID
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(CurrentModuleDataSavableStateChanged, bool);

/** 
**	Sent to begin process of importing tag template file from disk
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(ImportTagTemplateFileMessage);

/** 
**	Sent to begin process of new a tag template
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(NewTagTemplateMessage);

/** 
**	Sent to begin process of edit a tag template
**	param 1: tag template path. should NOT be empty.
**	param 2: tag template object. If empty, handler should load from path. Or, should reuse this object rather than reload again,
**			 especially for edge cases that physical file maybe absent.
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(EditTagTemplateMessage, dvacore::UTF16String, IPLTagTemplateReadonlyRef);

/** 
**	Sent when new tag templates are imported to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(TagTemplateAddedMessage, PL::TagTemplateList);

/** 
**	Sent when existing tag templates are changed to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(TagTemplateChangedMessage, ASL::StringVector);

/** 
**	Sent when existing tag templates are removed to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(TagTemplateRemovedMessage, PL::TagTemplateList);

/** 
**	Sent when tag templates order has been changed. For example, maybe UI need to refresh drop down list.
*/
ASL_DECLARE_MESSAGE_WITH_0_PARAM(TagTemplateOrderChangedMessage);

/** 
**	Sent when a new tag template are created to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(TagTemplateCreatedMessage, PL::TagTemplateInfoPtr);

/** 
**	Sent when a new tag template overwrites an old one to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(TagTemplateOverwrittenMessage, PL::TagTemplateInfoPtr);

/** 
**	Sent when importing tag templates unsuccessfully to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(ImportTagTemplateErrorMessage, ASL::StringVector);

/** 
**	Sent when fail to create a new tag template to update UI
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(NewTagTemplateErrorMessage, ASL::String);

/** 
**	When extended panel send API message kCSXSEventType_InsertDynamicColumns,
**	this message will be post.
**	First GuidStringMap is column id and name map, second GuidStringMap is column id and previous column name map for new column position.
*/
ASL_DECLARE_MESSAGE_WITH_2_PARAM(InsertDynamicColumnsMessage, PL::GuidStringMap, PL::GuidStringMap);

/** 
**	When extended panel send API message kCSXSEventType_DeleteDynamicColumns,
**	this message will be post.
**	The PL::GuidSet includes all column IDs which will be deleted.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(DeleteDynamicColumnsMessage, PL::GuidSet);

/** 
**	When extended panel send API message kCSXSEventType_UpdateDynamicColumnFields,
**	this message will be post.
*/
ASL_DECLARE_MESSAGE_WITH_1_PARAM(UpdateDynamicColumnFieldsMessage, PL::FieldValuesMap);
}

#endif
