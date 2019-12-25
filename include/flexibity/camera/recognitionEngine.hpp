#ifndef FLEXIBITY_CAMERA_RECOGNITION_ENGINE_INCLUDE_HPP
#define FLEXIBITY_CAMERA_RECOGNITION_ENGINE_INCLUDE_HPP

#include <flexibity/log.h>
#include <zbar.h>
#ifdef __cplusplus
	extern "C" {
#		include <quirc.h>
#		include <quirc_internal.h>
	}
#endif

#include <boost/signals2.hpp>

namespace Flexibity {
	using RecognitionCallback = boost::signals2::signal<void (const char*)>;
	using RecognitionCallback_Slot = RecognitionCallback::slot_type;

	class IRecognitionEngine : public logInstanceName{
	public:
		RecognitionCallback complete;

		IRecognitionEngine(unsigned char* buffer, unsigned int width, unsigned int height) : _buffer(buffer), _width(width), _height(height) {
			ILOG_INITSev(INFO)
		}

		virtual ~IRecognitionEngine() {
		}
		virtual void recognition_loop() = 0;

	protected:
		unsigned char * _buffer;
		unsigned int _width, _height;
	};

	class ZbarEngine : public IRecognitionEngine {
	public:
		ZbarEngine(unsigned char* buffer, unsigned int width, unsigned int height): IRecognitionEngine(buffer, width, height) {
			init();
		}

		virtual ~ZbarEngine() {
			cleanup();
		}

		void recognition_loop() override {
			int n = zbar::zbar_scan_image(scanner, image);

			/* extract results */
			const zbar::zbar_symbol_t *symbol = zbar::zbar_image_first_symbol(
					image);
			for (; symbol; symbol = zbar::zbar_symbol_next(symbol)) {
				/* do something useful with results */
				zbar::zbar_symbol_type_t typ = zbar::zbar_symbol_get_type(symbol);

				if(typ == zbar::ZBAR_QRCODE){
					const char *data = zbar::zbar_symbol_get_data(symbol);
					complete(data);
				}
			}
		}
	protected:
		void cleanup() {
			zbar::zbar_image_destroy(image);
			zbar::zbar_image_scanner_destroy(scanner);
		}

		void init() {
			scanner = zbar::zbar_image_scanner_create();

			/* configure the reader */
			zbar::zbar_image_scanner_set_config(scanner, zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
			zbar::zbar_image_scanner_set_config(scanner, zbar::ZBAR_NONE, zbar::ZBAR_CFG_X_DENSITY, 2);
			zbar::zbar_image_scanner_set_config(scanner, zbar::ZBAR_NONE, zbar::ZBAR_CFG_Y_DENSITY, 2);

			/* wrap image data */
			image = zbar::zbar_image_create();
			zbar::zbar_image_set_format(image, zbar_fourcc('G','R','E','Y'));

			zbar::zbar_image_set_size(image, _width, _height);
			zbar::zbar_image_set_data(image, _buffer, _width * _height, nullptr);
		}
	private:
		zbar::zbar_image_scanner_t *scanner = nullptr;
		zbar::zbar_image_t *image = nullptr;
	};

	class QuircEngine : public IRecognitionEngine {
	public:
		QuircEngine(unsigned char *buffer, unsigned int width, unsigned int height) : IRecognitionEngine(buffer, width, height) {
			init();
		}

		virtual ~QuircEngine() {
			cleanup();
		}

		virtual void recognition_loop() override {
			my_quirc_begin(_qr);
			quirc_end(_qr);


			int count = quirc_count(_qr);
			for (int i = 0; i < count; i++) {
				quirc_code code;
				quirc_data data;

				quirc_extract(_qr, i, &code);
				if (!quirc_decode(&code, &data))
				{
					printf("==> \%d %s\n", data.payload_len , data.payload);
					printf("    Version: %d, ECC: %c, Mask: %d, Type: %d\n\n",
						   data.version, "MLHQ"[data.ecc_level],
						   data.mask, data.data_type);
				}
			}
		}
	private:
		void init() {
			_qr = new quirc;
			// instead of quirc_resize
			_qr->w = _width;
			_qr->h = _height;
			_qr->image = _buffer;
		}

		void cleanup() {
			// we dont want to clear image buffer in quirc_destroy
			delete _qr;
		}

		// our quirc_begin analog, because original has strange signature
		void my_quirc_begin(quirc *q) {
			q->num_regions = QUIRC_PIXEL_REGION;
			q->num_capstones = 0;
			q->num_grids = 0;
		}

		quirc *_qr = nullptr;
	};

	class RecognitionEngineFactory {
	public:
		static IRecognitionEngine * create(const std::string &engine, unsigned char *buffer, unsigned int width, unsigned int height ) {
			if (engine == "zbar")
				return new ZbarEngine(buffer, width, height);
			if (engine == "quirc")
				return new QuircEngine(buffer, width, height);
		}
	};
}

#endif // FLEXIBITY_CAMERA_RECOGNITION_ENGINE_INCLUDE_HPP
