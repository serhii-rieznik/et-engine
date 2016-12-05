/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/buffer.h>

namespace et
{
class DX12;
class DX12BufferPrivate;
class DX12Buffer : public Buffer
{
public:
	ET_DECLARE_POINTER(DX12Buffer);

public:
	DX12Buffer(DX12&, const Description& desc);
	~DX12Buffer();

	uint8_t* map(uint32_t begin, uint32_t length);
	void modifyRange(uint64_t begin, uint64_t length);
	void unmap();

	bool mapped() const;

	void updateData(uint32_t offset, const BinaryDataStorage&);
	void transferData(Buffer::Pointer destination);

	uint32_t size() const;

private:
	ET_DECLARE_PIMPL(DX12Buffer, 128);
};
}
