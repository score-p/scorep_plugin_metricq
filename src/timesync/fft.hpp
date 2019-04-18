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

inline void check_finite(complex_type z)
{
    if (!(std::isfinite(z.real()) && std::isfinite(z.imag())))
    {
        throw std::runtime_error("Infinite complex value");
    }
}

inline void check_finite(double a)
{
    if (!std::isfinite(a))
    {
        throw std::runtime_error("Infinite value");
    }
}

template <typename T>
struct SizeHelper
{
    static std::size_t size(std::size_t s)
    {
        return s;
    }
};

// The complex vectors in the FFTW have less elements
template <>
struct SizeHelper<complex_type>
{
    static std::size_t size(std::size_t s)
    {
        return s / 2 + 1;
    }
};

template <typename T>
size_t type_size(size_t s)
{
    return SizeHelper<T>::size(s);
}

template <typename IN, typename OUT>
class FFTBase
{
public:
    FFTBase(std::size_t size)
    : size_(size), in_((IN*)fftw_malloc(sizeof(IN) * in_size())),
      out_((OUT*)fftw_malloc(sizeof(OUT) * out_size()))
    {
        assert(in_);
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
        return type_size<IN>(size_);
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
        return type_size<OUT>(size_);
    }

    OUT* out_begin()
    {
        return out_;
    }

    OUT* out_end()
    {
        return out_ + out_size();
    }

    void check_finite()
    {
        for (auto it = in_begin(); it != in_end(); ++it)
        {
            ::check_finite(*it);
        }
        for (auto it = out_begin(); it != out_end(); ++it)
        {
            ::check_finite(*it);
        }
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
