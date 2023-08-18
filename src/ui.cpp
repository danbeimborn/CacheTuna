#include "ui.hpp"

using namespace ftxui;

UserInterface::UserInterface():
   depth(0),
   cos_selected(0),
   tab_selected(0),
   bit_selected(0),
   core_selected(0),
   process_selected(0),
   autotuna_cos_selected(0),
   threshold(10),
   root_cos(-1),
   priority_cos_selected(0),
   stop_poll_data(false),
   tuning_feasible(false),
   frame_count(0),
   button_style(ButtonOption::Animated()),
   pqos(Pqos())
{
   pqos.init();
   std::ifstream exit_file(Misc::get_executable_path() + "unexpected_exit.conf");
   unexpected_exit = exit_file.good();
   if (!unexpected_exit) pqos.backup_config("unexpected_exit.conf");
}

Component UserInterface::get_tab_toggle() {
   tab_values = {"CacheTuna", "AutoTuna"};
   return Toggle(&tab_values, &tab_selected);
}

Element UserInterface::cpu_cache_info_hbox() {
   // CPU info
   std::vector<std::string> cpu_info = Misc::get_cpu_info();
   Elements cpu_info_elements;

   for (const auto& line : cpu_info) {
      cpu_info_elements.push_back(text(" " + line));
   }

   Element cpu_info_box = vbox({
         text("CPU Info") | hcenter | bold | color(Color::Blue),
         separator(),
         vbox({
               std::move(cpu_info_elements)
               }),
         });

   // Cache info
   std::string l3_detected = pqos.get_l3_detected() ? "True" : "False";
   std::string l3_num_ways = std::to_string(pqos.get_l3_num_ways());
   std::string l3_way_size = Misc::format_bytes(pqos.get_l3_way_size());
   std::string l3_total_size = Misc::format_bytes(pqos.get_l3_total_size());
   std::string l3_num_sets = std::to_string(pqos.get_l3_num_sets());
   std::string l3_num_partitions = std::to_string(pqos.get_l3_num_partitions());
   std::string l3_line_size = Misc::format_bytes(pqos.get_l3_line_size());

   Element cache_info_box = vbox({
         text("L3 Cache Info") | hcenter | bold | color(Color::Blue),
         separator(),
         hbox({
               vbox({
                     text(" Detected:"),
                     text(" Num cache ways:"),
                     text(" Cache way:"),
                     text(" Total size:"),
                     text(" Num sets:"),
                     text(" Partitions:"),
                     text(" Cache line size:"),
                     }),
               separatorEmpty(),
               vbox({
                     text(l3_detected),
                     text(l3_num_ways),
                     text(l3_way_size),
                     text(l3_total_size),
                     text(l3_num_sets),
                     text(l3_num_partitions),
                     text(l3_line_size),
                     }),
               }),
   });

   return hbox({
         cpu_info_box | size(WIDTH, EQUAL, int(Terminal::Size().dimx * 0.5)),
         separator(),
         cache_info_box | size(WIDTH, EQUAL, int(Terminal::Size().dimx * 0.5)),
         });
}

