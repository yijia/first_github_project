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

#include "ASLCoercion.h"
#include "ASLDebug.h"
#include "PLCompositeMarker.h"
#include "IPLPrimaryClipPlayback.h"
#include "PLModulePicker.h"

// dvacore
#include "dvacore/config/Localizer.h"


namespace PL
{

const dvacore::UTF16String GetMultipleValuesString()
{
	return dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
}

CottonwoodCompositeMarker::CottonwoodCompositeMarker(
	const PL::MarkerSet & inMarkerSelection) 
{
	mMarkerList = PL::BuildMarkersFromSelection(inMarkerSelection);

	// Defensive code, but this is not good solution. TODO zyfan: refactor CottonwoodCompositeMarker later.
	if (mMarkerList.empty())
	{
		mMarkerList.push_back(CottonwoodMarker());
	}
}

CottonwoodCompositeMarker::~CottonwoodCompositeMarker()
{
}


dvamediatypes::TickTime CottonwoodCompositeMarker::GetStartTime() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	return mMarkerList[0].GetStartTime();
}

dvamediatypes::TickTime CottonwoodCompositeMarker::GetDuration() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	return mMarkerList[0].GetDuration();
}

dvamediatypes::TickTime CottonwoodCompositeMarker::GetMinInPoint() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	if (mMarkerList.empty())
	{
		return dvamediatypes::kTime_Zero;
	}

	dvamediatypes::TickTime minInPoint = dvamediatypes::kTime_Max;
	BOOST_FOREACH(CottonwoodMarker const& eachMarker, mMarkerList)
	{
		minInPoint = std::min(minInPoint, eachMarker.GetStartTime());
	}

	return minInPoint;
}

dvamediatypes::TickTime CottonwoodCompositeMarker::GetMaxOutPoint() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	if (mMarkerList.empty())
	{
		return dvamediatypes::kTime_Invalid;
	}

	dvamediatypes::TickTime maxOutPoint = dvamediatypes::kTime_Min;
	BOOST_FOREACH(CottonwoodMarker const& eachMarker, mMarkerList)
	{
		maxOutPoint = std::max(maxOutPoint, (eachMarker.GetStartTime() + eachMarker.GetDuration()));
	}

	return maxOutPoint;
}

ASL::Guid CottonwoodCompositeMarker::GetGUID() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	return mMarkerList[0].GetGUID();
}


dvacore::UTF16String CottonwoodCompositeMarker::GetGUIDList() const
{
	DVA_ASSERT(mMarkerList.size() > 0);

	// The guid list is a comma-separated string of all the guids
	// in this composite marker
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String guidString = currentIter->GetGUID().AsString();
	currentIter++;

	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		guidString.append(DVA_STR(","));
		guidString.append(currentIter->GetGUID().AsString());
	}
	return guidString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetType() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	bool allMatch = true;
	dvacore::UTF16String typeString = currentIter->GetType();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (typeString != currentIter->GetType())
		{
			typeString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}
	return typeString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetName() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	bool allMatch = true;
	dvacore::UTF16String nameString = currentIter->GetNameOrTheFirstKeyword();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (nameString != currentIter->GetNameOrTheFirstKeyword())
		{
			nameString = dvacore::ZString("$$$/Prelude/PLCore/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return nameString;
}


bool CottonwoodCompositeMarker::HasMultipleMarkers() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	return mMarkerList.size() > 1;
}

bool CottonwoodCompositeMarker::HasMultipleMarkerTypes() const
{
	bool hasMultipleMarkerTypes = false;

	if (!mMarkerList.empty())
	{
		PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
		dvacore::UTF16String	markerType = currentIter->GetType();
		// should hit end at the same time, only check one
		for (; !hasMultipleMarkerTypes && (currentIter != mMarkerList.end()); ++currentIter)
		{
				// if guid changes they are different markers, if type changes we need to rebuild list item
			hasMultipleMarkerTypes = (markerType!= currentIter->GetType());
		}
	}

	return hasMultipleMarkerTypes;
}

