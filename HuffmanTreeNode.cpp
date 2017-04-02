#import "HuffmanTreeNode.h"

HuffmanTreeNode::HuffmanTreeNode(uint64_t weight, uchar value)
{
	_weight = weight;
	_value = value;
}

HuffmanTreeNode::HuffmanTreeNode(uint64_t weight, uchar value, HuffmanTreeNode* node0, HuffmanTreeNode* node1)
	: HuffmanTreeNode(weight, value)
{
	_0 = node0;
	_1 = node1;
}

HuffmanTreeNode::~HuffmanTreeNode()
{
	if (_0) delete _0;
	if (_1) delete _1;
}

HuffmanTreeNode* HuffmanTreeNode::Traverse(bool value)
{
	return value ? _1 : _0;
}

ostream& operator<< (ostream& os, HuffmanTreeNode& node)
{
	// rebuild args for serialization
	os << "[ w: " << node._weight << "; v: " << (int)node._value << " ]";

	if (node._0)
		os << " 0: (" << *node._0 << ")";

	if (node._1)
		os << " 1: (" << *node._1 << ")";
	
	return os;
}

string HuffmanTreeNode::print(uint64_t level)
{
	return print(level, "");
}


string HuffmanTreeNode::print(uint64_t level, string title)
{
	stringstream output;
//	output << setw(level * 4) << (title != "" ? title + ": " : "") << "[ w: " << _weight << "; v: " << (int)_value << " ]";

	if (_value || (!_0 && !_1))
		output << " (d: " << level << ")";

	if (_0)
		output << endl << _0->print(level + 1, "0");

	if (_1)
		output << endl << _1->print(level + 1, "1");

	return output.str();
}
