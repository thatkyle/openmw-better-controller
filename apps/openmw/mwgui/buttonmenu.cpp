#include "buttonmenu.hpp"

#include <MyGUI_LanguageManager.h>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "controllegend.hpp"

namespace MWGui
{

    ButtonMenu::ButtonMenu(const std::string& parLayout)
        : WindowModal(parLayout)
    {
        mUsesHighlightOffset = true;
    }

    void ButtonMenu::onOpen()
    {
        WindowModal::onOpen();
        highlightSelectedButton();
    }

    void ButtonMenu::registerButtonPress(MyGUI::Button* button, MyGUI::delegates::DelegateFunction< MyGUI::Widget* >* _del)
    {
        // register the button click for mouse use
        button->eventMouseButtonClick += _del;
        
        // add the same event to our map so we can invoke it with the joystick
        mButtonEvents[button] = _del;
    }

    void ButtonMenu::registerButtons(std::vector<MyGUI::Button*> buttons, bool areButtonsVertical)
    {
        mButtonMenuButtons = buttons;
        mAreButtonsVertical = areButtonsVertical;
        mHighlight = 0;

        for (auto button : mButtonMenuButtons)
        {
            // remove the buttons from tthe list on destruction to avoid referencing deleted memory
            button->eventWidgetDestroyed += MyGUI::newDelegate(this, &ButtonMenu::removeButton);

            // listen on key button presses for joystick use
            button->eventKeyButtonPressed += MyGUI::newDelegate(this, &ButtonMenu::onKeyButtonPressed);
        }

        if (hasOkButton())
        {
            auto it = std::find(mButtonMenuButtons.begin(), mButtonMenuButtons.end(), getOkButton());
            mHighlight = it - mButtonMenuButtons.begin();
        }

    }

    void ButtonMenu::removeButton(MyGUI::Widget* _sender)
    {
        auto button = dynamic_cast<MyGUI::Button*>(_sender);
        if (button == nullptr)
            return;

        auto it = mButtonMenuButtons.begin();
        while (it != mButtonMenuButtons.end())
        {
            if (*it == button)
            {
                mButtonMenuButtons.erase(it);
                if (mButtonEvents.count(*it))
                    mButtonEvents.erase(*it);
                return;
            }

            it++;
        }
    }

    void ButtonMenu::highlightSelectedButton()
    {
        if (mButtonMenuButtons.empty())
            widgetHighlight(nullptr);
        else
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonMenuButtons[mHighlight]);
            widgetHighlight(mButtonMenuButtons[mHighlight]);
        }
    }

    void ButtonMenu::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        if (action == MWInput::MA_A) // select button
        {
            if (mButtonEvents.count(mButtonMenuButtons[mHighlight]))
                mButtonEvents[mButtonMenuButtons[mHighlight]]->invoke(mButtonMenuButtons[mHighlight]);
        }
        else if (action == MWInput::MA_B && hasBackOrCancelButton()) // back
        {
            auto backButton = getBackOrCancelButton();
            if (mButtonEvents.count(backButton))
                mButtonEvents[backButton]->invoke(backButton);
        }
        else if (action == MWInput::MA_DPadUp && mAreButtonsVertical)
        {
            mHighlight = std::max(mHighlight - 1, 0);
            highlightSelectedButton();
        }
        else if (action == MWInput::MA_DPadDown && mAreButtonsVertical)
        {
            mHighlight = std::min(mHighlight + 1, static_cast<int>(mButtonMenuButtons.size() - 1));
            highlightSelectedButton();
        }
        else if (action == MWInput::MA_DPadLeft && !mAreButtonsVertical)
        {
            mHighlight = std::max(mHighlight - 1, 0);
            highlightSelectedButton();
        }
        else if (action == MWInput::MA_DPadRight && !mAreButtonsVertical)
        {
            mHighlight = std::min(mHighlight + 1, static_cast<int>(mButtonMenuButtons.size() - 1));
            highlightSelectedButton();
        }
    }

    ControlSet ButtonMenu::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"}
            },
            hasBackOrCancelButton() ? 
                std::vector<MenuControl>{MenuControl{MWInput::MenuAction::MA_B, "Back"}} : 
                std::vector<MenuControl>{}
        };
        
    }

    MyGUI::Button* ButtonMenu::getBackOrCancelButton()
    {
        return getButtonMatchingGmst({ "sNo", "sBack", "sCancel" });
    }

    MyGUI::Button* ButtonMenu::getOkButton()
    {
        return getButtonMatchingGmst({ "sOk", "sYes" });
    }

    bool ButtonMenu::hasBackOrCancelButton()
    {
        return getBackOrCancelButton() != nullptr;
    }

    bool ButtonMenu::hasOkButton()
    {
        return getOkButton() != nullptr;
    }

    MyGUI::Button* ButtonMenu::getButtonMatchingGmst(std::vector<std::string> gmstsToMatch)
    {
        for (MyGUI::Button* button : mButtonMenuButtons)
        {
            for (const std::string& gmst : gmstsToMatch)
            {
                std::string gmstTranslation = MWBase::Environment::get().getWindowManager()->getGameSettingString(gmst, "");
                std::string buttonCaption = button->getCaption().asUTF8();
                if (Misc::StringUtils::ciEqual(
                    gmstTranslation,
                    buttonCaption))
                {
                    return button;
                }
            }
        }

        return nullptr;
    }
}
