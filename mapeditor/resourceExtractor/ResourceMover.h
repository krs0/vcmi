/*
 * ResourceMover.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
 
#include "../lib/JsonNode.h"

namespace bfs = boost::filesystem;

class ResourceMover
{

public:
	ResourceMover();

	/// Parse all H3 original source folders (extracted previously) for all resources and moves them to teir corresponding places inside the Mod folder
	void parseOriginalDataFilesAndMoveToMods(bool move_extracted_files = false);

private:
	/// Move Artifact related files
	void moveArtifacts();
	/// Move creature banks files
	void moveCreatureBanks();
	/// Move spell related files
	void moveSpells();
	/// For each faction move faction files
	void moveFactions();
	/// Move no json files
	void moveNonJsonFiles();
	/// Move all creature files for a faction
	void moveCreaturesFiles(const std::string faction, const JsonNode factionConfigs);
	/// move hero classes files
	void moveHeroClasses(const std::string faction, const JsonNode factionConfigs);
	void moveIndividualHeroes(const std::string faction, const JsonNode factionConfigs);
	void moveDwellings(const std::string faction, const JsonNode factionConfigs);
	void moveCreatureBackgrounds(const std::string faction, const JsonNode factionConfigs);
	void movePuzzle(const std::string faction, const JsonNode factionConfigs);
	void moveSiege(const std::string faction, const JsonNode factionConfigs);
	void moveStructures(const std::string faction, const JsonNode factionConfigs);
	void moveTown(const std::string faction, const JsonNode factionConfigs);

	void moveCampaignAndGuiImages();
	void moveCampaignAndGuiSprites();

	void moveSounds();
	void moveVideos();

	void moveResource(const JsonNode node, std::string nodeStructure, bfs::path sourceRoot, bfs::path destinationRoot);

	bfs::path modsPath;
	bfs::path dataPath;
	bfs::path spritesPath;
	bfs::path imagesPath;
	bfs::path soundPath;
	bfs::path videoPath;
	bfs::path mp3Path;

	bfs::path modContentPath;
	bfs::path modSpritesPath;
	bfs::path modImagesPath;
	bfs::path modSoundsPath;
	bfs::path modMusicPath;
	bfs::path modDataPath;

	std::string modResourceRoot;

	bool deleteSource = true; // delete source files or leave a copy in place.

	bool move_non_json_files = true; // move files that are not yet supported by mods.
	bool move_artifacts = true; // no jsons yet
	bool move_creature_banks = true; // no jsons yet
	bool move_spells = true; // no jsons yet
};

#pragma region Helper Functions

/// Simplified wrapper over FileInfo::GetFilename
std::string getFileName(std::string filePath);

/// simplified wrapper over FileInfo::GetStem
std::string getFileStem(std::string filePath);

/// move file using complete file paths for source and destination files
void moveFile(bfs::path sourceFilePath, bfs::path destinationFilePath, bool deleteSource);

/// move file using file name and source and destination folders
void moveFile(std::string filename, bfs::path sourceFolder, bfs::path destinationFolder, bool deleteSource);

/// Moves a file from a list of filenames, if its name starts with filePrefix. (EG: ar matches arc23.waw)
/// Optionaly it also looks for a substring if its present anywhere in the filename. (EG filePrefix ar filePart _ will give a match in arc_23.wav)
void moveFileIfFoundInList(std::string filePrefix, std::vector<std::string> filenames, bfs::path sourceFolder, bfs::path destinationFolder, bool deleteSource, std::string filePart = "");

#pragma endregion
