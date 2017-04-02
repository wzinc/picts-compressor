#ifndef Utilities_h
#define Utilities_h

#include <opencv2/opencv.hpp>
#include <string>

#include "HuffmanTree.h"

#define QUALITY 50.0

using namespace std;
using namespace cv;

class Utilities
{
	public:
		static void RoundSingleDimMat(Mat*);
		static Mat* GenerateQuantizationMatricies(double);

		static HuffmanTree* OpenFile(string, HeaderOptions&);
		static Mat ToMat (HuffmanTree*, HeaderOptions*);
		static Mat ToMat (HuffmanTree*, HeaderOptions*, uint8_t);
		static void DecompressImage(Mat*, HeaderOptions*);
};

#endif