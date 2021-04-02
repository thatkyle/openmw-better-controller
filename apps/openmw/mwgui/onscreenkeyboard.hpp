#ifndef MWGUI_ONSCREENKEYBOARD_H
#define MWGUI_ONSCREENKEYBOARD_H

#include "windowbase.hpp"

namespace MWGui
{
    class OnscreenKeyboard : public WindowModal
    {
    public:
        OnscreenKeyboard();

        void onOpen() override;
        void onClose() override;

    private:
        MyGUI::EditBox* mInput;
    };
}

#endif
