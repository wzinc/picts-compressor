#include <iostream>

#include "HeaderOptions.h"

using namespace std;

void HeaderOptions::Serialize (ostream& outputStream)
{
	outputStream.write("PICTS", 5);

	uint8_t flags = (_layerCount & 0x0f) | (_yuvColor << 7 & 0x80) | (_huffmanCoding << 6 & 0x40) | (_subtract128 << 5 & 0x20);

	outputStream.write(reinterpret_cast<const char*>(&flags), 1);
	outputStream.write(reinterpret_cast<const char*>(&_width), sizeof(_width));
	outputStream.write(reinterpret_cast<const char*>(&_height), sizeof(_height));
	outputStream.write(reinterpret_cast<const char*>(&_padWidth), sizeof(_padWidth));
	outputStream.write(reinterpret_cast<const char*>(&_padHeight), sizeof(_padHeight));
}

HeaderOptions HeaderOptions::Deserialize (istream& inputStream)
{
	// file must be at least 14 bytes long
	inputStream.seekg(0, inputStream.end);
	int length = inputStream.tellg();

	if (length < 14)
		throw "File too short.";

	inputStream.seekg(0, inputStream.beg);

	char fileID[6];
	inputStream.read(fileID, 5);
	fileID[5] = '\0';	

	if (string("PICTS").compare(fileID))
		throw "Not a PICTS file.";

	uint8_t flags;
	inputStream.read((char*)&flags, 1);

	HeaderOptions options;
	inputStream.read((char*)&options._width, sizeof(options._width));
	inputStream.read((char*)&options._height, sizeof(options._height));

	inputStream.read((char*)&options._padWidth, sizeof(options._padWidth));
	inputStream.read((char*)&options._padHeight, sizeof(options._padHeight));

	options._layerCount = flags & 0x0f;
	options._yuvColor = (flags & 0x80) > 0;
	options._huffmanCoding = (flags & 0x40) > 0;
	options._subtract128 = (flags & 0x20) > 0;

	return options;
}