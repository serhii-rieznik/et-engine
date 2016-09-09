/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/base64.h>

namespace et
{

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline bool is_base64(unsigned char c)
	{ return (isalnum(c) || (c == '+') || (c == '/')); }

uint32_t et::base64::decodedDataSize(const std::string& encoded_string)
{
	uint32_t result = 0;
	
	size_t in_len = encoded_string.size();
	
	int i = 0;
	int j = 0;
	int in_ = 0;
	
	unsigned char char_array_4[4] = { };
	unsigned char char_array_3[3] = { };
	
	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;
			
			char_array_3[0] = (((char_array_4[0] & 0xff) << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
			char_array_3[1] = (((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
			char_array_3[2] = (((char_array_4[2] & 0x03) << 6) + char_array_4[3]) & 0xff;
			
			for (i = 0; (i < 3); i++)
				++result;
			
			i = 0;
		}
	}
	
	if (i > 0)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;
		
		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;
		
		char_array_3[0] = (((char_array_4[0] & 0xff) << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
		char_array_3[1] = (((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
		char_array_3[2] = (((char_array_4[2] & 0x03) << 6) + char_array_4[3]) & 0xff;
		
		for (j = 0; (j < i - 1); j++)
			++result;
	}
	
	return result;
}

BinaryDataStorage et::base64::decode(const std::string& encoded_string)
{
	BinaryDataStorage result(decodedDataSize(encoded_string), 0);
	
	size_t in_len = encoded_string.size();
	
	int i = 0;
	int j = 0;
	int in_ = 0;
	
	unsigned char char_array_4[4] = { };
	unsigned char char_array_3[3] = { };
	
	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;
			
			char_array_3[0] = (((char_array_4[0] & 0xff) << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
			char_array_3[1] = (((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
			char_array_3[2] = (((char_array_4[2] & 0x03) << 6) + char_array_4[3]) & 0xff;
			
			for (i = 0; (i < 3); i++)
				result.push_back(char_array_3[i]);
			
			i = 0;
		}
	}
	
	if (i > 0)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;
		
		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;
		
		char_array_3[0] = (((char_array_4[0] & 0xff) << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
		char_array_3[1] = (((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
		char_array_3[2] = (((char_array_4[2] & 0x03) << 6) + char_array_4[3]) & 0xff;
		
		for (j = 0; (j < i - 1); j++)
			result.push_back(char_array_3[j]);
	}
	
	return result;
}

std::string base64::encode(const BinaryDataStorage& bytes_to_encode)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	uint32_t n = 0;
	size_t in_len = bytes_to_encode.size();
	while (in_len--)
	{
		char_array_3[i++] = bytes_to_encode[n++];
		
		if (i == 3)
		{
			char_array_4[0] = ((char_array_3[0] & 0xfc) >> 2) & 0xff;
			char_array_4[1] = (((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4)) & 0xff;
			char_array_4[2] = (((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6)) & 0xff;
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = ((char_array_3[0] & 0xfc) >> 2) & 0xff;
		char_array_4[1] = (((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4)) & 0xff;
		char_array_4[2] = (((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6)) & 0xff;
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];
		
		while((i++ < 3))
			ret += '=';
	}
	
	return ret;
	
}

}