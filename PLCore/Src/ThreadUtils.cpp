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

//	Prefix
#include "Prefix.h"

#include "PLThreadUtils.h"

#include "boost/foreach.hpp"
#include "boost/bind.hpp"

namespace PL
{

namespace threads
{

//----------------------------------------------------------------------------
// Executor manage functions

typedef std::map<ASL::String, std::pair<dvacore::threads::AsyncThreadedExecutorPtr, bool> > ExectorMap;
static ExectorMap& GlobalExectorMap()
{
	static ExectorMap sExectorMap;
	return sExectorMap;
}

dvacore::threads::AsyncThreadedExecutorPtr CreateAndRegisterExecutor(
	const ASL::String& inExecutorName,
	int inThreadCount,
	bool inAutoUnregister,
	dvacore::threads::ThreadPriority inPriority,
	const boost::function<void()>& inPreThreadFn,
	const boost::function<void()>& inPostThreadFn)
{
	if (GlobalExectorMap().find(inExecutorName) == GlobalExectorMap().end())
	{
		dvacore::threads::AsyncThreadedExecutorPtr newExecutor = dvacore::threads::CreateAsyncThreadedExecutor(
			ASL::MakeStdString(inExecutorName),
			inThreadCount,
			inPriority,
			inPreThreadFn,
			inPostThreadFn);
		GlobalExectorMap()[inExecutorName].first = newExecutor;
		GlobalExectorMap()[inExecutorName].second = inAutoUnregister;
		return newExecutor;
	}
	else
	{
		return dvacore::threads::AsyncThreadedExecutorPtr();
	}
}

dvacore::threads::AsyncThreadedExecutorPtr GetExecutor(const ASL::String& inExecutorName)
{
	ExectorMap::iterator result = GlobalExectorMap().find(inExecutorName);
	return (result != GlobalExectorMap().end()) ? result->second.first : dvacore::threads::AsyncThreadedExecutorPtr();
}

void UnregisterExecutor(const ASL::String& inExecutorName)
{
	ExectorMap::iterator result = GlobalExectorMap().find(inExecutorName);
	if (result != GlobalExectorMap().end())
	{
		GlobalExectorMap().erase(result);
	}
}

void TerminateAllExecutors()
{
	ExectorMap::const_iterator iter = GlobalExectorMap().begin();
	ExectorMap::const_iterator iterEnd = GlobalExectorMap().end();
	for (; iter != iterEnd; ++iter)
	{
		if (!iter->second.second)
		{
			DVA_ASSERT_MSG(0, "Executor leak!" << iter->first);
		}
	}
	GlobalExectorMap().clear();
}

//-------------------------------------------------------------------------------------------
// class WorkQueue

WorkQueue::SharedPtr WorkQueue::Create(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount)
{
	int maxThreadCount = inUsableThreadCount < 0
		? inExecutor->ThreadCount()
		: std::min(inExecutor->ThreadCount(), inUsableThreadCount);
	return inExecutor ? WorkQueue::SharedPtr(new WorkQueue(inExecutor, maxThreadCount)) : WorkQueue::SharedPtr();
}

WorkQueue::WorkQueue(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount)
	: mExecutor(inExecutor)
	, mStopExecutingMore(dvacore::threads::WrapValue(false))
	, mMaxThreadCount((size_t)(inUsableThreadCount < 0 ? inExecutor->ThreadCount() : inUsableThreadCount))
	, mTerminated(dvacore::threads::WrapValue(false))
	, mQueueOwnerThreadID(dvacore::threads::GetCurrentThreadID())
{
	DVA_ASSERT_MSG(inExecutor, "WorkQueue should be constructed with valid executor!");
}

WorkQueue::~WorkQueue()
{
	CheckThreadID();

	dvacore::threads::AtomicCompareAndSet(mTerminated, false, true);

	StopExecutePendingRequests();

	std::set<ASL::IThreadedQueueRequestRef> localPendingSet;

	std::set<ASL::IThreadedQueueRequestRef> originalExecutingQueue;
	{
		dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
		originalExecutingQueue.swap(mExecutingRequests);
	}

	std::deque<ASL::IThreadedQueueRequestRef> originalPendingQueue;
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		originalPendingQueue.swap(mPendingRequests);
	}

