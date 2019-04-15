#include "fft.hpp"

#include <scorep/plugin/plugin.hpp>

#include <fstream>

class Shifter
{

public:
    Shifter(std::size_t size, const std::string& tag);

    template <typename T1, typename T2>
    std::pair<long, double> operator()(T1 left_begin, T1 left_end, T2 right_begin, T2 right_end)
    {
        assert(std::distance(left_begin, left_end) == size_);
        assert(std::distance(right_begin, right_end) == size_);

        fft_(left_begin, left_end);
        if (!fft_.isfinite())
        {
            throw std::runtime_error("left is not finite");
        }
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               type_size<complex_type>(extended_size_));

        std::copy(fft_.out_begin(), fft_.out_end(), tmp_.begin());

        fft_(std::reverse_iterator(right_end), std::reverse_iterator(right_begin));
        if (!fft_.isfinite())
        {
            throw std::runtime_error("right is not finite");
        }
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               type_size<complex_type>(extended_size_));

        for (int i = 0; i < tmp_.size(); i++)
        {
            tmp_[i] *= fft_.out_begin()[i];
            if (!my_isfinite(tmp_[i]))
            {
                throw std::runtime_error("product is not finite");
            }
        }

        ifft_(tmp_.begin(), tmp_.end());
        assert(std::distance(ifft_.out_begin(), ifft_.out_end()) == extended_size_);

        if (!ifft_.isfinite())
        {
            throw std::runtime_error("cross-correlation is not finite");
        }

        auto it = std::max_element(ifft_.out_begin(), ifft_.out_end(), [](auto a, auto b) {
            if (!std::isfinite(a) || !std::isfinite(b))
            {
                return false;
            }
            return a < b;
        });

        auto correlation_filename = scorep::environment_variable::get("CORRELATION_FILE");
        if (!correlation_filename.empty())
        {
            static int cnt = 0; // cheap hack for distinguishing begin/end synchronization
            correlation_filename += std::to_string(cnt);
            cnt++;
            std::ofstream file;
            file.exceptions(std::ofstream::badbit);
            file.open(correlation_filename);
            for (auto it = ifft_.out_begin(); it != ifft_.out_end(); ++it)
            {
                file << *it << "\n";
            }
        }

        std::cerr << "Found max element: " << std::distance(ifft_.out_begin(), it) << ": " << *it
                  << std::endl;
        return { long(size_) - 1 - std::distance(ifft_.out_begin(), it), *it };
    };

    template <typename T1, typename T2>
    auto operator()(T1 left, T2 right)
    {
        using std::begin;
        using std::end;
        return (*this)(begin(left), end(left), begin(right), end(right));
    }

private:
    std::string tag_;
    std::size_t size_;
    std::size_t extended_size_;
    FFT fft_;
    IFFT ifft_;
    std::vector<complex_type> tmp_;
};
