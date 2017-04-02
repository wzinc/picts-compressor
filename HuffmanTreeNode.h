#ifndef HuffmanTreeNode_h
#define HuffmanTreeNode_h

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class HuffmanTreeNode
{
	public:
		HuffmanTreeNode(uint64_t weight, uchar value);
		HuffmanTreeNode(uint64_t weight, uchar value, HuffmanTreeNode* node0, HuffmanTreeNode* node1);

		~HuffmanTreeNode();

		uint64_t getWeight() { return _weight; }
		uchar getValue() { return _value; }

		HuffmanTreeNode* get0() { return _0; }
		HuffmanTreeNode* get1() { return _1; }

		HuffmanTreeNode* Traverse(bool value);

		string print(uint64_t level);
		string print(uint64_t level, string title);

		friend ostream& operator<< (ostream&, HuffmanTreeNode&);

	private:
		uint64_t _weight;
		uchar _value;

		HuffmanTreeNode *_0 = NULL, *_1 = NULL;
};

#endif