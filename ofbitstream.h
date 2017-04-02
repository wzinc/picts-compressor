#ifndef ofbitstream_h
#define ofbitstream_h

#include <fstream>

using namespace std;

class ofbitstream : public ofstream
{
	public:
		ofbitstream(const char *);
		ofbitstream(string);

		void writeBit(uint8_t);
		void writeBits(uint8_t, uint8_t);

		void flush();
		void close();
	
	private:
		uint8_t _currentByte, _location, _bitLength, _byteLength;
};

#endif