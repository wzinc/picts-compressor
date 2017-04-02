#include "HuffmanTree.h"

#include <algorithm>
#include <assert.h>

HuffmanTree::HuffmanTree(uint8_t layerCount)
{
	assert(layerCount <= MAX_LAYERS);
	_layerCount = layerCount;
}

HuffmanTree::~HuffmanTree()
{
	for (HuffmanTreeNode* node : _roots)
		delete node;

	for (list<uint8_t>* layer : _layerData)
		delete layer;
	
	for (map<uint8_t, uint64_t>* valueWeightMap : _valueWeightMaps)
		delete valueWeightMap;
}

HuffmanTree* HuffmanTree::FromImage(Mat* inputImage, uint8_t layerCount)
{
	// max of 8 layers
	assert(layerCount <= 8);

	// Mat must be CV_89U & evenly divisible by 8
	assert(inputImage->type() == CV_8U   ||
		   inputImage->type() == CV_8UC1 ||
		   inputImage->type() == CV_8UC2 ||
		   inputImage->type() == CV_8UC3 ||
		   inputImage->type() == CV_8UC4 );

	uint32_t width = inputImage->size().width,
			 height = inputImage->size().height;
	
	/// cout << "width: " << width << " height: " << height << endl;
	
	assert(!(width % 8));
	assert(!(height % 8));

	/*
		layers break blocks into diagonal sections:

	    {0, 1, 2, 3, 4, 5, 6, 7},
		{1, 2, 3, 4, 5, 6, 7, 7},
		{2, 3, 4, 5, 6, 7, 7, 7},
		{3, 4, 5, 6, 7, 7, 7, 7},
		{4, 5, 6, 7, 7, 7, 7, 7},
		{5, 6, 7, 7, 7, 7, 7, 7},
		{6, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 7, 7}

		if there are <7 layers, all the rest of the data becomes the top layer

		- zig-zag traverse matrix
			- create list of values for each layer
			- prepend last non-zero value count to list
			- truncate list after each last-non-zero element
		- create Huffman tree from each list
		- create address lookup table from tree
		- for each layer
			- set-aside 4-byte space for tree size
			- serialize tree
			- seek to start of tree; save length to 4-byte space
			- set-aside 4-byte space for value size
			- serialize values to bitstream using Huffman tree lookup
			- seek to start of bitstream; save length to 4-byte space
		
		- header has size & layer count
		- for each layer
			- read tree size
			- load tree addresses and lengths
			- build tree from addresses and lengths
			- read value size
			- read values & run through Huffman tree into list
		- create Mat from size
		- for each block
			- for each layer
				- restore values to block
				- fill remainder of layer with zeros
			- copy to layer in image
		- export image to desired format (UIImage)
	*/

	// get direct access to channels
	vector<Mat> channels;
	split(*inputImage, channels);
	uint8_t channelCount = inputImage->channels();

	vector<uint8_t> elementCounts;
	map<uint8_t, vector<uint8_t>> currentLayerData;
	HuffmanTree *tree = new HuffmanTree(layerCount);

	for (uint8_t i = 0; i < layerCount; i++)
	{
		elementCounts.push_back(0);

		list<uint8_t> *layer = new list<uint8_t>();

		tree->_layerData.push_back(layer);

		vector<uint8_t> currentLayer;
		currentLayerData[i] = currentLayer;

		map<uchar, uint64_t> *layerByteCount = new map<uchar, uint64_t>;
		tree->_valueWeightMaps.push_back(layerByteCount);
	}

	for (uint8_t i = 0; i < channelCount; i++)
	{
		Mat currentChannel = channels[i];
		for (uint32_t j = 0; j < width; j += 8)
			for (uint32_t k = 0; k < height; k+= 8)
			{
				Mat currentBlock = currentChannel(Rect(j, k, 8, 8));
				
				// zig-zag traverse matrix
				ZigzagMatProcessor(&currentBlock, layerCount, [&](uchar* value, uint8_t index, uint8_t layer, uint8_t layerIndex)
				{
					uint8_t layerI = layer - 1;

					// save last non-zero value index
					if (*value)
						elementCounts[layerI] = layerIndex + 1;
					
					// create list of values for each layer
					currentLayerData[layerI].push_back(*value);

					return true;
				});

				// layer 0 has no count - always one element
				// layer 1..(n - 1) have n elements
				// layer n has all other elements - almost certain to have trailing-zero values
				for (uint8_t l = 0; l < layerCount; l++)
				{
					map<uint8_t, uint64_t> *valueWeightMap = tree->_valueWeightMaps[l];

					// other layers
					//	add size
					uint8_t value = elementCounts[l];
					tree->_layerData[l]->push_back(value);
					((*valueWeightMap)[value])++;

					// add values to count
					while (elementCounts[l]-- > 0)
					{
						value = currentLayerData[l].front();
						tree->_layerData[l]->push_back(value);
						currentLayerData[l].erase(currentLayerData[l].begin());
						((*valueWeightMap)[value])++;
					}

					// empty queue
					currentLayerData[l].clear();
					elementCounts[l] = 0;
				}
			}
	}

	// create trees from weight maps
	for (uint8_t l = 0; l < layerCount; l++)
		tree->_roots.push_back(_treeFromValueWeightMap(tree->_valueWeightMaps[l]));

	return tree;
}

