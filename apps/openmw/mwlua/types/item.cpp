#include <sol/sol.hpp>

#include "../../mwworld/class.hpp"

#include "types.hpp"

namespace MWLua
{
    void addItemBindings(sol::table item)
    {
        item["getEnchantmentCharge"]
            = [](const Object& object) { return object.ptr().getCellRef().getEnchantmentCharge(); };
        item["setEnchantmentCharge"]
            = [](const GObject& object, float charge) { object.ptr().getCellRef().setEnchantmentCharge(charge); };
        item["isRestocking"]
            = [](const Object& object) -> bool { return object.ptr().getRefData().getCount(false) < 0; };
    }
}
