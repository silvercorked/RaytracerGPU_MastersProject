module;

#include <vector>
#include <string>



export module BifurcationPlotter;

/*
	Created a bifurcation diagram of a 1D map
		- originally specifically the logistic map
		- so 1 input var and 1 input parameter
*/

import PrimitiveTypes;
import Bitmap;
import VulkanComputeHelper;

export class BifurcationPlotter {
	u32 sampleSize;
	double minVar;
	double maxVar;
	double minParam;
	double maxParam;
	Bitmap bmp;
	VulkanComputeHelper vulkanCompute;

public:
	BifurcationPlotter();
	~BifurcationPlotter() = default;

	auto setSampleSize(u32)-> void;
	auto setMinAndMaxOfVariable(f64, f64) -> void;
	auto setMinAndMaxOfParameter(f64, f64) -> void;
	auto generate() -> bool;
	auto saveAs(std::string) -> void;

private:

};

BifurcationPlotter::BifurcationPlotter() :
	vulkanCompute()
{
	this->vulkanCompute.mainLoop();
}
