#include "timesync.hpp"

namespace timesync
{

CCTimeSync::CCTimeSync()
{
    if (auto exponent_str = scorep::environment_variable::get("SYNC_EXPONENT");
        !exponent_str.empty())
    {
        footprint_msequence_exponent_ = std::stoi(exponent_str);
    }
    if (auto quantum_str = scorep::environment_variable::get("SYNC_QUANTUM"); !quantum_str.empty())
    {
        footprint_quantum_ = metricq::duration_parse(quantum_str);
    }
    if (auto sampling_str = scorep::environment_variable::get("SYNC_SAMPLING");
        !sampling_str.empty())
    {
        sampling_interval_ = metricq::duration_parse(sampling_str);
    }
    Log::debug() << "Using a footprint sequence with exponent " << footprint_msequence_exponent_
                 << " and a time quantum of " << footprint_quantum_;
}
} // namespace timesync
