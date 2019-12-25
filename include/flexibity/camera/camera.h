//Ivan Matveev 2014

#ifndef CAMERA_H_
#define CAMERA_H_

#include <stdint.h>
#include <linux/videodev2.h>

#include <vector>
#include <string>

using namespace std;

namespace Flexibity{

class camera{
public:
	struct frame{
		frame():
			data(nullptr),
			length(0){};
		uint8_t* data;
		size_t length;
	};

	struct fmtMapItem{
		const char * fmtStr;
		__u32 fmtV4L2;
	};

	static constexpr fmtMapItem fmtMap[] = {
			{"yuyv", V4L2_PIX_FMT_YUYV},
			{"mjpeg", V4L2_PIX_FMT_MJPEG},
			{"h264", V4L2_PIX_FMT_H264},
	};

public:
	camera();
	~camera();

	void SetParameters(uint32_t fps, uint32_t w, uint32_t h, const string& format){
		fps_ = fps;
		w_ = w;
		h_ = h;
		if(validateFormat(format))
			format_ = format;
	}

	bool Start();
	void Stop();

	uint32_t getW(){
		return w_;
	}

	uint32_t getH(){
		return h_;
	}

	uint32_t getFPS(){
		return fps_;
	}

	float getFPSCnt(){
		if ( fps_isochr_cnt_ >= 1.f )
			return fps_isochr_cnt_;
		else
			return fps_instant_cnt_;
	}

	uint32_t getFrameCounter(){
		return frame_cntr_;
	}

	float getBps(){
		if ( bps_isochr >= 1.f )
			return bps_isochr;
		else
			return bps_instant;
	}

	float getAvgFrameSize(){
		return avg_frame_size;
	}

	void SetDevice(const string& device) {
		device_name_ = device;
	}

	const frame* getFrame(int timeout_ms);

private:

	bool validateFormat(const string& format){
		auto fmtMapSize = sizeof (fmtMap)/sizeof(*fmtMap);
		for(int i = 0; i < fmtMapSize; i++ ){
			if (format == fmtMap[i].fmtStr)
				return true;
		}
		return false;
	}

	__u32 formatToV4L2(const string& format){
		auto fmtMapSize = sizeof (fmtMap)/sizeof(*fmtMap);
		for(int i = 0; i < fmtMapSize; i++ ){
			if (format == fmtMap[i].fmtStr)
				return fmtMap[i].fmtV4L2;
		}

		return fmtMap[0].fmtV4L2;
	}

	string formatFromV4L2(__u32 format){
		auto fmtMapSize = sizeof (fmtMap)/sizeof(*fmtMap);
		for(int i = 0; i < fmtMapSize; i++ ){
			if (format == fmtMap[i].fmtV4L2)
				return fmtMap[i].fmtStr;
		}

		return "unknown";
	}

	bool DeviceOpen();
	bool DeviceCheckCapabilities();
	bool DeviceSetCaptureMode();
	bool DeviceCaptureStart();
	void DeviceCaptureStop();
	bool DeviceBufferEnqueue();
	bool findFilledBuffer();
	const frame* DeviceBufferDequeue();
	void DeviceClose();

	bool BufferInit();
	void BufferUninit();

	bool WaitPacket(int timeout_ms);

	bool CheckSanity(){
		if(device_fh_ != -1)
			return false;

		return true;
	}

private:
	std::string device_name_;
	int device_fh_;
	uint32_t fps_;  //frame per sec AKA framerate
	uint32_t w_;
	uint32_t h_;
	string format_;

	std::vector<frame*> buffer_;
	struct v4l2_buffer buf_;
	timeval lastFrameTime_;
	timeval isochrFrameTime_;
	float fps_instant_cnt_;
	float fps_isochr_cnt_;
	float bps_instant;
	float bps_isochr;
	float avg_frame_size;
	int frame_cntr_;
	int last_frame_cntr_;
	uint32_t bytes_count_;
	uint32_t last_bytes_count_;
};

}

#endif //#ifndef CAMERA_H_
