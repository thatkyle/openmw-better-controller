﻿#include "controllegend.hpp"

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

    void ControlLegend::pushControls(std::vector<MenuControl>& leftControls,
                                     std::vector<MenuControl>& rightControls)
    {
        mControlSetStack.push_back(ControlSet{ leftControls, rightControls });
        updateControls();
    }

    void ControlLegend::popControls()
    {
        if (!mControlSetStack.empty())
            mControlSetStack.pop_back();

        updateControls();
    }

    void ControlLegend::updateControls()
    {
        if (mControlSetStack.empty())
        {
            this->setVisible(false);
        }
        else
        {
            std::string leftString = "";
            for (auto& control : mControlSetStack.back().leftControls)
            {
                leftString += gamepadActionToIcon(control.action) + " " + control.caption + "   ";
            }

            std::string rightString = "";
            for (auto& control : mControlSetStack.back().rightControls)
            {
                rightString += "   " + gamepadActionToIcon(control.action) + " " + control.caption;
            }

            setText("LeftControls", leftString);
            setText("RightControls", rightString);
            this->setVisible(true);
        }
        
    }

    std::string ControlLegend::gamepadActionToIcon(MWInput::MenuAction action)
    {
        // TODO: create DDS icons for each controller type, fallback to text like this if the controller type is unrecognized
        switch (action)
        {
        case MWInput::MenuAction::MA_A:
            return "(A)";
        case MWInput::MenuAction::MA_B:
            return "(B)";
        case MWInput::MenuAction::MA_X:
            return "(X)";
        case MWInput::MenuAction::MA_Y:
            return "(Y)";
        case MWInput::MenuAction::MA_RTrigger:
            return "(RT)";
        case MWInput::MenuAction::MA_LTrigger:
            return "(LT)";
        case MWInput::MenuAction::MA_DPadUp:
            return "(↑)";
        case MWInput::MenuAction::MA_DPadDown:
            return "(↓)";
        case MWInput::MenuAction::MA_DPadLeft:
            return "(←)";
        case MWInput::MenuAction::MA_DPadRight:
            return "(→)";
        case MWInput::MenuAction::MA_JSRightClick:
            return "(LS-↓)";
        case MWInput::MenuAction::MA_JSLeftClick:
            return "(RS-↓)";
        case MWInput::MenuAction::MA_Start:
            return "(START)";
        case MWInput::MenuAction::MA_Select:
            return "(SELECT)";
        case MWInput::MenuAction::MA_White:
            // we can say white and black here... but if you're playing with a modern controller it's LB and RB
            return "(LB)";
        case MWInput::MenuAction::MA_Black:
            return "(RB)";
        default:
            // TODO: throw an exception here; we shouldn't get any unknown menu actions
            return "";
        }
        return "";
    }

}
