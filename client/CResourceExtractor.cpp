#include "StdInc.h"

#include "../lib/JsonNode.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"

#include "CResourceExtractor.h"
#include "SDL.h"
#include "CDefHandler.h"
#include "CBitmapHandler.h"

#include "boost/filesystem/path.hpp"
#include "boost/locale.hpp"
#include <thread>
#include <chrono>

bool parse_and_move = 0; // enable disable the whole parsing and moving of data files thing. (Use only if you know what this is doing. krs)
bool move_non_json_files = 0; // move files that are not yet supported by mods. (Use only if you know what this is doing. krs)
bool only_copy_files = 1; // do not delete source files. Leave a copy in place. (Use only if you know what this is doing. krs)
bool pcx_to_bmp = 0; // converts .pcx to bmp. Can be used when you have .pcx converted already (Use only if you know what this is doing. krs)


// extracts filename with extrension <filename.ext>
std::string extractFileName(std::string source)
{
	int index = source.find_last_of("/");
	return source.substr(index+1);
}

// removes extension from a filename <filename>
std::string removeExtension(std::string filename)
{
	int index = filename.find_last_of(".");
	return filename.substr(0, index);
}

// converts all pcx files into bmp
void convertPcxToBmp(boost::filesystem::path dataPath)
{
	namespace fs = boost::filesystem;
	fs::directory_iterator end_iter;

	for ( fs::directory_iterator dir_itr( dataPath / "Images" ); dir_itr != end_iter; ++dir_itr )
	{
		try
		{
			if ( fs::is_regular_file( dir_itr->status() ) )
			{
				//std::cout << dir_itr->path().filename() << "\n";
				std::string filename = dir_itr->path().filename().string();
				filename = boost::locale::to_lower(filename);

				if(filename.find(".pcx") != std::string::npos)
				{
					SDL_Surface *bitmap;
					
					bitmap = BitmapHandler::loadBitmap(filename);
					
					if(!only_copy_files)
						fs::remove( dataPath / "Images" / filename);

					filename = (dataPath / "Images" / (removeExtension(filename) + ".bmp")).string();
	 				SDL_SaveBMP(bitmap, filename.c_str());
				}
			}
			else
			{
				std::cout << dir_itr->path().filename() << " [other]\n";
			}
		}
		catch ( const std::exception & ex )
		{
			std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
		}
	}
}

// splits a def file into individual parts
void splitDefFile(std::string fileName, boost::filesystem::path spritesPath)
{
	if (CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + fileName)))
	{
		CDefEssential * cde = CDefHandler::giveDefEss(fileName);
		
		for (size_t i=0; i<cde->ourImages.size(); i++)
		{
			boost::filesystem::path newFilename = spritesPath / (removeExtension(fileName) + (boost::lexical_cast<std::string>(i) + ".bmp"));
			SDL_SaveBMP(cde->ourImages[i].bitmap, newFilename.string().c_str());
		}

		if(!only_copy_files)
			boost::filesystem::remove(spritesPath / fileName);

	}
	else
		logGlobal->errorStream() << "Def File Split error! " + fileName;
}

void moveFile(boost::filesystem::path sourceFilePath, boost::filesystem::path destinationFilePath)
{
	if(boost::filesystem::exists(sourceFilePath))
	{
		if(only_copy_files)
			boost::filesystem::copy_file(sourceFilePath, destinationFilePath, boost::filesystem::copy_option::overwrite_if_exists);	
		else
			boost::filesystem::rename(sourceFilePath, destinationFilePath);
	}
}

void moveFile(std::string filename, boost::filesystem::path sourceFolder, boost::filesystem::path destinationFolder)
{
	boost::filesystem::path sourceFilePath = sourceFolder / filename;
	if(boost::filesystem::exists(sourceFilePath))
	{
		if(only_copy_files)
			boost::filesystem::copy_file(sourceFilePath, destinationFolder / filename, boost::filesystem::copy_option::overwrite_if_exists);
		else
			boost::filesystem::rename(sourceFilePath, destinationFolder / filename);
	}
}

