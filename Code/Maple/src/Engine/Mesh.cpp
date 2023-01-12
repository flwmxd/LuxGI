//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include <glm/glm.hpp>
#include <imgui.h>

#include <Scene/Component/MeshRenderer.h>
#include <IoC/SystemBuilder.h>


#include "Application.h"
#include "Mesh.h"
#include "RHI/StorageBuffer.h"
#include "Vertex.h"
#define _USE_MATH_DEFINES
#include "Math/BoundingBox.h"
#include <math.h>

namespace maple
{
	static std::atomic<int32_t> idGenerator = 1;

	Mesh::Mesh(const std::shared_ptr<VertexBuffer> &vertexBuffer, const std::shared_ptr<IndexBuffer> &indexBuffer) :
	    vertexBuffer(vertexBuffer),
	    indexBuffer(indexBuffer)
	{
		meshId = idGenerator++;
		subMeshIndex.emplace_back(indexBuffer->getCount());
		vertexCount = vertexBuffer->getSize() / sizeof(Vertex);
	}

	Mesh::Mesh(const std::vector<uint32_t> &indices, const std::vector<Vertex> &vertices) :
	    vertices(vertices), indices(indices)
	{
		vertexCount = vertices.size();
		boundingBox = std::make_shared<BoundingBox>();
		for (auto &vertex : vertices)
		{
			boundingBox->merge(vertex.pos);
		}
		vertexBuffer = VertexBuffer::create(vertices.data(), sizeof(Vertex) * vertices.size());
		indexBuffer  = IndexBuffer::create(indices.data(), indices.size());
		meshId       = idGenerator++;
		subMeshIndex.emplace_back(indexBuffer->getCount());
	}

	auto Mesh::setIndicies(uint32_t range) -> void
	{
		subMeshIndex.emplace_back(range);
	}

	auto Mesh::createQuad(bool screen) -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data(4);

		data[0].pos      = glm::vec3(-1.0f, -1.0f, 0.0f);
		data[0].texCoord = glm::vec2(0.0f, 0.0f);
		data[0].color    = glm::vec4(1.0f);

		data[1].pos      = glm::vec3(1.0f, -1.0f, 0.0f);
		data[1].texCoord = glm::vec2(1.0f, 0.0f);
		data[1].color    = glm::vec4(1.f);

		data[2].pos      = glm::vec3(1.0f, 1.0f, 0.0f);
		data[2].texCoord = glm::vec2(1.0f, 1.0f);
		data[2].color    = glm::vec4(1.f);

		data[3].pos      = glm::vec3(-1.0f, 1.0f, 0.0f);
		data[3].texCoord = glm::vec2(0.0f, 1.0f);
		data[3].color    = glm::vec4(1.f);

		std::vector<uint32_t> indices = {
		    0,
		    1,
		    2,
		    2,
		    3,
		    0,
		};
#ifdef MAPLE_VULKAN
		indices = {
		    0,
		    3,
		    2,
		    2,
		    1,
		    0,
		};
#endif        // MAPLE_VULKAN
		Mesh::generateTangents(data, indices);

		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createQuaterScreenQuad() -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data(4);

		data[0].pos      = glm::vec3(-1.0f, -1.0f, 0.0f);
		data[0].texCoord = glm::vec2(0.0f, 0.0f);
		data[0].color    = glm::vec4(1.f);

		data[1].pos      = glm::vec3(0.f, -1.0f, 0.0f);
		data[1].texCoord = glm::vec2(1.0f, 0.0f);
		data[1].color    = glm::vec4(1.f);

		data[2].pos      = glm::vec3(0.f, 0.f, 0.0f);
		data[2].texCoord = glm::vec2(1.0f, 1.0f);
		data[2].color    = glm::vec4(1.f);

		data[3].pos      = glm::vec3(-1.0f, 0.f, 0.0f);
		data[3].texCoord = glm::vec2(0.0f, 1.0f);
		data[3].color    = glm::vec4(1.f);

#ifdef MAPLE_VULKAN

		std::vector<uint32_t> indices = {
		    0,
		    3,
		    2,
		    2,
		    1,
		    0,
		};
#else
		std::vector<uint32_t> indices = {
		    0,
		    1,
		    2,
		    2,
		    3,
		    0,
		};
#endif        // MAPLE_VULKAN
		Mesh::generateTangents(data, indices);

		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createCube() -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data;
		data.resize(24);

