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

#include "Prefix.h"

// local
#include "PLAsyncCallAssembler.h"

//	MZ
#include "MZImportFailure.h"

//	ASL
#include "ASLStationID.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLThreadManager.h"
#include "ASLTypes.h"
#include "ASLListener.h"
#include "ASLMessageMap.h"
#include "ASLMessageMacros.h"
#include "ASLResults.h"
#include "ASLSleep.h"
#include "PLLibrarySupport.h"

//	BE

// dva
#include "dvacore/threads/SharedThreads.h"

namespace PL
{

namespace SRAsyncCallAssembler
{

ASL::StationID const kStation_SRAsyncCallAssemblerImpl(ASL_STATIONID("AC.Station.SRAsyncCallAssemblerImpl"));

struct ImportArgumentsWrapper
{
	const ASL::String&		mFilePath;
	MZ::ImportResultVector&		mImportResult;
	PL::CustomMetadata&		mCustomMetadata;
	XMPText					mXMPBuffer;

	ImportArgumentsWrapper(
			const ASL::String& inFilePath,
			MZ::ImportResultVector& outImportResult,
			PL::CustomMetadata& outCustomMetadata,
			XMPText inXMPBuffer)
		:
		mFilePath(inFilePath),
		mImportResult(outImportResult),
		mCustomMetadata(outCustomMetadata),
		mXMPBuffer(inXMPBuffer)
	{
	}
};

struct RequestThumbnailArgumentsWrapper
{
	const ASL::String&			mFilePath;
	const dvamediatypes::TickTime& mClipTime;
	SRLibrarySupport::RequestThumbnailCallbackFn	mRequestThumbnailCallbackFn;
	
	RequestThumbnailArgumentsWrapper(
			const ASL::String& inFilePath, 
			const dvamediatypes::TickTime& inClipTime,
			SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn)
		:
		mFilePath(inFilePath),
		mClipTime(inClipTime),
		mRequestThumbnailCallbackFn(inRequestThumbnailCallbackFn)
	{
	}
};

ASL_DECLARE_MESSAGE_WITH_4_PARAM(
								 InternalRouteImportToMainThreadMessage,
								 ImportArgumentsWrapper*,
								 volatile ASL::AtomicInt*,   
								 volatile ASL::AtomicInt*,
								 ImportXMPImplFn);	


ASL_DECLARE_MESSAGE_WITH_3_PARAM(
								 InternalRouteRequestThumbnailToMainThreadMessage,
								 RequestThumbnailArgumentsWrapper*,
								 volatile ASL::AtomicInt*,
								 RequestThumbnailImplFn);

class SRAsyncCallAssemblerImpl;
typedef boost::shared_ptr<SRAsyncCallAssemblerImpl> SRAsyncCallAssemblerImplPtr;

class SRAsyncCallAssemblerImpl : public ASL::Listener
{
	ASL_MESSAGE_MAP_DECLARE();
	
public:
	static void Initialize();

	static void Terminate();

	static SRAsyncCallAssemblerImplPtr Instance();
	
	~SRAsyncCallAssemblerImpl();
	
	ASL::Result ImportXMPInMainThread(
					const ASL::String& inFilePath,
					MZ::ImportResultVector& outFilesFailure,
					PL::CustomMetadata& outCustomMetadata,
					XMPText outXMPBuffer,
					ImportXMPImplFn inImportXMPImplFn);
	
	void RequestThumbnailInMainThread(
					const ASL::String& inFilePath,
					const dvamediatypes::TickTime& inClipTime,
					RequestThumbnailImplFn inRequestThumbnailImplFn,
					SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn);

private:
	SRAsyncCallAssemblerImpl();
	
	void OnRouteImportToMainThread(
			 ImportArgumentsWrapper* inImportArgumentsWrapper,
			 volatile ASL::AtomicInt* outDone,   
			 volatile ASL::AtomicInt* outResult,
			 ImportXMPImplFn inImportXMPImplFn);
			 
