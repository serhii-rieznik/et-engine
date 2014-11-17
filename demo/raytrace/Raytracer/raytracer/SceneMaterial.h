//
//  SceneMaterial.h
//  Raytracer
//
//  Created by Sergey Reznik on 16/11/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

namespace rt
{
	struct SceneMaterial
	{
	public:
		et::vec4 diffuseColor = et::vec4(1.0f);
		et::vec4 reflectiveColor = et::vec4(1.0f);
		et::vec4 emissiveColor = et::vec4(0.0f);
		
		float refractiveIndex = 0.0f;
		float roughness = 1.0f;
		
	public:
		SceneMaterial()
			{ }
		
		SceneMaterial(const et::vec4& kD, const et::vec4& kR, const et::vec4& kE, float rg, float ior = 0.0f) :
			diffuseColor(kD), reflectiveColor(kR), emissiveColor(kE), roughness(rg), refractiveIndex(ior) { }
	};
}