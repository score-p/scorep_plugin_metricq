#include "footprint.hpp"
#include "msequence.hpp"

#include <scorep/plugin/util/environment.hpp>

#include <chrono>
#include <vector>

#include <cassert>

namespace timesync
{
void Footprint::high()
{
    double m = 0.0;
    for (std::size_t r = 0; r < compute_rep; r++)
    {
        for (size_t i = 0; i < compute_size; i++)
        {
            m += compute_vec_a_[i] * compute_vec_b_[i];
        }
    }
    if (m == 42.0)
    {
// prevent optimization, sure there is an easier way
#ifdef __arm__
        __asm__ __volatile__("dmb ish;" :::);
#else
        __asm__ __volatile__("mfence;" :::);
#endif
    }
}

void Footprint::low()
{
    for (uint64_t i = 0; i < nop_rep; i++)
    {
#ifdef __arm__
        asm volatile("yield" ::: "memory");
#else
        asm volatile("rep; nop" ::: "memory");
#endif
    }
}

void Footprint::check_affinity()
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

void Footprint::restore_affinity()
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

void Footprint::run(int msequence_exponent, Duration quantum)
{
    check_affinity();

    recording_.resize(0);
    recording_.reserve(4096);

    Duration tolerance = std::chrono::seconds(2);
    if (auto tolerance_str = scorep::environment_variable::get("SYNC_TOLERANCE");
        !tolerance_str.empty())
    {
        tolerance = metricq::duration_parse(tolerance_str);
    }

    auto sequence = GroupedBinaryMSequence(msequence_exponent);

    time_begin_ = low(tolerance);
    time_end_ = time_begin_;
    auto deadline = time_begin_;
    while (auto elem = sequence.take())
    {
        auto [is_high, length] = *elem;
        auto duration = quantum * length;
        deadline += duration;
        if (deadline <= time_end_)
        {
            continue;
        }
        auto wait = deadline - time_end_;
        if (is_high)
        {
            time_end_ = high(wait);
        }
        else
        {
            time_end_ = low(wait);
        }
    }

    low(tolerance);

    restore_affinity();
}

} // namespace timesync
