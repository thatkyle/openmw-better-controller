#include "ornode.hpp"

#include <apps/opencs/model/filter/narynode.hpp>
#include <apps/opencs/model/filter/node.hpp>

namespace CSMWorld
{
    class IdTableBase;
}

CSMFilter::OrNode::OrNode(const std::vector<std::shared_ptr<Node>>& nodes)
    : NAryNode(nodes, "or")
{
}

bool CSMFilter::OrNode::test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const
{
    int size = getSize();

    for (int i = 0; i < size; ++i)
        if ((*this)[i].test(table, row, columns))
            return true;

    return false;
}
