####################################################
# Creates the PLCore project
####################################################

MP.include('../../Make/PremiereBase.mp')

MP.project_guid = '{E8265A85-5869-4CB2-AB38-AD882178681C}'

##############################
# Includes
##############################
MP.include_paths += [
	'ROOT/Prelude/Libraries/UIFramework/API/Inc',
	MCROOT + '/PhotoshopServer/API/Inc',
	]
	
##############################
# Excludes
##############################
MP.exclude_files += [
	]

##############################
# PreProcessor
##############################
if MP.win:
	MP.preprocessor_definitions += [
		'PL_EXPORT=__declspec(dllexport)',
		'CURL_STATICLIB'
		]

##############################
# Resourse
##############################

##############################
# Libraries
##############################
if MP.win:
	MP.libraries += [
		'SDK/ws2_32.lib',
		'SDK/Wldap32.lib',
		'SDK/mpr.lib',
		'SDK/Wininet.lib',
		]

##############################
# Direct Dependencies
##############################

call_once(include_XMLUtils)
call_once(include_boost_regex)
include_boost_filesystem()
include_zstring()
include_ASLFoundation()
include_UIFramework()
include_dvametadata()
include_dvametadataui()
include_dvaworkspace()
include_Mezzanine()
include_XMP()

include_EAClient()
include_EAMedia()

if using_ame:
	include_EncoderHost()
	include_PostEncodeHost()