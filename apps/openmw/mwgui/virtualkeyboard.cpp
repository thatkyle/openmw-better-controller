#include "VirtualKeyboard.hpp"

#include <algorithm>

#include <MyGUI_InputManager.h>
#include <MyGUI_LayerManager.h>
#include <MyGUI_RenderManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "controllegend.hpp"

namespace MWGui
{
    /*VirtualKeyboardManager::VirtualKeyboardManager()
        : mVk(new VirtualKeyboard)
    {

    }

    void VirtualKeyboardManager::registerEditBox(MyGUI::EditBox* editBox)
    {
        IDelegate* onSetFocusDelegate = newDelegate(mVk.get(), &VirtualKeyboard::delegateOnSetFocus);
        IDelegate* onLostFocusDelegate = newDelegate(mVk.get(), &VirtualKeyboard::delegateOnLostFocus);
        editBox->eventKeySetFocus += onSetFocusDelegate;
        editBox->eventKeyLostFocus += onLostFocusDelegate;

        mDelegates[editBox] = Delegates(onSetFocusDelegate, onLostFocusDelegate);
    }

    void VirtualKeyboardManager::unregisterEditBox(MyGUI::EditBox* editBox)
    {
        auto it = mDelegates.find(editBox);
        if (it != mDelegates.end())
        {
            editBox->eventKeySetFocus -= it->second.first;
            editBox->eventKeyLostFocus -= it->second.second;
            mDelegates.erase(it);
        }
    }*/


    static const char* mClassTypeName;

    VirtualKeyboard::VirtualKeyboard()
        : WindowModal("openmw_virtual_keyboard.layout")
        , mButtonBox(nullptr)
        , mTarget(nullptr)
        , mButtons()
        , mShift(false)
        , mCaps(false)
        , mHighlightRow(0)
        , mHighlightColumn(0)
    {
        getWidget(mButtonBox, "ButtonBox");
        mMainWidget->setNeedKeyFocus(false);
        mButtonBox->setNeedKeyFocus(false);
        mButtonBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &VirtualKeyboard::onKeyButtonPressed);
        updateMenu();

