#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include "layout.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    class DragAndDrop;

    struct ControlSet;

    struct ControlSet;

    class WindowBase : public Layout
    {
    public:
        WindowBase(std::string_view parLayout);

        virtual MyGUI::Widget* getDefaultKeyFocus() { return nullptr; }

        // Events
        typedef MyGUI::delegates::MultiDelegate<WindowBase*> EventHandle_WindowBase;

        /// Open this object in the GUI, for windows that support it
        virtual void setPtr(const MWWorld::Ptr& ptr) {}

        /// Called every frame if the window is in an active GUI mode
        virtual void onFrame(float duration) {}

        /// Notify that window has been made visible
        virtual void onOpen() {}
        /// Notify that window has been hidden
        virtual void onClose() {}
        /// Gracefully exits the window
        virtual bool exit() { return true; }
        /// Sets the visibility of the window
        void setVisible(bool visible) override;
        /// Returns the visibility state of the window
        bool isVisible();

        virtual void focus() {};

        void center();

        /// Clear any state specific to the running game
        virtual void clear() {}

        /// Called when GUI viewport changes size
        virtual void onResChange(int width, int height) {}

        virtual void onDeleteCustomData(const MWWorld::Ptr& ptr) {}

        virtual std::string_view getWindowIdForLua() const { return ""; }
        void setDisabledByLua(bool disabled) { mDisabledByLua = disabled; }

        /// Place gamepad highlight on a given child widget.
        void widgetHighlight(MyGUI::Widget *target);
        void updateHighlightVisibility();
        void updateGamepadTooltip(MyGUI::Widget* target);

    protected:
        virtual void onTitleDoubleClicked();

        bool mUsesHighlightSizeOverride;
        bool mUsesHighlightOffset;

        /// if mUsesHighlightSizeOverride is true, overrides the size of the highlight (not the position)
        virtual MyGUI::IntSize highlightSizeOverride() { return MyGUI::IntSize(); }
        /// if mUsesHighlightOffset is true, offsets the highlight by the given position IntCoord
        virtual MyGUI::IntCoord highlightOffset() { return MyGUI::IntCoord(); }

        /// should be overridden for any window needing control legend information
        virtual ControlSet getControlLegendContents();

        MyGUI::Widget* mGamepadHighlight;
        MyGUI::IntCoord mLastHighlightCoord;

    private:
        void onDoubleClick(MyGUI::Widget* _sender);

        bool mDisabledByLua = false;

        virtual void onFocusGained(MyGUI::Widget* sender, MyGUI::Widget* oldFocus) {};
        virtual void onFocusLost(MyGUI::Widget* sender, MyGUI::Widget* newFocus) {};

        bool mIsHighlightHidden;
    };

    /*
     * "Modal" windows cause the rest of the interface to be inaccessible while they are visible
     */
    class WindowModal : public WindowBase
    {
    public:
        WindowModal(const std::string& parLayout);
        void onOpen() override;
        void onClose() override;
        bool exit() override { return true; }

    protected:
        ControlSet getControlLegendContents() override;

        MyGUI::IntCoord highlightOffset() override { return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8)); };
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);
        int mHighlight;
    };

    /// A window that cannot be the target of a drag&drop action.
    /// When hovered with a drag item, the window will become transparent and allow click-through.
    class NoDrop
    {
    public:
        NoDrop(DragAndDrop* drag, MyGUI::Widget* widget);

        void onFrame(float dt);
        virtual void setAlpha(float alpha);
        virtual ~NoDrop() = default;

    private:
        MyGUI::Widget* mWidget;
        DragAndDrop* mDrag;
        bool mTransparent;
    };

    class BookWindowBase : public WindowBase
    {
    public:
        BookWindowBase(std::string_view parLayout);

    protected:
        float adjustButton(std::string_view name);
    };
}

#endif