	BOOST_FOREACH (ASL::IThreadedQueueRequestRef request, originalPendingQueue)
	{
		localPendingSet.insert(request);
	}
	BOOST_FOREACH (ASL::IThreadedQueueRequestRef request, originalExecutingQueue)
	{
		localPendingSet.erase(request);
	}
	BOOST_FOREACH (ASL::IThreadedQueueRequestRef request, localPendingSet)
	{
		mExecutor->CallAsynchronously(boost::bind(&ASL::IThreadedQueueRequest::Process, request));
	}

	Flush();
}

void WorkQueue::Push(ASL::IThreadedQueueRequestRef inRequest)
{
	CheckThreadID();
	if (inRequest == NULL)
		return;

	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		// Always push. When a request is done, all its duplicated requests will be removed from queue by working thread.
		mPendingRequests.push_back(inRequest);
	}
	StartExecutingIfNecessary();
}

void WorkQueue::ClearAllWaitingRequests()
{
	CheckThreadID();
	std::deque<ASL::IThreadedQueueRequestRef> tempQueue;
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		mPendingRequests.swap(tempQueue);
	}
}

void WorkQueue::StopExecutePendingRequests()
{
	CheckThreadID();
	dvacore::threads::AtomicCompareAndSet(mStopExecutingMore, false, true);
}

bool WorkQueue::IsStopExecutePendingRequests() const
{
	return dvacore::threads::AtomicRead(mStopExecutingMore);
}

void WorkQueue::ResumeExecutePendingRequests()
{
	CheckThreadID();
	dvacore::threads::AtomicCompareAndSet(mStopExecutingMore, true, false);
}

bool WorkQueue::IsRequestInQueue(ASL::IThreadedQueueRequestRef inRequest) const
{
	dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
	BOOST_FOREACH (ASL::IThreadedQueueRequestRef request, mPendingRequests)
	{
		if (request == inRequest)
			return true;
	}
	return false;
}

bool WorkQueue::PopRequest(ASL::IThreadedQueueRequestRef inRequest)
{
	CheckThreadID();
	bool foundExist = false;
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		foundExist = RemovePendingRequestWithoutLock(inRequest);
	}

	dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
	return mExecutingRequests.find(inRequest) == mExecutingRequests.end() ? foundExist : false;
}

void WorkQueue::PrioritizeRequest(ASL::IThreadedQueueRequestRef inRequest, ASL::UInt32 inPrioritizeType)
{
	CheckThreadID();
	switch (inPrioritizeType)
	{
	case kPrioritizeType_ToHead:
		{
			dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
			mPendingRequests.push_front(inRequest);
		}
		StartExecutingIfNecessary();
		break;
	case kPrioritizeType_ToTail:
		{
			dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
			RemovePendingRequestWithoutLock(inRequest);
			mPendingRequests.push_back(inRequest);
		}
		StartExecutingIfNecessary();
		break;
	default:
		break;
	}
}

void WorkQueue::Flush()
{
	CheckThreadID();
	std::deque<ASL::IThreadedQueueRequestRef> pendingRequests;
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		pendingRequests.swap(mPendingRequests);
	}

	{
		dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
		BOOST_FOREACH (ASL::IThreadedQueueRequestRef& request, pendingRequests)
		{
			if (mExecutingRequests.find(request) == mExecutingRequests.end())
				mExecutingRequests.insert(request);
			else
				request.Clear();
		}
	}

	BOOST_FOREACH (ASL::IThreadedQueueRequestRef& request, pendingRequests)
	{
		if (request)
			ExecuteRequestWithoutLock(request);
	}

	mExecutor->Flush();
}

bool WorkQueue::RemovePendingRequestWithoutLock(ASL::IThreadedQueueRequestRef inRequest)
{
	auto iter = std::remove(mPendingRequests.begin(), mPendingRequests.end(), inRequest);
	if (iter == mPendingRequests.end())
		return false;
	else
	{
		mPendingRequests.erase(iter, mPendingRequests.end());
		return true;
	}
}

void WorkQueue::ExecuteRequestWithoutLock(ASL::IThreadedQueueRequestRef inRequest)
{
	if (dvacore::threads::AtomicRead(mTerminated))
	{
		DVA_ASSERT_MSG(0, "Push request after terminated?!");
		return;
	}

	mExecutor->CallAsynchronously(boost::bind(&WorkQueue::ExecuteRequestAndTryNext, this, inRequest));
}

