#include <crtdbg.h>
#include "stdafx.h"

#include <et/core/tools.h>
#include <et/gui/fontgen.h>

using namespace et;
using namespace et::gui;

const int maxFontSize = 128;
const int maxFontOffset = 32;

void displayHelp(_TCHAR* argv[])
{
	std::cout << "Using: " << std::endl 
		<< getFileName(argv[0]) << std::endl 
		<< " -out OUTFILENAME" << std::endl 
		<< " -face FONTFACE" << std::endl 
		<< " -size FONTSIZE" << std::endl 
		<< " -offset CHAROFFSET" << std::endl;

}

int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	std::string fontFace = "Tahoma";
	std::string fontSize = "12";
	std::string outFile = "";
	std::string fontOffset = "0";

	if ((argc == 1) || (argc % 2 == 0))
	{
		displayHelp(argv);
		return 0;
	}

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-help") == 0)
		{
			displayHelp(argv);
			return 0;
		}
		else if (strcmp(argv[i], "-out") == 0)
		{
			if (++i >= argc) break;
			outFile = argv[i];
		}
		else if (strcmp(argv[i], "-face") == 0)
		{
			if (++i >= argc) break;
			fontFace = argv[i];
		}
		else if (strcmp(argv[i], "-size") == 0)
		{
			if (++i >= argc) break;
			fontSize = argv[i];
		}
		else if (strcmp(argv[i], "-offset") == 0)
		{
			if (++i >= argc) break;
			fontOffset = argv[i];
		}
	}

	if (outFile.size() == 0)
		outFile = fontFace;

	FontGenerator gen;
	gen.setFontFace(fontFace);
	gen.setOutputFile(outFile);

	int size = strToInt(fontSize);
	if ((size < 0) || (size > maxFontSize))
	{
		std::cout << "Font size is not valid. Should be greater than zero and less than " << maxFontSize << std::endl;
		return 0;
	}
	gen.setSize(size);

	int offset = strToInt(fontOffset);
	if ((offset < 0) || (offset > maxFontOffset))
	{
		std::cout << "Font offset is not valid. Should be greater than zero and less than " << maxFontSize << std::endl;
		return 0;
	}
	gen.setOffset(static_cast<float>(offset));

	std::cout << "Generating font: " << fontFace << ", " << fontSize << std::endl;
	std::cout.flush();

	switch (gen.generate())
	{
	case FontGeneratorResult_OutputFileFailed:
		{
			std::cout << "Output file failed to write." << std::endl;
			break;
		}
	case FontGeneratorResult_OutputFileNotDefined:
		{
			std::cout << "Output file is not defined." << std::endl;
			break;
		}
	case FontGeneratorResult_Success:
		{
			std::cout << "Success" << std::endl;
			break;
		}
	default:
		{
			std::cout << "???" << std::endl;
		}
	}

	return 0;
}

