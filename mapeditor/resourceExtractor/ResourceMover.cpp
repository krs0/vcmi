/*
 * ResourceMover.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ResourceMover.h"

#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"

#include "boost/filesystem/path.hpp"
#include "boost/locale.hpp"

namespace bfs = boost::filesystem;

bool move_non_json_files = false; // move files that are not yet supported by mods.
bool delete_source_files = false; // delete source files or leave a copy in place.
bool move_artifacts = false; // no jsons yet
bool move_creature_banks = false; // no jsons yet
bool move_spells = false; // no jsons yet

// extracts filename with extrension: returns <filename.ext>
std::string extractFileName(std::string source)
{
	int index = source.find_last_of("/");
	return source.substr(index+1);
}

// removes extension from a filename: returns <filename>
std::string removeExtension(std::string filename)
{
	int index = filename.find_last_of(".");
	return filename.substr(0, index);
}

// move file using complete file paths for source and destination files
void moveFile(bfs::path sourceFilePath, bfs::path destinationFilePath)
{
	boost::system::error_code returnedError;

	if (bfs::exists(sourceFilePath))
	{
		bfs::create_directories(destinationFilePath.parent_path(), returnedError);

		if (!returnedError)
			if (!delete_source_files)
				bfs::copy_file(sourceFilePath, destinationFilePath, bfs::copy_option::overwrite_if_exists);
			else
				bfs::rename(sourceFilePath, destinationFilePath);
		else
			logGlobal->error("Destination folder hierarchy could not be created! " + destinationFilePath.parent_path().string());
	}
}

// move file using file name and source and destination folders
void moveFile(std::string filename, bfs::path sourceFolder, bfs::path destinationFolder)
{
	boost::system::error_code returnedError;

	bfs::path sourceFilePath = sourceFolder / filename;
	if(bfs::exists(sourceFilePath))
	{
		bfs::create_directories(destinationFolder, returnedError);

		if (!returnedError)
			if(!delete_source_files)
				bfs::copy_file(sourceFilePath, destinationFolder / filename, bfs::copy_option::overwrite_if_exists);
			else
				bfs::rename(sourceFilePath, destinationFolder / filename);
		else
			logGlobal->error("Destination folder hierarchy could not be created! " + destinationFolder.string());
	}
}

void moveFileFromConfig(const JsonNode node, std::string nodeStructure, bfs::path sourceRoot, bfs::path destinationRoot)
{
	// add leading / if missing
	if (nodeStructure.substr(0, 1) != "/")
		nodeStructure = "/" + nodeStructure;

	auto resolvedNode = node.resolvePointer(nodeStructure);
	if (!resolvedNode.isNull())
	{
		std::string partialFilePath = node.resolvePointer(nodeStructure).String();

		// move actual file
		std::string fileToMove = extractFileName(partialFilePath);
		moveFile(sourceRoot / fileToMove, destinationRoot / partialFilePath);

		// move file's .msk
		std::string partialMaskPath = removeExtension(partialFilePath) + ".msk";
		std::string maskToMove = extractFileName(partialMaskPath);
		moveFile(sourceRoot / maskToMove, destinationRoot / partialMaskPath);
	}
}

ResourceMover::ResourceMover()
{
	modPath = VCMIDirs::get().userDataPath() / "Mods";
	dataPath = VCMIDirs::get().userDataPath() / "extracted";
	spritesPath = dataPath / "Sprites";
	imagesPath = dataPath / "Images";
	soundPath = dataPath / "Sound";
	videoPath = dataPath / "Video";
	mp3Path = VCMIDirs::get().userDataPath() / "Mp3";
	modContentPath = ""; 
}

void ResourceMover::parseOriginalDataFilesAndMoveToMods(bool move_extracted_files)
{
	if (!move_extracted_files)
		return;

	boost::locale::generator gen;				// Create locale generator 
	std::locale::global(gen(""));				// "" - the system default locale, set it globally

	logGlobal->info("Relocating Resources to SoD mod...");

	moveArtifacts(true);
	moveCreatureBanks(true);
	moveSpells(true);
	moveFactions(true);

	moveNonJsonFiles(true);

	logGlobal->info("Relocating resources complete!");

	//ToDo: Delete Data Temp when all files are placed in the right places

}

void ResourceMover::moveArtifacts(bool move_artifacts)
{
	if (move_artifacts)
	{
		logGlobal->info("\tRelocatingartifacts resources...");

		// Move artifact adventuere map icons
		bfs::path modSpritesPath = modPath.append("SoD/mods/artifacts/content/sprites/SoD/");
		for (int i = 1; i <= 144; i++)
		{
			char* buffer = new char[256];
			sprintf(buffer, "%04d", i);	// indexes are in 0xxx format.

			std::string filename = std::string("AVA") + buffer;
			moveFile(filename + ".def", spritesPath, modSpritesPath);
			moveFile(filename + ".msk", spritesPath, modSpritesPath);

			delete[] buffer;
		}

		// Move artifact icons, all in 1 file identified by index.
		moveFile("artifact.def", spritesPath, modSpritesPath);
		moveFile("artifBon.def", spritesPath, modSpritesPath);
	}
}

void ResourceMover::moveCreatureBanks(bool move_creature_banks)
{
	if (move_creature_banks)
	{
		logGlobal->info("\tRelocating creature banks resources...");

		bfs::path modSpritesPath = modPath.append("SoD/mods/creatureBanks/content/sprites/SoD/");
		const JsonNode configCreatureBanks(ResourceID("Mods/SoD/mods/creatureBanks/content/config/SoD/creatureBanks.json"));
		for (const JsonNode& oneBank : configCreatureBanks["banks"].Vector())
		{
			for (const JsonNode& graphics : oneBank["graphics"].Vector())
			{
				std::string sTemp = graphics["adventureMap"].String();
				moveFile(sTemp, spritesPath, modSpritesPath);
			}
		}
	}
}

void ResourceMover::moveSpells(bool move_spells)
{
	if (move_spells)
	{
		logGlobal->info("\tRelocating spells resources...");

		// Move spell sound files
		bfs::path modSpritesPath = modPath / "SoD/mods/spells/content/";
		const JsonNode configSpellInfo(ResourceID("Mods/SoD/mods/spells/content/config/SoD/spellInfo.json"));
		for (auto& spell : configSpellInfo["spells"].Struct())
		{
			std::string spellSoundPath = spell.second["soundfile"].String();
			if (spellSoundPath != "")
			{
				std::string spellSoundfile = extractFileName(spellSoundPath);
				moveFile(soundPath / spellSoundfile, modSpritesPath / spellSoundPath);
			}
		}

		// Move spell graphics files
		moveFile("spells.def", spritesPath, modPath / "SoD/mods/spells/content/sprites/SoD");
		moveFile("spellScr.def", spritesPath, modPath / "SoD/mods/spells/content/sprites/SoD");
	}
}

void ResourceMover::moveFactions(bool move_factions)
{
	std::vector<std::string> factions = { "castle", "conflux", "dungeon", "fortress", "inferno", "necropolis", "neutral", "rampart", "stronghold", "tower" };

	for(std::string faction : factions)
	{
		logGlobal->info("\tRelocating Factions/%s resources...", faction);

		const JsonNode factionConfigs(ResourceID("Mods/SoD/mods/" + faction + "/mod.json"));

		moveCreaturesFiles(faction, factionConfigs);

		moveHeroClasses(faction, factionConfigs);
		moveIndividualHeroes(faction, factionConfigs);

		logGlobal->info("\t\tRelocating Faction resources...");

		moveDwellings(faction, factionConfigs);
		moveCreatureBackgrounds(faction, factionConfigs);

		// exit for if current faction is neutral. Neutral faction does not have the rest of files.
		if(faction == "neutral")
			break;

		movePuzzle(faction, factionConfigs);

		logGlobal->info("\t\tRelocating Town resources...");

		moveSiege(faction, factionConfigs);

		moveStructures(faction, factionConfigs);

		moveTown(faction, factionConfigs);
	}
}

void ResourceMover::moveNonJsonFiles(bool move_non_json_files)
{
	if(move_non_json_files)
	{
		logGlobal->info("\tRelocating Campaign and GUI resources...");

		moveCampaignAndGuiImages();

		moveCampaignAndGuiSprites();

		moveVideos();

		moveSounds();
	}
}

void ResourceMover::moveCreaturesFiles(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modImagesPath = modContentPath / "images";
	bfs::path modSoundsPath = modContentPath / "sounds";

	logGlobal->info("\t\tRelocating Creature resources...");

	for(const JsonNode & creatureMod : factionConfigs["creatures"].Vector())
	{
		std::string configFilePath = creatureMod.String();
		std::string configFileName = extractFileName(configFilePath);
		std::string creatureName = removeExtension(configFileName);

		const JsonNode configCreatures(ResourceID("Mods/SoD/mods/" + faction + "/content/" + configFilePath));

		// move creature sprites
		moveFileFromConfig(configCreatures, creatureName + "/graphics/animation", spritesPath, modSpritesPath);
		moveFileFromConfig(configCreatures, creatureName + "/graphics/map", spritesPath, modSpritesPath);
		moveFileFromConfig(configCreatures, creatureName + "/graphics/missile/projectile", spritesPath, modSpritesPath);

		// move creature icons
		moveFileFromConfig(configCreatures, creatureName + "/graphics/iconSmall", spritesPath / "CPrSmalL", modImagesPath);
		moveFileFromConfig(configCreatures, creatureName + "/graphics/iconLarge", spritesPath / "TwCrPort", modImagesPath);

		// move creature sound files
		for(auto action : { "attack", "defend", "killed", "move", "shoot", "wince", "startMoving", "endMoving"} )
			moveFileFromConfig(configCreatures, creatureName + "/sound/" + action, soundPath, modSoundsPath);
	}
}

void ResourceMover::moveHeroClasses(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";

	logGlobal->info("\t\t\tRelocating Hero Classes resources...");

	for(const JsonNode& heroClasesNode : factionConfigs["heroClasses"].Vector())
	{
		std::string sTemp = heroClasesNode.String();
		std::string configFileName = extractFileName(sTemp);
		std::string heroClassName = removeExtension(configFileName);

		const JsonNode configHeroClass(ResourceID("Mods/SoD/mods/" + faction + "/content/config/heroClasses/" + configFileName));

		moveFileFromConfig(configHeroClass, heroClassName + "/animation/battle/female", spritesPath, modSpritesPath);
		moveFileFromConfig(configHeroClass, heroClassName + "/animation/battle/male", spritesPath, modSpritesPath);

		moveFileFromConfig(configHeroClass, heroClassName + "/mapObject/templates/default/animation", spritesPath, modSpritesPath);
		moveFileFromConfig(configHeroClass, heroClassName + "/mapObject/templates/default/editorAnimation", spritesPath, modSpritesPath);
	}
}

void ResourceMover::moveIndividualHeroes(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modSoundsPath = modContentPath / "sounds";

	logGlobal->info("\t\t\tRelocating Individual Hero resources...");

	for(const JsonNode & heroNode : factionConfigs["heroes"].Vector()) // list of config files
	{
		std::string sTemp = heroNode.String();
		std::string configFileName = extractFileName(sTemp);
		std::string heroName = removeExtension(configFileName);

		const JsonNode configHeroes(ResourceID("Mods/SoD/mods/" + faction + "/content/config/heroes/" + configFileName));

		for(std::string iconType : { "large", "small", "specialtySmall", "specialtyLarge" })
			moveFileFromConfig(configHeroes, heroName + "/images/" + iconType, imagesPath, modSpritesPath);
	}
}

void ResourceMover::moveDwellings(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modSoundsPath = modContentPath / "sounds";

	logGlobal->info("\t\t\tRelocating Dwellings resources...");

	const JsonNode configDwellings(ResourceID("Mods/SoD/mods/" + faction + "/content/config/mapObjects/dwellings.json"));
	for(auto & nodeName : configDwellings[faction]["dwellings"].Struct())
		moveFileFromConfig(configDwellings[faction], "dwellings/" + nodeName.first + "/graphics", spritesPath, modSpritesPath);
}

void ResourceMover::moveCreatureBackgrounds(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modSoundsPath = modContentPath / "sounds";

	logGlobal->info("\t\t\tRelocating Creature Backgrounds resources...");

	const JsonNode configFaction(ResourceID("Mods/SoD/mods/" + faction + "/content/config/factions/" + faction + "/faction.json"));
	moveFileFromConfig(configFaction[faction], "creatureBackground/120px", imagesPath, modSpritesPath);
	moveFileFromConfig(configFaction[faction], "creatureBackground/130px", imagesPath, modSpritesPath);
}

void ResourceMover::movePuzzle(const std::string faction, const JsonNode factionConfigs)
{
	logGlobal->info("\t\t\tRelocating Puzzle resources...");

	bfs::path modPuzzleMapPath = modPath / "SoD\\mods" / faction / "content\\sprites\\factions" / faction / "puzzleMap";
	const JsonNode configPuzzle(ResourceID("Mods/SoD/mods/" + faction + "/content/config/factions/" + faction + "/puzzleMap.json"));
	std::string puzzlePrefix = configPuzzle[faction]["puzzleMap"]["prefix"].String();

	for(int i = 0; i <= 48; i++)
	{
		char* buffer = new char[256];

		sprintf(buffer, "%02d", i); // index format is XX
		std::string filename = puzzlePrefix + buffer + ".png";
		moveFile(filename, imagesPath, modPuzzleMapPath);

		delete[] buffer;
	}
}

void ResourceMover::moveSiege(const std::string faction, const JsonNode factionConfigs)
{
	std::string  siegeBuildings[35] = { "Arch", "Back", "Drw1", "Drw2", "Drw3", "DrwC", "Man1", "Man2", "ManC",
	 "Mlip", "MnBr", "MnOk", "Moat", "TpWl", "tpw1", "Tw11", "Tw12", "Tw1C", "Tw21", "Tw22", "Tw2C",
	 "Wa11", "Wa12", "Wa13", "Wa2", "Wa31", "Wa32", "Wa33", "Wa41", "Wa42", "Wa43", "Wa5", "Wa61", "Wa62", "Wa63" };

	logGlobal->info("\t\t\tRelocating Siege resources...");

	bfs::path modSiegePath = modPath / "SoD\\mods" / faction / "content\\sprites\\factions" / faction / "siege";
	const JsonNode configSiege(ResourceID("Mods/SoD/mods/" + faction + "/content/config/factions/" + faction + "/town/siege.json"));
	std::string siegePrefix = configSiege[faction]["town"]["siege"]["imagePrefix"].String();

	for(std::string siegeBuilding : siegeBuildings)
	{
		std::string filename = siegePrefix + siegeBuilding + ".png";
		moveFile(filename, imagesPath, modSiegePath);
	}
}

void ResourceMover::moveStructures(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modDataPath = modContentPath / "data";

	logGlobal->info("\t\t\tRelocating Structure resources...");
	
	const JsonNode configStructures(ResourceID("Mods/SoD/mods/" + faction + "/content/config/factions/" + faction + "/town/structures.json"));
	const JsonNode& structuresNode = configStructures[faction]["town"];

	// move animation sprites + area and border images
	logGlobal->info("\t\t\tRelocating Animation + Area + Borders resources...");
	for(auto & nodeName : structuresNode["structures"].Struct())
	{
		moveFileFromConfig(structuresNode, "structures/" + nodeName.first + "/animation", spritesPath, modSpritesPath);
		moveFileFromConfig(structuresNode, "structures/" + nodeName.first + "/area", imagesPath, modDataPath);
		moveFileFromConfig(structuresNode, "structures/" + nodeName.first + "/border", imagesPath, modDataPath);
	}
}

void ResourceMover::moveTown(const std::string faction, const JsonNode factionConfigs)
{
	modContentPath = modPath / "SoD\\mods" / faction / "content";
	bfs::path modSpritesPath = modContentPath / "sprites";
	bfs::path modDataPath = modContentPath / "data";
	bfs::path modMusicPath = modContentPath / "music";

	logGlobal->info("\t\t\tRelocating Town resources...");

	const JsonNode configTown(ResourceID("Mods/SoD/mods/" + faction + "/content/config/factions/" + faction + "/town/town.json"));

	for(auto background : { "townBackground", "guildWindow", "hallBackground" })
		moveFileFromConfig(configTown, faction + "/town/" + background, imagesPath, modDataPath);

	moveFileFromConfig(configTown, faction + "/town/buildingsIcons", spritesPath, modSpritesPath);

	for(std::string townLevel : { "fort", "citadel", "castle", "village", "capitol"})
		moveFileFromConfig(configTown, faction + "/town/mapObject/templates/" + townLevel + "/animation", spritesPath, modSpritesPath);

	for(auto & nodeName : configTown[faction]["town"]["icons"].Struct())
	{
		for(auto townIcon : { "normal/small", "/normal/large", "built/small", "built/large" })
			moveFileFromConfig(configTown[faction]["town"]["icons"], nodeName.first + "/" + townIcon, spritesPath, modSpritesPath);
	}

	moveFileFromConfig(configTown, faction + "/town/musicTheme", mp3Path, modMusicPath);
}

void ResourceMover::moveVideos()
{
	std::string filename;

	bfs::path destinationPath = modPath / "SoD/video/content/video/SoD/";

	logGlobal->info("\tRelocating Videos...");

	for (bfs::directory_entry& entry : bfs::directory_iterator(videoPath))
	{
		try
		{
			if(bfs::is_regular_file(entry))
			{
				std::string filename = entry.path().filename().string();
				filename = boost::locale::to_lower(filename);

				moveFile(filename, videoPath, destinationPath);
			}
			else
				logGlobal->info("\t\t\t\tVideo file: %s has no processing rule!", filename);
		}
		catch(const std::exception & ex)
		{
			std::cout << filename << " " << ex.what() << std::endl;
		}
	}
}

void ResourceMover::moveSounds()
{
	bfs::path destinationPath = modPath / "SoD/sound/content/sound/SoD/";
	std::string filename;

	logGlobal->info("\tRelocating Sounds...");
	
	for (bfs::directory_entry & entry : bfs::directory_iterator(soundPath))
	{
		try
		{
			if(bfs::is_regular_file(entry))
			{
				filename = entry.path().filename().string();
				filename = boost::algorithm::to_lower_copy(filename);

				// adventure map sounds
				if((filename.find("horse") == 0) || (filename.find("loop") == 0) || (filename.find("pickup") == 0) ||
					(filename.find("treasure") == 0) || (filename.find("chest") == 0) || (filename.find("digsound") == 0) ||
					(filename.find("expernce") == 0) || (filename.find("flagmine") == 0) || (filename.find("getprotection") == 0) ||
					(filename.find("graveyard") == 0) || (filename.find("killfade") == 0) || (filename.find("lighthouse") == 0) ||
					(filename.find("luck") == 0) || (filename.find("military") == 0) || (filename.find("morale") == 0) ||
					(filename.find("quest") == 0) || (filename.find("storm") == 0) || (filename.find("telptin") == 0) || (filename.find("temple") == 0))
					moveFile(filename, soundPath, destinationPath / "adventureMap/");

				// battle sounds
				else if((filename.find("badluck") == 0) || (filename.find("badmrle") == 0) || (filename.find("drawbrg") == 0) ||
					(filename.find("goodluck") == 0) || (filename.find("goodmrle") == 0) || (filename.find("keepshot") == 0) ||
					(filename.find("wallhit") == 0) || (filename.find("wallmiss") == 0))
					moveFile(filename, soundPath, destinationPath / "battle/");
			}
		}
		catch(const std::exception & ex)
		{
			std::cout << filename << " " << ex.what() << std::endl;
		}
	}
}

void ResourceMover::moveCampaignAndGuiImages()
{
	// move campaign bonuses ??? Investigate where these bonuses are moved

	bfs::path destinationPath = modPath / "SoD/campaigns/content/images/SoD/";
	bfs::path interfaceDestinationPath = modPath / "SoD/interface/content/images/SoD/";

	// sort and move campaign and GUI images
	for (bfs::directory_entry & entry : bfs::directory_iterator(imagesPath))
	{
		std::string filename;

		try
		{
			if(bfs::is_regular_file(entry))
			{
				filename = entry.path().filename().string();
				filename = boost::algorithm::to_lower_copy(filename);

				// bo Something
				if(filename.find("bo") == 0)
					if ((filename.find("box") != 0))
						moveFile(filename, imagesPath, destinationPath / "boSomething/");
					else {}

				// Campaign Maps
				else if(filename.find(".h3c") != std::string::npos)
					moveFile(filename, imagesPath, destinationPath / "campaignMaps/");

				// Campaign Bonuses
				else if(filename.find("cbon") == 0)
					moveFile(filename, imagesPath, destinationPath / "campaignBonuses/");

				// Caption Screens
				if(filename.find("csl") == 0)
					moveFile(filename, imagesPath, destinationPath / "captionScreens/");

				// Campaign Images
				else if(filename.find("camp") == 0)
					if(filename.find("campback") != 0 || filename.find("campbrf") != 0 || filename.find("campchk") != 0 ||
						filename.find("campswrd") != 0 || filename.find("campbkx2") != 0)
						moveFile(filename, imagesPath, destinationPath / "campaignImages/");
					else {}

				// Campaign World Maps
				else if((filename.find("ar") == 0 || filename.find("bb") == 0 || filename.find("br") == 0 || filename.find("e1") == 0 ||
					filename.find("e2") == 0 || filename.find("el") == 0 || filename.find("g1") == 0 || filename.find("g2") == 0 ||
					filename.find("g3") == 0 || filename.find("hs") == 0 || filename.find("is") == 0 || filename.find("kr") == 0 ||
					filename.find("n1") == 0 || filename.find("nb") == 0 || filename.find("ni") == 0 || filename.find("rn") == 0 ||
					filename.find("s1") == 0 || filename.find("sp") == 0 || filename.find("ta") == 0 || filename.find("ua") == 0)
					&& (filename.find("_") != std::string::npos))
					moveFile(filename, imagesPath, destinationPath / "campaignsWorldMaps/");

				// Config Files
				else if(filename.find(".txt") != std::string::npos)
					moveFile(filename, imagesPath, dataPath / "Config/");

				// Fonts
				else if(filename.find(".fnt") != std::string::npos)
					moveFile(filename, imagesPath, dataPath / "Fonts/");

				// Battle Backgrounds
				else if(filename.find("cmbk") == 0)
					moveFile(filename, imagesPath, interfaceDestinationPath / "battleBackgrounds/");

				// Battle Obstacles
				else if(filename.find("ob") == 0)
					moveFile(filename, imagesPath, interfaceDestinationPath / "battleObstacles/");

				// GUI
				else
					moveFile(filename, imagesPath, interfaceDestinationPath / "gui/");
			}
		}
		catch(const std::exception& ex)
		{
			std::cout << filename << " " << ex.what() << std::endl;
		}
	}
}

void ResourceMover::moveCampaignAndGuiSprites()
{
	std::string  extraAdventureMapObjects[14] = { "ADcfra.def", "ADCFRA0.def", "ADVMWIND.def", "AF00E.def", "AF01E.def", 
		"AF02E.def", "AF03E.def", "AF04E.def", "AF05E.def", "AF06E.def", "AF07E.def", "AHplace.def", "AHRANDOM.def", "arrow.def" };

	std::string  adventureMapTerrains[21] = { "adag.def", "Clrrvr.def", "cobbrd.def", "dirtrd.def", "DIRTTL.def", "EDG.def", 
		"GRASTL.def", "gravrd.def", "Icyrvr.def", "Lavatl.def", "Lavrvr.def", "Mudrvr.def", "rocktl.def", "ROUGTL.def", 
		"sandtl.def", "Snowtl.def", "Subbtl.def", "Swmptl.def", "Tshrc.def", "Tshre.def", "Watrtl.def" };

	std::string filename;

	// sort and move campaign and GUI sprites
	bfs::path destinationPath = modPath / "SoD/campaigns/content/sprites/SoD/";
	bfs::path interfaceDestinationPath = modPath / "SoD/interface/content/sprites/SoD/";

	// extra adventure map objects
	for(auto adventureMapObject : extraAdventureMapObjects)
		moveFile(adventureMapObject, spritesPath, interfaceDestinationPath / "adventureMapObjects/");

	// adventure map terrains
	for(std::string filename : adventureMapTerrains)
		moveFile(filename, spritesPath, interfaceDestinationPath / "adventureMapTerrains/");

	for(bfs::directory_entry & entry : bfs::directory_iterator(spritesPath))
	{
		try
		{
			if(bfs::is_regular_file(entry))
			{
				filename = entry.path().filename().string();
				filename = boost::algorithm::to_lower_copy(filename);

				// combat Obstacles
				if(filename.find("ob") == 0)
					moveFile(filename, spritesPath, interfaceDestinationPath / "combatObstacles/");

				// Cursors
				else if(filename.find("cr") == 0)
					moveFile(filename, spritesPath, interfaceDestinationPath / "gui/cursors/");

				// Adventure Map Objects
				else if(filename.find("av") == 0)
					moveFile(filename, spritesPath, interfaceDestinationPath / "adventureMapObjects/");

				// Adventure Map Terrains / River Deltas
				else if(filename.find("delt") != std::string::npos)
					moveFile(filename, spritesPath, interfaceDestinationPath / "adventureMapTerrains/deltas/");

				// Campaign Maps
				else if(filename.find(".h3c") != std::string::npos)
					moveFile(filename, spritesPath, destinationPath / "campaignMaps/");

				// rest is GUI
				else
					moveFile(filename, spritesPath, interfaceDestinationPath / "gui/");
			}
		}
		catch(const std::exception & ex)
		{
			std::cout << filename << " " << ex.what() << std::endl;
		}
	}
}
