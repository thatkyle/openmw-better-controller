#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "windowpinnablebase.hpp"

#include "spellmodel.hpp"

namespace MWGui
{
    class SpellIcons;
    class SpellView;

    class SpellWindow : public WindowPinnableBase, public NoDrop
    {
    public:
        SpellWindow(DragAndDrop* drag);
        virtual ~SpellWindow();

        void updateSpells();

        void onFrame(float dt) override;

        /// Cycle to next/previous spell
        void cycle(bool next);

        void focus();

    protected:
        MyGUI::Widget* mEffectBox;

        std::string mSpellToDelete;

        void onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped);
        void onSpellSelected(const std::string& spellId);
        void onModelIndexSelected(SpellModel::ModelIndex index);
        void onFilterChanged(MyGUI::EditBox *sender);
        void onDeleteClicked(MyGUI::Widget *widget);
        void onDeleteSpellAccept();
        void askDeleteSpell(const std::string& spellId);

        void onPinToggled() override;
        void onTitleDoubleClicked() override;
        void onOpen() override;

        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

        SpellView* mSpellView;
        SpellIcons* mSpellIcons;
        MyGUI::EditBox* mFilterEdit;

    protected:

        MyGUI::IntSize highlightSizeOverride() override;
        MyGUI::IntCoord highlightOffset() override;

        ControlSet getControlLegendContents() override;

    private:
        void gamepadHighlightSelected();
        void onFocusGained(MyGUI::Widget* sender, MyGUI::Widget* oldFocus);
        void onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus);

        float mUpdateTimer;
        int mGamepadSelected;
    };
}

#endif
