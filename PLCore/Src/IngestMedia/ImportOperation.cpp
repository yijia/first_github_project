/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2011 Adobe Systems Incorporated                       */
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

// Local
#include "IngestMedia/ImportTypes.h"

// ASL
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLSleep.h"
#include "ASLFile.h"


namespace PL
{


ASL_DEFINE_CLASSREF(ImportXMPOperation, ASL::IBatchOperation);

typedef IngestOperationBase<ImportElementsQueue<ImportXMPElement>, ImportXMPProcessFn> ImportXMPBaseOperation;

class ImportXMPOperation 
	:
	public ASL::IBatchOperation,
	public ASL::IThreadedQueueRequest,
	public ImportXMPBaseOperation
{
	ASL_BASIC_CLASS_SUPPORT(ImportXMPOperation);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ASL::IBatchOperation)
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
	ASL_QUERY_MAP_END

public:

	/*
	**
	*/
	ImportXMPOperation()
	{
	}


	/*
	**
	*/
	virtual ~ImportXMPOperation()
	{
	}

private:

	/*
	**
	*/
	ASL::StationID GetStationID() const
	{
		return ImportXMPBaseOperation::GetStationID();
	}

	/*
	**
	*/
	ASL::String GetDescription() const
	{
		return ASL_STR("ImportXMPOperation");
	}

	/*
	**
	*/
	bool Start()
	{
		return ImportXMPBaseOperation::Start(this);
	}
	
	/*
	**
	*/
	void Abort()
	{
		ImportXMPBaseOperation::Abort();
	}

	/*
	**
	*/
	void Process()
	{
		ImportXMPBaseOperation::Process();
	}	
};

// *******************************************************************************************
ASL_DEFINE_CLASSREF(RequestThumbnailOperation, ASL::IBatchOperation);
typedef IngestOperationBase<ImportElementsQueue<RequestThumbnailElement>, RequestThumbnailProcessFn> RequestThumbnailBaseOperation;
class RequestThumbnailOperation 
	:
	public ASL::IBatchOperation,
	public ASL::IThreadedQueueRequest,
	public RequestThumbnailBaseOperation
{
	ASL_BASIC_CLASS_SUPPORT(RequestThumbnailOperation);
	ASL_QUERY_MAP_BEGIN
		ASL_QUERY_ENTRY(ASL::IBatchOperation)
		ASL_QUERY_ENTRY(ASL::IThreadedQueueRequest)
	ASL_QUERY_MAP_END

public:

	/*
	**
	*/
	RequestThumbnailOperation() 
	{
	}


	/*
	**
	*/
	virtual ~RequestThumbnailOperation()
	{
	}

private:

	/*
	**
	*/
	ASL::StationID GetStationID() const
	{
		return RequestThumbnailBaseOperation::GetStationID();
	}

	/*
	**
	*/
	ASL::String GetDescription() const
	{
		return ASL_STR("RequestThumbnailOperation");
	}

	/*
	**
	*/
	bool Start()
	{
		return RequestThumbnailBaseOperation::Start(this);
	}
	
	/*
	**
	*/
	void Abort()
	{
		RequestThumbnailBaseOperation::Abort();
	}

	/*
	**
	*/
	void Process()
	{
		RequestThumbnailBaseOperation::Process();
	}	
};

/*
**
*/
ASL::IBatchOperationRef CreateImportXMPOperation(
		ImportElementsQueue<ImportXMPElement>* inImportElementsQueue,
		ImportXMPProcessFn inOperationProcessCallback)
{
	ImportXMPOperationRef operation = ImportXMPOperation::CreateClassRef();
	DVA_ASSERT(inImportElementsQueue != NULL && inOperationProcessCallback != NULL);
	operation->mImportElementsQueue = inImportElementsQueue;
	operation->mImportOperationProcessCallback = inOperationProcessCallback;
	return ASL::IBatchOperationRef(operation);
}

/*
**
*/
ASL::IBatchOperationRef CreateRequestThumbnailOperation(
		ImportElementsQueue<RequestThumbnailElement>* inImportElementsQueue,
		RequestThumbnailProcessFn inOperationProcessCallback)
{
	RequestThumbnailOperationRef operation = RequestThumbnailOperation::CreateClassRef();
	DVA_ASSERT(inImportElementsQueue != NULL && inOperationProcessCallback != NULL);
	operation->mImportElementsQueue = inImportElementsQueue;
	operation->mImportOperationProcessCallback = inOperationProcessCallback;
	return ASL::IBatchOperationRef(operation);
}
} // namespace PL