Component UserInterface::get_l3cos_menu_options() {
   Components options;
   for (const auto& cos : pqos.get_l3_cos_vec()) {
      options.push_back(MenuEntry(""));
   }
   return Container::Horizontal(options, &cos_selected) | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::l3cos_menu_options_boxes(bool tab_focused) {  
   auto slider = [&]() -> Element {
      int section_size = floor(Terminal::Size().dimx / pqos.get_l3cos_count()); // amount of _ of each section
      int start_range = section_size * cos_selected;
      int end_range = start_range + section_size + 3;
      Elements slider;
      for (size_t i=0; i<Terminal::Size().dimx; ++i) {
         if (i > start_range && i < end_range) {
            slider.push_back(text("▂"));
         }
         else {
            slider.push_back(text("_"));
         }
      }
      return hbox({std::move(slider)});
   };

   const auto& cos_vec = pqos.get_l3_cos_vec();
   int num_cos = pqos.get_l3cos_count();
   Elements options;

   // horizontal scroll - only show 3 max
   int start = cos_selected - 1;
   int end = cos_selected + 1;
   int index = 1;

   if (start <= 0 || num_cos <= 3) {
      start = 0;
      end = 2;
      index = cos_selected;
   }
   else if (end > num_cos - 1) {
      start = num_cos - 3;
      end = num_cos - 1;
      index = cos_selected - start;
   }

   for (size_t i=start; i<=end; ++i) {
      const auto& cos = cos_vec[i];

      // Color scheme when focused
      ftxui::Color text_color, bg_color;
      if (cos_selected == i && !tab_focused) {
         text_color = Color::Black;
         if (cos_selected == 0) { // Cores assigned to Cos0 (default) means they're not pinned to any active cos
            bg_color = Color::GrayDark;
         }
         else if (cos.cores.empty()) { // Cos has no cores assigned
            bg_color = Color::Wheat1;
         }
         else{
            bg_color = Color::Blue;
         }
      }

      Color id_color = bg_color;
      std::string id = " Cos " + std::to_string(cos.id);
      if (cos.unsaved_changes) {
         id_color = Color::Orange1;
         id += " (Unsaved Changes)";
      }

      std::string tag = cos.id == 0 ? "Default" : cos.tag;
      std::string cores = Misc::to_range_extraction(cos.cores);
      tag = tag.size() > 41 ? tag.substr(tag.size()-41, 41) : tag;
      cores = cores.size() > 41 ? cores.substr(cores.size()-41, 41) : cores;

      Element info =
            vbox({
               separatorEmpty(),
               text(id) | color(id_color) | hcenter,
               separatorLight(),
               hbox({
                     vbox({
                           text(" Tag:"),
                           text(" Bitmask:"),
                           text(" Cos Size:"),
                           text(" Cores Assigned:"),
                           }),
                     separatorEmpty(),
                     vbox({
                           text(tag),
                           text(cos.bitmask),
                           text(Misc::format_bytes(cos.size)),
                           text(Misc::to_range_extraction(cos.cores)),
                           }) | xflex_grow
               }) | color(text_color) | bgcolor(bg_color)
            });
      info |= size(WIDTH, EQUAL, int(Terminal::Size().dimx * 0.5));

      options.push_back(info);
      options.push_back(separatorDouble());
   }

   Element option_box = hbox({std::move(options)});
   return vbox({slider(), option_box});
}

Element UserInterface::cos_stats_box() {
   auto llc_percentage = [&] (const uint64_t& val) -> std::string {
      std::stringstream res;
      res << "[" << std::setprecision(2) <<  (float) val / pqos.get_l3_total_size() * 100 <<  "%] ";
      return res.str();
   };

   std::vector<uint64_t> llc_vec = pqos.get_cos_llc_vec(cos_selected);
   std::vector<uint64_t> misses_vec = pqos.get_cos_misses_vec(cos_selected);

   // llc
   Element llc, llc_average, llc_peak;
   if (llc_vec.empty()) {
      llc = text("N/A");
      llc_average = text("N/A");
      llc_peak = text("N/A");
   }
   else {
      uint64_t llc_val = llc_vec.back();
      llc = hbox({
               text(Misc::format_bytes(llc_val)),
               separatorEmpty(),
               text(llc_percentage(llc_val)),
            });

      uint64_t llc_sum = std::accumulate(llc_vec.begin(), llc_vec.end(), static_cast<uint64_t>(0));
      uint64_t llc_average_val = llc_sum / llc_vec.size();
      llc_average = hbox({
            text(Misc::format_bytes(llc_average_val)),
            separatorEmpty(),
            text(llc_percentage(llc_average_val))
            });

      uint64_t llc_peak_val = *std::max_element(llc_vec.begin(), llc_vec.end());
      llc_peak = hbox({
            text(Misc::format_bytes(llc_peak_val)),
            separatorEmpty(),
            text(llc_percentage(llc_peak_val))
            });
   }

   // misses
   Element misses, misses_average, misses_peak;
   if (misses_vec.empty()) {
      misses = text("N/A");
      misses_average = text("N/A");
      misses_peak = text("N/A");
   }
   else {
      misses = text(Misc::format_misses(misses_vec.back()));

      uint64_t misses_sum = std::accumulate(misses_vec.begin(), misses_vec.end(), static_cast<uint64_t>(0));
      misses_average = text(Misc::format_misses(misses_sum / misses_vec.size()));

      uint64_t misses_max_element = *std::max_element(misses_vec.begin(), misses_vec.end());
      misses_peak = text(Misc::format_misses(misses_max_element));
   }

   // stat box
   return vbox({
         hbox({
               vbox({
                     text(" LLC: "),
                     text(" average: "),
                     text(" peak: "),
                     }),
               separatorEmpty(),
               vbox({
                     llc,
                     llc_average,
                     llc_peak,
                     })
               }),
         separator(),
         hbox({
               vbox({
                     text(" MISSES: "),
                     text(" average: "),
                     text(" peak: "),
                     }),
               separatorEmpty(),
               vbox({
                     misses,
                     misses_average,
                     misses_peak,
                     })
               }),
         }) | border;
}

Component UserInterface::get_tag_selector() {
   std::vector<Component> entries;
   entries.push_back(MenuEntry(""));
   return Container::Horizontal(entries, 0) | size(HEIGHT, EQUAL, 0);
}

Component UserInterface::get_bitmask_selector() {
   std::vector<Component> entries;
   for (size_t i=0; i<pqos.get_l3_num_ways(); ++i) {
      entries.push_back(MenuEntry(""));
   } 
   return Container::Horizontal(entries, &bit_selected) | size(HEIGHT, EQUAL, 0);
}

Component UserInterface::get_cores_selector() {
   std::vector<Component> entries;
   for (size_t i=0; i<pqos.get_num_cores(); ++i) {
      entries.push_back(MenuEntry(""));
   } 
   return Container::Vertical(entries, &core_selected) | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::tag_window(bool focused) {
   Elements tag_elements;
   if (cos_selected == 0) {
      tag_elements.push_back(text(" Default (can't change)"));
   }
   else {
      std::string new_tag = pqos.get_l3_cos_vec()[cos_selected].new_tag;
      new_tag = new_tag.size() > 24 ? new_tag.substr(new_tag.size()-24, 24) : new_tag; // trim tag per max char shown
      tag_elements.push_back(text(new_tag));
   }

   if (focused) {
      // cursor
      if (cos_selected == 0) {
         tag_elements.push_back(text("▌") | color(Color::GrayDark) | blink);
      } else {
         tag_elements.push_back(text("▌") | color(Color::Blue) | blink);
      }
   }
   return window(text("Tag"), hbox({std::move(tag_elements)}));
}

Element UserInterface::bitmask_window(bool focused) {
   auto color_code = [&](const int& state) -> Decorator {
      Color highlight;
      switch (state) {
         case 1:
            if (cos_selected == 0) highlight = Color::GrayDark;
            else highlight = Color::Blue;
            break;
         case 10:
            highlight = Color::RedLight;
            break;
         case 11:
            highlight = Color::Red;
            break;
         default:
            highlight = Color::White;
      }
      return color(highlight);
   };

   std::string bitmask = pqos.get_l3_cos_vec()[cos_selected].new_bitmask;

   std::vector<Element> bitmask_texts;
   bool overlap = false;

   for (size_t i=0; i<bitmask.size(); ++i) {
      std::set<int> bit_assoc = pqos.get_bit_assoc(i);
      std::string symbol = bitmask[i] == '1' ? "▣" : "☐";

      bitmask_texts.push_back(text(symbol));

      int state = 0;
      if (focused && bit_selected == i) ++state; // check if bit selected
      if (cos_selected != 0 && bitmask[i] == '1' && pqos.get_l3_cos_vec()[cos_selected].new_cores.size() > 0 && bit_assoc.size() > 1 ) {
         state += 10;
         overlap = true;
      }
      bitmask_texts[i] |= color_code(state);
   }
   std::vector<Element> bitmask_box;
   bitmask_box.push_back(hbox({bitmask_texts}));

   // Validity Check
   // Error - Non-contiguous
   if (Misc::not_contiguous(bitmask)) {
      Element error = text(" ERROR: not contiguous");
      error |= color(Color::Red);
      bitmask_box.push_back(error);
   }
   // Warning - Overlapped bit
   if (overlap) {
      Element warning = text(" Warning: bit overlapped");
      warning |= color(Color::Red);
      bitmask_box.push_back(warning);
   }

  return window(text("Bitmask"), vbox({bitmask_box}));
   
}

Element UserInterface::cores_window(bool focused) {
   auto core_text = [&](int core) -> Element {
      unsigned cos = pqos.get_core_assoc(core);
      if (cos == cos_selected) return text(" ▣ Core " + std::to_string(core));
      if (cos == 0) return text(" ☐ Core " + std::to_string(core)); // core pinned to Cos0 (default) if not pinned to any active cos
      else return text(" Core " + std::to_string(core) + " - Used by Cos " + std::to_string(cos)) | color(Color::GrayDark);

   };

   const std::set<int>& cores = pqos.get_l3_cos_vec()[cos_selected].cores;
   Elements cores_texts;

   // only show 5 max
   int start = core_selected - 2;
   int end = core_selected + 2;
   int index = 2;
   int num_cores = pqos.get_num_cores();

   if (start <=0 || num_cores <= 5) {
      start = 0;
      end = num_cores > 5 ? 4 : num_cores - 1;
      index = core_selected;
   }
   else if (end > num_cores -1) {
      start = num_cores - 5;
      end = num_cores - 1;
      index = core_selected - start;
   }

   for (size_t i=start; i<=end; ++i) {
      cores_texts.push_back(core_text(i));
   }

   if (focused) {
      unsigned cos = pqos.get_core_assoc(core_selected);
      if (cos_selected != 0 && (cos == cos_selected || cos == 0)) {
         cores_texts[index] |= bgcolor(Color::Blue);
      } else {
         cores_texts[index] |= bgcolor(Color::GrayLight);
      }
   }
         
   return window(text("Cores"), vbox({std::move(cores_texts)}));
}

Component UserInterface::get_processes_selector() {
   const int& process_count = std::stoi(Misc::run_cmd("ps -Al | wc -l")[0]);
   std::vector<Component> entries;
   for (size_t i=0; i<process_count; ++i) {
      entries.push_back(MenuEntry(""));
   } 
   return Container::Vertical(entries, &process_selected) | size(WIDTH, EQUAL, 0) | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::processes_list(bool focused) {
   int terminal_height = (Terminal::Size().dimy * 0.5) - 8; // minus process list titles and separators
   const std::vector<std::string> processes = pqos.get_l3_cos_vec()[cos_selected].processes;
   int process_count = processes.size();

   if (process_count == 0) {
      process_selected = 0;
   }
   else if (process_selected >= process_count) {
      process_selected = process_count - 1;
   }

   int start = process_selected - ((terminal_height / 2) - 1);
   int end = process_selected + (terminal_height / 2);
   int index = terminal_height / 2;

   if (start <= 0 || process_count < terminal_height) {
      start = 0;
      end = process_count > terminal_height ? terminal_height : process_count;
      index = process_selected;
   }
   else if (end > process_count - 1) {
      start = process_count - (terminal_height / 2);
      end = process_count;
      index = process_selected - start;
   }

   Elements process_texts;
   // processes
   if (process_count > 0) {
      for (size_t i=start; i<end; ++i) {
         process_texts.push_back(text(processes[i]));
      }
   } else {
         process_texts.push_back(text("No process running") | hcenter);
   }
   if (focused) {
      if (process_count > 0) {
         index = index > process_texts.size() - 1 ? process_texts.size() - 1 : index;
      } else {
         index = 0;
      }
      process_texts[index] |= color(Color::Black);
      process_texts[index] |= bgcolor(Color::Green);
   }

   // header
   const std::string header = Misc::run_cmd("ps ao user,pid,pcpu,pmem,psr,stat,start,time,command | head -1")[0];
   process_texts.insert(process_texts.begin(), separatorLight());
   process_texts.insert(process_texts.begin(), text(header) | color(Color::Black) |  bgcolor(Color::Yellow));
   process_texts.insert(process_texts.begin(), separatorLight());
   process_texts.insert(process_texts.begin(), text("Process List") | hcenter);
         
   return vbox({std::move(process_texts)}) | size(WIDTH, EQUAL, int(Terminal::Size().dimx * 0.4));
}

void UserInterface::poll_data(ScreenInteractive &screen) {
   while (pqos.monInitialised && pqos.run_thread) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      pqos.poll_mon_group();
      screen.PostEvent(Event::Custom);
   }
}

Component UserInterface::get_save_options_menu() {
   auto update_depth = [](int retval, int& depth, int& save_error_code) {
      if (retval == PQOS_RETVAL_OK) {
         depth = 0;
         save_error_code = 0;
      } else {
         depth = 2;
         save_error_code = retval;
      }
         save_error_code = retval;
      return;
   };

   auto apply_and_save_changes = [&] {
      update_depth(pqos.apply_changes(), depth, save_error_code);
      if (!unexpected_exit) pqos.backup_config("unexpected_exit.conf");
      return;
   };

   auto load_backup_config = [&] {
      update_depth(pqos.load_config("backup.conf"), depth, save_error_code);
      return;
   };

   auto load_unexpected_exit_backup_config = [&] {
      update_depth(pqos.load_config("unexpected_exit.conf"), depth, save_error_code);
      return;
   };

   Components buttons;
   buttons.push_back(Button("Apply and Save Changes", apply_and_save_changes, button_style));

   std::ifstream backup_file(Misc::get_executable_path() + "backup.conf");
   if (backup_file.good()) buttons.push_back(Button("Load Backup Config", load_backup_config, button_style));
   if (unexpected_exit) buttons.push_back(Button("Load Unexpected Exit Backup Config", load_unexpected_exit_backup_config, button_style));
   buttons.push_back(Button("Back", [&] {depth=0;}, button_style));
   return Container::Vertical({std::move(buttons)});
}

Component UserInterface::get_analyse_options_menu() {
   auto process_autotuna_analysis = [&] {
      threads.push_back(std::thread(&Pqos::process_autotuna_analysis, std::ref(pqos), threshold, root_cos, std::ref(tuning_feasible), std::ref(depth), std::ref(save_error_code)));
      depth=4;
      return;
   };

   Components buttons;
   buttons.push_back(Button("Cancel", [&] {depth=0;}, button_style));
   buttons.push_back(Button("Continue", process_autotuna_analysis, button_style));
   return Container::Vertical({std::move(buttons)});
}

Component UserInterface::get_autotuning_options_menu() {
   auto process_autotuna_tuning = [&] {
      depth=0;
      pqos.process_autotuna_tuning(root_cos, depth, save_error_code);
      return;
   };

   Components buttons;
   buttons.push_back(Button("Cancel", [&] {depth=0;}, button_style));
   buttons.push_back(Button("Continue", process_autotuna_tuning, button_style));
   return Container::Vertical({std::move(buttons)});
}

Component UserInterface::get_perf_summary_selector() {
   std::vector<Component> entries;
   for (const auto& cos : pqos.get_l3_cos_vec()) {
      entries.push_back(MenuEntry(""));
   }
   return Container::Vertical(entries, &autotuna_cos_selected) | size(WIDTH, EQUAL, 0) | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::perf_summary_window(bool focused) {
   // header
   Elements header;
   header.push_back(text("Policy"));
   header.push_back(text("Core"));
   header.push_back(text("Bitmasks"));
   header.push_back(text("Size"));
   header.push_back(text("Avg LLC"));
   header.push_back(text("Avg Misses"));
   header.push_back(text("Status"));
   header.push_back(text("Junk/Root"));
   for (auto& text : header) {
      text |= hcenter;
      text |= bgcolor(Color::Blue);
      text |= size(WIDTH, EQUAL, Terminal::Size().dimx * 0.15);
   }

   // Options
   // only show 3 max
   int start = autotuna_cos_selected - 1;
   int end = autotuna_cos_selected + 1;
   int num_cos = pqos.get_l3cos_count();
   int index = 1;
   if (start <=0 || num_cos <= 3) {
      start = 0;
      end = num_cos > 3 ? 2 : num_cos - 1;
      index = autotuna_cos_selected;
   }
   else if (end > num_cos -1) {
      start = num_cos - 3;
      end = num_cos - 1;
      index = autotuna_cos_selected - start;
   }

   Elements options;
   auto l3cos_vec = pqos.get_l3_cos_vec();
   int scaled_threshold = threshold * 1000; // convert thresold from tens to thousands
   for (size_t i=start; i<=end; ++i) {
      auto cos = l3cos_vec[i];
      uint64_t llc_average, misses_average;
      if (!cos.llc.empty()) {
         llc_average = std::accumulate(cos.llc.begin(), cos.llc.end(), static_cast<uint64_t>(0)) / cos.llc.size();
      } else { llc_average = 0;}
      if (!cos.misses.empty()) {
         misses_average = std::accumulate(cos.misses.begin(), cos.misses.end(), static_cast<uint64_t>(0)) / cos.misses.size();
      } else { misses_average = 0;}

      Elements option_texts;
      option_texts.push_back(text(std::to_string(cos.id)));
      option_texts.push_back(text(Misc::to_range_extraction(cos.cores)));
      option_texts.push_back(text(cos.bitmask));
      option_texts.push_back(text(Misc::format_bytes(cos.size)));
      option_texts.push_back(text(Misc::format_bytes(llc_average)));
      option_texts.push_back(text(Misc::format_misses(misses_average)));
      if (cos.cores.empty()) {
         option_texts.push_back(text("N/A")); // status
         option_texts.push_back(text("")); // Junk/root
      } else {
         if (cos.id == 0) {
            option_texts.push_back(text("Cores Error") | color(Color::Red));
         }
         else if (llc_average/cos.size >= 1.0 || misses_average >= scaled_threshold) {
            option_texts.push_back(text("High") | color(Color::Red));
         }
         else if (llc_average/cos.size >= 0.85 || misses_average >= std::max(0, scaled_threshold-1000)) {
            option_texts.push_back(text("Limit") | color(Color::Yellow));
         }
         else {
            option_texts.push_back(text("Good") | color(Color::Green));
         }
         if (cos.id == 0) {
            option_texts.push_back(text("")); // Junk/root
         } else if (cos.id == root_cos) {
            option_texts.push_back(text("▣ ")); // Junk/root
         } else {
            option_texts.push_back(text("☐")); // Junk/root
         }
      }
      for (auto& text : option_texts) {
         text |= hcenter;
         text |= size(WIDTH, EQUAL, Terminal::Size().dimx * 0.145);
      }

      Element option = hbox({std::move(option_texts)});
      if (focused && i == autotuna_cos_selected) {
         if (cos.cores.empty()) {
            option |= bgcolor(Color::GrayDark);
         }
         else if (cos.id != root_cos){
            option |= bgcolor(Color::Blue);
         }
      }
      if (cos.id == root_cos) {
        option |= color(Color::Black);
        option |= bgcolor(Color::Yellow);
      }
      options.push_back(separatorEmpty());
      options.push_back(option);
   }

   return vbox({
         text("Cos Performance Summary") | hcenter | color(Color::Blue),
         separator(),
         hbox({std::move(header)}),
         vbox({std::move(options)})
         }) | border;
}

void UserInterface::update_root_cos() {
   if (autotuna_cos_selected == root_cos) {
      root_cos = -1;
   }
   else if (!pqos.get_l3_cos_vec()[autotuna_cos_selected].cores.empty()) {
      root_cos = autotuna_cos_selected;
   }
}

Component UserInterface::get_threshold_slider() {
   return Slider("", &threshold, 0, 100, 2);
}

Element UserInterface::threshold_window(Component threshold_slider) {
   return vbox({ 
          window(text("Cache Misses Threshold"), threshold_slider->Render()),
          text(std::to_string(threshold) + "k") | hcenter
          }) | xflex_grow;
}

Component UserInterface::get_priority_selector() {
   Component entries = Container::Vertical({}, &priority_cos_selected);
   for (const auto& cos : pqos.get_l3_cos_vec()) {
      if (cos.id == 0) continue;
      entries->Add(MenuEntry(std::to_string(cos.id)));
      pqos.init_priority_map_key_val(cos.id);
   }
   return entries | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::priority_window(bool focused) {
   Elements priority_texts;
 
   // Only show 5 max
   int start = priority_cos_selected - 2;
   int end = priority_cos_selected + 2;
   int index = 2;
   int len_priority_map = pqos.get_priority_map().size();

   if (start <=0 || len_priority_map <= 5) {
      start = 0;
      end = len_priority_map > 5 ? 4 : len_priority_map - 1;
      index = priority_cos_selected;
   }
   else if (end > len_priority_map - 1) {
      start = len_priority_map - 5;
      end = len_priority_map - 1;
      index = priority_cos_selected - start;
   }

   for (size_t i=start; i<=end; ++i) {
      Element item;
      unsigned cos = i + 1;
      int rank = pqos.get_priority_map()[cos];
      auto cores = pqos.get_l3_cos_vec()[cos].cores;
      if (cos == root_cos || cores.empty()) { // ignore Junk/Root and cos with no cores assigned for tuning
         item = text(" Cos " + std::to_string(cos)) | color(Color::GrayDark);
      } else if (rank == 0) {
         item = text(" ☐ Cos " + std::to_string(cos));
      } else {
         item = text(" ▣ Cos " + std::to_string(cos) + " - rank " + std::to_string(rank));
      }

      if (focused && i == priority_cos_selected) {
         item |= bgcolor(Color::Blue);
      }
      priority_texts.push_back(item);
   }
   return window(text("Set Priority"), vbox({std::move(priority_texts)}));
}

Component UserInterface::get_analyse_button_selector() {
   Component entry = Container::Vertical({Button("", [&] {depth=3;})});
   return entry | size(WIDTH, EQUAL, 0) | size(HEIGHT, EQUAL, 0);
}

Element UserInterface::analyse_button_texts(bool focused) {
   std::vector<Element> button_texts;
   
   if (pqos.get_l3_num_ways() >= pqos.get_num_active_cos() && pqos.get_num_active_cos() != 0) {
      button_texts.push_back(text("╭───────╮"));
      button_texts.push_back(text("│Analyse│"));
      button_texts.push_back(text("╰───────╯"));
   } else {
      button_texts.push_back(text("╭──────────────────────────────────────────────────╮"));
      button_texts.push_back(text("│Cannot autotune (insufficient ways/no active cos)!│"));
      button_texts.push_back(text("╰──────────────────────────────────────────────────╯"));
      depth = 0;
   }

   if (focused) {
      for (auto& item: button_texts) {
         item |= color(Color::Black);
         item |= bgcolor(Color::White);
      }
   } 
   return vbox({std::move(button_texts)});
}

Element UserInterface::analysis_report() {
   Elements result;
   if (tuning_feasible) {
      Elements cos_result;
      std::map<unsigned, int> min_ways_map = pqos.get_autotuna_min_ways_map(); 
      std::map<unsigned, std::vector<uint64_t>> misses_matrix = pqos.get_cos_misses_matrix();
      for (const auto&[cos, min_ways]: min_ways_map) {
         std::string cos_str = "Cos " + std::to_string(cos);
         std::string ways_str = std::to_string(min_ways) + " ways";
         std::string misses_str = (cos == root_cos) ? "Junk/Root" : Misc::format_misses(misses_matrix[cos][min_ways-1]);
         cos_result.push_back(text(cos_str));
         cos_result.push_back(text(ways_str));
         cos_result.push_back(text(misses_str));

         for (auto& text : cos_result) {
            text |= hcenter;
            text |= size(WIDTH, EQUAL, Terminal::Size().dimx * 0.4);
         }

         result.push_back(hbox({std::move(cos_result)}));
      }
      if (!pqos.autotuning_completed) {
         int total_min_ways = std::accumulate(min_ways_map.begin(), min_ways_map.end(), 0, [](auto prev_total, auto& map) { return prev_total + map.second; }); 
         Element total_min_ways_text = text("Total minimum needed ways: " + std::to_string(total_min_ways));
         Element total_avail_ways_text = text("Total available cache ways: " + std::to_string(pqos.get_l3_num_ways())); 
         Element info_text = (total_min_ways > pqos.get_l3_num_ways()) ? 
                              text("Not enough cache ways for optimal config, please set priorities!") | color(Color::Yellow) :
                              text("Enough cache ways available for optimal config!") | color(Color::Green);
         result.push_back(separator());
         result.push_back(total_min_ways_text);
         result.push_back(total_avail_ways_text);
         result.push_back(info_text);
      }
   } 
   else {
      result.push_back(text("Insufficient number of cache ways for tuning."));
      result.push_back(text("Please check if Junk/Root is taking up most of the cache ways!"));
   }
   return vbox({
               text("Optimal Configuration:") | hcenter,
               vbox({std::move(result)})
              }) | border; 
}

bool UserInterface::KeyCallback(bool tag_focused, bool bitmask_focused, bool cores_focused, bool perf_summary_focused, bool priority_focused, ScreenInteractive& screen, Event& event) {

   /* Key Logging */
   std::string key;
   for (auto& it: event.input()) {
      key += std::to_string((unsigned int)it);
   }

   /* escape handler */
   if (event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
   }
   /* backspace and characters input handler - tag window */
   if (tag_focused && (event.is_character() || key == "8") && cos_selected != 0) {
      std::string tag = pqos.get_l3_cos_vec()[cos_selected].new_tag;
      if (event.is_character()) {
         tag += event.character()[0];
      }
      else if (key == "8" && !tag.empty()) {
         tag.pop_back();
      }
      pqos.update_new_tag(tag, cos_selected);
   }

   /* return handler - bitmask, cores windows, root_cos & priority selector */
   int priority_cos = priority_cos_selected + 1;
   auto cores = pqos.get_l3_cos_vec()[priority_cos].cores;
   if (event == Event::Return) {
      if (perf_summary_focused) {
         update_root_cos();
      }
      else if (priority_focused && priority_cos != root_cos) {
         if (!cores.empty()) pqos.update_priority_rank(priority_cos);
      }
      else if (cos_selected != 0) {
         if (bitmask_focused) { 
            pqos.update_new_bitmask(bit_selected, cos_selected);
         }
         else if (cores_focused) {
            pqos.update_new_cores(core_selected, cos_selected);
         }
      }
      return false;
   }

   /* S key handler - change depth to toggle Save Options Modal */
   if (tab_selected == 0 && !tag_focused && event.is_character() && event.character()[0] == 's') {
      switch (depth) {
         case 0:
            depth = 1;
            break;
         case 1:
            depth = 0;
            break;
         return false;
      }
   }

   return false;
}

void UserInterface::start() {
   // Tab toggle
   auto tab_toggle = get_tab_toggle();

   // CacheTuna tab
   Graph llc_bar_graph("LLC", "llc");
   Graph misses_bar_graph("MISSES", "misses");

   Component l3cos_menu_options = get_l3cos_menu_options();
   Component tag_selector = get_tag_selector();
   Component bitmask_selector = get_bitmask_selector();
   Component cores_selector = get_cores_selector();
   Component processes_selector = get_processes_selector();

   auto cachetuna_components = Container::Vertical({
         l3cos_menu_options,
         Container::Horizontal({
               Container::Vertical({
                     tag_selector,
                     bitmask_selector,
                     cores_selector,
                     }),
               processes_selector, 
               }),
         });

   auto cachetuna_renderer = Renderer(cachetuna_components, [&] {
         return vbox({
               cpu_cache_info_hbox(), // cpu and cache info
               separator(),
               vbox({ // CoS options
                     text("Policies") | hcenter | bold | color(Color::Blue),
                     separator(),
                     l3cos_menu_options->Render(), // hidden
                     l3cos_menu_options_boxes(tab_toggle->Focused()), // shown
                     }),
               separator(),
               hbox({ // CoS Info
                     vbox({
                           // stats
                           cos_stats_box(),
                           // settings
                              // tag
                           tag_selector->Render(),
                           tag_window(tag_selector->Focused()),
                              // bitmask
                           bitmask_selector->Render(),
                           bitmask_window(bitmask_selector->Focused()),
                              // cores
                           cores_selector->Render(),
                           cores_window(cores_selector->Focused()),
                           })
                           | size(WIDTH, EQUAL, int(Terminal::Size().dimx * 0.15)),
                     hbox({
                           // Graphs 
                           hbox({
                              llc_bar_graph.get_graph(pqos.get_l3_cos_vec()[cos_selected].llc) | xflex_grow,
                              separator(),
                              misses_bar_graph.get_graph(pqos.get_l3_cos_vec()[cos_selected].misses) | xflex_grow,
                              }) | xflex_grow,
                           separator(),
                           // Process list
                           processes_selector->Render(),
                           processes_list(processes_selector->Focused()),
                           })
                     | border 
                     | xflex_grow 
                     | size(HEIGHT, EQUAL, int(Terminal::Size().dimy * 0.5))
                     })
               });
         });

   // AutoTuna tab
   Graph llc_line_plot("LLC Usage", "llc");
   Graph misses_line_plot("Cache Miss Rate", "misses");

   Component perf_summary_selector = get_perf_summary_selector();
   Component threshold_slider = get_threshold_slider();
   Component priority_selector = get_priority_selector();
   Component analyse_button_selector = get_analyse_button_selector();

   Component autotuning_button = Button("Auto-Tune", [&] {depth=5;}, ButtonOption::Border());
   auto reset_autotuning = [&] {
         std::filesystem::remove(Misc::get_executable_path() + "autotuna_rollback.conf"); 
         tuning_feasible = false;
         pqos.analysis_completed = false;
         pqos.autotuning_completed = false;
         depth=0;
   };
   Component revert_button = Button("Revert Changes", [&] {pqos.load_config("autotuna_rollback.conf"); reset_autotuning(); analyse_button_selector->TakeFocus();}, ButtonOption::Border());
   Component confirm_button = Button("Confirm Changes", [&] {pqos.backup_config("unexpected_backup.conf"); reset_autotuning(); analyse_button_selector->TakeFocus();}, ButtonOption::Border());

   auto autotuna_components = Container::Vertical({
         perf_summary_selector,
         threshold_slider,
         analyse_button_selector,
         priority_selector,
         autotuning_button,
         Container::Horizontal({
               revert_button,
               confirm_button,
               })
         });

   auto autotuna_renderer = Renderer(autotuna_components, [&] {
         if (!pqos.analysis_completed && priority_selector->Focused()) analyse_button_selector->TakeFocus();
         if (pqos.analysis_completed && !tuning_feasible && priority_selector->Focused()) analyse_button_selector->TakeFocus();
         if (pqos.autotuning_completed && autotuning_button->Focused()) revert_button->TakeFocus();
         if (pqos.analysis_completed && !pqos.autotuning_completed && (revert_button->Focused() || confirm_button->Focused())) autotuning_button->TakeFocus();

         return vbox({
               vbox({ // CoS performance summary
                     perf_summary_selector->Render(),
                     perf_summary_window(perf_summary_selector->Focused()),
                     }),
               hbox({
                  vbox({ // Line plots
                        llc_line_plot.get_graph(pqos.get_all_llc_vec(), pqos.get_mon_data_index_vec()),
                        separatorEmpty(),
                        misses_line_plot.get_graph(pqos.get_all_misses_vec(), pqos.get_mon_data_index_vec())
                        }) | size(WIDTH, EQUAL, Terminal::Size().dimx * 0.5),
                  separator(),
                  vbox({
                        text("Auto Tuning") | hcenter,
                        separator(),
                        hbox({
                             threshold_window(threshold_slider),
                             separatorEmpty(),
                             analyse_button_selector->Render(), analyse_button_texts(analyse_button_selector->Focused()),
                        }),
                        pqos.analysis_completed ? analysis_report() : emptyElement(),
                        pqos.analysis_completed && tuning_feasible && !pqos.autotuning_completed ? 
                             priority_selector->Render(), priority_window(priority_selector->Focused()) : emptyElement(),
                        hbox({
                             pqos.analysis_completed && tuning_feasible && !pqos.autotuning_completed ? autotuning_button->Render() : emptyElement(),
                             pqos.autotuning_completed ? revert_button->Render() : emptyElement(),
                             pqos.autotuning_completed ? confirm_button->Render() : emptyElement(),
                        }),
                  }) | border | flex 
               }) | size(HEIGHT, EQUAL, Terminal::Size().dimy * 0.7) 
            });
         });

   // Tab container - CacheTuna & AutoTuna
   auto tab_container = Container::Tab({
         cachetuna_renderer,
         autotuna_renderer,
         },
         &tab_selected);

   // Put all sub-components in primary container
   auto primary_container = Container::Vertical({
         tab_toggle,
         tab_container,
         });

   // Render all primary components
   auto primary_renderer = Renderer(primary_container, [&] {
         return vbox({
               tab_toggle->Render(), // Tab toggle
               separator(),
               tab_container->Render(), // CacheTuna tab
               })
         | border;
         });

   // Modals
   // CacheTuna
   auto save_options_menu = get_save_options_menu();
   auto save_options_modal = Renderer(save_options_menu, [&] {
         return vbox({
               text( " Save Options") | hcenter,
               separator(),
               filler(),
               vbox({
                     save_options_menu->Render(),
                     }),
               })
               | border
               | center;
         });

   Component back_button = Button("Exit", [&] {
         if (save_error_code > 3 && save_error_code < 9) reset_autotuning();
         depth=0;
         },
         button_style);

   auto error_modal = Renderer(back_button, [&] {
         std::string error_msg;
         switch (save_error_code) {
            // 1, 2, 3: cachetuna - applying changes
            case 1:
               error_msg = " core allocation association error";
               break;
            case 2:
               error_msg = " L3CA Ways Mask Update Error";
               break;
            case 3:
               error_msg = " Failed to write to " + Misc::get_executable_path() + "cache_policy";
               break;
            // 4, 5: autotuna - analyses
            case 4:
               error_msg = " core allocation association error during analyses, resetting now";
               break;
            case 5:
               error_msg = " L3CA Ways Mask Update Error during analyses, resetting now";
               break;
            // 6, 7, 8: autotuna - applying tuned config
            case 6:
               error_msg = " core allocation association error, resetting now";
               break;
            case 7:
               error_msg = " L3CA Ways Mask Update Error, resetting now";
               break;
            case 8:
               error_msg = " Failed to write to " + Misc::get_executable_path() + "cache_policy, resetting now";
               break;
            case 9:
               error_msg = " File not found";
               break;
         }
         return vbox({
               text(error_msg),
               separator(),
               filler(),
               vbox({
                     back_button->Render(),
                     }),
               })
               | border
               | center;
         });

   // AutoTuna
   auto analyse_options_menu = get_analyse_options_menu();
   auto analyse_options_modal = Renderer(analyse_options_menu, [&] {
         return vbox({
               text( " Warning") | hcenter,
               separatorEmpty(),
               text( " policies will be reset to default during analysis") | hcenter,
               text( " and currently running programs may be affected!") | hcenter,
               separator(),
               filler(),
               vbox({
                     analyse_options_menu->Render(),
                     }),
               })
               | border
               | center;
   });

   // Loading animation
   std::string img_path = Misc::get_executable_path() + "img/Tuna/";
   std::vector<std::vector<std::string>> tuna_frames = Braille_Generator::generate_all_frames(int(Terminal::Size().dimx*0.5), int(Terminal::Size().dimy*0.7), img_path, 10);

   auto analyse_loading_modal = Renderer([&] {
         auto get_frame = [&] (int index) -> Element{
              Elements lines;
              for (const auto& it : tuna_frames[index % tuna_frames.size()]) {
                  lines.push_back(text(it));
              }
              return vbox({std::move(lines)});
         };

         frame_count++;
         return vbox({
               text( "Analysing . . .") | hcenter,
               separator(),
               filler(),
               get_frame(frame_count),
               })
               | border
               | center;
   });

   auto autotuning_options_menu = get_autotuning_options_menu();
   auto autotuning_options_modal = Renderer(autotuning_options_menu, [&] {
         return vbox({
               text( " Are you sure you want to proceed auto-tuning?") | hcenter,
               separator(),
               filler(),
               vbox({
                     autotuning_options_menu->Render(),
                     }),
               })
               | border
               | center;
         });

   // Render primary with modals
   auto main_container = Container::Tab({
         primary_renderer,
         save_options_modal,
         error_modal, 
         analyse_options_modal,
         analyse_loading_modal,
         autotuning_options_modal,
         },
         &depth);

   auto main_renderer = Renderer(main_container, [&] {
         Element document = primary_renderer->Render();
         switch (depth) {
            case 1: // save options menu
               document = dbox({
                     document,
                     save_options_modal->Render() | clear_under | center,
                     });
               break;
            case 2: // save error
               document = dbox({
                     document,
                     error_modal->Render() | clear_under | center,
                     });
               break;
            case 3: // analyse options menu
               document = dbox({
                     document,
                     analyse_options_modal->Render() | clear_under | center,
                     });
               break;
            case 4: // analyse loading menu
               document = dbox({
                     document,
                     analyse_loading_modal->Render() | clear_under | center,
                     });
               break;
            case 5: // autotuning options menu
               document = dbox({
                     document,
                     autotuning_options_modal->Render() | clear_under | center,
                     });
               break;
         }
         return document;
         });

   ScreenInteractive screen = ScreenInteractive::Fullscreen();
   // key handler
   Component main_component = CatchEvent(main_renderer, [&](Event event) {
         return KeyCallback(tag_selector->Focused(), bitmask_selector->Focused(), cores_selector->Focused(), perf_summary_selector->Focused(), priority_selector->Focused(), screen, event);
         });

   // threads
   threads.push_back(std::thread(&UserInterface::poll_data, this, std::ref(screen)));

   // main loop
   screen.Loop(main_component);
   std::filesystem::remove(Misc::get_executable_path() + "unexpected_exit.conf"); 
   // signal threads to stop
   pqos.run_thread = false;
   // joining threads
   for (auto& thread : threads) {
      thread.join();
   }
   pqos.close();

}
