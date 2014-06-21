#include "Prefix.h"

#include "PLFactory.h"
#include "PLUtilitiesPrivate.h"
#include "PLProject.h"
#include "PLLoggingClip.h"
#include "PLRoughCut.h"
#include "PLUtilities.h"
#include "PLBEUtilities.h"

//	MZ
#include "MZProjectActions.h"
#include "MZBEUtilities.h"
#include "MZUtilities.h"
#include "MZProject.h"

//	BE
#include "BE/Project/IProject.h"
#include "BE/Clip/IChildClip.h"
#include "BE/Clip/IMasterClipFwd.h"
#include "BE/Clip/IClip.h"
#include "BE/Sequence/IClipTrack.h"
#include "BE/Sequence/ITrackItem.h"
#include "BE/media/MediaFactory.h"
#include "BEClip.h"
#include "BEMasterClipFactory.h"

//	ASL
#include "ASLClassFactory.h"
#include "ASLPathUtils.h"
#include "ASLAsyncCallFromMainThread.h"
#include "ASLFile.h"

//	DVA
#include "dvacore/filesupport/file/file.h"
#include "dvacore/config/Localizer.h"



namespace PL
{
	namespace
	{
		/*
		**
		*/
		ISRPrimaryClipRef CreateLoggingPrimaryClip(PL::AssetItemPtr const& inAssetItem)
		{
			ISRPrimaryClipRef  loggingClip;

			loggingClip = ISRPrimaryClipRef(SRLoggingClip::Create(inAssetItem));
			DVA_ASSERT(NULL != loggingClip);

			return ISRPrimaryClipRef(loggingClip);
		}

		/*
		**
		*/
		ISRPrimaryClipRef CreateRoughCutPrimaryClip(PL::AssetItemPtr const& inAssetItem)
		{
			ISRPrimaryClipRef roughcut;
			roughcut = ISRPrimaryClipRef(SRRoughCut::Create(inAssetItem));
			DVA_ASSERT(NULL != roughcut);
			return ISRPrimaryClipRef(roughcut);
		}
	}
	namespace SRFactory
	{
		/*
		**
		*/
		BE::IProjectItemRef CreateEmptySequence(
			BE::IProjectRef const& inProject,
			BE::IProjectItemRef const& inContainingBin/* = BE::IProjectItemRef()*/,
			ASL::String const& inSequenceName/* = ASL_STR("Dummy Sequence")*/)
		{
			return PL::BEUtilities::CreateEmptySequence(inProject, inContainingBin, inSequenceName);
		}

		/*
		**
		*/
		ISRPrimaryClipRef FindAndCreatePrimaryClip(PL::AssetItemPtr const& inAssetItem)
		{
			ISRPrimaryClipRef primaryClip;

			primaryClip = SRProject::GetInstance()->GetPrimaryClip(inAssetItem->GetMediaPath());
			if (NULL == primaryClip)
			{
				primaryClip = CreatePrimaryClip(inAssetItem);
			}

			return primaryClip;
		}

		/*
		**
		*/
		ISRPrimaryClipRef CreatePrimaryClip(PL::AssetItemPtr const& inAssetItem)
		{
			ISRPrimaryClipRef primaryClip;
			PL::AssetMediaType assetMediaType = inAssetItem->GetAssetMediaType();
			switch(assetMediaType)
			{
			case kAssetLibraryType_SubClip:
			case kAssetLibraryType_MasterClip:
			case kAssetLibraryType_RCSubClip:
				primaryClip = CreateLoggingPrimaryClip(inAssetItem);
				if (!UIF::IsEAMode())
				{
					if (primaryClip->IsOffline())
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/CreateOfflinePrimarClipWarning=The open clip is OFFLINE. Prelude cannot apply metadata to an OFFLINE clip."));
					}
					else if (!PL::Utilities::PathSupportsXMP(inAssetItem->GetMediaPath()))
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/CreateUnsupportXMPPrimarClipWarning=The open clip doesn't support XMP. Prelude cannot apply metadata to it."));
					}
					else if (!primaryClip->IsWritable())
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/CreateReadOnlyPrimarClipWarning=The open clip is READ ONLY. Prelude cannot apply metadata to a READ ONLY clip."));
					}
				}
				break;
			case kAssetLibraryType_RoughCut:
				primaryClip = CreateRoughCutPrimaryClip(inAssetItem);
				if (!UIF::IsEAMode())
				{
					if (primaryClip->IsOffline())
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/CreateOfflineRCWarning=The open rough cut is OFFLINE. Prelude cannot edit an OFFLINE rough cut."));
					}
					else if (!primaryClip->IsWritable())
					{
						ML::SDKErrors::SetSDKErrorString(dvacore::ZString("$$$/Prelude/PLCore/CreateReadOnlyRCWarning=The open rough cut is READ ONLY. Prelude cannot edit a READ ONLY rough cut."));
					}
				}
				break;
			default:
				DVA_ASSERT(0);
			}

			return primaryClip;
		}

		/*
		**
		*/
		SRUnassociatedMetadataRef CreateFileBasedSRUnassociatedMetadata(
			const ASL::String& inMetadataID,
			const ASL::String& inMetadataPath)
		{
			SRUnassociatedMetadataRef srUnassociatedMetadata;
			// We should always trust ID rather than path.
			if (!ASL::PathUtils::ExistsOnDisk(inMetadataID))
			{
				return srUnassociatedMetadata;
			}

			srUnassociatedMetadata = SRUnassociatedMetadata::Create(inMetadataID, inMetadataPath);

			return srUnassociatedMetadata;
		}

		/*
		**
		*/
		SRUnassociatedMetadataRef CreateSRUnassociatedMetadata(
			const ASL::String& inMetadataID,
			const ASL::String& inMetadataPath, 
			const XMPText& inXMPText)
		{
			return SRUnassociatedMetadata::Create(inMetadataID, inMetadataPath, inXMPText);
		}
	}
}
