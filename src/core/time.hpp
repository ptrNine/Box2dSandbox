#pragma once

#include <chrono>
#include <ctime>
#include <iostream>

#include <fmt/format.h>
#include <scl/scl.hpp>

#include "types.hpp"
#include "helper_macros.hpp"


struct DateTimePoint {
    DateTimePoint(const std::tm& rTime, uint32_t milliseconds);

    void print     (std::ostream& os = std::cout,
                    const scl::String& format = "DD.MM.YYYY hh:mm:ss") const;
    auto to_string (const scl::String& format = "DD.MM.YYYY hh:mm:ss") const -> scl::String;

    friend auto operator<< (std::ostream& os, const DateTimePoint& tp) -> std::ostream& {
        tp.print(os);
        return os;
    }

    uint32_t ms;
    uint32_t sec;
    uint32_t min;
    uint32_t hour;
    uint32_t day;
    uint32_t month;
    uint32_t year;
};



class TimeDuration {
public:
    explicit
    TimeDuration(const std::chrono::duration<intmax_t, std::nano>& d): _duration(d) {}

    double  sec  () const { return std::chrono::duration<double>(_duration).count(); }
    float   secf () const { return std::chrono::duration<float>(_duration).count(); }
    int64_t milli() const { return std::chrono::duration_cast<std::chrono::milliseconds>(_duration).count(); }
    int64_t micro() const { return std::chrono::duration_cast<std::chrono::microseconds>(_duration).count(); }
    int64_t nano () const { return _duration.count(); }

private:
    std::chrono::duration<intmax_t, std::nano> _duration;
};


class Timestamp {
    using TimePointT = std::chrono::steady_clock::time_point;

public:
    Timestamp() = default;
    explicit Timestamp(const TimePointT& t): _timestamp(t) {}

    TimeDuration operator-(const Timestamp& ts) const {
        return TimeDuration(_timestamp - ts._timestamp);
    }

private:
    TimePointT _timestamp;
};



class GlobalTimer {
    SINGLETON_IMPL(GlobalTimer);

    using SteadyT     = std::chrono::steady_clock;

public:
    auto timestamp        () -> Timestamp { return Timestamp(SteadyT::now()); }
    auto getSystemDateTime() -> DateTimePoint;

private:
    GlobalTimer () = default;
    ~GlobalTimer() = default;
};

inline GlobalTimer&	timer() { return GlobalTimer::instance(); }


class Timer {
public:
    Timer() {
        _last = timer().timestamp();
    }

    TimeDuration tick() {
        auto timestamp = timer().timestamp();
        auto diff = timestamp - _last;

        _last = timestamp;
        return diff;
    }

private:
    Timestamp _last;
};


inline void sleep(uint32_t milliseconds) {
    auto cur = timer().timestamp();
    while ((timer().timestamp() - cur).milli() < milliseconds);
}


namespace fmt {
    template<>
    struct formatter<DateTimePoint> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const DateTimePoint& date, FormatContext& ctx) {
            return format_to(ctx.out(), "{}", date.to_string());
        }
    };
}

