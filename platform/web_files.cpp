// web_files.cpp - C++ side of the browser file bridge (see web_files.js).

#include "web_files.h"
#include "meshgen/Application.h"

#include <emscripten.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

extern "C" {
	// Implemented in web_files.js
	void meshgen_js_pick_folder();
	void meshgen_js_request_zone(const char* zone);
	void meshgen_js_pick_navmesh(const char* destPath);
	void meshgen_js_download(const char* path, const char* name);
}

static Application* s_app = nullptr;
static std::set<std::string> s_files;   // lower case file names in the picked folder
static std::string s_folderName;

namespace WebFiles {

void SetApplication(Application* app) { s_app = app; }
void PickEverQuestFolder() { meshgen_js_pick_folder(); }
bool HasFolder() { return !s_files.empty(); }

bool HasZone(const std::string& zoneShortName)
{
	std::string z = zoneShortName;
	std::transform(z.begin(), z.end(), z.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s_files.count(z + ".s3d") || s_files.count(z + ".eqg");
}

std::string GetFolderDescription()
{
	if (s_files.empty())
		return "No EverQuest folder chosen yet.";
	std::ostringstream ss;
	ss << "Folder: " << s_folderName << " (" << s_files.size() << " files)";
	return ss.str();
}

void RequestZoneFiles(const std::string& zoneShortName) { meshgen_js_request_zone(zoneShortName.c_str()); }
void PickNavMeshFile(const std::string& destPath) { meshgen_js_pick_navmesh(destPath.c_str()); }
void DownloadFile(const std::string& path, const std::string& name) { meshgen_js_download(path.c_str(), name.c_str()); }

} // namespace WebFiles

extern "C" {

// Called by JS after a folder was picked: folder name, then newline separated file names.
EMSCRIPTEN_KEEPALIVE void meshgen_files_ready(const char* folderName, const char* fileList)
{
	s_folderName = folderName ? folderName : "";
	s_files.clear();
	std::istringstream ss(fileList ? fileList : "");
	std::string line;
	while (std::getline(ss, line))
	{
		if (!line.empty())
			s_files.insert(line);
	}
}

// Called by JS after a zone's files were copied into /eq.
EMSCRIPTEN_KEEPALIVE void meshgen_open_zone(const char* zone)
{
	if (s_app && zone)
		s_app->OnZoneFilesReady(zone);
}

// Called by JS after a navmesh file was written.
EMSCRIPTEN_KEEPALIVE void meshgen_navmesh_ready()
{
	if (s_app)
		s_app->OnNavMeshFileReady();
}

}

extern "C" EMSCRIPTEN_KEEPALIVE void meshgen_web_pick_folder()
{
	meshgen_js_pick_folder();
}

// Script hooks (also used by the automated browser test).
extern "C" EMSCRIPTEN_KEEPALIVE void meshgen_web_open_zone(const char* zone, int loadMesh)
{
	if (s_app && zone)
		s_app->RequestZone(zone, loadMesh != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void meshgen_web_build()
{
	if (s_app)
		s_app->BuildNavMesh();
}

extern "C" EMSCRIPTEN_KEEPALIVE int meshgen_web_status()
{
	// 0 = idle/no zone, 1 = zone loaded, 2 = building, 3 = navmesh ready
	return s_app ? s_app->GetWebStatus() : 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void meshgen_web_save()
{
	if (s_app)
		s_app->SaveMeshPublic();
}
