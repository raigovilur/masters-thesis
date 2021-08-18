#include <time.h>
#include <string>

namespace Utils {
    inline const std::string getCurrentDateTime() {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[32];
        tstruct = *localtime(&now);
        
        strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M-%S", &tstruct);

        return buf;
    }

    inline const std::string getCurrentTime() {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[32];
        tstruct = *localtime(&now);
        
        strftime(buf, sizeof(buf), "%H-%M-%S", &tstruct);

        return buf;
    }

}