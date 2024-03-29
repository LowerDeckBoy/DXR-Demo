#include "../../Core/DeviceContext.hpp"
#include <assimp/scene.h>
#include "Importer.hpp"
#include "../../Utilities/FileUtils.hpp"
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

Importer::Importer(DeviceContext* pDevice, std::string_view Filepath)
{
}

bool Importer::Import(DeviceContext* pDevice, std::string_view Filepath)
{
	Assimp::Importer importer;
	auto loadFlags{  
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_PreTransformVertices |
		aiProcess_ValidateDataStructure 
	};
	importer.SetExtraVerbose(true);
	const aiScene* scene{ importer.ReadFile(Filepath.data(), loadFlags) };
	
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		throw std::runtime_error(importer.GetErrorString());
	}

	m_ModelPath = Filepath;
	assert(m_Device = pDevice);

	ProcessNode(scene, scene->mRootNode, nullptr, XMMatrixIdentity());
	
	if (scene->HasMaterials())
		m_Materials.reserve(scene->mNumMaterials);
	
	if (scene->HasTextures())
		m_Textures.reserve(scene->mNumTextures);
	
	importer.FreeScene();

	return true;
}

void Importer::ProcessNode(const aiScene* pScene, const aiNode* pNode, model::Node* ParentNode, XMMATRIX ParentMatrix)
{
	model::Node* newNode{ new model::Node() };
	newNode->Parent = ParentNode;
	newNode->Name = std::string(pNode->mName.C_Str());

	if (!pNode->mTransformation.IsIdentity())
	{
		XMFLOAT4X4 temp{ XMFLOAT4X4() };
		temp._11 = static_cast<float>(pNode->mTransformation.a1);
		temp._12 = static_cast<float>(pNode->mTransformation.a2);
		temp._13 = static_cast<float>(pNode->mTransformation.a3);
		temp._14 = static_cast<float>(pNode->mTransformation.a4);
		temp._21 = static_cast<float>(pNode->mTransformation.b1);
		temp._22 = static_cast<float>(pNode->mTransformation.b2);
		temp._23 = static_cast<float>(pNode->mTransformation.b3);
		temp._24 = static_cast<float>(pNode->mTransformation.b4);
		temp._31 = static_cast<float>(pNode->mTransformation.c1);
		temp._32 = static_cast<float>(pNode->mTransformation.c2);
		temp._33 = static_cast<float>(pNode->mTransformation.c3);
		temp._34 = static_cast<float>(pNode->mTransformation.c4);
		temp._41 = static_cast<float>(pNode->mTransformation.d1);
		temp._42 = static_cast<float>(pNode->mTransformation.d2);
		temp._43 = static_cast<float>(pNode->mTransformation.d3);
		temp._44 = static_cast<float>(pNode->mTransformation.d4);
		newNode->Matrix = XMLoadFloat4x4(&temp);
	}
	else
	{
		aiVector3D translation;
		aiQuaternion rotation;
		aiVector3D scale;

		pNode->mTransformation.Decompose(scale, rotation, translation);
		newNode->Translation = XMFLOAT3(translation.x, translation.y, translation.z);
		newNode->Rotation = XMFLOAT4(rotation.x, rotation.y, rotation.z, rotation.w);
		newNode->Scale = XMFLOAT3(scale.x, scale.y, scale.z);
	}

	XMMATRIX local{ XMMatrixScalingFromVector(XMLoadFloat3(&newNode->Scale)) * XMMatrixRotationQuaternion(XMLoadFloat4(&newNode->Rotation)) * XMMatrixTranslationFromVector(XMLoadFloat3(&newNode->Translation))  };
	XMMATRIX next{ local * ParentMatrix };

	if (pNode->mChildren)
	{
		for (size_t i = 0; i < pNode->mNumChildren; i++)
			ProcessNode(pScene, pNode->mChildren[i], newNode, next);
	}

	if (pNode->mMeshes)
	{
		for (uint32_t i = 0; i < pNode->mNumMeshes; i++)
			m_Meshes.emplace_back(ProcessMesh(pScene, pScene->mMeshes[pNode->mMeshes[i]], next));
	}

	//if (ParentNode)
	//	ParentNode->Children.emplace_back(newNode);
	//else
	//	m_Nodes.emplace_back(newNode);
}

