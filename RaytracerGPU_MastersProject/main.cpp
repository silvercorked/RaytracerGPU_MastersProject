
#include "Config.hpp"

#include "LogisticMap.hpp"
//#include "Raytracer.hpp"
#include "RaytracerBVH.hpp"

int main() {
	if constexpr (Config::CurrentProgram == Config::Programs::LogisticMap) {
		LogisticMapRenderer::LogisticMap comp{};
		comp.mainLoop();
	}
	//else if constexpr (Config::CurrentProgram == Config::Programs::Raytracer) {
	//	RaytracerRenderer::Raytracer comp{};
	//	comp.mainLoop();
	//}
	else if constexpr (Config::CurrentProgram == Config::Programs::RaytracerBVH) {
		RaytracerBVHRenderer::Raytracer comp{};
		comp.mainLoop();
	}
}
