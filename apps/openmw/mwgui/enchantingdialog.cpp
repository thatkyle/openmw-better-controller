#include "enchantingdialog.hpp"

#include <iomanip>

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_ScrollView.h>

#include <components/misc/strings/format.hpp>
#include <components/settings/values.hpp>
#include <components/widgets/list.hpp>

#include <components/esm3/loadgmst.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "itemselection.hpp"
#include "itemwidget.hpp"

#include "sortfilteritemmodel.hpp"
#include "ustring.hpp"

#include "controllegend.hpp"

namespace MWGui
{

    EnchantingDialog::EnchantingDialog()
        : WindowBase("openmw_enchanting_dialog.layout")
        , EffectEditorBase(EffectEditorBase::Enchanting)
        , mHighlight(0)
    {
        getWidget(mName, "NameEdit");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAvailableEffectsList, "AvailableEffects");
        getWidget(mUsedEffectsView, "UsedEffects");
        getWidget(mItemBox, "ItemBox");
        getWidget(mSoulBox, "SoulBox");
        getWidget(mEnchantmentPoints, "Enchantment");
        getWidget(mCastCost, "CastCost");
        getWidget(mCharge, "Charge");
        getWidget(mSuccessChance, "SuccessChance");
        getWidget(mChanceLayout, "ChanceLayout");
        getWidget(mTypeButton, "TypeButton");
        getWidget(mBuyButton, "BuyButton");
        getWidget(mPrice, "PriceLabel");
        getWidget(mPriceText, "PriceTextLabel");

