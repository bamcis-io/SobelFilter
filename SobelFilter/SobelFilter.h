#pragma warning(disable : 4996)

#ifndef SobelFilter_H
#define SobelFilter_H

#include<vector>
#include<string>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

class SobelFilter
{
private:
	std::vector<cl_platform_id> Platforms;
	std::vector<cl_device_id> Devices;
	cl_mem HorizontalFilter;
	cl_mem VerticalFilter;
	cl_kernel Kernel;
	cl_command_queue Queue;
	cl_context Context;
	cl_program Program;
	cl_int ErrNum;
	cl_mem InputImage;
	cl_mem OutputImage;
	cl_device_id PrimaryDevice;
	cl_uint ImageHeight;
	cl_uint ImageWidth;

	int GetPlatforms();
	int GetDevices();
	int CreateContext(const int platformId);
	int CreateCommandQueue();
	int CreateProgram(const char* fileName);
	int CreateOutputImage();
	int CreateKernel(const std::string kernelName);

public:
	SobelFilter();
	~SobelFilter();
	int Run();
	int LoadImage(const char* fileName);
	int SaveImage(const char* fileName);
};

#endif // !SobelFilter_H

