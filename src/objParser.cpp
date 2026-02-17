#include "objParser.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <charconv>

namespace sceneIO::parser
{
	struct VertexKey
	{
		uint32_t posIndex = 0;
		uint32_t uvIndex = 0;
		uint32_t normalIndex = 0;

		bool operator==(const VertexKey& other) const noexcept
		{
			return (posIndex == other.posIndex &&
					uvIndex == other.uvIndex &&
					normalIndex == other.normalIndex);
		}
	};
}

namespace std
{
	template<>
	struct hash<sceneIO::parser::VertexKey>
	{
		size_t operator()(const sceneIO::parser::VertexKey& k) const noexcept
		{
			size_t seed = k.posIndex + 0x9e3779b9;
			seed ^= k.uvIndex + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= k.normalIndex + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}

namespace sceneIO::parser
{

	Asset parseObj(const std::string &path)
	{
		Asset res;
		parseObj(res, path);
		return res;
	}

	static inline vec3 parseVec3(const char *ptr, const char *err = "Invalid value")
	{
		vec3 v;
		const char* end = ptr + std::strlen(ptr);

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r1 = std::from_chars(ptr, end, v.x);
		if (r1.ec != std::errc()) throw ObjParseError(std::string(err));
		ptr = r1.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r2 = std::from_chars(ptr, end, v.y);
		if (r2.ec != std::errc()) throw ObjParseError(std::string(err));
		ptr = r2.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r3 = std::from_chars(ptr, end, v.z);
		if (r3.ec != std::errc()) throw ObjParseError(std::string(err));

		return v;
	}

	static inline vec2 parseVec2(const char *ptr, const char *err = "Invalid value")
	{
		vec2 v;
		const char* end = ptr + std::strlen(ptr);

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r1 = std::from_chars(ptr, end, v.x);
		if (r1.ec != std::errc()) throw ObjParseError(std::string(err));
		ptr = r1.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r2 = std::from_chars(ptr, end, v.y);
		if (r2.ec != std::errc()) throw ObjParseError(std::string(err));
		
		return v;
	}

	static inline void micro_atoi(uint32_t &res, const char *&str, const char *type)
	{
	#if defined(__GNUC__) || defined(__clang__)
		while (*str >= '0' && *str <= '9')
		{
			if (__builtin_mul_overflow(res, 10u, &res) ||
				__builtin_add_overflow(res, *str - '0', &res))
				throw ObjParseError(std::string(type) + " index too large");
			str++;
		}
	#else
		uint32_t overflow_check = 0;

		while (*str >= '0' && *str <= '9')
		{
			res = res * 10 + *str - '0';
			str++;
			if (overflow_check > res) throw ObjParseError(std::string(type) + " index too large");
			overflow_check = res;
		}
	#endif
	}

	/**
	 * @return true if valid, false if end of the line
	 */
	static inline bool parseFaceVertex(VertexKey &v, const char *&str)
	{
		while (std::isspace(*str)) str++;

		if (!isdigit(*str)) return false;

		micro_atoi(v.posIndex, str, "vertex");

		if (*str == '/') str++;
		else if (std::isspace(*str) || !*str) return true;
		else throw ObjParseError("Maformated face");

		micro_atoi(v.uvIndex, str, "uv");

		if (*str == '/') str++;
		else if (std::isspace(*str) || !*str) return true;
		else throw ObjParseError("Maformated face");

		micro_atoi(v.normalIndex, str, "normal");

		return true;
	}

	static inline vec2 project(const vec3& v, const vec3& faceNormal)
	{
		float ax = std::abs(faceNormal.x);
		float ay = std::abs(faceNormal.y);
		float az = std::abs(faceNormal.z);

		if (az >= ax && az >= ay)
			return { v.x, v.y };
		if (ay >= ax && ay >= az)
			return { v.x, v.z };
		return { v.y, v.z };
	}

	static inline float cross2D(vec2 a, vec2 b, vec2 c)
	{
		return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	}

