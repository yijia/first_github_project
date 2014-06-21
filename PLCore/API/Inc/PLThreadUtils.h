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

#ifndef PLTHREADUTILS_H
#define PLTHREADUTILS_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef DVACORE_THREADS_ASYNCTHREADEDEXECUTOR_H
#include "dvacore/threads/AsyncThreadedExecutor.h"
#endif

#ifndef DVACORE_THREADS_ATOMICWRAPPER_H
#include "dvacore/threads/AtomicWrapper.h"
#endif

#include "dvacore/utility/Utility.h"

#ifndef ASLTHREADEDQUEUEREQUEST_H
#include "ASLThreadedQueueRequest.h"
#endif

#include "boost/smart_ptr.hpp"
#include "boost/utility.hpp"

namespace PL
{

namespace threads
{

/*----------------------------------------------------------------
Utilities for named global threads.
Sameples:
// for tempoparily use:
const ASL::String executorName(ASL_STR("temp thread"));
CreateAndRegisterExecutor(executorName);
GetExecutor(executorName)->CallAsynchronously(...);
UnregisterExecutor(executorName);

// for long life-cicle use:
const ASL::String executorName(ASL_STR("thread available until end of main"));
CreateAndRegisterExecutor(executorName, dvacore::threads::kThreadAllocationPolicies_SingleThreaded, true);
GetExecutor(executorName)->CallAsynchronously(...);
// no UnregisterExecutor call is needed. When app exit, TerminateAllExecutors will be called and release them.

----------------------------------------------------------------*/

/**
 ** If there is an executor with given name, return NULL.
 */
PL_EXPORT
dvacore::threads::AsyncThreadedExecutorPtr CreateAndRegisterExecutor(
	const ASL::String& inExecutorName,
	int inThreadCount = dvacore::threads::kThreadAllocationPolicies_SingleThreaded,
	bool inAutoUnregister = false,
	dvacore::threads::ThreadPriority inPriority = dvacore::threads::kThreadPriority_Normal,
	const boost::function<void()>& inPreThreadFn = boost::function<void()>(),
	const boost::function<void()>& inPostThreadFn = boost::function<void()>());

PL_EXPORT
dvacore::threads::AsyncThreadedExecutorPtr GetExecutor(const ASL::String& inExecutorName);

/**
 ** Who register executor should unregister it, so only support unregister with name, rather than executor ptr.
 */
PL_EXPORT
void UnregisterExecutor(const ASL::String& inExecutorName);

PL_EXPORT
void TerminateAllExecutors();


/**
**	Why another work queue?
**	Because ASL ThreadedWorkQueue can't remove pushed requests.
**	I need a queue which can manage requests order and cancel them.
**	NOTE: work queue require that all non-const public functions are called from same thread (not need to be main thread).
*/
class WorkQueue
	:	public dvacore::utility::RefCountedObject
	,	public boost::noncopyable
{
public:
	DVA_UTIL_TYPEDEF_SHARED_REF_PTR(WorkQueue) SharedPtr;

	/**
	**	If inExecutor is null, will return null ptr.
	**	inUsableThreadCount is only a hint, count of real executing requests can be more than it.
	*/
	PL_EXPORT
	static SharedPtr Create(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount = -1);

	/**
	**	Will flush to ensure all requests are done.
	*/
	PL_EXPORT
	virtual ~WorkQueue();

	/**
	**	If same request is pushed again before it's done, it will only be executed once.
	**	Push to the end of queue.
	*/
	PL_EXPORT
	virtual void Push(ASL::IThreadedQueueRequestRef inRequest);

	/**
	**	Waiting request means the one which has not be called by executor.
	*/
	PL_EXPORT
	virtual void ClearAllWaitingRequests();

	/**
	**	Keep all pending requests in queue but don't execute them even though executor is not busy.
	*/
	PL_EXPORT
	virtual void StopExecutePendingRequests();

	/**
	**	
	*/
	PL_EXPORT
	virtual bool IsStopExecutePendingRequests() const;

	/**
	**	
	*/
	PL_EXPORT
	virtual void ResumeExecutePendingRequests();

	/**
	**	Return true if the request is pending or executing.
	*/
	PL_EXPORT
	virtual bool IsRequestInQueue(ASL::IThreadedQueueRequestRef inRequest) const;

	/**
	**	Pop a request from pending request list, so it will never be executed.
	**	Return true if the request is pending.
	**	Or return false, such as the request is not in queue or it's executing.
	*/
	PL_EXPORT
	virtual bool PopRequest(ASL::IThreadedQueueRequestRef inRequest);

	/**
	**	If the request has not been pushed, it will be pushed.
	*/
	static const ASL::UInt32 kPrioritizeType_ToHead = 0;
	static const ASL::UInt32 kPrioritizeType_ToTail = 1;
	PL_EXPORT
	virtual void PrioritizeRequest(ASL::IThreadedQueueRequestRef inRequest, ASL::UInt32 inPrioritizeType);

	/**
	**	Flush will block caller thread until all requests (including pending requests) are done.
	**	Flush will only ensure all requests which has been pushed in at the call time are done.
	**	If any requests are pushed after Flush is called, they maybe not be done after Flush return.
	*/
	PL_EXPORT
	virtual void Flush();

protected:
	/**
	**	inExecutor should be non-null.
	*/
	PL_EXPORT
	WorkQueue(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount);

	/**
	**	Remove all same instances from pending queue. Return true if any real remove operation is done.
	**	NOTE that caller should lock pending queue by itself. RemovePendingRequestWithoutLock will not lock anything.
	*/
	PL_EXPORT
	bool RemovePendingRequestWithoutLock(ASL::IThreadedQueueRequestRef inRequest);

	/**
	**	Only call request from executor. Caller maybe need to insert it into executing set.
	*/
	PL_EXPORT
	void ExecuteRequestWithoutLock(ASL::IThreadedQueueRequestRef inRequest);

	/**
	**	If executing requests don't reach max limit and there are pending requests, will start new.
	*/
	PL_EXPORT
	void StartExecutingIfNecessary();

	/**
	**	This is the real function called by executor.
	**	It will first call request's process function and then try start new executing if necessary.
	*/
	void ExecuteRequestAndTryNext(ASL::IThreadedQueueRequestRef inRequest);

	void CheckThreadID();

protected:
	dvacore::threads::AsyncThreadedExecutorPtr	mExecutor;
	size_t 										mMaxThreadCount;
	dvacore::threads::RecursiveMutex			mPendingRequestsMutex;
	std::deque<ASL::IThreadedQueueRequestRef>	mPendingRequests;
	dvacore::threads::RecursiveMutex			mExecutingRequestsMutex;
	std::set<ASL::IThreadedQueueRequestRef>		mExecutingRequests;
	dvacore::threads::AtomicBool				mStopExecutingMore;
	dvacore::threads::AtomicBool				mTerminated;

	dvacore::threads::ThreadID					mQueueOwnerThreadID;
};


ASL_DEFINE_CLASSREF(ITaskRequest, ASL::IThreadedQueueRequest);
// {18E8B8B1-BCDA-446B-8057-3E5228C6EFAB}
ASL_DEFINE_IMPLID(ITaskRequest, 0x18e8b8b1, 0xbcda, 0x446b, 0x80, 0x57, 0x3e, 0x52, 0x28, 0xc6, 0xef, 0xab);

class ITaskRequest
	:
	public ASL::IThreadedQueueRequest
{
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
		ASL_QUERY_ENTRY(ITaskRequest)
	ASL_QUERY_MAP_END

	/**
	**	
	*/
	PL_EXPORT
	ITaskRequest();

	/**
	**	
	*/
	PL_EXPORT
	virtual void Cancel();

	/**
	**	
	*/
	PL_EXPORT
	virtual bool IsCanceled() const;

	/**
	**	
	*/
	PL_EXPORT
	virtual bool IsDone() const;

protected:
	PL_EXPORT
	virtual void Done();

protected:
	dvacore::threads::AtomicBool	mCanceled;
	dvacore::threads::AtomicBool	mDone;
};

class TaskQueue : public WorkQueue
{
public:
	DVA_UTIL_TYPEDEF_SHARED_REF_PTR(TaskQueue) SharedPtr;

	/**
	**	If inExecutor is null, will return null ptr.
	**	If inUsableThreadCount less than zero, it means all thread in executor can be used.
	*/
	PL_EXPORT
	static SharedPtr Create(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount = -1);

	/**
	**	
	*/
	PL_EXPORT
	virtual ~TaskQueue();

	/**
	**	If same request is pushed again before it's done, it will only be executed once.
	**	For TaskQueue, we should ensure inRequest is an available ITaskRequestRef, so overwrite.
	**	If inRequest is not an valid ITaskRequestRef, will do nothing (assert for debug version).
	*/
	PL_EXPORT
	virtual void Push(ASL::IThreadedQueueRequestRef inRequest);

	/**
	**	If same request is pushed again before it's done, it will only be executed once.
	*/
	PL_EXPORT
	virtual void Push(ITaskRequestRef inRequest);

	/**
	**	Pop all pending requests and cancel executing requests. No blocking.
	*/
	PL_EXPORT
	virtual void CancelAll();

protected:
	/**
	**	inExecutor should be non-null.
	*/
	PL_EXPORT
	TaskQueue(dvacore::threads::AsyncThreadedExecutorPtr inExecutor, int inUsableThreadCount);

};

} // namespace thread

} // namespace PL

#endif