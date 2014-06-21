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

#ifndef PLSEQUENCECONTEXTINFO_H
#define PLSEQUENCECONTEXTINFO_H

//	 UIF
#ifndef UIFMULTICONTEXTHANDLER_H
#include "UIFMultiContextHandler.h"
#endif

//	BE
#ifndef BESEQUENCE_H
#include "BESequence.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef IPLPRIMARYCLIP_H
#include "IPLPrimaryClip.h"
#endif


namespace PL {

PL_EXPORT
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_NULL;
PL_EXPORT
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_Clip;
PL_EXPORT
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_RoughCut;
PL_EXPORT
extern const UIF::MultiContextHandler::ContextID	kSequenceContextID_Empty;

struct SequenceContextInfo
{
	UIF::MultiContextHandler::ContextID mContextID;
	PL::ISRPrimaryClipRef mPrimaryClip;
	ASL::String mName;
	bool mIsActive;
	SequenceContextInfo(
		UIF::MultiContextHandler::ContextID inContextID,
		PL::ISRPrimaryClipRef inPrimaryClip,
		ASL::String const& inName,
		bool inIsActive)
		: mContextID(inContextID),mPrimaryClip(inPrimaryClip),  mName(inName), mIsActive(inIsActive) {};
};
typedef boost::shared_ptr<SequenceContextInfo> SequenceContextInfoPtr;
typedef std::map<UIF::MultiContextHandler::ContextID, SequenceContextInfoPtr> SequenceContextInfoMap;

class SequenceContextInfoSupport
	: 
	public ASL::Listener
{
public:
	typedef boost::shared_ptr<SequenceContextInfoSupport>	SharedPtr;

	ASL_MESSAGE_MAP_DECLARE();

	/*
	**
	*/
	PL_EXPORT
	static SequenceContextInfoSupport::SharedPtr GetInstance();

	/*
	**
	*/
	PL_EXPORT
	static void Initialize();

	/**
	**
	*/
	PL_EXPORT
	static void Terminate();

	PL_EXPORT
	SequenceContextInfoMap GetSequenceContextInfoMap();

	PL_EXPORT
	SequenceContextInfoPtr GetSequenceContextInfo(UIF::MultiContextHandler::ContextID inContextID);

	PL_EXPORT
	SequenceContextInfoPtr GetActiveSequenceContextInfo();

	PL_EXPORT
	bool AllowCloseContext(UIF::MultiContextHandler::ContextID inContextID);

	PL_EXPORT
	bool SwitchContext(UIF::MultiContextHandler::ContextID inContextID);

	PL_EXPORT
	void CloseContext(UIF::MultiContextHandler::ContextID inContextID);

	/**
	**
	*/
	PL_EXPORT
	~SequenceContextInfoSupport();


private:
	SequenceContextInfoSupport();

	/*
	**
	*/
	void OnEditModeChanged();

	/*
	**
	*/
	void OnSilkRoadMediaDirtyChanged();

	/*
	**
	*/
	void OnSilkRoadRoughCutDirtyChanged();

	/*
	**
	*/
	void OnLoggingClipRelinked();

	/*
	**
	*/
	void OnRoughCutRelinked();

	/*
	**
	*/
	void OnPrimaryClipRenamed(
		const ASL::String& inMediaPath,
		const ASL::String& inNewName,
		const ASL::String& inOldName);

	void InitializeSequenceContextInfos();
	void RemoveAllSequenceContextInfos();
	ASL::String GetAssetName(ISRPrimaryClipRef inPrimaryClip);

	static SequenceContextInfoSupport::SharedPtr	sSequenceContextInfoSupport;

	SequenceContextInfoMap mSequenceContextInfoMap;
};

} // namespace PL

#endif // PLSEQUENCECONTEXTINFO_H
