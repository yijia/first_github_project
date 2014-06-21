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
#include "PLMessage.h"
#include "PLMediaMetaDataProvider.h"


#include "MZMetaDataProvider.h"
#include "MZMetadataActions.h"
#include "MZExternalData.h"

#include "ASLThreadManager.h"
#include "BE/PreferencesConstants.h"
#include "BEBackend.h"
#include "BEProperties.h"

#include "dvametadata/Initialize.h"

#include "IncludeXMP.h"


namespace PL
{

ASL_MESSAGE_MAP_DEFINE(CottonwoodMediaMetadataProvider)
	ASL_MESSAGE_HANDLER(PL::MediaMetaDataChanged, OnMetaDataChanged)
ASL_MESSAGE_MAP_END

CottonwoodMediaMetadataProvider::CottonwoodMediaMetadataProvider(
	BE::IMediaMetaDataRef inMediaMetaData)
{
	ASL_ASSERT(inMediaMetaData);
	ResetMediaMetaData(inMediaMetaData);
}

CottonwoodMediaMetadataProvider::~CottonwoodMediaMetadataProvider()
{
}

void CottonwoodMediaMetadataProvider::ResetMediaMetaData(
	BE::IMediaMetaDataRef inMediaMetaData)
{
	if (inMediaMetaData != mMediaMetaData)
	{
		mMediaMetaData = inMediaMetaData;
	}

	OnMetaDataChanged();
}

/*
**
*/
void CottonwoodMediaMetadataProvider::OnMetaDataChanged()
{
	if (mMediaMetaData)
	{
		mMediaMetaData->ReadMetaDatum(
		MF::BinaryData(),
		BE::kMetaDataType_XMPBinary,
		0,
		mLastMetaDataBuffer);
	}

	// Reset buffer is what causes the controls in the metadata UI to redraw with new data.
	ResetBuffer(
		mLastMetaDataBuffer.GetSize()
		? reinterpret_cast<const dvacore::UTF8Char*>(
		mLastMetaDataBuffer.GetData()): 0,
		mLastMetaDataBuffer.GetSize());
}

/*
**
*/
bool CottonwoodMediaMetadataProvider::QueryPathStatus(
	const dvametadata::Path& inPath,
	dvametadata::PathStatus inQuery) const
{
	bool status;
	switch (inQuery)
		{
		case dvametadata::kPathStatus_ExternalIsWritable:
			status = mMediaMetaData && mMediaMetaData->CanWriteXMPToFile() && !dvametadata::IsXMPWritingDisabled();
			break;
		default:
			status = dvametadata::MetaDataProvider::QueryPathStatus(inPath, inQuery);
		}	
	return status;
}

void CottonwoodMediaMetadataProvider::WriteChanges(
	const dvametadata::MetaDataChangeList& inChanges)
{
	XMPMetaRef meta = MetaDataAccess();
	DVA_ASSERT(meta);
	if (!meta)
	{
		return;
	}

	try
	{
		dvacore::StdString buffer;
		SXMPMeta(meta).SerializeToBuffer(&buffer);

		WriteBuffer(
			inChanges,
			reinterpret_cast<const dvacore::UTF8Char*>(buffer.c_str()),
			buffer.size());

		SXMPMeta(meta).SerializeToBuffer(&buffer);
	}
	catch (const XMP_Error& e)
	{
		DVA_ASSERT_MSG(false, "CottonwoodMediaMetadataProvider::WriteChanges xmp exception: " << e.GetErrMsg());
	}
	catch (const std::exception& e)
	{
		DVA_ASSERT_MSG(false, "CottonwoodMediaMetadataProvider::WriteChanges std exception: " << e.what());
	}
	catch (...)
	{
		DVA_ASSERT(false);
	}
}


void CottonwoodMediaMetadataProvider::DoWriteChanges(
	const dvametadata::MetaDataChangeList& inChangeList)
{
	XMPMetaRef metaData = MetaDataAccess();
	if (metaData && !inChangeList.empty() && !mWriting)
	{
		mWriting = true;
		WriteChanges(inChangeList);
		mWriting = false;
	}
}

void CottonwoodMediaMetadataProvider::WriteBuffer(
	const dvametadata::MetaDataChangeList& inPaths,
	const dvacore::UTF8Char* inBuffer,
	std::size_t inSize)
{
	DVA_ASSERT(mMediaMetaData);
	if (!mMediaMetaData)
	{
		return;
	}

	// stop listening because we know it will change

	dvacore::UTF16String defaultTrack;

	MF::BinaryData newBuffer(inBuffer, inSize);
	MZ::SetMetadata(
		inPaths,
		mMediaMetaData,
		mLastMetaDataBuffer,
		newBuffer,
		defaultTrack);

	// Update this now, as it can take a long time for us to receive
	// the OnMetaDataChanged(), and we're out of sync until then.
	mLastMetaDataBuffer = newBuffer;

	ASL::StationUtils::BroadcastMessage(mMediaMetaData->GetStationID(),
		MZ::MetaDataWritingChangeListMessage(inPaths));

}

} // namespace PL
