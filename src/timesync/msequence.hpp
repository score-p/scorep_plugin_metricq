#pragma once

#include <initializer_list>
#include <iostream>
#include <optional>
#include <type_traits>

#include <cassert>

class BinaryMSequenceIter
{
public:
    using ValueType = int;

private:
    static ValueType compute_coeffs(const std::vector<int>& coefficient_indices)
    {
        assert(coefficient_indices.size() > 0);
        int n = *coefficient_indices.begin();
        ValueType coeffs = 0;
        for (auto index : coefficient_indices)
        {
            assert(n >= index);
            coeffs |= 1 << (n - index);
        }
        return coeffs;
    }

    static std::vector<int> get_coefficient_indices(int n)
    {
        switch (n)
        {
        case 3:
            return { 3, 1 };
        case 4:
            return { 4, 1 };
        case 5:
            return { 5, 2 };
        case 6:
            return { 6, 1 };
        case 7:
            return { 7, 1 };
        case 8:
            return { 8, 6, 5, 1 };
        case 9:
            return { 9, 4 };
        case 10:
            return { 10, 3 };
        case 11:
            return { 11, 2 };
        case 12:
            return { 12, 7, 4, 3 };
        case 13:
            return { 13, 4, 3, 1 };
        case 14:
            return { 14, 12, 11, 1 };
        default:
            throw std::runtime_error("Unsupported sequence length");
        }
    }

    BinaryMSequenceIter(int n, std::vector<int> coefficient_indices)
    : BinaryMSequenceIter(n, compute_coeffs(coefficient_indices))
    {
    }

    BinaryMSequenceIter(int n, ValueType coeffs) : n_(n), coeffs_(coeffs)
    {
    }

public:
    BinaryMSequenceIter(int n) : BinaryMSequenceIter(n, get_coefficient_indices(n))
    {
    }

    bool operator*()
    {
        return __builtin_parity(reg_ & coeffs_);
    }

    bool operator==(const BinaryMSequenceIter& other)
    {
        assert(coeffs_ == other.coeffs_);
        return (reg_ == other.reg_) && (overflow_ == other.overflow_);
    }

    BinaryMSequenceIter& operator++()
    {
        ValueType feedback = **this;
        reg_ = reg_ >> 1 | feedback << (n_ - 1);
        if (reg_ == 0b1)
        {
            overflow_++;
        }
        return *this;
    }

    operator bool()
    {
        return !overflow_;
    }

private:
    int n_;
    ValueType coeffs_;
    ValueType reg_ = 0b1;
    int overflow_ = 0;
};

class GroupedBinaryMSequence
{
public:
    GroupedBinaryMSequence(int n) : underlying_iter_(n)
    {
    }

    std::optional<std::pair<bool, int>> take()
    {
        if (!underlying_iter_)
        {
            return {};
        }
        bool level = *underlying_iter_;
        int count = 0;
        do
        {
            ++underlying_iter_;
            ++count;
        } while (underlying_iter_ && level == *underlying_iter_);
        return std::make_pair(level, count);
    }

private:
    BinaryMSequenceIter underlying_iter_;
};
