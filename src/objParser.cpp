#include "objParser.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <functional>


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
			size_t seed = 0;
			seed ^= std::hash<uint32_t>{}(k.posIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint32_t>{}(k.uvIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint32_t>{}(k.normalIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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

	vec3 parseVec3(const char *start, const char *err = "Invalid value")
	{
		char *newpos = NULL;
		vec3 v;

		v.x = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err));
		start = newpos;

		v.y = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err));
		start = newpos;

		v.z = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err));

		return v;
	}

	vec2 parseVec2(const char *start, const char *err = "Invalid value")
	{
		char *newpos = NULL;
		vec2 v;

		v.x = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err));
		start = newpos;

		v.y = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err));

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
	bool parseFaceVertex(VertexKey &v, const char *&str)
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

	void parseObj(Asset &asset, const std::string &path)
	{
		std::ifstream in(path, std::ios_base::in);

		if (!in.is_open()) throw std::ios_base::failure("Cannot open file: " + path);

		std::string line;
		size_t line_count = 0;

		std::vector<vec3> pos;
		std::vector<vec3> normal;
		std::vector<vec2> uv;

		uint32_t currentMeshID = -1;
		uint32_t currentSubMeshID = -1;
		std::string currentMaterial = "default";

		std::unordered_map<VertexKey, uint32_t> vertexMap;

		try
		{
			while (getline(in, line))
			{
				line_count++;

				if (line.starts_with("v "))
				{
					pos.push_back(parseVec3(line.c_str() + 2, "Malformated vertex"));
				}
				else if (line.starts_with("vn "))
				{
					normal.push_back(parseVec3(line.c_str() + 3, "Malformated normal direction"));
				}
				else if (line.starts_with("vt "))
				{
					uv.push_back(parseVec2(line.c_str() + 3, "Malformated uv"));
				}
				else if (line.starts_with("o "))
				{
					asset.meshes_.push_back(std::make_unique<Mesh>(line.substr(2)));
					currentMeshID++;
					currentSubMeshID = -1;
				}
				else if (line.starts_with("usemtl "))
				{
					currentMaterial = line.substr(8);

					if (currentMeshID < 0) continue;

					asset.meshes_[currentMeshID]->subMeshes_.push_back(std::make_unique<SubMesh>(currentMaterial));
					currentSubMeshID++;
				}
				else if (line.starts_with("f "))
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
					const char *str = line.c_str() + 2;

					VertexKey tmp;
					while (parseFaceVertex(tmp, str))
					{
						faceVertex.push_back(tmp);
						tmp = {0, 0, 0};
					}

					std::vector<uint32_t> faceVertexIndexes;
					faceVertexIndexes.reserve(faceVertex.size());

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

								if (vertexKeyIt->normalIndex == 0) finalVertex.normal = vec3(0); // TODO: Calculer selon la face
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
				}
			}

		}
		catch (const std::exception& err)
		{
			throw ObjParseError(path + ":" + std::to_string(line_count) + ": " + err.what());
		}
	}

}
