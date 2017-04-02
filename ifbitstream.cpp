#import "ifbitstream.h"

ifbitstream::ifbitstream(const char * fileName)
	: _currentByte(0), _location(8), _bitLength(sizeof(_currentByte) * 8), _byteLength(sizeof(_currentByte)), ifstream(fileName, ifstream::binary | ifstream::in) { }

ifbitstream::ifbitstream(string fileName) : ifbitstream(fileName.c_str()) {  }

uint8_t ifbitstream::readBit()
{
	return readBits(1);
}

uint8_t ifbitstream::readBits(uint8_t length)
{
	if (length > 8) throw "Langth cannot be >8.";

	uint8_t value = 0;

	for (uint8_t i = 0; i < length; i++)
	{
		if (_location == _bitLength)
		{
			read((char*)&_currentByte, 1);
			_location = 0;
		}

		value = (value << 1) | ((_currentByte >> (_bitLength - _location - 1)) & 0x01);
		_location++;
	}

	return value;
}

void ifbitstream::skipByte()
{
	// flush rest of byte
	readBits(_bitLength - _location);
}