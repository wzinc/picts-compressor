#include "Utilities.h"
#include "ifbitstream.h"

// standard quantization matricies
//	http://www.ijg.org
double _dataLuminance[8][8] =
{
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

double _dataChrominance[8][8] =
{
    {17, 18, 24, 27, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
};

void Utilities::RoundSingleDimMat(Mat* mat)
{
	for (int y = 0; y < mat->rows; y++)
	{
		double* row = mat->ptr<double>(y);

		for (int x = 0; x < mat->cols; x++)
			row[x] = round(row[x]);
	}
}

Mat* Utilities::GenerateQuantizationMatricies(double quality)
{
	Mat luminanceOriginal = Mat(8, 8, CV_64FC1, &_dataLuminance),
		chrominanceOriginal = Mat(8, 8, CV_64FC1, &_dataChrominance);
	
	Mat luminance(8, 8, CV_64FC1),
		chrominance(8, 8, CV_64FC1);
	
	luminanceOriginal.copyTo(luminance);
	chrominanceOriginal.copyTo(chrominance);
	
	if (quality > 50.0 && quality < 100.0)
	{
		multiply(luminance, (100.0 - quality) / 50.0, luminance);
		multiply(chrominance, (100.0 - quality) / 50.0, chrominance);
	}
	else if (quality < 50.0)
	{
		multiply(luminance, 50.0 / quality, luminance);
		multiply(chrominance, 50.0 / quality, chrominance);
	}

	RoundSingleDimMat(&luminance);
	RoundSingleDimMat(&chrominance);

	return new Mat[2] { luminance, chrominance };
}

HuffmanTree* Utilities::OpenFile(string filePath, HeaderOptions &header)
{
	ifbitstream inFile(filePath);
	header = HeaderOptions::Deserialize(inFile);
	return HuffmanTree::Deserialize(inFile, header);
}

Mat Utilities::ToMat (HuffmanTree *tree, HeaderOptions *header)
{
	return ToMat(tree, header, 0);
}

Mat Utilities::ToMat (HuffmanTree *tree, HeaderOptions *header, uint8_t maxLayers)
{
	Mat *inImage = tree->ToImage(*header, maxLayers);

	DecompressImage(inImage, header);

	if (maxLayers == 8)
	{
		Mat outputImage = (*inImage)(Rect(0, 0, header->getWidth(), header->getHeight()));

		if (header->getYUVColor())
			cvtColor(outputImage, outputImage, CV_YCrCb2BGR);

		return outputImage;
	}
	
	Mat outputImage(header->getPadHeight() / 8 * maxLayers, header->getPadWidth() / 8 * maxLayers, CV_8UC3, Scalar(255));
	
	for (uint32_t j = 0; j < header->getPadWidth(); j += 8)
		for (uint32_t k = 0; k < header->getPadHeight(); k+= 8)
		{
			Rect sourceRect(j, k, maxLayers, maxLayers);
			Rect destRect(j / 8 * maxLayers, k / 8 * maxLayers, maxLayers, maxLayers);

			(*inImage)(sourceRect).copyTo(outputImage(destRect));
		}

	if (header->getYUVColor())
		cvtColor(outputImage, outputImage, CV_YCrCb2BGR);
	
	return outputImage;
}

void Utilities::DecompressImage(Mat *inImage, HeaderOptions *header)
{
	Mat* quantizationMatricies = GenerateQuantizationMatricies(QUALITY);
	Mat luminance = quantizationMatricies[0],
		chrominance = quantizationMatricies[1];
	
	vector<Mat> inChannels;
	split(*inImage, inChannels);
	uint8_t channelCount = inImage->channels();

	// decompress
	for (int i = 0; i < channelCount; i++)
	{
		Mat currentChannel = inChannels[i];
		for (uint32_t j = 0; j < header->getPadWidth(); j += 8)
			for (uint32_t k = 0; k < header->getPadHeight(); k+= 8)
			{
				Mat currentBlock = currentChannel(Rect(j, k, 8, 8));
				currentBlock.convertTo(currentBlock, CV_64F);

				// if (!i && !j && !k)
				// 	cout << "original: " << endl << currentBlock << endl << "===============================================" << endl;
				
				if (header->getSubtract128())
				{
					subtract(currentBlock, 128.0, currentBlock);
				
					// if (!i && !j && !k)
					// 	cout << "after -128.0: " << endl << currentBlock << endl << "===============================================" << endl;
				}
				
				// dequantization
				if (QUALITY < 100.0)
					multiply(currentBlock, !i ? luminance : chrominance, currentBlock);
				
				// if (!i && !j && !k)
				// 	cout << "after dequantization: " << endl << currentBlock << endl << "===============================================" << endl;
				
				// iDCT
				dct(currentBlock, currentBlock, DCT_INVERSE);

				// if (!i && !j && !k)
				// 	cout << "after iDCT: " << endl << currentBlock << endl << "===============================================" << endl;
				
				// if -128 cli flag
				if (header->getSubtract128())
				{
					add(currentBlock, 128.0, currentBlock);

					// if (!i && !j && !k)
					// 	cout << "after +128: " << endl << currentBlock << endl << "===============================================" << endl;
				}

				RoundSingleDimMat(&currentBlock);

				// if (!i && !j && !k)
				// 	cout << "after _roundSingleDimMat (final): " << endl << currentBlock << endl << "===============================================" << endl;

				currentBlock.copyTo(currentChannel(Rect(j, k, 8, 8)));
			}
	}

	merge(inChannels, *inImage);
	inImage->convertTo(*inImage, CV_8UC3);
}