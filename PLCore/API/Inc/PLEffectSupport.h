/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2014 Adobe Systems Incorporated                       */
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

#ifndef PLEFFECTSUPPORT_H
#define PLEFFECTSUPPORT_H

//	ASL
#ifndef ASLOBJECTPTR_H
#include "ASLObjectPtr.h"
#endif

#ifndef ASLCLASS_H
#include "ASLClass.h"
#endif

#ifndef PLEXPORT_H
#include "PLExport.h"
#endif

#ifndef ITRANSITIONMODULE_H
#include "Transition/ITransitionModule.h"
#endif

#ifndef IAUDIOTRANSITIONMODULE_H
#include "Transition/IAudioTransitionModule.h"
#endif

#ifndef IVIDEOTRANSITIONMODULE_H
#include "Transition/IVideoTransitionModule.h"
#endif

#ifndef DVAMEDIATYPES_TICKTIME_H
#include "dvamediatypes/TickTime.h"
#endif

#if ASL_PRAGMA_ONCE
#pragma once
#endif

//------------------------------------------------------------------------

namespace PL
{
	typedef std::vector<ML::ITransitionModuleRef> TransitionItemList;
	class EffectSupport;
	typedef boost::shared_ptr<EffectSupport>		EffectSupportPtr;

	class EffectSupport
	{
	public:
		PL_EXPORT
			static EffectSupportPtr GetInstance();

		~EffectSupport();

		PL_EXPORT
			static void Initialize();

		PL_EXPORT
			static void Terminate();

		PL_EXPORT
			TransitionItemList GetAudioTransitionList();
		PL_EXPORT
			TransitionItemList GetVideoTransitionList();

		PL_EXPORT
			bool IsApplyAudioTransition();
		PL_EXPORT
			bool IsApplyVideoTransition();

		PL_EXPORT
			void SetApplyAudioTransition(bool inApply);
		PL_EXPORT
			void SetApplyVideoTransition(bool inApply);

		PL_EXPORT
			ML::ITransitionModuleRef GetSelectedAudioTransition();
		PL_EXPORT
			ML::ITransitionModuleRef GetSelectedVideoTransition();

		PL_EXPORT
			void SetSelectedAudioTransition(ASL::String const& inTransitionName);
		PL_EXPORT
			void SetSelectedVideoTransition(ASL::String const& inTransitionName);

		PL_EXPORT
			dvamediatypes::TickTime GetDefaultAudioTransitionDuration();
		PL_EXPORT
			ASL::UInt32 GetDefaultVideoTransitionDuration();//in frames

		PL_EXPORT
			void SetDefaultAudioTransitionDuration(dvamediatypes::TickTime inDuration);
		PL_EXPORT
			void SetDefaultVideoTransitionDuration(ASL::UInt32 inDuration);//in frames

	private:
		static EffectSupportPtr sEffectSupport;
		/**
		**	Constructor.
		*/
		EffectSupport();

		/**
		**	Load the factory audio transitions.
		*/
		void LoadFactoryAudioTransitions();

		/**
		**	Load the factory video transitions.
		*/
		void LoadFactoryVideoTransitions();

		TransitionItemList			mAudioTransitionList;
		TransitionItemList			mVideoTransitionList;
	}; // class EffectSupport

} // namespace PLCore

#endif // PLEFFECTSUPPORT_H
