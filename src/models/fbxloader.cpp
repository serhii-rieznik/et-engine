/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <assert.h>
#include <fbxsdk.h>

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/apiobjects/vertexbuffer.h>
#include <et/vertexbuffer/IndexArray.h>
#include <et/primitives/primitives.h>
#include <et/resources/textureloader.h>
#include <et/scene3d/supportmesh.h>
#include <et/models/fbxloader.h>

using namespace FBXSDK_NAMESPACE;

const std::string s_supportMeshProperty = "support=true";
const std::string s_collisionMeshProperty = "collision=true";
const std::string s_lodMeshProperty = "lod=";

namespace et
{
	class FBXLoaderPrivate
	{
	private:
		RenderContext* _rc;
		ObjectsCache& _texCache;
		std::string _folder;

	public:
		FbxManager* manager;
		FbxImporter* importer;
		FbxScene* scene;

		s3d::Scene3dStorage::Pointer storage;
		
	public:
		FBXLoaderPrivate(RenderContext* rc, ObjectsCache& ObjectsCache);
		~FBXLoaderPrivate();

		s3d::ElementContainer::Pointer parse();

		bool import(const std::string& filename);
		void loadTextures();
		void loadNode(FbxNode* node, s3d::Element::Pointer parent);
		void buildVertexBuffers(RenderContext* rc, s3d::Element::Pointer root);

		s3d::Mesh::Pointer loadMesh(FbxMesh* mesh, s3d::Element::Pointer parent,
			const s3d::Material::List& materials, const StringList& params);

		s3d::Material loadMaterial(FbxSurfaceMaterial* material);

		void loadMaterialValue(s3d::Material& m, size_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);
		
		void loadMaterialTextureValue(s3d::Material& m, size_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);

		StringList loadNodeProperties(FbxNode* node);
	};
}

using namespace et;

/* 
 *
 * Private implementation
 *
 */

FBXLoaderPrivate::FBXLoaderPrivate(RenderContext* rc, ObjectsCache& ObjectsCache) :
	manager(FbxManager::Create()), _rc(rc), _texCache(ObjectsCache)
{
	scene = FbxScene::Create(manager, 0);
}

FBXLoaderPrivate::~FBXLoaderPrivate()
{
	if (manager)
		manager->Destroy();
}

bool FBXLoaderPrivate::import(const std::string& filename)
{
	_folder = getFilePath(filename);

	int lFileMajor = 0;
	int lFileMinor = 0;
	int lFileRevision = 0;
	int lSDKMajor = 0;
	int lSDKMinor = 0;
	int lSDKRevision = 0;;

	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);
	importer = FbxImporter::Create(manager, 0);
	bool status = importer->Initialize(filename.c_str(), -1, manager->GetIOSettings());
	importer->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!status)
	{
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n", importer->GetStatus().GetErrorString());

		if (importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			printf("FBX version number for this FBX SDK is %d.%d.%d\n", lSDKMajor,
				lSDKMinor, lSDKRevision);
			
			printf("FBX version number for file %s is %d.%d.%d\n\n", filename.c_str(),
				lFileMajor, lFileMinor, lFileRevision);
		}
		
		importer->Destroy();
		return false;
	}

	status = importer->IsFBX();
	if (!status)
	{
		log::error("FBXLoader error: %s is not an FBX file", filename.c_str());
		importer->Destroy();
		return false;
	}

	status = importer->Import(scene);
	if (!status)
	{
		log::error("FBXLoader error: unable to import scene from from %s", filename.c_str());
		importer->Destroy();
		return false;
	}

	importer->Destroy();
	return true;
}

s3d::ElementContainer::Pointer FBXLoaderPrivate::parse()
{
	s3d::ElementContainer::Pointer result(new s3d::ElementContainer("_fbx", 0));
	storage = s3d::Scene3dStorage::Pointer(new s3d::Scene3dStorage("_fbx_storage", result.ptr()));

	FbxAxisSystem targetAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd,
		FbxAxisSystem::eRightHanded);

	int upAxis = scene->GetGlobalSettings().GetOriginalUpAxis();
	if (upAxis != 1)
		targetAxisSystem.ConvertScene(scene);

	FbxSystemUnit sceneSystemUnit = scene->GetGlobalSettings().GetOriginalSystemUnit();
	if (sceneSystemUnit.GetScaleFactor() != 1.0f)
		FbxSystemUnit::cm.ConvertScene(scene);

	FbxNode* root = scene->GetRootNode(); 
	root->ResetPivotSetAndConvertAnimation();

	loadTextures();
	loadNode(root, result);
	buildVertexBuffers(_rc, result);

	return result;
}

