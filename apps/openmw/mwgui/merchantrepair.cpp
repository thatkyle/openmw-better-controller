#include "merchantrepair.hpp"

#include <components/esm3/loadgmst.hpp>
#include <components/settings/values.hpp>

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ScrollView.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWGui
{
    MerchantRepair::MerchantRepair()
        : WindowBase("openmw_merchantrepair.layout")
        , mGamepadSelected(0)
{
        getWidget(mList, "RepairView");
        getWidget(mOkButton, "OkButton");
        getWidget(mGoldLabel, "PlayerGold");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MerchantRepair::onOkButtonClick);
    
    mList->eventKeyButtonPressed += MyGUI::newDelegate(this, &MerchantRepair::onKeyButtonPressed);
}

    void MerchantRepair::setPtr(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in MerchantRepair::setPtr");
        mActor = actor;

        while (mList->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mList->getChildAt(0));

        const int lineHeight = Settings::gui().mFontSize + 2;
        int currentY = 0;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        int categories = MWWorld::ContainerStore::Type_Weapon | MWWorld::ContainerStore::Type_Armor;
        for (MWWorld::ContainerStoreIterator iter(store.begin(categories)); iter != store.end(); ++iter)
        {
            if (iter->getClass().hasItemHealth(*iter))
            {
                int maxDurability = iter->getClass().getItemMaxHealth(*iter);
                int durability = iter->getClass().getItemHealth(*iter);
                if (maxDurability == durability || maxDurability == 0)
                    continue;

                int basePrice = iter->getClass().getValue(*iter);
                float fRepairMult = MWBase::Environment::get()
                                        .getESMStore()
                                        ->get<ESM::GameSetting>()
                                        .find("fRepairMult")
                                        ->mValue.getFloat();

                float p = static_cast<float>(std::max(1, basePrice));
                float r = static_cast<float>(std::max(1, static_cast<int>(maxDurability / p)));

                int x = static_cast<int>((maxDurability - durability) / r);
                x = static_cast<int>(fRepairMult * x);
                x = std::max(1, x);

                int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mActor, x, true);

                std::string name{ iter->getClass().getName(*iter) };
                name += " - " + MyGUI::utility::toString(price)
                    + MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>().find("sgp")->mValue.getString();

                MyGUI::Button* button = mList->createWidget<MyGUI::Button>(price <= playerGold
                        ? "SandTextButton"
                        : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                    0, currentY, 0, lineHeight, MyGUI::Align::Default);

                currentY += lineHeight;

                button->setUserString("Price", MyGUI::utility::toString(price));
                button->setUserData(MWWorld::Ptr(*iter));
                button->setCaptionWithReplacing(name);
                button->setSize(mList->getWidth(), lineHeight);
                button->eventMouseWheel += MyGUI::newDelegate(this, &MerchantRepair::onMouseWheel);
                button->setUserString("ToolTipType", "ItemPtr");
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MerchantRepair::onRepairButtonClick);
            }
        }
        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mList->setVisibleVScroll(false);
        mList->setCanvasSize(MyGUI::IntSize(mList->getWidth(), std::max(mList->getHeight(), currentY)));
        mList->setVisibleVScroll(true);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
    
    gamepadHighlightSelected();
}

    void MerchantRepair::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mList->getViewOffset().top + _rel * 0.3f > 0)
            mList->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mList->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mList->getViewOffset().top + _rel * 0.3f)));
    }

    void MerchantRepair::onOpen()
    {
        center();
        // Reset scrollbars
        mList->setViewOffset(MyGUI::IntPoint(0, 0));
    
    mGamepadSelected = 0;
    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mList);
}

void MerchantRepair::onClose()
{
    MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(nullptr, nullptr);
}

    void MerchantRepair::onRepairButtonClick(MyGUI::Widget* sender)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();

        int price = MyGUI::utility::parseInt(sender->getUserString("Price"));
        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        // repair
        MWWorld::Ptr item = *sender->getUserData<MWWorld::Ptr>();
        item.getCellRef().setCharge(item.getClass().getItemMaxHealth(item));

        player.getClass().getContainerStore(player).restack(item);

        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Repair"));

        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& actorStats = mActor.getClass().getCreatureStats(mActor);
        actorStats.setGoldPool(actorStats.getGoldPool() + price);

        setPtr(mActor);
    }

    void MerchantRepair::onOkButtonClick(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_MerchantRepair);
    }

void MerchantRepair::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
{
    if (character != 1) // Gamepad control.
        return;

    int itemCount = mList->getChildCount();

    if (itemCount == 0)
        return;

    MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
    MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

    //TODO: support going through active effects; for now, just support spell selection

    if (action == MWInput::MenuAction::MA_DPadDown)
    {
        if (mGamepadSelected < itemCount - 1)
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
        onRepairButtonClick(mList->getChildAt(mGamepadSelected));

        gamepadHighlightSelected();
    }
    else if (action == MWInput::MenuAction::MA_B)
        onOkButtonClick(mOkButton);
    else
    {
        MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
    }
}

void MerchantRepair::gamepadHighlightSelected()
{
    int itemCount = mList->getChildCount();

    if (mGamepadSelected > itemCount - 1)
        mGamepadSelected = itemCount - 1;
    if (mGamepadSelected < 0)
        mGamepadSelected = 0;

    if (itemCount)
    {
        widgetHighlight(mList->getChildAt(mGamepadSelected));

        updateGamepadTooltip(mList->getChildAt(mGamepadSelected));
    }
    else
    {
        widgetHighlight(nullptr);
    }
}

}
