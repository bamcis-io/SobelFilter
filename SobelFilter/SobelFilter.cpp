#include "SobelFilter.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include "common/FreeImage/include/FreeImage.h"

// Creates a new Wrapper for the OpenCL program with the 
// provided OpenCL kernel filename
SobelFilter::SobelFilter()
{
	this->GetPlatforms();

	this->GetDevices();

	this->CreateContext(0);

	this->CreateCommandQueue();

	this->CreateProgram("SobelFilter.cl");
}

// Destructor to cleanup the SobelFilter object
SobelFilter::~SobelFilter()
{
	if (this->Queue != 0)
	{
		clReleaseCommandQueue(this->Queue);
	}

	if (this->Kernel != 0)
	{
		clReleaseKernel(this->Kernel);
	}

	if (this->Program != 0)
	{
		clReleaseProgram(this->Program);
	}

	if (this->Context != 0)
	{
		clReleaseContext(this->Context);
	}

	if (this->InputImage != nullptr)
	{
		clReleaseMemObject(this->InputImage);
	}

	if (this->OutputImage != nullptr)
	{
		clReleaseMemObject(this->OutputImage);
	}
}

// Gets all available platforms
int SobelFilter::GetPlatforms()
{
	// Declare some temp variables
	cl_int ErrNum;
	cl_uint NumberOfPlatforms;

	// This will retrieve the number of platforms
	ErrNum = clGetPlatformIDs(0, nullptr, &NumberOfPlatforms);

	if (ErrNum != CL_SUCCESS || NumberOfPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL platforms." << std::endl;
		return -1;
	}

	// Allocate space on the heap to hold the platform Ids
	cl_platform_id * PlatformIdsPtr = new cl_platform_id[NumberOfPlatforms];

	// Gets the platform Ids and assign to the above array
	ErrNum = clGetPlatformIDs(NumberOfPlatforms, PlatformIdsPtr, NULL);

	// Add the elements from the array
	for (unsigned int i = 0; i < NumberOfPlatforms; i++)
	{
		this->Platforms.push_back(PlatformIdsPtr[i]);
	}

	// Cleanup pointer
	delete[] PlatformIdsPtr;
	PlatformIdsPtr = nullptr;

	return EXIT_SUCCESS;
}

// Gets all devices on the available platforms
int SobelFilter::GetDevices()
{
	cl_int ErrNum;

	// Will hold the device count for each platform
	cl_uint DeviceCount;

	// Iterate the platforms and get each of their devices
	for (unsigned int i = 0; i < this->Platforms.size(); i++)
	{
		// Gets the number of devices on the platform
		ErrNum = clGetDeviceIDs(
			this->Platforms.at(i),
			CL_DEVICE_TYPE_ALL,
			0,
			nullptr,
			&DeviceCount
		);

		if (ErrNum != CL_SUCCESS || DeviceCount == 0)
		{
			std::cerr << "Could find any devices on platform: " << this->Platforms.at(i) << std::endl;
		}

		// Declare an array on the heap to hold the device Ids
		cl_device_id * DeviceIdsPtr = new cl_device_id[DeviceCount];

		// Get the device Ids
		ErrNum = clGetDeviceIDs(this->Platforms.at(i), CL_DEVICE_TYPE_ALL, DeviceCount, DeviceIdsPtr, nullptr);

		// Add the ids to the vector
		for (unsigned int j = 0; j < DeviceCount; j++)
		{
			this->Devices.push_back(DeviceIdsPtr[j]);
		}

		// Cleanup pointer
		delete[] DeviceIdsPtr;
		DeviceIdsPtr = nullptr;
	}

	return EXIT_SUCCESS;
}

// Creates the OpenCL context
int SobelFilter::CreateContext(const int platformId)
{
	cl_int ErrNum;

	cl_context_properties ContextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)this->Platforms.at(platformId),
		0
	};

	// Attempte to create a GPU context on the platform
	this->Context = clCreateContextFromType(ContextProperties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &ErrNum);

	// If that fails, attempt to create a CPU context
	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Could not create a GPU context, trying CPU..." << std::endl;

		this->Context = clCreateContextFromType(ContextProperties, CL_DEVICE_TYPE_CPU, nullptr, nullptr, &ErrNum);

		if (ErrNum != CL_SUCCESS)
		{
			std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
			return -1;
		}
	}

	return EXIT_SUCCESS;
}

