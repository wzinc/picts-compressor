#include <iostream>
#include <algorithm>
#include <map>
#include <opencv2/opencv.hpp>

#include "HeaderOptions.h"
#include "Parameters.h"
#include "HuffmanTree.h"
#include "ofbitstream.h"
#include "Utilities.h"

using namespace std;
using namespace cv;

// double dataTest[8][8] =
// {
// 	{154, 123, 123, 123, 123, 123, 123, 136},
// 	{192, 180, 136, 154, 154, 154, 136, 110},
// 	{254, 198, 154, 154, 180, 154, 123, 123},
// 	{239, 180, 136, 180, 180, 166, 123, 123},
// 	{180, 154, 136, 167, 166, 149, 136, 136},
// 	{128, 136, 123, 136, 154, 180, 198, 154},
// 	{123, 105, 110, 149, 136, 136, 180, 166},
// 	{110, 136, 123, 123, 123, 136, 154, 136}
// };

int main (int argc, char** argv)
{
	// parse CLI input
	Parameters parameters = Parameters::ParseCommandLine(argc, argv);
	// cout << "Parameters: " << parameters << endl;

	// open file / read into cv:Mat
	// file has been stat'd at this point, so we know it exists
	Mat inputImage = imread(parameters.InputFileName, CV_LOAD_IMAGE_COLOR);

	if (!inputImage.data)
	{
		cerr << "Error reading image file: " << parameters.InputFileName << endl;
		exit(1);
	}

	// start header
	uint32_t width = inputImage.size().width,
			 height = inputImage.size().height;
	
	HeaderOptions options = HeaderOptions();
	options.setWidth(width);
	options.setHeight(height);

	options.setYUVColor(parameters.YUVConversion);
	options.setSubtract128(parameters.Subtract128);
	options.setHuffmanCoding(parameters.HuffmanCoding);
	options.setLayerCount(8);

	// cout << "width: " << options.getWidth() << endl;
	// cout << "height: " << options.getHeight() << endl;

	// pad image, if necessary
	int padWidth = width % 8 ? 8 - (width % 8) : 0,
		padHeight = height % 8 ? 8 - (height % 8) : 0;
	
	if (padWidth || padHeight)
	{
		// copy to padded Mat & replicate last pixel to border
		Mat paddedImage = Mat(Size(width + padWidth, height + padHeight), inputImage.type());

		inputImage.copyTo(paddedImage(Rect(0, 0, width, height)));

		Mat row = inputImage.row(height - 1);
		for (uint32_t i = height; i < height + padHeight; i++)
			// if needs height, replicate last image row to outer padded row
			row.copyTo(paddedImage(Rect(0, i, width, 1)));

		Mat col = inputImage.col(width - 1);
		for (uint32_t i = width; i < width + padWidth; i++)
			// if needs width, replicate last image col to outer padded col
			col.copyTo(paddedImage(Rect(i, 0, 1, height)));

		// fill-in corner with last (H - 1 x W - 1) pixel
		Vec3b cornerColor = inputImage.at<Vec3b>(Point(width - 1, height - 1));
		for (uint32_t w = width; w < (width + padWidth); w++)
			for (uint32_t h = height; h < (height + padHeight); h++)
				paddedImage.at<Vec3b>(Point(w, h)) = cornerColor;

		inputImage = paddedImage;
	}

	padWidth += width;
	padHeight += height;

	options.setPadWidth(padWidth);
	options.setPadHeight(padHeight);

	// cout << "width: " << width << " | " << padWidth << endl;
	// cout << "height: " << height << " | " << padHeight << endl;

	// convert color to YUV
	if (parameters.YUVConversion)
		cvtColor(inputImage, inputImage, CV_BGR2YCrCb);
	
	// convert to signed float for subtract & DCT
	inputImage.convertTo(inputImage, CV_64FC1);

	// get direct access to channels
	vector<Mat> channels;
	split(inputImage, channels);
	short channelCount = inputImage.channels();

	Mat* quantizationMatricies = Utilities::GenerateQuantizationMatricies(QUALITY);
	Mat luminance = quantizationMatricies[0],
		chrominance = quantizationMatricies[1];
	
	// divide into 8x8 blocks
	for (int i = 0; i < channelCount; i++)
	{
		Mat currentChannel = channels[i];
		for (uint32_t j = 0; j < padWidth; j += 8)
			for (uint32_t k = 0; k < padHeight; k+= 8)
		{
			Mat currentBlock = currentChannel(Rect(j, k, 8, 8));

			// if (!i && !j && !k)
			// 	cout << "original: " << endl << currentBlock << endl << "===============================================" << endl;

			// if -128 cli flag
			if (parameters.Subtract128)
			{
				subtract(currentBlock, 128.0, currentBlock);

				// if (!i && !j && !k)
				// 	cout << "after -128: " << endl << currentBlock << endl << "===============================================" << endl;
			}

			// DCT
			dct(currentBlock, currentBlock);

			// if (!i && !j && !k)
			// 	cout << "after DCT: " << endl << currentBlock << endl << "===============================================" << endl;
			
			// 80% quantization
			if (QUALITY < 100.0)
				divide(currentBlock, !i ? luminance : chrominance, currentBlock);

			// if (!i && !j && !k)
			// 	cout << "after quantization: " << endl << currentBlock << endl << "===============================================" << endl;

			Utilities::RoundSingleDimMat(&currentBlock);
			
			if (!i && !j && !k)
				cout << "after _roundSingleDimMat: " << endl << currentBlock << endl << "===============================================" << endl;
			exit(1);
			
			// if -128 cli flag, add it back now
			if (parameters.Subtract128)
			{
				add(currentBlock, 128.0, currentBlock);
			
				// if (!i && !j && !k)
				// 	cout << "after +128.0: " << endl << currentBlock << endl << "===============================================" << endl;
			}

			currentBlock.convertTo(currentBlock, CV_8U);
			currentBlock.convertTo(currentBlock, CV_64F);

			if (!i && !j && !k)
				cout << "after convertTo (final): " << endl << currentBlock << endl << "===============================================" << endl;

			currentBlock.copyTo(currentChannel(Rect(j, k, 8, 8)));
		}
	}

	Mat outputImage;
	merge(channels, outputImage);
	outputImage.convertTo(outputImage, CV_8UC3);

	HuffmanTree* tree;
	tree = HuffmanTree::FromImage(&outputImage, options.getLayerCount());

	ofbitstream file(parameters.OutputFileName);

	cout << parameters.OutputFileName << "\t" << options.getWidth() << "\t" << options.getHeight() << "\t";

	// write header
	options.Serialize(file);
	// write trees & layers
	for (uint8_t i = 0; i < options.getLayerCount(); i++)
	{
		uint64_t treeSize = tree->SerializeTree(file, i);
		uint64_t layerSize = tree->SerializeLayer(file, i);
		cout << (treeSize + layerSize) << "\t";
		// cout << "layer[" << (int)i << "] - tree: " << treeSize << " + layer: " << layerSize << " = " << (treeSize + layerSize) << " B" << endl;
	}

	cout << endl;

	file.close();
	// delete tree;

	// for (uint8_t layer = 1; layer <= options.getLayerCount(); layer++)
	// {
	// 	HeaderOptions inHeader;
	// 	tree = Utilities::OpenFile(parameters.OutputFileName, inHeader);

	// 	Mat inFileImage = Utilities::ToMat(tree, &inHeader, layer);
	// 	imwrite("./test_" + to_string(layer) + ".png", inFileImage);
	// 	delete tree;
	// }

	// imshow("Output Image", croppedOutputImage);
	// waitKey(0);
	
	return 0;
}