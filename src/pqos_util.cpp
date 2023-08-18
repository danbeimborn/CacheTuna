#include "pqos_util.hpp"

Pqos::Pqos():
   p_cpu(NULL),
   p_cap(NULL),
   l3cat_count(0),
   ret(EXIT_SUCCESS),
   exit_val(EXIT_SUCCESS),
   monInitialised(false),
   monReset(false),
   analysis_completed(false),
   autotuning_completed(false),
   run_thread(true),
   priority_count(0)
{}

std::string pqos_retval_msg(int retval) {
   switch(retval) {
      case (1):
         return "1: Gerneric Error";
      case (2):
         return "2: Parameter Error";
      case (3):
         return "3: Resource Error";
      case (4):
         return "4: Initialization Error";
      case (5):
         return "5: Transport Error";
      case (6):
         return "6: Performance Counter Error";
      case (7):
         return "7: Resource Busy Error";
      case (8):
         return "8: Interface Not Supported";
      case (9):
         return "9: Data Overflow";
      default:
         return std::to_string(retval);
   }
}

unsigned Pqos::get_num_cores() {
   return p_cpu->num_cores;
}

bool Pqos::get_l3_detected() {
   switch (p_cpu->l3.detected) {
      case (0):
         return false;
      case (1):
         return true;
      default:
         return false;
   };
}

unsigned Pqos::get_l3_num_ways() {
   return p_cpu->l3.num_ways;
}

unsigned Pqos::get_l3_num_sets() {
   return p_cpu->l3.num_sets;
}

unsigned Pqos::get_l3_num_partitions() {
   return p_cpu->l3.num_partitions;
}

unsigned Pqos::get_l3_line_size() {
   return p_cpu->l3.line_size;
}

unsigned Pqos::get_l3_total_size() {
   return p_cpu->l3.total_size;
}

unsigned Pqos::get_l3_way_size() {
   return p_cpu->l3.way_size;
}

unsigned Pqos::get_l3cos_count() {
   return l3cos_count;
}

std::string Pqos::get_way_contention() {
  return way_contention;
}

std::vector<struct L3_Cos> Pqos::get_l3_cos_vec() {
   return l3_cos_vec;
}

std::vector<uint64_t> Pqos::get_cos_llc_vec(int cos) {
   return l3_cos_vec[cos].llc;
}

std::vector<uint64_t> Pqos::get_cos_misses_vec(int cos) {
   return l3_cos_vec[cos].misses;
}

std::map<unsigned, int> Pqos::get_autotuna_min_ways_map() {
   return autotuna_min_ways_map;
}

std::map<unsigned, std::vector<uint64_t>> Pqos::get_cos_misses_matrix() {
   return cos_misses_matrix;
}

std::map<unsigned, int> Pqos::get_priority_map() {
   return priority_map;
}

int Pqos::get_num_active_cos() {
   int non_active_cos = std::count_if(l3_cos_vec.begin(), l3_cos_vec.end(), [](const auto& cos) 
                        { return (cos.id == 0 || cos.cores.empty());});
   return get_l3cos_count() - non_active_cos;
}

std::pair<int, int> Pqos::get_way_contention_index() {
   int start = static_cast<int>(way_contention.find('1'));
   int end = static_cast<int>(way_contention.find_last_of('1'));
   return std::make_pair(start, end);
}

std::set<int> Pqos::get_bit_assoc(int bit) {
   std::set<int> cos_set;
   for (const L3_Cos& cos : l3_cos_vec) {
      if (cos.new_bitmask[bit] == '1' && cos.id != 0 && !cos.new_cores.empty()) {
         cos_set.insert(cos.id);
      }
   }
   return cos_set;
}

int Pqos::get_core_assoc(int core) {
   for (const L3_Cos& cos : l3_cos_vec) {
      if (cos.new_cores.find(core) != cos.new_cores.end()) {
         return cos.id;
      }
   }
   return 0;
}

std::vector<unsigned> Pqos::get_mon_data_index_vec() {
   std::vector<unsigned> index_vec;
   for (const L3_Cos& cos : l3_cos_vec) {
      if (!cos.cores.empty())
         index_vec.push_back(cos.id);
   }
   return index_vec;
}

std::vector<std::vector<uint64_t>> Pqos::get_all_llc_vec() {
   std::vector<std::vector<uint64_t>> llc_vec;
   for (const L3_Cos& cos : l3_cos_vec) {
      if (!cos.cores.empty())
         llc_vec.push_back(cos.llc);
   }
   return llc_vec;
}

