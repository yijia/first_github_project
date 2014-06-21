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

#ifndef PLEXPORTCLIPITEM_H
#define PLEXPORTCLIPITEM_H


#ifndef PLASSETITEM_H
#include "PLAssetItem.h"
#endif

#ifndef BE_CLIP_IMASTERCLIP_H
#include "BE/Clip/IMasterClip.h"
#endif

#ifndef ASLTYPES_H
#include "ASLTypes.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

namespace PL
{
	class ExportClipItem;
	typedef boost::shared_ptr<ExportClipItem>	ExportClipItemPtr;
	typedef std::list<ExportClipItemPtr>		ExportClipItems;

	class ExportClipItem
	{
	public:
		ExportClipItem(AssetItemPtr const& inAssetItem);
		PL_EXPORT
		ExportClipItem(AssetItemPtr const& inAssetItem, BE::IMasterClipRef const& inMasterClip);

		BE::IMasterClipRef GetMasterClip() const;
		dvamediatypes::TickTime GetStartTime() const;
		dvamediatypes::TickTime GetDuration() const;
		ASL::String GetAliasName() const;
		ASL::String GetExportName();

	private:
		AssetItemPtr		mAssetItem;
		BE::IMasterClipRef	mMasterClip;
	};
}

#endif	//PLEXPORTCLIPITEM_H