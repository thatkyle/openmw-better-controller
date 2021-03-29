#ifndef MWGUI_CONTROLLEGEND_H
#define MWGUI_CONTROLLEGEND_H

#include <vector>
#include <string>

#include "windowbase.hpp"
#include "mode.hpp"

#include "../mwinput/actions.hpp"

namespace MWGui
{

    struct MenuControl
    {
        MWInput::MenuAction action;
        std::string caption;
    };

    struct ControlSet
    {
        std::vector<MenuControl> leftControls;
        std::vector<MenuControl> rightControls;
    };

    class ControlLegend : public WindowBase
    {
    public:
        ControlLegend();

        void pushControls(std::vector<MenuControl>& leftControls, std::vector<MenuControl>& rightControls);
        void popControls();

    private:
        void updateControls();
        std::string gamepadActionToIcon(MWInput::MenuAction action);

        std::vector<ControlSet> mControlSetStack;
    };
}

#endif
