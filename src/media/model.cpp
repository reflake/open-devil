#include <algorithm>
#include <array>
#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <glm/glm.hpp>

#include "model.hpp"

// Функция для нахождения AABB границ
std::pair<glm::vec3, glm::vec3> findAABB(const std::vector<glm::vec3>& vectors) {
    glm::vec3 minPoint(std::numeric_limits<float>::max());
    glm::vec3 maxPoint(std::numeric_limits<float>::lowest());

    for (const auto& vec : vectors) {
        if (vec.x < minPoint.x) minPoint.x = vec.x;
        if (vec.y < minPoint.y) minPoint.y = vec.y;
        if (vec.z < minPoint.z) minPoint.z = vec.z;

        if (vec.x > maxPoint.x) maxPoint.x = vec.x;
        if (vec.y > maxPoint.y) maxPoint.y = vec.y;
        if (vec.z > maxPoint.z) maxPoint.z = vec.z;
    }

    return {minPoint, maxPoint};
}

Mesh readModel( const std::string& meshPath ) {

    Assimp::Importer importer;

	const auto aiFlags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile( meshPath, aiFlags );

	if ( scene == nullptr ) {

		throw std::runtime_error("Failed to load mesh");
	}

	if ( scene->HasMaterials() ) {

	} else {
		
		throw std::runtime_error("Materials not found");
	}

	if ( scene->HasMeshes() ) {

		const int sizeOfFace = 3;
		auto meshes = scene->mMeshes;

		// Count vertice offsets
		const int meshesCount = scene->mNumMeshes;

		std::vector<int> vertOffsets(meshesCount);
		int vertCount = 0;

		std::vector<int> indexOffsets(meshesCount);
		int indicesCount = 0;

		for ( int i = 0; i < meshesCount; i++ ) {

			auto mesh = meshes[i];

			assert( mesh->GetNumUVChannels() == 1 ); // only one UV channel should be presented
			assert( mesh->mNumUVComponents[0] == 2 ); // two components for texture coordinates U and V

			indexOffsets[i] = indicesCount;
			indicesCount += mesh->mNumFaces * sizeOfFace;

			vertOffsets[i] = vertCount;
			vertCount += mesh->mNumVertices;
		}

		// Combine meshes
		std::vector<glm::vec3> vertPositions( vertCount );
		std::vector<glm::vec2> texCoords( vertCount );
		std::vector<int>	   triIndices( indicesCount );

		for ( int i = 0; i < meshesCount; i++ ) {

			const int vertOffset = vertOffsets[i];
			const int indicesOffset = indexOffsets[i];
			const auto mesh = meshes[i];

			memcpy( &vertPositions[vertOffset], mesh->mVertices, sizeof( glm::vec3 ) * mesh->mNumVertices );

			// Write texture coordinates
			auto textureCoordsChannel = mesh->mTextureCoords[0];

			for ( int j = 0; j < mesh->mNumVertices; j++ ) {

				auto uvVec = textureCoordsChannel[j];

				texCoords[vertOffset + j] = glm::vec2( uvVec.x, uvVec.y );
			}

			// Adjust triangle indices to offset for each submesh
			for ( int j = 0; j < mesh->mNumFaces; j++ ) {

				triIndices[indicesOffset + j * sizeOfFace + 0] = mesh->mFaces[j].mIndices[0] + vertOffset;
				triIndices[indicesOffset + j * sizeOfFace + 1] = mesh->mFaces[j].mIndices[1] + vertOffset;
				triIndices[indicesOffset + j * sizeOfFace + 2] = mesh->mFaces[j].mIndices[2] + vertOffset;
			}
		}

		auto aabb = findAABB( vertPositions );

		printf("Max bound %f %f, %f\n", aabb.first.x, aabb.first.y, aabb.first.z );
		printf("Min bound %f %f, %f\n", aabb.second.x, aabb.second.y, aabb.second.z );

		return Mesh { 
			.vertPositions = vertPositions,
			.texCoords = texCoords,
			.indices = triIndices,
		};
	}

	throw std::runtime_error("No meshes found!");
}