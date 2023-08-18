#include "graph.hpp"

using namespace ftxui;

Graph::Graph(const std::string& _title, const std::string& _data_type):
   title(_title),
   data_type(_data_type),
   bar_width(3),
   bar_separator_width(1),
   bar_datum_width(bar_width + bar_separator_width)
{
}

double Graph::get_y_scale(uint64_t max){
   if (max==0) return max;
   
   double log_val = std::log10(max);
   double order_of_mag = std::floor(log_val);
   double log_factor = log_val - order_of_mag;
   double ceil_factor = std::ceil(pow(11, log_factor));
   double scale = ceil_factor * std::pow(10, order_of_mag);
   return scale;
}

Element Graph::get_y_labels(double scale) {
   if (scale == 0) return emptyElement();

   Element max_label, med_label, min_label;

   if (data_type == "llc") {
      max_label = text(Misc::format_bytes(scale));
      med_label = text(Misc::format_bytes(scale/2));
      min_label = text(Misc::format_bytes(0));
   }
   else if (data_type == "misses") {
      max_label = text(Misc::format_misses(scale));
      med_label = text(Misc::format_misses(scale/2));
      min_label = text(Misc::format_misses(0));
   }

   return vbox({
         max_label,
         filler(),
         med_label,
         filler(),
         min_label,
         });
}

GraphFunction Graph::bar_graph_function(const std::vector<uint64_t>& data, double scale) {
   return [&, data, scale](int width, int height) -> std::vector<int>{
      std::vector<int> graph_data;
      double datum;
      size_t i = data.size() * bar_datum_width > width ? data.size() - width / bar_datum_width : 0; // start point

      for (i; i<data.size(); ++i) {
         datum = data[i];
         if (datum != 0) { // scale bar height per datum
            datum = datum / scale * height;
         }
         graph_data.insert(graph_data.end(), bar_width, datum);
         graph_data.insert(graph_data.end(), bar_separator_width, 0);
      }
      while (graph_data.size() < width) { // fill remaining width
         graph_data.push_back(0);
      }
      return graph_data;
   };
}

// Bar graph
Element Graph::get_graph(const std::vector<uint64_t> data) {
   if (data.empty()) {
      return vbox({
            text(title) | hcenter,
            filler(),
            text("No Data") | hcenter,
            });
   }

   double scale = get_y_scale(*(std::max_element(data.begin(), data.end())));
   return vbox({
        text(title) | hcenter,
        hbox({
              get_y_labels(scale),
              separatorEmpty(),
              graph(bar_graph_function(data, scale)) | color(Color::DodgerBlue1)
              }) | yflex_grow
        });
}

Canvas Graph::line_plot_function(const std::vector<std::vector<uint64_t>> mon_data_vec, const std::vector<unsigned> index_vec, uint64_t max, uint64_t min) {
   size_t j;
   int y1, y2, x1, x2;
   std::vector<uint64_t> data;

   int datum_width = 15;
   int label_width = 20;

   int canvas_height = Terminal::Size().dimy;
   int canvas_width = Terminal::Size().dimx;
   Canvas canvas = Canvas(canvas_width, canvas_height + canvas_height * 0.05);

   auto scale = [&](uint64_t value) -> double{
      double max_scale = get_y_scale(max);
      return canvas_height - ((value/max_scale) * canvas_height);
   };

   // line for each cos
   std::vector<double> label_position;
   for (size_t i=0; i<mon_data_vec.size(); ++i) {
      x1=0;
      x2=datum_width;
      data = mon_data_vec[i];
      int data_width = data.size() * datum_width;
      j = canvas_width <= data_width ? (data_width - canvas_width) / datum_width : 0;
      for (j; j<data.size(); ++j) {
         y1 = scale(data[j-1]);
         y2 = scale(data[j]);
         canvas.DrawPointLine(x1, y1, x2, y2, Color::White);
         x1 += datum_width;
         x2 += datum_width;
      }
      label_position.push_back(data[data.size()-2]);
   }
   // label
   for (size_t i=0; i<label_position.size(); ++i) {
      std::string label = "Cos " + std::to_string(index_vec[i]);
      canvas.DrawText(i * label_width, scale(label_position[i]), label);
   }

   return canvas;
}

// Line Plot
Element Graph::get_graph(const std::vector<std::vector<uint64_t>> mon_data_vec, std::vector<unsigned> index_vec) {
   bool any_empty = std::any_of(mon_data_vec.begin(), mon_data_vec.end(), std::mem_fn(&std::vector<uint64_t>::empty));
   if (any_empty) {
      return vbox({
            text(title) | hcenter,
            filler(),
            text("No Data") | hcenter | border,
            });
   }

   uint64_t element;
   int start_index = mon_data_vec[0].size() >= 12 ? mon_data_vec[0].size() - 12 : 0;
   uint64_t max = mon_data_vec[0][start_index];
   uint64_t min = mon_data_vec[0][start_index];
   for (auto row : mon_data_vec) {
      for (size_t i=start_index; i<row.size(); ++i) {
         element = row[i];
         max = element > max? element : max;
         min = element < min? element : min;
      }
   }

   double scale = get_y_scale(max);

   return vbox({
         text(title)
         | hcenter,
         hbox({
               get_y_labels(scale),
               separatorEmpty(),
               vbox({
                  canvas(std::move(line_plot_function(mon_data_vec, index_vec, max, min))) | yflex_grow
                  })
               })
         })
   | border
   | flex;
}
