/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/indexarray.h>
#include <et/rendering/base/primitives.h>
#include <et-ext/formats/fbxloader.h>

#if (ET_HAVE_FBX_SDK)

#include <fbxsdk.h>

using namespace FBXSDK_NAMESPACE;

static const std::string s_AnimationProperty = "animation=";
static const float animationAnglesScale = -TO_RADIANS;

namespace et
{
struct RenderBatchConstructionInfo
{
	s3d::Mesh::Pointer owner;
	MaterialInstance::Pointer material;
	uint32_t startIndex = 0;
	uint32_t numIndices = 0;

	RenderBatchConstructionInfo() = default;

	RenderBatchConstructionInfo(uint32_t s, uint32_t n, s3d::Mesh::Pointer o, MaterialInstance::Pointer m) :
		owner(o), material(m), startIndex(s), numIndices(n)
	{
		owner->setMaterial(m);
	}
};

using MaterialCollection = Vector<MaterialInstance::Pointer>;

class FBXLoaderPrivate
{
public:
	FBXLoaderPrivate(RenderInterface::Pointer, ObjectsCache&);
	~FBXLoaderPrivate();

	s3d::ElementContainer::Pointer parse(s3d::Storage&);

	bool import(const std::string& filename);

	void loadAnimations();
	void loadTextures();

	/*
	 * Node loading
	 */
	void loadNode(s3d::Storage&, FbxNode* node, s3d::BaseElement::Pointer parent);
	void loadNodeAnimations(FbxNode* node, s3d::BaseElement::Pointer object, const StringList& props);

	MaterialCollection loadNodeMaterials(s3d::Storage&, FbxNode* node);
	StringList loadNodeProperties(FbxNode* node);

	void buildVertexBuffers(s3d::Storage&);

	bool meshHasSkin(FbxMesh*);

	s3d::Mesh::Pointer loadMesh(s3d::Storage&, FbxMesh*, s3d::BaseElement::Pointer parent,
		const MaterialCollection& materials);

	s3d::LineElement::Pointer loadLine(s3d::Storage&, FbxLine*, s3d::BaseElement::Pointer parent,
		const StringList& params);

	s3d::SkeletonElement::Pointer loadBone(FbxSkeleton* node, s3d::BaseElement::Pointer parent);

	MaterialInstance::Pointer loadMaterial(FbxSurfaceMaterial* material);

	void linkSkeleton(s3d::ElementContainer::Pointer);
	void buildBlendWeightsForMesh(s3d::Mesh::Pointer);
	void loadMaterialTextureValue(MaterialInstance::Pointer, MaterialTexture, FbxSurfaceMaterial* fbxm, const char* fbxprop);

	template <class Remap>
	void loadMaterialValue(MaterialInstance::Pointer m, MaterialVariable, FbxSurfaceMaterial* fbxm, 
		const char* fbxprop, Remap remap);

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
	RenderInterface::Pointer _renderer;
	ObjectsCache& _texCache;
	std::string _file;
	std::string _folder;
	FbxManager* manager = nullptr;
	FbxImporter* importer = nullptr;
	FbxScene* scene = nullptr;
	FbxAnimLayer* sharedAnimationLayer = nullptr;
	std::map<size_t, s3d::BaseElement::Pointer> nodeToElementMap;
	std::map<const VertexStorage*, std::vector<RenderBatchConstructionInfo>> constructionInfo;
	s3d::BaseElement::Pointer root;
	bool shouldCreateRenderObjects = true;
};
}

using namespace et;

/*
 * Private implementation
 */
FBXLoaderPrivate::FBXLoaderPrivate(RenderInterface::Pointer renderer, ObjectsCache& ObjectsCache) :
	_renderer(renderer), _texCache(ObjectsCache), manager(FbxManager::Create()), scene(FbxScene::Create(manager, 0))
{
}

FBXLoaderPrivate::~FBXLoaderPrivate()
{
	if (manager)
		manager->Destroy();
}

