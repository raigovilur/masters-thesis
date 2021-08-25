#include<boost/filesystem.hpp>
#include<boost/tokenizer.hpp>
#include<string>
#include<iostream>

namespace Utils {

inline std::vector<std::string> parseCSVLine(const std::string& line){
   using namespace boost;

   std::vector<std::string> vec;

   // Tokenizes the input string
   tokenizer<escaped_list_separator<char> > tk(line, escaped_list_separator<char>
   ('\\', ',', '\"'));
   for (auto i = tk.begin();  i!=tk.end();  ++i)
   vec.push_back(*i);

   return vec;
}


inline unsigned int getLineNum(const std::string& filename){
   boost::filesystem::ifstream file(filename);
   int lineNum = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
   return lineNum;
}

inline size_t getFieldNum(const std::string& filename){
   boost::filesystem::ifstream file(filename);
   std::string line;
   getline(file, line);
   auto row = parseCSVLine(line);
   return row.size();
}

}