HuffmanTreeNode* HuffmanTree::_treeFromValueWeightMap(map<uchar, uint64_t> *valueWeightMap)
{
	/// cout << "\033[1;31mHuffmanTree::_treeFromValueWeightMap\033[0m" << endl;
	
	// create HuffmanTreeNodes & push values to vector
	vector<HuffmanTreeNode*> nodes;
	for (pair<uchar, uint64_t> valueWeight : *valueWeightMap)
	{
		HuffmanTreeNode *node = new HuffmanTreeNode(valueWeight.second, valueWeight.first);
		nodes.push_back(node);
	}

	// sort vector
	sort(nodes.begin(), nodes.end(), [](HuffmanTreeNode* a, HuffmanTreeNode* b) { return a->getWeight() < b->getWeight(); });

	while (nodes.size() > 1)
	{
		vector<HuffmanTreeNode*> combinedNodes;

		while (nodes.size() >= 1)
			switch (nodes.size())
			{
				case 1:
					// only one left
					combinedNodes.push_back(nodes.at(0));
					nodes.erase(nodes.begin());
					break;
				default:
					HuffmanTreeNode *node0 = nodes.at(0),
									*node1 = nodes.at(1),
									*nodeP = new HuffmanTreeNode(node0->getWeight() + node1->getWeight(), 0, node0, node1);
					
					nodes.erase(nodes.begin());
					nodes.erase(nodes.begin());
					combinedNodes.push_back(nodeP);
					break;
			}

		nodes = combinedNodes;
	}

	return nodes.at(0);
}

map<uchar, tuple<uchar, uchar>> HuffmanTree::Traverse(HuffmanTreeNode* baseNode)
{
	return Traverse(0, 0, baseNode);
}

map<uchar, tuple<uchar, uchar>> HuffmanTree::Traverse(uchar value, uchar depth, HuffmanTreeNode* baseNode)
{
	assert(baseNode != NULL);

	bool leaf = true;
	map <uchar, tuple<uchar, uchar>> values;

	// traverse left node
	if (baseNode->get0())
	{
		map <uchar, tuple<uchar, uchar>> values0 = Traverse(value << 1 | 0, depth + 1, baseNode->get0());
		values.insert(values0.begin(), values0.end());

		leaf = false;
	}
	
	// traverse right node
	if (baseNode->get1())
	{
		map <uchar, tuple<uchar, uchar>> values1 = Traverse(value << 1 | 1, depth + 1, baseNode->get1());
		values.insert(values1.begin(), values1.end());

		leaf = false;
	}

	// if this is a leaf, add it to the value list
	if (leaf)
		values[baseNode->getValue()] = make_tuple(value, depth);

	return values;
}