void FBXLoaderPrivate::loadTextures()
{
	int textures = scene->GetTextureCount();
	for (int i = 0; i < textures; ++i)
	{
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(scene->GetTexture(i));
		if (fileTexture && !fileTexture->GetUserDataPtr())
		{
			std::string fileName = normalizeFilePath(std::string(fileTexture->GetFileName()));
			
			std::string ext = lowercase(getFileExt(fileName));
			if ((ext != ".png") && (ext != ".dds") && (ext != ".pvr"))
			{
				std::string basePath = getFilePath(fileName);
				std::string baseName = removeFileExt(getFileName(fileName));
				
				if (fileExists(baseName + ".pvr"))
					fileName = baseName + ".pvr";
				if (fileExists(basePath + baseName + ".pvr"))
					fileName = basePath + baseName + ".pvr";
				if (fileExists(_folder + baseName + ".pvr"))
					fileName = _folder + baseName + ".pvr";

				if (fileExists(baseName + ".dds"))
					fileName = baseName + ".dds";
				if (fileExists(basePath + baseName + ".dds"))
					fileName = basePath + baseName + ".dds";
				if (fileExists(_folder + baseName + ".dds"))
					fileName = _folder + baseName + ".dds";
				
				if (fileExists(baseName + ".png"))
					fileName = baseName + ".png";
				if (fileExists(basePath + baseName + ".png"))
					fileName = basePath + baseName + ".png";
				if (fileExists(_folder + baseName + ".png"))
					fileName = _folder + baseName + ".png";
			}
						
			Texture texture;
			if (fileExists(fileName))
			{
				texture = _rc->textureFactory().loadTexture(fileName, _texCache);
				texture->setOrigin(getFileName(fileName));
			}
			else
			{
				BinaryDataStorage randomTextureData(4, 255);
				for (size_t i = 0; i < 3; ++i)
					randomTextureData[i] = rand() % 256;
				
				texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
					GL_UNSIGNED_BYTE, randomTextureData, fileName);
				texture->setOrigin(fileName);
				
				_texCache.manage(texture, _rc->textureFactoryPointer());
			}
			fileTexture->SetUserDataPtr(texture.ptr());
		}
	}
			
/*
			bool shouldLoad = false;
			
			std::string originalName;
			std::string fileName = normalizeFilePath(std::string(fileTexture->GetFileName()));
			
			if (fileExists(fileName))
			{
				originalName = getFileName(fileName);
				shouldLoad = true;
			}
			else if (getFileExt(fileName).size())
			{
				originalName = removeFileExt(getFileName(fileName));
	
				fileName = application().environment().resolveScalableFileName(_folder + originalName, _rc->screenScaleFactor());
				shouldLoad = fileExists(fileName);

				if (!shouldLoad)
					log::warning("Unable to resolve file name for texture:\n\t%s", fileName.c_str());
			}

			if (shouldLoad)
			{
				Texture texture = _rc->textureFactory().loadTexture(fileName, _texCache);
				if (texture.valid())
				{
					texture->setOrigin(originalName);
					storage->addTexture(texture);
					fileTexture->SetUserDataPtr(texture.ptr());
				}
			}
*/
}