		data[0].pos    = glm::vec3(1.0f, 1.0f, 1.0f);
		data[0].color  = glm::vec4(1.0f);
		data[0].normal = glm::vec3(0.0f, 0.0f, 1.0f);

		data[1].pos    = glm::vec3(-1.0f, 1.0f, 1.0f);
		data[1].color  = glm::vec4(1.0f);
		data[1].normal = glm::vec3(0.0f, 0.0f, 1.0f);

		data[2].pos    = glm::vec3(-1.0f, -1.0f, 1.0f);
		data[2].color  = glm::vec4(1.0f);
		data[2].normal = glm::vec3(0.0f, 0.0f, 1.0f);

		data[3].pos    = glm::vec3(1.0f, -1.0f, 1.0f);
		data[3].color  = glm::vec4(1.0f);
		data[3].normal = glm::vec3(0.0f, 0.0f, 1.0f);

		data[4].pos    = glm::vec3(1.0f, 1.0f, 1.0f);
		data[4].color  = glm::vec4(1.0f);
		data[4].normal = glm::vec3(1.0f, 0.0f, 0.0f);

		data[5].pos    = glm::vec3(1.0f, -1.0f, 1.0f);
		data[5].color  = glm::vec4(1.0f);
		data[5].normal = glm::vec3(1.0f, 0.0f, 0.0f);

		data[6].pos    = glm::vec3(1.0f, -1.0f, -1.0f);
		data[6].color  = glm::vec4(1.0f);
		data[6].normal = glm::vec3(1.0f, 0.0f, 0.0f);

		data[7].pos      = glm::vec3(1.0f, 1.0f, -1.0f);
		data[7].color    = glm::vec4(1.0f);
		data[7].texCoord = glm::vec2(0.0f, 1.0f);
		data[7].normal   = glm::vec3(1.0f, 0.0f, 0.0f);

		data[8].pos    = glm::vec3(1.0f, 1.0f, 1.0f);
		data[8].color  = glm::vec4(1.0f);
		data[8].normal = glm::vec3(0.0f, 1.0f, 0.0f);

		data[9].pos    = glm::vec3(1.0f, 1.0f, -1.0f);
		data[9].color  = glm::vec4(1.0f);
		data[9].normal = glm::vec3(0.0f, 1.0f, 0.0f);

		data[10].pos      = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[10].color    = glm::vec4(1.0f);
		data[10].texCoord = glm::vec2(0.0f, 1.0f);
		data[10].normal   = glm::vec3(0.0f, 1.0f, 0.0f);

		data[11].pos    = glm::vec3(-1.0f, 1.0f, 1.0f);
		data[11].color  = glm::vec4(1.0f);
		data[11].normal = glm::vec3(0.0f, 1.0f, 0.0f);

		data[12].pos    = glm::vec3(-1.0f, 1.0f, 1.0f);
		data[12].color  = glm::vec4(1.0f);
		data[12].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

		data[13].pos    = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[13].color  = glm::vec4(1.0f);
		data[13].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

		data[14].pos    = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[14].color  = glm::vec4(1.0f);
		data[14].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

		data[15].pos    = glm::vec3(-1.0f, -1.0f, 1.0f);
		data[15].color  = glm::vec4(1.0f);
		data[15].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

		data[16].pos    = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[16].color  = glm::vec4(1.0f);
		data[16].normal = glm::vec3(0.0f, -1.0f, 0.0f);

		data[17].pos    = glm::vec3(1.0f, -1.0f, -1.0f);
		data[17].color  = glm::vec4(1.0f);
		data[17].normal = glm::vec3(0.0f, -1.0f, 0.0f);

		data[18].pos    = glm::vec3(1.0f, -1.0f, 1.0f);
		data[18].color  = glm::vec4(1.0f);
		data[18].normal = glm::vec3(0.0f, -1.0f, 0.0f);

		data[19].pos    = glm::vec3(-1.0f, -1.0f, 1.0f);
		data[19].color  = glm::vec4(1.0f);
		data[19].normal = glm::vec3(0.0f, -1.0f, 0.0f);

