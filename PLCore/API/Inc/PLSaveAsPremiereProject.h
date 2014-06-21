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

#ifndef PLSAVEASPREMIEREPROJECT_H
#define PLSAVEASPREMIEREPROJECT_H

#ifndef ASLINTERFACE_H
#include "ASLInterface.h"
#endif

#ifndef BEPROJECTFWD_H
#include "BEProjectFwd.h"
#endif

#ifndef BEPROGRESS_H
#include "BEProgress.h"
#endif

#ifndef BEPROJECTITEM_H
#include "BEProjectItem.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef MZSEQUENCE_H
#include "MZSequence.h"
#endif


namespace PL
{
typedef std::set<ASL::String> MediaPathSet;

/**
**	InterfaceRef & ID for ISaveAsPremiereProjectOperation.
*/
ASL_DEFINE_IREF(ISaveAsPremiereProjectOperation);
ASL_DEFINE_IID(ISaveAsPremiereProjectOperation, 0x3b27907b, 0x78d3, 0x4c14, 0x89, 0x9d, 0xab, 0x97, 0xa5, 0x18, 0x4, 0x74);

/**
**	ISaveAsPremiereProjectOperation.
**	@see ASL::IBatchOperation.
*/
ASL_INTERFACE ISaveAsPremiereProjectOperation : public ASLUnknown
{
	/**
	**	Get the result of the save project operation (once completed.)
	**
	**	@return					the save project result.
	*/
	virtual ASL::Result GetResult() const = 0;

	/**
	**	Save the project synchronously, without ProgressBarDialog
	*/
	virtual void StartSynchronous() = 0;

	virtual MediaPathSet const& GetExportMediaFiles() const = 0;
};

/**
**	Create a new Save Project operation.
**	The returned object also supports ASL::IBatchOperation.
**
**	@param	inFilePath				the full path to the destination project file.
**  @param  inTitleString			the string will be showed on HSL::ProgressBarDialog
**  @param  inExportItems			AssetItem from client which used to export
*/
PL_EXPORT
ISaveAsPremiereProjectOperationRef CreateSaveAsPremiereProjectOperation(
    const ASL::String& inFilePath,
    const ASL::String& inTitleString,
	const PL::AssetItemList& inExportItems,
	bool inIsExportSequenceMarkers);

/**
**	Create a new Save Project operation.
**	The returned object also supports ASL::IBatchOperation.
**
**  @param  inAssetItems			AssetItem which used to build PPro project.
**  @param  inTreatMarkerAsSequenceMarker			If we want convert PPro project to FCP project, we can set this value to true.
**  @param  inProgress				Progress interface which used to show progress.
**  @param  outProject				Project which we want to create.
**  @param  outMediaPaths			Output all media path which used by PPro project
*/
PL_EXPORT
ASL::Result CreatePremiereProject(
	PL::AssetItemList const& inAssetItems,
	bool inTreatMarkerAsSequenceMarker,
	BE::IProgressRef  const& inProgress,
	BE::IProjectRef& outProject,
	MediaPathSet& outMediaPaths,
	BE::ProjectItemVector& outAddedProjectItemVector);

/**
**	Create a new BE sequence from input assetitem list.
**	Return the sequence, NULL if fails.
**
**  @param  inSubAssetItems			Sub assetitems from a RC.
**  @param  inProject				Which BE project the sequence should be in.
**  @param  inBin					Which Bin project item the sequence should be in.
**  @param  outErrorInfo			Error info during create sequence, empty if return a valid sequence.
**  @param  outFailedMediaName		We fail on which file.
*/
PL_EXPORT
BE::ISequenceRef CreateBESequenceFromRCSubClips(
	PL::AssetItemList const& inSubAssetItems,
	BE::IProjectRef const& inProject,
	ASL::String const& inSequenceName,
	ASL::String& outErrorInfo,
	ASL::String& outFailedMediaName,
	bool inScaleToFrame,
	MZ::SequenceAudioTrackRule inUseType);

} // namespace PL

#endif
