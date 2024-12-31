#ifndef MESH_LOADER_H_
#define MESH_LOADER_H_

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../container/string.h"
#include "../container/hash_map.h"

#include "mesh.h"

namespace llt
{
	class MeshLoader
	{
	public:
		MeshLoader();
		~MeshLoader();

		Mesh* loadMesh(const String& name, const String& path);

	private:
		void processNodes(Mesh* mesh, aiNode* node, const aiScene* scene);
		void processSubMesh(llt::SubMesh* submesh, aiMesh* assimpMesh, const aiScene* scene);

		Vector<Texture*> loadMaterialTextures(const aiMaterial* material, aiTextureType type);

		HashMap<String, Mesh*> m_meshCache;
		Assimp::Importer m_importer;
	};

	extern MeshLoader* g_meshLoader;
}

#endif // MESH_LOADER_H_