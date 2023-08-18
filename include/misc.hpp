#ifndef CACHETUNA_MISC_HPP
#define CACHETUNA_MISC_HPP

//std
#include <filesystem>
#include <iomanip> // setprecision
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <cmath>

namespace Misc {
	std::vector<std::string> run_cmd(std::string cmd);
	std::vector<std::string> get_cpu_info();
	std::string format_bytes(uint64_t bytes);
	std::string format_misses(uint64_t val);
	std::string to_range_extraction(const std::set<int>& numbers);
	std::string get_executable_path();
	bool not_contiguous(const std::string& bitmask);
	unsigned to_decimal(std::string bitmask);
}

#endif //cachetuna_misc_hpp
