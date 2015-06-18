#include <et/core/tools.h>
#include <et/imaging/pngloader.h>
#include "..\..\..\et-ext\include\et-ext\scene2d\textureatlaswriter.h"

using namespace et;

void printHelp()
{
	log::info("Using:\n"
		"atlas -root <ROOT FOLDER> -out <OUTPUT FILE>\n"
		"\tOPTIONAL: -size <SIZE>, default: 1024 - size of the output atlas\n"
		"\tOPTIONAL: -pattern <PATTERN>, default: texture_%%d.png\n"
		"\tOPTIONAL: -nospace, default off - don't add one pixel space between images in atlas.");
}

int main(int argc, char* argv[])
{
	log::addOutput(log::ConsoleOutput::Pointer::create());
	
	bool hasPattern = false;
	bool hasRoot = false;
	bool hasOutput = false;
	bool addSpace = true;

	std::string rootFolder;
	std::string outFile;
	std::string pattern = "texture_%d.png";
	int outputSize = 1024;

	for (int i = 1; i < argc; ++i)
	{
		if ((strcmp(argv[i], "-root") == 0) && (i + 1 < argc))
		{
			rootFolder = addTrailingSlash(std::string(argv[i+1]));
			if (folderExists(rootFolder))
			{
				hasRoot = true;
				++i;
			}
			else
			{
				log::error("Root folder not found: %s", rootFolder.c_str());
				return 0;
			}
		}
		else if ((strcmp(argv[i], "-out") == 0) && (i + 1 < argc))
		{
			outFile = std::string(argv[i+1]);
			hasOutput = true;
			++i;
		}
		else if ((strcmp(argv[i], "-pattern") == 0) && (i + 1 < argc))
		{
			pattern = std::string(argv[i+1]);
			hasPattern = true;
			++i;
		}
		else if (strcmp(argv[i], "-nospace") == 0)
		{
			addSpace = false;
		}
		else if ((strcmp(argv[i], "-size") == 0) && (i + 1 < argc))
		{
			outputSize = strToInt(std::string(argv[i+1]));
			++i;
		}
	}

	if (!hasRoot || !hasOutput)
	{
		printHelp();
		return 0;
	}
		
	StringList fileList;
	TextureDescription::List textureDescriptors;
	findFiles(rootFolder, "*.png", true, fileList);	

	for (auto i : fileList)
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		png::loadInfoFromFile(i, desc.reference());
		
		if ((desc->size.x > outputSize) || (desc->size.y > outputSize))
		{
			log::error("Image %s is larger (%d x %d) than ouput size (%d x %d), use -size option",
				i.c_str(), desc->size.x, desc->size.y, outputSize, outputSize);
			return 0;
		}
		textureDescriptors.push_back(desc);
	}
	
	std::sort(textureDescriptors.begin(), textureDescriptors.end(), [](const TextureDescription::Pointer& d1,
		const TextureDescription::Pointer& d2) { return d1->size.square() > d2->size.square(); });

	TextureAtlasWriter placer(addSpace);
	while (textureDescriptors.size())
	{
		TextureAtlasWriter::TextureAtlasItem& texture = placer.addItem(vec2i(outputSize));
		TextureDescription::List::iterator i = textureDescriptors.begin();

		int placedItems = 0;
		
		while (i != textureDescriptors.end())
		{
			if (placer.placeImage(*i, texture))
			{
				++placedItems;
				i = textureDescriptors.erase(i);
			}
			else 
			{
				++i;
			}
		}
		
		texture.texture->size.x = static_cast<int>(roundToHighestPowerOfTwo(texture.maxWidth));
		texture.texture->size.y = static_cast<int>(roundToHighestPowerOfTwo(texture.maxHeight));
	}
	
	for (const auto i : placer.items())
	{
		log::info("Texture: %d x %d", i.texture->size.x, i.texture->size.y);
		
		for (const auto& ii : i.images)
		{
			log::info("(% 5d, % 5d) | (% 5d % 5d) | %s", static_cast<int>(ii.place.origin.x),
				static_cast<int>(ii.place.origin.y), static_cast<int>(ii.place.size.x),
				static_cast<int>(ii.place.size.y), getFileName(ii.image->origin()).c_str());
		}
	}

	placer.writeToFile(outFile, pattern.c_str());

	return 0;
}

