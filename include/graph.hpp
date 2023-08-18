#ifndef CACHETUNA_GRAPH_HPP
#define CACHETUNA_GRAPH_HPP

// std
#include <math.h>

// ftxui
#include "ftxui/dom/elements.hpp"

// Cachetuna
#include "misc.hpp"

class Graph {
   private:
      std::string title;
      std::string graph_type; // bar graph, line plot
      std::string data_type; // llc, misses
      int bar_width;
      int bar_separator_width;
      int bar_datum_width;
      double get_y_scale(uint64_t max);
      ftxui::Element get_y_labels(double scale);
      ftxui::GraphFunction bar_graph_function(const std::vector<uint64_t>& data,double scale);
      ftxui::Canvas line_plot_function(std::vector<std::vector<uint64_t>> mon_data_vec, const std::vector<unsigned> index_vec, uint64_t max, uint64_t min);

   public:
      Graph(const std::string& _title, const std::string& _data_type);
      ftxui::Element get_graph(const std::vector<uint64_t> data); // bar graph
      ftxui::Element get_graph(const std::vector<std::vector<uint64_t>> mon_data_vec, const std::vector<unsigned> index_vec); // line plot
};

#endif // cachetuna_graph_hpp
