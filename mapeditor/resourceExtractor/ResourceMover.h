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
 
namespace bfs = boost::filesystem;

class ResourceMover
{

public:
	ResourceMover();

	void parseOriginalDataFilesAndMoveToMods(bool move_extracted_files = false);

private:
	bfs::path modPath;
	bfs::path dataPath;
	bfs::path spritesPath;
	bfs::path imagesPath;
	bfs::path soundPath;
	bfs::path videoPath;
	bfs::path mp3Path;
	bfs::path modContentPath;
};