uint64_t HuffmanTree::SerializeTree (ostream& outputStream, uint8_t layer)
{
	/// cout << "\033[1;31mHuffmanTree::SerializeTree: " << (int)layer << "\033[0m" << endl;
    
	// value, weight
	vector<uchar> tree;
	int sizeofFirst = sizeof(uchar),
		sizeofSecond = sizeof(uint64_t),
		recordSize = sizeofFirst + sizeofSecond;
	
	map<uint8_t, uint64_t> *valueWeightMap = _valueWeightMaps[layer];
	uint32_t entryCount = valueWeightMap->size();
	
	// save tree size
	outputStream.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

	// save items
	for (auto valueWeight : *valueWeightMap)
	{
		outputStream.write(reinterpret_cast<const char*>(&valueWeight.first), sizeofFirst);
		outputStream.write(reinterpret_cast<const char*>(&valueWeight.second), sizeofSecond);
	}

	return entryCount * 9;
}

HuffmanTreeNode* HuffmanTree::DeserializeTree (ifbitstream& inputStream, map<uint8_t, uint64_t> *valueWeightMap)
{
	/// cout << "\033[1;31mHuffmanTree::DeserializeTree\033[0m" << endl;
	
	if (!valueWeightMap)
	{
		map<uint8_t, uint64_t> tempValueWeightMap;
		valueWeightMap = &tempValueWeightMap;
	}
		
	// read entry count
	uint32_t entryCount = 0;
	inputStream.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

	// read tree
	for (uint8_t j = 0; j < entryCount; j++)
	{
		uint8_t value;
		uint64_t weight;
		inputStream.read(reinterpret_cast<char*>(&value), sizeof(value));
		inputStream.read(reinterpret_cast<char*>(&weight), sizeof(weight));

		// fill map, if map provided
		(*valueWeightMap)[value] = weight;

		// if we have the max number of values, we need to kill this loop here
		if (j == 255)
			break;
	}

	return _treeFromValueWeightMap(valueWeightMap);
}

HuffmanTree* HuffmanTree::Deserialize (ifbitstream& inputStream, HeaderOptions& header)
{
	/// cout << "\033[1;31mHuffmanTree::Deserialize\033[0m" << endl;
	
	HuffmanTree* tree = new HuffmanTree(0);
	// HuffmanTree* tree = new HuffmanTree(header.getLayerCount());

	// read-in each layer
	for (uint8_t i = 0; i < header.getLayerCount() && !inputStream.eof(); i++)
		AddLayer(inputStream, tree);

	return tree;
}

uint8_t HuffmanTree::AddLayer(ifbitstream& inputStream)
{
	return AddLayer(inputStream, this);
}

uint8_t HuffmanTree::AddLayer(ifbitstream& inputStream, HuffmanTree* tree)
{
	/// cout << "\033[1;31mHuffmanTree::AddLayer\033[0m" << endl;

	map<uint8_t, uint64_t> *valueWeightMap = new map<uint8_t, uint64_t>();
	HuffmanTreeNode* root = DeserializeTree(inputStream, valueWeightMap);
	tree->_roots.push_back(_treeFromValueWeightMap(valueWeightMap));

	tree->_valueWeightMaps. push_back(valueWeightMap);

	list<uint8_t> *layerData = DeserializeLayer(inputStream, root);
	tree->_layerData.push_back(layerData);

	return ++tree->_layerCount;
}

Mat* HuffmanTree::ToImage(HeaderOptions& header)
{
	return ToImage(header, 0);
}