		data[20].pos    = glm::vec3(1.0f, -1.0f, -1.0f);
		data[20].color  = glm::vec4(1.0f);
		data[20].normal = glm::vec3(0.0f, 0.0f, -1.0f);

		data[21].pos    = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[21].color  = glm::vec4(1.0f);
		data[21].normal = glm::vec3(0.0f, 0.0f, -1.0f);

		data[22].pos    = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[22].color  = glm::vec4(1.0f);
		data[22].normal = glm::vec3(0.0f, 0.0f, -1.0f);

		data[23].pos    = glm::vec3(1.0f, 1.0f, -1.0f);
		data[23].color  = glm::vec4(1.0f);
		data[23].normal = glm::vec3(0.0f, 0.0f, -1.0f);

		for (int i = 0; i < 6; i++)
		{
			data[i * 4 + 0].texCoord = glm::vec2(0.0f, 0.0f);
			data[i * 4 + 1].texCoord = glm::vec2(1.0f, 0.0f);
			data[i * 4 + 2].texCoord = glm::vec2(1.0f, 1.0f);
			data[i * 4 + 3].texCoord = glm::vec2(0.0f, 1.0f);
		}

		std::vector<uint32_t> indices{
		    0, 1, 2,
		    0, 2, 3,
		    4, 5, 6,
		    4, 6, 7,
		    8, 9, 10,
		    8, 10, 11,
		    12, 13, 14,
		    12, 14, 15,
		    16, 17, 18,
		    16, 18, 19,
		    20, 21, 22,
		    20, 22, 23};

		Mesh::generateTangents(data, indices);

		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createPyramid() -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data;
		data.resize(18);

		data[0].pos      = glm::vec3(1.0f, 1.0f, -1.0f);
		data[0].color    = glm::vec4(1.0f);
		data[0].texCoord = glm::vec2(0.24f, 0.20f);
		data[0].normal   = glm::vec3(0.0f, 0.8948f, 0.4464f);

		data[1].pos      = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[1].color    = glm::vec4(1.0f);
		data[1].texCoord = glm::vec2(0.24f, 0.81f);
		data[1].normal   = glm::vec3(0.0f, 0.8948f, 0.4464f);

		data[2].pos      = glm::vec3(0.0f, 0.0f, 1.0f);
		data[2].color    = glm::vec4(1.0f);
		data[2].texCoord = glm::vec2(0.95f, 0.50f);
		data[2].normal   = glm::vec3(0.0f, 0.8948f, 0.4464f);

		data[3].pos      = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[3].color    = glm::vec4(1.0f);
		data[3].texCoord = glm::vec2(0.24f, 0.21f);
		data[3].normal   = glm::vec3(-0.8948f, 0.0f, 0.4464f);

		data[4].pos      = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[4].color    = glm::vec4(1.0f);
		data[4].texCoord = glm::vec2(0.24f, 0.81f);
		data[4].normal   = glm::vec3(-0.8948f, 0.0f, 0.4464f);

		data[5].pos      = glm::vec3(0.0f, 0.0f, 1.0f);
		data[5].color    = glm::vec4(1.0f);
		data[5].texCoord = glm::vec2(0.95f, 0.50f);
		data[5].normal   = glm::vec3(-0.8948f, 0.0f, 0.4464f);

		data[6].pos      = glm::vec3(1.0f, 1.0f, -1.0f);
		data[6].color    = glm::vec4(1.0f);
		data[6].texCoord = glm::vec2(0.24f, 0.81f);
		data[6].normal   = glm::vec3(0.8948f, 0.0f, 0.4475f);

		data[7].pos      = glm::vec3(0.0f, 0.0f, 1.0f);
		data[7].color    = glm::vec4(1.0f);
		data[7].texCoord = glm::vec2(0.95f, 0.50f);
		data[7].normal   = glm::vec3(0.8948f, 0.0f, 0.4475f);

		data[8].pos      = glm::vec3(1.0f, -1.0f, -1.0f);
		data[8].color    = glm::vec4(1.0f);
		data[8].texCoord = glm::vec2(0.24f, 0.21f);
		data[8].normal   = glm::vec3(0.8948f, 0.0f, 0.4475f);

		data[9].pos      = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[9].color    = glm::vec4(1.0f);
		data[9].texCoord = glm::vec2(0.24f, 0.21f);
		data[9].normal   = glm::vec3(0.0f, -0.8948f, 0.448f);

