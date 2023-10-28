#include "windownavigator.hpp"

#include <algorithm>

#include <MyGUI_ListBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "itemview.hpp"
#include "widgets.hpp"
#include "controllegend.hpp"
#include "spellview.hpp"


namespace MWGui
{
    std::shared_ptr<NavigationNode> NavigationNode::onClick(std::function<void(MyGUI::Widget*)> listener)
    {
        // overwrite onClick to do something other than activate the eventMouseButtonClick widget event
        mClickOverride = listener;
        return shared_from_this();
    }

    void NavigationNode::click()
    {
        auto targetWidget = getTargetWidget();

        if (mClickOverride == nullptr)
        {
            if (dynamic_cast<Widgets::MWAttributePtr>(targetWidget) != nullptr)
                static_cast<Widgets::MWAttributePtr>(targetWidget)->eventClicked(static_cast<Widgets::MWAttributePtr>(targetWidget));
            else if (dynamic_cast<Widgets::MWSkillPtr>(targetWidget) != nullptr)
                static_cast<Widgets::MWSkillPtr>(targetWidget)->eventClicked(static_cast<Widgets::MWSkillPtr>(targetWidget));
            else
                targetWidget->eventMouseButtonClick(targetWidget);
        }
        else
            mClickOverride(targetWidget);
    }

    std::shared_ptr<NavigationNode> NavigationNode::onHover(std::function<void(MyGUI::Widget*)> listener)
    {
        mOnHover = listener;
        return shared_from_this();
    }

    void NavigationNode::fireHover()
    {
        if (mOnHover != nullptr)
            mOnHover(getTargetWidget());
    }

    /* ----------------------- */

    MyGUI::Widget* SimpleNavigationNode::getTargetWidget()
    {
        return mNodeWidget;
    }

    /* ----------------------- */