void FBXLoaderPrivate::loadNode(FbxNode* node, s3d::Element::Pointer parent)
{
	s3d::Material::List materials;
	StringList props = loadNodeProperties(node);

	const int lMaterialCount = node->GetMaterialCount();
	for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
	{
		FbxSurfaceMaterial* lMaterial = node->GetMaterial(lMaterialIndex);
		s3d::MaterialData* storedMaterial = static_cast<s3d::MaterialData*>(lMaterial->GetUserDataPtr());
		if (storedMaterial == 0)
		{
			s3d::Material m = loadMaterial(lMaterial);
			materials.push_back(m);
			storage->addMaterial(m);
			lMaterial->SetUserDataPtr(m.ptr());
		}
		else
		{
			materials.push_back(s3d::Material(storedMaterial));
		}
	}

	s3d::Element::Pointer createdElement;

	FbxNodeAttribute* lNodeAttribute = node->GetNodeAttribute();
	if (lNodeAttribute)
	{
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh* mesh = node->GetMesh();

			if (!mesh->IsTriangleMesh())
			{
				log::info("Triangulating %s ...", node->GetName());
				FbxGeometryConverter lConverter(node->GetFbxManager());
				lConverter.Triangulate(node->GetScene(), true);
				mesh = node->GetMesh();
			}
		
			s3d::Mesh* storedElement = static_cast<s3d::Mesh*>(mesh->GetUserDataPtr());

			if (storedElement == nullptr)
			{
				createdElement = loadMesh(mesh, parent, materials, props);
				mesh->SetUserDataPtr(createdElement.ptr());
			}
			else
			{
				s3d::Mesh* instance = nullptr;
				if (storedElement->type() == s3d::ElementType_Mesh)
					 instance = storedElement->duplicate();
				else if (storedElement->type() == s3d::ElementType_SupportMesh)
					instance = static_cast<s3d::SupportMesh*>(storedElement->duplicate());
				createdElement.reset(instance);
			}
		}
	}

	if (createdElement.invalid())
	{
		createdElement =
			s3d::ElementContainer::Pointer(new s3d::ElementContainer(node->GetName(), parent.ptr()));
	}

	const FbxMatrix& fbxTransform = node->EvaluateLocalTransform();
	mat4 transform;
	for (int v = 0; v < 4; ++v)
	{
		for (int u = 0; u < 4; ++u)
			transform[v][u] = static_cast<float>(fbxTransform.Get(v, u));
	}

	createdElement->setTransform(transform);
	createdElement->setName(node->GetName());
	
	ET_ITERATE(props, auto, p,
	{
		if (!createdElement->hasPropertyString(p))
			createdElement->addPropertyString(p);
	})

	int lChildCount = node->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		loadNode(node->GetChild(lChildIndex), createdElement);
}

void FBXLoaderPrivate::loadMaterialTextureValue(s3d::Material& m, size_t propName,
	FbxSurfaceMaterial* fbxm, const char* fbxprop)
{
	FbxProperty value = fbxm->FindProperty(fbxprop);
	if (value.IsValid())
	{
		int lTextureCount = value.GetSrcObjectCount<FbxFileTexture>();
		if (lTextureCount)
		{
			FbxFileTexture* lTexture = value.GetSrcObject<FbxFileTexture>(0);
			if (lTexture && lTexture->GetUserDataPtr())
			{
				TextureData* ptr = reinterpret_cast<TextureData*>(lTexture->GetUserDataPtr());
				m->setTexture(propName, Texture(ptr));
			} 
		}
	}
}

void FBXLoaderPrivate::loadMaterialValue(s3d::Material& m, size_t propName,
	FbxSurfaceMaterial* fbxm, const char* fbxprop)
{
	const FbxProperty value = fbxm->FindProperty(fbxprop);
	if (!value.IsValid()) return;

	EFbxType dataType = value.GetPropertyDataType().GetType();

	if (dataType == eFbxFloat)
	{
		m->setFloat(propName, value.Get<float>());
	}
	else if (dataType == eFbxDouble)
	{
		m->setFloat(propName, static_cast<float>(value.Get<double>()));
	}
	else if (dataType == eFbxDouble2)
	{
		FbxDouble2 data = value.Get<FbxDouble2>();
		m->setVector(propName, vec4(static_cast<float>(data[0]), static_cast<float>(data[1]), 0.0f, 0.0f));
	}
	else if (dataType == eFbxDouble3)
	{
		FbxDouble3 data = value.Get<FbxDouble3>();
		m->setVector(propName, vec4(static_cast<float>(data[0]), static_cast<float>(data[1]),
			static_cast<float>(data[2]), 1.0f));
	}
	else if (dataType == eFbxDouble4)
	{
		FbxDouble3 data = value.Get<FbxDouble4>();
		m->setVector(propName, vec4(static_cast<float>(data[0]), static_cast<float>(data[1]),
			static_cast<float>(data[2]), static_cast<float>(data[3])));
	}
	else if (dataType == eFbxString)
	{
		m->setString(propName, value.Get<FbxString>().Buffer());
	}
	else
	{
		log::warning("Unsupported data type %d for %s", dataType, fbxprop);
	}
}