        setWidgets(mAvailableEffectsList, mUsedEffectsView);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onCancelButtonClicked);
        mItemBox->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onSelectItem);
        mItemBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &EnchantingDialog::onKeyButtonPressed);
        mSoulBox->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onSelectSoul);
        mBuyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onBuyButtonClicked);
        mTypeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EnchantingDialog::onTypeButtonClicked);
        mName->eventEditSelectAccept += MyGUI::newDelegate(this, &EnchantingDialog::onAccept);

        // makes the columns even
        mAvailableEffectsList->setSpacingOverride(6);

        mUsesHighlightOffset = true;
    }

    void EnchantingDialog::onOpen()
    {
        center();
        if (MWBase::Environment::get().getInputManager()->joystickLastUsed())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mItemBox);
        else
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mName);
    }

    void EnchantingDialog::setSoulGem(const MWWorld::Ptr& gem)
    {
        if (gem.isEmpty())
        {
            mSoulBox->setItem(MWWorld::Ptr());
            mSoulBox->clearUserStrings();
            mEnchanting.setSoulGem(MWWorld::Ptr());
        }
        else
        {
            mSoulBox->setItem(gem);
            mSoulBox->setUserString("ToolTipType", "ItemPtr");
            mSoulBox->setUserData(MWWorld::Ptr(gem));
            mEnchanting.setSoulGem(gem);
        }
    }

    void EnchantingDialog::setItem(const MWWorld::Ptr& item)
    {
        if (item.isEmpty())
        {
            mItemBox->setItem(MWWorld::Ptr());
            mItemBox->clearUserStrings();
            mEnchanting.setOldItem(MWWorld::Ptr());
        }
        else
        {
            std::string_view name = item.getClass().getName(item);
            mName->setCaption(toUString(name));
            mItemBox->setItem(item);
            mItemBox->setUserString("ToolTipType", "ItemPtr");
            mItemBox->setUserData(MWWorld::Ptr(item));
            mEnchanting.setOldItem(item);
        }
    }

    void EnchantingDialog::updateLabels()
    {
        mEnchantmentPoints->setCaption(std::to_string(static_cast<int>(mEnchanting.getEnchantPoints(false))) + " / "
            + std::to_string(mEnchanting.getMaxEnchantValue()));
        mCharge->setCaption(std::to_string(mEnchanting.getGemCharge()));
        mSuccessChance->setCaption(std::to_string(std::clamp(mEnchanting.getEnchantChance(), 0, 100)));
        mCastCost->setCaption(std::to_string(mEnchanting.getEffectiveCastCost()));
        mPrice->setCaption(std::to_string(mEnchanting.getEnchantPrice()));

        switch (mEnchanting.getCastStyle())
        {
            case ESM::Enchantment::CastOnce:
                mTypeButton->setCaption(toUString(
                    MWBase::Environment::get().getWindowManager()->getGameSettingString("sItemCastOnce", "Cast Once")));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::WhenStrikes:
                mTypeButton->setCaption(toUString(MWBase::Environment::get().getWindowManager()->getGameSettingString(
                    "sItemCastWhenStrikes", "When Strikes")));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::WhenUsed:
                mTypeButton->setCaption(toUString(MWBase::Environment::get().getWindowManager()->getGameSettingString(
                    "sItemCastWhenUsed", "When Used")));
                setConstantEffect(false);
                break;
            case ESM::Enchantment::ConstantEffect:
                mTypeButton->setCaption(toUString(MWBase::Environment::get().getWindowManager()->getGameSettingString(
                    "sItemCastConstant", "Cast Constant")));
                setConstantEffect(true);
                break;
        }
    }

    void EnchantingDialog::setPtr(const MWWorld::Ptr& ptr)
    {
        if (ptr.isEmpty() || (ptr.getType() != ESM::REC_MISC && !ptr.getClass().isActor()))
            throw std::runtime_error("Invalid argument in EnchantingDialog::setPtr");

        mName->setCaption({});

        if (ptr.getClass().isActor())
        {
            mEnchanting.setSelfEnchanting(false);
            mEnchanting.setEnchanter(ptr);
            mBuyButton->setCaptionWithReplacing("#{sBuy}");
            mChanceLayout->setVisible(false);
            mPtr = ptr;
            setSoulGem(MWWorld::Ptr());
            mPrice->setVisible(true);
            mPriceText->setVisible(true);
        }
        else
        {
            mEnchanting.setSelfEnchanting(true);
            mEnchanting.setEnchanter(MWMechanics::getPlayer());
            mBuyButton->setCaptionWithReplacing("#{sCreate}");
            mChanceLayout->setVisible(Settings::game().mShowEnchantChance);
            mPtr = MWMechanics::getPlayer();
            setSoulGem(ptr);
            mPrice->setVisible(false);
            mPriceText->setVisible(false);
        }

        setItem(MWWorld::Ptr());

        widgetHighlight(0);
        mItemLastUsed = true;
        mAvailableEffectColumnLastUsed = true;

        startEditing();
        updateLabels();
    }

    void EnchantingDialog::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Enchanting);
        resetReference();
    }

    void EnchantingDialog::resetReference()
    {
        ReferenceInterface::resetReference();
        setItem(MWWorld::Ptr());
        setSoulGem(MWWorld::Ptr());
        mPtr = MWWorld::Ptr();
        mEnchanting.setEnchanter(MWWorld::Ptr());
    }

    void EnchantingDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Enchanting);
    }

    void EnchantingDialog::onSelectItem(MyGUI::Widget* sender)
    {
        if (mEnchanting.getOldItem().isEmpty())
        {
            mItemSelectionDialog = std::make_unique<ItemSelectionDialog>("#{sEnchantItems}");
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onItemSelected);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onItemCancel);
            mItemSelectionDialog->setVisible(true);
            mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
            mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyEnchantable);
        }
        else
        {
            setItem(MWWorld::Ptr());
            updateLabels();
        }
    }

    void EnchantingDialog::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        setItem(item);
        MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
        mEnchanting.nextCastStyle();
        updateLabels();
    }

    void EnchantingDialog::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSoulSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        mEnchanting.setSoulGem(item);
        if (mEnchanting.getGemCharge() == 0)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage32}");
            return;
        }

        setSoulGem(item);
        MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
        updateLabels();
    }

    void EnchantingDialog::onSoulCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void EnchantingDialog::onSelectSoul(MyGUI::Widget* sender)
    {
        if (mEnchanting.getGem().isEmpty())
        {
            mItemSelectionDialog = std::make_unique<ItemSelectionDialog>("#{sSoulGemsWithSouls}");
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &EnchantingDialog::onSoulSelected);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &EnchantingDialog::onSoulCancel);
            mItemSelectionDialog->setVisible(true);
            mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
            mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyChargedSoulstones);

            // MWBase::Environment::get().getWindowManager()->messageBox("#{sInventorySelectNoSoul}");
        }
        else
        {
            setSoulGem(MWWorld::Ptr());
            mEnchanting.nextCastStyle();
            updateLabels();
            updateEffectsView();
        }
    }

    void EnchantingDialog::notifyEffectsChanged()
    {
        mEffectList.mList = mEffects;
        mEnchanting.setEffect(mEffectList);
        updateLabels();
    }

    void EnchantingDialog::onTypeButtonClicked(MyGUI::Widget* sender)
    {
        mEnchanting.nextCastStyle();
        updateLabels();
        updateEffectsView();
    }

    void EnchantingDialog::onAccept(MyGUI::EditBox* sender)
    {
        onBuyButtonClicked(sender);

        // To do not spam onAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void EnchantingDialog::onBuyButtonClicked(MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sEnchantmentMenu11}");
            return;
        }

        if (mName->getCaption().empty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage10}");
            return;
        }

        if (mEnchanting.soulEmpty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage52}");
            return;
        }

        if (mEnchanting.itemEmpty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage11}");
            return;
        }

        if (static_cast<int>(mEnchanting.getEnchantPoints(false)) > mEnchanting.getMaxEnchantValue())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage29}");
            return;
        }

        mEnchanting.setNewItemName(mName->getCaption());
        mEnchanting.setEffect(mEffectList);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);
        if (mPtr != player && mEnchanting.getEnchantPrice() > playerGold)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage18}");
            return;
        }

        // check if the player is attempting to use a soulstone or item that was stolen from this actor
        if (mPtr != player)
        {
            for (int i = 0; i < 2; ++i)
            {
                MWWorld::Ptr item = (i == 0) ? mEnchanting.getOldItem() : mEnchanting.getGem();
                if (MWBase::Environment::get().getMechanicsManager()->isItemStolenFrom(
                        item.getCellRef().getRefId(), mPtr))
                {
                    std::string msg = MWBase::Environment::get()
                                          .getESMStore()
                                          ->get<ESM::GameSetting>()
                                          .find("sNotifyMessage49")
                                          ->mValue.getString();
                    msg = Misc::StringUtils::format(msg, item.getClass().getName(item));
                    MWBase::Environment::get().getWindowManager()->messageBox(msg);

                    MWBase::Environment::get().getMechanicsManager()->confiscateStolenItemToOwner(
                        player, item, mPtr, 1);

                    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Enchanting);
                    MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
                    return;
                }
            }
        }

        if (mEnchanting.create())
        {
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("enchant success"));
            MWBase::Environment::get().getWindowManager()->messageBox("#{sEnchantmentMenu12}");
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Enchanting);
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("enchant fail"));
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage34}");
            if (!mEnchanting.getGem().isEmpty() && !mEnchanting.getGem().getRefData().getCount())
            {
                setSoulGem(MWWorld::Ptr());
                mEnchanting.nextCastStyle();
                updateLabels();
                updateEffectsView();
            }
        }
    }

    void EnchantingDialog::onFrame(float dt)
    {
        checkReferenceAvailable();

        // we never want to focus the name edit field when using the controller
        if (MWBase::Environment::get().getInputManager()->joystickLastUsed() && 
                    !mAddEffectDialog.isVisible() &&
                    (mItemSelectionDialog == nullptr || !mItemSelectionDialog->isVisible()) &&
                    !MWBase::Environment::get().getWindowManager()->virtualKeyboardVisible())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mItemBox);
    }

    MyGUI::IntCoord EnchantingDialog::highlightOffset()
    {
        // increase the highlight size if we're targetting the spell name, item, or soul gem
        if (mHighlight < 4)
            return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8));

        return MyGUI::IntCoord();
    }

    void EnchantingDialog::widgetHighlight(unsigned int index)
    {
        if (index < 0)
            index = 0;
        if (index > mAvailableEffectsList->getItemCount() + mUsedEffectsView->getChildCount() + 3)
            index = mAvailableEffectsList->getItemCount() + mUsedEffectsView->getChildCount() + 3;

        mHighlight = index;

        if (mHighlight == 0)
            WindowBase::widgetHighlight(mName);
        else if (mHighlight == 1)
        {
            WindowBase::widgetHighlight(mItemBox);
            mItemLastUsed = true;
        }
        else if (mHighlight == 2)
        {
            WindowBase::widgetHighlight(mSoulBox);
            mItemLastUsed = false;
        }
        else if (mHighlight == 3)
            WindowBase::widgetHighlight(mTypeButton);
        else if (mHighlight - 4 < mAvailableEffectsList->getItemCount())
        {
            WindowBase::widgetHighlight(mAvailableEffectsList->getItemWidget(mAvailableEffectsList->getItemNameAt(mHighlight - 4)));
            mAvailableEffectColumnLastUsed = true;
        }
        else
        {
            WindowBase::widgetHighlight(mUsedEffectsView->getChildAt(mHighlight - mAvailableEffectsList->getItemCount() - 4));
            mAvailableEffectColumnLastUsed = false;
        }
    }

    void EnchantingDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;

        if (mAddEffectDialog.isVisible())
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        if (action == MWInput::MA_B) // back
            onCancelButtonClicked(sender);
        else if (action == MWInput::MA_A) // select
        {
            if (mHighlight == 0)
                MWBase::Environment::get().getWindowManager()->startVirtualKeyboard(mName);
            else if (mHighlight == 1)
                onSelectItem(mItemBox);
            else if (mHighlight == 2)
                onSelectSoul(mSoulBox);
            else if (mHighlight == 3)
                onTypeButtonClicked(mTypeButton);
            else if (mHighlight - 4 < mAvailableEffectsList->getItemCount())
                onAvailableEffectClicked(mAvailableEffectsList->getItemWidget(mAvailableEffectsList->getItemNameAt(mHighlight - 4)));
            else
                onEditEffect(mUsedEffectsView->getChildAt(mHighlight - mAvailableEffectsList->getItemCount() - 4));
        }
        else if (action == MWInput::MA_X) // buy
            onBuyButtonClicked(mBuyButton);
        else if (action == MWInput::MA_DPadUp)
        {
            if (mHighlight == 0)
            {
                // do nothing
            }
            else if (mHighlight < 3)
                widgetHighlight(0);
            else if (mHighlight == 3)
            {
                if (mAvailableEffectColumnLastUsed)
                    widgetHighlight(mAvailableEffectsList->getItemCount() + 3);
                else
                    widgetHighlight(mAvailableEffectsList->getItemCount() + mUsedEffectsView->getChildCount() + 3);
            }
            else if (mHighlight == 4)
                widgetHighlight(1);
            else if (mHighlight == 4 + mAvailableEffectsList->getItemCount())
                widgetHighlight(2);
            else
                widgetHighlight(mHighlight - 1);
        }
        else if (action == MWInput::MA_DPadDown)
        {
            if (mHighlight == 0)
            {
                if (mItemLastUsed)
                    widgetHighlight(1);
                else
                    widgetHighlight(2);
            }
            else if (mHighlight == 1)
                widgetHighlight(4);
            else if (mHighlight == 2 && mUsedEffectsView->getChildCount() == 0)
                widgetHighlight(4);
            else if (mHighlight == 2)
                widgetHighlight(4 + mAvailableEffectsList->getItemCount());
            else if (mHighlight == mAvailableEffectsList->getItemCount() + 3 || mHighlight == mAvailableEffectsList->getItemCount() + mUsedEffectsView->getChildCount() + 3)
                widgetHighlight(3);
            else if (mHighlight > 3)
                widgetHighlight(mHighlight + 1);
        }
        else if (action == MWInput::MA_DPadRight)
        {
            if (mHighlight == 1)
                widgetHighlight(2);
            else if (mHighlight > 3 && mHighlight - 4 < mAvailableEffectsList->getItemCount())
                widgetHighlight(std::min((unsigned long long) (mHighlight + mAvailableEffectsList->getItemCount()), mAvailableEffectsList->getItemCount() + mUsedEffectsView->getChildCount() + 3));
        }
        else if (action == MWInput::MA_DPadLeft)
        {
            if (mHighlight == 2)
                widgetHighlight(1);
            else if (mHighlight > mAvailableEffectsList->getItemCount() + 3)
                widgetHighlight(std::min(mHighlight - mAvailableEffectsList->getItemCount(), mAvailableEffectsList->getItemCount() + 3));
        }
    }

    ControlSet EnchantingDialog::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"},
                MenuControl{MWInput::MenuAction::MA_X, "Buy"},
                MenuControl{MWInput::MenuAction::MA_Y, "Info"}
            },
            {
                MenuControl{MWInput::MenuAction::MA_B, "Back"},
            }
        };
    }
}
