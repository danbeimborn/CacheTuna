#include "autotuna.hpp"

using namespace Autotuna;

float MAX_VALUE = std::numeric_limits<float>::max();

std::string Autotuna::construct_bitmask_str(std::pair<int, int> way_contention_index, int l3_num_ways, int num_active_ways, int pos_0) {
   std::string bitmask; 
   std::string active_ways = std::string(num_active_ways, '1');

   int num_tail_ways = 0;
   int num_head_ways = pos_0;
   int contention_start = way_contention_index.first + 1;
   int contention_end = way_contention_index.second + 1;

   /* Check if num_active_ways splits way contention
    * Example: 
    * l3_num_ways = 11
    * way_contention_index = [0, 1] -> 11000000000
    * num_active_ways = 1 -> 10000000000  
   */
   if ((num_active_ways + pos_0) >= contention_start && (num_active_ways + pos_0) < contention_end) {
      // calculate starting point of num_active ways
      num_tail_ways = l3_num_ways - contention_end;
      if (num_active_ways <= num_tail_ways) {
         num_head_ways = l3_num_ways - num_active_ways;
         num_tail_ways = 0;
      }
      else {
         num_head_ways = contention_start - 1;
         num_tail_ways = l3_num_ways - (num_head_ways + num_active_ways);
         if (num_tail_ways < 0) {
            num_head_ways -= num_tail_ways;
            num_tail_ways = 0;
         }
      }
   }
   else {
      num_tail_ways = l3_num_ways - pos_0 - num_active_ways;
   }
   bitmask = std::string(num_head_ways, '0') + active_ways + std::string(num_tail_ways, '0');
   return bitmask;
}

scaled_data Autotuna::scale_misses_matrix(const std::map<unsigned, std::vector<uint64_t>>& cos_misses_matrix, std::map<unsigned, int>& priority_map) {
   scaled_data data;

   auto normalise_priority_ranking = [&](int rank) -> float {
      if (rank == 0) return 1.0;
      auto max_rank = std::max_element(priority_map.begin(), priority_map.end(), [](const auto &x, const auto &y) 
                      { return x.second < y.second; });
      float weight = ((float)rank - 0.0) / ((float)max_rank->second - 0.0);
      return weight;
   };

   for (const auto&[cos, misses_vec] : cos_misses_matrix) {
      // scale cos_misses_matrix by the weight calculated based on each cos' priority rankings 
      float weight = normalise_priority_ranking(priority_map[cos]);
      std::vector<float> scaled_vec(misses_vec.begin(), misses_vec.end());
      std::transform(scaled_vec.begin(), scaled_vec.end(), scaled_vec.begin(), [weight](uint64_t x) { return weight * (float)x; });
      // populate scaled_data struct
      data.matrix.push_back(scaled_vec);
      data.order.push_back(cos); 
   }
   return data;
}

void Autotuna::adjust_misses_matrix_values(scaled_data& data, const std::map<unsigned, int>& min_ways_map, int root_cos, int remaining_ways) {
   // Adjust scaled matrix's values based on each cos' minimum needed ways to prune dynamic programming
   for (const auto&[cos, min_ways] : min_ways_map) {
      if (cos == root_cos) continue;

      size_t start, end;
      int cos_index = std::distance(data.order.begin(), std::find(data.order.begin(), data.order.end(), cos));

      if (remaining_ways < 0) {
         start = min_ways;
         end = data.matrix[cos_index].size();
      }
      else {
         start = 0;
         end = min_ways - 1;
      }
      for (start; start < end; ++start) data.matrix[cos_index][start] = MAX_VALUE;
   }
}

void Autotuna::calculate_optimal_ways_combination(const scaled_data& data, std::map<unsigned, int>& min_ways_map, const std::map<unsigned, int>& priority_map, int root_cos) {
   struct Allocation {
      float misses;
      int num_ways;
   };

   // Construct a (n+1)x(W+1) lookup table T to store previously calculated Allocation values
   int n = data.matrix.size();
   int W = data.matrix[0].size();
   Allocation initial_values = {MAX_VALUE, 0};
   std::vector<std::vector<Allocation>> T(n + 1, std::vector<Allocation>(W + 1, initial_values));

   // Fill 0th row of T with misses = 0.0 and num_ways = 0
   for (int j=0; j<T[0].size(); ++j) {
      T[0][j].misses = 0.0;
   }

   // Populate T
   for (int i=1; i<=n; ++i) {
      for (int j=1; j<=W; ++j) {
         for (int k=1; k<=j; ++k) {
            float curr_misses = data.matrix[i-1][k-1];
            float total_misses = curr_misses + T[i-1][j-k].misses;
            if (total_misses < T[i][j].misses) {
               T[i][j].misses = total_misses;
               T[i][j].num_ways = k;
            }
         }
      }
   }

   // Backtrack on lookup table T to retrive the calculated optimal num_ways for each cos
   int ways = W;
   for (int cos = n; cos >= 1; cos--) {
      unsigned cos_id = data.order[cos-1];
      min_ways_map[cos_id] = T[cos][ways].num_ways;
      ways -= T[cos][ways].num_ways; 
   }

   // Handle the case where there are extra unallocated ways due to numerous cos with 0 misses
   auto sort_by_rank = [&](const std::pair<unsigned, int>& a, const std::pair<unsigned, int>& b) -> bool {
      return (a.second < b.second);
   };

   if (ways > 0) {
      // sort cos by priority rank
      std::vector<std::pair<unsigned, int>> sorted_priority_vec;
      for (const auto&[cos, rank] : priority_map) {
         if (cos == root_cos) continue;
         sorted_priority_vec.push_back(std::make_pair(cos, rank));
      }
      sort(sorted_priority_vec.begin(), sorted_priority_vec.end(), sort_by_rank);

      // allocate extra ways to each cos by portion
      int portion = std::ceil(ways / n);
      for (const auto& cos : sorted_priority_vec) {
         unsigned cos_id = cos.first;
         if (ways == 0) { 
            continue;
         } else if (ways < portion) {
            min_ways_map[cos_id] += ways;
         } else {
            min_ways_map[cos_id] += portion;
         }
         ways -= portion;
      }
   }
} 
