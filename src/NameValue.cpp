/*
 * Copyright (C) 2022 rpf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <iostream>
#include <fstream>
#include <string>
#include <StringUtils.hpp>

#include "NameValue.hpp"


bool
NameValue::read(const std::string &name)
{
    std::ifstream  stat;
    std::string  str;
	m_values.clear();
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit  | std::ios::eofbit;
    stat.exceptions(exceptionMask);
    /* Open status file  */
	bool ret = true;
    try  {
        stat.open (name);
        if (stat.is_open()) {
            while (!stat.eof()) {
                std::getline(stat, str);
				int pos = str.find(":");
                if (pos > 0) {
                    std::string name = str.substr(0, pos+1);
                    std::string value = str.substr(pos+1);
                    StringUtils::trim(value);
					//std::cout << "Read " << name << value << std::endl;
					m_values.insert(std::pair<std::string, std::string>(name, value));
                }
            }
        }
    }
    catch (std::ios_base::failure &e) {
        if (!stat.eof()) {  // as we may hit eof while reading ...
            std::cerr << name << " what " << e.what() << " val " << e.code().value() << " Err " << e.code().message() << std::endl;
			ret = false;
        }
    }
    if (stat.is_open()) {
        stat.close();
    }
	return ret;
}

unsigned long
NameValue::getUnsigned(const std::string &name)
{
	unsigned long ulVal = 0ul;
	auto val = m_values.find(name);
	if (val != m_values.end()) {
		auto sval = val->second;
		//std::cout << "Got " << name << sval << std::endl;
		auto pos = sval.rfind(" ");
		if (pos != std::string::npos) {
			sval = sval.substr(0, pos);	// usually the number is followed by a unit so cut it
		}
		//std::cout << "Got trunc " << name << sval << std::endl;
		ulVal = std::stoul(sval);
	}
	return ulVal;
}

std::string
NameValue::getString(const std::string &name)
{
	auto val = m_values.find(name);
	if (val != m_values.end()) {
		auto sval = val->second;
		//std::cout << "Got " << name << sval << std::endl;
		return sval;
	}
	return "";
}