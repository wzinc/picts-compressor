#ifndef HuffmanTree_h
#define HuffmanTree_h

#include <opencv2/opencv.hpp>
#include <list>

#include "HeaderOptions.h"
#include "HuffmanTreeNode.h"
#include "ofbitstream.h"
#include "ifbitstream.h"

using namespace std;
using namespace cv;

#define MAX_LAYERS 8

class HuffmanTree
{
	public:
		static void ZigzagMatProcessor (Mat*, uint8_t, function<bool(int8_t*, uint8_t, uint8_t, uint8_t)>);
		static void ZigzagMatProcessor (Mat*, uint8_t, int8_t, function<bool(int8_t*, uint8_t, uint8_t, uint8_t)>);

		// static HuffmanTree* deserialize (const uchar*);
		static HuffmanTree* Deserialize (ifbitstream&, HeaderOptions&);

		static HuffmanTreeNode* DeserializeTree (ifbitstream&, map<int8_t, uint64_t>*);
		static list<uint8_t>* DeserializeLayer (ifbitstream&, HuffmanTreeNode*);

		static HuffmanTree* FromImage(Mat*, uint8_t);
		
		uint8_t AddLayer(ifbitstream&);
		static uint8_t AddLayer(ifbitstream&, HuffmanTree*);
		Mat* ToImage(HeaderOptions&);
		Mat* ToImage(HeaderOptions&, uint8_t);

		~HuffmanTree();

		static map<uchar, tuple<uchar, uchar>> Traverse(HuffmanTreeNode*);
		static map<uchar, tuple<uchar, uchar>> Traverse(int8_t, uchar, HuffmanTreeNode*);

		// returns serialized length
		uint64_t SerializeTree (ostream&, uint8_t);

		// write to files; returns layer offsets (from 0)
		uint64_t SerializeLayer(ofbitstream&, uint8_t);

		HuffmanTreeNode* getRoot(uint8_t layer) { return _roots.at(layer); }
    
        uint8_t getLayerCount() { return _layerCount; }

	private:
		static HuffmanTreeNode* _treeFromValueWeightMap(map<int8_t, uint64_t>*);
		static int8_t _nextValueFromBitstream(ifbitstream&, HuffmanTreeNode*);

		uint8_t _layerCount;

		vector<HuffmanTreeNode*> _roots;
		vector<list<uint8_t>*> _layerData;
		vector<map<int8_t, uint64_t>*> _valueWeightMaps;

		HuffmanTree(uint8_t);
};

#endif
