#include "scrollwindow.hpp"

#include <MyGUI_ScrollView.h>

#include <components/esm3/loadbook.hpp>
#include <components/esm4/loadbook.hpp>
#include <components/widgets/imagebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/actiontake.hpp"
#include "../mwworld/class.hpp"

#include "formatting.hpp"
#include "controllegend.hpp"

namespace MWGui
{

    ScrollWindow::ScrollWindow()
        : BookWindowBase("openmw_scroll.layout")
        , mTakeButtonShow(true)
        , mTakeButtonAllowed(true)
    {
        getWidget(mTextView, "TextView");

        getWidget(mCloseButton, "CloseButton");
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onCloseButtonClicked);

        getWidget(mTakeButton, "TakeButton");
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onTakeButtonClicked);

        adjustButton("CloseButton");
        adjustButton("TakeButton");

        mCloseButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &ScrollWindow::onKeyButtonPressed);
        mTakeButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &ScrollWindow::onKeyButtonPressed);

        center();
    }

    ControlSet ScrollWindow::getControlLegendContents()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        bool showTakeButton = mScroll.getContainerStore() != &player.getClass().getContainerStore(player);

        std::vector<MenuControl> leftControls;
        std::vector<MenuControl> rightControls{
            MenuControl{MWInput::MenuAction::MA_B, "Back"}
        };

        if (showTakeButton)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_A, "Take" });
        if (mTextView->getViewOffset().top > 0)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_LTrigger, "Scroll Up" });
        if (mTextView->getViewOffset().top < mTextView->getChildAt(0)->getSize().height)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_RTrigger, "Scroll Down" });

        return { leftControls, rightControls };
    }

    ControlSet ScrollWindow::getControlLegendContents()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        bool showTakeButton = mScroll.getContainerStore() != &player.getClass().getContainerStore(player);

        std::vector<MenuControl> leftControls;
        std::vector<MenuControl> rightControls{
            MenuControl{MWInput::MenuAction::MA_B, "Back"}
        };

        if (showTakeButton)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_A, "Take" });
        if (mTextView->getViewOffset().top > 0)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_LTrigger, "Scroll Up" });
        if (mTextView->getViewOffset().top < mTextView->getChildAt(0)->getSize().height)
            leftControls.push_back(MenuControl{ MWInput::MenuAction::MA_RTrigger, "Scroll Down" });

        return { leftControls, rightControls };
    }

    void ScrollWindow::setPtr(const MWWorld::Ptr& scroll)
    {
        if (scroll.isEmpty() || (scroll.getType() != ESM::REC_BOOK && scroll.getType() != ESM::REC_BOOK4))
            throw std::runtime_error("Invalid argument in ScrollWindow::setPtr");
        mScroll = scroll;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        bool showTakeButton = scroll.getContainerStore() != &player.getClass().getContainerStore(player);

        const std::string* text;
        if (scroll.getType() == ESM::REC_BOOK)
            text = &scroll.get<ESM::Book>()->mBase->mText;
        else
            text = &scroll.get<ESM4::Book>()->mBase->mText;
        bool shrinkTextAtLastTag = scroll.getType() == ESM::REC_BOOK;

        Formatting::BookFormatter formatter;
        formatter.markupToWidget(mTextView, *text, 390, mTextView->getHeight(), shrinkTextAtLastTag);
        MyGUI::IntSize size = mTextView->getChildAt(0)->getSize();

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mTextView->setVisibleVScroll(false);
        if (size.height > mTextView->getSize().height)
            mTextView->setCanvasSize(mTextView->getWidth(), size.height);
        else
            mTextView->setCanvasSize(mTextView->getWidth(), mTextView->getSize().height);
        mTextView->setVisibleVScroll(true);

        mTextView->setViewOffset(MyGUI::IntPoint(0, 0));

        setTakeButtonShow(showTakeButton);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);
        MWBase::Environment::get().getWindowManager()->setMenuControls(getControlLegendContents());
    }

    void ScrollWindow::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (character != 1)
        {
            int scroll = 0;
            if (key == MyGUI::KeyCode::ArrowUp)
                scroll = 40;
            else if (key == MyGUI::KeyCode::ArrowDown)
                scroll = -40;

            if (scroll != 0)
                mTextView->setViewOffset(mTextView->getViewOffset() + MyGUI::IntPoint(0, scroll));

            return;
        }

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        MWWorld::Ptr player = MWMechanics::getPlayer();
        bool showTakeButton = mScroll.getContainerStore() != &player.getClass().getContainerStore(player);

        if (action == MWInput::MenuAction::MA_A && showTakeButton)
        {
            onTakeButtonClicked(sender);
            return;
        }
        else if (action == MWInput::MenuAction::MA_B)
        {
            onCloseButtonClicked(sender);
            return;
        }
        else if (action == MWInput::MA_LTrigger)
            mTextView->setViewOffset(mTextView->getViewOffset() + MyGUI::IntPoint(0, 40.f * MWBase::Environment::get().getInputManager()->getAxisRatio(static_cast<int>(action))));
        else if (action == MWInput::MA_RTrigger)
            mTextView->setViewOffset(mTextView->getViewOffset() + MyGUI::IntPoint(0, -40.f * MWBase::Environment::get().getInputManager()->getAxisRatio(static_cast<int>(action))));

        MWBase::Environment::get().getWindowManager()->setMenuControls(getControlLegendContents());
    }

    void ScrollWindow::setTakeButtonShow(bool show)
    {
        mTakeButtonShow = show;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void ScrollWindow::setInventoryAllowed(bool allowed)
    {
        mTakeButtonAllowed = allowed;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void ScrollWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
    }

    void ScrollWindow::onTakeButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Item Book Up"));

        MWWorld::ActionTake take(mScroll);
        take.execute(MWMechanics::getPlayer());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
    }
}
