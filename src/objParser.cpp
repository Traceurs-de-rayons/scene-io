#include "objParser.hpp"
#include <fstream>
#include <iostream>

namespace sceneIO::parser {

	Asset parseObj(const std::string& path) {
		Asset res;
		parseObj(res, path);
		return res;
	}

	vec3 parseVec3(const char *start, size_t line, const char* err = "Invalid value")
	{
		char *newpos = NULL;
		vec3 v;

		v.x = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err) + " line " + std::to_string(line));
		start = newpos;
		v.y = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err) + " line " + std::to_string(line));
		start = newpos;
		v.z = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err) + " line " + std::to_string(line));

		return v;
	}

	vec2 parseVec2(const char *start, size_t line, const char* err = "Invalid value")
	{
		char *newpos = NULL;
		vec2 v;

		v.x = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err) + " line " + std::to_string(line));
		start = newpos;
		v.y = std::strtof(start, &newpos);
		if (start == newpos) throw ObjParseError(std::string(err) + " line " + std::to_string(line));

		return v;
	}

	void parseObj(Asset& asset, const std::string& path)
	{
		std::ifstream in(path, std::ios_base::in);

		if (!in.is_open()) throw std::ios_base::failure("Cannot open file: " + path);

		std::string line;
		size_t line_count = 0;

		std::vector<vec3> pos;
		std::vector<vec3> normal;
		std::vector<vec2> uv;

		while (getline(in, line))
		{
			line_count++;

			if (line.starts_with("v "))
			{
				pos.push_back(parseVec3(line.c_str() + 2, line_count, "Malformated vertex"));
			}
			else if (line.starts_with("vn "))
			{
				normal.push_back(parseVec3(line.c_str() + 3, line_count, "Malformated normal direction"));
			}
			else if (line.starts_with("vt "))
			{
				uv.push_back(parseVec2(line.c_str() + 3, line_count, "Malformated uv"));
			}
		}
	}

}