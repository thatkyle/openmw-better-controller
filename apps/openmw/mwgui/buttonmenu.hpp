#ifndef MWGUI_BUTTONMENU_H
#define MWGUI_BUTTONMENU_H

#include <vector>
#include <map>
#include <MyGUI_Button.h>

#include "widgets.hpp"

#include "windowbase.hpp"

namespace MWGui
{
    class ButtonMenu : public WindowModal
    {
    public:
        ButtonMenu(const std::string& parLayout);

        void onOpen() override;

    protected:
        void registerButtonPress(MyGUI::Button* button, MyGUI::delegates::DelegateFunction< MyGUI::Widget* >* _del);
        void registerButtons(std::vector<MyGUI::Button*> buttons, bool areButtonsVertical);
        void highlightSelectedButton();

        ControlSet getControlLegendContents() override;
        MyGUI::IntCoord highlightOffset() override { return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8)); };
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

    private:
        void removeButton(MyGUI::Widget* _sender);

        bool hasOkButton();
        bool hasBackOrCancelButton();
        MyGUI::Button* getOkButton();
        MyGUI::Button* getBackOrCancelButton();
        MyGUI::Button* getButtonMatchingGmst(std::vector<std::string> gmsts);

        std::vector<MyGUI::Button*> mButtonMenuButtons;

        std::map< MyGUI::Button*, MyGUI::delegates::DelegateFunction< MyGUI::Widget* >* > mButtonEvents;

        bool mAreButtonsVertical;
        int mHighlight;
    };

}

#endif
