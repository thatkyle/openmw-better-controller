#include "itemselection.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_TextBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "inventoryitemmodel.hpp"
#include "itemview.hpp"
#include "sortfilteritemmodel.hpp"

#include "controllegend.hpp"

namespace MWGui
{

    ItemSelectionDialog::ItemSelectionDialog(const std::string& label)
        : WindowModal("openmw_itemselection_dialog.layout")
        , mSortModel(nullptr)
        , mGamepadSelected(0)
    {
        getWidget(mItemView, "ItemView");
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ItemSelectionDialog::onSelectedItem);
        mItemView->eventKeyButtonPressed += MyGUI::newDelegate(this, &ItemSelectionDialog::onKeyButtonPressed);

        MyGUI::TextBox* l;
        getWidget(l, "Label");
        l->setCaptionWithReplacing(label);

        getWidget(mCancelButton, "CancelButton");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemSelectionDialog::onCancelButtonClicked);

        center();
    }

    void ItemSelectionDialog::onClose()
    {
        updateGamepadTooltip(nullptr);
        WindowModal::onClose();
    }

    bool ItemSelectionDialog::exit()
    {
        eventDialogCanceled();
        updateGamepadTooltip(nullptr);
        return true;
    }

    void ItemSelectionDialog::openContainer(const MWWorld::Ptr& container)
    {
        auto sortModel = std::make_unique<SortFilterItemModel>(std::make_unique<InventoryItemModel>(container));
        mSortModel = sortModel.get();
        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mItemView);
        gamepadHighlightSelected();
    }

    void ItemSelectionDialog::setCategory(int category)
    {
        mSortModel->setCategory(category);
        mItemView->update();
        gamepadHighlightSelected();
    }

    void ItemSelectionDialog::setFilter(int filter)
    {
        mSortModel->setFilter(filter);
        mItemView->update();
        gamepadHighlightSelected();
    }

    void ItemSelectionDialog::onSelectedItem(int index)
    {
        ItemStack item = mSortModel->getItem(index);
        eventItemSelected(item.mBase);
    }

    void ItemSelectionDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        exit();
    }

    void ItemSelectionDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (character != 1) // Gamepad control.
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        MyGUI::Widget* filterButton = nullptr;

        switch (action)
        {
        case MWInput::MA_A:
        {
            if (mSortModel->getItemCount() > 0)
                onSelectedItem(mGamepadSelected);
            else
                onCancelButtonClicked(mCancelButton);
            break;
        }
        case MWInput::MA_B:
            onCancelButtonClicked(mCancelButton);
            break;
        case MWInput::MA_Y:

            //MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(mItemView->getHighlightWidget());
            // TODO: Actually make the tooltip; will need to create a new static window at top of screen
            break;
        case MWInput::MA_DPadLeft:
            // if we're in the first column, break
            if (mSortModel->getItemCount() == 0 || mGamepadSelected < mItemView->getRowCount())
                break;

            mGamepadSelected -= mItemView->getRowCount();
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadRight:
            // if we're in the last column, break
            if (mSortModel->getItemCount() == 0 || mGamepadSelected + mItemView->getRowCount() >= mSortModel->getItemCount())
                break;

            mGamepadSelected += mItemView->getRowCount();
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadUp:
            // if we're in the first row, break
            if (mSortModel->getItemCount() == 0 || mGamepadSelected % mItemView->getRowCount() == 0)
                break;

            --mGamepadSelected;
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadDown:
            // if we're in the last row, break
            if (mSortModel->getItemCount() == 0 || mGamepadSelected % mItemView->getRowCount() == mItemView->getRowCount() - 1)
                break;

            ++mGamepadSelected;
            gamepadHighlightSelected();
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    void ItemSelectionDialog::gamepadHighlightSelected()
    {
        if (mSortModel->getItemCount() == 0)
        {
            widgetHighlight(mCancelButton);
            updateGamepadTooltip(nullptr);
            return;
        }

        if (mGamepadSelected > (int)mSortModel->getItemCount() - 1)
            mGamepadSelected = (int)mSortModel->getItemCount() - 1;
        if (mGamepadSelected < 0)
            mGamepadSelected = 0;

        mItemView->highlightItem(mGamepadSelected);
        widgetHighlight(mItemView->getHighlightWidget());

        updateGamepadTooltip(mItemView->getHighlightWidget());
    }

    ControlSet ItemSelectionDialog::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"},
                MenuControl{MWInput::MenuAction::MA_Y, "Info"}
            },
            {
                MenuControl{MWInput::MenuAction::MA_B, "Back"},
            }
        };
    }

}
