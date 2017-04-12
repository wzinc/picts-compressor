#ifndef HeaderOptions_h
#define HeaderOptions_h

#include <iostream>
#include <stdint.h>
#include <opencv2/opencv.hpp>

using namespace std;

class HeaderOptions
{
	public:
		static HeaderOptions Deserialize (istream&);

		void Serialize(ostream&);

		uint32_t getWidth() { return _width; }
		uint32_t getHeight() { return _height; }

		uint32_t getPadWidth() { return _padWidth; }
		uint32_t getPadHeight() { return _padHeight; }

		bool getHuffmanCoding() { return _huffmanCoding; }
		bool getSubtract128() { return _subtract128; }
		bool getYUVColor() { return _yuvColor; }

		uint8_t getLayerCount() { return _layerCount; }
		uint8_t getQuality() { return _quailty; }

		void setWidth(uint32_t width) { _width = width; }
		void setHeight(uint32_t height) { _height = height; }

		void setPadWidth(uint32_t padWidth) { _padWidth = padWidth; }
		void setPadHeight(uint32_t padHeight) { _padHeight = padHeight; }

		void setYUVColor(bool yuvColor) { _yuvColor = yuvColor; }
		void setSubtract128(bool subtract128) { _subtract128 = subtract128; }
		void setHuffmanCoding(bool huffmanCoding) { _huffmanCoding = huffmanCoding; }

		void setLayerCount(uint8_t layerCount) { _layerCount = layerCount; }
		void setQuality(uint8_t quailty) { _quailty = quailty; }

	private:
		uint32_t _width, _height, _padWidth, _padHeight;
		bool _yuvColor, _subtract128, _huffmanCoding;
		uint8_t _layerCount, _quailty;
};

#endif