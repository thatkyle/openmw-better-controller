#include "cellid.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    const std::string CellId::sDefaultWorldspace = "sys::default";

    void CellId::load(ESMReader& esm)
    {
        mWorldspace = esm.getHNString("SPAC");

        if (esm.isNextSub("CIDX"))
        {
            esm.getHTSized<8>(mIndex);
            mPaged = true;
        }
        else
        {
            mPaged = false;
            mIndex.mX = 0;
            mIndex.mY = 0;
        }
    }

    void CellId::save(ESMWriter& esm) const
    {
        esm.writeHNString("SPAC", mWorldspace);

        if (mPaged)
            esm.writeHNT("CIDX", mIndex, 8);
    }

    ESM::RefId CellId::getCellRefId() const
    {
        if (mPaged)
        {
            return ESM::RefId::stringRefId("#" + std::to_string(mIndex.mX) + "," + std::to_string(mIndex.mY));
        }
        else
        {
            return ESM::RefId::stringRefId(mWorldspace);
        }
    }

    bool operator==(const CellId& left, const CellId& right)
    {
        return left.mWorldspace == right.mWorldspace && left.mPaged == right.mPaged
            && (!left.mPaged || (left.mIndex.mX == right.mIndex.mX && left.mIndex.mY == right.mIndex.mY));
    }

    bool operator!=(const CellId& left, const CellId& right)
    {
        return !(left == right);
    }

    bool operator<(const CellId& left, const CellId& right)
    {
        if (left.mPaged < right.mPaged)
            return true;
        if (left.mPaged > right.mPaged)
            return false;

        if (left.mPaged)
        {
            if (left.mIndex.mX < right.mIndex.mX)
                return true;
            if (left.mIndex.mX > right.mIndex.mX)
                return false;

            if (left.mIndex.mY < right.mIndex.mY)
                return true;
            if (left.mIndex.mY > right.mIndex.mY)
                return false;
        }

        return left.mWorldspace < right.mWorldspace;
    }

}
