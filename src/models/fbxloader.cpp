/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_HAVE_FBX_SDK)

#include <fbxsdk.h>

#include <et/rendering/rendercontext.h>
#include <et/vertexbuffer/indexarray.h>
#include <et/imaging/textureloader.h>
#include <et/scene3d/supportmesh.h>
#include <et/models/fbxloader.h>

using namespace FBXSDK_NAMESPACE;

static const std::string s_supportMeshProperty = "support=true";
static const std::string s_collisionMeshProperty = "collision=true";
static const std::string s_lodMeshProperty = "lod=";
static const std::string s_AnimationProperty = "animation=";
static const float animationAnglesScale = -TO_RADIANS;

namespace et
{
	class FBXLoaderPrivate
	{
	private:
		RenderContext* _rc = nullptr;
		ObjectsCache& _texCache;
		std::string _folder;

	public:
		FbxManager* manager = nullptr;
		FbxImporter* importer = nullptr;
		FbxScene* scene = nullptr;
		FbxAnimLayer* sharedAnimationLayer = nullptr;

		s3d::BaseElement::Pointer rootNode;
		s3d::Storage::Pointer storage;
		
	public:
		FBXLoaderPrivate(RenderContext* rc, ObjectsCache& ObjectsCache);
		~FBXLoaderPrivate();

		s3d::ElementContainer::Pointer parse();

		bool import(const std::string& filename);
		
		void loadAnimations();
		void loadTextures();
		
		/* 
		 * Node loading
		 */
		void loadNode(FbxNode* node, s3d::BaseElement::Pointer parent);
		void loadNodeAnimations(FbxNode* node, s3d::BaseElement::Pointer object, const StringList& props);
		s3d::Material::List loadNodeMaterials(FbxNode* node);
		StringList loadNodeProperties(FbxNode* node);
		
		void buildVertexBuffers(RenderContext* rc, s3d::BaseElement::Pointer root);

		s3d::Mesh::Pointer loadMesh(FbxMesh* mesh, s3d::BaseElement::Pointer parent,
			const s3d::Material::List& materials, const StringList& params);

		s3d::Material::Pointer loadMaterial(FbxSurfaceMaterial* material);

		void loadMaterialValue(s3d::Material::Pointer m, uint32_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);
		
		void loadMaterialTextureValue(s3d::Material::Pointer m, uint32_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);
	};
}

using namespace et;

/* 
 * Private implementation
 */