std::vector<std::vector<uint64_t>> Pqos::get_all_misses_vec() {
   std::vector<std::vector<uint64_t>> misses_vec;
   for (const L3_Cos& cos : l3_cos_vec) {
      if (!cos.cores.empty())
         misses_vec.push_back(cos.misses);
   }
   return misses_vec;
}

void Pqos::get_cos_tags() {
   // Open file for reading
   std::ifstream file("/etc/sysconfig/cache_policy");

   // Initialize regex pattern
   // 
   // Example of a 3-part config
   // POLICY_1=11000000000 ;  NAME_1="Junk/Root"        ;  CORES_1="0,18"
   // POLICY_2=00111110000 ;  NAME_2="Qube Fast Path"   ;  CORES_2="1-17"
   // POLICY_3=00000001111 ;  NAME_3="Qube Slow Path"   ;  CORES_3="19-35"
   //
   // five capture groups:
   // 1 (policy number): `(\d+)`: Matches one or more digits after "POLICY_"
   // 2 (name number): `(\d+)`: Matches one or more digits after "NAME_"
   // 3 (tag string): `"([^"]+"`: Matches quote string after name number
   // 4 (cores number): `(\d+)`: Matches one or more digits after "CORES_"
   // 5 (cores string): `"([^"]+"`: Matches quote string after cores number

   std::regex tag_regex(".*POLICY_(\\d+)=\\d+\\s*;\\s*NAME_(\\d+)=\"([^\"]+)\"\\s*;\\s*CORES_(\\d+)=\"([^\"]+).*");

   // Initialize variables for parsing
   std::smatch matches; // object to hold matched subexpressions
   std::string line;

   // Parse the file
   size_t i=0;
   while (std::getline(file, line)) {
      int policy_number = -1, name_number = -1, cores_number = -1;
      std::string tag_string = "", cores_string ="";
      ++i;
      // check if line matches the policy regex and has the same integer value for policy, name and cores
      if ((std::regex_match(line, matches, tag_regex))  && (line.find("#") == std::string::npos)) {
          // Extract integer values from subgroups
          int current_policy_number = std::stoi(matches[1].str());
          int current_name_number = std::stoi(matches[2].str());
          int current_cores_number = std::stoi(matches[4].str());

          // Check if current integers match previous integers
          if (current_policy_number == current_name_number && current_name_number == current_cores_number) {
             // Add tag_string to vector if integers match
             l3_cos_vec[current_name_number].tag = matches[3].str();
             l3_cos_vec[current_name_number].new_tag = matches[3].str();
          } 
      }
   }
   file.close();
}

void Pqos::update_processes_vec() {
   for (L3_Cos& cos : l3_cos_vec) {
      std::vector<std::string> processes;
      for (const int& core : cos.cores) {
         std::ostringstream cmd;
         cmd << "ps ao user,pid,pcpu,pmem,psr,stat,start,time,command | awk '$5==\"" << core << "\" {print $0}'";
         for (const std::string& line : Misc::run_cmd(cmd.str())) {
            processes.push_back(line);
         }
      }
      cos.processes = processes;
   }
}

bool Pqos::start_resource_monitoring() {
   pqos_mon_reset(); // Resets monitoring by binding all cores with RMID0 

   for (auto& cos : l3_cos_vec) {
      // flush all cos' mon data
      cos.llc.clear();
      cos.misses.clear();

      std::cout << "Creating resource monitoring data group for COS " << cos.id << std::endl;
      if (cos.cores.empty()) {
         std::cout << "No cores assigned to COS group " << cos.id << std::endl;
         pqos_mon_data_vec.push_back(NULL);
      }
      else {
         pqos_mon_data_vec.push_back(new pqos_mon_data);
         // Converting cores set to array for pqos function
         unsigned cores_array[cos.cores.size()];
         std::copy(cos.cores.begin(), cos.cores.end(), cores_array);
         // Starting monitoring event
         ret = pqos_mon_start(cos.cores.size(), &cores_array[0], mon_events, nullptr, pqos_mon_data_vec.back());
         if (ret != PQOS_RETVAL_OK) {
            std::cout << "Error starting resource monitoring on COS " << cos.id << std::endl;
            return 0;
         }
      }
   }
   return 1;
}