	void OnInternalRouteRequestThumbnailToMainThread(
			 RequestThumbnailArgumentsWrapper* inRequestThumbnailArgumentsWrapper,
			 volatile ASL::AtomicInt* outDone,   
			 RequestThumbnailImplFn inRequestThumbnailImplFn);
	
private:
	static SRAsyncCallAssemblerImplPtr sSRAsyncCallAssemblerImpl;
};

		
SRAsyncCallAssemblerImplPtr SRAsyncCallAssemblerImpl::sSRAsyncCallAssemblerImpl;

void SRAsyncCallAssemblerImpl::Initialize()
{
	if (sSRAsyncCallAssemblerImpl == NULL)
	{
		ASL::StationRegistry::RegisterStation(kStation_SRAsyncCallAssemblerImpl);
		sSRAsyncCallAssemblerImpl.reset(new SRAsyncCallAssemblerImpl);
	}
}
	
void SRAsyncCallAssemblerImpl::Terminate()
{
	sSRAsyncCallAssemblerImpl.reset((SRAsyncCallAssemblerImpl*)NULL);
	ASL::StationRegistry::DisposeStation(kStation_SRAsyncCallAssemblerImpl);
}

SRAsyncCallAssemblerImplPtr SRAsyncCallAssemblerImpl::Instance()
{
	return sSRAsyncCallAssemblerImpl;
}
	
SRAsyncCallAssemblerImpl::SRAsyncCallAssemblerImpl()
{
	ASL::StationUtils::AddListener(kStation_SRAsyncCallAssemblerImpl, this);
}

SRAsyncCallAssemblerImpl::~SRAsyncCallAssemblerImpl()
{
	ASL::StationUtils::RemoveListener(kStation_SRAsyncCallAssemblerImpl, this);
}	

ASL::Result SRAsyncCallAssemblerImpl::ImportXMPInMainThread(
										const ASL::String& inFilePath,
										MZ::ImportResultVector& outFilesFailure,
										PL::CustomMetadata& outCustomMetadata,
										XMPText outXMPBuffer,
										ImportXMPImplFn inImportXMPImplFn)
{
	if (dvacore::threads::CurrentThreadIsMainThread())
	{
		return inImportXMPImplFn(inFilePath, outFilesFailure, outCustomMetadata, outXMPBuffer);
	}
	else 
	{
		ASL::AtomicInt volatile result = 0;
		ASL::AtomicInt volatile done = 0;
		ImportArgumentsWrapper arguments(inFilePath, outFilesFailure, outCustomMetadata, outXMPBuffer);
		ASL::StationUtils::PostMessageToUIThread(kStation_SRAsyncCallAssemblerImpl, InternalRouteImportToMainThreadMessage(&arguments, &done, &result, inImportXMPImplFn));
		do {
			ASL::Sleep(10);
		} while (ASL::AtomicRead(done) == 0);
		return (ASL::Result)ASL::AtomicRead(result);
	}
}									

void SRAsyncCallAssemblerImpl::OnRouteImportToMainThread(
		 ImportArgumentsWrapper* ioImportArgumentsWrapper,
		 volatile ASL::AtomicInt* outDone,   
		 volatile ASL::AtomicInt* outResult,
		 ImportXMPImplFn inImportXMPImplFn
		)
{
	ASL::Result result = inImportXMPImplFn(
								ioImportArgumentsWrapper->mFilePath,
								ioImportArgumentsWrapper->mImportResult,
								ioImportArgumentsWrapper->mCustomMetadata,
								ioImportArgumentsWrapper->mXMPBuffer);
	
	ASL::AtomicExchange(*outResult, (ASL::AtomicInt)result);
	ASL::AtomicExchange(*outDone, (ASL::AtomicInt)1);
}

void SRAsyncCallAssemblerImpl::OnInternalRouteRequestThumbnailToMainThread(
		 RequestThumbnailArgumentsWrapper* inRequestThumbnailArgumentsWrapper,
		 volatile ASL::AtomicInt* outDone,   
		 RequestThumbnailImplFn inRequestThumbnailImplFn
		 )
{
	inRequestThumbnailImplFn(
			inRequestThumbnailArgumentsWrapper->mFilePath,
			inRequestThumbnailArgumentsWrapper->mClipTime,
			inRequestThumbnailArgumentsWrapper->mRequestThumbnailCallbackFn);
					
	ASL::AtomicExchange(*outDone, (ASL::AtomicInt)1);
}		 

void SRAsyncCallAssemblerImpl::RequestThumbnailInMainThread(
		const ASL::String& inFilePath,
		const dvamediatypes::TickTime& inClipTime,
		RequestThumbnailImplFn inRequestThumbnailImplFn,
		SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn)
{
	if (dvacore::threads::CurrentThreadIsMainThread())
	{
		return inRequestThumbnailImplFn(inFilePath, inClipTime, inRequestThumbnailCallbackFn);
	}
	else 
	{
		ASL::AtomicInt volatile done = 0;
		RequestThumbnailArgumentsWrapper arguments(inFilePath, inClipTime, inRequestThumbnailCallbackFn);
		ASL::StationUtils::PostMessageToUIThread(kStation_SRAsyncCallAssemblerImpl, InternalRouteRequestThumbnailToMainThreadMessage(&arguments, &done, inRequestThumbnailImplFn));
		do {
			ASL::Sleep(10);
		} while (ASL::AtomicRead(done) == 0);
	}
}

void Initialize()
{
	SRAsyncCallAssemblerImpl::Initialize();
}

void Terminate()
{
	SRAsyncCallAssemblerImpl::Terminate();
}

bool NeedImportXMPInMainThread(const ASL::String& inFilePath)
{
	std::size_t len = inFilePath.length();
	// ImporterFlash crash if it is being executed in worker thread. So get thumbnail in main thread for SWF file.
	if (len >= 4 && inFilePath[len-4] == '.')
	{ 
		if (
			(inFilePath[len-3] == 's' || inFilePath[len-3] == 'S') && 
			(inFilePath[len-2] == 'w' || inFilePath[len-2] == 'W') && 
			(inFilePath[len-1] == 'f' || inFilePath[len-1] == 'F')
			)
		{
			return true;
		}
	}
	return false;
}

ASL::Result ImportXMPInMainThread(
				const ASL::String& inFilePath,
				MZ::ImportResultVector& outFilesFailure,
				PL::CustomMetadata& outCustomMetadata,
				XMPText outXMPBuffer,
				ImportXMPImplFn inImportXMPImplFn)
{
	return 	SRAsyncCallAssemblerImpl::Instance()->ImportXMPInMainThread(
													inFilePath,
													outFilesFailure,
													outCustomMetadata,
													outXMPBuffer,
													inImportXMPImplFn);
}

bool NeedRequestThumbnailInMainThread(const ASL::String& inFilePath)
{
	return NeedImportXMPInMainThread(inFilePath);
}

void RequestThumbnailInMainThread(
				const ASL::String& inFilePath,
				const dvamediatypes::TickTime& inClipTime,
				RequestThumbnailImplFn inRequestThumbnailImplFn,
				SRLibrarySupport::RequestThumbnailCallbackFn inRequestThumbnailCallbackFn)
{
	SRAsyncCallAssemblerImpl::Instance()->RequestThumbnailInMainThread(
											inFilePath,
											inClipTime,
											inRequestThumbnailImplFn,
											inRequestThumbnailCallbackFn);
}				

ASL_MESSAGE_MAP_DEFINE(SRAsyncCallAssemblerImpl)
	ASL_MESSAGE_HANDLER(InternalRouteImportToMainThreadMessage, OnRouteImportToMainThread)
	ASL_MESSAGE_HANDLER(InternalRouteRequestThumbnailToMainThreadMessage, OnInternalRouteRequestThumbnailToMainThread)
ASL_MESSAGE_MAP_END

}
} // End MZ
