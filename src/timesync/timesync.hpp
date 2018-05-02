#pragma once

#include "../log.hpp"

#include "fft.hpp"
#include "footprint.hpp"

#include <memory>
#include <stdexcept>
#include <vector>

namespace timesync
{
template <typename T, typename TP, typename DUR>
std::vector<double> sample(const T& recording, TP time_begin, TP time_end, DUR interval)
{
    using std::begin;
    using std::end;
    auto it = begin(recording);

    std::vector<double> output;
    output.reserve((time_end - time_begin) / interval);
    for (auto tp = time_begin; tp < time_end; tp += interval)
    {
        while (it->time < tp)
        {
            if (it == end(recording))
            {
                throw std::out_of_range("insufficient time range for sampling");
            }
            it++;
        }
        // now: tp <= it->time
        output.push_back(it->value);
    }
    return output;
}

class CCTimeSync
{
public:
    void sync_begin()
    {
        footprint_begin_ = std::make_unique<Footprint>();
    }

    void sync_end()
    {
        footprint_end_ = std::make_unique<Footprint>();
    }

    template <typename T>
    auto find_offsets(const T& measured_raw_signal)
    {
        Log::debug() << "computing metric time offsets";
        assert(footprint_begin_);
        assert(footprint_end_);
        auto offset_begin =
            find_offset(*footprint_begin_, measured_raw_signal) * sampling_interval_;
        auto offset_end = find_offset(*footprint_end_, measured_raw_signal) * sampling_interval_;

        auto footprint_duration = footprint_end_->time() - footprint_begin_->time();
        auto measurement_duration = footprint_duration + offset_end - offset_begin;
        time_rate_ = static_cast<double>(footprint_duration.count()) / measurement_duration.count();
        offset_zero_ = footprint_begin_->time() -
                       time_point_scale(footprint_begin_->time() + offset_begin, time_rate_);

        Log::info() << "offsets" << offset_begin << ", " << offset_end << ", rate: " << time_rate_;
        Log::debug() << "Offset0: " << offset_zero_.count();
    }

    dataheap2::TimePoint to_local(dataheap2::TimePoint measurement_time)
    {
        return time_point_scale(measurement_time, time_rate_) + offset_zero_;
    }

private:
    static dataheap2::TimePoint time_point_scale(dataheap2::TimePoint time, double factor)
    {
        return dataheap2::TimePoint(dataheap2::duration_cast(time.time_since_epoch() * factor));
    }

    template <typename T>
    auto find_offset(Footprint& footprint, const T& measured_raw_signal)
    {
        auto st_begin = footprint.time_begin();
        auto st_end = footprint.time_end();

        auto footprint_signal = sample(footprint.recording(), st_begin, st_end, sampling_interval_);
        auto measured_signal = sample(measured_raw_signal, st_begin, st_end, sampling_interval_);

        assert(measured_signal.size() == footprint_signal.size());
        Log::debug() << "looking for shift in " << measured_signal.size() << " data points";

        Shifter shifter(measured_signal.size());
        auto result = shifter(footprint_signal, measured_signal);
        Log::debug() << "completed timesync with correlation of " << result.second;
        return result.first;
    }

private:
    dataheap2::Duration sampling_interval_ = std::chrono::microseconds(2);
    std::unique_ptr<Footprint> footprint_begin_;
    std::unique_ptr<Footprint> footprint_end_;

    double time_rate_;                // local time per measurement time
    dataheap2::Duration offset_zero_; // diff between local time and measurement time
};

}; // namespace timesync