// Creates the OpenCL command queue
int SobelFilter::CreateCommandQueue()
{
	cl_int ErrNum;
	size_t DeviceSizeBuffer = -1;

	ErrNum = clGetContextInfo(this->Context, CL_CONTEXT_DEVICES, 0, nullptr, &DeviceSizeBuffer);

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Failed to call clGetContextInfo(...,CL_CONTEXT_DEVICES,...)" << std::endl;
		return  -1;
	}

	if (DeviceSizeBuffer <= 0)
	{
		std::cerr << "No devices available." << std::endl;
		return -1;
	}

	cl_device_id * Devices = new cl_device_id[DeviceSizeBuffer / sizeof(cl_device_id)];

	ErrNum = clGetContextInfo(this->Context, CL_CONTEXT_DEVICES, DeviceSizeBuffer, Devices, nullptr);

	if (ErrNum != CL_SUCCESS)
	{
		delete[] Devices;
		Devices = nullptr;

		std::cerr << "Failed to get devices Ids." << std::endl;
	}

	this->PrimaryDevice = Devices[0];

	// OpenCL 2.0
	// this->Queue = clCreateCommandQueueWithProperties(this->Context, Devices[0], 0, &ErrNum);

	this->Queue = clCreateCommandQueue(this->Context, Devices[0], 0, &ErrNum);

	if (this->Queue == nullptr)
	{
		delete[] Devices;
		Devices = nullptr;
		std::cerr << "Failed to create command queue for device 0." << std::endl;
		return -1;
	}

	delete[] Devices;
	Devices = nullptr;

	return EXIT_SUCCESS;
}

// Creates the OpenCL program from the .cl file
int SobelFilter::CreateProgram(const char* fileName)
{
	cl_int ErrNum;

	std::ifstream KernelFile(fileName, std::ios::in);

	if (!KernelFile.is_open())
	{
		std::cerr << "Failed to open " << fileName << " for reading." << std::endl;
		return -1;
	}

	std::ostringstream OutputStringStream;

	// Read from the cl file buffer into the output string stream
	OutputStringStream << KernelFile.rdbuf();

	// Convert to std string
	std::string ClSourceContents = OutputStringStream.str();

	// Convert to char*
	const char* ClSourceCharArr = ClSourceContents.c_str();

	this->Program = clCreateProgramWithSource(this->Context, 1, (const char**)&ClSourceCharArr, nullptr, nullptr);

	if (this->Program == nullptr)
	{
		std::cerr << "Failed to create CL program from source." << std::endl;
		return -1;
	}

	// Build the program from the string input contents
	ErrNum = clBuildProgram(this->Program, 0, nullptr, nullptr, nullptr, nullptr);

	if (ErrNum != CL_SUCCESS)
	{
		char BuildLog[16384];

		clGetProgramBuildInfo(this->Program,
			this->PrimaryDevice,
			CL_PROGRAM_BUILD_LOG,
			sizeof(BuildLog),
			BuildLog,
			nullptr
		);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << BuildLog;

		return -1;
	}

	return EXIT_SUCCESS;
}

// This creates the kernel and runs it
int SobelFilter::Run()
{
	cl_int ErrNum;

	ErrNum = this->CreateKernel("SobelFilter");

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create kernel." << std::endl;
		return ErrNum;
	}

	size_t LocalWorkSize[2] = { 1, 1 };

	// Use this to find the maximum local work size
	/*
	ErrNum = clGetKernelWorkGroupInfo(
		this->Kernel,
		this->PrimaryDevice,
		CL_KERNEL_WORK_GROUP_SIZE,
		0,
		&LocalWorkSize,
		nullptr
	);
	*/

	size_t GlobalWorkSize[2] = { 
		this->ImageWidth >= LocalWorkSize[0] ? this->ImageWidth : LocalWorkSize[0],
		this->ImageHeight >= LocalWorkSize[1] ? this->ImageHeight : LocalWorkSize[1] 
	};

	ErrNum = clEnqueueNDRangeKernel(
		this->Queue,
		this->Kernel,
		2,
		nullptr,
		GlobalWorkSize,
		LocalWorkSize,
		0,
		nullptr,
		nullptr
	);

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "**Error queueing kernel for execution: " << ErrNum << std::endl;
		return ErrNum;
	}

	return EXIT_SUCCESS;
}

// Creates an OpenCL kernel with the provided kernel name
int SobelFilter::CreateKernel(const std::string kernelName)
{
	cl_int ErrNum;

	this->Kernel = clCreateKernel(
		this->Program, 
		kernelName.c_str(), 
		&ErrNum
	);

	if (ErrNum != CL_SUCCESS || this->Kernel == nullptr)
	{
		std::cerr << "Failed to create kernel." << std::endl;
		return -1;
	}

	ErrNum = 0;

	// Add the arguments to the kernel
	ErrNum = clSetKernelArg(this->Kernel, 0, sizeof(cl_mem), &this->InputImage);
	ErrNum |= clSetKernelArg(this->Kernel, 1, sizeof(cl_mem), &this->OutputImage);	
	ErrNum |= clSetKernelArg(this->Kernel, 2, sizeof(cl_int), &this->ImageWidth);
	ErrNum |= clSetKernelArg(this->Kernel, 3, sizeof(cl_int), &this->ImageHeight);

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Error setting kernel arguments: " << ErrNum << std::endl;
		return ErrNum;
	}

	return EXIT_SUCCESS;
}

