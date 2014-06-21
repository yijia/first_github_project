/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2014 Adobe Systems Incorporated                    */
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

//	Self
#include "PLEffectSupport.h"

//	ASL
#include "ASLStringUtils.h"
#include "ASLClassFactory.h"
#include "ASLPathUtils.h"
#include "ASLResourceUtils.h"
#include "ASLDirectories.h"
#include "ASLDirectoryRegistry.h"
#include "ASLTimer.h"

// BE
#include "BEBackend.h"
#include "BEPreferences.h"
#include "BEProperties.h"

//  ML
#include "Transition/IAudioTransitionFactory.h"
#include "Transition/IVideoTransitionFactory.h"

namespace PL
{
	namespace
	{
		typedef std::vector<ASL::String> TransitionNameList;

		TransitionNameList GetAudioTransitionNameList()
		{
			TransitionNameList result;
			result.push_back(DVA_STR("Constant Power"));
			result.push_back(DVA_STR("Constant Gain"));
			return result;
		}

		TransitionNameList GetVideoTransitionNameList()
		{
			TransitionNameList result;
			result.push_back(DVA_STR("ADBE Dip To White"));
			result.push_back(DVA_STR("ADBE Dip To Black"));
			result.push_back(DVA_STR("AE.ADBE Cross Dissolve New"));
			return result;
		}
		const BE::PropertyKey kPropertyKey_ApplyVideoTransition(BE_PROPERTYKEY("MZ.ApplyVideoTransition"));
		const BE::PropertyKey kPropertyKey_ApplyAudioTransition(BE_PROPERTYKEY("MZ.ApplyAudioTransition"));
	}


	EffectSupportPtr EffectSupport::sEffectSupport = EffectSupportPtr();
	EffectSupportPtr EffectSupport::GetInstance()
	{
		return sEffectSupport;
	}

	void EffectSupport::Initialize()
	{
		if (sEffectSupport == NULL)
		{
			sEffectSupport = EffectSupportPtr(new EffectSupport());
		}
		sEffectSupport->LoadFactoryAudioTransitions();
		sEffectSupport->LoadFactoryVideoTransitions();
	}

	void EffectSupport::Terminate()
	{
		if (sEffectSupport)
		{
			sEffectSupport.reset();
		}
	}

	/*
	**
	*/
	EffectSupport::EffectSupport()
	{
	}

	/*
	**
	*/
	EffectSupport::~EffectSupport()
	{
	}

