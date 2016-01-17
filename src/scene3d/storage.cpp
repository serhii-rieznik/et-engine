/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/json/json.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace et::s3d;

Storage::Storage()
{
}

void Storage::addVertexStorage(const VertexStorage::Pointer& vs)
{
	_vertexStorages.push_back(vs);
}

void Storage::setIndexArray(const IndexArray::Pointer& ia)
{
	_indexArray = ia;
}

VertexStorage::Pointer Storage::addVertexStorageWithDeclaration(const VertexDeclaration& decl, size_t size)
{
	auto storage = VertexStorage::Pointer::create(decl, size);
	storage->setName("vertexStorage" + intToStr(_vertexStorages.size()));
	_vertexStorages.push_back(storage);

	return _vertexStorages.back();
}

VertexStorage::Pointer Storage::vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, size_t size)
{
	for (const auto&  i : _vertexStorages)
	{
		if (i->declaration().hasSameElementsAs(decl) && (i->capacity() + size < IndexArray::MaxShortIndex))
			return i;
	}
	
	return addVertexStorageWithDeclaration(decl, 0);
}

int Storage::indexOfVertexStorage(const VertexStorage::Pointer& vs)
{
	int index = 0;

	for (const auto&  i : _vertexStorages)
	{
		if (i == vs)
			return index;
		
		++index;
	}

	return -1;
}

Dictionary Storage::serialize(const std::string& basePath)
{
	Dictionary stream;

	if (!_materials.empty())
	{
		Dictionary materialsDictionary;
		for (auto& kv : _materials)
		{
			if (kv.first != kv.second->name())
			{
				log::warning("Mismatch for material's key and name: %s and %s, using key to serialize", 
					kv.first.c_str(), kv.second->name().c_str());
			}

			Dictionary materialDictionary;
			kv.second->serialize(materialDictionary, basePath);
			materialsDictionary.setDictionaryForKey(kv.first, materialDictionary);
		}

		std::string libraryName = replaceFileExt(basePath, "-materials.json");

		auto serializedData = json::serialize(materialsDictionary, json::SerializationFlag_ReadableFormat);
		BinaryDataStorage binaryData(serializedData.size() + 1, 0);
		etCopyMemory(binaryData.data(), serializedData.data(), serializedData.size());
		binaryData.writeToFile(libraryName);

		stream.setStringForKey(kMaterials, getFileName(libraryName));
	}

	size_t index = 0;
	Dictionary storagesDictionary;
	for (const auto& vs : _vertexStorages)
	{
		std::string binaryName = replaceFileExt(basePath, ".storage-" + intToStr(index) + ".etvs");

		ArrayValue declaration;
		declaration->content.reserve(vs->declaration().elements().size());
		for (const auto& e : vs->declaration().elements())
		{
			Dictionary declDictionary;
			declDictionary.setStringForKey(kUsage, vertexAttributeUsageToString(e.usage()));
			declDictionary.setStringForKey(kType, dataTypeToString(e.type()));
			declDictionary.setStringForKey(kDataType, dataFormatToString(e.dataFormat()));
			declDictionary.setIntegerForKey(kStride, e.stride());
			declDictionary.setIntegerForKey(kOffset, e.offset());
			declDictionary.setIntegerForKey(kComponents, e.components());
			declaration->content.push_back(declDictionary);
		}

		Dictionary storageDictionary;
		storageDictionary.setStringForKey(kName, vs->name());
		storageDictionary.setStringForKey(kBinary, getFileName(binaryName));
		storageDictionary.setArrayForKey(kVertexDeclaration, declaration);
		storageDictionary.setIntegerForKey(kDataSize, vs->data().dataSize());
		storagesDictionary.setDictionaryForKey(vs->name(), storageDictionary);

		std::ofstream fOut(binaryName, std::ios::out | std::ios::binary);
		fOut.write(vs->data().binary(), vs->data().dataSize());
		fOut.flush();
		fOut.close();

		++index;
	}
	stream.setDictionaryForKey(kVertexStorages, storagesDictionary);

	std::string binaryName = replaceFileExt(basePath, ".indexes.etvs");

	size_t indexesDataSize = std::min(_indexArray->dataSize(),
		_indexArray->actualSize() * static_cast<size_t>(_indexArray->format()));

	Dictionary indexArrayDictionary;
	indexArrayDictionary.setStringForKey(kBinary, getFileName(binaryName));
	indexArrayDictionary.setIntegerForKey(kDataSize, indexesDataSize);
	indexArrayDictionary.setIntegerForKey(kIndexesCount, _indexArray->actualSize());
	indexArrayDictionary.setStringForKey(kPrimitiveType, primitiveTypeToString(_indexArray->primitiveType()));
	indexArrayDictionary.setStringForKey(kFormat, indexArrayFormatToString(_indexArray->format()));
	stream.setDictionaryForKey(kIndexArray, indexArrayDictionary);

	std::ofstream fOut(binaryName, std::ios::out | std::ios::binary);
	fOut.write(_indexArray->binary(), indexesDataSize);
	fOut.flush();
	fOut.close();

	return stream;
}

