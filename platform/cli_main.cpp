// cli_main.cpp - headless test driver: load a zone, build a navmesh, save it.
// Usage: meshgen-cli <eq dir> <output mq dir> <zone shortname>

#include "meshgen/Application.h"
#include "meshgen/EQConfig.h"
#include "meshgen/InputGeom.h"
#include "meshgen/MapGeometryLoader.h"
#include "meshgen/NavMeshTool.h"
#include "common/NavMesh.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <zone-utilities/log/log_base.h>
#include <zone-utilities/log/log_macros.h>

#include <chrono>
#include <filesystem>
#include <thread>

RecastContext::RecastContext() { m_logger = spdlog::get("Recast"); }
void RecastContext::doLog(const rcLogCategory category, const char* message, const int length)
{
	if (category == RC_LOG_WARNING) m_logger->warn(std::string_view(message, length));
	else if (category == RC_LOG_ERROR) m_logger->error(std::string_view(message, length));
}
void RecastContext::doResetTimers() { for (auto& t : m_accTime) t = std::chrono::nanoseconds(); }
void RecastContext::doStartTimer(const rcTimerLabel label) { m_startTime[label] = std::chrono::steady_clock::now(); }
void RecastContext::doStopTimer(const rcTimerLabel label) { m_accTime[label] += std::chrono::steady_clock::now() - m_startTime[label]; }
int RecastContext::doGetAccumulatedTime(const rcTimerLabel label) const
{
	return (int)std::chrono::duration_cast<std::chrono::microseconds>(m_accTime[label]).count();
}

class EQEmuLogSink : public EQEmu::Log::LogBase
{
public:
	void OnRegister(int) override {}
	void OnUnregister() override {}
	void OnMessage(EQEmu::Log::LogType type, const std::string& message) override
	{
		if (type >= EQEmu::Log::LogWarn) spdlog::warn("[EQEmu] {}", message);
		else spdlog::debug("[EQEmu] {}", message);
	}
};

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		fprintf(stderr, "usage: %s <eq dir> <mq dir> <zone>\n", argv[0]);
		return 1;
	}

	auto logger = spdlog::stdout_color_mt("MeshGen");
	spdlog::set_default_logger(logger);
	spdlog::register_logger(logger->clone("Recast"));
	spdlog::register_logger(logger->clone("EQEmu"));
	spdlog::set_level(spdlog::level::info);
	eqLogInit(-1);
	eqLogRegister(std::make_shared<EQEmuLogSink>());

	EQConfig::s_everquestPath = argv[1];
	EQConfig::s_mq2Path = argv[2];
	EQConfig::s_zonesIni = std::string(SOURCE_DIR) + "/src/resources/Zones.ini";
	std::string zone = argv[3];

	EQConfig config;
	std::filesystem::create_directories(config.GetOutputPath());

	auto navMesh = std::make_shared<NavMesh>(config.GetOutputPath(), zone);
	auto meshTool = std::make_unique<NavMeshTool>(navMesh);
	RecastContext ctx;
	meshTool->setContext(&ctx);
	meshTool->setOutputPath(config.GetOutputPath().c_str());

	auto t0 = std::chrono::steady_clock::now();
	auto geom = std::make_unique<InputGeom>(zone, config.GetEverquestPath());
	auto loader = std::make_unique<MapGeometryLoader>(zone, config.GetEverquestPath(), config.GetOutputPath());
	auto iter = MaxZoneExtents.find(zone);
	if (iter != MaxZoneExtents.end())
		loader->SetMaxExtents(iter->second);

	if (!geom->loadGeometry(std::move(loader), &ctx))
	{
		spdlog::error("Failed to load zone {}", zone);
		return 2;
	}
	auto* ml = geom->getMeshLoader();
	auto t1 = std::chrono::steady_clock::now();
	spdlog::info("Loaded {}: {} verts, {} tris in {} ms", zone, ml->getVertCount(), ml->getTriCount(),
		std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

	meshTool->handleGeometryChanged(geom.get());
	navMesh->SetZoneName(zone);

	meshTool->UpdateTileSizes();
	if (!meshTool->handleBuild())
	{
		spdlog::error("Build failed to start");
		return 3;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	while (meshTool->isBuildingTiles())
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto t2 = std::chrono::steady_clock::now();
	int tw, th, tm;
	meshTool->getTileStatistics(tw, th, tm);
	spdlog::info("Built {} tiles ({}x{}) in {} ms", meshTool->getTilesBuilt(), tw, th,
		std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());

	meshTool->CancelBuildAllTiles(); // joins the build thread
	meshTool->SaveNavMesh();
	spdlog::info("Saved to {}/{}.navmesh", config.GetOutputPath(), zone);
	return 0;
}