		data[10].pos      = glm::vec3(1.0f, -1.0f, -1.0f);
		data[10].color    = glm::vec4(1.0f);
		data[10].texCoord = glm::vec2(0.24f, 0.81f);
		data[10].normal   = glm::vec3(0.0f, -0.8948f, 0.448f);

		data[11].pos      = glm::vec3(0.0f, 0.0f, 1.0f);
		data[11].color    = glm::vec4(1.0f);
		data[11].texCoord = glm::vec2(0.95f, 0.50f);
		data[11].normal   = glm::vec3(0.0f, -0.8948f, 0.448f);

		data[12].pos      = glm::vec3(-1.0f, 1.0f, -1.0f);
		data[12].color    = glm::vec4(1.0f);
		data[12].texCoord = glm::vec2(0.0f, 0.0f);
		data[12].normal   = glm::vec3(0.0f, 0.0f, -1.0f);

		data[13].pos      = glm::vec3(1.0f, 1.0f, -1.0f);
		data[13].color    = glm::vec4(1.0f);
		data[13].texCoord = glm::vec2(0.0f, 1.0f);
		data[13].normal   = glm::vec3(0.0f, 0.0f, -1.0f);

		data[14].pos      = glm::vec3(1.0f, -1.0f, -1.0f);
		data[14].color    = glm::vec4(1.0f);
		data[14].texCoord = glm::vec2(1.0f, 1.0f);
		data[14].normal   = glm::vec3(0.0f, 0.0f, -1.0f);

		data[15].pos      = glm::vec3(-1.0f, -1.0f, -1.0f);
		data[15].color    = glm::vec4(1.0f);
		data[15].texCoord = glm::vec2(0.96f, 0.0f);
		data[15].normal   = glm::vec3(0.0f, 0.0f, -1.0f);

		data[16].pos      = glm::vec3(0.0f, 0.0f, 0.0f);
		data[16].color    = glm::vec4(1.0f);
		data[16].texCoord = glm::vec2(0.0f, 0.0f);
		data[16].normal   = glm::vec3(0.0f, 0.0f, 0.0f);

		data[17].pos      = glm::vec3(0.0f, 0.0f, 0.0f);
		data[17].color    = glm::vec4(1.0f);
		data[17].texCoord = glm::vec2(0.0f, 0.0f);
		data[17].normal   = glm::vec3(0.0f, 0.0f, 0.0f);

		const std::vector<uint32_t> indices{
		    0, 1, 2,
		    3, 4, 5,
		    6, 7, 8,
		    9, 10, 11,
		    12, 13, 14,
		    15, 12, 14};
		Mesh::generateTangents(data, indices);
		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createOctahedron() -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data;
		data.resize(6);

