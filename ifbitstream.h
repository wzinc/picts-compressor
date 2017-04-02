#ifndef ifbitstream_h
#define ifbitstream_h

#include <fstream>

using namespace std;

class ifbitstream : public ifstream
{
	public:
		ifbitstream(const char *);
		ifbitstream(string);

		uint8_t readBit();
		uint8_t readBits(uint8_t);

		void skipByte();

	private:
		uint8_t _currentByte, _location, _bitLength, _byteLength;
};

#endif