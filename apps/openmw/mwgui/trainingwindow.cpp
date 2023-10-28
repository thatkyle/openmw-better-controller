#include "trainingwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextIterator.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/settings/values.hpp>

#include "tooltips.hpp"
#include "controllegend.hpp"

namespace MWGui
{

    TrainingWindow::TrainingWindow()
        : WindowBase("openmw_trainingwindow.layout")
        , mTimeAdvancer(0.05f)
        , mGamepadSelected(0)
    {
        getWidget(mTrainingOptions, "TrainingOptions");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onCancelButtonClicked);

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &TrainingWindow::onTrainingProgressChanged);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &TrainingWindow::onTrainingFinished);

        mTrainingOptions->eventKeyButtonPressed += MyGUI::newDelegate(this, &TrainingWindow::onKeyButtonPressed);
    }

    void TrainingWindow::onOpen()
    {
        if (mTimeAdvancer.isRunning())
        {
            mProgressBar.setVisible(true);
            setVisible(false);
        }
        else
            mProgressBar.setVisible(false);

        mGamepadSelected = 0;
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTrainingOptions);

        center();
    }

    void TrainingWindow::onClose()
    {
        MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(nullptr, nullptr);
    }

    void TrainingWindow::onClose()
    {
        MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(nullptr, nullptr);
    }

    void TrainingWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in TrainingWindow::setPtr");
        mPtr = actor;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));

        const auto& store = MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::GameSetting>& gmst = store->get<ESM::GameSetting>();
        const MWWorld::Store<ESM::Skill>& skillStore = store->get<ESM::Skill>();

        // NPC can train you in their best 3 skills
        constexpr size_t maxSkills = 3;
        std::vector<std::pair<const ESM::Skill*, float>> skills;
        skills.reserve(maxSkills);

        const auto sortByValue
            = [](const std::pair<const ESM::Skill*, float>& lhs, const std::pair<const ESM::Skill*, float>& rhs) {
                  return lhs.second > rhs.second;
              };
        // Maintain a sorted vector of max maxSkills elements, ordering skills by value and content file order
        const MWMechanics::NpcStats& actorStats = actor.getClass().getNpcStats(actor);
        for (const ESM::Skill& skill : skillStore)
        {
            float value = getSkillForTraining(actorStats, skill.mId);
            if (skills.size() < maxSkills)
            {
                skills.emplace_back(&skill, value);
                std::stable_sort(skills.begin(), skills.end(), sortByValue);
            }
            else
            {
                auto& lowest = skills[maxSkills - 1];
                if (lowest.second < value)
                {
                    lowest.first = &skill;
                    lowest.second = value;
                    std::stable_sort(skills.begin(), skills.end(), sortByValue);
                }
            }
        }

        MyGUI::EnumeratorWidgetPtr widgets = mTrainingOptions->getEnumerator();
        MyGUI::Gui::getInstance().destroyWidgets(widgets);

        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const int lineHeight = Settings::gui().mFontSize + 2;

        for (size_t i = 0; i < skills.size(); ++i)
        {
            const ESM::Skill* skill = skills[i].first;
            int price = static_cast<int>(
                pcStats.getSkill(skill->mId).getBase() * gmst.find("iTrainingMod")->mValue.getInteger());
            price = std::max(1, price);
            price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

            MyGUI::Button* button = mTrainingOptions->createWidget<MyGUI::Button>(price <= playerGold
                    ? "SandTextButton"
                    : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                MyGUI::IntCoord(5, 5 + i * lineHeight, mTrainingOptions->getWidth() - 10, lineHeight),
                MyGUI::Align::Default);

            button->setUserData(skills[i].first);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onTrainingSelected);

            button->setCaptionWithReplacing(
                MyGUI::TextIterator::toTagsString(skill->mName) + " - " + MyGUI::utility::toString(price));

            button->setSize(button->getTextSize().width + 12, button->getSize().height);

            ToolTips::createSkillToolTip(button, skill->mId);
        }

        gamepadHighlightSelected();

        center();
    }

    void TrainingWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onTrainingSelected(MyGUI::Widget* sender)
    {
        const ESM::Skill* skill = *sender->getUserData<const ESM::Skill*>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        int price = pcStats.getSkill(skill->mId).getBase()
            * store.get<ESM::GameSetting>().find("iTrainingMod")->mValue.getInteger();
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        if (getSkillForTraining(mPtr.getClass().getNpcStats(mPtr), skill->mId)
            <= pcStats.getSkill(skill->mId).getBase())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sServiceTrainingWords}");
            return;
        }

        // You can not train a skill above its governing attribute
        if (pcStats.getSkill(skill->mId).getBase()
            >= pcStats.getAttribute(ESM::Attribute::indexToRefId(skill->mData.mAttribute)).getBase())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage17}");
            return;
        }

        // increase skill
        MWWorld::LiveCellRef<ESM::NPC>* playerRef = player.get<ESM::NPC>();

        const ESM::Class* class_ = store.get<ESM::Class>().find(playerRef->mBase->mClass);
        pcStats.increaseSkill(skill->mId, *class_, true);

        // remove gold
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::NpcStats& npcStats = mPtr.getClass().getNpcStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        setVisible(false);
        mProgressBar.setVisible(true);
        mProgressBar.setProgress(0, 2);
        mTimeAdvancer.run(2);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.2);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.2, false, 0.2);
    }

    void TrainingWindow::onTrainingProgressChanged(int cur, int total)
    {
        mProgressBar.setProgress(cur, total);
    }

    void TrainingWindow::onTrainingFinished()
    {
        mProgressBar.setVisible(false);

        // advance time
        MWBase::Environment::get().getMechanicsManager()->rest(2, false);
        MWBase::Environment::get().getWorld()->advanceTime(2);

        // go back to game mode
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    float TrainingWindow::getSkillForTraining(const MWMechanics::NpcStats& stats, ESM::RefId id) const
    {
        if (Settings::game().mTrainersTrainingSkillsBasedOnBaseSkill)
            return stats.getSkill(id).getBase();
        return stats.getSkill(id).getModified();
    }

    void TrainingWindow::onFrame(float dt)
    {
        checkReferenceAvailable();
        mTimeAdvancer.onFrame(dt);
    }

    bool TrainingWindow::exit()
    {
        return !mTimeAdvancer.isRunning();
    }

    void TrainingWindow::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (character != 1) // Gamepad control.
            return;

        int trainingCount = mTrainingOptions->getChildCount();

        if (trainingCount == 0)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        //TODO: support going through active effects; for now, just support spell selection

        if (action == MWInput::MenuAction::MA_DPadDown)
        {
            if (mGamepadSelected < trainingCount - 1)
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
            onTrainingSelected(mTrainingOptions->getChildAt(mGamepadSelected));

            gamepadHighlightSelected();
        }
        else if (action == MWInput::MenuAction::MA_B)
        {
            onCancelButtonClicked(sender);
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
        }
    }

    void TrainingWindow::gamepadHighlightSelected()
    {
        int trainingCount = mTrainingOptions->getChildCount();

        if (mGamepadSelected > trainingCount - 1)
            mGamepadSelected = trainingCount - 1;
        if (mGamepadSelected < 0)
            mGamepadSelected = 0;

        if (trainingCount)
        {
            widgetHighlight(mTrainingOptions->getChildAt(mGamepadSelected));

            updateGamepadTooltip(mTrainingOptions->getChildAt(mGamepadSelected));
        }
        else
        {
            widgetHighlight(nullptr);
        }
    }

    ControlSet TrainingWindow::getControlLegendContents()
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
