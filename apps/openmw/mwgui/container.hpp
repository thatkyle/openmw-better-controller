#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include "../mwinput/actions.hpp"

#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include "itemmodel.hpp"

namespace MyGUI
{
    class Gui;
    class Widget;
}

namespace MWGui
{
    class ContainerWindow;
    class ItemView;
    class SortFilterItemModel;
}

namespace MWGui
{
    class ContainerWindow : public WindowBase, public ReferenceInterface
    {
    public:
        ContainerWindow(DragAndDrop* dragAndDrop);

        void setPtr(const MWWorld::Ptr& container) override;
        void onClose() override;
        void clear() override { resetReference(); }

        void onFrame(float dt) override { checkReferenceAvailable(); }

        void resetReference() override;

        void onDeleteCustomData(const MWWorld::Ptr& ptr) override;

        void treatNextOpenAsLoot() { mTreatNextOpenAsLoot = true; }

        std::string_view getWindowIdForLua() const override { return "Container"; }


        void focus() override;
        void onBackgroundSelected();

    protected:
        ControlSet getControlLegendContents() override;

    private:
        DragAndDrop* mDragAndDrop;

        MWGui::ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        ItemModel* mModel;
        int mSelectedItem;
        bool mTreatNextOpenAsLoot;
        MyGUI::Button* mDisposeCorpseButton;
        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        void onItemSelected(int index);
        void dragItem(MyGUI::Widget* sender, int count);
        void dropItem();
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeAllButtonClicked(MyGUI::Widget* _sender);
        void onDisposeCorpseButtonClicked(MyGUI::Widget* sender);

        /// @return is taking the item allowed?
        bool onTakeItem(const ItemStack& item, int count);

        void onReferenceUnavailable() override;
        
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);
        void onFocusGained(MyGUI::Widget* sender, MyGUI::Widget* oldFocus);
        void onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus);
        void gamepadDelayedAction();
        void gamepadHighlightSelected();
        //void gamepadCycleFilter(MWInput::MenuAction action);
        int mGamepadSelected;
        int mGamepadFilterSelected;
        bool isFilterCycleMode;
        MWInput::MenuAction mLastAction;
    };
}
#endif // CONTAINER_H