		std::vector<glm::vec3> octVertices{
		    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

		for (auto i = 0; i < 6; i++)
		{
			data[i].pos      = octVertices[i];
			data[i].color    = glm::vec4(1.0f);
			data[i].texCoord = glm::vec2(0.0f, 0.0f);
			data[i].normal   = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		const std::vector<uint32_t> indices = {
		    0, 2, 4, 0, 4, 3, 0, 3, 5,
		    0, 5, 2, 1, 2, 5, 1, 5, 3,
		    1, 3, 4, 1, 4, 2};

		Mesh::generateTangents(data, indices);
		Mesh::generateNormals(data, indices);
		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createCapsule(float radius /*= 0.5f*/, float midHeight /*= 2.0f*/, int32_t radialSegments /*= 64*/, int32_t rings /*= 8*/) -> std::shared_ptr<Mesh>
	{
		PROFILE_FUNCTION();
		int32_t prevRow = 0;
		int32_t thisRow = 0;
		int32_t point   = 0;

		float           x, y, z, u, v, w;
		constexpr float oneThird  = 1.0f / 3.0f;
		constexpr float twoThirds = 2.0f / 3.0f;

		std::vector<Vertex>   data;
		std::vector<uint32_t> indices;

		/* top hemisphere */
		for (auto j = 0; j <= (rings + 1); j++)
		{
			v = float(j);

			v /= (rings + 1);
			w = std::sin(0.5f * M_PI * v);
			y = radius * std::cos(0.5f * M_PI * v);

			for (auto i = 0; i <= radialSegments; i++)
			{
				u = float(i);
				u /= radialSegments;

				x = std::sin(u * (M_PI * 2.0f));
				z = -std::cos(u * (M_PI * 2.0f));

				glm::vec3 p = glm::vec3(x * radius * w, y, z * radius * w);

				Vertex vertex;
				vertex.pos      = p + glm::vec3(0.0f, 0.5f * midHeight, 0.0f);
				vertex.normal   = glm::normalize((p + glm::vec3(0.0f, 0.5f * midHeight, 0.0f)));
				vertex.texCoord = glm::vec2(u, oneThird * v);
				vertex.color    = glm::vec4(1, 1, 1, 1.f);
				data.emplace_back(vertex);
				point++;

				if (i > 0 && j > 0)
				{
					indices.push_back(prevRow + i - 1);
					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i - 1);

					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i);
					indices.push_back(thisRow + i - 1);
				};
			};

			prevRow = thisRow;
			thisRow = point;
		};

		/* cylinder */
		thisRow = point;
		prevRow = 0;
		for (auto j = 0; j <= (rings + 1); j++)
		{
			v = float(j);
			v /= (rings + 1);

			y = midHeight * v;
			y = (midHeight * 0.5f) - y;

			for (auto i = 0; i <= radialSegments; i++)
			{
				u = float(i);
				u /= radialSegments;

				x = std::sin(u * (M_PI * 2.0f));
				z = -std::cos(u * (M_PI * 2.0f));

				glm::vec3 p = glm::vec3(x * radius, y, z * radius);

				Vertex vertex;
				vertex.pos      = p;
				vertex.normal   = glm::vec3(x, z, 0.0f);
				vertex.texCoord = glm::vec2(u, oneThird + (v * oneThird));
				vertex.color    = glm::vec4(1, 1, 1, 1.f);
				data.emplace_back(vertex);

				point++;

				if (i > 0 && j > 0)
				{
					indices.push_back(prevRow + i - 1);
					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i - 1);

					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i);
					indices.push_back(thisRow + i - 1);
				};
			};