    ItemViewNavigationNode::ItemViewNavigationNode(MWGui::ItemView* itemView)
        : mItemView(itemView)
        , mItemSelected(0)
    {
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ItemViewNavigationNode::onItemSelected);
    }

    void ItemViewNavigationNode::click()
    {
        //TODO add a validation method and call in every relevant method
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = mItemView->getItemCount() - 1;

        if (mItemView->getItemCount() > 0)
            mItemView->eventItemClicked(mItemSelected);
    }

    MyGUI::Widget* ItemViewNavigationNode::getTargetWidget()
    {
        mItemView->highlightItem(mItemSelected);
        return mItemView->getHighlightWidget();
    }

    std::vector<MyGUI::Widget*> ItemViewNavigationNode::getAllWidgets()
    {
        return { mItemView };
    }

    std::shared_ptr<NavigationNode> ItemViewNavigationNode::up()
    {
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = mItemView->getItemCount() - 1;

        // if we're in the first row, return the node above this
        if (mItemSelected % mItemView->getRowCount() == 0)
            return NavigationNode::up();

        mItemSelected--;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> ItemViewNavigationNode::down()
    {
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = mItemView->getItemCount() - 1;

        // if we're in the last row, return the node below this
        if (mItemSelected % mItemView->getRowCount() == mItemView->getRowCount() - 1)
            return NavigationNode::down();

        mItemSelected++;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> ItemViewNavigationNode::left()
    {
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = mItemView->getItemCount() - 1;

        // if we're in the first column, return the node to the left of this
        if (mItemSelected < mItemView->getRowCount())
            return NavigationNode::left();

        mItemSelected -= mItemView->getRowCount();

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> ItemViewNavigationNode::right()
    {
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = mItemView->getItemCount() - 1;

        // if we're in the last column, return the node to the right of this
        if (mItemSelected + mItemView->getRowCount() >= mItemView->getItemCount())
            return NavigationNode::right();

        mItemSelected += mItemView->getRowCount();

        return shared_from_this();
    }

    void ItemViewNavigationNode::onItemSelected(int index)
    {
        // moves the selected item if the selected item was removed and at the end
        if (mItemSelected >= mItemView->getItemCount())
            mItemSelected = std::max(mItemView->getItemCount() - 1, 0);
    }

    /* ----------------------- */

    SpellViewNavigationNode::SpellViewNavigationNode(MWGui::SpellView* spellView)
        : mSpellView(spellView)
        , mItemSelected(0)
    {
    }

    void SpellViewNavigationNode::click()
    {
        if (mSpellView->getItemCount() > 0)
            getTargetWidget()->eventMouseButtonClick(getTargetWidget());
    }

    MyGUI::Widget* SpellViewNavigationNode::getTargetWidget()
    {
        mSpellView->highlightItem(mItemSelected);
        return mSpellView->getHighlightWidget();
    }

    std::vector<MyGUI::Widget*> SpellViewNavigationNode::getAllWidgets()
    {
        return { mSpellView };
    }

    std::shared_ptr<NavigationNode> SpellViewNavigationNode::up()
    {
        if (mItemSelected >= mSpellView->getItemCount())
            mItemSelected = mSpellView->getItemCount() - 1;

        // if we're in the first row, return the node above this
        if (mItemSelected == 0)
            return NavigationNode::up();

        mItemSelected--;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> SpellViewNavigationNode::down()
    {
        if (mItemSelected >= mSpellView->getItemCount())
            mItemSelected = mSpellView->getItemCount() - 1;

        // if we're in the last row, return the node below this
        if (mSpellView->getItemCount() == 0 || mItemSelected >= mSpellView->getItemCount() - 1)
            return NavigationNode::down();

        mItemSelected++;

        return shared_from_this();
    }

    /* ----------------------- */

    template <class T>
    ListBoxNavigationNode<T>::ListBoxNavigationNode(T* listBox)
        : mListBox(listBox)
        , mItemSelected(0)
    {
        
    }

    template <class T>
    void ListBoxNavigationNode<T>::click()
    {
        mListBox->eventListSelectAccept(mListBox, mItemSelected);
    }

    template <class T>
    void ListBoxNavigationNode<T>::fireHover()
    {
        mListBox->setIndexSelected(mItemSelected);
        mListBox->eventListChangePosition(mListBox, mItemSelected);
        mListBox->beginToItemAt(std::max(mItemSelected - 3, 0));
    }

    template <class T>
    MyGUI::Widget* ListBoxNavigationNode<T>::getTargetWidget()
    {
        if (mListBox->getItemCount() == 0)
            return nullptr;
        else
            return mListBox->getChildAt(mItemSelected);
    }

    template <class T>
    std::vector<MyGUI::Widget*> ListBoxNavigationNode<T>::getAllWidgets()
    {
        return { mListBox };
    }

    template <class T>
    std::shared_ptr<NavigationNode> ListBoxNavigationNode<T>::up()
    {
        if (mItemSelected >= mListBox->getItemCount())
            mItemSelected = mListBox->getItemCount() - 1;

        // if we're in the first row, return the node above this
        if (mItemSelected == 0)
            return NavigationNode::up();

        mItemSelected--;

        return shared_from_this();
    }

    template <class T>
    std::shared_ptr<NavigationNode> ListBoxNavigationNode<T>::down()
    {
        if (mItemSelected >= mListBox->getItemCount())
            mItemSelected = mListBox->getItemCount() - 1;

        // if we're in the last row, return the node below this
        if (mListBox->getItemCount() == 0 || mItemSelected == mListBox->getItemCount() - 1)
            return NavigationNode::down();

        mItemSelected++;

        return shared_from_this();
    }

    /* ----------------------- */

    void NavigationNodeSet::click()
    {
        mSubNodes[mLastVisitedIndex]->click();
    }

    MyGUI::Widget* NavigationNodeSet::getTargetWidget()
    {
        return mSubNodes[mLastVisitedIndex]->getTargetWidget();
    }

    std::vector<MyGUI::Widget*> NavigationNodeSet::getAllWidgets()
    {
        std::vector<MyGUI::Widget*> result;

        for (auto node : mSubNodes)
        {
            auto allWidgets = node->getAllWidgets();
            result.insert(result.end(), allWidgets.begin(), allWidgets.end());
        }

        return result;
    }


    std::shared_ptr<NavigationNode> NavigationNodeSet::up()
    {
        if (std::dynamic_pointer_cast<NavigationNodeSet>(mSubNodes[mLastVisitedIndex]) != nullptr)
        {
            // current node is a set as well, so try to navigate that first
            if (mSubNodes[mLastVisitedIndex]->up() != nullptr)
                return shared_from_this();
        }

        if (!mIsVertical || mLastVisitedIndex == 0)
            return NavigationNode::up();

        mLastVisitedIndex--;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> NavigationNodeSet::down()
    {
        if (std::dynamic_pointer_cast<NavigationNodeSet>(mSubNodes[mLastVisitedIndex]) != nullptr)
        {
            // current node is a set as well, so try to navigate that first
            if (mSubNodes[mLastVisitedIndex]->down() != nullptr)
                return shared_from_this();
        }

        if (!mIsVertical || mLastVisitedIndex == mSubNodes.size() - 1)
            return NavigationNode::down();

        mLastVisitedIndex++;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> NavigationNodeSet::left()
    {
        if (std::dynamic_pointer_cast<NavigationNodeSet>(mSubNodes[mLastVisitedIndex]) != nullptr)
        {
            // current node is a set as well, so try to navigate that first
            if (mSubNodes[mLastVisitedIndex]->left() != nullptr)
                return shared_from_this();
        }

        if (mIsVertical || mLastVisitedIndex == 0)
            return NavigationNode::left();

        mLastVisitedIndex--;

        return shared_from_this();
    }

    std::shared_ptr<NavigationNode> NavigationNodeSet::right()
    {
        if (std::dynamic_pointer_cast<NavigationNodeSet>(mSubNodes[mLastVisitedIndex]) != nullptr)
        {
            // current node is a set as well, so try to navigate that first
            if (mSubNodes[mLastVisitedIndex]->right() != nullptr)
                return shared_from_this();
        }

        if (mIsVertical || mLastVisitedIndex == mSubNodes.size() - 1)
            return NavigationNode::right();

        mLastVisitedIndex++;

        return shared_from_this();
    }

    /* ----------------------- */

    WindowNavigator::WindowNavigator(MyGUI::Widget* origin)
    {
        addWidget(origin);

        mOrigin = mWidgetsToNodes[origin];
        mCurrentNode = mOrigin;
    }

    WindowNavigator::WindowNavigator(MyGUI::EditBox* editBox, const std::function<void()> onTextAccept)
    {
        addEditBox(editBox, onTextAccept);

        mOrigin = mWidgetsToNodes[editBox];
        mCurrentNode = mOrigin;
    }

    void WindowNavigator::selectCurrentNode()
    {
        mCurrentNode->click();
    }

    void WindowNavigator::addWidget(MyGUI::Widget* widget)
    {
        if (dynamic_cast<MyGUI::EditBox*>(widget) != nullptr)
        {
            addEditBox(dynamic_cast<MyGUI::EditBox*>(widget), [] {});
            return;
        }

        std::shared_ptr<NavigationNode> node = createNode(widget);

        if (mOrigin == nullptr)
            mOrigin = mCurrentNode = node;

        mWidgetsToNodes[widget] = node;
    }

    void WindowNavigator::addEditBox(MyGUI::EditBox* editBox, const std::function<void()> onTextAccept)
    {
        std::shared_ptr<NavigationNode> node = createNode(editBox);

        node->onClick([editBox, onTextAccept](auto sender) {
            MWBase::Environment::get().getWindowManager()->startVirtualKeyboard(editBox, onTextAccept);
        });

        if (mOrigin == nullptr)
            mOrigin = mCurrentNode = node;

        mWidgetsToNodes[editBox] = node;
    }

    void WindowNavigator::addWidgetSet(std::vector<MyGUI::Widget*> widgets, bool isVertical)
    {
        std::vector<std::shared_ptr<NavigationNode>> subNodes;
        for (auto widget : widgets)
            subNodes.push_back(createNode(widget));

        std::shared_ptr<NavigationNodeSet> set = std::make_shared<NavigationNodeSet>(subNodes, isVertical);

        if (mOrigin == nullptr)
            mOrigin = mCurrentNode = set;

        for (auto widget : widgets)
            mWidgetsToNodes[widget] = set;
    }

    void WindowNavigator::onClick(MyGUI::Widget* widget, std::function<void(MyGUI::Widget*)> listener)
    {
        mWidgetsToNodes[widget]->onClick(listener);
    }

    void WindowNavigator::onHover(MyGUI::Widget* widget, std::function<void(MyGUI::Widget*)> listener)
    {
        mWidgetsToNodes[widget]->onHover(listener);
    }


    void WindowNavigator::addLeftRightConnection(MyGUI::Widget* left, MyGUI::Widget* right)
    {
        auto leftNode = mWidgetsToNodes[left];
        auto rightNode = mWidgetsToNodes[right];
        leftNode->mRight = rightNode;
        rightNode->mLeft = leftNode;
    }

    void WindowNavigator::addUpDownConnection(MyGUI::Widget* higher, MyGUI::Widget* lower)
    {
        auto higherNode = mWidgetsToNodes[higher];
        auto lowerNode = mWidgetsToNodes[lower];
        higherNode->mDown = lowerNode;
        lowerNode->mUp = higherNode;
    }
    
    bool WindowNavigator::processInput(MWInput::MenuAction action)
    {
        std::shared_ptr<NavigationNode> nextNode;

        if (action == MWInput::MenuAction::MA_A)
        {
            selectCurrentNode();
            return true;
        }
        else if (action == MWInput::MenuAction::MA_DPadUp)
            nextNode = mCurrentNode->up();
        else if (action == MWInput::MenuAction::MA_DPadDown)
            nextNode = mCurrentNode->down();
        else if (action == MWInput::MenuAction::MA_DPadLeft)
            nextNode = mCurrentNode->left();
        else if (action == MWInput::MenuAction::MA_DPadRight)
            nextNode = mCurrentNode->right();
        else
            return false;

        if (nextNode != nullptr)
            mCurrentNode = nextNode;

        mCurrentNode->fireHover();

        return true;
    }

    void WindowNavigator::addNode(std::shared_ptr<NavigationNode> node)
    {
        if (mOrigin == nullptr)
            mOrigin = mCurrentNode = node;

        for (auto widget : node->getAllWidgets())
            mWidgetsToNodes[widget] = node;
    }

    std::shared_ptr<NavigationNode> WindowNavigator::createNode(MyGUI::Widget* widget)
    {
        if (dynamic_cast<MWGui::ItemView*>(widget) != nullptr)
            return std::make_shared<ItemViewNavigationNode>(dynamic_cast<MWGui::ItemView*>(widget));
        else if (dynamic_cast<MyGUI::ListBox*>(widget) != nullptr)
            return std::make_shared<ListBoxNavigationNode<MyGUI::ListBox>>(dynamic_cast<MyGUI::ListBox*>(widget));
        else if (dynamic_cast<SpellView*>(widget) != nullptr)
            return std::make_shared<SpellViewNavigationNode>(dynamic_cast<SpellView*>(widget));
        else
            return std::make_shared<SimpleNavigationNode>(widget);
    }
}
