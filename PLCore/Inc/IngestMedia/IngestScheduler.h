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

#ifndef INGESTSCHEDULER_H
#define INGESTSCHEDULER_H

#pragma once

// ASL
#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#include "IngestMedia/PLIngestJob.h"

// Boost
#include <boost/shared_ptr.hpp>

namespace PL
{

	namespace IngestTask
	{
		//class TaskBase;
		//typedef boost::shared_ptr<TaskBase>		TaskBasePtr;
		//typedef std::list<TaskBasePtr>			TaskList;

		//enum TaskState
		//{
		//	kTaskState_Init,
		//	kTaskState_Running,
		//	kTaskState_Done,
		//	kTaskState_Paused,
		//	kTaskState_Aborted
		//};

		//enum TaskStep
		//{
		//	kTaskType_None,
		//	kTaskType_Copy,
		//	kTaskType_Import,
		//	kTaskType_Transcode
		//};

		//class TaskBase
		//{
		//public:
		//	TaskBase(TaskState inState = kTaskState_Init, TaskStep inStep = kTaskType_None);
		//	virtual ~TaskBase();

		//	TaskState		GetTaskState() const;
		//	void			SetTaskState(TaskState inState);

		//	TaskStep		GetTaskStep() const;
		//	void			SetTaskStep(TaskStep inState);

		//protected:
		//	TaskStep		mNextStep;
		//	TaskState		mTaskState;
		//};

		//class CopyTask : public TaskBase
		//{
		//public:
		//	CopyTask();
		//	virtual ~CopyTask();

		//public:
		//};

		//class ImportTask : public TaskBase
		//{
		//public:
		//	ImportTask();
		//	virtual ~ImportTask();

		//public:
		//};

		//class VerifyTask : public TaskBase
		//{
		//public:
		//	VerifyTask();
		//	virtual ~VerifyTask();

		//public:
		//};

		//class TranscodeTask : public TaskBase
		//{
		//public:
		//	TranscodeTask();
		//	virtual ~TranscodeTask();

		//public:
		//};

		//class TaskFactory
		//{
		//public:
		//	TaskFactory();
		//	~TaskFactory();

		//public:
		//	TaskBasePtr CreateTask();

		//private:
		//};

		//class TaskScheduler
		//{
		//public:
		//	TaskScheduler();
		//	~TaskScheduler();

		//public:
		//	bool Start();
		//	bool Abort();
		//	bool Pause();
		//	bool Resume();

		//	bool IsDone();

		//private:
		//	TaskList	mCopyTaskQueue;
		//	TaskList	mImportTaskQueue;
		//	TaskList	mTranscodeTaskQueue;
		//};

//#ifndef INGESTJOBTEST
//#define INGESTJOBTEST
//#endif
//
//#ifdef INGESTJOBTEST
//
//		void Test();
//
//#endif
//
	}

} // namespace PL

#endif // INGESTSCHEDULER_H