#pragma once

#include "systems/system.hpp"

struct BossFactory {   
    virtual std::unique_ptr<System> GetLeadInSystem(int power) const = 0;
    virtual std::unique_ptr<System> GetBossSystem(int power) const = 0;
    virtual std::unique_ptr<System> GetLeadOutSystem(int power) const = 0;

    virtual ~BossFactory() = default;
};
