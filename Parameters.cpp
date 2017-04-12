#include <sys/stat.h>
#include <stdlib.h>

#include "Parameters.h"

Parameters::Parameters()
	: YUVConversion(true), HuffmanCoding(true), Subtract128(true) { }

Parameters Parameters::ParseCommandLine(int argc, char** argv)
{
	// print usage if not enough args
	if (argc < 2)
		_printUsageExit("Not enough arguments.", 1);

	Parameters parameters;

	// parse args
	for (short i = 1; i < argc; i++)
	{
		char* current = argv[i];

		if (current[0] == '-')
			for (int j = 1; current[j] != '\0'; j++)
				switch (current[j])
				{
					case 'c':
						parameters.YUVConversion = _extractParameter(current[++j], "Unrecognized YUV option value");
						break;
					case 'u':
						parameters.HuffmanCoding = _extractParameter(current[++j], "Unrecognized Huffman option value");
						break;
					// case 's':
					// 	parameters.Subtract128 = _extractParameter(current[++j], "Unrecognized -128 option value");
					// 	break;
					case 'q':
						{
							string quality(current);
							parameters.Quality = stoi(quality.substr(2));

							if (parameters.Quality == 0)
								_printUsageExit("Unrecognized quality value", 1);
							
							j = quality.size() - 1;
							break;
						}
					default:
						_printUsageExit("Unrecognized options: " + string(current), 1);
				}
		else
		{
			if (parameters.InputFileName == "")
			{
				// stat input file
				struct stat buffer;
				if (!stat(current, &buffer))
					parameters.InputFileName = string(current);
				else
					_printUsageExit("Invalid input file path: " + string(current), 1);
			}
			else if (parameters.OutputFileName == "")
				parameters.OutputFileName = string(current);
			else
				_printUsageExit("Invalid file path: " + string(current), 1);
		}
	}

	if (parameters.InputFileName == "")
		_printUsageExit("No input file specified.", 1);
	
	if (parameters.OutputFileName == "")
	{
		size_t lastDot = parameters.InputFileName.find_last_of(".");

		if (lastDot == string::npos)
			parameters.OutputFileName = parameters.InputFileName + ".picts";
		else
			parameters.OutputFileName = parameters.InputFileName.substr(0, lastDot) + ".picts";
	}

	return parameters;
}

void Parameters::_printUsageExit(string errorMessage, int code)
{
	(code != 0 ? cerr : cout)
		<< "Error: " << errorMessage << endl;

	(code != 0 ? cerr : cout)
		<< "usage: picts-compressor [options] <input file path> [output path]" << endl
		<< "Options:" << endl
		<< "    -h         this help text" << endl
		<< "    -c<1/0>    do YUV color conversion; default 1" << endl
		<< "If no output path is specified, input file path with .picts extension is used." << endl;

	exit(code);
}

bool Parameters::_extractParameter(char value, string errorMessage)
{
	switch (value)
	{
		case '1':
			return true;
		case '0':
			return false;
		default:
			(errorMessage += ": ").push_back(value);
			_printUsageExit(errorMessage, 1);
			return false;
	}
}

ostream& operator<< (ostream& os, Parameters& parameters)
{
	// rebuild args for serialization
	os << "-c"  << parameters.YUVConversion
	   << " -u" << parameters.HuffmanCoding
	//    << " -s" << parameters.Subtract128
	   << " -q" << (int)parameters.Quality
	   << " "   << parameters.InputFileName
	   << " "   << parameters.OutputFileName;
	return os;
}