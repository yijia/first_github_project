/*************************************************************************
*
* ADOBE CONFIDENTIAL
* ___________________
*
*  Copyright 2008 Adobe Systems Incorporated
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

#ifndef PLINGESTVERIFYTYPES_H
#define PLINGESTVERIFYTYPES_H

namespace PL
{

	enum VerifyOption
	{
		kVerify_None = -1,
		kVerify_FileMD5,
		//kVerify_FolderStruct, 
		kVerify_FileSize,
		kVerify_FileContent,
		kVerify_End
	};

	enum VerifyFileResult
	{
		kVerifyFileResult_Equal = 0,
		kVerifyFileResult_ContentDifferent,
		kVerifyFileResult_FileBad,
		kVerifyFileResult_SizeDifferent,
		kVerifyFileResult_CountDifferent,
		kVerifyFileResult_NameDifferent,
		kVerifyFileResult_SrcNotExist,
		kVerifyFileResult_DestNotExist,
		kVerifyFileResult_Conflict, 
        kVerifyFileResult_Cancel,
        kVerifyFileResult_Folder,
		kVerifyFileResult_MD5Diff,
		kVerifyFileResult_FileNotExist
	};

} // namespace PL

#endif