bool FBXLoaderPrivate::import(const std::string& filename)
{
	_file = filename;
	_folder = getFilePath(filename);
	importer = FbxImporter::Create(manager, nullptr);

	if (importer->Initialize(filename.c_str(), -1, manager->GetIOSettings()) == false)
	{
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n", importer->GetStatus().GetErrorString());

		if (importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			int lSDKMajor = 0;
			int lSDKMinor = 0;
			int lSDKRevision = 0;
			FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

			int lFileMajor = 0;
			int lFileMinor = 0;
			int lFileRevision = 0;
			importer->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

			log::info("FBX version number for this FBX SDK is %d.%d.%d\n", lSDKMajor,
				lSDKMinor, lSDKRevision);

			log::info("FBX version number for file %s is %d.%d.%d\n\n", filename.c_str(),
				lFileMajor, lFileMinor, lFileRevision);
		}

		importer->Destroy();
		return false;
	}

	if (importer->IsFBX() == false)
	{
		log::error("FBXLoader error: %s is not an FBX file", filename.c_str());
		importer->Destroy();
		return false;
	}

	if (importer->Import(scene) == false)
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
	geometryConverter.RemoveBadPolygonsFromMeshes(scene);

	loadAnimations();

	if (shouldCreateRenderObjects)
	{
		loadTextures();
	}

	auto fbxRootNode = scene->GetRootNode();
	root = s3d::ElementContainer::Pointer::create(fbxRootNode->GetName(), nullptr);

	int lChildCount = fbxRootNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		loadNode(storage, fbxRootNode->GetChild(lChildIndex), root);
	}

	linkSkeleton(root);

	if (shouldCreateRenderObjects)
		buildVertexBuffers(storage);

	vec3 minExtent(+std::numeric_limits<float>::max());
	vec3 maxExtent(-std::numeric_limits<float>::max());
	auto meshes = root->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->calculateSupportData();
		maxExtent = maxv(maxExtent, mesh->tranformedBoundingBox().maxVertex());
		minExtent = minv(minExtent, mesh->tranformedBoundingBox().minVertex());
	}
	
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
	application().setShouldSilentPathResolverErrors(true);

	int textures = scene->GetTextureCount();
	for (int i = 0; i < textures; ++i)
	{
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(scene->GetTexture(i));
		if (fileTexture && (fileTexture->GetUserDataPtr() == nullptr))
		{
			const char* fileTextureFileName = fileTexture->GetRelativeFileName();
			if ((fileTextureFileName != nullptr) && (strlen(fileTextureFileName) > 0))
			{
				std::string fileName = normalizeFilePath(std::string(fileTextureFileName));

				if (!fileExists(fileName))
				{
					fileName = application().resolveFileName(getFileName(fileName));
				}

				if (!fileExists(fileName))
				{
					fileName = _folder + getFileName(fileName);
				}

				Texture::Pointer texture = _renderer->loadTexture(fileName, _texCache);
				if (texture.invalid())
				{
					log::error("Failed to load texture: %s", fileName.c_str());
					texture = _renderer->checkersTexture();
					texture->setOrigin(fileName);
					_texCache.manage(texture, ObjectLoader::Pointer()); // TODO : provide (re)loader
				}
				fileTexture->SetUserDataPtr(texture.pointer());
			}
		}
	}
	application().popSearchPaths();
	application().setShouldSilentPathResolverErrors(false);
}

