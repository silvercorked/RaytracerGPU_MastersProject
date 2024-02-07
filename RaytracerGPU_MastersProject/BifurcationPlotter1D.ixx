module;

#include <vector>
#include <string>



export module BifurcationPlotter1D;

/*
	Created a bifurcation diagram of a 1D map
		- originally specifically the logistic map
		- so 1 input var and 1 input parameter
*/

import PrimitiveTypes;
import Bitmap;
import VulkanComputeHelper;

export class BifurcationPlotter1D {
	u32 sampleSize;
	double minVar;
	double maxVar;
	double minParam;
	double maxParam;
	Bitmap bmp;
	VulkanComputeHelper vulkanCompute;

public:
	BifurcationPlotter1D();
	~BifurcationPlotter1D() = default;

	auto setSampleSize(u32)-> void;
	auto setMinAndMaxOfVariable(f64, f64) -> void;
	auto setMinAndMaxOfParameter(f64, f64) -> void;
	auto generate() -> bool;
	auto saveAs(std::string) -> void;

private:

};

BifurcationPlotter1D::BifurcationPlotter1D() :
	vulkanCompute()
{
	this->vulkanCompute.mainLoop();
}
