const sampler_t Sampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP |
							CLK_FILTER_NEAREST;

/*
// These are not actually needed, but included for reference
__constant int HorizontalFilter[9] = {
	1, 0, -1,
	2, 0, -2,
	1, 0, -1
};

__constant int VerticalFilter[9] = {
	1, 2, 1,
	0, 0, 0,
	-1, -2, -1
};
*/

// Runs the Sobel Filter on an image, this expects the image to be read in
// using unsigned integers to represent the RGB channels and should be between
// 0 and 255. This means the format CL_UNSIGNED_INT8 should be used when creating
// the image2d_t source and output
__kernel void SobelFilter(
	__read_only image2d_t sourceImage, 
	__write_only image2d_t outputImage,
	int width,
	int height
	)
{ 
	// This is the currently focused pixel and is the output pixel
	// location
	int2 ImageCoordinate = (int2)(get_global_id(0), get_global_id(1));

	// Make sure we are within the image bounds
	if (ImageCoordinate.x < width && ImageCoordinate.y < height)
	{ 		
		int x = ImageCoordinate.x;
		int y = ImageCoordinate.y;

		// Read the 8 pixels around the currently focused pixel
		uint4 Pixel00 = read_imageui(sourceImage, Sampler, (int2)(x - 1, y - 1));
		uint4 Pixel01 = read_imageui(sourceImage, Sampler, (int2)(x, y - 1));
		uint4 Pixel02 = read_imageui(sourceImage, Sampler, (int2)(x + 1, y - 1));

		uint4 Pixel10 = read_imageui(sourceImage, Sampler, (int2)(x - 1, y));
		uint4 Pixel12 = read_imageui(sourceImage, Sampler, (int2)(x + 1, y));

		uint4 Pixel20 = read_imageui(sourceImage, Sampler, (int2)(x - 1, y + 1));
		uint4 Pixel21 = read_imageui(sourceImage, Sampler, (int2)(x, y + 1));
		uint4 Pixel22 = read_imageui(sourceImage, Sampler, (int2)(x + 1, y + 1));

		// This is equivalent to looping through the 9 pixels
		// under this convolution and applying the appropriate
		// filter, here we've already applied the filter coefficients
		// since they are static
		uint4 Gx = Pixel00 + (2 * Pixel10) + Pixel20 -
				Pixel02 - (2 * Pixel12) - Pixel22;

		uint4 Gy = Pixel00 + (2 * Pixel01) + Pixel02 -
				Pixel20 - (2 * Pixel21) - Pixel22;
		
		// Holds the output RGB values
		uint4 OutColor = (uint4)(0, 0, 0, 1);

		// Compute the gradient magnitude
		OutColor.x = sqrt((float)(Gx.x * Gx.x + Gy.x * Gy.x)); // R
		OutColor.y = sqrt((float)(Gx.y * Gx.y + Gy.y * Gy.y)); // G
		OutColor.z = sqrt((float)(Gx.z * Gx.z + Gy.z * Gy.z)); // B

		// Adjust all of the RGB values to not be more than 255
		if (OutColor.x > 255)
		{
			OutColor.x = 255;
		}

		if (OutColor.y > 255)
		{
			OutColor.y = 255;
		}

		if (OutColor.z > 255)
		{
			OutColor.z = 255;
		}
		
		// Convert to grayscale using luminosity method
		uint Gray = (OutColor.x * 0.2126) + (OutColor.y * 0.7152) + (OutColor.z * 0.0722);

		// Write the RGB value to the output image
		write_imageui(outputImage, ImageCoordinate, (uint4)(Gray, Gray, Gray, 0));
	}		
}


/*
// Alternative way to generate Gx and Gy
int2 StartImageCoordinate = (int2)(ImageCoordinate.x - 1, ImageCoordinate.y - 1);
int2 EndImageCoordinate = (int2)(ImageCoordinate.x + 1, ImageCoordinate.y + 1);

uint4 Gx = (uint4)(0, 0, 0, 0);
uint4 Gy = (uint4)(0, 0, 0, 0);

int Index = 0;

for (int y = StartImageCoordinate.y; y < EndImageCoordinate.y; y++)
{
	for (int x = StartImageCoordinate.x; x < EndImageCoordinate.x; x++)
	{ 
		uint4 Pixel = read_imageui(sourceImage, Sampler, (int2)(x, y));
		Gx += Pixel * HorizontalFilter[Index];
		Gy += Pixel * VerticalFilter[Index];

		Index++;
	}
}
*/