MaterialCollection FBXLoaderPrivate::loadNodeMaterials(s3d::Storage& storage, FbxNode* node)
{
	MaterialCollection materials;
	const int lMaterialCount = node->GetMaterialCount();
	for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
	{
		FbxSurfaceMaterial* lMaterial = node->GetMaterial(lMaterialIndex);
		MaterialInstance* storedMaterial = static_cast<MaterialInstance*>(lMaterial->GetUserDataPtr());
		if (storedMaterial == nullptr)
		{
			MaterialInstance::Pointer m = loadMaterial(lMaterial);
			materials.push_back(m);
			storage.addMaterial(m);
			lMaterial->SetUserDataPtr(m.pointer());
		}
		else
		{
			materials.emplace_back(storedMaterial);
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
				for (uint32_t ci = 0; ci < curveNode->GetChannelsCount(); ++ci)
				{
					auto curve = curveNode->GetCurve(ci);
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
		log::info("Node %s has %llu frames in animation.", node->GetName(), uint64_t(keyFramesToTime.size()));
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

void et::FBXLoaderPrivate::loadNode(s3d::Storage& storage, FbxNode* node, s3d::BaseElement::Pointer parent)
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
					createdElement = loadMesh(storage, mesh, parent, materials);
					mesh->SetUserDataPtr(createdElement.pointer());
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
			createdElement = s3d::ElementContainer::Pointer::create(nodeName, parent.pointer());
		}
		else
		{
			log::warning("Unsupported node found in FBX, with type: %s", node->GetTypeName());
		}
	}

	if (createdElement.invalid())
		createdElement = s3d::ElementContainer::Pointer::create(nodeName, parent.pointer());

	nodeToElementMap[reinterpret_cast<size_t>(node)] = createdElement;

	auto t = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	auto r = node->GetGeometricRotation(FbxNode::eSourcePivot);
	auto s = node->GetGeometricScaling(FbxNode::eSourcePivot);

	const auto& transform = node->EvaluateLocalTransform();
	createdElement->setTransform(fbxMatrixToMat4(transform));
	createdElement->setAdditionalTransform(fbxMatrixToMat4(FbxAMatrix(t, r, s)));

	for (const auto& p : props)
	{
		if (!createdElement->hasPropertyString(p))
			createdElement->addPropertyString(p);
	}

	loadNodeAnimations(node, createdElement, props);

	int lChildCount = node->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		loadNode(storage, node->GetChild(lChildIndex), createdElement);
	}
}

void FBXLoaderPrivate::loadMaterialTextureValue(MaterialInstance::Pointer m, MaterialTexture texId,
	FbxSurfaceMaterial* fbxm, const char* fbxprop)
{
	FbxProperty value = fbxm->FindProperty(fbxprop);
	if (!value.IsValid())
		return;

	int lTextureCount = value.GetSrcObjectCount<FbxFileTexture>();
	for (int i = 0; i < lTextureCount; ++i)
	{
		FbxFileTexture* lTexture = value.GetSrcObject<FbxFileTexture>(i);
		if ((lTexture != nullptr) && (lTexture->GetUserDataPtr() != nullptr))
			m->setTexture(texId, Texture::Pointer(reinterpret_cast<Texture*>(lTexture->GetUserDataPtr())));
	}
}

inline vec4 NullRemap(const vec4& a)
{
	return a;
}

inline vec4 RoughnessRemap(const vec4& a)
{
	return sqrtv(vec4(2.0f) / (a + vec4(2.0f)));
}

inline vec4 TransparencyRemap(const vec4& a)
{
	return vec4(1.0f) - a;
}

template <class Remap>
inline void FBXLoaderPrivate::loadMaterialValue(MaterialInstance::Pointer m, MaterialVariable propName,
	FbxSurfaceMaterial* fbxm, const char* fbxprop, Remap remap)
{
	const FbxProperty value = fbxm->FindProperty(fbxprop);
	if (!value.IsValid()) return;

	EFbxType dataType = value.GetPropertyDataType().GetType();
	if (dataType == eFbxFloat)
	{
		m->setFloat(propName, remap(vec4(value.Get<float>())).x);
	}
	else if (dataType == eFbxDouble)
	{
		m->setFloat(propName, remap(vec4(static_cast<float>(value.Get<double>()))).x);
	}
	else if (dataType == eFbxDouble2)
	{
		FbxDouble2 data = value.Get<FbxDouble2>();
		m->setVector(propName, remap(vec4(static_cast<float>(data[0]), static_cast<float>(data[1]), 0.0f, 0.0f)));
	}
	else if (dataType == eFbxDouble3)
	{
		FbxDouble3 data = value.Get<FbxDouble3>();
		m->setVector(propName, remap(vec4(static_cast<float>(data[0]), static_cast<float>(data[1]),
			static_cast<float>(data[2]), 1.0f)));
	}
	else if (dataType == eFbxDouble4)
	{
		FbxDouble3 data = value.Get<FbxDouble4>();
		m->setVector(propName, remap(vec4(static_cast<float>(data[0]), static_cast<float>(data[1]),
			static_cast<float>(data[2]), static_cast<float>(data[3]))));
	}
	else
	{
		log::warning("Unsupported data type %d for %s", dataType, fbxprop);
	}
}