s3d::Material FBXLoaderPrivate::loadMaterial(FbxSurfaceMaterial* mat)
{
	s3d::Material m;

	m->setName(mat->GetName());

	loadMaterialTextureValue(m, MaterialParameter_DiffuseMap, mat, FbxSurfaceMaterial::sDiffuse);
	loadMaterialTextureValue(m, MaterialParameter_AmbientMap, mat, FbxSurfaceMaterial::sAmbient);
	loadMaterialTextureValue(m, MaterialParameter_EmissiveMap, mat, FbxSurfaceMaterial::sEmissive);
	loadMaterialTextureValue(m, MaterialParameter_SpecularMap, mat, FbxSurfaceMaterial::sSpecular);
	loadMaterialTextureValue(m, MaterialParameter_NormalMap, mat, FbxSurfaceMaterial::sNormalMap);
	loadMaterialTextureValue(m, MaterialParameter_BumpMap, mat, FbxSurfaceMaterial::sBump);
	loadMaterialTextureValue(m, MaterialParameter_ReflectionMap, mat, FbxSurfaceMaterial::sReflection);

	loadMaterialValue(m, MaterialParameter_AmbientColor, mat, FbxSurfaceMaterial::sAmbient);
	loadMaterialValue(m, MaterialParameter_DiffuseColor, mat, FbxSurfaceMaterial::sDiffuse);
	loadMaterialValue(m, MaterialParameter_SpecularColor, mat, FbxSurfaceMaterial::sSpecular);
	loadMaterialValue(m, MaterialParameter_EmissiveColor, mat, FbxSurfaceMaterial::sEmissive);
	
	loadMaterialValue(m, MaterialParameter_TransparentColor, mat, FbxSurfaceMaterial::sTransparentColor);

	loadMaterialValue(m, MaterialParameter_AmbientFactor, mat, FbxSurfaceMaterial::sAmbientFactor);
	loadMaterialValue(m, MaterialParameter_DiffuseFactor, mat, FbxSurfaceMaterial::sDiffuseFactor);
	loadMaterialValue(m, MaterialParameter_SpecularFactor, mat, FbxSurfaceMaterial::sSpecularFactor);
	loadMaterialValue(m, MaterialParameter_BumpFactor, mat, FbxSurfaceMaterial::sBumpFactor);
	loadMaterialValue(m, MaterialParameter_ReflectionFactor, mat, FbxSurfaceMaterial::sReflectionFactor);

	loadMaterialValue(m, MaterialParameter_Roughness, mat, FbxSurfaceMaterial::sShininess);
	loadMaterialValue(m, MaterialParameter_Transparency, mat, FbxSurfaceMaterial::sTransparencyFactor);

	loadMaterialValue(m, MaterialParameter_ShadingModel, mat, FbxSurfaceMaterial::sShadingModel);

	return m;
}

