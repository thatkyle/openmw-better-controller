#include "windowbase.hpp"

#include <climits>
#include <vector>
#include <tuple>

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include <components/widgets/imagebutton.hpp>

#include "draganddrop.hpp"
#include "exposedwindow.hpp"
#include "controllegend.hpp"
#include "itemwidget.hpp"

using namespace MWGui;

WindowBase::WindowBase(std::string_view parLayout)
    : Layout(parLayout)
  , mUsesHighlightOffset(false)
  , mUsesHighlightSizeOverride(false)
{
    mMainWidget->setVisible(false);

    mIsHighlightHidden = true;
    mGamepadHighlight = mMainWidget->createWidget<MyGUI::Widget>("MW_Highlight_Frame", 0, 0, 0, 0, MyGUI::Align::Default, mPrefix + "GamepadHighlight");
    mGamepadHighlight->setVisible(false);
    mGamepadHighlight->setDepth(INT_MAX);
    mGamepadHighlight->setNeedMouseFocus(false);
    mGamepadHighlight->setNeedKeyFocus(false);

    Window* window = mMainWidget->castType<Window>(false);
    if (!window)
        return;

    MyGUI::Button* button = nullptr;
    MyGUI::VectorWidgetPtr widgets = window->getSkinWidgetsByName("Action");
    for (MyGUI::Widget* widget : widgets)
    {
        if (widget->isUserString("SupportDoubleClick"))
            button = widget->castType<MyGUI::Button>();
    }

    if (button)
        button->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WindowBase::onDoubleClick);
}

void WindowBase::onTitleDoubleClicked()
{
    if (MyGUI::InputManager::getInstance().isShiftPressed())
        MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
}

void WindowBase::onDoubleClick(MyGUI::Widget* _sender)
{
    onTitleDoubleClicked();
}

void WindowBase::setVisible(bool visible)
{
    visible = visible && !mDisabledByLua;
    bool wasVisible = mMainWidget->getVisible();
    mMainWidget->setVisible(visible);

    if (visible)
        onOpen();
    else if (wasVisible)
        onClose();
}

bool WindowBase::isVisible()
{
    return mMainWidget->getVisible();
}

void WindowBase::center()
{
    // Centre dialog

    MyGUI::IntSize layerSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (mMainWidget->getLayer())
        layerSize = mMainWidget->getLayer()->getSize();

    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (layerSize.width - coord.width) / 2;
    coord.top = (layerSize.height - coord.height) / 2;
    mMainWidget->setCoord(coord);
}

void WindowBase::widgetHighlight(MyGUI::Widget *target)
{
    if (target)
    {
        mIsHighlightHidden = false;

        // move the highlight to its new location
        auto firstChildInHierarchy = target;
        while (!firstChildInHierarchy->getParent()->isRootWidget())
            firstChildInHierarchy = firstChildInHierarchy->getParent();

        // we substract the absolute point of the first non-root element in the hierarchy to account for any padding
        // that may be in the main window
        auto coords = target->getAbsoluteCoord() - firstChildInHierarchy->getAbsoluteCoord().point();
        
        if (mUsesHighlightSizeOverride)
        {
            auto sizeOverride = highlightSizeOverride();

            coords.width = sizeOverride.width;
            coords.height = sizeOverride.height;
        }

        if (mUsesHighlightOffset)
            coords += highlightOffset();

        //if (dynamic_cast<MyGUI::EditBox*>(target) != nullptr ||
        //    dynamic_cast<MyGUI::Button*>(target) != nullptr ||
        //    dynamic_cast<ItemWidget*>(target) != nullptr)
        //{
        //    coords += MyGUI::IntCoord(-4, -4, 8, 8);
        //}

        mGamepadHighlight->setCoord(coords);
        mLastHighlightCoord = coords;

        Log(Debug::Info) << "Highlight coords for layout " << mLayoutName << ": " << coords;
    }
    else
        mIsHighlightHidden = true;
        
    updateHighlightVisibility();
}

