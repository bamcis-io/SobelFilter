#ifndef __linux__
#include "stdafx.h"
#else
#include <string.h>
#endif

#include <iostream>
#include <sstream>
#include "SobelFilter.h"

int main(int argc, const char** argv)
{
	std::cout << "Beginning program." << std::endl;

	if (argc < 2)
	{
		std::cout << "Program requires 1 input, the path to the image file." << std::endl;
		return 1;
	}

	std::string Path = argv[1];

	std::cout << "Creating edge detector." << std::endl;

	// Declare the edge detector
	SobelFilter EdgeDetector;

	std::cout << "Loading image" << std::endl;

	// Load the image from the file
	EdgeDetector.LoadImage(Path.c_str());

	std::cout << "Running edge detection" << std::endl;

	// Run it
	EdgeDetector.Run();

	std::cout << "Saving output" << std::endl;

	// Save the edge detected image
	// Use .bmp to make working with FreeImage easy
	std::string OutputPath = Path;

	// Find the last forward slash to get the directory path
	// this won't work for windows style paths
	std::string::size_type LastSlash = OutputPath.rfind('/');

	// If there was a slash, get everything after it, that's the file name
	if (LastSlash != std::string::npos)
	{
		OutputPath = OutputPath.substr(LastSlash + 1, OutputPath.size() - LastSlash + 1);
	}

	// Make the same check for the dot '.' before the extension
	std::string::size_type Dot = OutputPath.rfind('.');

	// Track the extension
	std::string Extension = "";

	if (Dot != std::string::npos)
	{
		Extension = OutputPath.substr(Dot, OutputPath.size() - Dot);
		OutputPath = OutputPath.substr(0, Dot);
	}

	std::ostringstream OutStream;

	// Write out the saved file name with the box dimensions and thresholds
	OutStream << "sobel_" << OutputPath << ".bmp";

	EdgeDetector.SaveImage(OutStream.str().c_str());

	std::cout << "Filter successfully ran and image saved to: " << OutStream.str() << std::endl;
}