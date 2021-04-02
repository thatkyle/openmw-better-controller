#include "onscreenkeyboard.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    OnscreenKeyboard::OnscreenKeyboard() 
        : WindowModal("openmw_onscreen_keyboard.layout")
    {
        getWidget(mInput, "Input");
    }

    void OnscreenKeyboard::onOpen()
    {
        WindowModal::onOpen();

        center();
    }

    void OnscreenKeyboard::onClose()
    {

    }
}
