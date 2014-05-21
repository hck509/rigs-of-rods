/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ContentManager.h"

#include "Settings.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "SoundScriptManager.h"
#include "SkinManager.h"
#include "Language.h"
#include "PlatformUtils.h"

#include "CacheSystem.h"

#include "OgreShaderParticleRenderer.h"
#include "OgreBoxEmitterFactory.h"

#ifdef USE_ANGELSCRIPT
#include "FireExtinguisherAffectorFactory.h"
#include "ExtinguishableFireAffectorFactory.h"
#endif // USE_ANGELSCRIPT

#include "Utils.h"

using namespace Ogre;
using namespace std;
using namespace RoR;

// ================================================================================
// Static variables
// ================================================================================

const ContentManager::ResourcePack ContentManager::ResourcePack::OGRE_CORE             (BITMASK_64(1),              "OgreCore",   "Bootstrap");
const ContentManager::ResourcePack ContentManager::ResourcePack::GUI_MENU_WALLPAPERS   (BITMASK_64(2),   "gui_menu_wallpapers",  "Wallpapers");
const ContentManager::ResourcePack ContentManager::ResourcePack::GUI_STARTUP_SCREEN    (BITMASK_64(3),    "gui_startup_screen",   "Bootstrap");

// ================================================================================
// Functions
// ================================================================================

ContentManager::ContentManager():
	m_loaded_resource_packs(0)
{
}

ContentManager::~ContentManager()
{
}

void ContentManager::AddResourcePack(ResourcePack const & resource_pack)
{
	std::stringstream log_msg;
	log_msg << "[RoR|ContentManager] Loading resource pack '" << resource_pack.name << "' from group '" << resource_pack.resource_group_name << "'";
	Ogre::String resources_dir = SSETTING("Resources Path", "resources" + PlatformUtils::DIRECTORY_SEPARATOR);
	Ogre::String zip_path = resources_dir + resource_pack.name + Ogre::String(".zip");
	if (PlatformUtils::FileExists(zip_path))
	{
		log_msg << " (ZIP archive)";
		LOG(log_msg.str());
		ResourceGroupManager::getSingleton().addResourceLocation(zip_path, "Zip", resource_pack.resource_group_name);
		BITMASK_64_SET_1(m_loaded_resource_packs, resource_pack.mask);
	}
	else
	{
		Ogre::String dir_path = resources_dir + resource_pack.name;
		if (PlatformUtils::FolderExists(dir_path))
		{
			log_msg << " (directory)";
			LOG(log_msg.str());
			ResourceGroupManager::getSingleton().addResourceLocation(dir_path, "FileSystem", resource_pack.resource_group_name);
			BITMASK_64_SET_1(m_loaded_resource_packs, resource_pack.mask);
		}
		else
		{
			log_msg << " failed, data not found.";
			throw std::runtime_error(log_msg.str());
		}
	}
}


void ContentManager::loadMainResource(String name, String group)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif

	String zipFilename = SSETTING("Resources Path", "resources\\")+name+".zip";
	if (PlatformUtils::FileExists(zipFilename.c_str()))
	{
		ResourceGroupManager::getSingleton().addResourceLocation(zipFilename, "Zip", group);
	} else
	{
		String dirname = SSETTING("Resources Path", "resources\\")+name;
		LOG("resource zip '"+zipFilename+"' not existing, using directory instead: " + dirname);
		ResourceGroupManager::getSingleton().addResourceLocation(dirname, "FileSystem", group);
	}
}
/*
void ContentManager::initBootstrap(void)
{
	LOG("Loading Bootstrap");
	loadMainResource("OgreCore", "Bootstrap");
	loadMainResource("gui_startup_screen", "Bootstrap");
	LOG("Loading Wallpapers");
	loadMainResource("gui_menu_wallpapers", "Wallpapers");
}*/

bool ContentManager::init(void)
{
    // set listener if none has already been set
    if (!Ogre::ResourceGroupManager::getSingleton().getLoadingListener())
        Ogre::ResourceGroupManager::getSingleton().setLoadingListener(this);

	// try to get correct paths
	// note: we don't have LogManager available yet!
	// FIRST: Get the "program path" and the user space path

	// note: this is now done in the settings class, so set it up
	// note: you need to set the build mode correctly before you build the paths!

	// by default, display everything in the depth map
	Ogre::MovableObject::setDefaultVisibilityFlags(DEPTHMAP_ENABLED);

	CACHE.setLocation(SSETTING("Cache Path", ""), SSETTING("Config Root", ""));

	ColoredTextAreaOverlayElementFactory *cef = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(cef);



	// load bootstrap and main resources
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	// bootstrap
	// we load the bootstrap before already

	// main game resources
	LOG("Loading main resources");
	loadMainResource("airfoils");
	loadMainResource("beamobjects");
	loadMainResource("materials");
	loadMainResource("meshes");
	loadMainResource("overlays");
	loadMainResource("particles");
#ifdef USE_MYGUI
	loadMainResource("mygui");
	loadMainResource("dashboards");
#endif // USE_MYGUI
	loadMainResource("scripts");
	loadMainResource("textures");
	loadMainResource("flags");
	loadMainResource("icons");
	loadMainResource("famicons");


#ifdef WIN32
	// TODO: FIX UNDER LINUX!
	// register particle classes
	LOG("Registering Particle Box Emitter");
	ParticleSystemRendererFactory *mParticleSystemRendererFact = OGRE_NEW ShaderParticleRendererFactory();
	ParticleEmitterFactory *mParticleEmitterFact = OGRE_NEW BoxEmitterFactory();
	ParticleSystemManager::getSingleton().addRendererFactory(mParticleSystemRendererFact);
	ParticleSystemManager::getSingleton().addEmitterFactory(mParticleEmitterFact);
#endif // WIN32

#ifdef USE_ANGELSCRIPT
	// FireExtinguisherAffector
	ParticleAffectorFactory* pAffFact = OGRE_NEW FireExtinguisherAffectorFactory();
	ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);

	// ExtinguishableFireAffector
	pAffFact = OGRE_NEW ExtinguishableFireAffectorFactory();
	ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);
