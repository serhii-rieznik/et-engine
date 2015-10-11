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
	public:
		FBXLoaderPrivate(RenderContext* rc, ObjectsCache& ObjectsCache);
		~FBXLoaderPrivate();

		s3d::ElementContainer::Pointer parse(s3d::Storage&);

		bool import(const std::string& filename);
		
		void loadAnimations();
		void loadTextures();
		
		/* 
		 * Node loading
		 */
		void loadNode(s3d::Storage&, FbxNode* node, s3d::BaseElement::Pointer parent, bool isRootNode);
		void loadNodeAnimations(FbxNode* node, s3d::BaseElement::Pointer object, const StringList& props);

		s3d::Material::List loadNodeMaterials(s3d::Storage&, FbxNode* node);
		StringList loadNodeProperties(FbxNode* node);
		
		void buildVertexBuffers(RenderContext* rc, s3d::BaseElement::Pointer root, s3d::Storage&);

		bool meshHasSkin(FbxMesh*);
		
		s3d::Mesh::Pointer loadMesh(s3d::Storage&, FbxMesh*, s3d::BaseElement::Pointer parent,
			const s3d::Material::List& materials, const StringList& params);

		s3d::LineElement::Pointer loadLine(s3d::Storage&, FbxLine*, s3d::BaseElement::Pointer parent, 
			const StringList& params);

		s3d::SkeletonElement::Pointer loadBone(FbxSkeleton* node, s3d::BaseElement::Pointer parent);
		
		s3d::Material::Pointer loadMaterial(FbxSurfaceMaterial* material);
		
		void linkSkeleton(s3d::Storage&, s3d::ElementContainer::Pointer);
		void buildBlendWeightsForMesh(s3d::Storage&, s3d::Mesh::Pointer);

		void loadMaterialValue(s3d::Material::Pointer m, uint32_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);
		
		void loadMaterialTextureValue(s3d::Material::Pointer m, uint32_t propName,
			FbxSurfaceMaterial* fbxm, const char* fbxprop);

		mat4 fbxMatrixToMat4(const FbxAMatrix& m)
		{
			mat4 transform;
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
					transform[r][c] = static_cast<float>(m.Get(r, c));
			}
			return transform;
		}
		
	public:
		RenderContext* _rc = nullptr;
		ObjectsCache& _texCache;
		std::string _folder;
		FbxManager* manager = nullptr;
		FbxImporter* importer = nullptr;
		FbxScene* scene = nullptr;
		FbxAnimLayer* sharedAnimationLayer = nullptr;
		std::map<size_t, s3d::BaseElement::Pointer> nodeToElementMap;
		s3d::BaseElement::Pointer root;
		bool shouldCreateRenderObjects = true;
		
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
	int lSDKRevision = 0;

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