inline void describeFBXProperty(const fbxsdk::FbxProperty& prop)
{
	const char* name = prop.GetNameAsCStr();
	int lTextureCount = prop.GetSrcObjectCount<FbxFileTexture>();
	for (int i = 0; i < lTextureCount; ++i)
	{
		if (prop.GetSrcObject<FbxFileTexture>(i))
		{
			log::info("TEXTURE: %s", name);
			break;
		}
	}
	EFbxType dataType = prop.GetPropertyDataType().GetType();
	if (dataType == eFbxFloat)
	{
		log::info("FLOAT: %s = %f", name, prop.Get<float>());
	}
	else if (dataType == eFbxDouble)
	{
		log::info("DOUBLE: %s = %g", name, prop.Get<double>());
	}
	else if (dataType == eFbxDouble2)
	{
		FbxDouble2 data = prop.Get<FbxDouble2>();
		log::info("VECTOR2: %s = (%g, %g)", name, data[0], data[1]);
	}
	else if (dataType == eFbxDouble3)
	{
		FbxDouble3 data = prop.Get<FbxDouble3>();
		log::info("VECTOR3: %s = (%g, %g, %g)", name, data[0], data[1], data[2]);
	}
	else if (dataType == eFbxDouble4)
	{
		FbxDouble4 data = prop.Get<FbxDouble4>();
		log::info("VECTOR4: %s = (%g, %g, %g, %g)", name, data[0], data[1], data[2], data[3]);
	}
	else
	{
		log::info("ANOTHER PROPERTY: %s", name);
	}
}

MaterialInstance::Pointer FBXLoaderPrivate::loadMaterial(FbxSurfaceMaterial* mat)
{
	MaterialInstance::Pointer m = _renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::Microfacet)->instance();
	m->setName(mat->GetName());
	m->setFloat(MaterialVariable::MetallnessScale, 1.0f);
	m->setFloat(MaterialVariable::RoughnessScale, 1.0f);
	m->setFloat(MaterialVariable::OpacityScale, 1.0f);

	/*
	 * enumerate material properties to find something interesting
	 *
	log::info("Material %s contains properties:", mat->GetName());
	auto prop = mat->GetFirstProperty();
	while (prop.IsValid())
	{
		describeFBXProperty(prop);
		prop = mat->GetNextProperty(prop);
	}
	// */

	if (shouldCreateRenderObjects)
	{
		loadMaterialTextureValue(m, MaterialTexture::BaseColor, mat, FbxSurfaceMaterial::sDiffuse);
		loadMaterialTextureValue(m, MaterialTexture::EmissiveColor, mat, FbxSurfaceMaterial::sEmissive);
		loadMaterialTextureValue(m, MaterialTexture::Normal, mat, FbxSurfaceMaterial::sNormalMap);
		loadMaterialTextureValue(m, MaterialTexture::Metallness, mat, FbxSurfaceMaterial::sSpecularFactor);
		loadMaterialTextureValue(m, MaterialTexture::Roughness, mat, FbxSurfaceMaterial::sShininess);
		loadMaterialTextureValue(m, MaterialTexture::Opacity, mat, FbxSurfaceMaterial::sTransparentColor);
	}
	
	loadMaterialValue(m, MaterialVariable::DiffuseReflectance, mat, FbxSurfaceMaterial::sDiffuse, NullRemap);
	loadMaterialValue(m, MaterialVariable::SpecularReflectance, mat, FbxSurfaceMaterial::sSpecular, NullRemap);
	loadMaterialValue(m, MaterialVariable::EmissiveColor, mat, FbxSurfaceMaterial::sEmissive, NullRemap);
	loadMaterialValue(m, MaterialVariable::NormalScale, mat, FbxSurfaceMaterial::sBumpFactor, NullRemap);

	return m;
}


