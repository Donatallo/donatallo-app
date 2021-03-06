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

#include <getopt.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <exception>

#include <libdonatallo/database.hh>
#include <libdonatallo/detectorfactory.hh>

static struct option longopts[] = {
	{ "all",       no_argument,       nullptr, 'a' },
	{ "database",  required_argument, nullptr, 'd' },
	{ "help",      no_argument,       nullptr, 'h' },
	{ "invert",    no_argument,       nullptr, 'i' },
	{ "method",    required_argument, nullptr, 'm' },
	{ "methods",   required_argument, nullptr, 'm' },
	{ nullptr,     0,                 nullptr, 0   },
};

void Usage(const std::string& progname) {
	//            [ 75 character width                                                      ]
	std::cerr << "Usage: " << progname << " [-ah] [-d DATABASE] [-m METHOD,...]" << std::endl;
	std::cerr << std::endl;
	std::cerr << " -a, --all             list all projects in the database" << std::endl;
	std::cerr << " -d, --database=PATH   specify path to the database" << std::endl;
	std::cerr << " -m, --method=METHOD, --methods=METHOD,..." << std::endl;
	std::cerr << "                       specify donation methods filter" << std::endl;
	std::cerr << "                       (see --methods=list for supported methods list)" << std::endl;
	std::cerr << " -i, --invert          invert project detection: show not detected projects" << std::endl;
	std::cerr << " -h, --help            show this help" << std::endl;
	//            [ 75 character width                                                      ]
}

void MethodsList(const Donatallo::Database& database) {
	std::cerr << "Available donation methods:" << std::endl;
	size_t maxlength = 0;
	database.ForEachDonationMethod([&](const Donatallo::DonationMethod& method) {
		maxlength = std::max(maxlength, method.keyword.length());
	});

	database.ForEachDonationMethod([&](const Donatallo::DonationMethod& method) {
		std::cerr << "\t" << std::setw(maxlength) << std::left << method.keyword;
		std::cerr << " " << std::setw(0) << method.name << std::endl;
	});
}

int main(int argc, char** argv) try {
	const std::string progname = argv[0];

	bool all_mode = false;
	bool methods_list_mode = false;
	int query_flags = 0;

	std::string database_path = DONATALLO_DATADIR "/database";
	std::set<std::string> wanted_methods;

	int ch;
	while ((ch = getopt_long(argc, argv, "ad:him:", longopts, NULL)) != -1) {
		switch (ch) {
		case 'a':
			all_mode = true;
			break;
		case 'd':
			database_path = optarg;
			break;
		case 'm':
			{
				std::string methods = optarg;
				size_t start = 0, end;
				do {
					end = methods.find(",", start);
					std::string methodname = methods.substr(start, end == std::string::npos ? std::string::npos : end - start);

					if (methodname == "list")
						methods_list_mode = true;
					else
						wanted_methods.insert(methodname);

					start = end + 1;
				} while (end != std::string::npos);
			}
			break;
		case 'i':
			query_flags |= Donatallo::Database::INVERT_DETECTION;
			break;
		case 'h':
			Usage(progname);
			exit(0);
		default:
			Usage(progname);
			exit(1);
		}
	}

	// Load database
	Donatallo::Database db;
	db.Load(database_path);

	if (methods_list_mode) {
		MethodsList(db);
		exit(0);
	} else {
		for (const auto& method : wanted_methods)
			if (!db.HasDonationMethod(method))
				std::cerr << "Warning: unknown donation method " << method << std::endl;
	}

	Donatallo::Result projects;

	// Query
	if (all_mode) {
		projects = db.GetAll();
	} else {
		Donatallo::DetectorChain detectors = Donatallo::DetectorFactory::GetInstance()->GetAllDetectors();

		detectors.Prepare([](int current, int total) {
			std::cerr << "Gathering information... " << current << "/" << total << "\r";
		});

		std::cerr << "Gathering information... Done" << std::endl;

		projects = db.Query(detectors, query_flags);
	}

	projects = projects.SortByName();

	if (!wanted_methods.empty())
		projects = projects.FilterByMethods(wanted_methods);

	std::cerr << "Listing " << projects.size() << " project(s) of " << db.size() << " total in the database" << std::endl << std::endl;

	// Display
	for (const auto& project : projects) {
		std::cout << "    Project: " << project->name << std::endl;
		if (!project->comment.empty())
			std::cout << "Description: " << project->comment << std::endl;
		std::cout << "    Website: " << project->url << std::endl;
		if (!project->donation_url.empty())
			std::cout << "  Donations: " << project->donation_url << std::endl;
		std::cout << "    Methods: ";
		for (auto imethod = project->donation_methods.cbegin(); imethod != project->donation_methods.cend(); imethod++) {
			if (imethod != project->donation_methods.cbegin())
				std::cout << ", ";
			std::cout << db.GetDonationMethod(*imethod).name;
		}
		std::cout << std::endl;
		std::cout << std::endl;
	}

	if (projects.empty())
		std::cerr << "No projects found, sorry :(" << std::endl;

	return 0;
} catch (std::exception& e) {
	std::cerr << "Fatal error: " << e.what() << std::endl;
	return 1;
}
