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

#ifndef DONATALLO_FILEINETCDETECTOR_HH
#define DONATALLO_FILEINETCDETECTOR_HH

#include <unordered_set>
#include <string>

#include <libdonatallo/detector.hh>

namespace Donatallo {

class Project;

class FileInEtcDetector : public Detector {
private:
	std::unordered_set<std::string> files_;

public:
	virtual ~FileInEtcDetector();

	virtual void Prepare();

	virtual bool Check(const Project& project) const final;
};

}

#endif
