#ifndef TIME_RECORDER_H
#define TIME_RECORDER_H

#include <time.h>
#include <string>

#include "util/CSVWriter.h"

namespace Utils {
    class TimeRecorder {
        public:
            explicit TimeRecorder(const std::string filename) : _filename(filename){ }
            TimeRecorder();

            void writeInfoWithTime(std::string info) {
                infoRecordCSV.newRow() << getCurrentTime() << info;
            }

            void writeInfoWithDateTime(std::string info) {
                infoRecordCSV.newRow() << getCurrentDateTime() << info;
            }

            void writeToFile(){
                infoRecordCSV.writeToFile(_filename, false);
            }
            
        private:
            Utils::CSVWriter infoRecordCSV{","};
            const std::string _filename;

            std::string getCurrentDateTime() {
                time_t now = time(0);
                struct tm tstruct;
                char buf[32];
                tstruct = *localtime(&now);
                
                strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tstruct);

                return buf;
            }

            std::string getCurrentTime() {
                time_t now = time(0);
                struct tm tstruct;
                char buf[32];
                tstruct = *localtime(&now);
                
                strftime(buf, sizeof(buf), "%H:%M:%S", &tstruct);

                return buf;
            }
    };
}

#endif