#ifndef MWGUI_WINDOWNAVIGATOR_H
#define MWGUI_WINDOWNAVIGATOR_H

#include <map>
#include <vector>
#include <memory>
#include <functional>

#include <MyGUI_Types.h>

namespace MWInput
{
    enum MenuAction;
}

namespace MWGui
{
    class ItemView;
    class SpellView;

    class NavigationNode: public std::enable_shared_from_this<NavigationNode>
    {
    public:
        std::shared_ptr<NavigationNode> onClick(std::function<void(MyGUI::Widget*)> listener);
        virtual void click();

        std::shared_ptr<NavigationNode> onHover(std::function<void(MyGUI::Widget*)> listener);
        virtual void fireHover();

        //virtual std::shared_ptr<NavigationNode> up()    { return    mUp; == nullptr ? shared_from_this() : mUp; }
        //virtual std::shared_ptr<NavigationNode> down()  { return  mDown == nullptr ? shared_from_this() : mDown; }
        //virtual std::shared_ptr<NavigationNode> left()  { return  mLeft == nullptr ? shared_from_this() : mLeft; }
        //virtual std::shared_ptr<NavigationNode> right() { return mRight == nullptr ? shared_from_this() : mRight; }

        virtual std::shared_ptr<NavigationNode> up() { return mUp; }
        virtual std::shared_ptr<NavigationNode> down() { return mDown; }
        virtual std::shared_ptr<NavigationNode> left() { return mLeft; }
        virtual std::shared_ptr<NavigationNode> right() { return mRight; }

        virtual MyGUI::Widget* getTargetWidget() = 0;
        virtual std::vector<MyGUI::Widget*> getAllWidgets() = 0;

        std::shared_ptr<NavigationNode> mUp;
        std::shared_ptr<NavigationNode> mDown;
        std::shared_ptr<NavigationNode> mLeft;
        std::shared_ptr<NavigationNode> mRight;

    private:
        std::function<void(MyGUI::Widget*)> mClickOverride;
        std::function<void(MyGUI::Widget*)> mOnHover;
    };



    class SimpleNavigationNode : public NavigationNode
    {
    public:
        SimpleNavigationNode(MyGUI::Widget* widget) : mNodeWidget(widget) {};

        MyGUI::Widget* getTargetWidget() override;
        std::vector<MyGUI::Widget*> getAllWidgets() override { return { getTargetWidget() }; }

    private:

        MyGUI::Widget* mNodeWidget;
    };



    class ItemViewNavigationNode : public NavigationNode
    {
    public:
        ItemViewNavigationNode(MWGui::ItemView* itemView);

        void click() override;

        std::shared_ptr<NavigationNode> up() override;
        std::shared_ptr<NavigationNode> down() override;
        std::shared_ptr<NavigationNode> left() override;
        std::shared_ptr<NavigationNode> right() override;

        MyGUI::Widget* getTargetWidget() override;
        std::vector<MyGUI::Widget*> getAllWidgets() override;

    private:
        void onItemSelected(int index);

        MWGui::ItemView* mItemView;
        int mItemSelected;
    };



    class SpellViewNavigationNode : public NavigationNode
    {
    public:
        SpellViewNavigationNode(MWGui::SpellView* itemView);

        void click() override;

        std::shared_ptr<NavigationNode> up() override;
        std::shared_ptr<NavigationNode> down() override;

        MyGUI::Widget* getTargetWidget() override;
        std::vector<MyGUI::Widget*> getAllWidgets() override;

    private:

        MWGui::SpellView* mSpellView;
        int mItemSelected;
    };


    template <class T>
    class ListBoxNavigationNode : public NavigationNode
    {
    public:
        ListBoxNavigationNode(T* listBox);

        void click() override;
        void fireHover() override;

        std::shared_ptr<NavigationNode> up() override;
        std::shared_ptr<NavigationNode> down() override;

        MyGUI::Widget* getTargetWidget() override;
        std::vector<MyGUI::Widget*> getAllWidgets() override;

    private:
        T* mListBox;
        int mItemSelected;
    };



    class NavigationNodeSet : public NavigationNode
    {
    public:
        NavigationNodeSet(std::vector<std::shared_ptr<NavigationNode>> subNodes, bool isVertical)
            : mSubNodes(subNodes), mLastVisitedIndex(0), mIsVertical(isVertical) {}
        
        void click() override;

        std::shared_ptr<NavigationNode> up() override;
        std::shared_ptr<NavigationNode> down() override;
        std::shared_ptr<NavigationNode> left() override;
        std::shared_ptr<NavigationNode> right() override;

        MyGUI::Widget* getTargetWidget() override;
        std::vector<MyGUI::Widget*> getAllWidgets() override;

    private:
        std::vector<std::shared_ptr<NavigationNode>> mSubNodes;
        int mLastVisitedIndex;
        bool mIsVertical;
    };



    class WindowNavigator
    {

    public:
        WindowNavigator() {}
        WindowNavigator(MyGUI::Widget* origin);
        WindowNavigator(MyGUI::EditBox* editBox, const std::function<void()> onTextAccept);

        void selectCurrentNode();
        bool processInput(MWInput::MenuAction action);

        void addNode(std::shared_ptr<NavigationNode> node);

        void addWidget(MyGUI::Widget* widget);
        void addEditBox(MyGUI::EditBox* editBox, const std::function<void()> onTextAccept);

        void addWidgetSet(std::vector<MyGUI::Widget*> widgets, bool isVertical);

        void onClick(MyGUI::Widget* widget, std::function<void(MyGUI::Widget*)> listener);
        void onHover(MyGUI::Widget* widget, std::function<void(MyGUI::Widget*)> listener);

        void addLeftRightConnection(MyGUI::Widget* left, MyGUI::Widget* right);
        void addUpDownConnection(MyGUI::Widget* up, MyGUI::Widget* down);

        MyGUI::Widget* getSelectedWidget() { return mCurrentNode->getTargetWidget(); };

        /// creates navigation nodes based on what type they are
        static std::shared_ptr<NavigationNode> createNode(MyGUI::Widget* widget);

    private:
        std::shared_ptr<NavigationNode> mCurrentNode;
        std::shared_ptr<NavigationNode> mOrigin;

        std::map<MyGUI::Widget*, std::shared_ptr<NavigationNode>> mWidgetsToNodes;
    };
}

#endif