s3d::Mesh::Pointer FBXLoaderPrivate::loadMesh(FbxMesh* mesh, s3d::Element::Pointer parent, 
	const s3d::Material::List& materials, const StringList& params)
{
	const char* mName = mesh->GetName();
	const char* nName = mesh->GetNode()->GetName();
	std::string meshName(strlen(mName) == 0 ? nName : mName);

	size_t lodIndex = 0;
	bool support = false;
	ET_ITERATE(params, auto, p,
	{
		lowercase(p);
		p.erase(std::remove_if(p.begin(), p.end(), [](char c){ return isWhitespaceChar(c); } ), p.end());

		if ((p.find(s_collisionMeshProperty) == 0) || (p.find(s_supportMeshProperty) == 0))
		{
			support = true;
			break;
		}

		if (p.find_first_of(s_lodMeshProperty) == 0)
		{
			std::string prop = p.substr(s_lodMeshProperty.size());
			lodIndex = strToInt(trim(prop));
		}
	})

	int lPolygonCount = mesh->GetPolygonCount();
	int lPolygonVertexCount = lPolygonCount * 3;

	bool isContainer = false;
	int numMaterials = 0;

	FbxGeometryElementMaterial* material = mesh->GetElementMaterial();
	FbxLayerElementArrayTemplate<int>* materialIndices = 0;
	if (material)
	{
		numMaterials = 1;
		FbxGeometryElement::EMappingMode mapping = material->GetMappingMode();
		assert((mapping == FbxGeometryElement::eAllSame) || (mapping == FbxGeometryElement::eByPolygon));

		isContainer = mapping == FbxGeometryElement::eByPolygon;
		if (isContainer)
		{
			materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
			assert(materialIndices->GetCount() == lPolygonCount);
			for (int i = 0; i < materialIndices->GetCount(); ++i)
				numMaterials = etMax(numMaterials, materialIndices->GetAt(i) + 1);
		}
	}

	s3d::Mesh::Pointer element;
	if (support)
		element = s3d::SupportMesh::Pointer(new s3d::SupportMesh(meshName, parent.ptr()));
	else
		element = s3d::Mesh::Pointer(new s3d::Mesh(meshName, parent.ptr()));
	
	size_t uvChannels = mesh->GetElementUVCount();

	bool hasNormal = mesh->GetElementNormalCount() > 0;
	bool hasTangents = mesh->GetElementTangentCount() > 0;
	bool hasSmoothingGroups = mesh->GetElementSmoothingCount() > 0;
	bool hasColor = mesh->GetElementVertexColorCount() > 0;

	const FbxVector4* lControlPoints = mesh->GetControlPoints();
	FbxGeometryElementTangent* tangents = hasTangents ? mesh->GetElementTangent() : nullptr;
	FbxGeometryElementSmoothing* smoothing = hasSmoothingGroups ? mesh->GetElementSmoothing() : nullptr;
	FbxGeometryElementVertexColor* vertexColor = hasColor ? mesh->GetElementVertexColor() : nullptr;

	if (hasNormal)
		hasNormal = mesh->GetElementNormal()->GetMappingMode() != FbxGeometryElement::eNone;

	if (hasTangents)
		hasTangents = tangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex;

	if (hasSmoothingGroups)
		hasSmoothingGroups = smoothing->GetMappingMode() == FbxGeometryElement::eByPolygon;

	if (hasColor)
		hasColor = vertexColor->GetMappingMode() == FbxGeometryElement::eByPolygonVertex;

	VertexDeclaration decl(true, Usage_Position, Type_Vec3);

	if (hasNormal)
		decl.push_back(Usage_Normal, Type_Vec3);

	if (hasTangents)
		decl.push_back(Usage_Tangent, Type_Vec3);

	if (hasColor)
		decl.push_back(Usage_Color, Type_Vec4);
	
	if (mesh->GetElementUV()->GetMappingMode() != FbxGeometryElement::eNone)
	{
		for (size_t i = 0; i < uvChannels; ++i)
			decl.push_back(static_cast<VertexAttributeUsage>(Usage_TexCoord0 + i), Type_Vec2);
	}

	VertexArray::Pointer va = storage->vertexArrayWithDeclarationForAppendingSize(decl, lPolygonVertexCount);
	int vbIndex = storage->indexOfVertexArray(va);
	size_t vertexBaseOffset = va->size();
	va->increase(lPolygonVertexCount);

	IndexArray::Pointer ib = storage->indexArray();
	ib->resizeToFit(ib->actualSize() + lPolygonCount * 3);

	RawDataAcessor<vec3> pos = va->chunk(Usage_Position).accessData<vec3>(vertexBaseOffset);
	RawDataAcessor<vec3> nrm = va->chunk(Usage_Normal).accessData<vec3>(vertexBaseOffset);
	RawDataAcessor<vec3> tang = va->chunk(Usage_Tangent).accessData<vec3>(vertexBaseOffset);
	RawDataAcessor<vec4> clr = va->chunk(Usage_Color).accessData<vec4>(vertexBaseOffset);
	RawDataAcessor<int> smooth = va->smoothing().accessData<int>(vertexBaseOffset);
	
	std::vector<RawDataAcessor<vec2>> uvs;
	for (size_t i = 0; i < uvChannels; ++i)
		uvs.push_back(va->chunk(static_cast<VertexAttributeUsage>(Usage_TexCoord0 + i)).accessData<vec2>(vertexBaseOffset));

	FbxStringList lUVNames;
	mesh->GetUVSetNames(lUVNames);

	size_t vertexCount = 0;
	size_t indexOffset = vertexBaseOffset;

#define ET_FBX_LOADER_PUSH_VERTEX FbxVector4 v = lControlPoints[lControlPointIndex]; \
		pos[vertexCount] = vec3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));

