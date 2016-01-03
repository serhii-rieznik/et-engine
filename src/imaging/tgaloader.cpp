/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/tgaloader.h>

using namespace et;

struct TGADescription
{
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t bitsPerPixel = 0;
	uint8_t dummy = 0;
};

typedef unsigned char TGAHeader[12];
TGAHeader uTGAcompare = {0,0, 2,0,0,0,0,0,0,0,0,0};
TGAHeader cTGAcompare = {0,0,10,0,0,0,0,0,0,0,0,0};

/*
 *
 * Taken from NeHe Lesson 33
 *
bool LoadCompressedTGA(Texture * texture, char * filename, FILE * fTGA)		// Load COMPRESSED TGAs
{
	if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)					// Attempt to read header
	{
		MessageBox(NULL, "Could not read info header", "ERROR", MB_OK);		// Display Error
		if(fTGA != NULL)													// If file is open
		{
			fclose(fTGA);													// Close it
		}
		return false;														// Return failed
	}
	

	
	do
	{
		GLubyte chunkheader = 0;											// Storage for "chunk" header
		
		if(fread(&chunkheader, sizeof(GLubyte), 1, fTGA) == 0)				// Read in the 1 byte header
		{
			MessageBox(NULL, "Could not read RLE header", "ERROR", MB_OK);	// Display Error
			if(fTGA != NULL)												// If file is open
			{
				fclose(fTGA);												// Close file
			}
			if(texture->imageData != NULL)									// If there is stored image data
			{
				free(texture->imageData);									// Delete image data
			}
			return false;													// Return failed
		}
		
		if(chunkheader < 128)												// If the ehader is < 128, it means the that is the number of RAW color packets minus 1
		{																	// that follow the header
			chunkheader++;													// add 1 to get number of following color values
			for(short counter = 0; counter < chunkheader; counter++)		// Read RAW color values
			{
				if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel) // Try to read 1 pixel
				{
					MessageBox(NULL, "Could not read image data", "ERROR", MB_OK);		// IF we cant, display an error
					
					if(fTGA != NULL)													// See if file is open
					{
						fclose(fTGA);													// If so, close file
					}
					
					if(colorbuffer != NULL)												// See if colorbuffer has data in it
					{
						free(colorbuffer);												// If so, delete it
					}
					
					if(texture->imageData != NULL)										// See if there is stored Image data
					{
						free(texture->imageData);										// If so, delete it too
					}
					
					return false;														// Return failed
				}
				// write to memory
				texture->imageData[currentbyte		] = colorbuffer[2];				    // Flip R and B vcolor values around in the process
				texture->imageData[currentbyte + 1	] = colorbuffer[1];
				texture->imageData[currentbyte + 2	] = colorbuffer[0];
				
				if(tga.bytesPerPixel == 4)												// if its a 32 bpp image
				{
					texture->imageData[currentbyte + 3] = colorbuffer[3];				// copy the 4th byte
				}
				
				currentbyte += tga.bytesPerPixel;										// Increase thecurrent byte by the number of bytes per pixel
				currentpixel++;															// Increase current pixel by 1
				
				if(currentpixel > pixelcount)											// Make sure we havent read too many pixels
				{
					MessageBox(NULL, "Too many pixels read", "ERROR", NULL);			// if there is too many... Display an error!
					
					if(fTGA != NULL)													// If there is a file open
					{
						fclose(fTGA);													// Close file
					}
					
					if(colorbuffer != NULL)												// If there is data in colorbuffer
					{
						free(colorbuffer);												// Delete it
					}
					
					if(texture->imageData != NULL)										// If there is Image data
					{
						free(texture->imageData);										// delete it
					}
					
					return false;														// Return failed
				}
			}
		}
		else																			// chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
		{
			chunkheader -= 127;															// Subteact 127 to get rid of the ID bit
			if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel)		// Attempt to read following color values
			{
				MessageBox(NULL, "Could not read from file", "ERROR", MB_OK);			// If attempt fails.. Display error (again)
				
				if(fTGA != NULL)														// If thereis a file open
				{
					fclose(fTGA);														// Close it
				}
				
				if(colorbuffer != NULL)													// If there is data in the colorbuffer
				{
					free(colorbuffer);													// delete it
				}
				
				if(texture->imageData != NULL)											// If thereis image data
				{
					free(texture->imageData);											// delete it
				}
				
				return false;															// return failed
			}
			
			for(short counter = 0; counter < chunkheader; counter++)					// copy the color into the image data as many times as dictated
			{																			// by the header
				texture->imageData[currentbyte		] = colorbuffer[2];					// switch R and B bytes areound while copying
				texture->imageData[currentbyte + 1	] = colorbuffer[1];
				texture->imageData[currentbyte + 2	] = colorbuffer[0];
				
				if(tga.bytesPerPixel == 4)												// If TGA images is 32 bpp
				{
					texture->imageData[currentbyte + 3] = colorbuffer[3];				// Copy 4th byte
				}
				
				currentbyte += tga.bytesPerPixel;										// Increase current byte by the number of bytes per pixel
				currentpixel++;															// Increase pixel count by 1
				
				if(currentpixel > pixelcount)											// Make sure we havent written too many pixels
				{
					MessageBox(NULL, "Too many pixels read", "ERROR", NULL);			// if there is too many... Display an error!
					
					if(fTGA != NULL)													// If there is a file open
					{
						fclose(fTGA);													// Close file
					}
					
					if(colorbuffer != NULL)												// If there is data in colorbuffer
					{
						free(colorbuffer);												// Delete it
					}
					
					if(texture->imageData != NULL)										// If there is Image data
					{
						free(texture->imageData);										// delete it
					}
					
					return false;														// Return failed
				}
			}
		}
	}
	
	while(currentpixel < pixelcount);													// Loop while there are still pixels left
	fclose(fTGA);																		// Close the file
	return true;																		// return success
}
*/