	static inline bool pointInTriangle2D(vec2 p, vec2 a, vec2 b, vec2 c)
	{
		float areaABC = std::abs(cross2D(a, b, c));
		
		float areaPBC = std::abs(cross2D(p, b, c));
		float areaPCA = std::abs(cross2D(p, c, a));
		float areaPAB = std::abs(cross2D(p, a, b));
		
		float epsilon = 1e-6f;
		return std::abs(areaABC - (areaPBC + areaPCA + areaPAB)) < epsilon
			   && areaPBC > epsilon && areaPCA > epsilon && areaPAB > epsilon;
	}

	static inline bool isEar(const std::vector<Vertex>& meshVertices, uint32_t iPrev, uint32_t iCurr, uint32_t iNext, const vec3& faceNormal, std::vector<uint32_t>& polygon)
	{
		vec2 prev = project(meshVertices[iPrev].pos, faceNormal);
		vec2 curr = project(meshVertices[iCurr].pos, faceNormal);
		vec2 next = project(meshVertices[iNext].pos, faceNormal);

		float area = cross2D(prev, curr, next);
		if (std::abs(area) < 1e-6f)
			return false;

		for (uint32_t vertex : polygon)
		{
			if (vertex == iPrev || vertex == iCurr || vertex == iNext)
				continue;

			vec2 p = project(meshVertices[vertex].pos, faceNormal);
			if (pointInTriangle2D(p, prev, curr, next))
				return false;
		}

		return true;
	}

	static inline void earClipping(const std::vector<Vertex>& meshVertices, std::vector<uint32_t>& polygon, std::vector<uint32_t>& result, vec3& faceNormal)
	{		
		while (polygon.size() > 3)
		{
			bool earFound = false;
			
			for (auto it = polygon.begin(); it != polygon.end(); ++it)
			{
				auto prev = (it == polygon.begin()) ? std::prev(polygon.end()) : std::prev(it);
				auto next = std::next(it);
				if (next == polygon.end()) next = polygon.begin();
				
				if (isEar(meshVertices, *prev, *it, *next, faceNormal, polygon))
				{
					result.push_back(*prev);
					result.push_back(*it);
					result.push_back(*next);
					
					polygon.erase(it);
					earFound = true;
					break;
				}
			}
			
			if (!earFound) throw ObjParseError("The face is a degenerated polygon. Is this face counter clock wise ?");
		}
		
		auto it = polygon.begin();
		result.push_back(*it++);
		result.push_back(*it++);
		result.push_back(*it);
	}

