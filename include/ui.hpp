#ifndef CACHETUNA_UI_HPP
#define CACHETUNA_UI_HPP

// std
#include <cstdint>
#include <iomanip> // setprecision
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// ftxui
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"

// cachetuna
#include "pqos_util.hpp"
#include "graph.hpp"
#include "misc.hpp"

// autotuna
#include "braille_generator.hpp"

class UserInterface {
   private:
      std::vector<std::thread> threads;
      bool unexpected_exit;

      /* PQoS */
      Pqos pqos;

      /* ftxui */
      // modal
      int depth;
      int save_error_code;
      ftxui::ButtonOption button_style;
      ftxui::Component get_save_options_menu();
      ftxui::Component get_analyse_options_menu();
      ftxui::Component get_autotuning_options_menu();
      // tab
      int tab_selected;
      std::vector<std::string> tab_values;
      ftxui::Component get_tab_toggle();
      // info
      ftxui::Element cpu_cache_info_hbox();
      // policies Menu options
      int cos_selected;
      ftxui::Component get_l3cos_menu_options();
      ftxui::Element l3cos_menu_options_boxes(bool focused);
      // stats
      ftxui::Element cos_stats_box();
      // tag settings
      ftxui::Component get_tag_selector();
      ftxui::Element tag_window(bool focused);
      // bitmask settings
      int bit_selected;
      ftxui::Component get_bitmask_selector();
      ftxui::Element bitmask_window(bool focused);
      // cores settings
      int core_selected;
      ftxui::Component get_cores_selector();
      ftxui::Element cores_window(bool focused);
      // process list
      int process_selected;
      ftxui::Component get_processes_selector();
      ftxui::Element processes_list(bool focused);
      // auto tuna
      int root_cos;
      int autotuna_cos_selected;
      void update_root_cos();
      // cos performance summary
      ftxui::Component get_perf_summary_selector();
      ftxui::Element perf_summary_window(bool focused);
      // cache misses threshold
      int threshold;
      ftxui::Component get_threshold_slider();
      ftxui::Element threshold_window(ftxui::Component threshold_slider);
      // analysis report
      bool tuning_feasible;
      bool analysis_completed;
      ftxui::Element analysis_report();
      // analyse button 
      ftxui::Component get_analyse_button_selector();
      ftxui::Element analyse_button_texts(bool focused);
      // cos priority
      int priority_cos_selected;
      ftxui::Component get_priority_selector();
      ftxui::Element priority_window(bool focused);
      // key event handler
      bool KeyCallback(bool tag_focused, bool bitmask_focused, bool cores_focused, bool perf_summary_focused, bool priority_focused, ftxui::ScreenInteractive& screen, ftxui::Event& event);
      // thread for updateFrame
      std::atomic<bool> stop_poll_data;
      void poll_data(ftxui::ScreenInteractive &screen);
      // thread for refresh_tuna
      int frame_count;

   public:
      UserInterface();
      void start();
};

#endif //CACHETUNA_UI_HPP
