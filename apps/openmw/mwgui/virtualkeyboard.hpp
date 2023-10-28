#ifndef OPENMW_GAME_VIRTUALKEYBOARD_H
#define OPENMW_GAME_VIRTUALKEYBOARD_H

#include "windowbase.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <map>

namespace MWGui
{
    class VirtualKeyboard : public WindowModal
    {
    public:

        VirtualKeyboard();
        ~VirtualKeyboard();

        void onResChange(int w, int h) override;

        void onFrame(float dt) override;

        bool exit() override;

        void open(MyGUI::EditBox* target);
        void open(MyGUI::EditBox* target, const std::function<void()> onAccept);

        void close();

        void delegateOnSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);
        void delegateOnLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);

    protected:
        MyGUI::IntCoord highlightOffset() override { return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8)); };

        ControlSet getControlLegendContents() override;
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

        unsigned int mHighlightRow;
        unsigned int mHighlightColumn;

    private:
        void onButtonClicked(MyGUI::Widget* sender);
        void textInput(const std::string& symbol);
        void onEsc();
        void onTab();
        void onCaps();
        void onShift();
        void onBackspace();
        void onReturn();
        void updateMenu();


        MyGUI::Widget* mButtonBox;
        MyGUI::EditBox* mTarget;
        MyGUI::Widget* mLastFocusedWidget;
        std::vector<std::vector<std::string>> mButtonRows;
        std::map<std::string, MyGUI::Button*> mButtons;
        bool mShift;
        bool mCaps;

        std::function<void()> mOnAccept;
    };
}

#endif