			prevRow = thisRow;
			thisRow = point;
		};

		/* bottom hemisphere */
		thisRow = point;
		prevRow = 0;

		for (auto j = 0; j <= (rings + 1); j++)
		{
			v = float(j);

			v /= (rings + 1);
			v += 1.0f;
			w = std::sin(0.5f * M_PI * v);
			y = radius * std::cos(0.5f * M_PI * v);

			for (auto i = 0; i <= radialSegments; i++)
			{
				float u2 = float(i);
				u2 /= radialSegments;

				x = std::sin(u2 * (M_PI * 2.0f));
				z = -std::cos(u2 * (M_PI * 2.0f));

				glm::vec3 p = glm::vec3(x * radius * w, y, z * radius * w);

				Vertex vertex;
				vertex.pos      = p + glm::vec3(0.0f, -0.5f * midHeight, 0.0f);
				vertex.normal   = glm::normalize((p + glm::vec3(0.0f, -0.5f * midHeight, 0.0f)));
				vertex.texCoord = glm::vec2(u2, twoThirds + ((v - 1.0f) * oneThird));
				vertex.color    = glm::vec4(1, 1, 1, 1.f);
				data.emplace_back(vertex);

				point++;

				if (i > 0 && j > 0)
				{
					indices.push_back(prevRow + i - 1);
					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i - 1);

					indices.push_back(prevRow + i);
					indices.push_back(thisRow + i);
					indices.push_back(thisRow + i - 1);
				};
			};

			prevRow = thisRow;
			thisRow = point;
		}

		Mesh::generateNormals(data, indices);
		Mesh::generateTangents(data, indices);
		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createSphere(uint32_t xSegments /*= 64*/, uint32_t ySegments /*= 64*/) -> std::shared_ptr<Mesh>
	{
		std::vector<Vertex> data;
		float               sectorCount = static_cast<float>(xSegments);
		float               stackCount  = static_cast<float>(ySegments);
		float               sectorStep  = 2 * M_PI / sectorCount;
		float               stackStep   = M_PI / stackCount;
		float               radius      = 1.0f;

		for (int i = 0; i <= stackCount; ++i)
		{
			float stackAngle = M_PI / 2 - i * stackStep;
			float xy         = radius * cos(stackAngle);
			float z          = radius * sin(stackAngle);

			for (int32_t j = 0; j <= sectorCount; ++j)
			{
				float sectorAngle = j * sectorStep;
				float x           = xy * cosf(sectorAngle);
				float y           = xy * sinf(sectorAngle);

				float s = static_cast<float>(j / sectorCount);
				float t = static_cast<float>(i / stackCount);

				Vertex &vertex  = data.emplace_back();
				vertex.pos      = glm::vec3(x, y, z);
				vertex.texCoord = glm::vec2(s, t);
				vertex.normal   = glm::normalize(glm::vec3(x, y, z));
				vertex.color    = glm::vec4(1.f);
			}
		}

		std::vector<uint32_t> indices;
		uint32_t              k1, k2;
		for (uint32_t i = 0; i < stackCount; ++i)
		{
			k1 = i * (static_cast<uint32_t>(sectorCount) + 1U);
			k2 = k1 + static_cast<uint32_t>(sectorCount) + 1U;

			for (uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}

				if (i != (stackCount - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}
		Mesh::generateTangents(data, indices);

		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::createPlane(float width, float height, const glm::vec3 &normal) -> std::shared_ptr<Mesh>
	{
		glm::vec3 vec = glm::radians(normal * 90.0f);
		glm::quat rotation =
		    glm::angleAxis(vec.z, glm::vec3(1.0f, 0.0f, 0.0f)) *
		    glm::angleAxis(vec.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
		    glm::angleAxis(vec.x, glm::vec3(0.0f, 0.0f, 1.0f));

		std::vector<Vertex> data;
		data.resize(4);
		data[0].pos      = rotation * glm::vec3(-width / 2.0f, -1.0f, -height / 2.0f);
		data[0].normal   = normal;
		data[0].texCoord = glm::vec2(0.0f, 0.0f);
		data[0].color    = glm::vec4(1.f);

		data[1].pos      = rotation * glm::vec3(-width / 2.0f, -1.0f, height / 2.0f);
		data[1].normal   = normal;
		data[1].texCoord = glm::vec2(0.0f, 1.0f);
		data[1].color    = glm::vec4(1.f);

		data[2].pos      = rotation * glm::vec3(width / 2.0f, 1.0f, height / 2.0f);
		data[2].normal   = normal;
		data[2].texCoord = glm::vec2(1.0f, 1.0f);
		data[2].color    = glm::vec4(1.f);

		data[3].pos      = rotation * glm::vec3(width / 2.0f, 1.0f, -height / 2.0f);
		data[3].normal   = normal;
		data[3].texCoord = glm::vec2(1.0f, 0.0f);
		data[3].color    = glm::vec4(1.f);

		std::vector<uint32_t> indices{
		    0, 1, 2,
		    2, 3, 0};

		Mesh::generateTangents(data, indices);

		return std::make_shared<Mesh>(indices, data);
	}

	auto Mesh::getSubMeshesBuffer() -> std::shared_ptr<StorageBuffer>
	{
		if (subMeshesBuffer == nullptr)
		{
			subMeshesBuffer = StorageBuffer::create(sizeof(glm::uvec2) * subMeshCount, nullptr, {false,  MemoryUsage::MEMORY_USAGE_CPU_TO_GPU});
		}
		return subMeshesBuffer;
	}

	auto Mesh::getAccelerationStructure(BatchTask::Ptr task) -> AccelerationStructure::Ptr
	{
		if (bottomAs == nullptr)
			bottomAs = AccelerationStructure::createBottomLevel(vertexBuffer, indexBuffer, vertexCount, task);
		return bottomAs;
	}

	auto Mesh::generateTangent(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec2 &ta, const glm::vec2 &tb, const glm::vec2 &tc) -> glm::vec3
	{
		const glm::vec2 coord1  = tb - ta;
		const glm::vec2 coord2  = tc - ta;
		const glm::vec3 vertex1 = b - a;
		const glm::vec3 vertex2 = c - a;
		const glm::vec3 axis    = (vertex1 * coord2.y - vertex2 * coord1.y);
		const float     factor  = 1.0f / (coord1.x * coord2.y - coord2.x * coord1.y);
		return (vertex1 * coord2.y - vertex1 * coord1.y) * factor;
	}
};        // namespace maple
