#ifndef OPENMW_GAME_VIRTUALKEYBOARD_H
#define OPENMW_GAME_VIRTUALKEYBOARD_H

#include "windowbase.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
//#include "components/widgets/virtualkeyboardmanager.hpp"

#include <map>

//namespace Gui
//{
//    class VirtualKeyboardManager;
//}

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
        // divided into three sets:
        // 0: the spell name
        // 1 to n: the available spell effects
        // n+1 to m: the spell effects added to the current spell
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
        std::vector<std::vector<std::string>> mButtonRows;
        std::map<std::string, MyGUI::Button*> mButtons;
        bool mShift;
        bool mCaps;

        std::function<void()> mOnAccept;
    };

    //class VirtualKeyboardManager : public Gui::VirtualKeyboardManager
    //{
    //public:
    //    VirtualKeyboardManager();

    //    void registerEditBox(MyGUI::EditBox* editBox) override;
    //    void unregisterEditBox(MyGUI::EditBox* editBox) override;
    //    VirtualKeyboard& virtualKeyboard() { return *mVk; };

    //private:
    //    std::unique_ptr<VirtualKeyboard>   mVk;

    //    // MyGUI deletes delegates when you remove them from an event.
    //    // Therefore i need one pair of delegates per box instead of being able to reuse one pair.
    //    // And i have to set them aside myself to know what to remove from each event.
    //    // There is an IDelegateUnlink type that might simplify this, but it is poorly documented.
    //    using IDelegate = MyGUI::EventHandle_WidgetWidget::IDelegate;
    //    // .first = onSetFocus, .second = onLostFocus
    //    using Delegates = std::pair<IDelegate*, IDelegate*>;
    //    std::map<MyGUI::EditBox*, Delegates> mDelegates;
    //};
}

#endif
