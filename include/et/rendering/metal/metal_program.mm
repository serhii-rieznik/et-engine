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
    
void MetalProgram::build(const std::string& source)
{
	NSError* error = nil;
	id<MTLLibrary> lib = [_private->state.device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];

	if ((lib == nil) && (error != nil))
	{
		log::error("Failed to compile Metal shader:\n%s", [[error description] UTF8String]);
	}
	else
	{
		if (error != nil)
		{
			log::error("Metal shader compile report:\n%s", [[error description] UTF8String]);
		}

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

	if ((_private->program.vertexFunction == nil) || (_private->program.fragmentFunction == nil))
	{
		log::error("Shader source code does not contain all required functions:\n%s", source.c_str());
	}

	ET_ASSERT(_private->program.vertexFunction != nil);
	ET_ASSERT(_private->program.fragmentFunction != nil);

	ET_OBJC_RELEASE(lib);
}
    
const MetalNativeProgram& MetalProgram::nativeProgram() const
{
    return _private->program;
}

}