s3d::ElementContainer::Pointer FBXLoaderPrivate::parse(s3d::Storage& storage)
{
	storage.flush();
	
	FbxGeometryConverter geometryConverter(manager);
	geometryConverter.Triangulate(scene, true);
	
	loadAnimations();
	
	if (shouldCreateRenderObjects)
		loadTextures();

	auto fbxRootNode = scene->GetRootNode();
	root = s3d::ElementContainer::Pointer::create(fbxRootNode->GetName(), nullptr);

	int lChildCount = fbxRootNode->GetChildCount();

	auto gt = fbxRootNode->EvaluateGlobalTransform();
	auto lt = fbxRootNode->EvaluateLocalTransform();

	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		loadNode(storage, fbxRootNode->GetChild(lChildIndex), root, true);

	linkSkeleton(storage, root);
	
	if (shouldCreateRenderObjects)
		buildVertexBuffers(_rc, root, storage);
	
	auto meshes = root->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
		mesh->calculateSupportData();

	meshes = root->childrenOfType(s3d::ElementType::SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
		mesh->calculateSupportData();

	return root;
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

s3d::Material::List FBXLoaderPrivate::loadNodeMaterials(s3d::Storage& storage, FbxNode* node)
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
			storage.addMaterial(m);
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
	
	if (keyFramesToTime.size() > 1)
	{
		log::info("Node %s has %llu frames in animation.",
			node->GetName(), uint64_t(keyFramesToTime.size()));
	}
	
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

void et::FBXLoaderPrivate::loadNode(s3d::Storage& storage, FbxNode* node, s3d::BaseElement::Pointer parent, bool isRootNode)
{
	auto props = loadNodeProperties(node);
	auto materials = loadNodeMaterials(storage, node);
	
	s3d::BaseElement::Pointer createdElement;

	std::string nodeName(node->GetName());
	FbxNodeAttribute* lNodeAttribute = node->GetNodeAttribute();
	if (lNodeAttribute)
	{
		auto nodeType = lNodeAttribute->GetAttributeType();
		if (nodeType == FbxNodeAttribute::eMesh)
		{
			FbxMesh* mesh = node->GetMesh();
			if (mesh->IsTriangleMesh())
			{
				s3d::Mesh* storedElement = static_cast<s3d::Mesh*>(mesh->GetUserDataPtr());
				
				if (storedElement == nullptr)
				{
					createdElement = loadMesh(storage, mesh, parent, materials, props);
					mesh->SetUserDataPtr(createdElement.ptr());
				}
				else
				{
					createdElement.reset(storedElement->duplicate());
				}
			}
			else
			{
				log::warning("Non-triangle meshes are not supported.");
			}
		}
		else if (nodeType == FbxNodeAttribute::eLine)
		{
			createdElement = loadLine(storage, node->GetLine(), parent, props);
		}
		else if (nodeType == FbxNodeAttribute::eSkeleton)
		{
			createdElement = loadBone(node->GetSkeleton(), parent);
		}
		else if (nodeType == FbxNodeAttribute::eNull)
		{
			createdElement = s3d::ElementContainer::Pointer::create(nodeName, parent.ptr());
		}
		else
		{
			log::warning("Unsupported node found in FBX, with type: %s", node->GetTypeName());
		}
	}
	
	if (createdElement.invalid())
		createdElement = s3d::ElementContainer::Pointer::create(nodeName, parent.ptr());
	
	nodeToElementMap[reinterpret_cast<size_t>(node)] = createdElement;

	const auto& transform = node->EvaluateLocalTransform();
	createdElement->setTransform(fbxMatrixToMat4(transform));

	for (const auto& p : props)
	{
		if (!createdElement->hasPropertyString(p))
			createdElement->addPropertyString(p);
	}
	
	loadNodeAnimations(node, createdElement, props);

	int lChildCount = node->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		loadNode(storage, node->GetChild(lChildIndex), createdElement, false);
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
			if ((lTexture != nullptr) && lTexture->GetUserDataPtr())
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

	if (shouldCreateRenderObjects)
	{
		loadMaterialTextureValue(m, MaterialParameter_DiffuseMap, mat, FbxSurfaceMaterial::sDiffuse);
		loadMaterialTextureValue(m, MaterialParameter_AmbientMap, mat, FbxSurfaceMaterial::sAmbient);
		loadMaterialTextureValue(m, MaterialParameter_EmissiveMap, mat, FbxSurfaceMaterial::sEmissive);
		loadMaterialTextureValue(m, MaterialParameter_SpecularMap, mat, FbxSurfaceMaterial::sSpecular);
		loadMaterialTextureValue(m, MaterialParameter_NormalMap, mat, FbxSurfaceMaterial::sNormalMap);
		loadMaterialTextureValue(m, MaterialParameter_BumpMap, mat, FbxSurfaceMaterial::sBump);
		loadMaterialTextureValue(m, MaterialParameter_ReflectionMap, mat, FbxSurfaceMaterial::sReflection);
	}
	
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


s3d::SkeletonElement::Pointer FBXLoaderPrivate::loadBone(FbxSkeleton* skeleton, s3d::BaseElement::Pointer parent)
{
	auto node = skeleton->GetNode();
	s3d::SkeletonElement::Pointer result = s3d::SkeletonElement::Pointer::create(node->GetName(), parent.ptr());
	result->tag = reinterpret_cast<size_t>(node);
	return result;
}

bool FBXLoaderPrivate::meshHasSkin(FbxMesh* mesh)
{
	int numClusters = 0;
	int numDeformers = mesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int i = 0; i < numDeformers; ++i)
	{
		FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));
		numClusters += skin ? skin->GetClusterCount() : 0;
	}
	
	if (numDeformers + numClusters > 0)
	{
		log::info(">>> Mesh '%s' has %d deformers and %d clusters", mesh->GetNode()->GetName(),
			numDeformers, numClusters);
	}
	
	return numClusters > 0;
}

