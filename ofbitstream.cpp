#include "ofbitstream.h"

ofbitstream::ofbitstream(const char * fileName)
	: _currentByte(0), _location(0), _bitLength(sizeof(_currentByte) * 8), _byteLength(sizeof(_currentByte)), ofstream(fileName, ofstream::binary | ofstream::out) { }

ofbitstream::ofbitstream(string fileName) : ofbitstream(fileName.c_str()) { }

void ofbitstream::writeBit(uint8_t value) { writeBits(value, 1); }

void ofbitstream::writeBits(uint8_t value, uint8_t length)
{
	if (length > 8) throw "Langth cannot be >8.";

	for (uint8_t i = 0; i < length; i++)
	{
		if (_location == _bitLength)
		{
			write((char*)&_currentByte, _byteLength);
			_currentByte = 0;
			_location = 0;
		}

		_currentByte = (_currentByte << 1) | (value >> (length - (i + 1)) & 0x1);
		_location++;
	}
}

void ofbitstream::flush()
{
	// only write last byte if the location is >0
	//	user is responsible to know if the last usable bit
	if (_location)
	{
		// shift last byte to end
		_currentByte <<= (_bitLength - _location);
		ofstream::write((char*)&_currentByte, 1);
		_currentByte = _location = 0;
	}
	
	ofstream::flush();
}

void ofbitstream::close()
{
	flush();
	ofstream::close();
}