s3d::SkeletonElement::Pointer FBXLoaderPrivate::loadBone(FbxSkeleton* skeleton, s3d::BaseElement::Pointer parent)
{
	auto node = skeleton->GetNode();
	s3d::SkeletonElement::Pointer result = s3d::SkeletonElement::Pointer::create(node->GetName(), parent.pointer());
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
	s3d::BaseElement::Pointer parent, const MaterialCollection& materials)
{
	const char* mName = mesh->GetName();
	const char* nName = mesh->GetNode()->GetName();
	std::string meshName(strlen(mName) == 0 ? nName : mName);

	bool oneMaterialForPolygons = false;

	FbxGeometryElementMaterial* fbxMaterial = mesh->GetElementMaterial();
	ET_ASSERT(fbxMaterial != nullptr);

	FbxGeometryElement::EMappingMode mapping = fbxMaterial->GetMappingMode();
	ET_ASSERT((mapping == FbxGeometryElement::eAllSame) || (mapping == FbxGeometryElement::eByPolygon));
	oneMaterialForPolygons = (mapping == FbxGeometryElement::eAllSame);

	s3d::Mesh::Pointer result = s3d::Mesh::Pointer::create(meshName, parent.pointer());

	size_t uvChannels = mesh->GetElementUVCount();

	bool hasNormal = mesh->GetElementNormalCount() > 0;
	bool hasSkin = meshHasSkin(mesh);

	const FbxVector4* lControlPoints = mesh->GetControlPoints();

	bool hasTangents = mesh->GetElementTangentCount() > 0;
	FbxGeometryElementTangent* tangents = hasTangents ? mesh->GetElementTangent() : nullptr;

	bool hasColor = mesh->GetElementVertexColorCount() > 0;
	FbxGeometryElementVertexColor* vertexColor = hasColor ? mesh->GetElementVertexColor() : nullptr;

	if (hasNormal)
	{
		hasNormal &= (mesh->GetElementNormal()->GetMappingMode() != FbxGeometryElement::eNone);
	}

	if (tangents)
	{
		hasTangents &= (tangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);
	}

	if (vertexColor)
	{
		hasColor &= (vertexColor->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);
	}

	VertexDeclaration decl(true, VertexAttributeUsage::Position, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Tangent, DataType::Vec3);

	if (hasColor)
		decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);

	auto uv = mesh->GetElementUV();
	uint32_t texCoord0 = static_cast<uint32_t>(VertexAttributeUsage::TexCoord0);
	if ((uv != nullptr) && (uv->GetMappingMode() != FbxGeometryElement::eNone))
	{
		for (uint32_t i = 0; i < uvChannels; ++i)
		{
			decl.push_back(static_cast<VertexAttributeUsage>(texCoord0 + i), DataType::Vec2);
		}
	}

	if (hasSkin)
	{
		decl.push_back(VertexAttributeUsage::BlendWeights, DataType::Vec4);
		decl.push_back(VertexAttributeUsage::BlendIndices, DataType::IntVec4);
	}

	int lPolygonCount = mesh->GetPolygonCount();
	int lPolygonVertexCount = mesh->GetPolygonVertexCount();

	VertexStorage::Pointer vs = storage.vertexStorageWithDeclarationForAppendingSize(decl, lPolygonVertexCount);
	uint32_t vertexBaseOffset = vs->capacity();
	vs->increaseSize(lPolygonVertexCount);

	IndexArray::Pointer ia = storage.indexArray();
	if (ia.invalid())
	{
		ia = IndexArray::Pointer::create(IndexArrayFormat::Format_32bit, lPolygonVertexCount, PrimitiveType::Triangles);
		ia->setName("fbx-indexes");
		storage.setIndexArray(ia);
	}
	else
	{
		ia->resizeToFit(ia->actualSize() + lPolygonVertexCount);
	}
	uint32_t firstIndex = ia->actualSize() / 3;

	VertexDataAccessor<DataType::Vec3> pos;
	VertexDataAccessor<DataType::Vec3> nrm;
	VertexDataAccessor<DataType::Vec3> tan;
	VertexDataAccessor<DataType::Vec4> clr;
	VertexDataAccessor<DataType::Int> smg;
	VertexDataAccessor<DataType::Vec4> blw;
	VertexDataAccessor<DataType::IntVec4> bli;

	pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, vertexBaseOffset);

	if (hasNormal)
		nrm = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, vertexBaseOffset);

	if (hasColor)
		clr = vs->accessData<DataType::Vec4>(VertexAttributeUsage::Color, vertexBaseOffset);

	if (hasTangents)
		tan = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Tangent, vertexBaseOffset);

	if (hasSkin)
	{
		blw = vs->accessData<DataType::Vec4>(VertexAttributeUsage::BlendWeights, vertexBaseOffset);
		bli = vs->accessData<DataType::IntVec4>(VertexAttributeUsage::BlendIndices, vertexBaseOffset);
	}

	std::vector<VertexDataAccessor<DataType::Vec2>> uvs;
	uvs.reserve(uvChannels);
	for (uint32_t i = 0; i < uvChannels; ++i)
	{
		uvs.push_back(vs->accessData<DataType::Vec2>(static_cast<VertexAttributeUsage>(texCoord0 + i),
			vertexBaseOffset));
	}

	FbxStringList lUVNames;
	mesh->GetUVSetNames(lUVNames);

	uint32_t vertexCount = 0;
	uint32_t indexOffset = static_cast<uint32_t>(vertexBaseOffset);
	std::map<size_t, std::vector<uint32_t>> controlPointToMeshIndex;

