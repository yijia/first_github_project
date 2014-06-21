/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2004 Adobe Systems Incorporated                       */
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

#ifndef PLMEDIAMETADATAPROVIDER_H
#define PLMEDIAMETADATAPROVIDER_H


#include "ASLMessageMap.h"
#include "ASLStationUtilsManaged.h"
#include "BE/Media/IMediaMetaData.h"
#include "BEExecutorFwd.h"
#include "dvametadata/MetaDataProviders.h"
#include "PLExport.h"
#include "BEMasterClip.h"


namespace PL
{

/**
**	This should be moved into or at least closer to MediaCore
**	This only provides the metadata from the media, if project clip metadata
**		is also desired see MZMetaDataProvider.h
*/

class CottonwoodMediaMetadataProvider :
	public dvametadata::BufferProviderImpl,
	public ASL::Listener
{
public:
	PL_EXPORT
	CottonwoodMediaMetadataProvider(
		BE::IMediaMetaDataRef = BE::IMediaMetaDataRef());

	PL_EXPORT
	virtual ~CottonwoodMediaMetadataProvider();

	PL_EXPORT
	void ResetMediaMetaData(
		BE::IMediaMetaDataRef);

	PL_EXPORT
	virtual bool QueryPathStatus(
		const dvametadata::Path&,
		dvametadata::PathStatus) const;

protected:
	ASL_MESSAGE_MAP_DECLARE();

	void OnMetaDataChanged();
	void DoWriteChanges(
		const dvametadata::MetaDataChangeList& inChangeList);
	void WriteChanges(
		const dvametadata::MetaDataChangeList&);
	void WriteBuffer(
		const dvametadata::MetaDataChangeList&,
		const dvacore::UTF8Char*,
		std::size_t);

	MF::BinaryData mLastMetaDataBuffer;
	BE::IMediaMetaDataRef mMediaMetaData;
};
typedef boost::shared_ptr<CottonwoodMediaMetadataProvider> CottonwoodMediaMetaDataProviderPtr;

} // namespace PL


#endif