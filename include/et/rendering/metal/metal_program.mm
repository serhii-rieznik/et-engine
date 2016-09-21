/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal.h>

namespace et
{
class MetalProgramPrivate
{
public:
	MetalProgramPrivate(MetalState& mtl)
		: state(mtl) { }

	MetalState& state;
    MetalNativeProgram program;
};

MetalProgram::MetalProgram(MetalState& state)
{
    ET_PIMPL_INIT(MetalProgram, state)
}

MetalProgram::~MetalProgram()
{
	ET_PIMPL_FINALIZE(MetalProgram)
}
    
void MetalProgram::build(const std::string& vertexSource, const std::string& fragmentSource)
{
	std::string fusedSource = "using namespace metal;\n" + vertexSource + "\n" + fragmentSource;
	NSError* error = nil;

	id<MTLLibrary> lib = [_private->state.device newLibraryWithSource:[NSString stringWithUTF8String:fusedSource.c_str()]
													 options:nil error:&error];
	if (error != nil)
	{
		log::error("Failed to compile Metal shader:\n%s", [[error description] UTF8String]);
	}
	else
	{
		ET_ASSERT(lib != nil);
		for (NSString* functionName in lib.functionNames)
		{
			id<MTLFunction> func = [lib newFunctionWithName:functionName];
			if (func.functionType == MTLFunctionTypeVertex)
			{
				_private->program.vertexFunction = func;
			}
			else if (func.functionType == MTLFunctionTypeFragment)
			{
				_private->program.fragmentFunction = func;
			}
		}
	}

	ET_ASSERT(_private->program.vertexFunction != nil);
	ET_ASSERT(_private->program.fragmentFunction != nil);

	ET_OBJC_RELEASE(lib);
}
    
const MetalNativeProgram& MetalProgram::nativeProgram() const
{
    return _private->program;
}

void MetalProgram::setTransformMatrix(const mat4 &m, bool force)
{
}

void MetalProgram::setCameraProperties(const Camera& cam)
{
}

void MetalProgram::setDefaultLightPosition(const vec3& p, bool force)
{
}
    
}