void CottonwoodCompositeMarker::SetStartTime(
	const dvamediatypes::TickTime& inTime)
{
	DVA_ASSERT(inTime >= dvamediatypes::kTime_Zero);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetStartTime(inTime);
	}
}

void CottonwoodCompositeMarker::SetDuration(
	const dvamediatypes::TickTime& inTime)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetDuration(inTime);
	}
}

void CottonwoodCompositeMarker::SetType(
	const dvacore::UTF16String& inType)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetType(inType);
	}
}

void CottonwoodCompositeMarker::SetName(
	const dvacore::UTF16String& inName)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetName(inName);
	}
}

void CottonwoodCompositeMarker::SetComment(
	const dvacore::UTF16String& inComment)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetComment(inComment);
	}
}

void CottonwoodCompositeMarker::SetLocation(
	const dvacore::UTF16String& inLocation)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetLocation(inLocation);
	}
}

void CottonwoodCompositeMarker::SetTarget(
	const dvacore::UTF16String& inTarget)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetTarget(inTarget);
	}
}

void CottonwoodCompositeMarker::SetCuePointType(
	const dvacore::UTF16String& inCuePointType)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetCuePointType(inCuePointType);
	}
}

void CottonwoodCompositeMarker::SetCuePointList(
	const dvatemporalxmp::CustomMarkerParamList& inCuePointList)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	mMarkerList[0].SetCuePointList(inCuePointList);
}

void CottonwoodCompositeMarker::SetSpeaker(
	const dvacore::UTF16String& inSpeaker)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetSpeaker(inSpeaker);
	}
}

void CottonwoodCompositeMarker::SetProbability(
	const dvacore::UTF16String& inProbability)
{
	DVA_ASSERT(mMarkerList.size() > 0);
	PL::CottonwoodMarkerList::iterator currentIter = mMarkerList.begin();
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentIter->SetProbability(inProbability);
	}
}

dvacore::UTF16String CottonwoodCompositeMarker::GetComment() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetComment();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetComment())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return returnString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetLocation() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetLocation();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetLocation())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return returnString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetTarget() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetTarget();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetTarget())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return returnString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetCuePointType() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetCuePointType();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetCuePointType())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return returnString;
}

const dvatemporalxmp::CustomMarkerParamList CottonwoodCompositeMarker::GetCuePointList() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	dvatemporalxmp::CustomMarkerParamList paramList;

	if (HasMultipleMarkerTypes())
	{
		// Return empty param list if the marker types are different
		return paramList;
	}
	else if (! HasMultipleMarkers())
	{
		// If there is only one marker, return its cuepoint list.
		return mMarkerList[0].GetCuePointList();
	}

	// Loop through all the cuepoint params for each marker in the marker list.
	// For each unique param found, keep track of the count of how many times the same value
	// is encountered. At the end of the loop, construct the return param list
	// by looping through the param values map and setting the corresponding value in the param list to the
	// actual value only if the counts for the param equals the number of markers (all markers
	// have the same value for the param). Otherwise, set the param value to the
	// multiple value string.

	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	std::map<dvacore::UTF16String, dvacore::UTF16String> currentFieldValues;
	std::map<dvacore::UTF16String, int> equalValuesCount;

	dvatemporalxmp::CustomMarkerParamList currentParamList;
	dvatemporalxmp::CustomMarkerParamList::iterator currentParamIter;
	for (; currentIter != mMarkerList.end(); ++currentIter)	
	{
		currentParamList = currentIter->GetCuePointList();
		for (currentParamIter = currentParamList.begin(); currentParamIter != currentParamList.end(); currentParamIter++)
		{
			if (currentFieldValues.find(currentParamIter->mKey) == currentFieldValues.end())
			{
				currentFieldValues[currentParamIter->mKey] = currentParamIter->mValue;
				equalValuesCount[currentParamIter->mKey] = 1;
			}		
			else if (currentFieldValues[currentParamIter->mKey] == currentParamIter->mValue)
			{
				equalValuesCount[currentParamIter->mKey] ++;
			}
		}
	}

	// Construct return param list
	dvatemporalxmp::CustomMarkerParamList returnParamList;
	std::map<dvacore::UTF16String, dvacore::UTF16String>::iterator fieldValuesIter = currentFieldValues.begin();

	for (; fieldValuesIter != currentFieldValues.end(); fieldValuesIter++)
	{
		// Replace values with the actual string here if the count is 
		// equal to the number of markers.
		dvatemporalxmp::CustomMarkerParam newParam(fieldValuesIter->first, GetMultipleValuesString());
		if (equalValuesCount[fieldValuesIter->first] == mMarkerList.size())
		{
			newParam.mValue = fieldValuesIter->second;
		}
		returnParamList.push_back(newParam);
	}

	return returnParamList;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetSpeaker() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetSpeaker();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetSpeaker())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}
	return returnString;
}