#define ET_FBX_LOADER_PUSH_NORMAL if (hasNormal) { \
		FbxVector4 n; mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, n); \
		nrm[vertexCount] = vec3(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2])); }

#define ET_FBX_LOADER_PUSH_UV \
	for (size_t i = 0; i < uvChannels; ++i) \
	{ \
		FbxVector2 t; \
		bool unmap = false; \
		mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVNames[static_cast<int>(i)].Buffer(), t, unmap); \
		uvs[i][vertexCount] = vec2(static_cast<float>(t[0]), static_cast<float>(t[1])); \
	}

#define ET_FBX_LOADER_PUSH_TANGENT if (hasTangents) { FbxVector4 t; \
		if (tangents->GetReferenceMode() == FbxGeometryElement::eDirect) \
			t = tangents->GetDirectArray().GetAt(static_cast<int>(vertexCount)); \
		else if (tangents->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) \
			t = tangents->GetDirectArray().GetAt(tangents->GetIndexArray().GetAt(static_cast<int>(vertexCount))); \
		tang[vertexCount] = vec3(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])); }

#define ET_FBX_LOADER_PUSH_COLOR if (hasColor) { FbxColor c; \
		if (vertexColor->GetReferenceMode() == FbxGeometryElement::eDirect) \
			c = vertexColor->GetDirectArray().GetAt(static_cast<int>(vertexCount)); \
		else if (vertexColor->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) \
			c = vertexColor->GetDirectArray().GetAt(vertexColor->GetIndexArray().GetAt(static_cast<int>(vertexCount))); \
		clr[vertexCount] = vec4(static_cast<float>(c.mRed), static_cast<float>(c.mGreen), \
			static_cast<float>(c.mBlue), static_cast<float>(c.mAlpha)); }

	if (isContainer)
	{
		for (size_t m = 0; m < materials.size(); ++m)
		{
			s3d::Element* aParent = (m == 0) ? parent.ptr() : element.ptr();
			std::string aName = (m == 0) ? meshName : (meshName + "~" + materials.at(m)->name());

			s3d::Mesh::Pointer meshElement;
			if (m == 0)
			{
				meshElement = element;
			}
			else
			{
				if (support)
					meshElement = s3d::SupportMesh::Pointer(new s3d::SupportMesh(aName, aParent));
				else
					meshElement = s3d::Mesh::Pointer(new s3d::Mesh(aName, aParent));
			}

			meshElement->tag = vbIndex;
			meshElement->setStartIndex(static_cast<IndexType>(indexOffset));
			meshElement->setMaterial(materials.at(m));
			
			ET_ITERATE(aParent->properties(), auto, prop, meshElement->addPropertyString(prop))

			for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
			{
				if (materialIndices->GetAt(lPolygonIndex) == m)
				{
					for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
					{
						const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
						ib->setIndex(static_cast<IndexType>(vertexBaseOffset + vertexCount), indexOffset);

						ET_FBX_LOADER_PUSH_VERTEX
						ET_FBX_LOADER_PUSH_NORMAL
						ET_FBX_LOADER_PUSH_UV
						ET_FBX_LOADER_PUSH_TANGENT
						ET_FBX_LOADER_PUSH_COLOR

						++vertexCount;
						++indexOffset;
					}
				}
			}

			meshElement->setNumIndexes(indexOffset - meshElement->startIndex());
			if ((lodIndex > 0) && (parent->type() == s3d::ElementType_Mesh))
			{
				s3d::Mesh::Pointer p = parent;
				p->attachLod(lodIndex, meshElement);
			}
		}
	}
	else
	{
		s3d::Mesh* me = static_cast<s3d::Mesh*>(element.ptr());

		me->tag = vbIndex;
		me->setStartIndex(static_cast<IndexType>(vertexBaseOffset));
		me->setNumIndexes(lPolygonVertexCount);

		if (materials.size())
			me->setMaterial(materials.front());

		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
		{
			for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
			{
				const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				ib->setIndex(static_cast<IndexType>(vertexBaseOffset + vertexCount), indexOffset);
				
				ET_FBX_LOADER_PUSH_VERTEX
				ET_FBX_LOADER_PUSH_NORMAL
				ET_FBX_LOADER_PUSH_UV
				ET_FBX_LOADER_PUSH_TANGENT
				ET_FBX_LOADER_PUSH_COLOR

				if (hasSmoothingGroups)
				{
					int sgIndex = 0;
					if (smoothing->GetReferenceMode() == FbxGeometryElement::eDirect)
						sgIndex = lPolygonIndex;
					else if (smoothing->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						sgIndex = smoothing->GetIndexArray().GetAt(static_cast<int>(vertexCount));
					sgIndex = smoothing->GetDirectArray().GetAt(sgIndex);
					smooth[vertexCount] = sgIndex;
				}

				++vertexCount;
				++indexOffset;
			}
		}

		if ((lodIndex > 0) && (parent->type() == s3d::ElementType_Mesh))
		{
			s3d::Mesh::Pointer p = parent;
			p->attachLod(lodIndex, element);
		}
	}

#undef ET_FBX_LOADER_PUSH_VERTEX
#undef ET_FBX_LOADER_PUSH_NORMAL
#undef ET_FBX_LOADER_PUSH_UV
#undef ET_FBX_LOADER_PUSH_TANGENT
#undef ET_FBX_LOADER_PUSH_COLOR

	return element;
}

