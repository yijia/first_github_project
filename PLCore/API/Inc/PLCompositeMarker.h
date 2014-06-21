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

#ifndef PLCOMPOSITEMARKER_H
#define PLCOMPOSITEMARKER_H

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef PLMARKER_H
#include "PLMarker.h"
#endif

#ifndef IPLMARKERSELECTION_H
#include "IPLMarkerSelection.h"
#endif

namespace PL
{

PL_EXPORT
const dvacore::UTF16String GetMultipleValuesString();

class CottonwoodCompositeMarker
{
public:


	PL_EXPORT
	CottonwoodCompositeMarker(
	const PL::MarkerSet & inMarkerSelection);

	PL_EXPORT
	~CottonwoodCompositeMarker();

	PL_EXPORT 
	ASL::Guid GetGUID() const;

	PL_EXPORT 
	dvacore::UTF16String GetGUIDList() const;

	PL_EXPORT
	dvamediatypes::TickTime GetStartTime() const;

	PL_EXPORT
	dvamediatypes::TickTime GetDuration() const;
	
	PL_EXPORT
	dvacore::UTF16String GetType() const;
	
	PL_EXPORT
	dvacore::UTF16String GetName() const;

	PL_EXPORT
	dvacore::UTF16String GetComment() const;

	PL_EXPORT
	dvacore::UTF16String GetLocation() const;

	PL_EXPORT
	dvacore::UTF16String GetTarget() const;

	PL_EXPORT
	dvacore::UTF16String GetCuePointType() const;

	PL_EXPORT
	const dvatemporalxmp::CustomMarkerParamList GetCuePointList() const;
	
	PL_EXPORT
	dvacore::UTF16String GetSpeaker() const;

	PL_EXPORT
	dvacore::UTF16String GetProbability() const;

	PL_EXPORT
	dvacore::UTF16String GetPayload() const;

	PL_EXPORT
	bool HasMultipleMarkerTypes() const;

	PL_EXPORT
	bool HasMultipleMarkers() const;

	PL_EXPORT
	dvamediatypes::TickTime GetMinInPoint() const;

	PL_EXPORT
	dvamediatypes::TickTime GetMaxOutPoint() const;

	PL_EXPORT
	void SetStartTime(
		const dvamediatypes::TickTime& inTime);

	PL_EXPORT
	void SetDuration(
		const dvamediatypes::TickTime& inTime);

	PL_EXPORT
	void SetType(
		const dvacore::UTF16String& inType);

	PL_EXPORT
	void SetName(
		const dvacore::UTF16String& inName);

	PL_EXPORT
	void SetComment(
		const dvacore::UTF16String& inComment);

	PL_EXPORT
	void SetLocation(
		const dvacore::UTF16String& inLocation);

	PL_EXPORT
	void SetTarget(
		const dvacore::UTF16String& inTarget);

	PL_EXPORT
	void SetCuePointType(
		const dvacore::UTF16String& inCuePointType);

	PL_EXPORT
	void SetCuePointList(
		const dvatemporalxmp::CustomMarkerParamList& inCuePointList);

	PL_EXPORT
	void SetSpeaker(
		const dvacore::UTF16String& inSpeaker);

	PL_EXPORT
	void SetProbability(
		const dvacore::UTF16String& inProbability);

	PL_EXPORT
	dvacore::UTF8String ToPropertyXML() const;


private:
	PL::CottonwoodMarkerList	mMarkerList;
};

}

#endif