void WindowBase::updateHighlightVisibility()
{
    // only turn it on if the key focus widget is in this layout; this allows us to update widget positions without
    // actually turning it on.
    bool shouldTurnOn = !mIsHighlightHidden && MWBase::Environment::get().getInputManager()->joystickLastUsed()
        && !MWBase::Environment::get().getInputManager()->isGamepadGuiCursorEnabled()
        && isWidgetInLayout(MyGUI::InputManager::getInstance().getKeyFocusWidget());

    mGamepadHighlight->setVisible(shouldTurnOn);

    if (shouldTurnOn)
    {
        mGamepadHighlight->setCoord(mLastHighlightCoord);
        MWBase::Environment::get().getWindowManager()->setMenuControls(getControlLegendContents());
    }
    else
    {
        mLastHighlightCoord = mGamepadHighlight->getCoord();
        mGamepadHighlight->setCoord(MyGUI::IntCoord());
    }
}

ControlSet WindowBase::getControlLegendContents()
{
    return { {}, {} };
}

void WindowBase::updateGamepadTooltip(MyGUI::Widget* target)
{
    if (!target)
        MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(nullptr, nullptr);
    else if (mMainWidget->isVisible() && isWidgetInLayout(MyGUI::InputManager::getInstance().getKeyFocusWidget()))
        MWBase::Environment::get().getWindowManager()->setGamepadGuiFocusWidget(target, this);
}

WindowModal::WindowModal(const std::string& parLayout)
    : WindowBase(parLayout)
{
}

void WindowModal::onOpen()
{
    MWBase::Environment::get().getWindowManager()->addCurrentModal(this); // Set so we can escape it if needed

    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(focus);

    MWBase::Environment::get().getWindowManager()->setMenuControls(getControlLegendContents());
}

void WindowModal::onClose()
{
    MWBase::Environment::get().getWindowManager()->removeCurrentModal(this);

    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);

    MWBase::Environment::get().getWindowManager()->clearMenuControls();
}

ControlSet WindowModal::getControlLegendContents()
{
    return {
        {
            MenuControl{MWInput::MenuAction::MA_A, "Select"}
        },
        {
            MenuControl{MWInput::MenuAction::MA_B, "Back"}
        }
    };
}

NoDrop::NoDrop(DragAndDrop* drag, MyGUI::Widget* widget)
    : mWidget(widget)
    , mDrag(drag)
    , mTransparent(false)
{
}

void NoDrop::onFrame(float dt)
{
    if (!mWidget)
        return;

    MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

    if (mDrag->mIsOnDragAndDrop)
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        while (focus && focus != mWidget)
            focus = focus->getParent();

        if (focus == mWidget)
            mTransparent = true;
    }
    if (!mWidget->getAbsoluteCoord().inside(mousePos))
        mTransparent = false;

    if (mTransparent)
    {
        mWidget->setNeedMouseFocus(false); // Allow click-through
        setAlpha(std::max(0.13f, mWidget->getAlpha() - dt * 5));
    }
    else
    {
        mWidget->setNeedMouseFocus(true);
        setAlpha(std::min(1.0f, mWidget->getAlpha() + dt * 5));
    }
}

void NoDrop::setAlpha(float alpha)
{
    if (mWidget)
        mWidget->setAlpha(alpha);
}

BookWindowBase::BookWindowBase(std::string_view parLayout)
    : WindowBase(parLayout)
{
}

float BookWindowBase::adjustButton(std::string_view name)
{
    Gui::ImageButton* button;
    WindowBase::getWidget(button, name);
    MyGUI::IntSize requested = button->getRequestedSize();
    float scale = float(requested.height) / button->getSize().height;
    MyGUI::IntSize newSize = requested;
    newSize.width /= scale;
    newSize.height /= scale;
    button->setSize(newSize);

    if (button->getAlign().isRight())
    {
        MyGUI::IntSize diff = (button->getSize() - requested);
        diff.width /= scale;
        diff.height /= scale;
        button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width, 0));
    }

    return scale;
}
