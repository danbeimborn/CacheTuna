#include "misc.hpp"

using namespace Misc;

std::vector<std::string> Misc::run_cmd(std::string cmd) {
   std::vector<std::string> result;
   FILE *stream;
   const int max_buffer = 256;
   char buffer[max_buffer];
   cmd.append(" 2>&1");
   stream = popen(cmd.c_str(), "r");

   if (stream) {
      while (!feof(stream)) {
         if (fgets(buffer, max_buffer, stream) != NULL) {
            result.push_back(buffer);
         }
      }
      pclose(stream);
   }
   return result;
}

std::vector<std::string> Misc::get_cpu_info() {
   std::string cmd = "lscpu | egrep 'Model name|Socket|Thread|NUMA|CPU\\(s\\)|Architecture'";
   return run_cmd(cmd);
}

std::string Misc::format_bytes(uint64_t bytes) {
   double result;
   std::string unit;
   //const uint64_t kb = static_cast<double>(1000);
   //const uint64_t mb = kb * static_cast<double>(1000);
   const uint64_t kb = 1000;
   const uint64_t mb = kb * 1000;

   if (bytes >= mb) {
      result = static_cast<double>(bytes) / mb;
      unit = " MB";
   }
   else if (bytes >= kb) {
      result = static_cast<double>(bytes) / kb;
      unit = " KB";
   }
   else {
      result = bytes;
      unit = " B";
   }

   std::ostringstream oss;
   oss << std::setprecision(3) << result << unit;
   return oss.str();
}

std::string Misc::get_executable_path() {
   return std::filesystem::canonical("/proc/self/exe").parent_path().string() + "/";
}

std::string Misc::format_misses(uint64_t val) {
   if (val > 1000) {
      std::ostringstream oss;
      double result = static_cast<double>(val) / static_cast<double>(1000);
      oss << std::fixed << std::setprecision(2) << result << " k";
      return oss.str();
   }
   else return std::to_string(val);
}

std::string Misc::to_range_extraction(const std::set<int>& numbers) {
   if (numbers.empty()) {
      return "No cores assigned";
   }

   std::ostringstream res;
   auto cur = numbers.begin();
   int start = *cur;
   int prev = *cur;
   ++cur;

   while (cur != numbers.end()) {
      if (*cur == prev + 1) {   // if current is consecutive to previous
         prev = *cur;   // update previous number to current one
      } else {  // if current is not consecutive to previous
         if (start != prev) { 
            res << start << "-" << prev << ","; // append range to result
         } else {
            res << start << ","; // append single number to result
         }
         start = *cur;
         prev = *cur;
      }
      ++cur;
   }

   // appending last range or single number
   if (start != prev) {
      res << start << "-" << prev;
   } else {
      res << start;
   }

   return res.str();
}

bool Misc::not_contiguous(const std::string& bitmask) {
   bool foundOne = false;
   for (size_t i=1; i<bitmask.size(); ++i) {
      if (bitmask[i] == '1') {
         if (foundOne && bitmask[i-1] != '1') return true;
         foundOne = !foundOne || foundOne;
      }
   }
   return false;
}

// binary to decimal for updating ways mask
unsigned Misc::to_decimal(std::string bitmask) {
   long long n = stoll(bitmask);
   int i=0, res=0;
   while (n>0) {
      res = ((n%10) * pow(2,i))+res;
      n/=10;
      ++i;
   }
   return res;
}
