#include "time.hpp"

#include <sstream>
#include <iomanip>
#include "time.hpp"

DateTimePoint::DateTimePoint(const std::tm &rTime, uint32_t milliseconds)  {
    ms      = milliseconds;
    sec     = static_cast<uint32_t>(rTime.tm_sec);
    min     = static_cast<uint32_t>(rTime.tm_min);
    hour    = static_cast<uint32_t>(rTime.tm_hour);
    day     = static_cast<uint32_t>(rTime.tm_mday);
    month   = static_cast<uint32_t>(rTime.tm_mon + 1);
    year    = static_cast<uint32_t>(rTime.tm_year + 1900);
}

auto DateTimePoint::to_string(const scl::String& format) const -> scl::String {
    std::stringstream ss;
    print(ss, format);
    return scl::String(ss.str());
}

void DateTimePoint::print(std::ostream& os, const scl::String& format) const {
    scl::String formats = "DMYhmsx";
    int   counter    = 0;
    Char8 lastFormat = '\0';

    for (SizeT i = 0; i < format.length() + 1; ++i) {
        Char8 c = format[i];
        if (lastFormat == c)
            ++counter;
        else {
            switch (lastFormat) {
                case 'D': os << std::setfill('0') << std::setw(counter) << day;   break;
                case 'M': os << std::setfill('0') << std::setw(counter) << month; break;
                case 'Y': os << std::setfill('0') << std::setw(counter) << year;  break;
                case 'h': os << std::setfill('0') << std::setw(counter) << hour;  break;
                case 'm': os << std::setfill('0') << std::setw(counter) << min;   break;
                case 's': os << std::setfill('0') << std::setw(counter) << sec;   break;
                case 'x': os << std::setfill('0') << std::setw(counter) << ms;    break;
                default:               break;
            }

            lastFormat = c;
            counter    = 1;
        }

        if (formats.find_first_of(c) == scl::String::npos && c != '\0')
            os << c;
    }
}


auto GlobalTimer::getSystemDateTime() -> DateTimePoint  {
    std::chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::now();
    std::chrono::duration<intmax_t, std::nano> time = tp.time_since_epoch();
    auto now = time.count();
    now /= 1000000;
    std::time_t res =  std::chrono::system_clock::to_time_t(tp);

    std::tm res2 = *localtime(&res);
    return DateTimePoint(res2, static_cast<uint32_t>(now % 1000));
}