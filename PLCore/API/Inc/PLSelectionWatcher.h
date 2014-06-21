/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2008 Adobe Systems Incorporated		               */
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

#ifndef PLSELECTIONWATCHER_H
#define PLSELECTIONWATCHER_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef ASLLISTENER_H
#include "ASLListener.h"
#endif

#ifndef ASLMESSAGEMAP_H
#include "ASLMessageMap.h"
#endif

#ifndef ASLTYPEINFO_H
#include "ASLTypeInfo.h"
#endif

#ifndef MZINFOSTATION_H
#include "MZInfoStation.h"
#endif

#ifndef PLCLIPDURATIONMAP_H
#include "PLClipDurationMap.h"
#endif

#ifndef MLPLAYMODE_H
#include "MLPlayMode.h"
#endif

#ifndef BESEQUENCEMESSAGES_H
#include "BESequenceMessages.h"
#endif

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include "boost/shared_ptr.hpp"
#endif

#ifndef DVACORE_UTILITY_SHAREDREFPTR_H
#include "dvacore/utility/sharedRefPtr.h"
#endif

#ifndef PLASSETMEDIAINFOSELECTION_H
#include "PLAssetMediaInfoSelection.h"
#endif


namespace PL 
{

class SRSelectionWatcher;
typedef boost::shared_ptr<SRSelectionWatcher>	SRSelectionWatcherPtr;

class SelectionWatcherClient
	:
	public dvacore::utility::RefCountedObject
{
public:

	PL_EXPORT
	virtual void SelectionDidChange() = 0;
	PL_EXPORT
	virtual void UpdateMetadata() = 0;
	PL_EXPORT
	virtual ~SelectionWatcherClient() { }
};

DVA_UTIL_TYPEDEF_SHARED_REF_PTR(SelectionWatcherClient) SelectionWatcherClientPtr;
typedef std::list<SelectionWatcherClientPtr>		SelectionWatcherClientList;
typedef std::vector<ASL::ASLUnknownRef>				SelectionContainer;

/**
** Watch for the recent selection on time line editor or library
*/
class SRSelectionWatcher 
	:
	public ASL::Listener
{	
public:
	/*
	**
	*/
	PL_EXPORT
	SRSelectionWatcher();

	/*
	**
	*/
	PL_EXPORT
	virtual ~SRSelectionWatcher();

	/*
	**
	*/
	PL_EXPORT
	void Register(SelectionWatcherClientPtr inClient);

	/*
	**
	*/
	PL_EXPORT
	void UnRegister(SelectionWatcherClientPtr inClient);

	/*
	**
	*/
	PL_EXPORT
	SelectionContainer GetSelectedItems();
	
	/*
	**
	*/
	PL_EXPORT
	ASL::TypeInfo& GetSelectedItemsType();

	ASL_MESSAGE_MAP_DECLARE(); 

private:

	/*
	**
	*/
	void OnMultipleItemsInfo(
		MZ::MultipleItemsInfo inInfo);

	/*
	**
	*/
	void OnNothingInfo(bool);


	/*
	**
	*/
	void OnTrackItemSelected(BE::ISequenceRef inSequence,
							 BE::ITrackItemRef inTrackItem,
							 bool inAudioDisplay);

	/*
	**
	*/
	void OnLinkedTrackItemSelected(
							BE::ISequenceRef inSequence,
							BE::ITrackItemRef inFirstTrackItem,
							BE::ITrackItemRef inSecondTrackItem,
							bool	inAudioDisplay);

	/*
	**
	*/
	void OnPLStaticMetadataChanged();

	/*
	** Handle a clip selection from the EA Browser 
	*/
	void OnAssetMediaInfoSelection(
		PL::AssetMediaInfoSelectionList const& inSelection);

	/*
	**
	*/
	void SelectZeroItems();

	/*
	**
	*/
	void SelectSingleItem(BE::IMasterClipRef inItem);

	/*
	**
	*/
	void SelectMultipleItems(const std::vector<BE::IMasterClipRef>& inItem);

	/*
	**
	*/
	bool ClearStoredSelection();

	/*
	**
	*/
	void NotifySelectionChanged();
	
private:

	SelectionWatcherClientList	mClientList;
	ASL::TypeInfo				mSelectionType;
	SelectionContainer			mSelectedItems;
};

} // namespace PLSELECTIONWATCHER_H

#endif

