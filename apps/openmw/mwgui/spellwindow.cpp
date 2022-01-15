#include "spellwindow.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>

#include <components/misc/stringops.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/spellutil.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwinput/actions.hpp"

#include "spellicons.hpp"
#include "confirmationdialog.hpp"
#include "spellview.hpp"
#include "controllegend.hpp"

namespace MWGui
{

    SpellWindow::SpellWindow(DragAndDrop* drag)
        : WindowPinnableBase("openmw_spell_window.layout")
        , NoDrop(drag, mMainWidget)
        , mSpellView(nullptr)
        , mUpdateTimer(0.0f)
        , mGamepadSelected(0)
    {
        mSpellIcons = new SpellIcons();

        MyGUI::Widget* deleteButton;
        getWidget(deleteButton, "DeleteSpellButton");

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");
        getWidget(mFilterEdit, "FilterEdit");

        mSpellView->eventSpellClicked += MyGUI::newDelegate(this, &SpellWindow::onModelIndexSelected);
        mFilterEdit->eventEditTextChange += MyGUI::newDelegate(this, &SpellWindow::onFilterChanged);
        deleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onDeleteClicked);

        mSpellView->eventKeyButtonPressed += MyGUI::newDelegate(this, &SpellWindow::onKeyButtonPressed);

        mSpellView->eventKeySetFocus += MyGUI::newDelegate(this, &SpellWindow::onFocusGained);
        mSpellView->eventKeyLostFocus += MyGUI::newDelegate(this, &SpellWindow::onFocusLost);

        setCoord(498, 300, 302, 300);

        // Adjust the spell filtering widget size because of MyGUI limitations.
        int filterWidth = mSpellView->getSize().width - deleteButton->getSize().width - 3;
        mFilterEdit->setSize(filterWidth, mFilterEdit->getSize().height);

