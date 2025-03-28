#include "mesh_loader.h"
#include "texture_mgr.h"
#include "material_system.h"

#include <filesystem>

llt::MeshLoader *llt::g_meshLoader = nullptr;

using namespace llt;

MeshLoader::MeshLoader()
	: m_meshCache()
	, m_importer()
	, m_quadMesh(nullptr)
	, m_cubeMesh(nullptr)
{
	createQuadMesh();
	createCubeMesh();
}

MeshLoader::~MeshLoader()
{
	delete m_quadMesh;
	delete m_cubeMesh;

	for (auto &[name, mesh] : m_meshCache) {
		delete mesh;
	}

	m_meshCache.clear();
}

void MeshLoader::createQuadMesh()
{
	Vector<PrimitiveUVVertex> vertices =
	{
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }
	};

	Vector<uint16_t> indices =
	{
		0, 1, 2,
		0, 2, 3
	};

	m_quadMesh = new SubMesh();
	m_quadMesh->build(
		g_primitiveUvVertexFormat,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

void MeshLoader::createCubeMesh()
{
	Vector<PrimitiveVertex> vertices =
	{
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } }
	};

	Vector<uint16_t> indices =
	{
		0, 2, 1,
		2, 0, 3,

		7, 5, 6,
		5, 7, 4,

		4, 1, 5,
		1, 4, 0,

		3, 6, 2,
		6, 3, 7,

		1, 6, 5,
		6, 1, 2,

		4, 3, 0,
		3, 4, 7
	};

	m_cubeMesh = new SubMesh();
	m_cubeMesh->build(
		g_primitiveVertexFormat,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

SubMesh *MeshLoader::getQuadMesh()
{
	return m_quadMesh;
}

SubMesh *MeshLoader::getCubeMesh()
{
	return m_cubeMesh;
}

Mesh *MeshLoader::loadMesh(const String &name, const String &path)
{
	if (m_meshCache.contains(name)) {
		return m_meshCache.get(name);
	}

	const aiScene *scene = m_importer.ReadFile(path.cstr(),
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs
	);

	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		LLT_ERROR("Failed to load mesh at path: %s, Error: %s", path.cstr(), m_importer.GetErrorString());
		return nullptr;
	}

	Mesh *mesh = new Mesh();

	std::filesystem::path filePath(path.cstr());
	std::string directory = filePath.parent_path().string() + "/";

	mesh->setDirectory(directory.c_str());

	aiMatrix4x4 identity(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	processNodes(mesh, scene->mRootNode, scene, identity);

	m_meshCache.insert(name, mesh);
	return mesh;
}

void MeshLoader::processNodes(Mesh *mesh, aiNode *node, const aiScene *scene, const aiMatrix4x4& transform)
{
	for(int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *assimpMesh = scene->mMeshes[node->mMeshes[i]];
		processSubMesh(mesh->createSubmesh(), assimpMesh, scene, node->mTransformation * transform);
	}

	for(int i = 0; i < node->mNumChildren; i++)
	{
		processNodes(mesh, node->mChildren[i], scene, node->mTransformation * transform);
	}
}

void MeshLoader::processSubMesh(SubMesh *submesh, aiMesh *assimpMesh, const aiScene *scene, const aiMatrix4x4& transform)
{
	Vector<ModelVertex> vertices(assimpMesh->mNumVertices);
	Vector<uint16_t> indices;

	for (int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		const aiVector3D &vtx = transform * assimpMesh->mVertices[i];

		ModelVertex vertex = {};

		vertex.position = { vtx.x, vtx.y, vtx.z };

		if (assimpMesh->HasTextureCoords(0))
		{
			const aiVector3D &uv = assimpMesh->mTextureCoords[0][i];

			vertex.uv = { uv.x, uv.y };
		}
		else
		{
			vertex.uv = { 0.0f, 0.0f };
		}

		if (assimpMesh->HasVertexColors(0))
		{
			const aiColor4D &col = assimpMesh->mColors[0][i];

			vertex.colour = { col.r, col.g, col.b };
		}
		else
		{
			vertex.colour = { 1.0f, 1.0f, 1.0f };
		}

		if (assimpMesh->HasNormals())
		{
			const aiVector3D &nml = transform * assimpMesh->mNormals[i]; // this literally wont work lmao

			vertex.normal = { nml.x, nml.y, nml.z };
		}
		else
		{
			vertex.normal = { 0.0f, 0.0f, 1.0f };
		}

		if (assimpMesh->HasTangentsAndBitangents())
		{
			const aiVector3D &tangent = transform * assimpMesh->mTangents[i];
			const aiVector3D &bitangent = transform * assimpMesh->mBitangents[i];

			vertex.tangent = { tangent.x, tangent.y, tangent.z };
			vertex.bitangent = { bitangent.x, bitangent.y, bitangent.z };
		}
		else
		{
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
			vertex.bitangent = { 0.0f, 0.0f, 0.0f };
		}

		vertices[i] = vertex;
	}

	for (int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		const aiFace &face = assimpMesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.pushBack(face.mIndices[j]);
		}
	}

	submesh->build(
		g_modelVertexFormat,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);

	if (assimpMesh->mMaterialIndex >= 0)
	{
		const aiMaterial *assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];

		MaterialData data;
		data.technique = "texturedPBR_opaque"; // temporarily just the forced material type

		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE,				g_materialSystem->getDiffuseFallback());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_LIGHTMAP,				g_materialSystem->getAOFallback());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS,	g_materialSystem->getRoughnessMetallicFallback());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_NORMALS,				g_materialSystem->getNormalFallback());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_EMISSIVE,				g_materialSystem->getEmissiveFallback());

		Material *material = g_materialSystem->getRegistry().buildMaterial(data);

		submesh->setMaterial(material);
	}
}

void MeshLoader::fetchMaterialBoundTextures(Vector<TextureView> &textures, const String &localPath, const aiMaterial *material, aiTextureType type, Texture *fallback)
{
	Vector<Texture *> maps = loadMaterialTextures(material, type, localPath);

	if (maps.size() >= 1)
	{
		textures.pushBack(maps[0]->getStandardView());
	}
	else
	{
		if (fallback)
		{
			textures.pushBack(fallback->getStandardView());
		}
	}
}

Vector<Texture *> MeshLoader::loadMaterialTextures(const aiMaterial *material, aiTextureType type, const String &localPath)
{
	Vector<Texture *> result;

	for (int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString texturePath;
		material->GetTexture(type, i, &texturePath);

		aiString basePath = aiString(localPath.cstr());
		basePath.Append(texturePath.C_Str());

		Texture *tex = g_textureManager->getTexture(basePath.C_Str());

		if (!tex)
			tex = g_textureManager->load(basePath.C_Str(), basePath.C_Str());

		result.pushBack(tex);
	}

	return result;
}