void Storage::deserializeWithOptions(RenderContext* rc, Dictionary stream, SerializationHelper* helper,
	ObjectsCache& cache, uint32_t options)
{
	auto materialsLibrary = stream.stringForKey(kMaterials)->content;
	if (!fileExists(materialsLibrary))
		materialsLibrary = helper->serializationBasePath() + materialsLibrary;

	if (fileExists(materialsLibrary))
	{
		ValueClass vc = ValueClass_Invalid;
		Dictionary materials = json::deserialize(loadTextFile(materialsLibrary), vc);
		if (vc == ValueClass_Dictionary)
		{
			for (const auto kv : materials->content)
			{
				Dictionary materialInfo(kv.second);
				SceneMaterial::Pointer material = SceneMaterial::Pointer::create();
				material->deserializeWithOptions(materialInfo, rc, cache,
					helper->serializationBasePath(), options);
				addMaterial(material);
			}
		}
	}

	Dictionary vsmap = stream.dictionaryForKey(kVertexStorages);
	for (const auto& kv : vsmap->content)
	{
		Dictionary storage(kv.second);
		ArrayValue declInfo = storage.arrayForKey(kVertexDeclaration);

		VertexDeclaration decl(true);
		for (Dictionary e : declInfo->content)
		{
			bool comp = false;
			decl.push_back(stringToVertexAttributeUsage(e.stringForKey(kUsage)->content, comp),
				stringToDataType(e.stringForKey(kType)->content));
		}
		size_t capacity = static_cast<size_t>(storage.integerForKey(kDataSize)->content) / decl.dataSize();

		VertexStorage::Pointer vs = VertexStorage::Pointer::create(decl, capacity);
		vs->setName(storage.stringForKey(kName)->content);

		auto binaryFileName = storage.stringForKey(kBinary)->content;
		if (!fileExists(binaryFileName))
			binaryFileName = helper->serializationBasePath() + binaryFileName;

		std::ifstream fIn(binaryFileName, std::ios::in | std::ios::binary);
		if (fIn.good())
		{
			fIn.read(vs->data().binary(), vs->data().dataSize());
			fIn.close();
		}
		addVertexStorage(vs);
	}

	Dictionary iaInfo = stream.dictionaryForKey(kIndexArray);
	IndexArrayFormat fmt = stringToIndexArrayFormat(iaInfo.stringForKey(kFormat)->content);
	PrimitiveType pt = stringToPrimitiveType(iaInfo.stringForKey(kPrimitiveType)->content);
	uint32_t indexesCount = static_cast<uint32_t>(iaInfo.integerForKey(kIndexesCount)->content);
	
	IndexArray::Pointer ia = IndexArray::Pointer::create(fmt, indexesCount, pt);
	ia->setActualSize(indexesCount);

	auto binaryFileName = iaInfo.stringForKey(kBinary)->content;
	if (!fileExists(binaryFileName))
		binaryFileName = helper->serializationBasePath() + binaryFileName;

	std::ifstream fIn(binaryFileName, std::ios::in | std::ios::binary);
	if (fIn.good())
	{
		fIn.read(ia->binary(), ia->dataSize());
		fIn.close();
	}
	setIndexArray(ia);
}

void Storage::flush()
{
	auto vi = _vertexStorages.begin();
	while (vi != _vertexStorages.end())
	{
		if (vi->ptr()->atomicCounterValue() == 1)
			vi = _vertexStorages.erase(vi);
		else
			++vi;
	}
	
	if (_indexArray.valid() && (_indexArray->atomicCounterValue() == 1))
		_indexArray = IndexArray::Pointer();
	
	SceneMaterial::Map::iterator mi = _materials.begin();
	while (mi != _materials.end())
	{
		if (mi->second.ptr()->atomicCounterValue() == 1)
			mi = _materials.erase(mi);
		else
			++mi;
	}
	
	auto ti = _textures.begin();
	while (ti != _textures.end())
	{
		if (ti->ptr()->atomicCounterValue() == 1)
			ti = _textures.erase(ti);
		else
			++ti;
	}
}
