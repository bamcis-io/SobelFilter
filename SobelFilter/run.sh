#!/bin/bash

echo "Compiling SobelFilter"
g++ -c SobelFilter.cpp -lOpenCL -Icommon/OpenCL/include -Icommon/FreeImage/include -std=c++11 -o SobelFilter.a

echo "Compiling EdgeDetector"
g++ RunSobel.cpp SobelFilter.a common/FreeImage/lib/linux/x86_64/libfreeimage.a -lOpenCL -Icommon/OpenCL/include -std=c++11 -o sobel

./sobel lena.bmp
./sobel valve.png
./sobel bacteria.jpg