void moveFileFromConfig(const JsonNode node, std::string firstNode, std::string secondNode,
	boost::filesystem::path sourceRoot, boost::filesystem::path destinationRoot)
{
	if (!node[firstNode][secondNode].isNull())
	{
		std::string partialFilePath = node[firstNode][secondNode].String();
		std::string fileToMove = extractFileName(partialFilePath);
		moveFile(sourceRoot / fileToMove, destinationRoot / partialFilePath);

		std::string partialMaskPath = removeExtension(partialFilePath) + ".msk";
		std::string maskToMove = extractFileName(partialMaskPath);
		moveFile(sourceRoot / maskToMove, destinationRoot / partialMaskPath);
	}
}

void moveFileFromConfig(const JsonNode node, std::string firstNode, std::string secondNode, std::string thirdNode,
	boost::filesystem::path sourceRoot, boost::filesystem::path destinationRoot)
{
	if (!node[firstNode][secondNode][thirdNode].isNull())
	{
		std::string partialFilePath = node[firstNode][secondNode][thirdNode].String();
		std::string fileToMove = extractFileName(partialFilePath);
		moveFile(sourceRoot / fileToMove, destinationRoot / partialFilePath);

		std::string partialMaskPath = removeExtension(partialFilePath) + ".msk";
		std::string maskToMove = extractFileName(partialMaskPath);
		moveFile(sourceRoot / maskToMove, destinationRoot / partialMaskPath);
	}
}

void moveFileFromConfig(const JsonNode node, std::string firstNode, std::string secondNode, std::string thirdNode, std::string fourthNode,
	boost::filesystem::path sourceRoot, boost::filesystem::path destinationRoot)
{
	if (!node[firstNode][secondNode][thirdNode][fourthNode].isNull())
	{
		std::string partialFilePath = node[firstNode][secondNode][thirdNode][fourthNode].String();
		std::string fileToMove = extractFileName(partialFilePath);
		moveFile(sourceRoot / fileToMove, destinationRoot / partialFilePath);

		std::string partialMaskPath = removeExtension(partialFilePath) + ".msk";
		std::string maskToMove = extractFileName(partialMaskPath);
		moveFile(sourceRoot / maskToMove, destinationRoot / partialMaskPath);
	}
}