model::Mesh* Importer::ProcessMesh(const aiScene* pScene, const aiMesh* pMesh, XMMATRIX Matrix)
{
	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT2> uvs;
	std::vector<XMFLOAT3> normals;
	//std::vector<XMFLOAT3> tangents;
	//std::vector<XMFLOAT3> bitangents;

	model::Mesh* newMesh{ new model::Mesh() };

	newMesh->Matrix = Matrix;
	newMesh->Name = pMesh->mName.data;

	newMesh->BaseVertexLocation = static_cast<uint32_t>(m_Vertices.size());
	newMesh->FirstIndexLocation = static_cast<uint32_t>(m_Indices.size());
	newMesh->StartVertexLocation = static_cast<uint32_t>(m_Vertices.size());

	newMesh->VertexCount = static_cast<uint32_t>(pMesh->mNumVertices);

	for (uint32_t i = 0; i < pMesh->mNumVertices; i++)
	{
		if (pMesh->HasPositions())
			positions.emplace_back(pMesh->mVertices[i].x, pMesh->mVertices[i].y, pMesh->mVertices[i].z);
		else
			positions.emplace_back(0.0f, 0.0f, 0.0f);

		if (pMesh->mTextureCoords[0])
			uvs.emplace_back(pMesh->mTextureCoords[0][i].x, pMesh->mTextureCoords[0][i].y);
		else
			uvs.emplace_back(0.0f, 0.0f);
		
		
		if (pMesh->HasNormals())
			normals.emplace_back(pMesh->mNormals[i].x, pMesh->mNormals[i].y, pMesh->mNormals[i].z);
		else
			normals.emplace_back(0.0f, 0.0f, 0.0f);

		//if (pMesh->HasTangentsAndBitangents())
		//{
		//	tangents.emplace_back(pMesh->mTangents[i].x, pMesh->mTangents[i].y, pMesh->mTangents[i].z);
		//	bitangents.emplace_back(pMesh->mBitangents[i].x, pMesh->mBitangents[i].y, pMesh->mBitangents[i].z);
		//}
		//else
		//{
		//	tangents.emplace_back(0.0f, 0.0f, 0.0f);
		//	bitangents.emplace_back(0.0f, 0.0f, 0.0f);
		//}
		
	}

	newMesh->VertexCount = static_cast<uint32_t>(positions.size());
	for (uint32_t i = 0; i < pMesh->mNumVertices; i++)
		m_Vertices.emplace_back(positions.at(i), uvs.at(i), normals.at(i));
		//m_Vertices.emplace_back(positions.at(i), uvs.at(i), normals.at(i), tangents.at(i), bitangents.at(i));

	uint32_t indexCount{ 0 };
	if (pMesh->HasFaces())
	{
		newMesh->bHasIndices = true;
		for (uint32_t i = 0; i < pMesh->mNumFaces; i++)
		{
			aiFace& face{ pMesh->mFaces[i] };
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				m_Indices.push_back(face.mIndices[j]);
				indexCount++;
			}
		}
	}

	newMesh->IndexCount = static_cast<uint32_t>(indexCount);

	ProcessMaterials(pScene, pMesh);

	return newMesh;
}

void Importer::ProcessMaterials(const aiScene* pScene, const aiMesh* pMesh)
{
	model::MaterialData* newMaterial{ new model::MaterialData() };

	if (pMesh->mMaterialIndex < 0)
	{
		m_Materials.emplace_back(newMaterial);
		return;
	}

	aiMaterial* material{ pScene->mMaterials[pMesh->mMaterialIndex] };
	for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i)
	{
		if (material->GetTextureCount(aiTextureType_DIFFUSE) >= 0)
		{
			aiString materialPath;
			if (material->GetTexture(aiTextureType_DIFFUSE, i, &materialPath) == aiReturn_SUCCESS)
			{
				auto texPath{ files::glTF::GetTexturePath(m_ModelPath.data(), std::string(materialPath.C_Str())) };
				Texture* BaseColorTexture = new Texture(m_Device, texPath);
				m_Textures.push_back(BaseColorTexture);
				newMaterial->BaseColorIndex = BaseColorTexture->m_Descriptor.Index;

				aiColor4D colorFactor{};
				aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &colorFactor);
				newMaterial->BaseColorFactor = XMFLOAT4(colorFactor.r, colorFactor.g, colorFactor.b, colorFactor.a);

				std::string path{ "Loaded: " + texPath };
				//Logger::Log(path);
			}
			//else
			//	Logger::Log("Failed to get Diffuse texture!");
		}
	}
	
	for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); ++i)
	{
		aiString materialPath;
		if (material->GetTexture(aiTextureType_NORMALS, i, &materialPath) == aiReturn_SUCCESS)
		{
			auto texPath{ files::glTF::GetTexturePath(m_ModelPath.data(), std::string(materialPath.C_Str())) };
			Texture* NormalTexture = new Texture(m_Device, texPath);
			m_Textures.push_back(NormalTexture);
			newMaterial->NormalIndex = NormalTexture->m_Descriptor.Index;

			//std::string path{ "Loaded: " + texPath };
			//Logger::Log(path);
		}
		//else
		//	Logger::Log("Failed to get Normal texture!", LogType::eError);
	}

	for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_METALNESS); ++i)
	{
		aiString materialPath{};
		if (material->GetTexture(aiTextureType_METALNESS, i, &materialPath) == aiReturn_SUCCESS)
		{
			auto texPath{ files::glTF::GetTexturePath(m_ModelPath.data(), std::string(materialPath.C_Str())) };
			Texture* MetallicRoughnessTexture = new Texture(m_Device, texPath);
			m_Textures.push_back(MetallicRoughnessTexture);
			newMaterial->MetallicRoughnessIndex = MetallicRoughnessTexture->m_Descriptor.Index;

			aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &newMaterial->MetallicFactor);
			aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &newMaterial->RoughnessFactor);
			
			//std::string path{ "Loaded: " + texPath };
			//Logger::Log(path.c_str());
		}
		//else
		//	Logger::Log("Failed to get MetalRoughness texture!", LogType::eError);
	}

	for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_EMISSIVE); ++i)
	{
		aiString materialPath{};
		if (material->GetTexture(aiTextureType_EMISSIVE, i, &materialPath) == aiReturn_SUCCESS)
		{
			auto texPath{ files::glTF::GetTexturePath(m_ModelPath.data(), std::string(materialPath.C_Str())) };
			Texture* EmissiveTexture = new Texture(m_Device, texPath);
			m_Textures.push_back(EmissiveTexture);
			newMaterial->EmissiveIndex = EmissiveTexture->m_Descriptor.Index;

			aiColor4D colorFactor{};
			aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &colorFactor);
			newMaterial->EmissiveFactor = XMFLOAT4(colorFactor.r, colorFactor.g, colorFactor.b, colorFactor.a);

			//std::string path{ "Loaded: " + texPath };
			//Logger::Log(path.c_str());
		}
		//else
		//	Logger::Log("Failed to get Emissive texture!", LogType::eError);
	}

	aiGetMaterialFloat(material, AI_MATKEY_GLTF_ALPHACUTOFF, &newMaterial->AlphaCutoff);

	m_Materials.emplace_back(newMaterial);
}
