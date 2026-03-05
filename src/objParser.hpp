#pragma once

#include "scene-core.hpp"
#include "../src/tdr/error.hpp"
#include <istream>
#include <vector>
#include <string>

namespace sceneIO::parser {

	using ObjSourceLocation = sceneIO::tdr::SourceLocation;

	struct ObjError
	{
		ObjSourceLocation location;
		std::string message;

		std::string getError() const { return location.format() + " " + message; }
	};

	class ObjErrorCollector
	{
	public:
		void report(ObjSourceLocation loc, const std::string& msg)
		{
			errors_.push_back({loc, msg});
		}

		void report(const std::string& msg)
		{
			errors_.push_back({{{}, UINT64_MAX, UINT64_MAX}, msg});
		}

		void setFilePath(const std::string& path)
		{
			for (ObjError& e : errors_)
				e.location.filepath = path;
		}

		bool        hasErrors()  const { return !errors_.empty(); }
		const std::vector<ObjError>& getErrors() const { return errors_; }

	private:
		std::vector<ObjError> errors_;
	};

	void parseObj(Asset& asset, std::istream& in, ObjErrorCollector& errors,
	              uint64_t startLine = 1, uint64_t startColumn = 1);

	void parseObj(Asset& asset, const std::string& path, ObjErrorCollector& errors);
	Asset parseObj(const std::string& path, ObjErrorCollector& errors);

}