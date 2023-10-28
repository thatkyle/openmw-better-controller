#include "layout.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_LayoutManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Window.h>

#include "ustring.hpp"

namespace MWGui
{
    void Layout::initialise(std::string_view _layout)
    {
        const auto MAIN_WINDOW = "_Main";
        mLayoutName = _layout;

        mPrefix = MyGUI::utility::toString(this, "_");
        mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix);

        const std::string main_name = mPrefix + MAIN_WINDOW;
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            if (widget->getName() == main_name)
                mMainWidget = widget;

            // Force the alignment to update immediately
            widget->_setAlign(widget->getSize(), widget->getParentSize());
        }
        MYGUI_ASSERT(
            mMainWidget, "root widget name '" << MAIN_WINDOW << "' in layout '" << mLayoutName << "' not found.");
    }

    void Layout::shutdown()
    {
        setVisible(false);
        MyGUI::Gui::getInstance().destroyWidget(mMainWidget);
        mListWindowRoot.clear();
    }

    void Layout::setCoord(int x, int y, int w, int h)
    {
        mMainWidget->setCoord(x, y, w, h);
    }

    void Layout::setVisible(bool b)
    {
        mMainWidget->setVisible(b);
    }

    void Layout::setText(std::string_view name, std::string_view caption)
    {
        MyGUI::Widget* pt;
        getWidget(pt, name);
        static_cast<MyGUI::TextBox*>(pt)->setCaption(toUString(caption));
    }

    void Layout::setTitle(std::string_view title)
    {
        MyGUI::Window* window = static_cast<MyGUI::Window*>(mMainWidget);
        MyGUI::UString uTitle = toUString(title);

        if (window->getCaption() != uTitle)
            window->setCaptionWithReplacing(uTitle);
    }

    MyGUI::Widget* Layout::getWidget(std::string_view _name)
    {
        std::string target = mPrefix;
        target += _name;
        return getWidgetByFullName(mPrefix + _name);
    }

    MyGUI::Widget* Layout::getWidgetByFullName(const std::string& _name) {
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            MyGUI::Widget* find = widget->findWidget(target);
            if (nullptr != find)
            {
                return findResult;
            }
        }
        return nullptr;
    }

    bool Layout::isWidgetInLayout(const MyGUI::Widget* target) {
        return target && (target == mMainWidget || isWidgetInLayout(target->getParent()));
    }

}
