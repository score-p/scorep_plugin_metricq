#pragma once

#include "../log.hpp"

#include "footprint.hpp"
#include "shifter.hpp"

#include <metricq/ostream.hpp>

#include <memory>
#include <stdexcept>
#include <vector>

#include <cmath>

namespace timesync
{
template <typename T, typename TP, typename DUR>
std::vector<double> sample(const T& recording, TP time_begin, TP time_end, DUR interval)
{
    using std::begin;
    using std::end;
    auto it = begin(recording);

    assert(!recording.empty());

    assert(time_begin <= time_end);

    std::vector<double> output;
    output.reserve((time_end - time_begin) / interval);
    for (auto tp = time_begin; tp < time_end; tp += interval)
    {
        while (it != end(recording) && (it->time < tp))
        {
            it++;
        }
        if (it == end(recording))
        {
            Log::error() << "Failed to sample in range " << time_begin << " to " << time_end;
            Log::error() << "Recording goes from: " << begin(recording)->time << " to "
                         << (it - 1)->time;

            throw std::out_of_range(
                "Insufficient time range for sampling - maybe clock drift is too large?");
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
        assert(footprint_begin_);
        assert(footprint_end_);
        Log::debug() << "find begin offsets...";
        auto offset_begin =
            find_offset(*footprint_begin_, measured_raw_signal, "begin") * sampling_interval_;
        Log::debug() << "find end offsets...";
        auto offset_end =
            find_offset(*footprint_end_, measured_raw_signal, "end") * sampling_interval_;

        auto footprint_duration = footprint_end_->time() - footprint_begin_->time();
        auto measurement_duration = footprint_duration + offset_end - offset_begin;
        time_rate_ = static_cast<double>(footprint_duration.count()) / measurement_duration.count();
        offset_zero_ = footprint_begin_->time() -
                       time_point_scale(footprint_begin_->time() + offset_begin, time_rate_);

        Log::info() << "offsets" << offset_begin << ", " << offset_end << ", rate: " << time_rate_;
        Log::debug() << "Offset0: " << offset_zero_.count();
    }

    metricq::TimePoint to_local(metricq::TimePoint measurement_time)
    {
        return time_point_scale(measurement_time, time_rate_) + offset_zero_;
    }

private:
    static metricq::TimePoint time_point_scale(metricq::TimePoint time, double factor)
    {
        return metricq::TimePoint(metricq::duration_cast(time.time_since_epoch() * factor));
    }

    template <typename T>
    auto find_offset(Footprint& footprint, const T& measured_raw_signal, const std::string& tag)
    {
        auto st_begin = footprint.time_begin();
        auto st_end = footprint.time_end();

        Log::debug() << "Sampling footprint from " << st_begin.time_since_epoch().count() << " to "
                     << st_end.time_since_epoch().count() << " with interval "
                     << sampling_interval_.count() << ":";
        auto footprint_signal = sample(footprint.recording(), st_begin, st_end, sampling_interval_);

        Log::debug() << "Sampling raw signal:";

        auto measured_signal = sample(measured_raw_signal, st_begin, st_end, sampling_interval_);

        assert(measured_signal.size() == footprint_signal.size());
        assert(!measured_signal.empty());
        Log::debug() << "looking for shift in " << measured_signal.size() << " data points";

        Shifter shifter(measured_signal.size(), tag);
        auto result = shifter(footprint_signal, measured_signal);
        Log::debug() << "completed timesync with correlation of " << result.second << " and "
                     << result.first;
        if (std::isnan(result.second) || result.second <= 0)
        {
            throw std::runtime_error("Correlation doesn't look good to me :(");
        }
        return result.first;
    }

private:
    metricq::Duration sampling_interval_ = std::chrono::microseconds(2);
    std::unique_ptr<Footprint> footprint_begin_;
    std::unique_ptr<Footprint> footprint_end_;

    double time_rate_;              // local time per measurement time
    metricq::Duration offset_zero_; // diff between local time and measurement time
};

}; // namespace timesync
