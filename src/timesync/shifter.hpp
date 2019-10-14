#include "fft.hpp"

#include <metricq/logger/nitro.hpp>

#include <scorep/plugin/plugin.hpp>

#include <fstream>

using Log = metricq::logger::nitro::Log;

class Shifter
{

public:
    Shifter(std::size_t size, const std::string& tag);

    template <typename T1, typename T2>
    int operator()(T1 left_begin, T1 left_end, T2 right_begin, T2 right_end,
                   int oversampling_factor)
    {
        static int cnt = -1; // cheap hack for distinguishing begin/end synchronization
        cnt++;

        assert(std::distance(left_begin, left_end) == size_);
        assert(std::distance(right_begin, right_end) == size_);

        fft_(left_begin, left_end);
        fft_.check_finite();
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               type_size<complex_type>(extended_size_));

        std::copy(fft_.out_begin(), fft_.out_end(), tmp_.begin());

        fft_(right_begin, right_end);
        fft_.check_finite();
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               type_size<complex_type>(extended_size_));

        for (int i = 0; i < tmp_.size(); i++)
        {
            auto other = fft_.out_begin()[i];
            // complex conjugate
            other.imag(-other.imag());
            tmp_[i] *= other;
            check_finite(tmp_[i]);
        }

        ifft_(tmp_.begin(), tmp_.end());
        assert(std::distance(ifft_.out_begin(), ifft_.out_end()) == extended_size_);

        ifft_.check_finite();

        auto mainlobe = std::max_element(ifft_.out_begin(), ifft_.out_end(), [](auto a, auto b) {
            if (!std::isfinite(a) || !std::isfinite(b))
            {
                return false;
            }
            return a < b;
        });

        auto correlation_filename = scorep::environment_variable::get("CORRELATION_FILE");
        if (!correlation_filename.empty())
        {
            correlation_filename += std::to_string(cnt);
            std::ofstream file;
            file.exceptions(std::ofstream::badbit);
            file.open(correlation_filename);
            for (auto it = ifft_.out_begin(); it != ifft_.out_end(); ++it)
            {
                file << *it << "\n";
            }
        }

        auto mainlobe_index = std::distance(ifft_.out_begin(), mainlobe);

        // determine the sidelobe.
        // need to consider the circular fashion here
        auto sidelobe_value = std::numeric_limits<double>::lowest();
        for (int i = 0; i < ifft_.out_size(); ++i)
        {
            const auto value = fabs(ifft_.out_begin()[i]);
            if (value > sidelobe_value)
            {
                auto distance = std::min((i - mainlobe_index) % ifft_.out_size(),
                                         (mainlobe_index - i) % ifft_.out_size());
                if (distance >= oversampling_factor)
                {
                    sidelobe_value = value;
                }
            }
        }

        if (mainlobe_index >= size_)
        {
            mainlobe_index -= ifft_.out_size();
        }
        auto sidelobe_factor = *mainlobe / sidelobe_value;
        Log::debug() << "Found max correlation with offset " << mainlobe_index << ": " << *mainlobe;
        if (sidelobe_factor < 3)
        {
            Log::warn() << "The time synchronization probably did not work (" << sidelobe_factor
                        << ")";
        }
        else
        {
            Log::debug() << "Correlation main-sidelobe-factor: " << sidelobe_factor;
        }

        return -mainlobe_index;
    };

    template <typename T1, typename T2>
    auto operator()(T1 left, T2 right, int oversampling_factor)
    {
        using std::begin;
        using std::end;
        return (*this)(begin(left), end(left), begin(right), end(right), oversampling_factor);
    }

private:
    std::string tag_;
    std::size_t size_;
    std::size_t extended_size_;
    FFT fft_;
    IFFT ifft_;
    std::vector<complex_type> tmp_;
};