// parse all H3 original source folders (extracted previously) for all resources and moves them to teir corresponding places inside the Mod folder
void parseOriginalDataFilesAndMoveToMods()
{
	if (!parse_and_move)
		return;

	// All Videos remain in Data/Videos
	boost::filesystem::path modPath = VCMIDirs::get().binaryPath() / "Mods";
	boost::filesystem::path dataPath = VCMIDirs::get().binaryPath() / "Data" / "Temp";
	boost::filesystem::path spritesPath = dataPath / "Sprites";
	boost::filesystem::path imagesPath = dataPath / "Images";
	boost::filesystem::path soundPath = dataPath / "Sound";
	boost::filesystem::path videoPath = dataPath / "Video";
	boost::filesystem::path mp3Path = VCMIDirs::get().binaryPath() / "Mp3";
	boost::filesystem::path destinationPath = "";

	// split def files, so that factions are independent
	splitDefFile("TwCrPort.def", spritesPath);	// split town creature portraits
	splitDefFile("CPRSMALL.def", spritesPath);	// split hero army creature portraits 
	splitDefFile("FlagPort.def", spritesPath);	// adventure map dwellings
	splitDefFile("ITPA.def", spritesPath);		// small town icons
	splitDefFile("ITPt.def", spritesPath);		// big town icons

	boost::locale::generator gen;				// Create locale generator 
	std::locale::global(gen(""));				// "" - the system default locale, set it globally

	// convert from pcx to bmp (H3 saves images as .pcx)
	if(pcx_to_bmp)
		convertPcxToBmp(dataPath);

	/////////////////////////////////////////////
	//// Move Artifact related stuff

	//// Move artifact adventuere map icons
	//destinationPath = modPath.append("SoD/mods/artifacts/resources/sprites/SoD/");
	//for(int i=1; i<=144; i++)
	//{
	//	char *buffer = new char[256];
	//	sprintf(buffer, "%04d", i);	// indexes are in 0xxx format.

	//	std::string filename = std::string("AVA") + buffer;
	//	moveFile(filename + ".def", spritesPath, destinationPath);
	//	moveFile(filename + ".msk", spritesPath, destinationPath);

	//	delete[] buffer;
	//}

	//// Move artifact icons, all in 1 file identified by index.
	//moveFile("artifact.def", spritesPath, destinationPath);
	//moveFile("artifBon.def", spritesPath, destinationPath);


	/////////////////////////////////////////////
	//// Move creature banks files

	//destinationPath = modPath.append("SoD/mods/creatureBanks/resources/sprites/SoD/");
	//const JsonNode configCreatureBanks(ResourceID("Mods/SoD/mods/creatureBanks/resources/config/SoD/creatureBanks.json"));
	//for(const JsonNode &oneBank : configCreatureBanks["banks"].Vector())
	//{
	//	for(const JsonNode &graphics : oneBank["graphics"].Vector())
	//	{
	//		std::string sTemp = graphics["adventureMap"].String();
	//		moveFile(sTemp, spritesPath, destinationPath);
	//	}
	//}

	/////////////////////////////////////////////
	//// Move spell related files

	//// Move spell sound files
	//destinationPath = modPath / "SoD/mods/spells/resources/";
	//const JsonNode configSpellInfo(ResourceID("Mods/SoD/mods/spells/resources/config/SoD/spellInfo.json"));
	//for(auto &spell : configSpellInfo["spells"].Struct())
	//{
	//	std::string spellSoundPath = spell.second["soundfile"].String();
	//	if(spellSoundPath != "")
	//		moveFile(soundPath / extractFileName(spellSoundPath), destinationPath / spellSoundPath);
	//}
	//
	//// Move spell graphics files
	//moveFile("spells.def", spritesPath, modPath / "SoD/mods/spells/resources/sprites/SoD");
	//moveFile("spellScr.def", spritesPath, modPath / "SoD/mods/spells/resources/sprites/SoD");


	//////////////////////////////////////////////////////
	// For each faction move files

	// Definitions
	std::vector<std::string> factions;
	factions.push_back("castle");
	factions.push_back("conflux");
	factions.push_back("dungeon");
	factions.push_back("fortress");
	factions.push_back("inferno");
	factions.push_back("necropolis");
	factions.push_back("neutral");
	factions.push_back("rampart");
	factions.push_back("stronghold");
	factions.push_back("tower");

	std::string  siegeBuildings[35] = {"Arch", "Back", "Drw1", "Drw2", "Drw3", "DrwC", "Man1", "Man2", "ManC",
		 "Mlip", "MnBr", "MnOk", "Moat", "TpWl", "tpw1", "Tw11", "Tw12", "Tw1C", "Tw21", "Tw22", "Tw2C",
		 "Wa11", "Wa12", "Wa13", "Wa2", "Wa31", "Wa32", "Wa33", "Wa41", "Wa42", "Wa43", "Wa5", "Wa61", "Wa62", "Wa63"};

	std::string  extraAdventureObjects[14] = {"ADcfra.def", "ADCFRA0.def", "ADVMWIND.def", "AF00E.def", "AF01E.def", 
		"AF02E.def", "AF03E.def", "AF04E.def", "AF05E.def", "AF06E.def", "AF07E.def", "AHplace.def", "AHRANDOM.def", "arrow.def" };					  

	std::string  adventureMapTerrains[21] = {"adag.def", "Clrrvr.def", "cobbrd.def", "dirtrd.def", "DIRTTL.def", "EDG.def",
		"GRASTL.def", "gravrd.def", "Icyrvr.def", "Lavatl.def", "Lavrvr.def", "Mudrvr.def", "rocktl.def", "ROUGTL.def",
		"sandtl.def", "Snowtl.def", "Subbtl.def", "Swmptl.def", "Tshrc.def", "Tshre.def", "Watrtl.def"};


	// for each faction move files
	for(std::string faction: factions)
	{
		//////////////////////////////////////////////////////
		// move creature files

		// get list of creature config files
		const JsonNode configCreatureList(ResourceID("Mods/SoD/mods/" + faction + "/mod.json"));
		destinationPath = modPath / "SoD\\mods" / faction / "resources";
		for(const JsonNode &creatureMod : configCreatureList["creatures"].Vector())
		{
			std::string configFilePath = creatureMod.String();
			std::string configFileName = extractFileName(configFilePath);
			std::string creatureName = removeExtension(configFileName);

			// move creature sprites
			const JsonNode configCreatures(ResourceID("Mods/SoD/mods/"+ faction + "/resources/" + configFilePath));

			moveFileFromConfig(configCreatures, creatureName, "graphics", "animation", spritesPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "graphics", "map", spritesPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "graphics", "missile", "projectile", spritesPath, destinationPath);

			// move creature icons
			moveFileFromConfig(configCreatures, creatureName, "graphics", "iconSmall", spritesPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "graphics", "iconLarge", spritesPath, destinationPath);

			// move creature sound files
			moveFileFromConfig(configCreatures, creatureName, "sound", "attack", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "defend", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "killed", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "move", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "shoot", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "wince", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "startMoving", soundPath, destinationPath);
			moveFileFromConfig(configCreatures, creatureName, "sound", "endMoving", soundPath, destinationPath);

		}


		//////////////////////////////////////////////////////
		// move hero files
		
		destinationPath = modPath / "SoD\\mods" / faction / "resources";
		const JsonNode configHeroesList(ResourceID("Mods/SoD/mods/" + faction + "/mod.json")); // list of config files

		// move hero classes files
		for(const JsonNode &heroClasesNode : configHeroesList["heroClasses"].Vector())
		{
			std::string sTemp = heroClasesNode.String();
			std::string configFileName = extractFileName(sTemp);
			std::string heroClassName = removeExtension(configFileName);

			const JsonNode configHeroClass(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/heroClasses/" + configFileName));

			moveFileFromConfig(configHeroClass, heroClassName, "animation", "battle", "female", spritesPath, destinationPath);
			moveFileFromConfig(configHeroClass, heroClassName, "animation", "battle", "male", spritesPath, destinationPath);

			moveFileFromConfig(configHeroClass, heroClassName, "animation", "map", "female", spritesPath, destinationPath);
			moveFileFromConfig(configHeroClass, heroClassName, "animation", "map", "male", spritesPath, destinationPath);
		}

		// move individual hero files
		for(const JsonNode &heroNode : configHeroesList["heroes"].Vector()) // list of config files
		{
			std::string sTemp = heroNode.String();
			std::string configFileName = extractFileName(sTemp);
			std::string heroName = removeExtension(configFileName);

			const JsonNode configHeroes(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/heroes/" + configFileName));

			moveFileFromConfig(configHeroes, heroName, "images", "large", imagesPath, destinationPath);
			moveFileFromConfig(configHeroes, heroName, "images", "small", imagesPath, destinationPath);
		}


		//////////////////////////////////////////////////////
		// move faction files

		// move dwellings files
		destinationPath = modPath / "SoD\\mods" / faction / "resources";
		const JsonNode configDwellings(ResourceID("Mods/SoD/mods/"+ faction + "/resources/config/factions/" + faction + "/dwellings.json"));
		for(auto &nodeName : configDwellings[faction]["dwellings"].Struct())
			moveFileFromConfig(configDwellings[faction], "dwellings", nodeName.first, "graphics", spritesPath, destinationPath );

		// move creature backgrounds
		const JsonNode configFaction(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/factions/" + faction + "/faction.json"));
		moveFileFromConfig(configFaction[faction], "creatureBackground", "120px", imagesPath, destinationPath );
		moveFileFromConfig(configFaction[faction], "creatureBackground", "130px", imagesPath, destinationPath );

		// exit for if neutral faction. Does not have the rest of files.
		if (faction == "neutral")
			break;

		// move puzzle files
		destinationPath = modPath / "SoD\\mods" / faction / "resources\\sprites\\factions" / faction / "puzzleMap";
		const JsonNode configPuzzle(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/factions/" + faction +  "/puzzleMap.json"));
		std::string puzzlePrefix =  configPuzzle[faction]["puzzleMap"]["prefix"].String();

		for(int i=0; i<=48; i++)
		{
			char *buffer = new char[256];

			sprintf(buffer, "%02d", i); // index format is XX
			std::string filename = puzzlePrefix + buffer + ".bmp";
			moveFile(filename, imagesPath, destinationPath);

			delete[] buffer;
		}


		//////////////////////////////////////////////////////		
		// move town files

		// move siege files
		destinationPath = modPath / "SoD\\mods" / faction / "resources\\sprites\\factions" / faction / "siege";
		const JsonNode configSiege(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/factions/" + faction + "/town/siege.json"));
		std::string siegePrefix =  configSiege[faction]["town"]["siege"]["imagePrefix"].String();

		for(std::string siegeBuilding : siegeBuildings)
		{
			std::string filename = siegePrefix + siegeBuilding + ".bmp";
			moveFile(filename, imagesPath, destinationPath);
		}

		// move structure files
		destinationPath = modPath / "SoD\\mods" / faction / "resources\\sprites\\factions" / faction / "townScreen";
		const JsonNode configStructures(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/factions/" + faction + "/town/structures.json"));
		const JsonNode &structuresNode = configStructures[faction]["town"];

		// move animation sprites + area and border images
		for(auto &nodeName : structuresNode["structures"].Struct())
		{
			moveFileFromConfig(structuresNode, "structures", nodeName.first, "animation", spritesPath, destinationPath / "animation");
			moveFileFromConfig(structuresNode, "structures", nodeName.first, "area", imagesPath, destinationPath / "area");
			moveFileFromConfig(structuresNode, "structures", nodeName.first, "border", imagesPath, destinationPath / "border");
		}

		// move town files

		const JsonNode configTown(ResourceID("Mods/SoD/mods/" + faction + "/resources/config/factions/" + faction + "/town/town.json"));
		destinationPath = modPath / "SoD\\mods" / faction / "resources";
		
		moveFileFromConfig(configTown, faction, "town", "townBackground", imagesPath, destinationPath);
		moveFileFromConfig(configTown, faction, "town", "guildWindow", imagesPath, destinationPath); 
		moveFileFromConfig(configTown, faction, "town", "hallBackground", imagesPath, destinationPath);

		moveFileFromConfig(configTown, faction, "town", "adventureMap", "capitol", spritesPath, destinationPath);
		moveFileFromConfig(configTown, faction, "town", "adventureMap", "castle", spritesPath, destinationPath);
		moveFileFromConfig(configTown, faction, "town", "adventureMap", "village", spritesPath, destinationPath);
		moveFileFromConfig(configTown, faction, "town", "buildingsIcons", spritesPath, destinationPath);

		for(auto &nodeName : configTown[faction]["town"]["icons"].Struct())
		{
			moveFileFromConfig(configTown[faction]["town"]["icons"], nodeName.first, "normal", "small", spritesPath, destinationPath); 
			moveFileFromConfig(configTown[faction]["town"]["icons"], nodeName.first, "normal", "large", spritesPath, destinationPath); 
			moveFileFromConfig(configTown[faction]["town"]["icons"], nodeName.first, "built", "small", spritesPath, destinationPath); 
			moveFileFromConfig(configTown[faction]["town"]["icons"], nodeName.first, "built", "large", spritesPath, destinationPath); 
		}
		
		// move town music theme
		moveFileFromConfig(configTown, "town", "musicTheme", mp3Path, destinationPath);

	}

	////////////////////////////////////////////////////////
	//// move Campaign Files and GUI!!!
	//if(move_non_json_files)
	//{

	//// move campaign bonuses
	//destinationPath = modPath + "SoD/campaigns/content/images/SoD/";
	//std::string interfaceDestinationPath = modPath + "SoD/interface/content/images/SoD/";

	//namespace fs = boost::filesystem;
	//fs::directory_iterator end_iter;

	//// sort and move campaign and GUI images
	//for ( fs::directory_iterator dir_itr( imagesPath ); dir_itr != end_iter; ++dir_itr )
	//{
	//	try
	//	{
	//		if ( fs::is_regular_file( dir_itr->status() ) )
	//		{
	//			std::string filename = dir_itr->path().filename().string();
	//			filename = boost::locale::to_lower(filename);

	//			// bo Something
	//			if(filename.find("bo") == 0)
	//				if((filename.find("box") != 0 ))
	//					moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "boSomething/");
	//				else{}
	//			
	//			// Campaign Maps
	//			else if (filename.find(".h3c") != std::string::npos)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "campaignMaps/");
	//				
	//			// Campaign Bonuses
	//			else if(filename.find("cbon") == 0)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "campaignBonuses/");

	//			// Caption Screens
	//			if(filename.find("csl") == 0)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "captionScreens/");

	//			// Campaign Images
	//			else if(filename.find("camp") == 0)
	//				if((filename.find("campback") != 0 ) || (filename.find("campbrf") != 0) || 
	//					(filename.find("campchk") != 0)  || (filename.find("campswrd") != 0)  || (filename.find("campbkx2") != 0))
	//					moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "campaignImages/");
	//				else {}

	//			// Campaign World Maps
	//			else if(((filename.find("ar") == 0 ) || (filename.find("bb") == 0) || (filename.find("br") == 0)  || (filename.find("e1") == 0)  || 
	//				(filename.find("e2") == 0) || (filename.find("el") == 0) || (filename.find("g1") == 0) || (filename.find("g2") == 0) || 
	//				(filename.find("g3") == 0) || (filename.find("hs") == 0) || (filename.find("is") == 0) || (filename.find("kr") == 0) ||
	//				(filename.find("n1") == 0) || (filename.find("nb") == 0) || (filename.find("ni") == 0) || (filename.find("rn") == 0) ||
	//				(filename.find("s1") == 0) || (filename.find("sp") == 0) || (filename.find("ta") == 0) || (filename.find("ua") == 0))
	//				&& ((filename.find("_") != std::string::npos)))
	//				moveFile(dir_itr->path().filename().string(), imagesPath, destinationPath + "campaignsWorldMaps/");

	//			// Config Files
	//			else if (filename.find(".txt") != std::string::npos)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, dataPath + "Config/");

	//			// Fonts
	//			else if (filename.find(".fnt") != std::string::npos)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, dataPath + "Fonts/");

	//			// Battle Backgrounds
	//			else if (filename.find("cmbk") == 0)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, interfaceDestinationPath + "battleBackgrounds/");

	//			// Battle Obstacles
	//			else if (filename.find("ob") == 0)
	//				moveFile(dir_itr->path().filename().string(), imagesPath, interfaceDestinationPath + "battleObstacles/");

	//			// GUI
	//			else
	//				moveFile(dir_itr->path().filename().string(), imagesPath, interfaceDestinationPath + "gui/");
	//		}
	//		else
	//		{
	//			std::cout << dir_itr->path().filename() << "\n";
	//		}
	//	}
	//	catch ( const std::exception & ex )
	//	{
	//		std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
	//	}
	//}

	//// sort and move campaign and GUI sprites
	//destinationPath = modPath + "SoD/campaigns/content/sprites/SoD/";
	//interfaceDestinationPath = modPath + "SoD/interface/content/sprites/SoD/";

	//// extra adventure map objects
	//for(std::string filename : extraAdventureObjects)
	//	moveFile(filename, spritesPath, interfaceDestinationPath + "adventureMapObjects/");

	//// adventure map terrains
	//for(std::string filename : adventureMapTerrains)
	//	moveFile(filename, spritesPath, interfaceDestinationPath + "adventureMapTerrains/");

	//for ( fs::directory_iterator dir_itr( spritesPath ); dir_itr != end_iter; ++dir_itr )
	//{
	//	try
	//	{
	//		if ( fs::is_regular_file( dir_itr->status() ) )
	//		{
	//			std::string filename = dir_itr->path().filename().string();
	//			filename = boost::locale::to_lower(filename);

	//			// combat Obstacles
	//			if(filename.find("ob") == 0)
	//				moveFile(dir_itr->path().filename().string(), spritesPath, interfaceDestinationPath + "combatObstacles/");

	//			// Cursors
	//			else if (filename.find("cr") == 0)
	//				moveFile(dir_itr->path().filename().string(), spritesPath, interfaceDestinationPath + "gui/cursors/");

	//			// Adventure Map Objects
	//			else if (filename.find("av") == 0)
	//				moveFile(dir_itr->path().filename().string(), spritesPath, interfaceDestinationPath + "adventureMapObjects/");

	//			// Adventure Map Terrains / River Deltas
	//			else if (filename.find("delt") != std::string::npos)
	//				moveFile(dir_itr->path().filename().string(), spritesPath, interfaceDestinationPath + "adventureMapTerrains/deltas/");

	//			// Campaign Maps
	//			else if (filename.find(".h3c") != std::string::npos)
	//				moveFile(dir_itr->path().filename().string(), spritesPath, destinationPath + "campaignMaps/");

	//			// rest is GUI
	//			else
	//				moveFile(dir_itr->path().filename().string(), spritesPath, interfaceDestinationPath + "gui/");
	//		}
	//		else
	//		{
	//			std::cout << dir_itr->path().filename() << "\n";
	//		}
	//	}
	//	catch ( const std::exception & ex )
	//	{
	//		std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
	//	}
	//}

	/////////////////////////////////////////////
	//// Move Videos

	//destinationPath = modPath + "SoD/video/content/video/SoD/";
	//for ( fs::directory_iterator dir_itr( videoPath ); dir_itr != end_iter; ++dir_itr )
	//{
	//	try
	//	{
	//		if ( fs::is_regular_file( dir_itr->status() ) )
	//		{
	//			std::string filename = dir_itr->path().filename().string();
	//			filename = boost::locale::to_lower(filename);

	//			moveFile(dir_itr->path().filename().string(), videoPath, destinationPath);
	//		}
	//		else
	//		{
	//			std::cout << dir_itr->path().filename() << "\n";
	//		}
	//	}
	//	catch ( const std::exception & ex )
	//	{
	//		std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
	//	}
	//}


	/////////////////////////////////////////////
	//// Move Sounds

	//destinationPath = modPath + "SoD/sound/content/sound/SoD/";
	//for (fs::directory_iterator dir_itr(soundPath); dir_itr != end_iter; ++dir_itr)
	//{
	//	try
	//	{
	//		if (fs::is_regular_file(dir_itr->status()))
	//		{
	//			std::string filename = dir_itr->path().filename().string();
	//			filename = boost::locale::to_lower(filename);

	//			// adventure map sounds
	//			if ((filename.find("horse") == 0) || (filename.find("loop") == 0) || (filename.find("pickup") == 0) ||
	//				(filename.find("treasure") == 0) || (filename.find("chest") == 0) || (filename.find("digsound") == 0) ||
	//				(filename.find("expernce") == 0) || (filename.find("flagmine") == 0) || (filename.find("getprotection") == 0) ||
	//				(filename.find("graveyard") == 0) || (filename.find("killfade") == 0) || (filename.find("lighthouse") == 0) ||
	//				(filename.find("luck") == 0) || (filename.find("military") == 0) || (filename.find("morale") == 0) ||
	//				(filename.find("quest") == 0) || (filename.find("storm") == 0) || (filename.find("telptin") == 0) || (filename.find("temple") == 0))
	//				moveFile(dir_itr->path().filename().string(), soundPath, destinationPath + "adventureMap/");

	//			else if ((filename.find("badluck") == 0) || (filename.find("badmrle") == 0) || (filename.find("drawbrg") == 0) ||
	//				(filename.find("goodluck") == 0) || (filename.find("goodmrle") == 0) || (filename.find("keepshot") == 0) ||
	//				(filename.find("wallhit") == 0) || (filename.find("wallmiss") == 0))
	//				moveFile(dir_itr->path().filename().string(), soundPath, destinationPath + "battle/");
	//		}
	//		else
	//		{
	//			std::cout << dir_itr->path().filename() << "\n";
	//		}
	//	}
	//	catch (const std::exception & ex)
	//	{
	//		std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
	//	}
	//}

	//}

	// TODO: Delete Data Temp when all files are placed in the right places

}





