        mUsesHighlightOffset = true;
    }

    VirtualKeyboard::~VirtualKeyboard()
    {

    }

    void VirtualKeyboard::onResChange(int w, int h)
    {
        updateMenu();
    }

    void VirtualKeyboard::onFrame(float dt)
    {
        if (isVisible() && !MWBase::Environment::get().getInputManager()->joystickLastUsed())
        {
            // if we switched back to the keyboard, focus the edit box and exit the virtual keyboard
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTarget);
            close();
        }
    }

    void VirtualKeyboard::open(MyGUI::EditBox* target)
    {
        open(target, {});
    }

    void VirtualKeyboard::open(MyGUI::EditBox* target, const std::function<void()> onAccept)
    {
        mLastFocusedWidget = MyGUI::InputManager::getInstance().getKeyFocusWidget();

        mHighlightColumn = 0;
        mHighlightRow = 0;
        mOnAccept = onAccept;

        updateMenu();
        mTarget = target;
        mMainWidget->setPosition({ mTarget->getAbsolutePosition().left, mTarget->getAbsoluteRect().bottom + 2 });

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        if (mMainWidget->getAbsoluteRect().bottom > viewSize.height)
            mMainWidget->setPosition({ mTarget->getAbsolutePosition().left, mTarget->getAbsoluteRect().top - 2 - mMainWidget->getSize().height });

        setVisible(true);

        widgetHighlight(mButtons[mButtonRows[0][0]]);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonBox);
    }

    void VirtualKeyboard::close()
    {
        setVisible(false);
        mMainWidget->setPosition(0, 0);
        updateMenu();
        mTarget = nullptr;

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mLastFocusedWidget);
        mLastFocusedWidget = nullptr;
    }

    void VirtualKeyboard::delegateOnSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old)
    {
        open(static_cast<MyGUI::EditBox*>(_sender));
    }

    void VirtualKeyboard::delegateOnLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new)
    {
        close();
    }

    void VirtualKeyboard::onButtonClicked(MyGUI::Widget* sender)
    {
        assert(mTarget);

        std::string name = *sender->getUserData<std::string>();

        if (name == "Esc")
            onEsc();
        if (name == "Tab")
            onTab();
        if (name == "Caps")
            onCaps();
        if (name == "Shift")
            onShift();
        else
            mShift = false;
        if (name == "Back")
            onBackspace();
        if (name == "Return")
            onReturn();
        if (name == "Space")
            textInput(" ");
        if (name == "->")
            textInput("->");
        if (name.length() == 1)
            textInput(name);

        mMainWidget->setPosition(0, 0);
        updateMenu();       
        
        mMainWidget->setPosition({ mTarget->getAbsolutePosition().left, mTarget->getAbsoluteRect().bottom + 2 });

    }

    void VirtualKeyboard::textInput(const std::string& symbol)
    {
        mTarget->addText(symbol);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonBox);
    }

    void VirtualKeyboard::onEsc()
    {
        close();
    }

    void VirtualKeyboard::onTab()
    {
        mTarget->addText("\t");
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonBox);
    }

    void VirtualKeyboard::onCaps()
    {
        mCaps = !mCaps;
    }

    void VirtualKeyboard::onShift()
    {
        mShift = !mShift;
    }

    void VirtualKeyboard::onBackspace()
    {
        mTarget->eraseText(mTarget->getTextLength() - 1);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonBox);
    }

    void VirtualKeyboard::onReturn()
    {
        mTarget->addText("\n");
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtonBox);
    }

    bool VirtualKeyboard::exit()
    {
        close();
        return true;
    }

    void VirtualKeyboard::updateMenu()
    {

        // TODO: Localization?
        static std::vector<std::string> row1{ "`",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Back" };
        static std::vector<std::string> row2{ "Tab",   "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Return" };
        static std::vector<std::string> row3{ "Caps",  "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "\\", "->" };
        static std::vector<std::string> row4{ "Shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Space" };
        std::map<std::string, std::string> shiftMap;
        shiftMap["1"] = "!";
        shiftMap["2"] = "@";
        shiftMap["3"] = "#";
        shiftMap["4"] = "$";
        shiftMap["5"] = "%";
        shiftMap["6"] = "^";
        shiftMap["7"] = "&";
        shiftMap["8"] = "*";
        shiftMap["9"] = "(";
        shiftMap["0"] = ")";
        shiftMap["-"] = "_";
        shiftMap["="] = "+";
        shiftMap["\\"] = "|";
        shiftMap[","] = "<";
        shiftMap["."] = ">";
        shiftMap["/"] = "?";
        shiftMap[";"] = ":";
        shiftMap["'"] = "\"";
        shiftMap["["] = "{";
        shiftMap["]"] = "}";
        shiftMap["`"] = "~";

        mButtonRows = { row1, row2, row3, row4 };

        int sideSize = 25;
        int margin = 5;
        int xmax = 0;
        int ymax = 0;
        int xmin = 0;
        int ymin = 0;

        if (mButtons.empty())
        {
            int y = margin;
            for (auto& row : mButtonRows)
            {
                int x = margin;
                for (std::string& buttonId : row)
                {
                    int width = sideSize + 10 * (buttonId.length() - 1);
                    MyGUI::Button* button = mButtonBox->createWidget<MyGUI::Button>(
                        "MW_Button", MyGUI::IntCoord(x, y, width, sideSize), MyGUI::Align::Default, buttonId);
                    button->eventMouseButtonClick += MyGUI::newDelegate(this, &VirtualKeyboard::onButtonClicked);
                    button->setUserData(std::string(buttonId));
                    button->setVisible(true);
                    button->setFontHeight(16);
                    button->setCaption(buttonId);
                    button->setNeedKeyFocus(false);
                    mButtons[buttonId] = button;
                    x += width + margin;
                }
                y += sideSize + margin;
            }
        }

        for (auto& row : mButtonRows)
        {
            for (std::string& buttonId : row)
            {
                auto* button = mButtons[buttonId];
                xmax = std::max(xmax, button->getAbsoluteRect().right);
                ymax = std::max(ymax, button->getAbsoluteRect().bottom);
                xmin = std::min(xmin, button->getAbsoluteRect().left);
                ymin = std::min(ymin, button->getAbsoluteRect().top);

                if (buttonId.length() == 1)
                {
                    auto caption = buttonId;
                    if (mShift ^ mCaps)
                        caption[0] = std::toupper(caption[0]);
                    else
                        caption[0] = std::tolower(caption[0]);
                    button->setCaption(caption);
                    button->setUserData(caption);
                }

                if (mShift)
                {
                    auto it = shiftMap.find(buttonId);
                    if (it != shiftMap.end())
                    {
                        button->setCaption(it->second);
                        button->setUserData(it->second);
                    }
                }
            }
        }

        std::cout << xmax << ", " << ymax << std::endl;

        mButtonBox->setCoord(0, 0, xmax - xmin + margin, ymax - ymin + margin);
        mMainWidget->setSize(xmax - xmin + margin*2, ymax - ymin + margin*2);

        //mButtonBox->setCoord (margin, margin, width, height);
        mButtonBox->setVisible(true);

    }

    void VirtualKeyboard::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        if (action == MWInput::MA_B) // back
            close();
        else if (action == MWInput::MA_A) // select
            onButtonClicked(mButtons[mButtonRows[mHighlightRow][mHighlightColumn]]);
        else if (action == MWInput::MA_X) // backspace
            onBackspace();
        else if (action == MWInput::MA_Y) // space
            textInput(" ");
        else if (action == MWInput::MA_JSLeftClick) // caps
            onCaps();
        else if (action == MWInput::MA_Start) // accept
        {
            close();
            mOnAccept();
        }
        else if (action == MWInput::MA_DPadUp && mHighlightRow > 0)
        {
            mHighlightRow--;
            mHighlightColumn = std::min(mHighlightColumn, static_cast<unsigned int>(mButtonRows[mHighlightRow].size() - 1));
            widgetHighlight(mButtons[mButtonRows[mHighlightRow][mHighlightColumn]]);
        }
        else if (action == MWInput::MA_DPadDown && mHighlightRow < mButtonRows.size() - 1)
        {
            mHighlightRow++;
            mHighlightColumn = std::min(mHighlightColumn, static_cast<unsigned int>(mButtonRows[mHighlightRow].size() - 1));
            widgetHighlight(mButtons[mButtonRows[mHighlightRow][mHighlightColumn]]);
        }
        else if (action == MWInput::MA_DPadLeft && mHighlightColumn > 0)
        {
            mHighlightColumn--;
            widgetHighlight(mButtons[mButtonRows[mHighlightRow][mHighlightColumn]]);
        }
        else if (action == MWInput::MA_DPadRight && mHighlightColumn < mButtonRows[mHighlightRow].size() - 1)
        {
            mHighlightColumn++;
            widgetHighlight(mButtons[mButtonRows[mHighlightRow][mHighlightColumn]]);
        }
    }

    ControlSet VirtualKeyboard::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"},
                MenuControl{MWInput::MenuAction::MA_X, "Delete"},
                MenuControl{MWInput::MenuAction::MA_Y, "Space"}
            },
            {
                MenuControl{MWInput::MenuAction::MA_JSLeftClick, "Caps"},
                MenuControl{MWInput::MenuAction::MA_Start, "Accept"},
                MenuControl{MWInput::MenuAction::MA_B, "Back"}
            }
        };
    }
}