#define PUSH_VERTEX FbxVector4 v = lControlPoints[lControlPointIndex]; \
		if (hasSkin) controlPointToMeshIndex[lControlPointIndex].push_back(vertexCount); \
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

	int materialIndex = 0;
	const auto& materialIndices = mesh->GetElementMaterial()->GetIndexArray();
	auto& batches = constructionInfo[vs.pointer()];
	for (const auto& material : materials)
	{
		uint32_t startIndex = indexOffset;
		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
		{
			if (oneMaterialForPolygons || (materialIndices.GetAt(lPolygonIndex) == materialIndex))
			{
				int pSize = mesh->GetPolygonSize(lPolygonIndex);
				for (int lVerticeIndex = 0; lVerticeIndex < pSize; ++lVerticeIndex)
				{
					int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
					ia->setIndex(static_cast<uint32_t>(vertexBaseOffset + vertexCount), indexOffset);
					PUSH_TANGENT
						PUSH_VERTEX
						PUSH_NORMAL
						PUSH_COLOR
						PUSH_UV
						++vertexCount;
					++indexOffset;
				}
				ET_ASSERT(pSize == 3);
			}
		}
		if (indexOffset > startIndex)
		{
			batches.emplace_back(startIndex, indexOffset - startIndex, result, material);
		}
		++materialIndex;
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
					for (uint32_t ci : convertedIndices)
						cluster->weights().emplace_back(ci, weight);
				}
			}
			cluster->sortWeights();
			deformer->addCluster(cluster);
		}

		result->setDeformer(deformer);
	}

	if (!hasNormal)
		primitives::calculateNormals(vs, ia, firstIndex, firstIndex + static_cast<uint32_t>(lPolygonCount));
	
	if (!hasTangents)
		primitives::calculateTangents(vs, ia, firstIndex, firstIndex + static_cast<uint32_t>(lPolygonCount));

	return result;
}

void FBXLoaderPrivate::buildVertexBuffers(s3d::Storage& storage)
{
	Buffer::Pointer primaryIndexBuffer;
	for (const VertexStorage::Pointer& i : storage.vertexStorages())
	{
		Buffer::Pointer vb = _renderer->createVertexBuffer("fbx-v", i, Buffer::Location::Device);
		if (primaryIndexBuffer.invalid())
		{
			primaryIndexBuffer = _renderer->createIndexBuffer("fbx-i", storage.indexArray(), Buffer::Location::Device);
		}
		
		VertexStream::Pointer vStream = VertexStream::Pointer::create();
		vStream->setVertexBuffer(vb, i->declaration());
		vStream->setIndexBuffer(primaryIndexBuffer, storage.indexArray()->format());
		vStream->setPrimitiveType(storage.indexArray()->primitiveType());

		auto ci = constructionInfo.find(i.pointer());
		if (ci != constructionInfo.end())
		{
			for (auto& mc : ci->second)
			{
				RenderBatch::Pointer rb = _renderer->allocateRenderBatch(mc.material, vStream, mc.startIndex, mc.numIndices);
				rb->setVertexStorage(i);
				rb->setIndexArray(storage.indexArray());
				rb->calculateBoundingBox();
				mc.owner->addRenderBatch(rb);
			}
		}
	}
}

