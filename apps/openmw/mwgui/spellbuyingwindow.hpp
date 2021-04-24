#ifndef MWGUI_SpellBuyingWINDOW_H
#define MWGUI_SpellBuyingWINDOW_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace ESM
{
    struct Spell;
}

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class SpellBuyingWindow : public ReferenceInterface, public WindowBase
    {
        public:
            SpellBuyingWindow();

            void setPtr(const MWWorld::Ptr& actor) override;
            void setPtr(const MWWorld::Ptr& actor, int startOffset);

            void onFrame(float dt) override { checkReferenceAvailable(); }
            void onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus);
            void clear() override { resetReference(); }

            void onResChange(int, int) override { center(); }

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;

            MyGUI::ScrollView* mSpellsView;

            std::map<MyGUI::Widget*, std::string> mSpellsWidgetMap;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onSpellButtonClick(MyGUI::Widget* _sender);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            void addSpell(const ESM::Spell& spell);
            void clearSpells();
            int mCurrentY;

            void updateLabels();

            void onReferenceUnavailable() override;

            bool playerHasSpell (const std::string& id);

            // Gamepad controls:
            void onKeyButtonPressed(MyGUI::Widget *sender, MyGUI::KeyCode key, MyGUI::Char character);
            std::vector<MyGUI::Widget*> mSpellWidgets;
            unsigned int mSpellHighlight;

        private:
            static bool sortSpells (const ESM::Spell* left, const ESM::Spell* right);

            void scrollToTarget();
    };
}

#endif