s3d::Mesh::Pointer FBXLoaderPrivate::loadMesh(s3d::Storage& storage, FbxMesh* mesh, 
	s3d::BaseElement::Pointer parent, const s3d::Material::List& materials, const StringList& params)
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

	FbxGeometryElementMaterial* material = mesh->GetElementMaterial();
	FbxLayerElementArrayTemplate<int>* materialIndices = 0;
	if (material)
	{
		FbxGeometryElement::EMappingMode mapping = material->GetMappingMode();
		ET_ASSERT((mapping == FbxGeometryElement::eAllSame) || (mapping == FbxGeometryElement::eByPolygon));
		if (mapping == FbxGeometryElement::eByPolygon)
		{
			isContainer = true;
			materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
			ET_ASSERT(materialIndices->GetCount() == lPolygonCount);
		}
	}

	s3d::Mesh::Pointer element;
	
	if (support)
		element = s3d::SupportMesh::Pointer::create(meshName, parent.ptr());
	else
		element = s3d::Mesh::Pointer::create(meshName, parent.ptr());
	
	size_t uvChannels = mesh->GetElementUVCount();

	bool hasNormal = mesh->GetElementNormalCount() > 0;
	bool hasSkin = meshHasSkin(mesh);

	const FbxVector4* lControlPoints = mesh->GetControlPoints();
	
	bool hasTangents = mesh->GetElementTangentCount() > 0;
	FbxGeometryElementTangent* tangents = hasTangents ? mesh->GetElementTangent() : nullptr;
	
	bool hasSmoothingGroups = false; // mesh->GetElementSmoothingCount() > 0;
	FbxGeometryElementSmoothing* smoothing = nullptr; // hasSmoothingGroups ? mesh->GetElementSmoothing() : nullptr;
	
	bool hasColor = mesh->GetElementVertexColorCount() > 0;
	FbxGeometryElementVertexColor* vertexColor = hasColor ? mesh->GetElementVertexColor() : nullptr;

	if (hasNormal)
		hasNormal &= (mesh->GetElementNormal()->GetMappingMode() != FbxGeometryElement::eNone);

	if (tangents)
		hasTangents &= (tangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);
	
	if (smoothing)
		hasSmoothingGroups &= (smoothing->GetMappingMode() == FbxGeometryElement::eByPolygon);

	if (vertexColor)
		hasColor &= (vertexColor->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);

	VertexDeclaration decl(true, VertexAttributeUsage::Position, VertexAttributeType::Vec3);

	if (hasNormal)
		decl.push_back(VertexAttributeUsage::Normal, VertexAttributeType::Vec3);

	if (hasColor)
		decl.push_back(VertexAttributeUsage::Color, VertexAttributeType::Vec4);
	
	if (hasTangents)
		decl.push_back(VertexAttributeUsage::Tangent, VertexAttributeType::Vec3);
	
	auto uv = mesh->GetElementUV();
	uint32_t texCoord0 = static_cast<uint32_t>(VertexAttributeUsage::TexCoord0);
	if ((uv != nullptr) && (uv->GetMappingMode() != FbxGeometryElement::eNone))
	{
		for (uint32_t i = 0; i < uvChannels; ++i)
			decl.push_back(static_cast<VertexAttributeUsage>(texCoord0 + i), VertexAttributeType::Vec2);
	}

	if (hasSkin)
	{
		decl.push_back(VertexAttributeUsage::BlendWeights, VertexAttributeType::Vec4);
		decl.push_back(VertexAttributeUsage::BlendIndices, VertexAttributeType::IntVec4);
	}
	
	if (hasSmoothingGroups)
		decl.push_back(VertexAttributeUsage::Smoothing, VertexAttributeType::Int);
	
	
	VertexStorage::Pointer vs = storage.vertexStorageWithDeclarationForAppendingSize(decl, lPolygonVertexCount);
	int vbIndex = storage.indexOfVertexStorage(vs);
	size_t vertexBaseOffset = vs->capacity();
	vs->increaseSize(lPolygonVertexCount);

	IndexArray::Pointer ia = storage.indexArray();
	if (ia.invalid())
	{
		ia = IndexArray::Pointer::create(IndexArrayFormat::Format_32bit, 3 * lPolygonCount, PrimitiveType::Triangles);
		storage.setIndexArray(ia);
	}
	else 
	{
		ia->resizeToFit(ia->actualSize() + 3 * lPolygonCount);
	}
	
	VertexDataAccessor<VertexAttributeType::Vec3> pos;
	VertexDataAccessor<VertexAttributeType::Vec3> nrm;
	VertexDataAccessor<VertexAttributeType::Vec3> tan;
	VertexDataAccessor<VertexAttributeType::Vec4> clr;
	VertexDataAccessor<VertexAttributeType::Int> smg;
	VertexDataAccessor<VertexAttributeType::Vec4> blw;
	VertexDataAccessor<VertexAttributeType::IntVec4> bli;
	
	pos = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, vertexBaseOffset);
	
	if (hasNormal)
		nrm = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Normal, vertexBaseOffset);

	if (hasColor)
		clr = vs->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::Color, vertexBaseOffset);
	
	if (hasTangents)
		tan = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Tangent, vertexBaseOffset);
	
	if (hasSmoothingGroups)
		smg = vs->accessData<VertexAttributeType::Int>(VertexAttributeUsage::Smoothing, vertexBaseOffset);
	
	if (hasSkin)
	{
		blw = vs->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::BlendWeights, vertexBaseOffset);
		bli = vs->accessData<VertexAttributeType::IntVec4>(VertexAttributeUsage::BlendIndices, vertexBaseOffset);
	}
	
	std::vector<VertexDataAccessor<VertexAttributeType::Vec2>> uvs;
	for (uint32_t i = 0; i < uvChannels; ++i)
		uvs.push_back(vs->accessData<VertexAttributeType::Vec2>(static_cast<VertexAttributeUsage>(texCoord0 + i), vertexBaseOffset));

	FbxStringList lUVNames;
	mesh->GetUVSetNames(lUVNames);

	size_t vertexCount = 0;
	uint32_t indexOffset = static_cast<uint32_t>(vertexBaseOffset);
	
	std::map<size_t, std::vector<size_t>> controlPointToMeshIndex;

