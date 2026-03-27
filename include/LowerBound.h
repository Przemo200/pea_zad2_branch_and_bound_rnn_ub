#ifndef LOWERBOUND_H
#define LOWERBOUND_H

#include "BranchAndBoundNode.h"
#include "TSPInstance.h"

namespace LowerBound {
    int compute(const TSPInstance& instance, const BranchAndBoundNode& node);
}

#endif
