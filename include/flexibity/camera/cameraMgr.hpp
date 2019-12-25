#ifndef FLEXIBITY_CAMERA_MGR_INCLUDE_H
#define FLEXIBITY_CAMERA_MGR_INCLUDE_H

#include <memory>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <flexibity/log.h>
#include <jsoncpp/json/json.h>
#include <flexibity/camera/camera.h>
#include <flexibity/camera/image.hpp>
#include <flexibity/camera/tlv.hpp>
#include <flexibity/inject.hpp>
#include <flexibity/base64.hpp>
#include <flexibity/camera/debugServer.hpp>
#include <flexibity/camera/tlv.hpp>
#include <flexibity/camera/recognitionEngine.hpp>

#include <boost/bind.hpp>
#include <zbar.h>

extern bool g_running;

namespace Flexibity {

using namespace std;

class cameraMgr : public logInstanceName
{
public:
	cameraMgr(const Json::Value& cfg, shared_ptr<channelMgr> channelMgr);
	void start();
	virtual ~cameraMgr();
	void inject_callback(const char *data);
private:
	void recoginition_init();
	void recoginition_cleanup();
	void recoginition_loop();


	std::shared_ptr<camera> _cam;
	unsigned char *_buffer;
	unsigned char * _send_buffer;
	unsigned int _send_bufsize;

	boost::thread _cam_thread;
	bool showStat;
	std::weak_ptr<channelMgr> _channelMgr;

	string _forwardTo;

	bool recognition;
	IRecognitionEngine *qr_engine;
	debugServer *_dbg_server = nullptr;
};

cameraMgr::cameraMgr(const Json::Value &cfg, shared_ptr<channelMgr> channelMgr):
		_channelMgr(channelMgr)
{
	ILOG_INITSev(INFO)

	_cam = std::make_shared<camera>();
	_cam->SetParameters(
				cfg.get("fps", 30).asUInt(),
				cfg.get("width", 640).asUInt(),
				cfg.get("height", 480).asUInt(),
				cfg.get("format", "yuyv").asString());

	showStat = cfg.get("showStat", false).asBool();
	_forwardTo = cfg.get("forwardTo", "toDisplay").asString();

	_buffer = new unsigned char[_cam->getW() * _cam->getH()];
	_send_bufsize = _cam->getW() * _cam->getH() + sizeof(TLV);

	_send_buffer = new unsigned char[_send_bufsize];

	_cam->SetDevice(cfg.get("device", "/dev/video0").asString());

	recognition = cfg.get("recognition", false).asBool();
	if(recognition) {
		IDEBUG("QR recognition enabled");
		qr_engine = RecognitionEngineFactory::create(cfg.get("qr_engine", "zbar").asString(), _buffer, _cam->getW(), _cam->getH());
		qr_engine->complete.connect(boost::bind(&cameraMgr::inject_callback, this, _1));
	}

	if(cfg.get("enabled", false).asBool()) {
		if(!_cam->Start()){
			fprintf(stderr, "capture.Start() failed\n");
			return;
		}

		_cam_thread = boost::thread(boost::bind(&cameraMgr::start, this));
		_dbg_server = new debugServer(cfg["debugger"], _send_buffer, _send_bufsize);
	}
}

void cameraMgr::inject_callback(const char *data) {
	IINFO("Decoded: " << data);
	if(auto cm = _channelMgr.lock()){
		Json::Value injParam;
		injParam[inject::channelOption] = _forwardTo;
		injParam[inject::frontOption] = true;
		injParam[inject::dataOption] = "G1swOzBIG1sySg==";
		inject dispClear(injParam, cm);
		dispClear();

		injParam[inject::dataOption] = base64::encode(data);
		IINFO(injParam.toStyledString());
		inject showBarcode(injParam, cm);
		showBarcode();

		injParam.removeMember(inject::dataOption);
		injParam.removeMember(inject::channelOption);
		injParam[inject::textOption] = "ok\n";
		injParam[inject::portOption] = "nfcDevice";
		inject beep(injParam, cm);
		beep();
	}
}

void cameraMgr::start()
{
	IINFO("thread started");
	while(g_running) {
		int frame_counter = _cam->getFrameCounter();
		const camera::frame* frame = _cam->getFrame(20000);

		if (frame && frame->length != 0) {

			float fps = _cam->getFPSCnt();
			if(showStat)
				fprintf(stderr, "\e[2K\rFrame: %i, Fps: %f, Bps: %f, AvgFrameSize: %f", frame_counter, fps, _cam->getBps(), _cam->getAvgFrameSize());

			// TODO: Debug video output through socket/pipe
			if(recognition) {
				TLV tlv;
				tlv.fps = fps;
				tlv.size.width = _cam->getW();
				tlv.size.height = _cam->getH();
				ConvertYUYVToGrayscale(_cam->getW(), _cam->getH(), frame->data, _buffer);
				unsigned char * b = tlv.serialize(_send_buffer, _buffer);

				qr_engine->recognition_loop();
				_dbg_server->poll();
			}
		}
	}
	IINFO("thread ended");
}

cameraMgr::~cameraMgr() {
	_cam_thread.join();

	delete qr_engine;
	delete[] _buffer;
	delete[] _send_buffer;
	delete _dbg_server;
}


} // namespace Flexibity

#endif // FLEXIBITY_CAMERA_MGR_INCLUDE_H
