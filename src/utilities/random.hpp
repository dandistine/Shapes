#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <numeric>
#include <span>
#include <vector>


namespace utilities::random
{
    class uniform_int_distribution;

    /// @brief Generate a float or double between 0 and 1
    /// @tparam Generator 
    /// @tparam rType 
    /// @param gen 
    /// @return 
    template<typename Generator, typename rType = double>
    rType generate_canonical(Generator& gen) {
        double multiplier {0x1.0p-53};
        return static_cast<rType>((gen() >> 11) * multiplier);
    }

    /// @brief Generate a float or double between begin and end
    /// @tparam fType 
    template<typename fType>
    class uniform_real_distribution {
        public:
        const fType min;
        const fType range;

        uniform_real_distribution(fType begin, fType end) : min(begin), range(end - begin) {};

        template<typename Generator>
        fType operator()(Generator& gen) {
            fType x = generate_canonical<Generator, fType>(gen);
            return (x * range) + min;
        }
    };


    /// @brief Discrete distribution based off of walker's alias method
    /// @details https://en.wikipedia.org/wiki/Alias_method
    /// @tparam T 
    template<typename T>
    class discrete_distribution {

        struct alias {
            double probability{};
            size_t alternate{};
        };

        std::vector<alias> aliases;
        public:
        discrete_distribution(std::span<double> weights) {
            const auto len = weights.size();
            std::vector<double> probabilities(weights.size());
            //probabilities.reserve(len);
            aliases.reserve(len);
            const auto sum = std::accumulate(weights.begin(), weights.end(), 0.0);

            std::transform(weights.begin(), weights.end(), probabilities.begin(), [=](auto v){return (v * len) / sum;});

            std::vector<size_t> overfull;
            std::vector<size_t> underfull;

            // Setup the overfull and underfull vectors
            for(size_t i = 0; i < probabilities.size(); i++) {
                const auto& p = probabilities[i];
                aliases.push_back({p, 0u});

                if(p < 1.0) {
                    underfull.push_back(i);
                } else {
                    overfull.push_back(i);
                }
            }

            while(!underfull.empty() && !overfull.empty()) {
                auto low = underfull.back();
                auto high = overfull.back();
                underfull.pop_back();
                overfull.pop_back();

                // Assign any extra probability from "high" to the alternate of "low", filling its "total probability" to 1 in the process
                aliases[low].alternate = high;
                const auto rem = aliases[high].probability - (1.0 - aliases[low].probability);
                // Assign the remaining probability back to the "high"
                aliases[high].probability = rem;

                if(rem < 1.0) {
                    underfull.push_back(high);
                } else {
                    overfull.push_back(high);
                }
            }

            // Due to rounding errors, there is probably 1 item left over that *isn't quite* at 1.0, fudge it a bit
            for(auto i : overfull) {
                aliases[i] = {1.0, i};
            }

            for(auto i : underfull) {
                aliases[i] = {1.0, i};
            }
        };

        template<typename Generator>
        T operator()(Generator& gen) {
            const auto index = uniform_int_distribution{0, aliases.size()}(gen);
            const auto alias = aliases[index];
            if (generate_canonical(gen) < alias.probability) {
                return index;
            } else {
                return alias.alternate;
            }
        }
    };

    class uniform_int_distribution
    {
    public:
        const uint64_t min;
        const uint64_t range;
        uniform_int_distribution(uint64_t begin, uint64_t end) : min(begin), range(end - begin) {};

        template <typename Generator>
        uint64_t operator()(Generator& gen)
        {
            uint64_t x = gen();
            std::cout << x << std::endl;
            __uint128_t m = static_cast<__uint128_t>(x) * static_cast<__uint128_t>(range);
            uint64_t l = static_cast<uint64_t>(m);

            if (l < range)
            {
                uint64_t t = -range % range;
                while (l < t)
                {
                    x = gen();
                    m = static_cast<__uint128_t>(x) * static_cast<__uint128_t>(range);
                    l = static_cast<uint64_t>(m);
                }
            }
            uint64_t result = (m >> 64) + min;
            std::cout << result << std::endl;
            return result;
        }
    };

    template<typename T>
    class PertDistribution {
    public:
        PertDistribution(T a, T b, T c) : a(a), b(b), c(c) {
            alpha = 1 + (4.0f * (b - a))/(c - a);
            beta = 1 + (4.0f * (c - b))/(c - a);
            auto C = (std::tgamma(alpha) * std::tgamma(beta)) / std::tgamma(alpha + beta);
            denom = C * std::pow(c - a, alpha + beta - 1);

            //Everything is initialized, calculate the max
            std::cout << "Pert " << a << " " << b << " " << c << std::endl;
            max = (*this)(b);
        };

        template<typename Generator>
        T operator()(Generator& gen) {
            while(1) {
                const T x = (c - a) * generate_canonical(gen) + a;
                const T y = generate_canonical(gen) * max;
                if (y < (*this)(x)) {
                    return x;
                }
            }
        }

        T operator()(T x) {
            return std::pow(x - a, alpha - 1) * std::pow(c - x, beta - 1) / denom;
        }

    private:
        T a;
        T b;
        T c;
        T alpha;
        T beta;
        T denom;
        T max;
    };

    // template <typename RandomBitGenerator>
    // static inline uint64_t random_bounded_nearlydivisionless64(uint64_t range, RandomBitGenerator& gen)
    // {
    //     __uint128_t random64bit, multiresult;
    //     uint64_t leftover;
    //     uint64_t threshold;
    //     random64bit = gen();
    //     multiresult = random64bit * range;
    //     leftover = (uint64_t)multiresult;
    //     if (leftover < range)
    //     {
    //         threshold = -range % range;
    //         while (leftover < threshold)
    //         {
    //             random64bit = gen();
    //             multiresult = random64bit * range;
    //             leftover = (uint64_t)multiresult;
    //         }
    //     }
    //     return multiresult >> 64; // [0, range)
    // };
}