#endif // USE_ANGELSCRIPT

	// optional ones

	// sound is a bit special as we mark the base sounds so we don't clear them accidentally later on
#ifdef USE_OPENAL
	LOG("Creating Sound Manager");
	SoundScriptManager::getSingleton().setLoadingBaseSounds(true);
#endif // USE_OPENAL

	if (SSETTING("AudioDevice", "") != "No Output")
		loadMainResource("sounds");

	if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)")
		loadMainResource("caelum");

	loadMainResource("hydrax");

	if (SSETTING("Vegetation", "None (fastest)") != "None (fastest)")
		loadMainResource("paged");

	if (BSETTING("HDR", false))
		loadMainResource("hdr");

	if (BSETTING("DOF", false))
		loadMainResource("dof");

	if (BSETTING("Glow", false))
		loadMainResource("glow");

	if (BSETTING("Motion blur", false))
		loadMainResource("blur");

	if (BSETTING("HeatHaze", false))
		loadMainResource("heathaze");

	if (BSETTING("Sunburn", false))
		loadMainResource("sunburn");

	if (SSETTING("Shadow technique", "") == "Parallel-split Shadow Maps")
		loadMainResource("pssm");

	// streams path, to be processed later by the cache system
	LOG("Loading filesystems");

	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"cache", "FileSystem", "cache");
	// config, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"config", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"alwaysload", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	// packs, to be processed later by the cache system

	// add scripts folder
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"scripts", "FileSystem", "Scripts");

	// init skin manager, important to happen before trucks resource loading!
	LOG("registering Skin Manager");
	new SkinManager();

	LOG("registering colored text overlay factory");
	ColoredTextAreaOverlayElementFactory *pCT = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(pCT);

	// set default mipmap level (NB some APIs ignore this)
	if (TextureManager::getSingletonPtr())
		TextureManager::getSingleton().setDefaultNumMipmaps(5);
	String tft=SSETTING("Texture Filtering", "Trilinear");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;
	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

	// load all resources now, so the zip files are also initiated
	LOG("initialiseAllResourceGroups()");
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseAllResourceGroups();
		else
			ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Resource groups: " + e.getFullDescription());
	}
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().setLoadingBaseSounds(false);
#endif // USE_OPENAL

	// and the content
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("Program Path", "")+"packs", "FileSystem", "Packs", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"packs", "FileSystem", "Packs", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"mods",  "FileSystem", "Packs", true);

	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"vehicles", "FileSystem", "VehicleFolders");
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"terrains", "FileSystem", "TerrainFolders");

	exploreFolders("VehicleFolders");
	exploreFolders("TerrainFolders");
	exploreZipFolders("Packs"); // this is required for skins to work

	LOG("initialiseAllResourceGroups() - Content");
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("Packs");
		else
			ResourceGroupManager::getSingleton().initialiseResourceGroup("Packs");
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Content Resource groups: " + e.getFullDescription());
	}

	LanguageEngine::getSingleton().postSetup();

	return true;
}

Ogre::DataStreamPtr ContentManager::resourceLoading(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource)
{
	return Ogre::DataStreamPtr();
}

void ContentManager::resourceStreamOpened(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource, Ogre::DataStreamPtr& dataStream)
{
}

bool ContentManager::resourceCollision(Ogre::Resource *resource, Ogre::ResourceManager *resourceManager)
{
	/*
	// TODO: do something useful here
	if (resourceManager->getResourceType() == "Material")
	{
		if (instanceCountMap.find(resource->getName()) == instanceCountMap.end())
		{
			instanceCountMap[resource->getName()] = 1;
		}
		int count = instanceCountMap[resource->getName()]++;
		MaterialPtr mat = (MaterialPtr)resourceManager->create(resource->getName() + TOSTRING(count), resource->getGroup());
		resource = (Ogre::Resource *)mat.getPointer();
		return true;
	}
	*/
	return false;
}

	

void ContentManager::exploreZipFolders(Ogre::String rg)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr files= rgm.findResourceFileInfo(rg, "*.skinzip"); //search for skins
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (!iterFiles->archive) continue;
		String fullpath = iterFiles->archive->getName() + dirsep;
		rgm.addResourceLocation(fullpath + iterFiles->filename, "Zip", rg);
	}
	// DO NOT initialize ...
}

void ContentManager::exploreFolders(Ogre::String rg)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr files= rgm.findResourceFileInfo(rg, "*", true); // searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (!iterFiles->archive) continue;
		if (iterFiles->filename==String(".svn")) continue;
		// trying to get the full path
		String fullpath = iterFiles->archive->getName() + dirsep;
		rgm.addResourceLocation(fullpath+iterFiles->filename, "FileSystem", rg);
	}
	LOG("initialiseResourceGroups: "+rg);
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseResourceGroup(rg);
		else
			ResourceGroupManager::getSingleton().initialiseResourceGroup(rg);
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Resource group '" + rg + "' : " + e.getFullDescription());
	}
}