void WorkQueue::StartExecutingIfNecessary()
{
	// if no enough thread, return
	{
		dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
		if (mExecutingRequests.size() >= mMaxThreadCount)
			return;
	}

	// get one request if exist
	ASL::IThreadedQueueRequestRef nextRequest;
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		if (!mPendingRequests.empty())
		{
			nextRequest = mPendingRequests.front();
			mPendingRequests.pop_front();
		}
	}

	bool needExecute = false;
	if (nextRequest)
	{
		dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
		if (mExecutingRequests.find(nextRequest) == mExecutingRequests.end())
		{
			mExecutingRequests.insert(nextRequest);
			needExecute = true;
		}
	}

	if (needExecute)
		ExecuteRequestWithoutLock(nextRequest);

	if (!dvacore::threads::AtomicRead(mTerminated))
	{
		bool hasPendingRequests = false;
		{
			dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
			hasPendingRequests = !mPendingRequests.empty();
		}

		if (hasPendingRequests)
		{
			dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
			if (mExecutingRequests.size() < mMaxThreadCount)
				mExecutor->CallAsynchronously(boost::bind(&WorkQueue::StartExecutingIfNecessary, this));
		}
	}
}

void WorkQueue::ExecuteRequestAndTryNext(ASL::IThreadedQueueRequestRef inRequest)
{
	inRequest->Process();
	{
		dvacore::threads::RecursiveMutex::ScopedLock pendingRequestsLock(mPendingRequestsMutex);
		RemovePendingRequestWithoutLock(inRequest);
	}
	{
		dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
		mExecutingRequests.erase(inRequest);
	}

	if (!dvacore::threads::AtomicRead(mTerminated))
	{
		mExecutor->CallAsynchronously(boost::bind(&WorkQueue::StartExecutingIfNecessary, this));
	}
}

void WorkQueue::CheckThreadID()
{
	DVA_ASSERT_MSG(mQueueOwnerThreadID == dvacore::threads::GetCurrentThreadID(), "Work queue requires that all non-const public functions are called from same thread!");
}

//-------------------------------------------------------------------------------------------
// class TaskQueue

ITaskRequest::ITaskRequest()
	: mCanceled(dvacore::threads::WrapValue(false))
	, mDone(dvacore::threads::WrapValue(false))
{
}

void ITaskRequest::Cancel()
{
	dvacore::threads::AtomicCompareAndSet(mCanceled, false, true);
}

bool ITaskRequest::IsCanceled() const
{
	return dvacore::threads::AtomicRead(mCanceled);
}

void ITaskRequest::Done()
{
	dvacore::threads::AtomicCompareAndSet(mDone, false, true);
}

bool ITaskRequest::IsDone() const
{
	return dvacore::threads::AtomicRead(mDone);
}

//-------------------------------------------------------------------------------------------
// class TaskQueue

TaskQueue::SharedPtr TaskQueue::Create(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount)
{
	int maxThreadCount = inUsableThreadCount < 0
		? inExecutor->ThreadCount()
		: std::min(inExecutor->ThreadCount(), inUsableThreadCount);
	return inExecutor ? TaskQueue::SharedPtr(new TaskQueue(inExecutor, maxThreadCount)) : TaskQueue::SharedPtr();
}

TaskQueue::TaskQueue(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount)
	: WorkQueue(inExecutor, inUsableThreadCount)
{
}

TaskQueue::~TaskQueue()
{
}

void TaskQueue::Push(ASL::IThreadedQueueRequestRef inRequest)
{
	if (ITaskRequestRef(inRequest))
		WorkQueue::Push(inRequest);
}

void TaskQueue::Push(ITaskRequestRef inRequest)
{
	WorkQueue::Push(ASL::IThreadedQueueRequestRef(inRequest));
}

void TaskQueue::CancelAll()
{
	CheckThreadID();
	ClearAllWaitingRequests();
	dvacore::threads::RecursiveMutex::ScopedLock executingRequestsLock(mExecutingRequestsMutex);
	BOOST_FOREACH (ASL::IThreadedQueueRequestRef request, mExecutingRequests)
	{
		ITaskRequestRef(request)->Cancel();
	}
}

} // namespace thread

} // namespace PL
