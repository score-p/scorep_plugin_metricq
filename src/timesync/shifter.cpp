#include "shifter.hpp"

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

Shifter::Shifter(std::size_t size, const std::string& tag)
: tag_(tag), size_(size), extended_size_(next_power_of_2(2 * size_ - 1)), fft_(extended_size_),
  ifft_(extended_size_), tmp_(type_size<complex_type>(extended_size_))
{
}
