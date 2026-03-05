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

	static inline bool parseVec3(vec3& out, const char *ptr,
	                              ObjErrorCollector& errors, ObjSourceLocation loc,
	                              const char *err = "Invalid value")
	{
		const char* end = ptr + std::strlen(ptr);

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r1 = std::from_chars(ptr, end, out.x);
		if (r1.ec != std::errc()) { errors.report(loc, err); return false; }
		ptr = r1.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r2 = std::from_chars(ptr, end, out.y);
		if (r2.ec != std::errc()) { errors.report(loc, err); return false; }
		ptr = r2.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r3 = std::from_chars(ptr, end, out.z);
		if (r3.ec != std::errc()) { errors.report(loc, err); return false; }

		return true;
	}

	static inline bool parseVec2(vec2& out, const char *ptr,
	                              ObjErrorCollector& errors, ObjSourceLocation loc,
	                              const char *err = "Invalid value")
	{
		const char* end = ptr + std::strlen(ptr);

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r1 = std::from_chars(ptr, end, out.x);
		if (r1.ec != std::errc()) { errors.report(loc, err); return false; }
		ptr = r1.ptr;

		while (ptr < end && std::isspace(*ptr)) ptr++;

		auto r2 = std::from_chars(ptr, end, out.y);
		if (r2.ec != std::errc()) { errors.report(loc, err); return false; }

		return true;
	}

	static inline bool micro_atoi(uint32_t &res, const char *&str, const char *type,
	                               ObjErrorCollector& errors, ObjSourceLocation loc)
	{
	#if defined(__GNUC__) || defined(__clang__)
		while (*str >= '0' && *str <= '9')
		{
			if (__builtin_mul_overflow(res, 10u, &res) ||
				__builtin_add_overflow(res, *str - '0', &res))
			{
				errors.report(loc, std::string(type) + " index too large");
				return false;
			}
			str++;
		}
	#else
		uint32_t overflow_check = 0;

		while (*str >= '0' && *str <= '9')
		{
			res = res * 10 + *str - '0';
			str++;
			if (overflow_check > res)
			{
				errors.report(loc, std::string(type) + " index too large");
				return false;
			}
			overflow_check = res;
		}
	#endif
		return true;
	}

	/**
	 * @return true if a vertex was parsed, false if end of the line or on error
	 *         (error is reported to @p errors).
	 */
	static inline bool parseFaceVertex(VertexKey &v, const char *&str,
	                                   ObjErrorCollector& errors, ObjSourceLocation loc)
	{
		while (std::isspace(*str)) str++;

		if (!isdigit(*str)) return false;

		if (!micro_atoi(v.posIndex, str, "vertex", errors, loc)) return false;

		if (*str == '/') str++;
		else if (std::isspace(*str) || !*str) return true;
		else { errors.report(loc, "Malformed face"); return false; }

		if (!micro_atoi(v.uvIndex, str, "uv", errors, loc)) return false;

		if (*str == '/') str++;
		else if (std::isspace(*str) || !*str) return true;
		else { errors.report(loc, "Malformed face"); return false; }

		if (!micro_atoi(v.normalIndex, str, "normal", errors, loc)) return false;

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

	/**
	 * @return true on success, false if the polygon is degenerated (error reported).
	 */
	static inline bool earClipping(const std::vector<Vertex>& meshVertices,
	                                std::vector<uint32_t>& polygon,
	                                std::vector<uint32_t>& result,
	                                vec3& faceNormal,
	                                ObjErrorCollector& errors, ObjSourceLocation loc)
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

			if (!earFound)
			{
				errors.report(loc, "The face is a degenerated polygon. Is this face counter clock wise ?");
				return false;
			}
		}

		auto it = polygon.begin();
		result.push_back(*it++);
		result.push_back(*it++);
		result.push_back(*it);
		return true;
	}

	void parseObj(Asset& asset, std::istream& in, ObjErrorCollector& errors,
	              uint64_t startLine, uint64_t startColumn)
	{
		char line[512];
		Asset::ObjectData objAsset;

		uint64_t line_count = startLine - 1;

		std::vector<vec3> pos;    pos.reserve(1024);
		std::vector<vec3> normal; normal.reserve(1024);
		std::vector<vec2> uv;     uv.reserve(1024);

		uint32_t currentMeshID    = static_cast<uint32_t>(-1);
		uint32_t currentSubMeshID = static_cast<uint32_t>(-1);
		std::string currentMaterial = "default";

		std::unordered_map<VertexKey, uint32_t> vertexMap;

		while (in.getline(line, sizeof(line)))
		{
			line_count++;
			uint64_t baseCol = (line_count == startLine) ? startColumn : 1;

			const char* ptr = line;
			while (*ptr != '\0' && std::isspace(static_cast<unsigned char>(*ptr))) ptr++;
			uint64_t col = baseCol + static_cast<uint64_t>(ptr - line);

			if (*ptr == '\0' || *ptr == '#') continue;

			ObjSourceLocation loc{{}, line_count, col};

			if (std::strncmp(ptr, "v ", 2) == 0)
			{
				vec3 v;
				if (parseVec3(v, ptr + 2, errors, loc, "Malformed vertex"))
					pos.push_back(v);
			}
			else if (std::strncmp(ptr, "vn ", 3) == 0)
			{
				vec3 v;
				if (parseVec3(v, ptr + 3, errors, loc, "Malformed normal direction"))
					normal.push_back(v);
			}
			else if (std::strncmp(ptr, "vt ", 3) == 0)
			{
				vec2 v;
				if (parseVec2(v, ptr + 3, errors, loc, "Malformed uv"))
					uv.push_back(v);
			}
			else if (std::strncmp(ptr, "o ", 2) == 0)
			{
				objAsset.meshes.push_back(std::make_unique<Mesh>(std::string(ptr + 2)));
				currentMeshID++;
				currentSubMeshID = static_cast<uint32_t>(-1);
				vertexMap.clear();
			}
			else if (std::strncmp(ptr, "usemtl ", 7) == 0)
			{
				currentMaterial = std::string(ptr + 7);

				if (currentMeshID == static_cast<uint32_t>(-1)) continue;

				objAsset.meshes[currentMeshID]->subMeshes_.push_back(std::make_unique<SubMesh>(currentMaterial));
				currentSubMeshID++;
			}
			else if (std::strncmp(ptr, "f ", 2) == 0)
			{
				if (currentMeshID == static_cast<uint32_t>(-1))
				{
					objAsset.meshes.push_back(std::make_unique<Mesh>("Default"));
					currentMeshID++;
					currentSubMeshID = static_cast<uint32_t>(-1);
				}
				if (currentSubMeshID == static_cast<uint32_t>(-1))
				{
					objAsset.meshes[currentMeshID]->subMeshes_.push_back(std::make_unique<SubMesh>(currentMaterial));
					currentSubMeshID++;
				}

				std::vector<VertexKey> faceVertex;
				const char *str = ptr + 2;

				VertexKey tmp;
				while (parseFaceVertex(tmp, str, errors, loc))
				{
					faceVertex.push_back(tmp);
					tmp = {0, 0, 0};
				}

				if (faceVertex.size() < 3)
				{
					errors.report(loc, "Invalid vertex count on the face");
					continue;
				}

				std::vector<uint32_t> faceVertexIndexes;
				faceVertexIndexes.reserve(faceVertex.size());

				bool faceMissingNormal = false;
				bool indexError = false;

				for (auto vertexKeyIt = faceVertex.begin(); vertexKeyIt != faceVertex.end(); ++vertexKeyIt)
				{
					auto vert = vertexMap.find(*vertexKeyIt);
					uint32_t realIndex;

					if (vert == vertexMap.end())
					{
						realIndex = static_cast<uint32_t>(objAsset.meshes[currentMeshID]->vertices_.size());
						Vertex finalVertex;

						if (vertexKeyIt->posIndex == 0 || vertexKeyIt->posIndex > pos.size())
						{
							errors.report(loc, "Invalid position index in face");
							indexError = true; break;
						}
						finalVertex.pos = pos[vertexKeyIt->posIndex - 1];

						if (vertexKeyIt->uvIndex == 0)
							finalVertex.uv = vec2(0);
						else if (vertexKeyIt->uvIndex > uv.size())
						{
							errors.report(loc, "Invalid UV index in face");
							indexError = true; break;
						}
						else
							finalVertex.uv = uv[vertexKeyIt->uvIndex - 1];

						if (vertexKeyIt->normalIndex == 0)
						{
							faceMissingNormal = true;
							finalVertex.normal = vec3(0);
						}
						else if (vertexKeyIt->normalIndex > normal.size())
						{
							errors.report(loc, "Invalid normal index in face");
							indexError = true; break;
						}
						else
							finalVertex.normal = normal[vertexKeyIt->normalIndex - 1];

						objAsset.meshes[currentMeshID]->vertices_.push_back(finalVertex);
						vertexMap[*vertexKeyIt] = realIndex;
					}
					else realIndex = vert->second;

					faceVertexIndexes.push_back(realIndex);
				}

				if (indexError) continue;

				vec3 faceNormal = vec3::cross(
					objAsset.meshes[currentMeshID]->vertices_[faceVertexIndexes[1]].pos -
					objAsset.meshes[currentMeshID]->vertices_[faceVertexIndexes[0]].pos,
					objAsset.meshes[currentMeshID]->vertices_[faceVertexIndexes[2]].pos -
					objAsset.meshes[currentMeshID]->vertices_[faceVertexIndexes[0]].pos
				).normalized();

				if (faceMissingNormal)
				{
					for (uint32_t idx : faceVertexIndexes)
					{
						if (objAsset.meshes[currentMeshID]->vertices_[idx].normal == vec3(0))
							objAsset.meshes[currentMeshID]->vertices_[idx].normal = faceNormal;
					}
				}

				if (vec3::dot(objAsset.meshes[currentMeshID]->vertices_[faceVertexIndexes[0]].normal, faceNormal) < 0)
					faceNormal = -faceNormal;

				earClipping(objAsset.meshes[currentMeshID]->vertices_, faceVertexIndexes,
				            objAsset.meshes[currentMeshID]->subMeshes_[currentSubMeshID]->indices_,
				            faceNormal, errors, loc);
			}
		}

		asset.content_ = std::move(objAsset);
	}

	void parseObj(Asset& asset, const std::string& path, ObjErrorCollector& errors)
	{
		std::ifstream in(path, std::ios_base::in);
		if (!in.is_open())
		{
			errors.report("Cannot open file: " + path);
			return;
		}

		parseObj(asset, in, errors);
		errors.setFilePath(path);
	}

	Asset parseObj(const std::string& path, ObjErrorCollector& errors)
	{
		Asset res;
		parseObj(res, path, errors);
		return res;
	}

}
