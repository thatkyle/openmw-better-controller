#include "repair.hpp"

#include <iomanip>

#include <MyGUI_ScrollView.h>

#include <components/esm3/loadrepa.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"

#include "inventoryitemmodel.hpp"
#include "itemchargeview.hpp"
#include "itemselection.hpp"
#include "itemwidget.hpp"
#include "sortfilteritemmodel.hpp"

namespace MWGui
{

    Repair::Repair()
        : WindowBase("openmw_repair.layout")
    , mGamepadSelected(0)
    {
        getWidget(mRepairBox, "RepairBox");
        getWidget(mToolBox, "ToolBox");
        getWidget(mToolIcon, "ToolIcon");
        getWidget(mUsesLabel, "UsesLabel");
        getWidget(mQualityLabel, "QualityLabel");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onCancel);

        mRepairBox->eventItemClicked += MyGUI::newDelegate(this, &Repair::onRepairItem);
        mRepairBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &Repair::onKeyButtonPressed);
    mRepairBox->setDisplayMode(ItemChargeView::DisplayMode_Health);

        mToolIcon->eventMouseButtonClick += MyGUI::newDelegate(this, &Repair::onSelectItem);
    }

    void Repair::onOpen()
    {
        center();

    SortFilterItemModel * model = new SortFilterItemModel(new InventoryItemModel(MWMechanics::getPlayer()));
    model->setFilter(SortFilterItemModel::Filter_OnlyRepairable);
    mRepairBox->setModel(model);

    mGamepadSelected = 0;
    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mRepairBox);

        // Reset scrollbars
        mRepairBox->resetScrollbars();
    }

void Repair::onClose()
{
    MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(nullptr, nullptr);
}

    void Repair::setPtr(const MWWorld::Ptr& item)
    {
        if (item.isEmpty() || !item.getClass().isItem(item))
            throw std::runtime_error("Invalid argument in Repair::setPtr");

        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Item Repair Up"));

        mRepair.setTool(item);

        mToolIcon->setItem(item);
        mToolIcon->setUserString("ToolTipType", "ItemPtr");
        mToolIcon->setUserData(MWWorld::Ptr(item));

        updateRepairView();
    
    gamepadHighlightSelected();
}

    void Repair::updateRepairView()
    {
        MWWorld::LiveCellRef<ESM::Repair>* ref = mRepair.getTool().get<ESM::Repair>();

        int uses = mRepair.getTool().getClass().getItemHealth(mRepair.getTool());

        float quality = ref->mBase->mData.mQuality;

        mToolIcon->setUserData(mRepair.getTool());

        std::stringstream qualityStr;
        qualityStr << std::setprecision(3) << quality;

        mUsesLabel->setCaptionWithReplacing("#{sUses} " + MyGUI::utility::toString(uses));
        mQualityLabel->setCaptionWithReplacing("#{sQuality} " + qualityStr.str());

        bool toolBoxVisible = (mRepair.getTool().getRefData().getCount() != 0);
        mToolBox->setVisible(toolBoxVisible);
        mToolBox->setUserString("Hidden", toolBoxVisible ? "false" : "true");

        if (!toolBoxVisible)
        {
            mToolIcon->setItem(MWWorld::Ptr());
            mToolIcon->clearUserStrings();
        }

        mRepairBox->update();

        Gui::Box* box = dynamic_cast<Gui::Box*>(mMainWidget);
        if (box == nullptr)
            throw std::runtime_error("main widget must be a box");

        box->notifyChildrenSizeChanged();
        center();
    }

    void Repair::onSelectItem(MyGUI::Widget* sender)
    {
        mItemSelectionDialog = std::make_unique<ItemSelectionDialog>("#{sRepair}");
        mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &Repair::onItemSelected);
        mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &Repair::onItemCancel);
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
        mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyRepairTools);
    }

    void Repair::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        mToolIcon->setItem(item);
        mToolIcon->setUserString("ToolTipType", "ItemPtr");
        mToolIcon->setUserData(item);

        mRepair.setTool(item);

        MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
        updateRepairView();
    }

    void Repair::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void Repair::onCancel(MyGUI::Widget* /*sender*/)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Repair);
    }

    void Repair::onRepairItem(MyGUI::Widget* /*sender*/, const MWWorld::Ptr& ptr)
    {
        if (!mRepair.getTool().getRefData().getCount())
            return;

        mRepair.repair(ptr);

        updateRepairView();
    }

void Repair::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
{
    if (character != 1) // Gamepad control.
        return;

    int repairCount = mRepairBox->getItemCount();

    if (repairCount == 0)
        return;

    MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
    MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

    if (action == MWInput::MenuAction::MA_DPadDown)
    {
        if (mGamepadSelected < repairCount - 1)
        {
            mGamepadSelected++;
            gamepadHighlightSelected();
        }
    }
    else if (action == MWInput::MenuAction::MA_DPadUp)
    {
        if (mGamepadSelected > 0)
        {
            mGamepadSelected--;
            gamepadHighlightSelected();
        }
    }
    else if (action == MWInput::MenuAction::MA_A)
    {
        onRepairItem(nullptr, *mRepairBox->getItemWidget(mGamepadSelected)->getUserData<MWWorld::Ptr>());

        gamepadHighlightSelected();
    }
    else if (action == MWInput::MenuAction::MA_B)
    {
        onCancel(sender);
    }
    else
    {
        MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
    }
}

void Repair::gamepadHighlightSelected()
{
    int repairCount = mRepairBox->getItemCount();

    if (mGamepadSelected > repairCount - 1)
        mGamepadSelected = repairCount - 1;
    if (mGamepadSelected < 0)
        mGamepadSelected = 0;

    if (repairCount)
    {
        widgetHighlight(mRepairBox->getItemWidget(mGamepadSelected));

        updateGamepadTooltip(mRepairBox->getItemWidget(mGamepadSelected));
    }
    else
    {
        widgetHighlight(nullptr);
    }
}

}
