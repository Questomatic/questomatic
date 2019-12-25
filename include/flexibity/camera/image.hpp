#ifndef FLEXIBITY_CAMERA_IMAGE_INCLUDE_HPP
#define FLEXIBITY_CAMERA_IMAGE_INCLUDE_HPP

namespace Flexibity {

void ConvertYUYVToGrayscale(unsigned int width, unsigned int height, unsigned char* src, unsigned char* dst)
{
	unsigned char *ptr = src;
	unsigned int size = width * height;
	for (unsigned int i = 0; i < size; i+=4) {
		dst[i] = *ptr;
		ptr += 2;
		dst[i+1] = *ptr;
		ptr += 2;
		dst[i+2] = *ptr;
		ptr += 2;
		dst[i+3] = *ptr;
		ptr += 2;
	}
}

}
#endif
