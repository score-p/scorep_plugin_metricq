#pragma once

#include "../log.hpp"

#include "msequence.hpp"

#include <metricq/types.hpp>

#include <sched.h>

#include <chrono>
#include <vector>

#include <cassert>

using Clock = metricq::Clock;
using TimeValue = metricq::TimeValue;

namespace timesync
{

uint64_t sqrtsd_loop_(double* buffer, uint64_t elems, uint64_t repeat);

class Footprint
{
public:
    Footprint() : a(size, 1.0), b(size, 2.0)
    {
        Log::info() << "staring synchronization pattern";
        run();
        Log::info() << "completed synchronization pattern";
    }

    Clock::time_point time_begin() const
    {
        return time_begin_;
    }

    Clock::time_point time() const
    {
        return time_begin_ + (time_end_ - time_begin_) / 2;
    }

    Clock::time_point time_end() const
    {
        return time_end_;
    }

    const std::vector<TimeValue>& recording() const
    {
        return recording_;
    };

private:
protected:
    void low()
    {
        sqrtsd_loop_(a.data(), a.size(), 256);
    }

    void high()
    {
        double m = 0.0;
        for (size_t i = 0; i < a.size(); i++)
        {
            m += a[i] * b[i];
        }
        if (m == 42.0)
        {
            // prevent optimization, sure there is an easier way
            __asm__ __volatile__("mfence;" :::);
        }
    }

    template <typename DURATION>
    auto low(DURATION duration)
    {
        auto time = Clock::now();
        auto end = time + duration;
        do
        {
            low();
            time = Clock::now();
        } while (time < end);
        recording_.emplace_back(time, 0.0);
        return time;
    }

    template <typename DURATION>
    auto high(DURATION duration)
    {
        auto time = Clock::now();
        auto end = time + duration;
        do
        {
            high();
            time = Clock::now();
        } while (time < end);
        recording_.emplace_back(time, 1.0);
        return time;
    }

    template <typename DURATION>
    auto run(bool high_low, DURATION duration)
    {
        if (high_low)
        {
            return high(duration);
        }
        else
        {
            return low(duration);
        }
    }

    void run()
    {
        check_affinity();

        recording_.resize(0);
        recording_.reserve(12);

        time_begin_ = low(std::chrono::seconds(3));

        low(std::chrono::seconds(1));
        high(std::chrono::milliseconds(419));
        low(std::chrono::milliseconds(283));
        high(std::chrono::milliseconds(179));
        low(std::chrono::milliseconds(73));
        high(std::chrono::milliseconds(31));
        low(std::chrono::milliseconds(127));
        high(std::chrono::milliseconds(233));
        low(std::chrono::milliseconds(353));
        high(std::chrono::milliseconds(467));
        time_end_ = low(std::chrono::seconds(1));

        low(std::chrono::seconds(3));

        restore_affinity();
    }

    void check_affinity()
    {
        CPU_ZERO(&cpu_set_old_);
        auto err = sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set_old_);
        if (err)
        {
            Log::error() << "failed to get thread affinity: " << strerror(errno);
            return;
        }

        cpu_set_t cpu_set_target;
        CPU_ZERO(&cpu_set_target);
        CPU_SET(0, &cpu_set_target);
        err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set_target);
        if (err)
        {
            Log::error() << "failed to set thread affinity: " << strerror(errno);
            return;
        }
        restore_affinity_ = true;
    }

    void restore_affinity()
    {
        if (restore_affinity_)
        {
            auto err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set_old_);
            if (err)
            {
                Log::error() << "failed to restore thread affinity: " << strerror(errno);
            }
        }
    }

private:
    static constexpr std::size_t size = 2048;
    Clock::time_point time_begin_;
    Clock::time_point time_end_;

    std::vector<double> a;
    std::vector<double> b;
    std::vector<TimeValue> recording_;

    bool restore_affinity_ = false;
    cpu_set_t cpu_set_old_;
};
} // namespace timesync
