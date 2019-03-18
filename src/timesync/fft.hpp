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

static_assert(sizeof(complex_type) == sizeof(fftw_complex), "You're fucked.");

inline bool my_isfinite(complex_type z)
{
    return std::isfinite(z.real()) && std::isfinite(z.imag());
}

inline bool my_isfinite(double a)
{
    return std::isfinite(a);
}

template <typename T>
struct Helper
{
    static std::size_t size(std::size_t s)
    {
        return s;
    }
};

template <>
struct Helper<complex_type>
{
    static std::size_t size(std::size_t s)
    {
        return s / 2 + 1;
    }
};

template <typename T>
size_t sizey(size_t s)
{
    return Helper<T>::size(s);
}

template <typename IN, typename OUT>
class FFTBase
{
public:
    FFTBase(std::size_t size)
    : size_(size), in_((IN*)fftw_malloc(sizeof(IN) * in_size())),
      out_((OUT*)fftw_malloc(sizeof(OUT) * out_size()))
    {
        assert(_in);
        assert(out_);
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
        assert(len <= in_size());
        std::copy(begin, end, in_);
        std::fill(in_ + len, in_ + in_size(), IN());
        fftw_execute(plan_);
    }

    std::size_t in_size() const
    {
        return sizey<IN>(size_);
    }

    IN* in_begin()
    {
        return in_;
    }

    IN* in_end()
    {
        return in_ + in_size();
    }

    std::size_t out_size() const
    {
        return sizey<OUT>(size_);
    }

    OUT* out_begin()
    {
        return out_;
    }

    OUT* out_end()
    {
        return out_ + out_size();
    }

    bool isfinite()
    {
        bool result = true;
        for (auto it = in_begin(); it != in_end(); ++it)
        {
            if (!my_isfinite(*it))
            {
                std::cerr << "INPUT Index " << std::distance(in_begin(), it) << " out of "
                          << in_size() << " isn't finite: " << *it << "\n";
                result = false;
            }
        }

        for (auto it = out_begin(); it != out_end(); ++it)
        {
            if (!my_isfinite(*it))
            {
                std::cerr << "OUTPUT Index " << std::distance(out_begin(), it) << " out of "
                          << out_size() << " isn't finite: " << *it << "\n";
                result = false;
            }
        }

        return result;
    }

protected:
    std::size_t size_;
    IN* in_ = nullptr;
    OUT* out_ = nullptr;
    fftw_plan plan_;
};

class FFT : public FFTBase<double, complex_type>
{
public:
    FFT(std::size_t size) : FFTBase(size)
    {
        assert(in_);
        assert(out_);
        plan_ =
            fftw_plan_dft_r2c_1d(size_, in_, reinterpret_cast<fftw_complex*>(out_), FFTW_ESTIMATE);
        assert(plan_);
    }
};

class IFFT : public FFTBase<complex_type, double>
{
public:
    IFFT(std::size_t size) : FFTBase(size)
    {
        assert(in_);
        assert(out_);
        plan_ =
            fftw_plan_dft_c2r_1d(size_, reinterpret_cast<fftw_complex*>(in_), out_, FFTW_ESTIMATE);

        assert(plan_);
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
      ifft_(extended_size_), tmp_(sizey<complex_type>(extended_size_))
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
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               sizey<complex_type>(extended_size_));

        std::copy(fft_.out_begin(), fft_.out_end(), tmp_.begin());

        fft_(std::reverse_iterator(right_end), std::reverse_iterator(right_begin));
        if (!fft_.isfinite())
        {
            throw std::runtime_error("right is not finite");
        }
        assert(std::distance(fft_.out_begin(), fft_.out_end()) ==
               sizey<complex_type>(extended_size_));

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
            if (!std::isfinite(a))
            {
                std::cerr << a << " isn't finite" << std::endl;
                return true;
            }
            if (!std::isfinite(b))
            {
                std::cerr << b << " isn't finite" << std::endl;
                return false;
            }
            return a < b;
        });

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
    std::size_t size_;
    std::size_t extended_size_;
    FFT fft_;
    IFFT ifft_;
    std::vector<complex_type> tmp_;
};