	bool EffectSupport::IsApplyAudioTransition()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPropertiesRef bProp(backend);
		bool tempBool = true;
		bProp->GetValue(kPropertyKey_ApplyAudioTransition, tempBool);
		return tempBool;
	}

	bool EffectSupport::IsApplyVideoTransition()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPropertiesRef bProp(backend);
		bool tempBool = true;
		bProp->GetValue(kPropertyKey_ApplyVideoTransition, tempBool);
		return tempBool;
	}

	void EffectSupport::SetApplyAudioTransition(bool inApply)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPropertiesRef bProp(backend);
		bProp->SetValue(kPropertyKey_ApplyAudioTransition, inApply, BE::kPersistent);
	}

	void EffectSupport::SetApplyVideoTransition(bool inApply)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPropertiesRef bProp(backend);
		bProp->SetValue(kPropertyKey_ApplyVideoTransition, inApply, BE::kPersistent);
	}

	TransitionItemList EffectSupport::GetAudioTransitionList()
	{
		return mAudioTransitionList;
	}

	TransitionItemList EffectSupport::GetVideoTransitionList()
	{
		return mVideoTransitionList;
	}

	ML::ITransitionModuleRef EffectSupport::GetSelectedAudioTransition()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		ASL::String name = preferences->GetDefaultTransition(false);

		for (int i=0; i<mAudioTransitionList.size(); ++i)
		{
			ML::IAudioTransitionModuleRef audioTransition(mAudioTransitionList[i]);
			if (audioTransition && audioTransition->GetMatchName().ToString() == name)
			{
				return mAudioTransitionList[i];
			}
		}
		return mAudioTransitionList[0];
	}

	ML::ITransitionModuleRef EffectSupport::GetSelectedVideoTransition()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		ASL::String name = preferences->GetDefaultTransition(true);

		for (int i=0; i<mVideoTransitionList.size(); ++i)
		{
			ML::IVideoTransitionModuleRef videoTransition(mVideoTransitionList[i]);
			if (videoTransition && videoTransition->GetMatchName().ToString() == name)
			{
				return mVideoTransitionList[i];
			}
		}
		return mVideoTransitionList[0];
	}

	void EffectSupport::SetSelectedAudioTransition(ASL::String const& inTransitionName)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		preferences->SetDefaultTransition(false, inTransitionName);
	}

	void EffectSupport::SetSelectedVideoTransition(ASL::String const& inTransitionName)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		preferences->SetDefaultTransition(true, inTransitionName);
	}

	dvamediatypes::TickTime EffectSupport::GetDefaultAudioTransitionDuration()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		dvamediatypes::TickTime duration = preferences->GetDefaultAudioTransitionDuration();

		return duration;
	}

	ASL::UInt32 EffectSupport::GetDefaultVideoTransitionDuration()
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		return preferences->GetDefaultVideoTransitionDuration();
	}

	void EffectSupport::SetDefaultAudioTransitionDuration(dvamediatypes::TickTime inDuration)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		preferences->SetDefaultAudioTransitionDuration(inDuration);
	}

	void EffectSupport::SetDefaultVideoTransitionDuration(ASL::UInt32 inDuration)
	{
		BE::IBackendRef backend = BE::GetBackend();
		BE::IPreferencesRef preferences = backend->GetPreferences();
		preferences->SetDefaultVideoTransitionDuration(inDuration);
	}

	/*
	**
	*/
	void EffectSupport::LoadFactoryAudioTransitions()
	{
		//	Create an audio transition factory.
		ML::IAudioTransitionFactoryRef audioTransitionFactory(
			ASL::CreateClassInstanceRef(kAudioTransitionFactoryClassID));

		//	Fetch the video transition modules from the factory.
		ML::AudioTransitionModuleVector audioTransitions;
		audioTransitionFactory->GetAudioTransitions(audioTransitions);

		TransitionNameList nameList = GetAudioTransitionNameList();
		for (ML::AudioTransitionModuleVector::iterator iter = audioTransitions.begin();
			iter != audioTransitions.end(); ++iter)
		{
			ASL::String name = (*iter)->GetMatchName().ToString();
			if (std::find(nameList.begin(), nameList.end(), name) != nameList.end())
			{
				ML::ITransitionModuleRef transitionModule(*iter);
				mAudioTransitionList.push_back(transitionModule);
			}
		}
	}

	/*
	**
	*/
	void EffectSupport::LoadFactoryVideoTransitions()
	{
		//	Create a video transition factory.
		ML::IVideoTransitionFactoryRef videoTransitionFactory(
			ASL::CreateClassInstanceRef(kVideoTransitionFactoryClassID));

		//	Fetch the video transition modules from the factory.
		ML::VideoTransitionModuleVector videoTransitions;
		videoTransitionFactory->GetVideoTransitions(videoTransitions);

		TransitionNameList nameList = GetVideoTransitionNameList();
		for (ML::VideoTransitionModuleVector::iterator iter = videoTransitions.begin();
			iter != videoTransitions.end(); ++iter)
		{
			ML::ITransitionModuleRef transitionModule(*iter);
			if (transitionModule && !transitionModule->IsDeprecated())
			{
				ASL::String name = (*iter)->GetMatchName().ToString();
				if (std::find(nameList.begin(), nameList.end(), name) != nameList.end())
				{
					ML::ITransitionModuleRef transitionModule(*iter);
					mVideoTransitionList.push_back(transitionModule);
				}
			}
		}
	}

} // namespace PLCore
