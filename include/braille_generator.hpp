#ifndef CACHETUNA_BRAILLE_GENERATOR_HPP
#define CACHETUNA_BRAILLE_GENERATOR_HPP

// std
#include <algorithm>
#include <bitset>
#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace Braille_Generator {
   std::vector<std::string> generate_braille(int width_w, int height_w, const char *filename);
   std::vector<std::vector<std::string>> generate_all_frames(int width, int height, const std::string& directory, int frame_count);
}

#endif // CACHETUNA_BRAILLE_GENERATOR_HPP
