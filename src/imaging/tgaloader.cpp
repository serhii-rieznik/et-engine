/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/imaging/tgaloader.h>

using namespace et;

struct TGADescription
{
	uint16_t width;
	uint16_t height;
	uint8_t bitsPerPixel;
	uint8_t dummy;
/*
	GLubyte header[6];
	GLuint bytesPerPixel = 0;						
	GLuint imageSize = 0;
	GLuint temp = 0;
	GLuint type = 0;
	GLuint height = 0;
	GLuint width = 0;
	GLuint Bpp = 0;
*/
};

typedef GLubyte TGAHeader[12];
TGAHeader uTGAcompare = {0,0, 2,0,0,0,0,0,0,0,0,0};
TGAHeader cTGAcompare = {0,0,10,0,0,0,0,0,0,0,0,0};

/*
bool LoadUncompressedTGA(Texture * texture, char * filename, FILE * fTGA)	// Load an uncompressed TGA (note, much of this code is based on NeHe's
{																			// TGA Loading code nehe.gamedev.net)
	if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)					// Read TGA header
	{
		MessageBox(NULL, "Could not read info header", "ERROR", MB_OK);		// Display error
		if(fTGA != NULL)													// if file is still open
		{
			fclose(fTGA);													// Close it
		}
		return false;														// Return failular
	}
	
	texture->width  = tga.header[1] * 256 + tga.header[0];					// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = tga.header[3] * 256 + tga.header[2];					// Determine The TGA Height	(highbyte*256+lowbyte)
	texture->bpp	= tga.header[4];										// Determine the bits per pixel
	tga.Width		= texture->width;										// Copy width into local structure
	tga.Height		= texture->height;										// Copy height into local structure
	tga.Bpp			= texture->bpp;											// Copy BPP into local structure
	
	if((texture->width <= 0) || (texture->height <= 0) || ((texture->bpp != 24) && (texture->bpp !=32)))	// Make sure all information is valid
	{
		MessageBox(NULL, "Invalid texture information", "ERROR", MB_OK);	// Display Error
		if(fTGA != NULL)													// Check if file is still open
		{
			fclose(fTGA);													// If so, close it
		}
		return false;														// Return failed
	}
	
	if(texture->bpp == 24)													// If the BPP of the image is 24...
		texture->type	= GL_RGB;											// Set Image type to GL_RGB
	else																	// Else if its 32 BPP
		texture->type	= GL_RGBA;											// Set image type to GL_RGBA
	
	tga.bytesPerPixel	= (tga.Bpp / 8);									// Compute the number of BYTES per pixel
	tga.imageSize		= (tga.bytesPerPixel * tga.Width * tga.Height);		// Compute the total amout ofmemory needed to store data
	texture->imageData	= (GLubyte *)malloc(tga.imageSize);					// Allocate that much memory
	
	if(texture->imageData == NULL)											// If no space was allocated
	{
		MessageBox(NULL, "Could not allocate memory for image", "ERROR", MB_OK);	// Display Error
		fclose(fTGA);														// Close the file
		return false;														// Return failed
	}
	
	if(fread(texture->imageData, 1, tga.imageSize, fTGA) != tga.imageSize)	// Attempt to read image data
	{
		MessageBox(NULL, "Could not read image data", "ERROR", MB_OK);		// Display Error
		if(texture->imageData != NULL)										// If imagedata has data in it
		{
			free(texture->imageData);										// Delete data from memory
		}
		fclose(fTGA);														// Close file
		return false;														// Return failed
	}
	
	// Byte Swapping Optimized By Steve Thomas
	for(GLuint cswap = 0; cswap < (int)tga.imageSize; cswap += tga.bytesPerPixel)
	{
		texture->imageData[cswap] ^= texture->imageData[cswap+2] ^= texture->imageData[cswap] ^= texture->imageData[cswap+2];
	}
	
	fclose(fTGA);															// Close file
	return true;															// Return success
}

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

void et::tga::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	ET_FAIL("Not implemented");
}

void et::tga::loadFromStream(std::istream& source, TextureDescription& desc)
{
	TGAHeader header = { };
	source.read(reinterpret_cast<char*>(header), sizeof(TGAHeader));
	
	bool compressed = false;
	if (memcmp(uTGAcompare, header, sizeof(TGAHeader)) == 0)
	{

	}
	else if (memcmp(cTGAcompare, header, sizeof(TGAHeader)) == 0)
	{
		compressed = true;
		ET_FAIL("Not implemented");
	}
	else
	{
		log::error("Invalid TGA file.");
		return;
	}
	
	TGADescription info = { };
	source.read(reinterpret_cast<char*>(&info), sizeof(TGADescription));
	
	desc.target = GL_TEXTURE_2D;
	desc.type = GL_UNSIGNED_BYTE;
	desc.size.x = info.width;
	desc.size.y = info.height;
	desc.bitsPerPixel = info.bitsPerPixel;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.channels = desc.bitsPerPixel/ 8;

	if (desc.channels == 3)
	{
		desc.internalformat = GL_RGB;
		desc.format = GL_RGB;
	}
	else if (desc.channels == 4)
	{
		desc.internalformat = GL_RGBA;
		desc.format = GL_RGBA;
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