FBXLoaderPrivate::FBXLoaderPrivate(RenderContext* rc, ObjectsCache& ObjectsCache) :
	manager(FbxManager::Create()), _rc(rc), _texCache(ObjectsCache), importer(nullptr),
	scene(FbxScene::Create(manager, 0)), sharedAnimationLayer(nullptr)
{
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
	rootNode.reset(nullptr);
	storage = s3d::Storage::Pointer::create(emptyString, nullptr);
	
	FbxAxisSystem sceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem openglAxisSystem(FbxAxisSystem::eOpenGL);
	
	if (sceneAxisSystem != openglAxisSystem)
		openglAxisSystem.ConvertScene(scene);

	FbxSystemUnit sceneSystemUnit = scene->GetGlobalSettings().GetOriginalSystemUnit();
	if (sceneSystemUnit.GetScaleFactor() != 1.0f)
		FbxSystemUnit::cm.ConvertScene(scene);
	
	FbxGeometryConverter geometryConverter(manager);
	geometryConverter.Triangulate(scene, true);
	
	scene->GetRootNode()->ResetPivotSetAndConvertAnimation();
	
	loadAnimations();
	loadTextures();
	loadNode(scene->GetRootNode(), rootNode);

	buildVertexBuffers(_rc, rootNode);
	
	storage->setName(rootNode->name());
	storage->setParent(rootNode.ptr());

	auto meshes = rootNode->childrenOfType(s3d::ElementType_Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
		mesh->calculateSupportData();

	meshes = rootNode->childrenOfType(s3d::ElementType_SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
		mesh->calculateSupportData();

	return rootNode;
}

void FBXLoaderPrivate::loadAnimations()
{
	int numStacks = scene->GetSrcObjectCount<FbxAnimStack>();
	if (numStacks > 0)
	{
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
		if (stack != nullptr)
		{
			int numLayers = stack->GetMemberCount<FbxAnimLayer>();
			if (numLayers > 0)
				sharedAnimationLayer = stack->GetMember<FbxAnimLayer>(0);
		}
	}
		/*
		FbxTimeSpan timeSpan(0, 0);
		FbxTakeInfo* takeInfo = scene->GetTakeInfo(*fbxString);
		
		if (takeInfo == nullptr)
			scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(timeSpan);
		else
			timeSpan = takeInfo->mLocalTimeSpan;
		
		float start = static_cast<float>(timeSpan.GetStart().GetSecondDouble());
		float stop = static_cast<float>(timeSpan.GetStop().GetSecondDouble());
		log::info("Animation stack: %s (from %f to %f)", fbxString->Buffer(), start, stop);
		
		}
		FbxAnimCurve* curve = scene->GetRootNode()->LclTranslation.GetCurve(layer);
		if (curve)
		{
			int keys = curve->KeyGetCount();
			for (int i = 0; i < keys; ++i)
			{
				float time = static_cast<float>(curve->KeyGetTime(i).GetSecondDouble());
				float value = curve->KeyGetValue(i);
				
				log::info("%f / %f", time, value);
			}
		}
		*/
}

void FBXLoaderPrivate::loadTextures()
{
	application().pushSearchPath(_folder);

	int textures = scene->GetTextureCount();
	for (int i = 0; i < textures; ++i)
	{
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(scene->GetTexture(i));
		if (fileTexture && !fileTexture->GetUserDataPtr())
		{
			std::string fileName = normalizeFilePath(std::string(fileTexture->GetFileName()));

			if (!fileExists(fileName))
				fileName = application().resolveFileName(getFileName(fileName));

			if (!fileExists(fileName))
				fileName = _folder + getFileName(fileName);

			Texture::Pointer texture = _rc->textureFactory().loadTexture(fileName, _texCache);
			if (texture.invalid())
			{
				log::info("Unable to load texture: %s", fileName.c_str());
				texture = _rc->textureFactory().genNoiseTexture(vec2i(128), false, fileName);
				texture->setOrigin(fileName);
				_texCache.manage(texture, _rc->textureFactory().objectLoader());
			}
			fileTexture->SetUserDataPtr(texture.ptr());
		}
	}
	application().popSearchPaths();
}

s3d::Material::List FBXLoaderPrivate::loadNodeMaterials(FbxNode* node)
{
	s3d::Material::List materials;
	const int lMaterialCount = node->GetMaterialCount();
	for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
	{
		FbxSurfaceMaterial* lMaterial = node->GetMaterial(lMaterialIndex);
		s3d::Material* storedMaterial = static_cast<s3d::Material*>(lMaterial->GetUserDataPtr());
		if (storedMaterial == nullptr)
		{
			s3d::Material::Pointer m = loadMaterial(lMaterial);
			materials.push_back(m);
			storage->addMaterial(m);
			lMaterial->SetUserDataPtr(m.ptr());
		}
		else
		{
			materials.push_back(s3d::Material::Pointer(storedMaterial));
		}
	}
	return materials;
}

void FBXLoaderPrivate::loadNodeAnimations(FbxNode* node, s3d::BaseElement::Pointer object, const StringList& props)
{
	if (sharedAnimationLayer == nullptr) return;
	
	std::map<int, FbxTime> keyFramesToTime;
	{
		auto fillCurveMap = [&keyFramesToTime](FbxPropertyT<FbxDouble3> prop)
		{
			auto curveNode = prop.GetCurveNode();
			if (curveNode)
			{
				for (unsigned int i = 0; i < curveNode->GetChannelsCount(); ++i)
				{
					auto curve = curveNode->GetCurve(i);
					if (curve)
					{
						int keyFramesCount = curve->KeyGetCount();
						for (int i = 0; i < keyFramesCount; ++i)
							keyFramesToTime[i] = curve->KeyGetTime(i);
					}
				}
			}
		};
		fillCurveMap(node->LclTranslation);
		fillCurveMap(node->LclRotation);
		fillCurveMap(node->LclScaling);
	}
	
	if (keyFramesToTime.size() == 0) return;
	
	log::info("Node %s has %llu frames in animation.", node->GetName(), uint64_t(keyFramesToTime.size()));
	
	FbxTimeSpan pInterval;
	node->GetAnimationInterval(pInterval);

	s3d::Animation a;
	
	s3d::Animation::OutOfRangeMode mode = s3d::Animation::OutOfRangeMode_Loop;
	for (const auto& p : props)
	{
		if (p.find(s_AnimationProperty) == 0)
		{
			auto animType = p.substr(s_AnimationProperty.size());
			if (animType == "once")
			{
				mode = s3d::Animation::OutOfRangeMode_Once;
			}
			if (animType == "pingpong")
			{
				mode = s3d::Animation::OutOfRangeMode_PingPong;
			}
			else if (animType != "loop")
			{
				log::warning("Unknown animation mode in %s: %s", object->name().c_str(), animType.c_str());
				mode = s3d::Animation::OutOfRangeMode_Loop;
			}
		}
	}
	
	a.setOutOfRangeMode(mode);
	
	a.setFrameRate(static_cast<float>(FbxTime::GetFrameRate(
		node->GetScene()->GetGlobalSettings().GetTimeMode())));
	
	a.setTimeRange(static_cast<float>(keyFramesToTime.begin()->second.GetSecondDouble()),
		static_cast<float>(keyFramesToTime.rbegin()->second.GetSecondDouble()));
	
	auto eval = node->GetAnimationEvaluator();
	for (const auto& kf : keyFramesToTime)
	{
		auto t = eval->GetNodeLocalTranslation(node, kf.second);
		auto r = eval->GetNodeLocalRotation(node, kf.second);
		auto s = eval->GetNodeLocalScaling(node, kf.second);
		
		a.addKeyFrame(static_cast<float>(kf.second.GetSecondDouble()),
			vec3(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])),
			quaternionFromAngles(static_cast<float>(r[0]) * animationAnglesScale,
				static_cast<float>(r[1]) * animationAnglesScale, static_cast<float>(r[2]) * animationAnglesScale),
			vec3(static_cast<float>(s[0]), static_cast<float>(s[1]), static_cast<float>(s[2])));
	}
	
	object->addAnimation(a);
}