void Pqos::update_new_tag(const std::string& new_tag, int cos) {
   l3_cos_vec[cos].new_tag = new_tag;

   bool &unsaved = l3_cos_vec[cos].unsaved_changes;
   if (new_tag != l3_cos_vec[cos].tag) unsaved = true;
   else unsaved = false;
}

void Pqos::update_new_bitmask(int bit_selected, int cos) {
   std::string &mask = l3_cos_vec[cos].new_bitmask;
   std::set<int> &cores = l3_cos_vec[cos].new_cores;

   // DDIO Mask -- first two bits must be the same
   std::vector<int> bit_targets;
   if (bit_selected < 2) {
      bit_targets.push_back(0);
      bit_targets.push_back(1);
   } else {
      bit_targets.push_back(bit_selected);
   }

   // flip bit
   for (const int& bit : bit_targets) {
      mask[bit] = mask[bit] == '1' ? '0' : '1';
   }

   if (Misc::not_contiguous(mask)) {
      non_contiguous_cos_set.insert(cos);
   } else {
      non_contiguous_cos_set.erase(cos);
   }

   unsigned &size = l3_cos_vec[cos].new_size;
   size = std::count_if(mask.begin(), mask.end(), [&] (char bit) {return bit == '1';}) * get_l3_way_size();

   bool &unsaved = l3_cos_vec[cos].unsaved_changes;
   if (mask != l3_cos_vec[cos].bitmask) unsaved = true;
   else unsaved = false;
}

void Pqos::update_new_cores(int core_selected, int cos_selected) {
   L3_Cos &cos = l3_cos_vec[cos_selected]; 
   std::set<int> &new_cores = cos.new_cores;
   int assoc = get_core_assoc(core_selected);

   // unassociate the selected core with the selected cos
   if (assoc == cos_selected) {
      new_cores.erase(core_selected);
      l3_cos_vec[0].new_cores.insert(core_selected);
   }
   // associate the selected core with the selected cos
   else if (assoc == 0) {
      new_cores.insert(core_selected);
      l3_cos_vec[0].new_cores.erase(core_selected);
   }

   bool &unsaved = cos.unsaved_changes;
   if (new_cores != cos.cores || cos.bitmask != cos.new_bitmask) unsaved = true;
   else unsaved = false;
}

void Pqos::revert_changes() {
   
   for (size_t i=0; i<l3_cos_vec.size(); ++i) {
      L3_Cos& cos = l3_cos_vec[i];
      for (const int& core : cos.cores) {
        pqos_alloc_assoc_set(core, cos.id);
      }
      l3ca_table[i].u.ways_mask = Misc::to_decimal(cos.bitmask);
   }
   pqos_l3ca_set(l3cat_ids[0], get_l3cos_count(), l3ca_table);
}