dvacore::UTF16String CottonwoodCompositeMarker::GetProbability() const
{
	DVA_ASSERT(mMarkerList.size() > 0);
	bool allMatch = true;
	PL::CottonwoodMarkerList::const_iterator currentIter = mMarkerList.begin();
	dvacore::UTF16String returnString = currentIter->GetProbability();

	// should hit end at the same time, only check one
	for (; allMatch && (currentIter != mMarkerList.end()); ++currentIter)	
	{
		if (returnString != currentIter->GetProbability())
		{
			returnString = dvacore::ZString("$$$/Prelude/MZ/CottonwoodCompositeMarker/MultipleValuesMessage=<Multiple Values>");
			allMatch = false;
		}
	}

	return returnString;
}

dvacore::UTF8String CottonwoodCompositeMarker::ToPropertyXML() const
{
	dvacore::proplist::PropList propertyList;
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("type"), dvacore::utility::UTF16to8(GetType()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("guid"),dvacore::utility::UTF16to8(GetGUIDList()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("name"), dvacore::utility::UTF16to8(GetName()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("startTime"), dvacore::utility::UTF16to8(dvacore::utility::NumberToString(GetStartTime().GetTicks())));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("duration"), dvacore::utility::UTF16to8(dvacore::utility::NumberToString(GetDuration().GetTicks())));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("comment"), dvacore::utility::UTF16to8(GetComment()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("location"), dvacore::utility::UTF16to8(GetLocation()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("target"), dvacore::utility::UTF16to8(GetTarget()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("speaker"), dvacore::utility::UTF16to8(GetSpeaker()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("probability"), dvacore::utility::UTF16to8(GetProbability()));
	propertyList.SetValue(dvacore::utility::AsciiToUTF8("cuePointType"), dvacore::utility::UTF16to8(GetCuePointType()));

	// Loop through cue point params list and add key/value 
	// pairs to the property sub list
	dvacore::proplist::PropList* paramsSubList = propertyList.NewSubList(dvacore::utility::AsciiToUTF8("cuePointParams"));
	dvatemporalxmp::CustomMarkerParamList paramsList = GetCuePointList();
	dvatemporalxmp::CustomMarkerParamList::size_type i;
	dvacore::UTF16String key;
	dvacore::UTF16String value;

	for (i=0; i < paramsList.size(); i++)
	{
		key = paramsList[i].mKey;
		value = paramsList[i].mValue;

		paramsSubList->SetValue(dvacore::utility::UTF16to8(key), dvacore::utility::UTF16to8(value));
	}

	// Output property list string and send as marker data
	dvacore::UTF8String xmlStr;
	propertyList.OutputAsXMLFragment(xmlStr);
	propertyList.Clear();
	return xmlStr;
}



}