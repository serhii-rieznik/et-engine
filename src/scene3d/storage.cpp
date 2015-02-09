/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/json/json.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace et::s3d;

Storage::Storage(const std::string& name, BaseElement* parent) :
	ElementContainer(name, parent)
{
	_indexArray = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 0, PrimitiveType::Triangles);
	_indexArray->setName("mainIndexBuffer");
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

void Storage::serialize(Dictionary stream, const std::string& basePath)
{
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

		std::string libraryName = replaceFileExt(basePath, ".etxmtls");

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
			declDictionary.setStringForKey(kType, vertexAttributeTypeToString(e.type()));
			declDictionary.setStringForKey(kDataType, dataTypeToString(e.dataType()));
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

	size_t indexesDataSize = etMin(_indexArray->dataSize(),
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

	ElementContainer::serialize(stream, basePath);
}

void Storage::deserialize(Dictionary stream, ElementFactory* factory)
{
	ElementContainer::deserialize(stream, factory);
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
	
	Material::Map::iterator mi = _materials.begin();
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
