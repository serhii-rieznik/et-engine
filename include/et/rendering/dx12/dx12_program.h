/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/program.h>

namespace et
{
    class Camera;
    class DX12Program : public Program
    {
    public:
        ET_DECLARE_POINTER(DX12Program);

    public:
        void build(const std::string& source) override;
    };
}