int Pqos::apply_changes() {
   std::stringstream policies;
   for (size_t i=0; i<l3_cos_vec.size(); ++i) {
      L3_Cos& cos = l3_cos_vec[i];
      // Change core association to a corresponding class of service
      for (const int& core : cos.new_cores) {
         if (pqos_alloc_assoc_set(core, cos.id) != PQOS_RETVAL_OK) {
            revert_changes();
            return 1;
         }
      }

      // Update L3 Class of service's ways mask attribute
      l3ca_table[i].u.ways_mask = Misc::to_decimal(cos.new_bitmask);

      // Write to string for cache_policy
      if (!cos.new_cores.empty()) {
         std::stringstream policy;
         policy << "POLICY_" << cos.id << "=" << cos.new_bitmask << " ";
         std::stringstream name;
         name << "\tNAME_" << cos.id << "=\"" << cos.new_tag << "\" ";
         std::stringstream cores;
         cores << "\tCORES_" << cos.id << "=\"" << Misc::to_range_extraction(cos.new_cores); 

         policies << "\n" 
         << std::setw(15) << std::setfill(' ') << std::left << policy.str() << ";"
         << std::setw(30) << std::setfill(' ') << std::left << name.str() << ";"
         << cores.str() << "\"";
      }
   }
   // Set all class of service's ways mask
   if (pqos_l3ca_set(l3cat_ids[0], get_l3cos_count(), l3ca_table) != PQOS_RETVAL_OK) {
      revert_changes();
      return 2;
   }

   // Only update /etc/sysconfig/cache_policy if PQoS api returns OK
   std::string num_sockets = Misc::run_cmd("lscpu | egrep 'Socket' | cut -d: -f2 | tr -d '[:space:]'")[0];
   std::string num_cores = Misc::run_cmd("lscpu | egrep 'Core' | cut -d: -f2 | tr -d '[:space:]'")[0];
   std::string cpu = Misc::run_cmd("lscpu | egrep 'Model name' | cut -d: -f2 | awk '{$1=$1};1'")[0];
   std::stringstream output;

   output << "\n"
   << "#\n"
   << "# " << cpu
   << "# " << num_sockets << "x " << num_cores << " cores\n\n"
   << "WAYS=" << get_l3_num_ways() << " " 
   << "# Number of separate cache ways we can define" << "\n"
   << "WAYS_SIZE=" << (get_l3_way_size()/1024/1024) << " " 
   << "# The size in MB of each individual cache way" << "\n"
   << policies.str()
   << "\n\n"
   << "# Example of a 3-part config\n"
   << "#POLICY_1=11000000000 ;  NAME_1=\"Junk/Root\"        ;  CORES_1=\"0,18\"\n"
   << "#POLICY_2=00111110000 ;  NAME_2=\"Qube Fast Path\"   ;  CORES_2=\"1-17\"\n"
   << "#POLICY_3=00000001111 ;  NAME_3=\"Qube Slow Path\"   ;  CORES_3=\"19-35\"\n";

   std::ofstream file(Misc::get_executable_path() + "cache_policy");
   if (file.is_open()) {
      file << output.str();
      file.close();
   } else return 3;

   // Cores and bitmasks updated successfully, backup original config and update struct variables
   backup_config("backup.conf");
   for (size_t i=0; i<l3_cos_vec.size(); ++i) {
      L3_Cos& cos = l3_cos_vec[i];
      cos.cores = cos.new_cores;
      cos.bitmask = cos.new_bitmask;
      cos.size = std::count_if(cos.bitmask.begin(), cos.bitmask.end(), [&] (char bit) {return bit == '1';}) * get_l3_way_size();
      cos.tag = cos.new_tag;
      update_processes_vec();
      cos.unsaved_changes = false;
   }
   // reset pqos_mon_data_vec
   monReset = true;

   return PQOS_RETVAL_OK;
}

void Pqos::backup_config(const std::string& file_name) {
   // Parse stringstream
   std::stringstream ss;
   for (const L3_Cos& cos : l3_cos_vec) {
      ss << cos.tag << "\n";
      ss << cos.bitmask << "\n";
      for (const int& core : cos.cores) {
         ss << core;
         if (&core != &(*cos.cores.rbegin())) {
               ss << ",";
         }
      }
      ss << "\n";
   }
  
   // Write to file
   std::string file_path = Misc::get_executable_path() + file_name;
   std::ofstream outfile(file_path, std::ios::out | std::ios::trunc);
   if (outfile.is_open()) {
      outfile << ss.str();
      outfile.close();
   }
}

int Pqos::load_config(const std::string& file_name) {
   std::string file_path = Misc::get_executable_path() + file_name;
   std::ifstream infile(file_path);
   std::string line;
   int line_count = 0;
   int cos = 0;

   if (infile) {
      while (std::getline(infile, line)) {
         ++line_count;
         switch (line_count) {
            case 1:
               l3_cos_vec[cos].new_tag = line;
               break;
            case 2:
               l3_cos_vec[cos].new_bitmask = line;
               break;
            case 3:
               std::istringstream iss(line);
               std::string core;
               std::set<int> cores;
               while (std::getline(iss, core, ',')){
                  cores.insert(std::stoi(core));
               }
               l3_cos_vec[cos].new_cores = cores;

               ++cos;
               line_count = 0;
               break;
         }
      }
      infile.close();
      return apply_changes();
   } else {
      return 9; // file not found
   }
}

void Pqos::init_priority_map_key_val(int cos) {
   priority_map[cos] = 0;
}

void Pqos::update_priority_rank(int cos){
   int cos_priority = priority_map[cos];
   if (cos_priority == 0) { // priority not set
      ++priority_count;
      priority_map[cos] = priority_count;
   } else if (cos_priority == priority_count) { // cos is at top of stack, unset priority
      --priority_count;
      priority_map[cos] = 0;
   }
}

