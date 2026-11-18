// web_files.h - bridge between the mesh generator and the browser's file APIs.
#pragma once

#include <string>

class Application;

namespace WebFiles {

void SetApplication(Application* app);

// Opens the browser's folder picker for the EverQuest folder.
void PickEverQuestFolder();

// True once a folder was picked, and whether it holds a zone's files.
bool HasFolder();
bool HasZone(const std::string& zoneShortName);
std::string GetFolderDescription();

// Copies a zone's files from the picked folder into /eq, then calls
// Application::OnZoneFilesReady.
void RequestZoneFiles(const std::string& zoneShortName);

// Asks for a .navmesh file and writes it to destPath, then calls
// Application::OnNavMeshFileReady.
void PickNavMeshFile(const std::string& destPath);

// Offers a file from the virtual file system as a browser download.
void DownloadFile(const std::string& path, const std::string& downloadName);

} // namespace WebFiles
