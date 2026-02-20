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
		: std::runtime_error(loc.format() + " " + msg), msg_(msg), location(loc) {}
	TdrError(uint64_t line, uint64_t column, const std::string& msg)
		: std::runtime_error(msg), msg_(msg), location({.line = line, .column = column}) {}
	TdrError(const std::string& msg)
		: std::runtime_error(msg), msg_(msg), location({}) {}

	SourceLocation location;

	const std::string& getMessage() const { return msg_; }
	const std::string getError() const { return location.format() + " " + msg_; }

};

class ErrorCollector
{

private:
	std::vector<TdrError> errors_;
	
public:
	ErrorCollector() = default;
	~ErrorCollector() = default;

	void report(TdrError error) { errors_.push_back(std::move(error)); }

	void setFilePath(const std::string& path)
	{
		for (TdrError& e : errors_)
		{
			e.location.filepath = path;
		}
	}
	
	bool has_errors() const { return !errors_.empty(); }
	const std::vector<TdrError>& get_errors() const { return errors_; }
};


}