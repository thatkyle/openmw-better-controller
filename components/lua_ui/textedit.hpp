#ifndef OPENMW_LUAUI_TEXTEDIT
#define OPENMW_LUAUI_TEXTEDIT

#include <MyGUI_EditBox.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaTextEdit : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaTextEdit)

        protected:
            void initialize() override;
            void deinitialize() override;
            void updateProperties() override;
            void updateCoord() override;
            void updateChildren() override;
            MyGUI::IntSize calculateSize() override;

        private:
            void textChange(MyGUI::EditBox*);

            MyGUI::EditBox* mEditBox = nullptr;
            bool mMultiline{0};
            bool mAutoSize{0};
    };
}

#endif // OPENMW_LUAUI_TEXTEDIT