#define PUSH_VERTEX FbxVector4 v = lControlPoints[lControlPointIndex]; \
		controlPointToMeshIndex[lControlPointIndex].push_back(vertexCount); \
		pos[vertexCount] = vec3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])); 
#define PUSH_NORMAL if (hasNormal) { \
		FbxVector4 n; mesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, n); \
		nrm[vertexCount] = vec3(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2])); }
#define PUSH_UV for (size_t i = 0; i < uvChannels; ++i) { \
		FbxVector2 t; \
		bool unmap = false; \
		mesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVNames[static_cast<int>(i)].Buffer(), t, unmap); \
		uvs[i][vertexCount] = vec2(static_cast<float>(t[0]), static_cast<float>(t[1])); }
#define PUSH_TANGENT if (hasTangents) { FbxVector4 t; \
		if (tangents->GetReferenceMode() == FbxGeometryElement::eDirect) \
			t = tangents->GetDirectArray().GetAt(static_cast<int>(vertexCount)); \
		else if (tangents->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) \
			t = tangents->GetDirectArray().GetAt(tangents->GetIndexArray().GetAt(static_cast<int>(vertexCount))); \
		tan[vertexCount] = vec3(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])); }
#define PUSH_COLOR if (hasColor) { FbxColor c; \
		if (vertexColor->GetReferenceMode() == FbxGeometryElement::eDirect) \
			c = vertexColor->GetDirectArray().GetAt(static_cast<int>(vertexCount)); \
		else if (vertexColor->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) \
			c = vertexColor->GetDirectArray().GetAt(vertexColor->GetIndexArray().GetAt(static_cast<int>(vertexCount))); \
		clr[vertexCount] = vec4(static_cast<float>(c.mRed), static_cast<float>(c.mGreen), \
			static_cast<float>(c.mBlue), static_cast<float>(c.mAlpha)); }