void FBXLoaderPrivate::buildVertexBuffers(RenderContext* rc, s3d::Element::Pointer root)
{
	IndexBuffer primaryIndexBuffer =
		rc->vertexBufferFactory().createIndexBuffer("fbx-i", storage->indexArray(), BufferDrawType_Static);

	std::vector<VertexArrayObject> vertexArrayObjects;
	VertexArrayList& vertexArrays = storage->vertexArrays();
	ET_ITERATE(vertexArrays, auto, i,
	{
		VertexArrayObject vao = rc->vertexBufferFactory().createVertexArrayObject("fbx-vao");
		VertexBuffer vb = rc->vertexBufferFactory().createVertexBuffer("fbx-v", i, BufferDrawType_Static);
		vao->setBuffers(vb, primaryIndexBuffer);
		vertexArrayObjects.push_back(vao);
	})

	s3d::Element::List meshes = root->childrenOfType(s3d::ElementType_Mesh);
	ET_ITERATE(meshes, auto, i,
	{
		s3d::Mesh* mesh = static_cast<s3d::Mesh*>(i.ptr());
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
	})

	ET_ITERATE(meshes, auto, i,
	{
		s3d::Mesh* mesh = static_cast<s3d::Mesh*>(i.ptr());
		mesh->cleanupLodChildren();
	})

	meshes = root->childrenOfType(s3d::ElementType_SupportMesh);
	ET_ITERATE(meshes, auto, i,
	{
		s3d::SupportMesh* mesh = static_cast<s3d::SupportMesh*>(i.ptr());
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
		mesh->fillCollisionData(vertexArrays[mesh->tag], storage->indexArray());
	})

	rc->renderState().bindVertexArray(0);
	rc->renderState().bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	rc->renderState().bindBuffer(GL_ARRAY_BUFFER, 0);
}

StringList FBXLoaderPrivate::loadNodeProperties(FbxNode* node)
{
	StringList result;
	FbxProperty prop = node->GetFirstProperty();

	while (prop.IsValid())
	{
		if (prop.GetPropertyDataType().GetType() == eFbxString)
		{
			FbxString str = prop.Get<FbxString>();
			StringDataStorage line(str.GetLen() + 1, 0);

			char c = 0;
			const char* strData = str.Buffer();
			while ((c = *strData++))
			{
				if (isNewLineChar(c))
				{
					if (line.lastElementIndex())
						result.push_back(line.binary());
					
					line.setOffset(0);
					line.fill(0);
				}
				else
				{
					line.push_back(c);
				}
			}

			if (line.lastElementIndex())
				result.push_back(line.data());
		}

		prop = node->GetNextProperty(prop);
	};

	return result;
}

/*
 *
 * Base objects
 *
 */

FBXLoader::FBXLoader(const std::string& filename) :
	_filename(filename)
{
}

s3d::ElementContainer::Pointer FBXLoader::load(RenderContext* rc, ObjectsCache& ObjectsCache)
{
	s3d::ElementContainer::Pointer result;
	FBXLoaderPrivate* loader = new FBXLoaderPrivate(rc, ObjectsCache);

	if (loader->import(_filename))
		result = loader->parse();

	mainRunLoop().addTask(new DeletionTask<FBXLoaderPrivate>(loader), 0.0f);
	return result;
}
