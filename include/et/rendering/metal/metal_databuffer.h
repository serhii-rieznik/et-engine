//
//  metal_databuffer.hpp
//  et-static-mac
//
//  Created by Sergey Reznik on 9/27/16.
//  Copyright © 2016 Cheetek. All rights reserved.
//

#pragma once

#include <et/rendering/interface/databuffer.h>
#include <et/core/containers.h>

namespace et
{

struct MetalState;
class MetalNativeBuffer;
class MetalDataBufferPrivate;
class MetalDataBuffer : public DataBuffer
{
public:
	ET_DECLARE_POINTER(MetalDataBuffer);

public:
	MetalDataBuffer(MetalState&, const BinaryDataStorage&);
	MetalDataBuffer(MetalState&, uint32_t size);
	~MetalDataBuffer();

	void setData(const void* ptr, uint32_t offset, uint32_t size) override;
	uint32_t size() const override;

	const MetalNativeBuffer& nativeBuffer() const;

private:
	ET_DECLARE_PIMPL(MetalDataBuffer, 32)
};

}
