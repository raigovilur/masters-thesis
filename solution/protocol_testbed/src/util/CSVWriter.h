#ifndef CSVWRITER_H
#define CSVWRITER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <typeinfo>


namespace Utils {
    class CSVWriter
    {
        friend std::ostream& operator<<(std::ostream& os, CSVWriter & csv){
            return os << csv.toString();
        }

        public:
            CSVWriter(const std::string& seperator)
                : _firstRow(true)
                , _seperator(seperator)
                ,_valueCount(0) {
            }
            
            CSVWriter& add(const char *str){
                return add<std::string>(std::string(str));
            }

            CSVWriter& add(const double num){
                return add<double>(num);
            }

            template<typename T>
            CSVWriter& add(T str){
                if(_valueCount > 0)
                    _ss << _seperator;
                _ss << str;
                _valueCount++;
                
                return *this;
            }

            template<typename T>
            CSVWriter& operator<<(const T& t){
                return add(t);
            }


            std::string toString(){
                return _ss.str();
            }


            CSVWriter& newRow(){
                if(!_firstRow){
                    _ss << std::endl;
                }else{
                    _firstRow = false;
                }
                _valueCount = 0;
                return *this;
            }

            bool writeToFile(const std::string& filename, bool append){
                std::ofstream file;
                bool appendNewLine = false;
                if (append) {
                    std::ifstream fin;
                    fin.open(filename);
                    if (fin.is_open()) {
                        fin.seekg(-1, std::ios_base::end); 
                        int lastChar = fin.peek();
                        if (lastChar != -1 && lastChar != '\n') 
                            appendNewLine = true;
                    }
                    file.open(filename.c_str(), std::ios::out | std::ios::app);
                }
                else {
                    file.open(filename.c_str(), std::ios::out | std::ios::trunc);
                }
                if(!file.is_open())
                    return false;
                if(append && appendNewLine)
                    file << std::endl;
                file << toString();
                file.close();
                return file.good();
            }

        private:
            std::string _seperator;
            int _valueCount;
            bool _firstRow;
            std::stringstream _ss;

    };
}
#endif