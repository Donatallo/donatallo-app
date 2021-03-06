/*
 * Copyright (C) 2016 Dmitry Marakasov
 *
 * This file is part of donatallo.
 *
 * donatallo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * donatallo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with donatallo.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libdonatallo/database.hh>

#include <iostream>

#include <yaml-cpp/yaml.h>

#include <libdonatallo/detectorchain.hh>

namespace Donatallo {

Database::ParsingException::ParsingException(const std::string& file, int line, int column, const std::string& msg)
	: std::runtime_error(build_what(file, line, column, msg)),
	  file_(file),
	  line_(line),
	  column_(column),
	  msg_(file) {
}

std::string Database::ParsingException::build_what(const std::string& file, int line, int column, const std::string& msg) {
	std::stringstream output;
	output << file;

	if (line >= 0) {
		output << ":" << line + 1;
		if (column >= 0)
			output << ":" << column + 1;
	}

	output << ": " << msg;

	return output.str();
}

Database::Database() {
}

void Database::LoadMeta(const std::string& path, std::vector<std::string>& method_files_to_load, std::vector<std::string>& project_files_to_load) {
	YAML::Node meta = YAML::LoadFile(path);

	if (!meta.IsMap())
		throw YAML::Exception(meta.Mark(), "must be a map");

	{
		const auto& version_node = meta["version"];

		if (!version_node.IsDefined())
			throw YAML::Exception(meta.Mark(), "missing version");

		if (!version_node.IsScalar())
			throw YAML::Exception(meta.Mark(), "version must be a string");

		std::string version = version_node.as<std::string>();

		const int my_major = 1;
		const int my_minor = 0;

		size_t dot1pos = version.find('.');
		if (dot1pos == std::string::npos)
			throw YAML::Exception(version_node.Mark(), "bad version format");
		size_t dot2pos = version.find('.', dot1pos + 1);
		if (dot2pos == std::string::npos)
			throw YAML::Exception(version_node.Mark(), "bad version format");

		int major = std::stoi(version.substr(0, dot1pos));
		int minor = std::stoi(version.substr(dot1pos + 1, dot2pos - dot1pos - 1));

		if (major > my_major)
			throw std::runtime_error("database major version too new, not supported; please update donatallo");
		if (major < my_major)
			throw std::runtime_error("database major version too old, not supported; please update the database");

		if (minor > my_minor)
			std::cerr << "Warning: database minor version is newer than known to an application, which means that some database features may be unsupported" << std::endl;
	}

	{
		const auto projects_node = meta["projects"];

		if (!projects_node.IsDefined())
			throw YAML::Exception(projects_node.Mark(), "missing projects");

		if (!projects_node.IsSequence())
			throw YAML::Exception(projects_node.Mark(), "projects must be a sequence");

		for (const auto& file : projects_node) {
			if (!file.IsScalar())
				throw YAML::Exception(file.Mark(), "project file must be a string");

			project_files_to_load.push_back(file.as<std::string>());
		}
	}

	{
		const auto methods_node = meta["methods"];

		if (!methods_node.IsDefined())
			throw YAML::Exception(methods_node.Mark(), "missing methods");

		if (!methods_node.IsSequence())
			throw YAML::Exception(methods_node.Mark(), "methods must be a sequence");

		for (const auto& file : methods_node) {
			if (!file.IsScalar())
				throw YAML::Exception(file.Mark(), "method file must be a string");

			method_files_to_load.push_back(file.as<std::string>());
		}
	}
}

void Database::LoadMethods(const std::string& path) {
	YAML::Node data = YAML::LoadFile(path);

	if (!data.IsSequence())
		throw YAML::Exception(data.Mark(), "must be a sequence");

	for (const auto& entry : data) {
		DonationMethod method;

		{
			const auto& keyword_node = entry["keyword"];

			if (!keyword_node.IsDefined())
				throw YAML::Exception(entry.Mark(), "missing keyword");

			if (!keyword_node.IsScalar())
				throw YAML::Exception(keyword_node.Mark(), "keyword must be a string");

			method.keyword = keyword_node.as<std::string>();
		}

		{
			const auto& name_node = entry["name"];

			if (!name_node.IsDefined())
				throw YAML::Exception(entry.Mark(), "missing name");

			if (!name_node.IsScalar())
				throw YAML::Exception(name_node.Mark(), "name must be a string");

			method.name = name_node.as<std::string>();
		}

		{
			const auto& icon_node = entry["icon"];

			if (icon_node.IsDefined()) {
				if (!icon_node.IsScalar())
					throw YAML::Exception(icon_node.Mark(), "icon must be a string");

				method.icon = icon_node.as<std::string>();
			}
		}

		methods_.insert(std::make_pair(method.keyword, std::move(method)));
	}
}

void Database::LoadProjects(const std::string& path) {
	YAML::Node data = YAML::LoadFile(path);

	if (!data.IsSequence())
		throw YAML::Exception(data.Mark(), "must be a sequence");

	for (const auto& entry : data) {
		Project proj;

		{
			const auto& name_node = entry["name"];

			if (!name_node.IsDefined())
				throw YAML::Exception(entry.Mark(), "missing name");

			if (!name_node.IsScalar())
				throw YAML::Exception(name_node.Mark(), "name must be a string");

			proj.name = name_node.as<std::string>();
		}

		{
			const auto& comment_node = entry["comment"];

			if (comment_node.IsDefined()) {
				if (!comment_node.IsScalar())
					throw YAML::Exception(comment_node.Mark(), "comment must be a string");

				proj.comment = comment_node.as<std::string>();
			}
		}

		{
			const auto& url_node = entry["url"];

			if (!url_node.IsDefined())
				throw YAML::Exception(entry.Mark(), "missing url");

			if (!url_node.IsScalar())
				throw YAML::Exception(url_node.Mark(), "url must be a string");

			proj.url = url_node.as<std::string>();
		}

		{
			const auto& detection_node = entry["detection"];

			if (detection_node.IsDefined()) {
				if (detection_node.IsMap()) {
					for (const auto& detection : detection_node) {
						Project::DetectionTagVector vec;

						if (detection.second.IsSequence()) {
							for (const auto& keyword : detection.second)
								vec.emplace_back(keyword.as<std::string>());
						} else if (detection.second.IsScalar()) {
							vec.emplace_back(detection.second.as<std::string>());
						}

						proj.detection_tags.emplace(std::make_pair(detection.first.as<std::string>(), std::move(vec)));
					}
				} else {
					throw YAML::Exception(detection_node.Mark(), "detection must be a map");
				}
			}
		}

		{
			const auto& donation_url_node = entry["donations"];

			if (donation_url_node.IsDefined()) {
				if (!donation_url_node.IsScalar())
					throw YAML::Exception(donation_url_node.Mark(), "donation url must be a string");

				proj.donation_url = donation_url_node.as<std::string>();
			}
		}

		{
			const auto& methods_node = entry["methods"];

			if (!methods_node.IsDefined())
				throw YAML::Exception(entry.Mark(), "missing donation methods");

			if (!methods_node.IsSequence())
				throw YAML::Exception(methods_node.Mark(), "methods must be a sequence");

			for (const auto& method_node : methods_node) {
				if (!method_node.IsScalar())
					throw YAML::Exception(method_node.Mark(), "donation method must be a string");

				std::string method = method_node.as<std::string>();

				if (!HasDonationMethod(method))
					throw YAML::Exception(method_node.Mark(), "unknown donation method");
				else
					proj.donation_methods.emplace(std::move(method));
			}
		}

		projects_.emplace_back(std::move(proj));
	}
}

void Database::Load(const std::string& dbdir) {
	std::vector<std::string> method_files_to_load;
	std::vector<std::string> project_files_to_load;

	try {
		LoadMeta(dbdir + "/meta.yaml", method_files_to_load, project_files_to_load);
	} catch (YAML::Exception& e) {
		throw ParsingException(dbdir + "/meta.yaml", e.mark.line, e.mark.column, e.msg);
	}

	for (const auto& file : method_files_to_load) {
		try {
			LoadMethods(dbdir + "/" + file);
		} catch (YAML::Exception& e) {
			throw ParsingException(dbdir + "/" + file, e.mark.line, e.mark.column, e.msg);
		}
	}

	for (const auto& file : project_files_to_load) {
		try {
			LoadProjects(dbdir + "/" + file);
		} catch (YAML::Exception& e) {
			throw ParsingException(dbdir + "/" + file, e.mark.line, e.mark.column, e.msg);
		}
	}
}

Result Database::GetAll() const {
	Result res;

	for (const auto& project: projects_)
		res.Add(&project);

	return res;
}

Result Database::Query(const DetectorChain& detectors, int flags) const {
	Result res;

	for (const auto& project: projects_)
		if (detectors.Check(project) != (flags & INVERT_DETECTION))
			res.Add(&project);

	return res;
}

size_t Database::size() const {
	return projects_.size();
}

bool Database::HasDonationMethod(const std::string& keyword) const {
	return methods_.find(keyword) != methods_.end();
}

const DonationMethod& Database::GetDonationMethod(const std::string& keyword) const {
	auto iter = methods_.find(keyword);

	if (iter == methods_.end())
		throw std::runtime_error("bad donation method keyword");

	return iter->second;
}

void Database::ForEachDonationMethod(std::function<void(const DonationMethod&)>&& handler) const {
	for (const auto& method: methods_)
		handler(method.second);
}

}