void et::tga::loadInfoFromStream(std::istream&, TextureDescription&)
{
	ET_FAIL("Not implemented");
}

void et::tga::loadFromStream(std::istream& source, TextureDescription& desc)
{
	TGAHeader header = { };
	source.read(reinterpret_cast<char*>(header), sizeof(TGAHeader));
	
	if (memcmp(uTGAcompare, header, sizeof(TGAHeader)) == 0)
	{

	}
	else if (memcmp(cTGAcompare, header, sizeof(TGAHeader)) == 0)
	{
		ET_FAIL("Not implemented");
	}
	else
	{
		log::error("Invalid TGA file.");
		return;
	}
	
	TGADescription info = { };
	source.read(reinterpret_cast<char*>(&info), sizeof(TGADescription));
	
	desc.target = TextureTarget::Texture_2D;
	desc.type = DataFormat::UnsignedChar;
	desc.size.x = info.width;
	desc.size.y = info.height;
	desc.bitsPerPixel = info.bitsPerPixel;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.channels = desc.bitsPerPixel/ 8;

	if (desc.channels == 3)
	{
		desc.internalformat = TextureFormat::RGB;
		desc.format = TextureFormat::RGB;
	}
	else if (desc.channels == 4)
	{
		desc.internalformat = TextureFormat::RGBA;
		desc.format = TextureFormat::RGBA;
	}
	else
	{
		ET_FAIL("Not implemented");
	}
	
	desc.data.resize(desc.size.square() * desc.channels);
	source.read(desc.data.binary(), desc.data.size());
	
	unsigned char* pixels = desc.data.data();
	for (size_t i = 0; i < desc.data.size(); i += desc.channels)
	{
		pixels[i] ^= pixels[i+2];
		pixels[i+2] ^= pixels[i];
		pixels[i] ^= pixels[i+2];
	}
}

void et::tga::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void et::tga::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}
