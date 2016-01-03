/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/renderstate.h>
#include <et/rendering/materialfactory.h>

using namespace et;

namespace
{
	const std::string kVertexSource = "vertex-source";
	const std::string kFragmentSource = "fragment-source";
	const std::string kDefines = "defines";
}

Material::Material(MaterialFactory* mf) :
	_factory(mf)
{
	
}

void Material::loadFromJson(const std::string& jsonString, const std::string& baseFolder, ObjectsCache& cache)
{
	Dictionary obj;
	if (obj.loadFromJson(jsonString) == false)
	{
		log::error("Unable to load material from json.");
		return;
	}
	
	application().pushSearchPath(baseFolder);
	
	auto name = obj.stringForKey(kName)->content;
	auto cullMode = obj.stringForKey(kCullMode)->content;
	auto definesArray = obj.arrayForKey(kDefines);
	auto vertexSource = application().resolveFileName(obj.stringForKey(kVertexSource)->content);
	auto fragmentSource = application().resolveFileName(obj.stringForKey(kFragmentSource)->content);
	
	application().popSearchPaths();
	
	StringList defines;
	defines.reserve(definesArray->content.size());
	
	auto addDefine = [&defines](std::string& define)
	{
		if (!define.empty())
		{
			std::transform(define.begin(), define.end(), define.begin(), [](char c)
				{ return (c == '=') ? ' ' : c; });
			defines.push_back("#define " + define + "\n");
		}
	};
	
	for (auto def : definesArray->content)
	{
		if (def->valueClass() == ValueClass_String)
		{
			addDefine(StringValue(def)->content);
		}
	}
	
	_program = _factory->genProgram(name, loadTextFile(vertexSource), loadTextFile(fragmentSource), defines, baseFolder);
	_blend = deserializeBlendState(obj.dictionaryForKey(kBlendState));
	_depth = deserializeDepthState(obj.dictionaryForKey(kDepthState));
	
	if (stringToCullMode(cullMode, _cullMode) == false)
	{
		log::warning("Invalid or unsupported cull mode in material: %s", cullMode.c_str());
	}
}

void Material::enableInRenderState(RenderState& rs)
{
	rs.setCulling(_cullMode);
	rs.setBlendState(_blend);
	rs.setDepthState(_depth);
	
	rs.bindProgram(_program);
}