Mat* HuffmanTree::ToImage(HeaderOptions& header, uint8_t maxLayer)
{
	if (!maxLayer)
		maxLayer = _layerCount;
	else
		maxLayer = min(maxLayer, _layerCount);
	
	/// cout << "\033[1;31mHuffmanTree::ToImage: " << (int)maxLayer << "\033[0m" << endl;

	int32_t width = header.getPadWidth(),
		height = header.getPadHeight();

	Mat *image = new Mat(height, width, CV_8UC3, Scalar(0));

	// get direct access to channels
	vector<Mat> channels;
	split(*image, channels);
	uint8_t channelCount = image->channels();

	uint8_t index = 0;
	vector<list<uint8_t>> localLayerData;
	for (auto layerData : _layerData)
	{
		list<uint8_t> layer(layerData->begin(), layerData->end());
		localLayerData.push_back(layer);
	}

	int max = 10;
	list<uint8_t> *currentLayerData;
	for (uint8_t i = 0; i < channelCount; i++)
	{
		Mat currentChannel = channels[i];
		for (uint32_t j = 0; j < width; j += 8)
			for (uint32_t k = 0; k < height; k+= 8)
			{
				// Mat currentBlock = currentChannel(Rect(j, k, 8, 8));
				Mat currentBlock(8, 8, CV_8UC1, Scalar(128));
				// Mat currentBlock(8, 8, CV_8UC1, Scalar(!i ? 0 : 128));
				// Mat currentBlock(8, 8, CV_8UC1, Scalar(0));
				uint8_t currentLayer = 0, currentValueCount = 0;
				
				// zig-zag traverse matrix
				ZigzagMatProcessor(&currentBlock, maxLayer, [&](uchar* value, uint8_t index, uint8_t layer, uint8_t layerIndex)
				{
					// if first time through, read layer data
					if (currentLayer < layer)
					{
						currentLayerData = &localLayerData[currentLayer];

						// read count
						currentValueCount = currentLayerData->front();
						currentLayer = layer;

						// if this is layer0 & there's no value, that means it's a 0
						//	add one to value count and reuse that 0 as the value
						if (currentLayer == 1 && currentValueCount == 0)
							currentValueCount = 1;
						else
							currentLayerData->pop_front();
					}

					if (currentValueCount > layerIndex)
					{
						*value = currentLayerData->front();
						currentLayerData->pop_front();
					}

					return true;
				});

				currentBlock.copyTo(currentChannel(Rect(j, k, 8, 8)));
			}
	}

	merge(channels, *image);

	return image;
}

uint64_t HuffmanTree::SerializeLayer(ofbitstream& outputStream, uint8_t layer)
{
	/// cout << "\033[1;31mHuffmanTree::SerializeLayer: " << (int)layer << "\033[0m" << endl;
	
	// store: length (uint64_t), number of values (uchar), tree data (uchar[])
	map<uchar, tuple<uchar, uchar>> valueMap = Traverse(_roots[layer]);

	list<uint8_t> *layerData = _layerData[layer];
	uint32_t layerBits = 0,
			 layerDataCount = layerData->size(),
			 layerBytesLocation = outputStream.tellp();
	
	// leave space for layer size
	outputStream.write(reinterpret_cast<char*>(&layerBits), sizeof(layerBits));
	outputStream.write(reinterpret_cast<char*>(&layerDataCount), sizeof(layerDataCount));

	// get addresses for values
	for (auto value : *layerData)
	{
		tuple<uchar, uchar> addressTuple = valueMap[value];

		uint8_t length = get<1>(addressTuple);
		outputStream.writeBits(get<0>(addressTuple), length);
		layerBits += length;
	}

	outputStream.flush();

	// layer length value + layer byte length (bits / 8) +1 if we're in the middle of a byte
	uint32_t layerBytes =sizeof(layerDataCount) + (layerBits / 8 + (layerBits % 8 ? 1 : 0)),
			 serializedLayerLength = layerBytes + sizeof(layerBytes) + sizeof(layerDataCount);

	// go back, write value, return
	outputStream.seekp(layerBytesLocation);
	outputStream.write(reinterpret_cast<char*>(&layerBytes), sizeof(layerBytes));
	outputStream.seekp(0, ios::end);

	return serializedLayerLength;
}

