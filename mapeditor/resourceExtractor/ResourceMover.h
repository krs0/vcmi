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

	// parse all H3 original source folders (extracted previously) for all resources and moves them to teir corresponding places inside the Mod folder
	void parseOriginalDataFilesAndMoveToMods(bool move_extracted_files = false);

private:
	// Move Artifact related stuff
	void moveArtifacts(bool move_artifacts);
	// Move creature banks files
	void moveCreatureBanks(bool move_creature_banks);
	// Move spell related files
	void moveSpells(bool move_spells);
	// For each faction move files
	void moveFactions(bool move_factions);
	// Move no json files
	void moveNonJsonFiles(bool move_non_json_files);
	// Move all creature files for a faction
	void moveCreaturesFiles(const std::string faction, const JsonNode configCreatureList);
	// move hero classes files
	void moveHeroClasses(const std::string faction, const JsonNode factionConfigs);
	void moveIndividualHeroes(const std::string faction, const JsonNode factionConfigs);
	void moveDwellings(const std::string faction, const JsonNode factionConfigs);
	void moveCreatureBackgrounds(const std::string faction, const JsonNode factionConfigs);
	void movePuzzle(const std::string faction, const JsonNode factionConfigs);
	void moveSiege(const std::string faction, const JsonNode factionConfigs);
	void moveStructures(const std::string faction, const JsonNode factionConfigs);
	void moveTown(const std::string faction, const JsonNode factionConfigs);

	bfs::path modPath;
	bfs::path dataPath;
	bfs::path spritesPath;
	bfs::path imagesPath;
	bfs::path soundPath;
	bfs::path videoPath;
	bfs::path mp3Path;
	bfs::path modContentPath;
};