        mUsesHighlightOffset = true;
        mUsesHighlightSizeOverride = true;
    }

    SpellWindow::~SpellWindow()
    {
        delete mSpellIcons;
    }

    void SpellWindow::onPinToggled()
    {
        Settings::Manager::setBool("spells pin", "Windows", mPinned);

        MWBase::Environment::get().getWindowManager()->setSpellVisibility(!mPinned);
    }

    void SpellWindow::onTitleDoubleClicked()
    {
        if (MyGUI::InputManager::getInstance().isShiftPressed())
            MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
        else if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void SpellWindow::onOpen()
    {
        // Reset the filter focus when opening the window
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (focus == mFilterEdit)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr); 

        updateSpells();
    }

    void SpellWindow::focus()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSpellView);
    }

    void SpellWindow::onFocusGained(MyGUI::Widget* sender, MyGUI::Widget* oldFocus)
    {
        if (!MWBase::Environment::get().getInputManager()->isGamepadGuiCursorEnabled())
        {
            gamepadHighlightSelected();
        }

        std::vector<MenuControl> leftControls{
            MenuControl{MWInput::MenuAction::MA_A, "Select"},
            MenuControl{MWInput::MenuAction::MA_X, "Delete"}
        };
        std::vector<MenuControl> rightControls{
            MenuControl{MWInput::MenuAction::MA_LTrigger, "Inventory"},
            MenuControl{MWInput::MenuAction::MA_RTrigger, "Map"},
            MenuControl{MWInput::MenuAction::MA_B, "Back"},
        };

        MWBase::Environment::get().getWindowManager()->pushMenuControls(leftControls, rightControls);
    }

    void SpellWindow::onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus)
    {
        updateHighlightVisibility();

        MWBase::Environment::get().getWindowManager()->popMenuControls();

        updateGamepadTooltip(nullptr);
    }

    void SpellWindow::onFrame(float dt) 
    {
        NoDrop::onFrame(dt);
        mUpdateTimer += dt;
        if (0.5f < mUpdateTimer)
        {
            mUpdateTimer = 0;
            mSpellView->incrementalUpdate();
        }

        // Update effects in-game too if the window is pinned
        if (mPinned && !MWBase::Environment::get().getWindowManager()->isGuiMode())
            mSpellIcons->updateWidgets(mEffectBox, false);
    }

    void SpellWindow::updateSpells()
    {
        mSpellIcons->updateWidgets(mEffectBox, false);

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), mFilterEdit->getCaption()));
        
        gamepadHighlightSelected();
    }

    void SpellWindow::onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = store.begin();
        for (; it != store.end(); ++it)
        {
            if (*it == item)
            {
                break;
            }
        }
        if (it == store.end())
            throw std::runtime_error("can't find selected item");

        // equip, if it can be equipped and is not already equipped
        if (!alreadyEquipped
            && !item.getClass().getEquipmentSlots(item).first.empty())
        {
            MWBase::Environment::get().getWindowManager()->useItem(item);
            // make sure that item was successfully equipped
            if (!store.isEquipped(item))
                return;
        }

        store.setSelectedEnchantItem(it);
        // to reset WindowManager::mSelectedSpell immediately
        MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(*it);

        updateSpells();
    }

    void SpellWindow::askDeleteSpell(const std::string &spellId)
    {
        // delete spell, if allowed
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        std::string raceId = player.get<ESM::NPC>()->mBase->mRace;
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceId);
        // can't delete racial spells, birthsign spells or powers
        bool isInherent = race->mPowers.exists(spell->mId) || spell->mData.mType == ESM::Spell::ST_Power;
        const std::string& signId = MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();
        if (!isInherent && !signId.empty())
        {
            const ESM::BirthSign* sign = MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().find(signId);
            isInherent = sign->mPowers.exists(spell->mId);
        }

        if (isInherent)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sDeleteSpellError}");
        }
        else
        {
            // ask for confirmation
            mSpellToDelete = spellId;
            ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
            std::string question = MWBase::Environment::get().getWindowManager()->getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
            question = Misc::StringUtils::format(question, spell->mName);
            dialog->askForConfirmation(question);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &SpellWindow::onDeleteSpellAccept);
            dialog->eventCancelClicked.clear();
        }
    }

    void SpellWindow::onModelIndexSelected(SpellModel::ModelIndex index)
    {
        const Spell& spell = mSpellView->getModel()->getItem(index);
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        }
        else
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed())
                askDeleteSpell(spell.mId);
            else
                onSpellSelected(spell.mId);
        }
    }

    void SpellWindow::onFilterChanged(MyGUI::EditBox *sender)
    {
        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), sender->getCaption()));
    }

    void SpellWindow::onDeleteClicked(MyGUI::Widget *widget)
    {
        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            return;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType != Spell::Type_EnchantedItem)
            askDeleteSpell(spell.mId);
    }

    void SpellWindow::onSpellSelected(const std::string& spellId)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        store.setSelectedEnchantItem(store.end());
        MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));

        updateSpells();
    }

    void SpellWindow::onDeleteSpellAccept()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == mSpellToDelete)
            MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();

        spells.remove(mSpellToDelete);

        mGamepadSelected--;

        updateSpells();
    }

    void SpellWindow::cycle(bool next)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player))
            return;

        bool godmode = MWBase::Environment::get().getWorld()->getGodModeState();
        const MWMechanics::CreatureStats &stats = player.getClass().getCreatureStats(player);
        if ((!godmode && stats.isParalyzed()) || stats.getKnockedDown() || stats.isDead() || stats.getHitRecovery())
            return;

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), ""));

        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            selected = 0;

        selected += next ? 1 : -1;
        int itemcount = mSpellView->getModel()->getItemCount();
        if (itemcount == 0)
            return;
        selected = (selected + itemcount) % itemcount;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType == Spell::Type_EnchantedItem)
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        else
            onSpellSelected(spell.mId);
    }

    void SpellWindow::gamepadHighlightSelected()
    {
        if (mGamepadSelected > (int)mSpellView->getModel()->getItemCount() - 1)
            mGamepadSelected = (int)mSpellView->getModel()->getItemCount() - 1;
        if (mGamepadSelected < 0)
            mGamepadSelected = 0;

        if (mSpellView->getModel()->getItemCount())
        {
            mSpellView->highlightItem(mGamepadSelected);
            widgetHighlight(mSpellView->getHighlightWidget());

            updateGamepadTooltip(mSpellView->getHighlightWidget());
        }
        else
        {
            widgetHighlight(nullptr);
        }
    }

    void SpellWindow::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (character != 1) // Gamepad control.
            return;

        int spellCount = mSpellView->getModel()->getItemCount();

        if (mSpellView->getModel()->getItemCount() == 0)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        //TODO: support going through active effects; for now, just support spell selection

        if (action == MWInput::MenuAction::MA_DPadDown)
        {
            if (mGamepadSelected < spellCount - 1)
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
            onModelIndexSelected(mGamepadSelected);

            gamepadHighlightSelected();
        }
        else if (action == MWInput::MenuAction::MA_X)
        {
            const Spell& spell = mSpellView->getModel()->getItem(mGamepadSelected);
            if (spell.mType != Spell::Type_EnchantedItem)
                askDeleteSpell(spell.mId);

            gamepadHighlightSelected();
        }
        else if (action == MWInput::MenuAction::MA_LTrigger || action == MWInput::MenuAction::MA_RTrigger)
        {
            if (MWBase::Environment::get().getWindowManager()->processInventoryTrigger(action, GM_Inventory, GW_Magic))
            {
                MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
                updateHighlightVisibility();
            }
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
        }
    }

    MyGUI::IntCoord SpellWindow::highlightOffset()
    {
        return MyGUI::IntCoord(-2, -1, 2, 3);
    }

    MyGUI::IntSize SpellWindow::highlightSizeOverride()
    {
        return MyGUI::IntSize(mSpellView->getScrollViewWidth() - 6, mSpellView->getHighlightWidget()->getHeight());
    }
}
