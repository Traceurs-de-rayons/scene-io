#pragma once

#include <stdexcept>

namespace sceneIO::tdr {

struct SourceLocation {
	std::string filepath;
	uint64_t line   = UINT64_MAX;
	uint64_t column = UINT64_MAX;

	std::string format() const
	{
		std::string out;
		if (!filepath.empty()) out += filepath + ":";
		if (line != UINT64_MAX)        out += std::to_string(line) + ":";
		if (column != UINT64_MAX)      out += std::to_string(column) + ":";
		return out;
	}
};

class TdrError : public std::runtime_error
{

private:
	const std::string msg_;

public:
	TdrError(SourceLocation loc, const std::string& msg)
		: std::runtime_error(loc.format() + " " + msg), location(loc), msg_(msg) {}
	TdrError(const std::string& msg)
		: std::runtime_error(msg), location({}), msg_(msg) {}

	SourceLocation location;

	const std::string getMessage() { return msg_; }

};

}