	void parseObj(Asset &asset, const std::string &path)
	{
		std::ifstream in(path, std::ios_base::in);

		if (!in.is_open()) throw std::ios_base::failure("Cannot open file: " + path);

		char line[512];
		size_t line_count = 0;
		asset.type_ = AssetObject;

		std::vector<vec3> pos;    pos.reserve(1024);
		std::vector<vec3> normal; normal.reserve(1024);
		std::vector<vec2> uv;     uv.reserve(1024);

		uint32_t currentMeshID = -1;
		uint32_t currentSubMeshID = -1;
		std::string currentMaterial = "default";

		std::unordered_map<VertexKey, uint32_t> vertexMap;

		try
		{
			while (in.getline(line, sizeof(line)))
			{
				line_count++;

				if (std::strncmp(line, "v ", 2) == 0)
				{
					pos.push_back(parseVec3(line + 2, "Malformated vertex"));
				}
				else if (std::strncmp(line, "vn ", 3) == 0)
				{
					normal.push_back(parseVec3(line + 3, "Malformated normal direction"));
				}
				else if (std::strncmp(line, "vt ", 3) == 0)
				{
					uv.push_back(parseVec2(line + 3, "Malformated uv"));
				}
				else if (std::strncmp(line, "o ", 2) == 0)
				{
					asset.meshes_.push_back(std::make_unique<Mesh>(std::string(line + 2)));
					currentMeshID++;
					currentSubMeshID = -1;
					vertexMap.clear();
				}
				else if (std::strncmp(line, "usemtl ", 7) == 0)
				{
					currentMaterial = std::string(line + 7);

					if (currentMeshID < 0) continue;

					asset.meshes_[currentMeshID]->subMeshes_.push_back(std::make_unique<SubMesh>(currentMaterial));
					currentSubMeshID++;
				}
				else if (std::strncmp(line, "f ", 2) == 0)
				{
					if (currentMeshID < 0)
					{
						asset.meshes_.push_back(std::make_unique<Mesh>("Default"));
						currentMeshID++;
						currentSubMeshID = -1;
					}
					if (currentSubMeshID < 0)
					{
						asset.meshes_[currentMeshID]->subMeshes_.push_back(std::make_unique<SubMesh>(currentMaterial));
						currentSubMeshID++;
					}

					std::vector<VertexKey> faceVertex;
					const char *str = line + 2;

					VertexKey tmp;
					while (parseFaceVertex(tmp, str))
					{
						faceVertex.push_back(tmp);
						tmp = {0, 0, 0};
					}

					if (faceVertex.size() < 3) throw ObjParseError("Invalid vertex count on the face");

					std::vector<uint32_t> faceVertexIndexes;
					faceVertexIndexes.reserve(faceVertex.size());

					bool faceMissingNormal = false;

					for (auto vertexKeyIt = faceVertex.begin(); vertexKeyIt != faceVertex.end(); vertexKeyIt++)
					{
						auto vert = vertexMap.find(*vertexKeyIt);
						uint32_t realIndex;

						if (vert == vertexMap.end())
						{
							realIndex = asset.meshes_[currentMeshID]->vertices_.size();
							Vertex finalVertex;
							try
							{
								finalVertex.pos = pos[vertexKeyIt->posIndex - 1];

								if (vertexKeyIt->uvIndex == 0) finalVertex.uv = vec2(0);
								else finalVertex.uv = uv[vertexKeyIt->uvIndex - 1];

								if (vertexKeyIt->normalIndex == 0)
								{
									faceMissingNormal = true;
									finalVertex.normal = vec3(0);
								}
								else finalVertex.normal = normal[vertexKeyIt->normalIndex - 1];
							}
							catch(const std::exception& e)
							{
								throw ObjParseError("Invalid index it the face");
							}
							
							asset.meshes_[currentMeshID]->vertices_.push_back(finalVertex);
							vertexMap[*vertexKeyIt] = realIndex;
						}
						else realIndex = vert->second;
						
						faceVertexIndexes.push_back(realIndex);
					}

					vec3 faceNormal = vec3::cross(asset.meshes_[currentMeshID]->vertices_[faceVertexIndexes[1]].pos - asset.meshes_[currentMeshID]->vertices_[faceVertexIndexes[0]].pos,
												  asset.meshes_[currentMeshID]->vertices_[faceVertexIndexes[2]].pos - asset.meshes_[currentMeshID]->vertices_[faceVertexIndexes[0]].pos).normalized();

					if (faceMissingNormal)
					{
						for (auto vertexIt = faceVertexIndexes.begin(); vertexIt != faceVertexIndexes.end(); vertexIt++)
						{
							if (asset.meshes_[currentMeshID]->vertices_[*vertexIt].normal == vec3(0))
								asset.meshes_[currentMeshID]->vertices_[*vertexIt].normal = faceNormal;
						}
					}

					if (vec3::dot(asset.meshes_[currentMeshID]->vertices_[faceVertexIndexes[0]].normal, faceNormal) < 0)
						faceNormal = -faceNormal;

					earClipping(asset.meshes_[currentMeshID]->vertices_, faceVertexIndexes, asset.meshes_[currentMeshID]->subMeshes_[currentSubMeshID]->indices_, faceNormal);
				}
			}

		}
		catch (const std::exception& err)
		{
			throw ObjParseError(path + ":" + std::to_string(line_count) + ": " + err.what());
		}
	}

}
