#ifndef CACHETUNA_PQOS_HPP
#define CACHETUNA_PQOS_HPP

// std
#include <cstring> // memset
#include <fstream> // ifstream, outfile
#include <iostream> // cout
#include <ostream> // endl
#include <sstream> // stringstream
#include <future>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>

// PQoS
#include "pqos.h"
#include "log.h"

// cachetuna
#include "misc.hpp" 

// autotuna
#include "autotuna.hpp"

struct L3_Cos {
   unsigned id;
   bool unsaved_changes = false;
   // original settings
   unsigned size;
   std::string tag;
   std::string bitmask;
   std::set<int> cores;
   // new settings
   unsigned new_size;
   std::string new_tag;
   std::string new_bitmask;
   std::set<int> new_cores;
   // monitoring data
   std::vector<uint64_t> llc;
   std::vector<uint64_t> misses;
   std::vector<std::string> processes; // list of process running on cores
};

class Pqos {
   private:
      struct pqos_config config;
      const struct pqos_cpuinfo *p_cpu;
      const struct pqos_cap *p_cap;
      unsigned l3cat_count;
      unsigned l3cos_count;
      unsigned *l3cat_ids;
      unsigned l3num_ca;
      struct pqos_l3ca *l3ca_table;
      const struct pqos_capability *mon_cap_ptr;
      const struct pqos_capability *l3ca_cap_ptr;
      int ret, exit_val;
      enum pqos_mon_event mon_events;
      std::vector<struct L3_Cos> l3_cos_vec;
      void update_processes_vec();
      std::vector<struct pqos_mon_data*> pqos_mon_data_vec;
      bool start_resource_monitoring();
      std::set<int> non_contiguous_cos_set; // list of cos with unsaved bit assoc that are non-contiguous
      std::string way_contention;
      // autotuna
      int priority_count;
      std::map<unsigned, int> priority_map;
      std::map<unsigned, int> autotuna_min_ways_map;
      std::map<unsigned, std::vector<uint64_t>> cos_misses_matrix;

   public:
      Pqos();
      bool get_l3_detected();
      unsigned get_num_cores();
      unsigned get_l3_num_ways();
      unsigned get_l3_way_size();
      unsigned get_l3_total_size();
      unsigned get_l3_num_sets();
      unsigned get_l3_num_partitions();
      unsigned get_l3_line_size();
      unsigned get_l3cos_count();
      int get_num_active_cos();
      std::string get_way_contention();
      std::pair<int, int> get_way_contention_index();
      std::vector<struct L3_Cos> get_l3_cos_vec();
      std::vector<uint64_t> get_cos_llc_vec(int cos);
      std::vector<uint64_t> get_cos_misses_vec(int cos);
      std::set<int> get_bit_assoc(int bit);
      int get_core_assoc(int core);
      std::vector<unsigned> get_mon_data_index_vec();
      std::vector<std::vector<uint64_t>> get_all_llc_vec();
      std::vector<std::vector<uint64_t>> get_all_misses_vec();
      void get_cos_tags();
      int close();
      void init();
      void poll_mon_group();
      bool monInitialised;
      bool run_thread;
      bool monReset;
      void update_new_tag(const std::string& new_tag, int cos);
      void update_new_bitmask(int bit_selected, int cos);
      void update_new_cores(int core_selected, int cos);
      void revert_changes();
      int apply_changes();
      void backup_config(const std::string& file_name);
      int load_config(const std::string& file_name);
      // AutoTuna
      bool analysis_completed;
      bool autotuning_completed;
      void init_priority_map_key_val(int cos);
      void update_priority_rank(int cos_index);
      std::map<unsigned, int> get_priority_map();
      std::map<unsigned, int> get_autotuna_min_ways_map();
      std::map<unsigned, std::vector<uint64_t>> get_cos_misses_matrix();
      void process_autotuna_analysis(int threshold, int root_cos, bool& tuning_feasible, int& depth, int& save_error_code);
      int run_autotuna_analysis(int threshold, int root_cos, int num_free_ways);
      void process_autotuna_tuning(int root_cos, int& depth, int& save_error_code);
};

#endif //cachetuna_pqos_hpp
