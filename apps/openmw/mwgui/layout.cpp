#include "layout.hpp"

#include <MyGUI_LayoutManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Window.h>

namespace MWGui
{
    void Layout::initialise(const std::string& _layout, MyGUI::Widget* _parent)
    {
        const std::string MAIN_WINDOW = "_Main";
        mLayoutName = _layout;

        if (mLayoutName.empty())
            mMainWidget = _parent;
        else
        {
            mPrefix = MyGUI::utility::toString(this, "_");
            mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix, _parent);

            const std::string main_name = mPrefix + MAIN_WINDOW;
            for (MyGUI::Widget* widget : mListWindowRoot)
            {
                if (widget->getName() == main_name)
                {
                    mMainWidget = widget;
                    break;
                }
            }
            MYGUI_ASSERT(mMainWidget, "root widget name '" << MAIN_WINDOW << "' in layout '" << mLayoutName << "' not found.");
        }
    }

    void Layout::shutdown()
    {
        setVisible(false);
        MyGUI::Gui::getInstance().destroyWidget(mMainWidget);
        mListWindowRoot.clear();
    }

    void Layout::setCoord(int x, int y, int w, int h)
    {
        mMainWidget->setCoord(x,y,w,h);
    }

    void Layout::setVisible(bool b)
    {
        mMainWidget->setVisible(b);
    }

    void Layout::setText(const std::string &name, const std::string &caption)
    {
        MyGUI::Widget* pt;
        getWidget(pt, name);
        static_cast<MyGUI::TextBox*>(pt)->setCaption(caption);
    }

    void Layout::setTitle(const std::string& title)
    {
        MyGUI::Window* window = static_cast<MyGUI::Window*>(mMainWidget);

        if (window->getCaption() != title)
            window->setCaptionWithReplacing(title);
    }

    MyGUI::Widget* Layout::getWidget(const std::string &_name)
    {
        return getWidgetByFullName(mPrefix + _name);
    }

    MyGUI::Widget* Layout::getWidgetByFullName(const std::string& _name) {
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            MyGUI::Widget* findResult = widget->findWidget(_name);
            if (findResult)
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
