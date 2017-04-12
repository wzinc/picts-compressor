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
    
    HuffmanTree* tree = HuffmanTree::Deserialize(inFile, header);
    
    inFile.close();
    
    return tree;
}

HeaderOptions Utilities::ReadHeader(string filePath)
{
    ifbitstream inFile(filePath);
    HeaderOptions header = HeaderOptions::Deserialize(inFile);
    
    inFile.close();
    
    return header;
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

		outputImage.convertTo(outputImage, CV_16SC3);
		add(outputImage, 128, outputImage);
		outputImage.convertTo(outputImage, CV_8UC3);

		return outputImage;
	}
	
	Mat outputImage(header->getPadHeight() / 8 * maxLayers, header->getPadWidth() / 8 * maxLayers, CV_16SC3, Scalar(0));
	
	for (uint32_t j = 0; j < header->getPadWidth(); j += 8)
		for (uint32_t k = 0; k < header->getPadHeight(); k+= 8)
		{
			Rect sourceRect(j, k, maxLayers, maxLayers);
			Rect destRect(j / 8 * maxLayers, k / 8 * maxLayers, maxLayers, maxLayers);

			(*inImage)(sourceRect).copyTo(outputImage(destRect));
		}

	if (header->getYUVColor())
		cvtColor(outputImage, outputImage, CV_YCrCb2BGR);
	
	add(outputImage, 128, outputImage);
	outputImage.convertTo(outputImage, CV_8UC3);
	
	return outputImage;
}

void Utilities::DecompressImage(Mat *inImage, HeaderOptions *header)
{
	Mat* quantizationMatricies = GenerateQuantizationMatricies((double)header->getQuality());
	Mat luminance = quantizationMatricies[0],
		chrominance = quantizationMatricies[1];
	
	// cout << "luminance: " << endl << luminance << endl << "===============================================" << endl;
	// cout << "chrominance: " << endl << chrominance << endl << "===============================================" << endl;
	
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
				
				// if (header->getSubtract128())
				// {
				// 	subtract(currentBlock, 128.0, currentBlock);
				
				// 	// if (!i && !j && !k)
				// 	// 	cout << "after -128.0: " << endl << currentBlock << endl << "===============================================" << endl;
				// }
				
				// dequantization
				multiply(currentBlock, !i ? luminance : chrominance, currentBlock);

				// if (!i && !j && !k)
				// 	cout << "after dequantization: " << endl << currentBlock << endl << "===============================================" << endl;
				
				// iDCT
				dct(currentBlock, currentBlock, DCT_INVERSE);

				// if (!i && !j && !k)
				// 	cout << "after iDCT: " << endl << currentBlock << endl << "===============================================" << endl;
				
				// if -128 cli flag
				// if (header->getSubtract128())
				// {
				// 	add(currentBlock, 128.0, currentBlock);
				//
				// 	if (!i && !j && !k)
				// 		cout << "after +128: " << endl << currentBlock << endl << "===============================================" << endl;
				// }

				RoundSingleDimMat(&currentBlock);

				// if (!i && !j && !k)
				// 	cout << "after _roundSingleDimMat (final): " << endl << currentBlock << endl << "===============================================" << endl;

				currentBlock.copyTo(currentChannel(Rect(j, k, 8, 8)));
			}
	}

	merge(inChannels, *inImage);
	// inImage->convertTo(*inImage, CV_8SC3);
	// inImage->convertTo(*inImage, CV_8UC3);
}

// algorithms from: http://docs.opencv.org/2.4/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html
double Utilities::getPSNR(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    Scalar s = sum(s1);        // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if( sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double mse  = sse / (double)(I1.channels() * I1.total());
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}

Scalar Utilities::getMSSIM(const Mat& i1, const Mat& i2)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /***************************** INITS **********************************/
    int d = CV_32F;

    Mat I1, I2;
    i1.convertTo(I1, d);            // cannot calculate on one byte large values
    i2.convertTo(I2, d);

    Mat I2_2   = I2.mul(I2);        // I2^2
    Mat I1_2   = I1.mul(I1);        // I1^2
    Mat I1_I2  = I1.mul(I2);        // I1 * I2

    /*************************** END INITS **********************************/

    Mat mu1, mu2;                   // PRELIMINARY COMPUTING
    GaussianBlur(I1, mu1, Size(11, 11), 1.5);
    GaussianBlur(I2, mu2, Size(11, 11), 1.5);

    Mat mu1_2   =   mu1.mul(mu1);
    Mat mu2_2   =   mu2.mul(mu2);
    Mat mu1_mu2 =   mu1.mul(mu2);

    Mat sigma1_2, sigma2_2, sigma12;

    GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;

    GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;

    GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    ///////////////////////////////// FORMULA ////////////////////////////////
    Mat t1, t2, t3;

    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2);                 // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2);                 // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

    Mat ssim_map;
    divide(t3, t1, ssim_map);        // ssim_map =  t3./t1;

    Scalar mssim = mean(ssim_map);   // mssim = average of ssim map

	return mssim;
}

// http://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
string Utilities::type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}
