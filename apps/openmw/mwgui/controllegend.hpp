#ifndef MWGUI_CONTROLLERLEGEND_H
#define MWGUI_CONTROLLERLEGEND_H

#include "windowbase.hpp"
#include "mode.hpp"

namespace MWGui
{
    class ControlLegend : public WindowBase
    {
    public:
        ControlLegend();

        void updateControls(GuiMode mode);
        void updateControls(GuiWindow inventoryWindow);
    };
}

#endif
