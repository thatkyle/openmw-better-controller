#include "controllegend.hpp"

#include <string>
#include <utility>
#include <vector>

#include <MyGUI_RenderManager.h>

#include "mode.hpp"
#include "../mwinput/actions.hpp"

namespace MWGui
{

    ControlLegend::ControlLegend()
        : WindowBase("openmw_control_legend.layout")
    {
        MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos;
        pos.left = (gameWindowSize.width - mMainWidget->getWidth()) / 2;
        pos.top = (gameWindowSize.height - mMainWidget->getHeight() - 30 - 48);

        mMainWidget->setPosition(pos);
    }

    /*void ControlLegend::setControls(std::vector<std::pair<MWInput::MenuAction, std::string>> &leftActions,
                                    std::vector<std::pair<MWInput::MenuAction, std::string>> &rightActions)
    {
        for (auto &action : leftActions)
        {
            
        }
    }*/

    void ControlLegend::updateControls(GuiMode mode) 
    {
        this->setVisible(true);

        switch (mode)
        {
        case GM_Dialogue:
            setText("LeftControls", "(A) Ask");
            setText("RightControls", "(B) Back");
            return;
        }

        this->setVisible(false);
    }

    void ControlLegend::updateControls(GuiWindow inventoryWindow)
    {

    }

    std::string gamepadActionToIcon(MWInput::MenuAction action)
    {
        return "";
    }

}
