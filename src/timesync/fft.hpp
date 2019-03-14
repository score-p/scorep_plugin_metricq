#pragma once

#include <fftw3.h>

#include <algorithm>
#include <complex>
#include <iterator>
#include <vector>

#include <cassert>
#include <climits>
#include <cstddef>

using complex_type = std::complex<double>;

bool isfinite(const std::complex<double>& z)
{
    return std::isfinite(z.real()) && std::isfinite(z.imag());
}

template <typename IN, typename OUT>
class FFTBase
{
public:
    FFTBase(std::size_t size)
    : size_(size), in_((IN*)fftw_malloc(sizeof(IN) * size)),
      out_((OUT*)fftw_malloc(sizeof(OUT) * size))
    {
    }

    ~FFTBase()
    {
        fftw_free(in_);
        fftw_free(out_);
        fftw_destroy_plan(plan_);
    }

    template <typename IT>
    const void operator()(IT begin, IT end)
    {
        auto len = std::distance(begin, end);
        assert(len <= size_);
        std::copy(begin, end, in_);
        std::fill(in_ + len, in_ + size_, IN());
        fftw_execute(plan_);
    }

    IN* in_begin()
    {
        return in_;
    }

    IN* in_end()
    {
        return in_ + size_;
    }

    OUT* out_begin()
    {
        return out_;
    }

    OUT* out_end()
    {
        return out_ + size_;
    }

    bool isfinite() const
    {
        for (auto it = out_begin(); it != out_end(); ++it)
        {
            using std::isfinite;
            if (!isfinite(*it))
            {
                return false;
            }
        }
        return true;
    }

protected:
    std::size_t size_;
    IN* in_;
    OUT* out_;
    fftw_plan plan_;
};

class FFT : public FFTBase<double, complex_type>
{
public:
    FFT(std::size_t size) : FFTBase(size)
    {
        plan_ =
            fftw_plan_dft_r2c_1d(size_, in_, reinterpret_cast<fftw_complex*>(out_), FFTW_ESTIMATE);
    }
};

class IFFT : public FFTBase<complex_type, double>
{
public:
    IFFT(std::size_t size) : FFTBase(size)
    {
        plan_ =
            fftw_plan_dft_c2r_1d(size_, reinterpret_cast<fftw_complex*>(in_), out_, FFTW_ESTIMATE);
    }
};

class Shifter
{
    // Powers of two are much more efficient for FFTW.
    static std::size_t next_power_of_2(std::size_t size)
    {
        size--;
        for (size_t i = 1; i < sizeof(size) * CHAR_BIT; i *= 2)
        {
            size |= size >> i;
        }
        size++;
        return size;
    }

public:
    Shifter(std::size_t size)
    : size_(size), extended_size_(next_power_of_2(2 * size_ - 1)), fft_(extended_size_),
      ifft_(extended_size_), tmp_(extended_size_)
    {
    }

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
        assert(std::distance(fft_.out_begin(), fft_.out_end()) == extended_size_);

        std::copy(fft_.out_begin(), fft_.out_end(), tmp_.begin());

        fft_(std::reverse_iterator(right_end), std::reverse_iterator(right_begin));
        if (!fft_.isfinite())
        {
            throw std::runtime_error("right is not finite");
        }
        assert(std::distance(fft_.out_begin(), fft_.out_end()) == extended_size_);

        for (int i = 0; i < extended_size_; i++)
        {
            tmp_[i] *= fft_.out_begin()[i];
            if (!isfinite(tmp_[i]))
            {
                throw std::runtime_error("product is not finite");
            }
        }

        ifft_(tmp_.begin(), tmp_.end());
        assert(std::distance(ifft_.out_begin(), ifft_.out_end()) == extended_size_);

        if (!fft_.isfinite())
        {
            throw std::runtime_error("cross-correlation is not finite");
        }

        auto it = std::max_element(ifft_.out_begin(), ifft_.out_end(), [](auto a, auto b) {
            if (!std::isfinite(a))
                return true;
            if (!std::isfinite(b))
                return false;
            return a < b;
        });
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
    std::size_t size_;
    std::size_t extended_size_;
    FFT fft_;
    IFFT ifft_;
    std::vector<complex_type> tmp_;
};
