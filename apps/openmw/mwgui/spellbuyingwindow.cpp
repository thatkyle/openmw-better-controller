#include "spellbuyingwindow.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_Button.h>
#include <MyGUI_ScrollView.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwinput/actions.hpp"

namespace MWGui
{
    SpellBuyingWindow::SpellBuyingWindow() :
        WindowBase("openmw_spell_buying_window.layout")
        , mCurrentY(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mSpellsView, "SpellsView");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onCancelButtonClicked);
        mCancelButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &SpellBuyingWindow::onKeyButtonPressed);
    }

    bool SpellBuyingWindow::sortSpells (const ESM::Spell* left, const ESM::Spell* right)
    {
        std::string leftName = Misc::StringUtils::lowerCase(left->mName);
        std::string rightName = Misc::StringUtils::lowerCase(right->mName);

        return leftName.compare(rightName) < 0;
    }

    void SpellBuyingWindow::addSpell(const ESM::Spell& spell)
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        int price = std::max(1, static_cast<int>(spell.mData.mCost*store.get<ESM::GameSetting>().find("fSpellValueMult")->mValue.getFloat()));
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,price,true);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        // TODO: refactor to use MyGUI::ListBox

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;

        MyGUI::Button* toAdd =
            mSpellsView->createWidget<MyGUI::Button>(
                price <= playerGold ? "SandTextButton" : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                0,
                mCurrentY,
                200,
                lineHeight,
                MyGUI::Align::Default
            );

        mCurrentY += lineHeight;

        toAdd->setUserData(price);
        toAdd->setCaptionWithReplacing(spell.mName+"   -   "+MyGUI::utility::toString(price)+"#{sgp}");
        toAdd->setSize(mSpellsView->getWidth(), lineHeight);
        toAdd->eventMouseWheel += MyGUI::newDelegate(this, &SpellBuyingWindow::onMouseWheel);
        toAdd->setUserString("ToolTipType", "Spell");
        toAdd->setUserString("Spell", spell.mId);
        toAdd->setUserString("SpellCost", std::to_string(spell.mData.mCost));
        toAdd->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellBuyingWindow::onSpellButtonClick);
        mSpellsWidgetMap.insert(std::make_pair (toAdd, spell.mId));
    }

    void SpellBuyingWindow::clearSpells()
    {
        mSpellsView->setViewOffset(MyGUI::IntPoint(0,0));
        mCurrentY = 0;
        while (mSpellsView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellsView->getChildAt(0));
        mSpellsWidgetMap.clear();
        mSpellWidgets.clear();
        mSpellHighlight = 0;
    }

    void SpellBuyingWindow::setPtr(const MWWorld::Ptr &actor)
    {
        setPtr(actor, 0);
    }

    void SpellBuyingWindow::setPtr(const MWWorld::Ptr& actor, int startOffset)
    {
        center();
        mPtr = actor;
        clearSpells();

        MWMechanics::Spells& merchantSpells = actor.getClass().getCreatureStats (actor).getSpells();

        std::vector<const ESM::Spell*> spellsToSort;

        for (const ESM::Spell* spell : merchantSpells)
        {
            if (spell->mData.mType!=ESM::Spell::ST_Spell)
                continue; // don't try to sell diseases, curses or powers

            if (actor.getClass().isNpc())
            {
                const ESM::Race* race =
                        MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(
                        actor.get<ESM::NPC>()->mBase->mRace);
                if (race->mPowers.exists(spell->mId))
                    continue;
            }

            if (playerHasSpell(spell->mId))
                continue;

            spellsToSort.push_back(spell);
        }

        std::stable_sort(spellsToSort.begin(), spellsToSort.end(), sortSpells);

        for (const ESM::Spell* spell : spellsToSort)
        {
            addSpell(*spell);
        }

        spellsToSort.clear();

        updateLabels();

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSpellsView->setVisibleVScroll(false);
        mSpellsView->setCanvasSize (MyGUI::IntSize(mSpellsView->getWidth(), std::max(mSpellsView->getHeight(), mCurrentY)));
        mSpellsView->setVisibleVScroll(true);
        mSpellsView->setViewOffset(MyGUI::IntPoint(0, startOffset));

        // Gamepad controls, currently jumps to top whenever a spell is purchased.
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);
        MyGUI::EnumeratorWidgetPtr spellList = mSpellsView->getEnumerator();
        while (spellList.next())
        {
            // Must use price to determine if a spell is enabled due to setEnabled removing tooltip. Price is stored in UserData in addSpell.
            if (*spellList.current()->getUserData<int>() <= playerGold)
                mSpellWidgets.push_back(spellList.current());
        }

        if (!mSpellWidgets.empty())
            widgetHighlight(mSpellWidgets[mSpellHighlight]);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCancelButton);
    }

    bool SpellBuyingWindow::playerHasSpell(const std::string &id)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        return player.getClass().getCreatureStats(player).getSpells().hasSpell(id);
    }

    void SpellBuyingWindow::onSpellButtonClick(MyGUI::Widget* _sender)
    {
        int price = *_sender->getUserData<int>();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();
        spells.add (mSpellsWidgetMap.find(_sender)->second);
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        setPtr(mPtr, mSpellsView->getViewOffset().top);

        MWBase::Environment::get().getWindowManager()->playSound("Item Gold Up");
    }

    void SpellBuyingWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_SpellBuying);
    }

    void SpellBuyingWindow::updateLabels()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        mPlayerGold->setCoord(8,
                              mPlayerGold->getTop(),
                              mPlayerGold->getTextSize().width,
                              mPlayerGold->getHeight());
    }

    void SpellBuyingWindow::onReferenceUnavailable()
    {
        // remove both Spells and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_SpellBuying);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    void SpellBuyingWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSpellsView->getViewOffset().top + _rel*0.3 > 0)
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSpellsView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mSpellsView->getViewOffset().top + _rel*0.3f)));
    }

    void SpellBuyingWindow::onKeyButtonPressed(MyGUI::Widget *sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        if (action == MWInput::MA_B || (mSpellWidgets.empty() && action == MWInput::MA_A))
            onCancelButtonClicked(sender);
        else if (!mSpellWidgets.empty()) // Only control travel options if there are any.
        {
            if (action == MWInput::MA_A)
                onSpellButtonClick(mSpellWidgets[mSpellHighlight]);
            else if (action == MWInput::MA_DPadUp && mSpellHighlight > 0)
            {
                --mSpellHighlight;
                widgetHighlight(mSpellWidgets[mSpellHighlight]);
                onMouseWheel(mSpellsView, mSpellWidgets[mSpellHighlight]->getHeight());
            }
            else if (action == MWInput::MA_DPadDown && mSpellHighlight < mSpellWidgets.size() - 1)
            {
                ++mSpellHighlight;
                widgetHighlight(mSpellWidgets[mSpellHighlight]);
                onMouseWheel(mSpellsView, -(mSpellWidgets[mSpellHighlight]->getHeight()));
            }
        }
        else
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
    }
}

