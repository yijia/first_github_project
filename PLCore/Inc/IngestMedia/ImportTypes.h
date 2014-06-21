/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2011 Adobe Systems Incorporated
*  All Rights Reserved.
*
* NOTICE:  All information contained herein is, and remains
* the property of Adobe Systems Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to Adobe Systems Incorporated and its
* suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe Systems Incorporated.
**************************************************************************/

#pragma once

#ifndef IMPORTTYPES_H
#define IMPORTTYPES_H

// MZ
#ifndef PLINGESTJOB_H
#include "IngestMedia/PLIngestJob.h"
#endif

// ASL
#ifndef ASLMESSAGEMACROS_H
#include "ASLMessageMacros.h"
#endif

#ifndef ASLBATCHOPERATIONFWD_H
#include "ASLBatchOperationFwd.h"
#endif

#ifndef ASLCRITICALSECTION_H
#include "ASLCriticalSection.h"
#endif

#ifndef ASLSTATIONID_H
#include "ASLStationID.h"
#endif

#ifndef ASLGUID_H
#include "ASLGuid.h"
#endif

#ifndef ASLATOMIC_H
#include "ASLAtomic.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef ASLCLASSFACTORY_H
#include "ASLClassFactory.h"
#endif

#ifndef ASLRESULTS_H
#include "ASLResults.h"
#endif

#ifndef ASLSLEEP_H
#include "ASLSleep.h"
#endif

#ifndef ASLSTATIONREGISTRY_H
#include "ASLStationRegistry.h"
#endif

#ifndef ASLTHREADEDQUEUEREQUEST_H
#include "ASLThreadedQueueRequest.h"
#endif

#ifndef ASLSTRINGUTILS_H
#include "ASLStringUtils.h"
#endif

#ifndef ASLBATCHOPERATION_H
#include "ASLBatchOperation.h"
#endif

// Boost
#include "boost/bind.hpp"

// DVA
#include "dvacore/threads/SharedThreads.h"

// Std
#include <deque>

namespace PL
{
	using namespace IngestTask;
	// ********************** ImportElementsQueue *************************
	template <typename T>
	class ImportElementsQueue
	{
	public:
		typedef T ElementType;

		ImportElementsQueue()
			:
		mPaused(false)
		{
		}

		bool Empty() const
		{
			ASL::CriticalSectionLock lock(mCriticalSection);
			return mElements.empty() || mPaused;
		}

		void PushBack(ElementType inElement)
		{
			ASL::CriticalSectionLock lock(mCriticalSection);
			return mElements.push_back(inElement);
		}

		bool PopFront(ElementType& outElement)
		{
			ASL::CriticalSectionLock lock(mCriticalSection);
			if (mElements.empty() || mPaused)
			{
				return false;
			}
			else
			{
				outElement = *mElements.begin();
				mElements.pop_front();
				return true;
			}
		}

		void Pause()
		{
			ASL::CriticalSectionLock lock(mCriticalSection);
			mPaused = true;
		}

		void Resume()
		{
			ASL::CriticalSectionLock lock(mCriticalSection);
			mPaused = false;
		}

	private:

		ASL::CriticalSection	mCriticalSection;
		std::deque<ElementType>	mElements;
		bool					mPaused;
	};

	typedef void (*ImportXMPProcessFn)(
		ASL::StationID const& inStationID,
		ASL::AtomicInt* inAbortFlag,
		ImportXMPElement const& inImportElement
		);

	typedef void (*RequestThumbnailProcessFn)(
		ASL::StationID const& inStationID,
		ASL::AtomicInt* inAbortFlag,
		RequestThumbnailElement const& inImportElement
		);

	ASL::IBatchOperationRef CreateImportXMPOperation(
		ImportElementsQueue<ImportXMPElement>* inImportElementsQueue,
		ImportXMPProcessFn inOperationProcessCallback);

	ASL::IBatchOperationRef CreateRequestThumbnailOperation(
		ImportElementsQueue<RequestThumbnailElement>* inImportElementsQueue,
		RequestThumbnailProcessFn inOperationProcessCallback);

//----------------------------------------------------------------
//----------------------------------------------------------------
//							Ingest Operation Base
//----------------------------------------------------------------
//----------------------------------------------------------------
template <typename T, typename F>
class IngestOperationBase 
{
protected:

	typedef T ImportQueueType;
	typedef F CallbackFnType;

	/*
	**
	*/
	IngestOperationBase()
		:
		mStationID(ASL::StationRegistry::RegisterUniqueStation()),
		mThreadDone(0),
		mAbort(0),
		mImportOperationProcessCallback(NULL)
	{
	}

	/*
	**
	*/
	virtual ~IngestOperationBase()
	{
		ASL::StationRegistry::DisposeStation(mStationID);
	}

	/*
	**
	*/
	bool GetAbort()
	{
		return (ASL::AtomicRead(mAbort) != 0);
	}

	/*
	**
	*/
	ASL::StationID GetStationID() const
	{
		return mStationID;
	}

	/*
	**
	*/
	bool Start(ASL::IThreadedQueueRequest* inThreadedQueueRequest)
	{
		mThreadedWorkQueue = dvacore::threads::CreateIOBoundExecutor();
		ASL_ASSERT(mThreadedWorkQueue != NULL);

		return (mThreadedWorkQueue != NULL) && 
			mThreadedWorkQueue->CallAsynchronously(boost::bind(&ASL::IThreadedQueueRequest::Process, ASL::IThreadedQueueRequestRef(inThreadedQueueRequest)));
	}
	
	/*
	**
	*/
	void Abort()
	{
		ASL::AtomicExchange(mAbort, 1);	
		while (ASL::AtomicRead(mThreadDone) == 0)
		{
			//	Wait for the worker thread to finish.
			ASL::Sleep(5);
		}
	}

	/*
	**
	*/
	void Process()
	{
		DVA_ASSERT(mImportOperationProcessCallback != NULL);
		if (mImportOperationProcessCallback != NULL)
		{
			typename ImportQueueType::ElementType element;
			
			do {
				if (mImportElementsQueue->PopFront(element))
				{
					mImportOperationProcessCallback(
							mStationID,
							&mAbort,
							element);
				}
				
				ASL::Sleep(20);
			} while (!GetAbort());
		}
		
		ASL::AtomicExchange(mThreadDone, 1);
	}

private:

	/*
	**
	*/
	friend ASL::IBatchOperationRef CreateImportXMPOperation(
		ImportElementsQueue<ImportXMPElement>* inImportElementsQueue,
		ImportXMPProcessFn inOperationProcessCallback);
		
	/*
	**
	*/
	friend ASL::IBatchOperationRef CreateRequestThumbnailOperation(
		ImportElementsQueue<RequestThumbnailElement>* inImportElementsQueue,
		RequestThumbnailProcessFn inOperationProcessCallback);


	/*
	**
	*/
	ASL::StationID						mStationID;
	dvacore::threads::AsyncExecutorPtr	mThreadedWorkQueue;
	ASL::AtomicInt						mAbort;
	ASL::AtomicInt						mThreadDone;
	ImportQueueType*					mImportElementsQueue;
	CallbackFnType						mImportOperationProcessCallback;
};


} // namespace PL

#endif