StringList FBXLoaderPrivate::loadNodeProperties(FbxNode* node)
{
	StringList result;
	FbxProperty fbxProp = node->GetFirstProperty();

	while (fbxProp.IsValid())
	{
		if (fbxProp.GetPropertyDataType().GetType() == eFbxString)
		{
			FbxString str = fbxProp.Get<FbxString>();
			StringDataStorage line(static_cast<uint32_t>(str.GetLen() + 1), 0);

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

		fbxProp = node->GetNextProperty(fbxProp);
	};

	for (auto& prop : result)
	{
		lowercase(prop);
		prop.erase(std::remove_if(prop.begin(), prop.end(), [](char c) { return isWhitespaceChar(c); }), prop.end());
	}

	return result;
}

s3d::LineElement::Pointer FBXLoaderPrivate::loadLine(s3d::Storage&, FbxLine* line, s3d::BaseElement::Pointer parent,
	const StringList& /* params */)
{
	const char* lineName = line->GetName();
	if ((lineName == nullptr) || (strlen(lineName) == 0))
		lineName = line->GetNode()->GetName();

	auto result = s3d::LineElement::Pointer::create(lineName, parent.pointer());

	auto points = line->GetControlPoints();
	int numIndexes = line->GetIndexArraySize();
	for (int i = 0; i < numIndexes; ++i)
	{
		auto point = points[line->GetPointIndexAt(i)];
		result->addPoint(vec3(float(point[0]), float(point[1]), float(point[2])));
	}

	return result;
}

void FBXLoaderPrivate::linkSkeleton(s3d::ElementContainer::Pointer skeletonRoot)
{
	auto meshes = skeletonRoot->childrenOfType(s3d::ElementType::Mesh);
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
		buildBlendWeightsForMesh(mesh);
	}
}

void FBXLoaderPrivate::buildBlendWeightsForMesh(s3d::Mesh::Pointer mesh)
{
	if (mesh->deformer().invalid()) return;

	for (auto& rb : mesh->renderBatches())
	{
		auto vs = rb->vertexStorage();

		if (!vs->hasAttribute(VertexAttributeUsage::BlendIndices) ||
			!vs->hasAttribute(VertexAttributeUsage::BlendWeights)) return;

		std::map<size_t, uint32_t> placedIndices;
		auto bli = vs->accessData<DataType::IntVec4>(VertexAttributeUsage::BlendIndices, rb->firstIndex());
		auto blw = vs->accessData<DataType::Vec4>(VertexAttributeUsage::BlendWeights, rb->firstIndex());
		for (uint32_t i = 0; i < rb->numIndexes(); ++i)
		{
			bli[i] = vec4i(0);
			blw[i] = vec4(0.0f);
		}

		int clusterIndex = 0;
		const auto& clusters = mesh->deformer()->clusters();
		for (const auto& cluster : clusters)
		{
			const auto& weights = cluster->weights();
			for (const auto& weight : weights)
			{
				uint32_t componentIndex = 0;
				if (placedIndices.count(weight.index) > 0)
				{
					componentIndex = placedIndices[weight.index] + 1;
				}
				if (componentIndex > 3)
				{
					log::warning("More than four bones affecting vertex %u", weight.index);
				}
				else
				{
					bli[weight.index][componentIndex] = clusterIndex;
					blw[weight.index][componentIndex] = weight.weight;
				}

				placedIndices[weight.index] = componentIndex;
			}
			++clusterIndex;
		}
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

s3d::ElementContainer::Pointer FBXLoader::load(RenderInterface::Pointer renderer, s3d::Storage& storage, ObjectsCache& cache)
{
	s3d::ElementContainer::Pointer result(PointerInit::CreateInplace);
	FBXLoaderPrivate* loader = etCreateObject<FBXLoaderPrivate>(renderer, cache);
	loader->shouldCreateRenderObjects = _shouldCreateRenderObjects;

	if (loader->import(_filename))
	{
		result = loader->parse(storage);
	}

	Invocation([loader]()
	{
		etDestroyObject(loader);
	}).invokeInCurrentRunLoop();

	return result;
}

#else // ET_HAVE_FBX_SDK
#	if (ET_PLATFORM_WIN)
#		pragma message ("Define ET_HAVE_FBX_SDK to compile FBXLoader")
#	else
#		warning Set ET_HAVE_FBX_SDK to 1 in order to compile FBXLoader
#	endif
#endif
