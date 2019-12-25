#ifndef INCLUDE_FLEXIBITY_TLV_HPP
#define INCLUDE_FLEXIBITY_TLV_HPP

namespace Flexibity {

static constexpr uint8_t FOFOhead[] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00};

struct TLV {
	TLV() {
//		decoded_string.str = nullptr;
		memcpy(head, FOFOhead, 8);
	}

	uint8_t head[8];

	struct frame_size {
		uint32_t width;
		uint32_t height;
	} size;

	float fps;

//	struct decoded_string_t {
//		uint32_t length;
//		uint8_t *str;
//	} decoded_string;

	unsigned char* serialize(unsigned char *dst, unsigned char *frame) {
		memcpy(dst, head, 		sizeof(head)); 			dst += sizeof(FOFOhead);
		memcpy(dst, &size, 		sizeof(frame_size)); 	dst += sizeof(frame_size);
		memcpy(dst, &fps, 		sizeof(fps)); 			dst += sizeof(fps);
		memcpy(dst, frame,		size.width * size.height); dst += size.width * size.height;
		return dst;
	}

	unsigned char* deserialize(unsigned char *dst, unsigned char *frame) {
		memcpy(head,			dst, sizeof(head));			dst += sizeof(FOFOhead);
		memcpy(&size, 			dst, sizeof(frame_size));	dst += sizeof(frame_size);
		memcpy(&fps, 			dst, sizeof(fps)); 			dst += sizeof(fps);
		memcpy(frame,			dst, size.width * size.height); dst += size.width * size.height;
		return dst;
	}
};

}

#endif // INCLUDE_FLEXIBITY_TLV_HPP
