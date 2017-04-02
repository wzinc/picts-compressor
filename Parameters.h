#include <iostream>

using namespace std;

class Parameters
{
	public:
		string InputFileName, OutputFileName;
		bool YUVConversion, HuffmanCoding, Subtract128;

		Parameters();

		static Parameters ParseCommandLine(int, char**);

		friend ostream& operator<< (ostream&, Parameters&);
	
	private:
		static void _printUsageExit(string, int);
		static bool _extractParameter(char value, string errorMessage);
};