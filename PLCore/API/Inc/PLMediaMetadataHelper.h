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

#ifndef PLMEDIAMETADATAHELPER_H
#define PLMEDIAMETADATAHELPER_H

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef BEMEDIA_H
#include "BEMedia.h"
#endif

#ifndef DVAMETADATA_METADATAPROVIDERS_H
#include "dvametadata/MetadataProviders.h"
#endif

namespace PL
{

	class MediaMetaDataHelper : public BE::IMediaMetaData
	{
	public:
		MediaMetaDataHelper();
		~MediaMetaDataHelper();


		// IMediaMetaData methods

		virtual bool CanWriteTimecode() const;

		virtual ASL::Result SetTimeInfo(
			const MF::BinaryData& inPrefs,
			const dvamediatypes::TickTime& inOriginalTime,
			const dvamediatypes::TickTime& inAlternateTime, 
			const ASL::String& inOriginalTapeName, 
			const ASL::String& inAlternateTapeName, 
			const ASL::String& inLogComment, 
			const dvamediatypes::FrameRate& inLogFrameRate,
			const bool inDropFrame = true);

		virtual dvamediatypes::TickTime GetOrigTime() const;
		virtual dvamediatypes::TickTime GetAltTime() const;
		virtual ASL::String GetLogComment() const;
		virtual ASL::String GetOrigReel() const;
		virtual ASL::String GetAltReel() const;
		virtual bool GetIsDropFrame() const;
		virtual dvamediatypes::PixelAspectRatio GetPixelAspectRatio() const;
		virtual dvamediatypes::FieldType GetFieldType() const;
		virtual bool GetUseAlternateStart() const;
		virtual ASL::Result GetMetadataState(dvacore::utility::Guid& outMetadataState, const dvacore::StdString& inPartID = dvacore::StdString());

		virtual ASL::Result InjectMetadata(
			BE::IClipLoggingInfoRef inClipLogging,
			BE::ICaptureSettingsRef inCaptureSettings);

		virtual ASL::Result GetXMPCaptureProperties(
			ASL::String& outTapeName,
			ASL::String& outClipName,
			ASL::String& outDescription,
			ASL::String& outScene,
			ASL::String& outShot,
			ASL::String& outLogNote,
			ASL::String& outComment,
			ASL::String& outClient,
			ASL::Date& outShotDate,
			bool& outGood,
			ASL::String& outClipID);

		virtual ASL::Result UpdateXMPMetadataFromMedia(bool inCommitChangesToDisk = false);

		/*
		**	Must be overridden by inheriting class
		*/
		virtual ASL::Result ReadMetaDatum(
			const MF::BinaryData& inPrefs,
			BE::MetaDataType inType,
			void* inID,
			MF::BinaryData& outDatum) = 0;

		/*
		**	Must be overridden by inheriting class
		*/
		virtual ASL::Result WriteMetaDatum(
			const MF::BinaryData& inPrefs,
			BE::MetaDataType inType,
			void* inID,
			const MF::BinaryData& inDatum,
			bool inAlwaysPostToUIThread = true,
			bool inRefreshFile = true,
			ASL::SInt32* outRequestID = NULL,
			const dvametadata::MetaDataChangeList& inChangeList = dvametadata::MetaDataChangeList()) = 0;

		virtual BE::MediaSceneRecords GetMediaScenes();
		virtual bool GetIsPreviewRenderNoAlphaDetected();
		virtual ASL::StationID GetStationID() const;
		virtual bool CanWriteXMPToFile() const;

	protected:
		ASL::StationID mStationID;

	};

	/*
	** Helper function to set the MF::BinaryData
	** based on type and used by the templated
	** SetMetadataProvider class
	*/
	static void SetBinaryData(
		const dvacore::UTF8Char* inBuffer,
		std::size_t inSize,
		MF::BinaryData& outData)
	{
		outData = MF::BinaryData(inBuffer, inSize);
	}

	/*
	** Helper function to set the MF::BinaryData
	** based on type and used by the templated
	** SetMetadataProvider class
	*/
	static void SetBinaryData(
		const dvacore::UTF8Char* inBuffer,
		std::size_t inSize,
		const MF::BinaryData& outData)
	{
		// NO-OP, can't write to const, which is intentional
	}

	/**
	** An implementation of the pure virtual BufferProviderImpl
	** class to read/write the XMP from a MF::BinaryData
	** buffer.
	*/
	template<typename T>
	class BufferMetadataProvider :
		public dvametadata::BufferProviderImpl
	{
	public:
		BufferMetadataProvider(
			T& inData) 
			:
			dvametadata::BufferProviderImpl(
				reinterpret_cast<const dvacore::UTF8Char*>(inData.GetData()),
				inData.GetSize()),
			mBuffer(inData)
		{
		}

	protected:
		void WriteBuffer(
			const dvametadata::MetaDataChangeList&,
			const dvacore::UTF8Char* inBuffer,
			std::size_t inSize)
		{
			// Write the buffer to the MF::BinaryData referenced by mBuffer
			SetBinaryData(inBuffer, inSize, mBuffer);
		}

	private:
		T&	mBuffer;
	};	
}

#endif //PLMEDIAMETADATAHELPER_H