void FBXLoaderPrivate::loadNode(FbxNode* node, s3d::BaseElement::Pointer parent)
{
	auto props = loadNodeProperties(node);
	auto materials = loadNodeMaterials(node);
	
	s3d::BaseElement::Pointer createdElement;

	std::string nodeName(node->GetName());
	FbxNodeAttribute* lNodeAttribute = node->GetNodeAttribute();
	if (lNodeAttribute)
	{
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh* mesh = node->GetMesh();
			if (mesh->IsTriangleMesh())
			{
				s3d::Mesh* storedElement = static_cast<s3d::Mesh*>(mesh->GetUserDataPtr());
				
				if (storedElement == nullptr)
				{
					createdElement = loadMesh(mesh, parent, materials, props);
					mesh->SetUserDataPtr(createdElement.ptr());
				}
				else
				{
					createdElement.reset(storedElement->duplicate());
				}
			}
			else
			{
				log::warning("Non-triangle meshes are not supported and should not be present in scene.");
			}
		}
	}
	else if (parent.invalid() && rootNode.invalid())
	{
		rootNode = s3d::ElementContainer::Pointer::create(nodeName, nullptr);
		createdElement = rootNode;
	}
	
	if (createdElement.invalid())
		createdElement = s3d::ElementContainer::Pointer::create(nodeName, parent.ptr());

	mat4 transform;
	FbxAMatrix& fbxTransform = node->EvaluateLocalTransform();
	for (int v = 0; v < 4; ++v)
	{
		for (int u = 0; u < 4; ++u)
			transform[v][u] = static_cast<float>(fbxTransform.Get(v, u));
	}
	createdElement->setTransform(transform);
	
	for (const auto& p : props)
	{
		if (!createdElement->hasPropertyString(p))
			createdElement->addPropertyString(p);
	}
	
	loadNodeAnimations(node, createdElement, props);

	int lChildCount = node->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		loadNode(node->GetChild(lChildIndex), createdElement);
}