#define PUSH_SG if (hasSmoothingGroups) { \
		int sgIndex = 0; if (smoothing->GetReferenceMode() == FbxGeometryElement::eDirect) \
			sgIndex = lPolygonIndex; else if (smoothing->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) \
			sgIndex = smoothing->GetIndexArray().GetAt(static_cast<int>(vertexCount)); \
			sgIndex = smoothing->GetDirectArray().GetAt(sgIndex); smg[vertexCount] = sgIndex; }

	std::vector<s3d::Mesh::Pointer> createdMeshes;

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
			meshElement->setStartIndex(indexOffset);
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
						int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
						ia->setIndex(static_cast<uint32_t>(vertexBaseOffset + vertexCount), indexOffset);
						PUSH_VERTEX
						PUSH_NORMAL
						PUSH_UV
						PUSH_TANGENT
						PUSH_COLOR
						PUSH_SG
						++vertexCount;
						++indexOffset;
					}
				}
			}

			meshElement->setNumIndexes(indexOffset - meshElement->startIndex());
			if ((lodIndex > 0) && (parent->type() == s3d::ElementType::Mesh))
			{
				s3d::Mesh::Pointer p = parent;
				p->attachLod(lodIndex, meshElement);
			}
			createdMeshes.push_back(meshElement);
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

		if (!materials.empty())
			me->setMaterial(materials.front());

		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
		{
			for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
			{
				const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				ia->setIndex(static_cast<uint32_t>(vertexBaseOffset + vertexCount), indexOffset);
				PUSH_VERTEX
				PUSH_NORMAL
				PUSH_UV
				PUSH_TANGENT
				PUSH_COLOR
				PUSH_SG
				++vertexCount;
				++indexOffset;
			}
		}

		if ((lodIndex > 0) && (parent->type() == s3d::ElementType::Mesh))
		{
			s3d::Mesh::Pointer p = parent;
			p->attachLod(lodIndex, element);
		}
		createdMeshes.push_back(element);
	}

	if (hasSkin)
	{
		int numDeformers = mesh->GetDeformerCount(FbxDeformer::eSkin);
		
		if (numDeformers > 1)
		{
			log::warning("Multiple mesh deformers are not supported (mesh '%s'), first will be used",
				mesh->GetNode()->GetName());
		}
		
		FbxSkin* skin = nullptr;
		
		for (int i = 0; (skin == nullptr) && (i < numDeformers); ++i)
			skin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));
			
		s3d::MeshDeformer::Pointer deformer = s3d::MeshDeformer::Pointer::create();
		for (int cl = 0, ce = skin->GetClusterCount(); cl < ce; ++cl)
		{
			FbxCluster* fbxCluster = skin->GetCluster(cl);
			s3d::MeshDeformerCluster::Pointer cluster = s3d::MeshDeformerCluster::Pointer::create();
			
			FbxAMatrix fbxMeshTransformMatrix;
			FbxAMatrix fbxLinkTransformMatrix;
			fbxCluster->GetTransformMatrix(fbxMeshTransformMatrix);
			fbxCluster->GetTransformLinkMatrix(fbxLinkTransformMatrix);
			mat4 meshInitialTransform = fbxMatrixToMat4(fbxMeshTransformMatrix);
			mat4 linkInitialTransform = fbxMatrixToMat4(fbxLinkTransformMatrix);
					
			cluster->setMeshInitialTransform(meshInitialTransform);
			cluster->setLinkInitialTransform(linkInitialTransform);
			cluster->setLinkTag(reinterpret_cast<size_t>(fbxCluster->GetLink()));
			
			int numIndices = fbxCluster->GetControlPointIndicesCount();
			int* indices = fbxCluster->GetControlPointIndices();
			double* weights = fbxCluster->GetControlPointWeights();
			cluster->weights().reserve(numIndices);
			for (int i = 0; i < numIndices; ++i)
			{
				uint32_t index = static_cast<uint32_t>(indices[i]);
				float weight = static_cast<float>(weights[i]);
				if (controlPointToMeshIndex.count(index) > 0)
				{
					const auto& convertedIndices = controlPointToMeshIndex.at(index);
					for (size_t ci : convertedIndices)
						cluster->weights().emplace_back(ci, weight);
				}
			}
			cluster->sortWeights();
			deformer->addCluster(cluster);
		}
		
		for (auto mesh : createdMeshes)
			mesh->setDeformer(deformer);
	}

	return element;
}

