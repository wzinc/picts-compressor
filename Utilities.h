#ifndef Utilities_h
#define Utilities_h

#include <opencv2/opencv.hpp>
#include <string>

#include "HuffmanTree.h"

#define DEFAULT_QUALITY 50

using namespace std;
using namespace cv;

class Utilities
{
	public:
		static void RoundSingleDimMat(Mat*);
		static Mat* GenerateQuantizationMatricies(double);

		static HuffmanTree* OpenFile(string, HeaderOptions&);
        static HeaderOptions ReadHeader(string filePath);
		static Mat ToMat (HuffmanTree*, HeaderOptions*);
		static Mat ToMat (HuffmanTree*, HeaderOptions*, uint8_t);
		static void DecompressImage(Mat*, HeaderOptions*);

		static double getPSNR(const Mat&, const Mat&);
		static Scalar getMSSIM(const Mat&, const Mat&);

		static string type2str(int);
};

#endif
