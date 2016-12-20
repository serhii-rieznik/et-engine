//
//  metal_databuffer.hpp
//  et-static-mac
//
//  Created by Sergey Reznik on 9/27/16.
//  Copyright © 2016 Cheetek. All rights reserved.
//

#pragma once

#include <et/rendering/interface/buffer.h>
#include <et/core/containers.h>

namespace et
{
struct MetalState;
class MetalBufferPrivate;
class MetalBuffer : public Buffer
{
public:
	ET_DECLARE_POINTER(MetalBuffer);

public:
	MetalBuffer(MetalState&, const Description&);
	~MetalBuffer();

	uint8_t* map(uint32_t begin, uint32_t length) override;
	void modifyRange(uint64_t begin, uint64_t length) override;
	void unmap() override;

	bool mapped() const override;

	void updateData(uint32_t offset, const BinaryDataStorage&) override;
	void transferData(Buffer::Pointer destination) override;

	uint32_t size() const override;

private:
	ET_DECLARE_PIMPL(MetalBuffer, 32);
};

}