void FBXLoaderPrivate::loadMaterialTextureValue(s3d::Material::Pointer m, uint32_t propName,
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
				Texture* ptr = reinterpret_cast<Texture*>(lTexture->GetUserDataPtr());
				m->setTexture(propName, Texture::Pointer(ptr));
			} 
		}
	}
}

void FBXLoaderPrivate::loadMaterialValue(s3d::Material::Pointer m, uint32_t propName,
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

s3d::Material::Pointer FBXLoaderPrivate::loadMaterial(FbxSurfaceMaterial* mat)
{
	s3d::Material::Pointer m;

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

s3d::Mesh::Pointer FBXLoaderPrivate::loadMesh(FbxMesh* mesh, s3d::BaseElement::Pointer parent, 
	const s3d::Material::List& materials, const StringList& params)
{
	const char* mName = mesh->GetName();
	const char* nName = mesh->GetNode()->GetName();
	std::string meshName(strlen(mName) == 0 ? nName : mName);

	uint32_t lodIndex = 0;
	bool support = false;
	for (auto p : params)
	{
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
	}

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
		ET_ASSERT((mapping == FbxGeometryElement::eAllSame) || (mapping == FbxGeometryElement::eByPolygon));

		isContainer = mapping == FbxGeometryElement::eByPolygon;
		if (isContainer)
		{
			materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
			ET_ASSERT(materialIndices->GetCount() == lPolygonCount);
			for (int i = 0; i < materialIndices->GetCount(); ++i)
				numMaterials = etMax(numMaterials, materialIndices->GetAt(i) + 1);
		}
	}

	s3d::Mesh::Pointer element;
	
	if (support)
		element = s3d::SupportMesh::Pointer::create(meshName, parent.ptr());
	else
		element = s3d::Mesh::Pointer::create(meshName, parent.ptr());
	
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

	VertexDeclaration decl(true, VertexAttributeUsage::Position, VertexAttributeType::Vec3);

	if (hasNormal)
		decl.push_back(VertexAttributeUsage::Normal, VertexAttributeType::Vec3);

	if (hasTangents)
		decl.push_back(VertexAttributeUsage::Tangent, VertexAttributeType::Vec3);

	if (hasColor)
		decl.push_back(VertexAttributeUsage::Color, VertexAttributeType::Vec4);
	
	if (hasSmoothingGroups)
		decl.push_back(VertexAttributeUsage::Smoothing, VertexAttributeType::Int);
	
	uint32_t texCoord0 = static_cast<uint32_t>(VertexAttributeUsage::TexCoord0);
	
	auto uv = mesh->GetElementUV();
	if ((uv != nullptr) && (uv->GetMappingMode() != FbxGeometryElement::eNone))
	{
		for (uint32_t i = 0; i < uvChannels; ++i)
			decl.push_back(static_cast<VertexAttributeUsage>(texCoord0 + i), VertexAttributeType::Vec2);
	}

	VertexStorage::Pointer vs = storage->vertexStorageWithDeclarationForAppendingSize(decl, lPolygonVertexCount);
	int vbIndex = storage->indexOfVertexStorage(vs);
	size_t vertexBaseOffset = vs->capacity();
	vs->increaseSize(lPolygonVertexCount);

	IndexArray::Pointer ia = storage->indexArray();
	ia->resizeToFit(ia->actualSize() + lPolygonCount * 3);
	
	VertexDataAccessor<VertexAttributeType::Vec3> pos;
	VertexDataAccessor<VertexAttributeType::Vec3> nrm;
	VertexDataAccessor<VertexAttributeType::Vec3> tan;
	VertexDataAccessor<VertexAttributeType::Vec4> clr;
	VertexDataAccessor<VertexAttributeType::Int> smg;
	
	pos = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, vertexBaseOffset);
	
	if (hasNormal)
		nrm = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Normal, vertexBaseOffset);
	
	if (hasTangents)
		tan = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Tangent, vertexBaseOffset);
	
	if (hasColor)
		clr = vs->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::Color, vertexBaseOffset);
	
	if (hasSmoothingGroups)
		smg = vs->accessData<VertexAttributeType::Int>(VertexAttributeUsage::Smoothing, vertexBaseOffset);

	std::vector<VertexDataAccessor<VertexAttributeType::Vec2>> uvs;
	for (uint32_t i = 0; i < uvChannels; ++i)
		uvs.push_back(vs->accessData<VertexAttributeType::Vec2>(static_cast<VertexAttributeUsage>(texCoord0 + i), vertexBaseOffset));

	FbxStringList lUVNames;
	mesh->GetUVSetNames(lUVNames);

	size_t vertexCount = 0;
	uint32_t indexOffset = vertexBaseOffset & 0xffffffff;

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
		tan[vertexCount] = vec3(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])); }

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
			s3d::BaseElement* aParent = (m == 0) ? parent.ptr() : element.ptr();
			std::string aName = (m == 0) ? meshName : (meshName + "~" + materials.at(m)->name());

			s3d::Mesh::Pointer meshElement;
			if (m == 0)
			{
				meshElement = element;
			}
			else
			{
				if (support)
					meshElement = s3d::SupportMesh::Pointer::create(aName, aParent);
				else
					meshElement = s3d::Mesh::Pointer::create(aName, aParent);
			}

			meshElement->tag = vbIndex;
			meshElement->setStartIndex(static_cast<uint32_t>(indexOffset));
			meshElement->setMaterial(materials.at(m));
			meshElement->setVertexStorage(vs);
			meshElement->setIndexArray(ia);
			
			const auto& props = aParent->properties();
			for (const auto& prop : props)
				meshElement->addPropertyString(prop);

			for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
			{
				if (materialIndices->GetAt(lPolygonIndex) == m)
				{
					for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
					{
						const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
						ia->setIndex(static_cast<uint32_t>(vertexBaseOffset + vertexCount), indexOffset);

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
		me->setStartIndex(static_cast<uint32_t>(vertexBaseOffset));
		me->setNumIndexes(lPolygonVertexCount);
		me->setVertexStorage(vs);
		me->setIndexArray(ia);

		if (materials.size())
			me->setMaterial(materials.front());

		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
		{
			for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
			{
				const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				ia->setIndex(static_cast<uint32_t>(vertexBaseOffset + vertexCount), indexOffset);
				
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
					smg[vertexCount] = sgIndex;
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

void FBXLoaderPrivate::buildVertexBuffers(RenderContext* rc, s3d::BaseElement::Pointer root)
{
	IndexBuffer primaryIndexBuffer =
		rc->vertexBufferFactory().createIndexBuffer("fbx-i", storage->indexArray(), BufferDrawType::Static);

	std::vector<VertexArrayObject> vertexArrayObjects;
	for (const auto& i : storage->vertexStorages())
	{
		VertexArrayObject vao = rc->vertexBufferFactory().createVertexArrayObject("fbx-vao");
		VertexBuffer::Pointer vb = rc->vertexBufferFactory().createVertexBuffer("fbx-v", i, BufferDrawType::Static);
		vao->setBuffers(vb, primaryIndexBuffer);
		vertexArrayObjects.push_back(vao);
	}

	s3d::BaseElement::List meshes = root->childrenOfType(s3d::ElementType_Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
		mesh->cleanupLodChildren();
	}
	
	meshes = root->childrenOfType(s3d::ElementType_SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
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
						result.push_back(line.data());
					
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
	
	for (auto& prop : result)
	{
		lowercase(prop);
		prop.erase(std::remove_if(prop.begin(), prop.end(), [](char c) { return isWhitespaceChar(c); }), prop.end());
	}
	
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
	FBXLoaderPrivate* loader = sharedObjectFactory().createObject<FBXLoaderPrivate>(rc, ObjectsCache);

	if (loader->import(_filename))
		result = loader->parse();

	Invocation i;
	i.setTarget([loader]()
	{
		sharedObjectFactory().deleteObject(loader);
	});
	i.invokeInMainRunLoop();
	
	return result;
}

#else // ET_HAVE_FBX_SDK
#	if (ET_PLATFORM_WIN)
#		pragma message ("Define ET_HAVE_FBX_SDK to compile FBXLoader")
#	else
#		warning Set ET_HAVE_FBX_SDK to 1 in order to compile FBXLoader
#	endif
#endif
