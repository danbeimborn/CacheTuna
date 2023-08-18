#ifndef CACHETUNA_AUTOTUNA_HPP
#define CACHETUNA_AUTOTUNA_HPP

// std
#include <map>
#include <cmath>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>

namespace Autotuna {
   struct scaled_data {
      std::vector<unsigned> order;
      std::vector<std::vector<float>> matrix;
   };

   std::string construct_bitmask_str(std::pair<int, int> way_contention_index, int l3_num_ways, int num_active_ways, int pos_0);
   scaled_data scale_misses_matrix(const std::map<unsigned, std::vector<uint64_t>>& cos_misses_matrix, std::map<unsigned, int>& priority_map);
   void adjust_misses_matrix_values(scaled_data& data, const std::map<unsigned, int>& min_ways_map, int root_cos, int remaining_ways);
   void calculate_optimal_ways_combination(const scaled_data& data, std::map<unsigned, int>& min_ways_map, const std::map<unsigned, int>& priority_map, int root_cos);
}

#endif //cachetuna_autotuna_hpp