void FBXLoaderPrivate::buildVertexBuffers(RenderContext* rc, s3d::BaseElement::Pointer root, s3d::Storage& storage)
{
	IndexBuffer::Pointer primaryIndexBuffer;
	
	std::vector<VertexArrayObject> vertexArrayObjects;
	for (const auto& i : storage.vertexStorages())
	{
		VertexArrayObject vao = rc->vertexBufferFactory().createVertexArrayObject("fbx-vao");
		VertexBuffer::Pointer vb = rc->vertexBufferFactory().createVertexBuffer("fbx-v", i, BufferDrawType::Static);
		
		if (primaryIndexBuffer.invalid())
		{
			primaryIndexBuffer = rc->vertexBufferFactory().createIndexBuffer("fbx-i",
				storage.indexArray(), BufferDrawType::Static);
		}
		
		vao->setBuffers(vb, primaryIndexBuffer);
		vertexArrayObjects.push_back(vao);
	}

	s3d::BaseElement::List meshes = root->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
		mesh->cleanupLodChildren();
	}
	
	meshes = root->childrenOfType(s3d::ElementType::SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
	{
		mesh->setVertexArrayObject(vertexArrayObjects[mesh->tag]);
		mesh->cleanupLodChildren();
	}
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

s3d::LineElement::Pointer FBXLoaderPrivate::loadLine(s3d::Storage&, FbxLine* line, s3d::BaseElement::Pointer parent, 
	const StringList& params)
{
	const char* lineName = line->GetName();
	if ((lineName == nullptr) || (strlen(lineName) == 0))
		lineName = line->GetNode()->GetName();

	auto result = s3d::LineElement::Pointer::create(lineName, parent.ptr());

	auto points = line->GetControlPoints();
	int numIndexes = line->GetIndexArraySize();
	for (int i = 0; i < numIndexes; ++i)
	{
		auto point = points[line->GetPointIndexAt(i)];
		result->addPoint(vec3(float(point[0]), float(point[1]), float(point[2])));
	}

	return result;
}

void FBXLoaderPrivate::linkSkeleton(s3d::Storage& storage, s3d::ElementContainer::Pointer root)
{
	auto meshes = root->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		if (mesh->deformer().valid())
		{
			auto& clusters = mesh->deformer()->clusters();
			for (auto cluster : clusters)
			{
				size_t tag = cluster->linkTag();
				if (nodeToElementMap.count(tag) > 0)
					cluster->setLink(nodeToElementMap.at(tag));
				else
					log::warning("Unable to link deformer cluster in mesh %s - link is missing", mesh->name().c_str());
			}
		}
		buildBlendWeightsForMesh(storage, mesh);
	}
}

void FBXLoaderPrivate::buildBlendWeightsForMesh(s3d::Storage& storage, s3d::Mesh::Pointer mesh)
{
	if (mesh->deformer().invalid()) return;
	
	auto vs = mesh->vertexStorage();
	if (!vs->hasAttribute(VertexAttributeUsage::BlendIndices) ||
		!vs->hasAttribute(VertexAttributeUsage::BlendWeights)) return;
	
	auto bli = vs->accessData<VertexAttributeType::IntVec4>(VertexAttributeUsage::BlendIndices, mesh->startIndex());
	auto blw = vs->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::BlendWeights, mesh->startIndex());
	
	std::map<size_t, size_t> placedIndices;
	
	for (size_t i = 0; i < mesh->numIndexes(); ++i)
	{
		bli[i] = vec4i(0);
		blw[i] = vec4(0.0f);
	}
	
	int clusterIndex = 0;
	const auto& clusters = mesh->deformer()->clusters();
	for (const auto& cluster : clusters)
	{
		const auto& weights = cluster->weights();
		for (const auto& w : weights)
		{
			size_t componentIndex = 0;
			if (placedIndices.count(w.index) > 0)
				componentIndex = placedIndices[w.index] + 1;
			
			if (componentIndex > 3)
			{
				log::warning("More than four bones affecting vertex %llu", uint64_t(w.index));
			}
			else
			{
				bli[w.index][componentIndex] = clusterIndex;
				blw[w.index][componentIndex] = w.weight;
			}

			placedIndices[w.index] = componentIndex;
		}
		++clusterIndex;
	}
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

void FBXLoader::setShouldCreateRenderObjects(bool value)
{
	_shouldCreateRenderObjects = value;
}

s3d::ElementContainer::Pointer FBXLoader::load(RenderContext* rc, s3d::Storage& storage, ObjectsCache& ObjectsCache)
{
	s3d::ElementContainer::Pointer result;
	FBXLoaderPrivate* loader = etCreateObject<FBXLoaderPrivate>(rc, ObjectsCache);
	loader->shouldCreateRenderObjects = _shouldCreateRenderObjects;

	if (loader->import(_filename))
		result = loader->parse(storage);

	Invocation i;
	i.setTarget([loader]()
	{
		etDestroyObject(loader);
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