uint8_t HuffmanTree::_nextValueFromBitstream(ifbitstream& inputStream, HuffmanTreeNode* root)
{
	HuffmanTreeNode currentNode = *root;

	// keep reading values until a leaf node
	while (currentNode.get0() && currentNode.get1())
	{
		uint8_t bit = inputStream.readBit();

		currentNode = bit ?
			(currentNode = currentNode.get1() ? *currentNode.get1() : currentNode) :
			(currentNode = currentNode.get0() ? *currentNode.get0() : currentNode) ;
	}

	return currentNode.getValue();
}

list<uint8_t>* HuffmanTree::DeserializeLayer(ifbitstream& inputStream, HuffmanTreeNode* root)
{
	/// cout << "\033[1;31mHuffmanTree::DeserializeLayer\033[0m" << endl;
	
	// read layer value length
	uint32_t layerLength, layerDataCount;
	inputStream.read(reinterpret_cast<char*>(&layerLength), sizeof(layerLength));
	inputStream.read(reinterpret_cast<char*>(&layerDataCount), sizeof(layerDataCount));

	// read layer data
	//	layer0 has no counts
	list<uint8_t> *layerData = new list<uint8_t>();

	while (layerDataCount--)
	{
		uint8_t value = _nextValueFromBitstream(inputStream, root);
		layerData->push_back(value);
	}

	// skip rest of current byte
	inputStream.skipByte();

	return layerData;
}

void HuffmanTree::ZigzagMatProcessor (Mat* mat, uint8_t layerCount, function<bool(uchar* value, uint8_t index, uint8_t layer, uint8_t layerIndex)> elementCallback)
{
	return ZigzagMatProcessor(mat, layerCount, -1, elementCallback);
}

void HuffmanTree::ZigzagMatProcessor (Mat* mat, uint8_t layerCount, int8_t stopAfterElement, function<bool(uchar* value, uint8_t index, uint8_t layer, uint8_t layerIndex)> elementCallback)
{
	assert(layerCount <= MAX_LAYERS);
	
	bool down = true, lastFix = false;
	uint8_t rows = mat->rows - 1, cols = mat->cols - 1,
		elements = mat->rows * mat->cols,
		x = 0, y = 0, currentLayer = 1, layerIndex = 0;

	for (uint8_t i = 0u; i < elements; i++)
	{
		if (i)
		{
			if (!y && down)
			{
				x++;
				down = false;

				if (currentLayer < layerCount)
				{
					currentLayer++;
					layerIndex = 0;
				}

				if (x > cols)
				{
					y += x - cols;
					x = cols;
				}
			}
			else if (!x && !down)
			{
				y++;
				down = true;

				if (currentLayer < layerCount)
				{
					currentLayer++;
					layerIndex = 0;
				}

				if (y > rows)
				{
					x += y - rows;
					y = rows;
				}
			}
			else
			{
				if (down)
				{
					if (x == cols && !lastFix)
					{
						y++;
						down = false;
						lastFix = true;
					}
					else
					{
						x++;
						y--;
						lastFix = false;
					}
				}
				else
				{
					if (y == rows && !lastFix)
					{
						x++;
						down = true;
						lastFix = true;
					}
					else
					{
						x--;
						y++;
						lastFix = false;
					}
				}
			}
		}

		uchar* row = mat->ptr<uchar>(y);
		
		// stop here if callback returned false or we're supposed to stop after this element
		if (!elementCallback(&row[x], i, currentLayer, layerIndex++) || (stopAfterElement >= 0 && i == stopAfterElement))
			return;
	}
}
