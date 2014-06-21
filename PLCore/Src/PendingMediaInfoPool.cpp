/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2012 Adobe Systems Incorporated		               */
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

#include "PLPendingMediaInfoPool.h"

//	ASL
#include "ASLListener.h"
#include "ASLStationRegistry.h"
#include "ASLStationUtils.h"
#include "ASLMessageMap.h"

// DVA
#ifndef DVACORE_UTILITY_SHAREDREFPTR_H
#include "dvacore/utility/SharedRefPtr.h"
#endif

#ifndef BOOST_FOREACH
#include "boost/foreach.hpp"
#endif


namespace PL 
{
	namespace
	{
	

		class MediaPendingListener 
			:
			public dvacore::utility::RefCountedObject,
			public ASL::Listener
		{
		public:
			DVA_UTIL_TYPEDEF_SHARED_REF_PTR(MediaPendingListener) SharedPtr;

			typedef std::map<BE::IMediaInfoRef, MediaPendingListener::SharedPtr> MediaPendingMap;
			typedef std::vector<MediaOnlineNotification> MediaOnlineNotifications;

			ASL_MESSAGE_MAP_BEGIN(MediaPendingListener)
				ASL_MESSAGE_HANDLER(BE::MediaInfoChangedMessage, OnMediaInfoChanged)
			ASL_MESSAGE_MAP_END

			MediaPendingListener(
				BE::IMediaInfoRef const& inMediaInfo, 
				MediaOnlineNotification const& inOnlineNotification);

			void AddNotification(MediaOnlineNotification const& inOnlineNotification);

			static MediaPendingMap sMediaPendingMap;

		private:

			/*
			**
			*/
			void OnMediaInfoChanged();

			BE::IMediaInfoRef							mMediaInfo;
			MediaOnlineNotifications					mOnlineNotifications;
		};

		MediaPendingListener::MediaPendingMap MediaPendingListener::sMediaPendingMap;

		/*
		**
		*/
		MediaPendingListener::MediaPendingListener(
			BE::IMediaInfoRef const& inMediaInfo, 
			MediaOnlineNotification const& inOnlineNotification)
			:
		mMediaInfo(inMediaInfo)
		{
			ASL::StationUtils::AddListener(mMediaInfo->GetStationID(), this);
			mOnlineNotifications.push_back(inOnlineNotification);
		}

		/*
		**
		*/
		void MediaPendingListener::OnMediaInfoChanged()
		{
			if (!mMediaInfo->IsPending())
			{
				BOOST_FOREACH(MediaOnlineNotification const& eachNotification, mOnlineNotifications)
				{
					if (eachNotification)
					{
						eachNotification();
					}
				}
				MediaPendingListener::MediaPendingMap::iterator iter = MediaPendingListener::sMediaPendingMap.find(mMediaInfo);

				if (iter!= sMediaPendingMap.end())
				{
					sMediaPendingMap.erase(iter);
				}

				//	This will be released above, so shouldn't access any class member below.
			}
		}

		void MediaPendingListener::AddNotification(MediaOnlineNotification const& inOnlineNotification)
		{
			mOnlineNotifications.push_back(inOnlineNotification);
		}
	}

	/*
	**
	*/
	bool  RegisterPendingMediaInfo(
			BE::IMediaInfoRef const& inMediaInfo, 
			MediaOnlineNotification const& inOnlineNotification)
	{
		bool result(false);
		if (inMediaInfo && inMediaInfo->IsPending())
		{
			result = true;
			MediaPendingListener::MediaPendingMap::iterator iter = 
				MediaPendingListener::sMediaPendingMap.find(inMediaInfo);
			if (iter == MediaPendingListener::sMediaPendingMap.end())
			{
				MediaPendingListener::SharedPtr ptr(new MediaPendingListener(inMediaInfo, inOnlineNotification));
				MediaPendingListener::sMediaPendingMap.insert(std::make_pair(inMediaInfo, ptr));
			}
			else
			{
				((*iter).second)->AddNotification(inOnlineNotification);
			}
		}

		return result;
	}

}

