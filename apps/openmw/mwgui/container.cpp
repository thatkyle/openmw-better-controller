#include "container.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/aipackage.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwscript/interpretercontext.hpp"

#include "countdialog.hpp"
#include "inventorywindow.hpp"

#include "containeritemmodel.hpp"
#include "draganddrop.hpp"
#include "inventoryitemmodel.hpp"
#include "itemview.hpp"
#include "pickpocketitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tooltips.hpp"
#include "controllegend.hpp"

namespace MWGui
{

    ContainerWindow::ContainerWindow(DragAndDrop* dragAndDrop)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSortModel(nullptr)
        , mModel(nullptr)
        , mSelectedItem(-1)
        , mTreatNextOpenAsLoot(false)
        , mLastAction(MWInput::MA_None)
        , mGamepadSelected(0)
    {
        getWidget(mDisposeCorpseButton, "DisposeCorpseButton");
        getWidget(mTakeButton, "TakeButton");
        getWidget(mCloseButton, "CloseButton");

        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &ContainerWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ContainerWindow::onItemSelected);
        mItemView->eventKeyButtonPressed += MyGUI::newDelegate(this, &ContainerWindow::onKeyButtonPressed);
        mItemView->eventKeySetFocus += MyGUI::newDelegate(this, &ContainerWindow::onFocusGained);
        mItemView->eventKeyLostFocus += MyGUI::newDelegate(this, &ContainerWindow::onFocusLost);

        mDisposeCorpseButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &ContainerWindow::onDisposeCorpseButtonClicked);
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

        setCoord(200, 0, 600, 300);
    }

    void ContainerWindow::onItemSelected(int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            dropItem();
            return;
        }

        const ItemStack& item = mSortModel->getItem(index);

        // We can't take a conjured item from a container (some NPC we're pickpocketing, a box, etc)
        if (item.mFlags & ItemStack::Flag_Bound)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage1}");
            return;
        }

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;
        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        mSelectedItem = mSortModel->mapToSource(index);

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string name{ object.getClass().getName(object) };
            name += MWGui::ToolTips::getSoulString(object.getCellRef());
            dialog->openCountDialog(name, "#{sTake}", count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else
            dragItem(nullptr, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* sender, int count)
    {
        if (!mModel)
            return;

        if (!onTakeItem(mModel->getItem(mSelectedItem), count))
            return;

        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);

        if (MWBase::Environment::get().getInputManager()->joystickLastUsed())
            gamepadDelayedAction();
    }

    void ContainerWindow::dropItem()
    {
        if (!mModel)
            return;

        bool success = mModel->onDropItem(mDragAndDrop->mItem.mBase, mDragAndDrop->mDraggedCount);

        if (success)
            mDragAndDrop->drop(mModel, mItemView);
    }

    void ContainerWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
            dropItem();
    }

    void ContainerWindow::setPtr(const MWWorld::Ptr& container)
    {
        if (container.isEmpty() || (container.getType() != ESM::REC_CONT && !container.getClass().isActor()))
            throw std::runtime_error("Invalid argument in ContainerWindow::setPtr");
        bool lootAnyway = mTreatNextOpenAsLoot;
        mTreatNextOpenAsLoot = false;
        mPtr = container;

        bool loot = mPtr.getClass().isActor() && mPtr.getClass().getCreatureStats(mPtr).isDead();

        std::unique_ptr<ItemModel> model;
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            if (mPtr.getClass().isNpc() && !loot && !lootAnyway)
            {
                // we are stealing stuff
                model = std::make_unique<PickpocketItemModel>(mPtr, std::make_unique<InventoryItemModel>(container),
                    !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown());
            }
            else
                model = std::make_unique<InventoryItemModel>(container);
        }
        else
        {
            model = std::make_unique<ContainerItemModel>(container);
        }

        mDisposeCorpseButton->setVisible(loot);
        mModel = model.get();
        auto sortModel = std::make_unique<SortFilterItemModel>(std::move(model));
        mSortModel = sortModel.get();

        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        setTitle(container.getClass().getName(container));
    }

    void ContainerWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(nullptr);
        mModel = nullptr;
        mSortModel = nullptr;
        mGamepadSelected = 0;
    }

    void ContainerWindow::onClose()
    {
        WindowBase::onClose();

        // Make sure the window was actually closed and not temporarily hidden.
        if (MWBase::Environment::get().getWindowManager()->containsMode(GM_Container))
            return;

        if (mModel)
            mModel->onClose();

        if (!mPtr.isEmpty())
            MWBase::Environment::get().getMechanicsManager()->onClose(mPtr);
        resetReference();
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
    {
        if (!mModel)
            return;
        if (mDragAndDrop != nullptr && mDragAndDrop->mIsOnDragAndDrop)
            return;

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        // transfer everything into the player's inventory
        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        assert(mModel);
        mModel->update();

        // unequip all items to avoid unequipping/reequipping
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
            for (size_t i = 0; i < mModel->getItemCount(); ++i)
            {
                const ItemStack& item = mModel->getItem(i);
                if (invStore.isEquipped(item.mBase) == false)
                    continue;

                invStore.unequipItem(item.mBase);
            }
        }

        mModel->update();

        for (size_t i = 0; i < mModel->getItemCount(); ++i)
        {
            if (i == 0)
            {
                // play the sound of the first object
                MWWorld::Ptr item = mModel->getItem(i).mBase;
                const ESM::RefId& sound = item.getClass().getUpSoundId(item);
                MWBase::Environment::get().getWindowManager()->playSound(sound);
            }

            const ItemStack& item = mModel->getItem(i);

            if (!onTakeItem(item, item.mCount))
                break;

            mModel->moveItem(item, item.mCount, playerModel);
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget* sender)
    {
        if (mDragAndDrop == nullptr || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

            // Copy mPtr because onTakeAllButtonClicked closes the window which resets the reference
            MWWorld::Ptr ptr = mPtr;
            onTakeAllButtonClicked(mTakeButton);

            if (ptr.getClass().isPersistent(ptr))
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDisposeCorpseFail}");
            else
            {
                MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);

                // If we dispose corpse before end of death animation, we should update death counter counter manually.
                // Also we should run actor's script - it may react on actor's death.
                if (creatureStats.isDead() && !creatureStats.isDeathAnimationFinished())
                {
                    creatureStats.setDeathAnimationFinished(true);
                    MWBase::Environment::get().getMechanicsManager()->notifyDied(ptr);

                    const ESM::RefId& script = ptr.getClass().getScript(ptr);
                    if (!script.empty() && MWBase::Environment::get().getWorld()->getScriptsEnabled())
                    {
                        MWScript::InterpreterContext interpreterContext(&ptr.getRefData().getLocals(), ptr);
                        MWBase::Environment::get().getScriptManager()->run(script, interpreterContext);
                    }

                    // Clean up summoned creatures as well
                    auto& creatureMap = creatureStats.getSummonedCreatureMap();
                    for (const auto& creature : creatureMap)
                        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(ptr, creature.second);
                    creatureMap.clear();

                    // Check if we are a summon and inform our master we've bit the dust
                    for (const auto& package : creatureStats.getAiSequence())
                    {
                        if (package->followTargetThroughDoors() && !package->getTarget().isEmpty())
                        {
                            const auto& summoner = package->getTarget();
                            auto& summons = summoner.getClass().getCreatureStats(summoner).getSummonedCreatureMap();
                            auto it = std::find_if(summons.begin(), summons.end(),
                                [&](const auto& entry) { return entry.second == creatureStats.getActorId(); });
                            if (it != summons.end())
                            {
                                auto summon = *it;
                                summons.erase(it);
                                MWMechanics::purgeSummonEffect(summoner, summon);
                                break;
                            }
                        }
                    }
                }

                MWBase::Environment::get().getWorld()->deleteObject(ptr);
            }

            mPtr = MWWorld::Ptr();
        }
    }

    void ContainerWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    bool ContainerWindow::onTakeItem(const ItemStack& item, int count)
    {
        return mModel->onTakeItem(item.mBase, count);
    }

    void ContainerWindow::onDeleteCustomData(const MWWorld::Ptr& ptr)
    {
        if (mModel && mModel->usesContainer(ptr))
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::focus()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mItemView);
    }

    void ContainerWindow::onFocusGained(MyGUI::Widget* sender, MyGUI::Widget* oldFocus)
    {
        gamepadHighlightSelected();
    }

    void ContainerWindow::onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus)
    {
        updateHighlightVisibility();
    }

    ControlSet ContainerWindow::getControlLegendContents()
    {
        std::vector<MenuControl> leftControls{
            MenuControl{MWInput::MenuAction::MA_A, "Select"},
            MenuControl{MWInput::MenuAction::MA_X, "Take All"}
        };
        std::vector<MenuControl> rightControls{
            MenuControl{MWInput::MenuAction::MA_LTrigger, "Inventory"},
            MenuControl{MWInput::MenuAction::MA_B, "Back"}
        };

        if (mDisposeCorpseButton->isVisible())
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_White, "Dispose" });

        return { leftControls, rightControls };
    }

    void ContainerWindow::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        mLastAction = MWInput::MA_None;
        if (character != 1) // Gamepad control.
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        MyGUI::Widget* filterButton = nullptr;

        if (mSortModel->getItemCount() == 0)
            isFilterCycleMode = true; // Highlight filter selection if current sort has no elements (shortcut useful for selling all goods under a sort).

        switch (action)
        {
        case MWInput::MA_A:
        {
            mLastAction = action;
            onItemSelected(mGamepadSelected);

            gamepadHighlightSelected();
            break;
        }
        case MWInput::MA_B:
            onCloseButtonClicked(mCloseButton);
            break;
        case MWInput::MA_X:
            onTakeAllButtonClicked(mTakeButton);
            break;
        case MWInput::MA_Y:
            // TODO: Actually make the tooltip; will need to create a new static window at top of screen
            break;
        case MWInput::MA_LTrigger: // Trigger for menu cycling (inventory stats map magic) should be handled by upper window function.
        case MWInput::MA_RTrigger:
            if (MWBase::Environment::get().getWindowManager()->processInventoryTrigger(action, GM_Container))
            {
                MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
            }
            break; // Go to barter/container when trading
                   // Perhaps trade window should handle this. Go to inventory when trading/container.
        case MWInput::MA_DPadLeft:
            // if we're in the first column, break
            if (mGamepadSelected < mItemView->getRowCount())
                break;

            mGamepadSelected -= mItemView->getRowCount();
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadRight:
            // if we're in the last column, break
            if (mGamepadSelected + mItemView->getRowCount() >= mSortModel->getItemCount())
                break;

            mGamepadSelected += mItemView->getRowCount();
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadUp:
            // if we're in the first row, break
            if (mGamepadSelected % mItemView->getRowCount() == 0)
                break;

            --mGamepadSelected;
            gamepadHighlightSelected();
            break;
        case MWInput::MA_DPadDown:
            // if we're in the last row, break
            if (mGamepadSelected % mItemView->getRowCount() == mItemView->getRowCount() - 1)
                break;

            ++mGamepadSelected;
            gamepadHighlightSelected();
            break;
        case MWInput::MA_White:
            if (!mDisposeCorpseButton->getVisible())
                MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            else
                onDisposeCorpseButtonClicked(mDisposeCorpseButton);
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    void ContainerWindow::gamepadHighlightSelected()
    {
        if (mGamepadSelected > (int)mSortModel->getItemCount() - 1)
            mGamepadSelected = (int)mSortModel->getItemCount() - 1;
        if (mGamepadSelected < 0)
            mGamepadSelected = 0;

        if (mSortModel->getItemCount())
        {
            mItemView->highlightItem(mGamepadSelected);
            widgetHighlight(mItemView->getHighlightWidget());

            updateGamepadTooltip(mItemView->getHighlightWidget());
        }
        else
        {
            isFilterCycleMode = true;
            widgetHighlight(nullptr);
        }
    }
    
    void ContainerWindow::gamepadDelayedAction()
    {
        switch (mLastAction)
        {
        case MWInput::MA_A:
            if (mDragAndDrop->mIsOnDragAndDrop)
            {
                // onAvatarClicked has additional functionality that can't be used.
                MWWorld::Ptr ptr = mDragAndDrop->mItem.mBase;
                mDragAndDrop->finish();
                MWBase::Environment::get().getWindowManager()->getInventoryWindow()->onBackgroundSelected();
            }
            break;
        case MWInput::MA_Unequip:
            onBackgroundSelected();
            break;
        }


        mLastAction = MWInput::MA_None;
        gamepadHighlightSelected();
    }

}