// Loads an image with free image
int SobelFilter::LoadImage(const char* fileName)
{
	// Read in the image with FreeImage
	FREE_IMAGE_FORMAT FiFormat = FreeImage_GetFileType(fileName, 0);
	FIBITMAP* Image = FreeImage_Load(FiFormat, fileName);

	//Convert to 32-bit image
	FIBITMAP* Temp = Image;
	Image = FreeImage_ConvertTo32Bits(Image);
	FreeImage_Unload(Temp);

	// Get image details
	this->ImageHeight = FreeImage_GetHeight(Image);
	this->ImageWidth = FreeImage_GetWidth(Image);
	// Multiply times 4 because we're going from 8 bit to 32 bit
	char * Pixels = new char[4 * this->ImageHeight * this->ImageWidth];

	// Copy the data from the FreeImage bits to our pixel array
	std::memcpy(Pixels, FreeImage_GetBits(Image), 4 * this->ImageHeight * this->ImageWidth);

	FreeImage_Unload(Image);

	cl_int ErrNum;

	cl_channel_order ChannelOrder = CL_RGBA;
	cl_image_format Format;
	Format.image_channel_order = ChannelOrder;
	Format.image_channel_data_type = CL_UNSIGNED_INT8;

	cl_mem_flags Flags = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR;

	// OpenCL 2.0
	/*
	cl_image_desc ImageDescription;
	ImageDescription.image_type = CL_MEM_OBJECT_IMAGE2D;
	ImageDescription.image_width = this->ImageWidth;
	ImageDescription.image_height = this->ImageHeight;
	ImageDescription.buffer = nullptr;
	ImageDescription.image_slice_pitch = 0;
	ImageDescription.image_row_pitch = 0;

	this->InputImage = clCreateImage(
		this->Context,
		Flags,
		&Format,
		&ImageDescription,
		(void *)Pixels,
		&ErrNum
	);
	*/

	this->InputImage = clCreateImage2D(
		this->Context,
		Flags,
		&Format,
		this->ImageWidth,
		this->ImageHeight,
		0,
		Pixels,
		&ErrNum
	);

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Could not create 2D input image." << std::endl;
		return -1;
	}

	return CreateOutputImage();
}

// Creates the output image memory space
int SobelFilter::CreateOutputImage()
{
	cl_int ErrNum;

	// Define our output image
	cl_image_format OutputFormat;
	OutputFormat.image_channel_order = CL_RGBA;
	OutputFormat.image_channel_data_type = CL_UNSIGNED_INT8;

	// OpenCL 2.0
	/*
	cl_image_desc ImageDescription;
	ImageDescription.image_type = CL_MEM_OBJECT_IMAGE2D;
	ImageDescription.image_width = this->ImageWidth;
	ImageDescription.image_height = this->ImageHeight;
	ImageDescription.buffer = nullptr;
	ImageDescription.image_slice_pitch = 0;
	ImageDescription.image_row_pitch = 0;


	this->OutputImage = clCreateImage(
	this->Context,
	CL_MEM_WRITE_ONLY,
	&OutputFormat,
	&ImageDescription,
	nullptr,
	&ErrNum
	);
	*/

	this->OutputImage = clCreateImage2D(
		this->Context,
		CL_MEM_WRITE_ONLY,
		&OutputFormat,
		this->ImageWidth,
		this->ImageHeight,
		0,
		nullptr,
		&ErrNum
	);

	if (ErrNum != CL_SUCCESS)
	{
		std::cerr << "Could not create 2D output image." << std::endl;
		return -1;
	}

	return EXIT_SUCCESS;
}

// Saves an image with FreeImage
int SobelFilter::SaveImage(const char* fileName)
{
	cl_uint ErrNum;

	// Read the output image into a buffer
	char* Buffer = new char[this->ImageWidth * this->ImageHeight * 4];

	// Selects the origin to begin reading from, this is the top left corner
	const size_t Origin[3] = { 0, 0, 0 };
	// This is the region to read, i.e. the whole image
	const size_t Region[3] = { this->ImageWidth, this->ImageHeight, 1 };

	// Read into buffer
	ErrNum = clEnqueueReadImage(
		this->Queue,
		this->OutputImage,
		CL_TRUE,
		Origin,
		Region,
		0,
		0,
		Buffer,
		0,
		nullptr,
		nullptr
	);

	FREE_IMAGE_FORMAT Format = FreeImage_GetFIFFromFilename(fileName);

	// Convert the output buffer into the FreeImage format
	FIBITMAP* Image = FreeImage_ConvertFromRawBits(
		(BYTE*)Buffer,
		this->ImageWidth,
		this->ImageHeight,
		this->ImageWidth * 4,
		32,
		0, // Use 0 for these since the kernel has already converted the pixels to grayscale
		0,
		0,
		false
	);

	// Cleanup the buffer
	delete[] Buffer;
	Buffer = nullptr;

	// Save the image
	return (FreeImage_Save(Format, Image, fileName) == TRUE) ? 0 : -1;
}