// PQoS processing for AutoTuna analysis 
int Pqos::run_autotuna_analysis(int threshold, int root_cos, int num_free_ways) {
   for (const L3_Cos& cos : l3_cos_vec) {
      if (!run_thread) {
         revert_changes();
         return 3;
      }
      if (cos.id == 0 || cos.id == root_cos || cos.cores.empty()) continue; // cos 0 is default and Junk/Root does not need to be tuned
      
      // Reset all cos configurations (pqos -R): all cores pinned to cos 0 and all ways set to 1
      // cores
      std::vector<int> cores(get_num_cores());
      std::iota(cores.begin(), cores.end(), 0);
      for (const int& core : cores) {
         if (pqos_alloc_assoc_set(core, 0) != PQOS_RETVAL_OK) {
            revert_changes();
            return 1;
         } 
      }
      // bitmask
      for (const L3_Cos& cos : l3_cos_vec) {
         if (cos.id == 0) continue;
         std::string new_bitmask = std::string(get_l3_num_ways(), '1');
         l3ca_table[cos.id].u.ways_mask = Misc::to_decimal(new_bitmask); 
      }
      if (pqos_l3ca_set(l3cat_ids[0], get_l3cos_count(), l3ca_table) != PQOS_RETVAL_OK) {
         revert_changes();
         return 2;
      }

      // Restore original assigned cores for cos currently under test
      auto original_cores = l3_cos_vec[cos.id].cores;
      for (const int& core : original_cores) {
         if (pqos_alloc_assoc_set(core, cos.id) != PQOS_RETVAL_OK) {
            revert_changes();
            return 1;
         }
      }

      int curr_num_ways = 1;
      bool min_ways_recorded = false;
      cos_misses_matrix[cos.id] = std::vector<uint64_t>();

      while (curr_num_ways <= num_free_ways) {
         // Update and set bitmask to be tested on class of service (cos)
         std::string curr_bitmask = Autotuna::construct_bitmask_str(get_way_contention_index(), get_l3_num_ways(), curr_num_ways, 0);
         l3ca_table[cos.id].u.ways_mask = Misc::to_decimal(curr_bitmask);
         if (pqos_l3ca_set(l3cat_ids[0], get_l3cos_count(), l3ca_table) != PQOS_RETVAL_OK) {
            revert_changes();
            return 2;
         }

         // Record changes in cache misses for 10 seconds
         int end_point = get_cos_misses_vec(cos.id).size() + 10;
         while (get_cos_misses_vec(cos.id).size() <= end_point) {
            if (!run_thread) {
               revert_changes();
               return 3;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
         }

         // Calculate average misses for latest 10 cache misses
         std::vector<uint64_t> misses_vec = get_cos_misses_vec(cos.id);
         std::vector new_misses_vec(misses_vec.end() - 10, misses_vec.end());
         uint64_t new_misses_sum = std::accumulate(new_misses_vec.begin(), new_misses_vec.end(), static_cast<uint64_t>(0));
         uint64_t misses_average = new_misses_sum / new_misses_vec.size();

         // Record minimum ways needed by cos for misses <= threshold
         if (misses_average <= threshold && !min_ways_recorded) {
            autotuna_min_ways_map[cos.id] = curr_num_ways;
            min_ways_recorded = true;
         }

         // Populate cos_misses_matrix
         cos_misses_matrix[cos.id].push_back(misses_average);
         ++curr_num_ways;
      }
   
      // Check if threshold was never met even with max num_free_ways
      if (!min_ways_recorded) autotuna_min_ways_map[cos.id] = num_free_ways;
   }
   return PQOS_RETVAL_OK;
}

void Pqos::process_autotuna_analysis(int threshold, int root_cos, bool& tuning_feasible, int& depth, int& save_error_code) {
   // Clear previous analysis remains 
   autotuna_min_ways_map.clear();
   cos_misses_matrix.clear();

   // Determine free ways and tunable cos (non cos 0, non Junk/Root or co with no cores assigned)
   int num_free_ways = get_l3_num_ways();
   int num_tunable_cos = get_num_active_cos();

   if (root_cos != -1) {
      const auto root_bitmask = l3_cos_vec[root_cos].bitmask;
      int num_used_ways = std::count(root_bitmask.begin(), root_bitmask.end(), '1');
      num_free_ways -= num_used_ways;
      --num_tunable_cos;
      // check if there are sufficient free ways left for tuning 
      if (num_free_ways <= num_tunable_cos) {
         analysis_completed = true;
         tuning_feasible = false;
         depth = 0;
         return;
      }
      autotuna_min_ways_map[root_cos] = num_used_ways;
   }

   // Save original configuration
   std::string filename = "autotuna_rollback.conf";
   backup_config(filename);

   // Execute analysis
   int scaled_threshold = threshold * 1000;
   std::future<int> analysis = std::async(std::launch::async, &Pqos::run_autotuna_analysis, this, scaled_threshold, root_cos, num_free_ways);

   save_error_code = analysis.get();
   if (save_error_code == PQOS_RETVAL_OK) {
      // Rollback to original configuration
      load_config(filename);
      depth = 0;
   } else {
      save_error_code +=3;
      depth = 2;
   }
   analysis_completed = true;
   tuning_feasible = true;
   return;
}

void Pqos::process_autotuna_tuning(int root_cos, int& depth, int& save_error_code) {
   int total_min_ways = std::accumulate(autotuna_min_ways_map.begin(), autotuna_min_ways_map.end(), 0, [](auto prev_total, auto& map) { return prev_total + map.second; });
   int remaining_ways = get_l3_num_ways() - total_min_ways;

   if (remaining_ways != 0) {
      // scale cos_misses_matrix based on each cos priority rankings
      Autotuna::scaled_data data = Autotuna::scale_misses_matrix(cos_misses_matrix, priority_map);
      // adjust scaled matrix according to each cos' minimum needed ways to prune final calculation
      Autotuna::adjust_misses_matrix_values(data, autotuna_min_ways_map, root_cos, remaining_ways);
      // determine optimal num ways for each cos
      calculate_optimal_ways_combination(data, autotuna_min_ways_map, priority_map, root_cos); 
   }
   
   // Update new bitmask 
   std::vector<unsigned> on_reserve;
   std::string available_bitmask = std::string(get_l3_num_ways(), '0');

   for (const auto&[cos, min_ways] : autotuna_min_ways_map) {
      int pos_0 = static_cast<int>(available_bitmask.find('0'));
      if (pos_0 == 0 && min_ways < 2) {
         on_reserve.push_back(cos);
         continue;
      }
      available_bitmask.replace(pos_0, min_ways, std::string(get_l3_num_ways(), '1'), 0, min_ways);
      l3_cos_vec[cos].new_bitmask = Autotuna::construct_bitmask_str(get_way_contention_index(), get_l3_num_ways(), min_ways, pos_0);  
   } 

   for (const unsigned& cos : on_reserve) {
      int min_ways = autotuna_min_ways_map[cos];
      int pos_0 = static_cast<int>(available_bitmask.find('0'));
      available_bitmask.replace(pos_0, min_ways, std::string(get_l3_num_ways(), '1'), 0, min_ways);
      l3_cos_vec[cos].new_bitmask = Autotuna::construct_bitmask_str(get_way_contention_index(), get_l3_num_ways(), min_ways, pos_0);
   }

   save_error_code = apply_changes();
   if (save_error_code == PQOS_RETVAL_OK) {
      autotuning_completed = true;
   } else {
      save_error_code +=5;
      depth = 2;
   }
}

void Pqos::poll_mon_group() {
   if (monInitialised) {
      if (monReset) {
         pqos_mon_data_vec.clear();
         start_resource_monitoring();
         monReset = false;
      } else {
         // Set the max amount of data points that can be stored
         // poll rate: 1hz
         int max_size = 3600 * 24; // 24 hours

         // Polling one by one as value of cos with no cores are NULL
         for (size_t i=0; i<l3_cos_vec.size(); ++i) {
            auto& cos = l3_cos_vec[i];
            auto& mon = pqos_mon_data_vec[i];

            if (mon != NULL) {
               ret = pqos_mon_poll(&mon, 1);
               if (ret == PQOS_RETVAL_OK) {
                  // evict first element if exceed size
                  if (cos.llc.size() == max_size) cos.llc.erase(cos.llc.begin());
                  if (cos.misses.size() == max_size) cos.misses.erase(cos.misses.begin());
                  // Append llc and misses of each cos
                  cos.llc.push_back(mon->values.llc);
                  cos.misses.push_back(mon->values.llc_misses_delta);
                }
            }
         }
      }
   }
}

int Pqos::close() {
   /* Signal mon thread to stop */
   monInitialised = false;
   /* Reset monitoring */
   pqos_mon_reset();
   /* Shut down PQoS module */
   ret = pqos_fini();
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error closing  PQoS library!" << std::endl;
   } else {
      std::cout << "PQoS library closed!" << std::endl;
      exit_val = EXIT_FAILURE;
   }
   return exit_val;
}

