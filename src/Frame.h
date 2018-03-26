#ifndef _FRAME_H_
#define _FRAME_H_

#include <cstdint>

class Frame {
public:
	uint8_t * dataPointer;
	int dataSize;
	int width;
	int height;
	int pitch;
	int frameID;
	~Frame()
	{
		delete dataPointer;
	}
};

#endif // _FRAME_H_