void Pqos::init() {
   /* config */
   memset(&config, 0, sizeof(config));
   config.fd_log = STDOUT_FILENO;
   config.interface = PQOS_INTER_MSR;
   config.verbose = LOG_VER_SUPER_VERBOSE;

   /* PQoS Initialisation */
   ret = pqos_init(&config);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error initialising PQoS Library!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Cache and CPU Capabilities */
   ret = pqos_cap_get(&p_cap, &p_cpu);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error retrieving PQoS capabilities!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /*  L3 Cache Allocation Technology (CAT) from CPU structure */
   l3cat_ids = pqos_cpu_get_l3cat_ids(p_cpu, &l3cat_count);
   if (l3cat_ids == NULL) {
      std::cout << "Error retrieving PQoS L3 Cache Allocation Technology (CAT) ids!" << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Number of L3 allocation classes of service from cap structure*/
   ret = pqos_l3ca_get_cos_num(p_cap, &l3cos_count);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error retrieving number of PQoS l£ allocation classes of service!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Read L3 classes of service from a socket */
   l3ca_table = new struct pqos_l3ca[l3cos_count];
   ret = pqos_l3ca_get(l3cat_ids[0], l3cos_count, &l3num_ca, l3ca_table);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error reading L3 classes of service!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Retreive monitoring capabilities */
   ret = pqos_cap_get_type(p_cap, PQOS_CAP_TYPE_MON, &mon_cap_ptr);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error retrieving QoS monitoring capability!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Event combinations of monitoring events */
   mon_events = (enum pqos_mon_event) 0;
   for (size_t i=0; i < mon_cap_ptr->u.mon->num_events; ++i) {
      mon_events = static_cast<enum pqos_mon_event>(static_cast<int>(mon_events) | static_cast<int>(mon_cap_ptr->u.mon->events[i].type));
   }

   /* Retreive l3ca capabilities */
   ret = pqos_cap_get_type(p_cap, PQOS_CAP_TYPE_L3CA, &l3ca_cap_ptr);
   if (ret != PQOS_RETVAL_OK) {
      std::cout << "Error retrieving L3CA capability!" << std::endl;
      std::cout << pqos_retval_msg(ret) << std::endl;
      exit_val = EXIT_FAILURE;
      Pqos::close();
      return;
   }

   /* Way contention of L3CA */
   way_contention = std::bitset<sizeof(unsigned) * 8>(l3ca_cap_ptr->u.l3ca->way_contention).to_string().substr(sizeof(unsigned) * 8 - get_l3_num_ways());

   /* Per L3 Cos create struct */
   l3_cos_vec.resize(l3cos_count);

   /* Per Core Association */
   unsigned associated_cos; 

   for (size_t core=0; core < get_num_cores(); ++core) {
      pqos_alloc_assoc_get(core, &associated_cos);
      // Add core to its associated cos' struct
      L3_Cos &current_cos = l3_cos_vec[associated_cos];
      current_cos.cores.insert(core);
      current_cos.new_cores.insert(core);
   }

   for (size_t cos=0; cos < l3cos_count; ++cos) {
      L3_Cos &current_cos = l3_cos_vec[cos];
      // class id
      current_cos.id = l3ca_table[cos].class_id;
      // bit mask
      std::string bitmask = std::bitset<sizeof(unsigned) * 8>(l3ca_table[cos].u.ways_mask).to_string().substr(sizeof(unsigned) * 8 - get_l3_num_ways());
      current_cos.bitmask = bitmask;
      current_cos.new_bitmask = bitmask;
      // size = num of set bit * way size
      current_cos.size = std::count_if(bitmask.begin(), bitmask.end(), [&] (char bit) {return bit == '1';}) * get_l3_way_size();
      current_cos.new_size = current_cos.size;
   }
   // tag
   get_cos_tags();
   // process list
   update_processes_vec();

   std::cout << "PQoS Initialisation Success!" << std::endl;
   std::cout << "PQoS init - starting resource monitoring!" << std::endl;
   monInitialised = start_resource_monitoring();
}

