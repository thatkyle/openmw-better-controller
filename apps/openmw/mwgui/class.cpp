#include "class.hpp"

#include <MyGUI_ImageBox.h>
#include <MyGUI_ListBox.h>
#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/debug/debuglog.hpp>

#include "tooltips.hpp"
#include "controllegend.hpp"

namespace
{

    bool sortClasses(const std::pair<std::string, std::string>& left, const std::pair<std::string, std::string>& right)
    {
        return left.second.compare(right.second) < 0;
    }

}

namespace MWGui
{

    /* GenerateClassResultDialog */

    GenerateClassResultDialog::GenerateClassResultDialog()
      : ButtonMenu("openmw_chargen_generate_class_result.layout")
    {
        setText("ReflectT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sMessageQuestionAnswer1", ""));

        getWidget(mClassImage, "ClassImage");
        getWidget(mClassName, "ClassName");

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->setCaptionWithReplacing("#{sMessageQuestionAnswer3}");
        registerButtonPress(backButton, MyGUI::newDelegate(this, &GenerateClassResultDialog::onBackClicked));

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->setCaptionWithReplacing("#{sMessageQuestionAnswer2}");
        registerButtonPress(okButton, MyGUI::newDelegate(this, &GenerateClassResultDialog::onOkClicked));

        registerButtons({ backButton, okButton }, false);

        center();
    }

    std::string GenerateClassResultDialog::getClassId() const
    {
        return mClassName->getCaption();
    }

    void GenerateClassResultDialog::setClassId(const std::string &classId)
    {
        mCurrentClassId = classId;

        setClassImage(mClassImage, mCurrentClassId);

        mClassName->setCaption(MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(mCurrentClassId)->mName);

        center();
    }

    // widget controls

    void GenerateClassResultDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        eventDone(this);
    }

    void GenerateClassResultDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    /* PickClassDialog */

    PickClassDialog::PickClassDialog()
      : WindowModal("openmw_chargen_class.layout")
    {
        // Centre dialog
        center();

        getWidget(mSpecializationName, "SpecializationName");

        getWidget(mFavoriteAttribute[0], "FavoriteAttribute0");
        getWidget(mFavoriteAttribute[1], "FavoriteAttribute1");

        for(int i = 0; i < 5; i++)
        {
            char theIndex = '0'+i;
            getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
            getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
        }

        getWidget(mClassList, "ClassList");
        mClassList->setScrollVisible(true);
        mClassList->eventListSelectAccept += MyGUI::newDelegate(this, &PickClassDialog::onAccept);
        mClassList->eventListChangePosition += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
        mClassList->eventKeyButtonPressed += MyGUI::newDelegate(this, &PickClassDialog::onKeyButtonPressed);

        getWidget(mClassImage, "ClassImage");

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

        updateClasses();
        updateStats();
    }

    void PickClassDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    void PickClassDialog::onOpen()
    {
        WindowModal::onOpen ();
        updateClasses();
        updateStats();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mClassList);

        // Show the current class by default
        MWWorld::Ptr player = MWMechanics::getPlayer();

        const std::string &classId =
            player.get<ESM::NPC>()->mBase->mClass;

        if (!classId.empty())
            setClassId(classId);

        mWindowNavigator = WindowNavigator(mClassList);
    }

    void PickClassDialog::setClassId(const std::string &classId)
    {
        mCurrentClassId = classId;
        mClassList->setIndexSelected(MyGUI::ITEM_NONE);
        size_t count = mClassList->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (Misc::StringUtils::ciEqual(*mClassList->getItemDataAt<std::string>(i), classId))
            {
                mClassList->setIndexSelected(i);
                break;
            }
        }

        updateStats();
    }

    void PickClassDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;


        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        if (mWindowNavigator.processInput(action))
            return;

        switch (action)
        {
        case MWInput::MA_B:
            PickClassDialog::onBackClicked(sender);
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    // widget controls

    void PickClassDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if(mClassList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void PickClassDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    void PickClassDialog::onAccept(MyGUI::ListBox* _sender, size_t _index)
    {
        onSelectClass(_sender, _index);
        if(mClassList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void PickClassDialog::onSelectClass(MyGUI::ListBox* _sender, size_t _index)
    {
        if (_index == MyGUI::ITEM_NONE)
            return;

        const std::string *classId = mClassList->getItemDataAt<std::string>(_index);
        if (Misc::StringUtils::ciEqual(mCurrentClassId, *classId))
            return;

        mCurrentClassId = *classId;
        updateStats();
    }

    // update widget content

    void PickClassDialog::updateClasses()
    {
        mClassList->removeAllItems();

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

        std::vector<std::pair<std::string, std::string> > items; // class id, class name
        for (const ESM::Class& classInfo : store.get<ESM::Class>())
        {
            bool playable = (classInfo.mData.mIsPlayable != 0);
            if (!playable) // Only display playable classes
                continue;

            if (store.get<ESM::Class>().isDynamic(classInfo.mId))
                continue; // custom-made class not relevant for this dialog

            items.emplace_back(classInfo.mId, classInfo.mName);
        }
        std::sort(items.begin(), items.end(), sortClasses);

        int index = 0;
        for (auto& itemPair : items)
        {
            const std::string &id = itemPair.first;
            mClassList->addItem(itemPair.second, id);
            if (mCurrentClassId.empty())
            {
                mCurrentClassId = id;
                mClassList->setIndexSelected(index);
            }
            else if (Misc::StringUtils::ciEqual(id, mCurrentClassId))
            {
                mClassList->setIndexSelected(index);
            }
            ++index;
        }
    }

    void PickClassDialog::updateStats()
    {
        if (mCurrentClassId.empty())
            return;
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Class *klass = store.get<ESM::Class>().search(mCurrentClassId);
        if (!klass)
            return;

        ESM::Class::Specialization specialization = static_cast<ESM::Class::Specialization>(klass->mData.mSpecialization);

        static const char *specIds[3] = {
            "sSpecializationCombat",
            "sSpecializationMagic",
            "sSpecializationStealth"
        };
        std::string specName = MWBase::Environment::get().getWindowManager()->getGameSettingString(specIds[specialization], specIds[specialization]);
        mSpecializationName->setCaption(specName);
        ToolTips::createSpecializationToolTip(mSpecializationName, specName, specialization);

        mFavoriteAttribute[0]->setAttributeId(klass->mData.mAttribute[0]);
        mFavoriteAttribute[1]->setAttributeId(klass->mData.mAttribute[1]);
        ToolTips::createAttributeToolTip(mFavoriteAttribute[0], mFavoriteAttribute[0]->getAttributeId());
        ToolTips::createAttributeToolTip(mFavoriteAttribute[1], mFavoriteAttribute[1]->getAttributeId());

        for (int i = 0; i < 5; ++i)
        {
            mMinorSkill[i]->setSkillNumber(klass->mData.mSkills[i][0]);
            mMajorSkill[i]->setSkillNumber(klass->mData.mSkills[i][1]);
            ToolTips::createSkillToolTip(mMinorSkill[i], klass->mData.mSkills[i][0]);
            ToolTips::createSkillToolTip(mMajorSkill[i], klass->mData.mSkills[i][1]);
        }

        setClassImage(mClassImage, mCurrentClassId);
    }

    /* InfoBoxDialog */

    void InfoBoxDialog::fitToText(MyGUI::TextBox* widget)
    {
        MyGUI::IntCoord inner = widget->getTextRegion();
        MyGUI::IntCoord outer = widget->getCoord();
        MyGUI::IntSize size = widget->getTextSize();
        size.width += outer.width - inner.width;
        size.height += outer.height - inner.height;
        widget->setSize(size);
    }

    void InfoBoxDialog::layoutVertically(MyGUI::Widget* widget, int margin)
    {
        size_t count = widget->getChildCount();
        int pos = 0;
        pos += margin;
        int width = 0;
        for (unsigned i = 0; i < count; ++i)
        {
            MyGUI::Widget* child = widget->getChildAt(i);
            if (!child->getVisible() || child == mGamepadHighlight)
                continue;

            child->setPosition(child->getLeft(), pos);
            width = std::max(width, child->getWidth());
            pos += child->getHeight() + margin;
        }
        width += margin*2;
        widget->setSize(width, pos);
    }

    InfoBoxDialog::InfoBoxDialog()
        : ButtonMenu("openmw_infobox.layout")
    {
        getWidget(mTextBox, "TextBox");
        getWidget(mText, "Text");
        mText->getSubWidgetText()->setWordWrap(true);
        getWidget(mButtonBar, "ButtonBar");

        center();
    }

    void InfoBoxDialog::setText(const std::string &str)
    {
        mText->setCaption(str);
        mTextBox->setVisible(!str.empty());
        fitToText(mText);
    }

    std::string InfoBoxDialog::getText() const
    {
        return mText->getCaption();
    }

    void InfoBoxDialog::setButtons(ButtonList &buttons)
    {
        for (MyGUI::Button* button : this->mButtons)
        {
            MyGUI::Gui::getInstance().destroyWidget(button);
        }
        this->mButtons.clear();

        // TODO: The buttons should be generated from a template in the layout file, ie. cloning an existing widget
        MyGUI::Button* button;
        MyGUI::IntCoord coord = MyGUI::IntCoord(0, 0, mButtonBar->getWidth(), 10);
        for (const std::string &text : buttons)
        {
            button = mButtonBar->createWidget<MyGUI::Button>("MW_Button", coord, MyGUI::Align::Top | MyGUI::Align::HCenter, "");
            button->getSubWidgetText()->setWordWrap(true);
            button->setCaption(text);
            fitToText(button);

            registerButtonPress(button, MyGUI::newDelegate(this, &InfoBoxDialog::onButtonClicked));

            coord.top += button->getHeight();
            this->mButtons.push_back(button);
        }

        registerButtons(this->mButtons, true);
    }

    void InfoBoxDialog::onOpen()
    {
        ButtonMenu::onOpen();
        // Fix layout
        layoutVertically(mTextBox, 4);
        layoutVertically(mButtonBar, 6);
        layoutVertically(mMainWidget, 4 + 6);

        center();

        highlightSelectedButton();
    }

    void InfoBoxDialog::onButtonClicked(MyGUI::Widget* _sender)
    {
        int i = 0;
        for (MyGUI::Button* button : mButtons)
        {
            if (button == _sender)
            {
                eventButtonSelected(i);
                return;
            }
            ++i;
        }
    }

    /* ClassChoiceDialog */

    ClassChoiceDialog::ClassChoiceDialog()
        : InfoBoxDialog()
    {
        setText("");
        ButtonList buttons;
        buttons.push_back(MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu1", ""));
        buttons.push_back(MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu2", ""));
        buttons.push_back(MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu3", ""));
        buttons.push_back(MWBase::Environment::get().getWindowManager()->getGameSettingString("sBack", ""));
        setButtons(buttons);
    }

    /* CreateClassDialog */

    CreateClassDialog::CreateClassDialog()
      : WindowModal("openmw_chargen_create_class.layout")
      , mSpecDialog(nullptr)
      , mAttribDialog(nullptr)
      , mSkillDialog(nullptr)
      , mDescDialog(nullptr)
      , mAffectedAttribute(nullptr)
      , mAffectedSkill(nullptr)
    {
        // Centre dialog
        center();

        setText("SpecializationT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sChooseClassMenu1", "Specialization"));
        getWidget(mSpecializationName, "SpecializationName");
        mSpecializationName->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationClicked);

        setText("FavoriteAttributesT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
        getWidget(mFavoriteAttribute0, "FavoriteAttribute0");
        getWidget(mFavoriteAttribute1, "FavoriteAttribute1");
        mFavoriteAttribute0->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);
        mFavoriteAttribute1->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);

        setText("MajorSkillT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sSkillClassMajor", ""));
        setText("MinorSkillT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sSkillClassMinor", ""));
        for(int i = 0; i < 5; i++)
        {
            char theIndex = '0'+i;
            getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
            getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
            mSkills.push_back(mMajorSkill[i]);
            mSkills.push_back(mMinorSkill[i]);
        }

        for (Widgets::MWSkillPtr& skill : mSkills)
        {
            skill->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onSkillClicked);
        }

        setText("LabelT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sName", ""));
        getWidget(mEditName, "EditName");

        MyGUI::Button* descriptionButton;
        getWidget(descriptionButton, "DescriptionButton");
        descriptionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionClicked);

        getWidget(mBackButton, "BackButton");
        mBackButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onBackClicked);

        getWidget(mOkButton, "OKButton");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onOkClicked);
        mOkButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &CreateClassDialog::onKeyButtonPressed);

        // Make sure the edit box has focus
        if (!MWBase::Environment::get().getInputManager()->joystickLastUsed())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mEditName);
        else
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);

        // Set default skills, attributes

        mFavoriteAttribute0->setAttributeId(ESM::Attribute::Strength);
        mFavoriteAttribute1->setAttributeId(ESM::Attribute::Agility);

        mMajorSkill[0]->setSkillId(ESM::Skill::Block);
        mMajorSkill[1]->setSkillId(ESM::Skill::Armorer);
        mMajorSkill[2]->setSkillId(ESM::Skill::MediumArmor);
        mMajorSkill[3]->setSkillId(ESM::Skill::HeavyArmor);
        mMajorSkill[4]->setSkillId(ESM::Skill::BluntWeapon);

        mMinorSkill[0]->setSkillId(ESM::Skill::LongBlade);
        mMinorSkill[1]->setSkillId(ESM::Skill::Axe);
        mMinorSkill[2]->setSkillId(ESM::Skill::Spear);
        mMinorSkill[3]->setSkillId(ESM::Skill::Athletics);
        mMinorSkill[4]->setSkillId(ESM::Skill::Enchant);

        // set up window navigator
        mWindowNavigator.addWidget(mEditName);

        mWindowNavigator.addWidget(mSpecializationName);
        mWindowNavigator.addWidget(mFavoriteAttribute0);
        mWindowNavigator.addWidget(mFavoriteAttribute1);

        mWindowNavigator.addWidget(mMajorSkill[0]);
        mWindowNavigator.addWidget(mMajorSkill[1]);
        mWindowNavigator.addWidget(mMajorSkill[2]);
        mWindowNavigator.addWidget(mMajorSkill[3]);
        mWindowNavigator.addWidget(mMajorSkill[4]);

        mWindowNavigator.addWidget(mMinorSkill[0]);
        mWindowNavigator.addWidget(mMinorSkill[1]);
        mWindowNavigator.addWidget(mMinorSkill[2]);
        mWindowNavigator.addWidget(mMinorSkill[3]);
        mWindowNavigator.addWidget(mMinorSkill[4]);

        mWindowNavigator.addUpDownConnection(mEditName, mMinorSkill[0]);
        mWindowNavigator.addUpDownConnection(mEditName, mMajorSkill[0]);
        mWindowNavigator.addUpDownConnection(mEditName, mSpecializationName);

        mWindowNavigator.addUpDownConnection(mSpecializationName, mFavoriteAttribute0);
        mWindowNavigator.addUpDownConnection(mFavoriteAttribute0, mFavoriteAttribute1);

        for (auto i = 0; i < 4; i++)
        {
            mWindowNavigator.addUpDownConnection(mMajorSkill[i], mMajorSkill[i + 1]);
            mWindowNavigator.addUpDownConnection(mMinorSkill[i], mMinorSkill[i + 1]);
        }

        mWindowNavigator.addLeftRightConnection(mFavoriteAttribute1, mMajorSkill[4]);
        mWindowNavigator.addLeftRightConnection(mFavoriteAttribute1, mMajorSkill[3]);
        mWindowNavigator.addLeftRightConnection(mFavoriteAttribute0, mMajorSkill[2]);
        mWindowNavigator.addLeftRightConnection(mSpecializationName, mMajorSkill[1]);
        mWindowNavigator.addLeftRightConnection(mSpecializationName, mMajorSkill[0]);

        mWindowNavigator.addLeftRightConnection(mMajorSkill[0], mMinorSkill[0]);
        mWindowNavigator.addLeftRightConnection(mMajorSkill[1], mMinorSkill[1]);
        mWindowNavigator.addLeftRightConnection(mMajorSkill[2], mMinorSkill[2]);
        mWindowNavigator.addLeftRightConnection(mMajorSkill[3], mMinorSkill[3]);
        mWindowNavigator.addLeftRightConnection(mMajorSkill[4], mMinorSkill[4]);

        setSpecialization(0);
        update();

        mUsesHighlightOffset = true;

        widgetHighlight(mWindowNavigator.getSelectedWidget());
    }

    CreateClassDialog::~CreateClassDialog()
    {
        delete mSpecDialog;
        delete mAttribDialog;
        delete mSkillDialog;
        delete mDescDialog;
    }

    void CreateClassDialog::onFrame(float dt)
    {
        // we never want to focus the name edit field when using the controller
        if (MWBase::Environment::get().getInputManager()->joystickLastUsed() &&
                !MWBase::Environment::get().getWindowManager()->virtualKeyboardVisible())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);
    }

    void CreateClassDialog::update()
    {
        for (int i = 0; i < 5; ++i)
        {
            ToolTips::createSkillToolTip(mMajorSkill[i], mMajorSkill[i]->getSkillId());
            ToolTips::createSkillToolTip(mMinorSkill[i], mMinorSkill[i]->getSkillId());
        }

        ToolTips::createAttributeToolTip(mFavoriteAttribute0, mFavoriteAttribute0->getAttributeId());
        ToolTips::createAttributeToolTip(mFavoriteAttribute1, mFavoriteAttribute1->getAttributeId());

        widgetHighlight(mWindowNavigator.getSelectedWidget());
    }

    std::string CreateClassDialog::getName() const
    {
        return mEditName->getCaption();
    }

    std::string CreateClassDialog::getDescription() const
    {
        return mDescription;
    }

    ESM::Class::Specialization CreateClassDialog::getSpecializationId() const
    {
        return mSpecializationId;
    }

    std::vector<int> CreateClassDialog::getFavoriteAttributes() const
    {
        std::vector<int> v;
        v.push_back(mFavoriteAttribute0->getAttributeId());
        v.push_back(mFavoriteAttribute1->getAttributeId());
        return v;
    }

    std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMajorSkills() const
    {
        std::vector<ESM::Skill::SkillEnum> v;
        v.reserve(5);
        for(int i = 0; i < 5; i++)
        {
            v.push_back(mMajorSkill[i]->getSkillId());
        }
        return v;
    }

    std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMinorSkills() const
    {
        std::vector<ESM::Skill::SkillEnum> v;
        v.reserve(5);
        for(int i=0; i < 5; i++)
        {
            v.push_back(mMinorSkill[i]->getSkillId());
        }
        return v;
    }

    void CreateClassDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    // widget controls

    void CreateClassDialog::onDialogCancel()
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mSpecDialog);
        mSpecDialog = nullptr;

        MWBase::Environment::get().getWindowManager()->removeDialog(mAttribDialog);
        mAttribDialog = nullptr;

        MWBase::Environment::get().getWindowManager()->removeDialog(mSkillDialog);
        mSkillDialog = nullptr;

        MWBase::Environment::get().getWindowManager()->removeDialog(mDescDialog);
        mDescDialog = nullptr;
    }

    void CreateClassDialog::onSpecializationClicked(MyGUI::Widget* _sender)
    {
        delete mSpecDialog;
        mSpecDialog = new SelectSpecializationDialog();
        mSpecDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mSpecDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationSelected);
        mSpecDialog->setVisible(true);
    }

    void CreateClassDialog::onSpecializationSelected()
    {
        mSpecializationId = mSpecDialog->getSpecializationId();
        setSpecialization(mSpecializationId);

        MWBase::Environment::get().getWindowManager()->removeDialog(mSpecDialog);
        mSpecDialog = nullptr;
    }

    void CreateClassDialog::setSpecialization(int id)
    {
        mSpecializationId = (ESM::Class::Specialization) id;
        static const char *specIds[3] = {
            "sSpecializationCombat",
            "sSpecializationMagic",
            "sSpecializationStealth"
        };
        std::string specName = MWBase::Environment::get().getWindowManager()->getGameSettingString(specIds[mSpecializationId], specIds[mSpecializationId]);
        mSpecializationName->setCaption(specName);
        ToolTips::createSpecializationToolTip(mSpecializationName, specName, mSpecializationId);
    }

    void CreateClassDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
    {
        delete mAttribDialog;
        mAttribDialog = new SelectAttributeDialog();
        mAffectedAttribute = _sender;
        mAttribDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mAttribDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeSelected);
        mAttribDialog->setVisible(true);
    }

    void CreateClassDialog::onAttributeSelected()
    {
        ESM::Attribute::AttributeID id = mAttribDialog->getAttributeId();
        if (mAffectedAttribute == mFavoriteAttribute0)
        {
            if (mFavoriteAttribute1->getAttributeId() == id)
                mFavoriteAttribute1->setAttributeId(mFavoriteAttribute0->getAttributeId());
        }
        else if (mAffectedAttribute == mFavoriteAttribute1)
        {
            if (mFavoriteAttribute0->getAttributeId() == id)
                mFavoriteAttribute0->setAttributeId(mFavoriteAttribute1->getAttributeId());
        }
        mAffectedAttribute->setAttributeId(id);
        MWBase::Environment::get().getWindowManager()->removeDialog(mAttribDialog);
        mAttribDialog = nullptr;

        update();
    }

    void CreateClassDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
    {
        delete mSkillDialog;
        mSkillDialog = new SelectSkillDialog();
        mAffectedSkill = _sender;
        mSkillDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSkillSelected);
        mSkillDialog->setVisible(true);
    }

    void CreateClassDialog::onSkillSelected()
    {
        ESM::Skill::SkillEnum id = mSkillDialog->getSkillId();

        // Avoid duplicate skills by swapping any skill field that matches the selected one
        for (Widgets::MWSkillPtr& skill : mSkills)
        {
            if (skill == mAffectedSkill)
                continue;
            if (skill->getSkillId() == id)
            {
                skill->setSkillId(mAffectedSkill->getSkillId());
                break;
            }
        }

        mAffectedSkill->setSkillId(mSkillDialog->getSkillId());
        MWBase::Environment::get().getWindowManager()->removeDialog(mSkillDialog);
        mSkillDialog = nullptr;
        update();
    }

    void CreateClassDialog::onDescriptionClicked(MyGUI::Widget* _sender)
    {
        mDescDialog = new DescriptionDialog();
        mDescDialog->setTextInput(mDescription);
        mDescDialog->eventDone += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionEntered);
        mDescDialog->setVisible(true);
    }

    void CreateClassDialog::onDescriptionEntered(WindowBase* parWindow)
    {
        mDescription = mDescDialog->getTextInput();
        MWBase::Environment::get().getWindowManager()->removeDialog(mDescDialog);
        mDescDialog = nullptr;
    }

    void CreateClassDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if(getName().size() <= 0)
            return;
        eventDone(this);
    }

    void CreateClassDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    void CreateClassDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1 && character != 2)
            return;


        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        if (character == 2)
        {
            if (action == MWInput::MA_Y)
                updateGamepadTooltip(nullptr);
            return;
        }

        if (mWindowNavigator.processInput(action))
        {
            widgetHighlight(mWindowNavigator.getSelectedWidget());
            return;
        }

        switch (action)
        {
        case MWInput::MA_Start:
            mOkButton->eventMouseButtonClick(mOkButton);
            break;
        case MWInput::MA_B:
            mBackButton->eventMouseButtonClick(mBackButton);
            break;
        case MWInput::MA_Y:
            updateGamepadTooltip(mWindowNavigator.getSelectedWidget());
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    ControlSet CreateClassDialog::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"},
                MenuControl{MWInput::MenuAction::MA_Y, "Info"}
            },
            {
                MenuControl{MWInput::MenuAction::MA_Start, "Accept"},
                MenuControl{MWInput::MenuAction::MA_B, "Back"}
            }
        };
    }

    MyGUI::IntCoord CreateClassDialog::highlightOffset()
    { 
        if (dynamic_cast<MyGUI::EditBox*>(mWindowNavigator.getSelectedWidget()) != nullptr)
            return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8));
        else
            return MyGUI::IntCoord(MyGUI::IntPoint(-2, -2), MyGUI::IntSize(-16, 4));
    }


    /* SelectSpecializationDialog */

    SelectSpecializationDialog::SelectSpecializationDialog()
      : WindowModal("openmw_chargen_select_specialization.layout")
    {
        // Centre dialog
        center();

        getWidget(mSpecialization0, "Specialization0");
        getWidget(mSpecialization1, "Specialization1");
        getWidget(mSpecialization2, "Specialization2");
        std::string combat = MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Combat], "");
        std::string magic = MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Magic], "");
        std::string stealth = MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Stealth], "");

        mSpecialization0->setCaption(combat);
        mSpecialization0->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecialization1->setCaption(magic);
        mSpecialization1->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecialization2->setCaption(stealth);
        mSpecialization2->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecializationId = ESM::Class::Combat;

        ToolTips::createSpecializationToolTip(mSpecialization0, combat, ESM::Class::Combat);
        ToolTips::createSpecializationToolTip(mSpecialization1, magic, ESM::Class::Magic);
        ToolTips::createSpecializationToolTip(mSpecialization2, stealth, ESM::Class::Stealth);

        getWidget(mCancelButton, "CancelButton");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onCancelClicked);
        mCancelButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &SelectSpecializationDialog::onKeyButtonPressed);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCancelButton);

        mWindowNavigator.addWidgetSet({ mSpecialization0, mSpecialization1, mSpecialization2 }, true);

        widgetHighlight(mWindowNavigator.getSelectedWidget());
    }

    SelectSpecializationDialog::~SelectSpecializationDialog()
    {
    }

    // widget controls

    void SelectSpecializationDialog::onSpecializationClicked(MyGUI::Widget* _sender)
    {
        if (_sender == mSpecialization0)
            mSpecializationId = ESM::Class::Combat;
        else if (_sender == mSpecialization1)
            mSpecializationId = ESM::Class::Magic;
        else if (_sender == mSpecialization2)
            mSpecializationId = ESM::Class::Stealth;
        else
            return;

        eventItemSelected();
    }

    void SelectSpecializationDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectSpecializationDialog::exit()
    {
        eventCancel();
        return true;
    }

    void SelectSpecializationDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1 && character != 2)
            return;


        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        if (character == 2)
        {
            if (action == MWInput::MA_Y)
                updateGamepadTooltip(nullptr);
            return;
        }

        if (mWindowNavigator.processInput(action))
        {
            widgetHighlight(mWindowNavigator.getSelectedWidget());
            return;
        }

        switch (action)
        {
        case MWInput::MA_B:
            mCancelButton->eventMouseButtonClick(mCancelButton);
            break;
        case MWInput::MA_Y:
            updateGamepadTooltip(mWindowNavigator.getSelectedWidget());
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    /* SelectAttributeDialog */

    SelectAttributeDialog::SelectAttributeDialog()
      : WindowModal("openmw_chargen_select_attribute.layout")
      , mAttributeId(ESM::Attribute::Strength)
    {
        // Centre dialog
        center();

        std::vector<MyGUI::Widget*> attributes;
        for (int i = 0; i < 8; ++i)
        {
            Widgets::MWAttributePtr attribute;
            char theIndex = '0'+i;

            getWidget(attribute,  std::string("Attribute").append(1, theIndex));
            attribute->setAttributeId(ESM::Attribute::sAttributeIds[i]);
            attribute->eventClicked += MyGUI::newDelegate(this, &SelectAttributeDialog::onAttributeClicked);
            ToolTips::createAttributeToolTip(attribute, attribute->getAttributeId());
            attributes.push_back(attribute);
        }

        getWidget(mCancelButton, "CancelButton");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectAttributeDialog::onCancelClicked);
        mCancelButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &SelectAttributeDialog::onKeyButtonPressed);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCancelButton);

        mWindowNavigator.addWidgetSet(attributes, true);

        widgetHighlight(mWindowNavigator.getSelectedWidget());
    }

    SelectAttributeDialog::~SelectAttributeDialog()
    {
    }

    // widget controls

    void SelectAttributeDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
    {
        // TODO: Change MWAttribute to set and get AttributeID enum instead of int
        mAttributeId = static_cast<ESM::Attribute::AttributeID>(_sender->getAttributeId());
        eventItemSelected();
    }

    void SelectAttributeDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectAttributeDialog::exit()
    {
        eventCancel();
        return true;
    }

    void SelectAttributeDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1 && character != 2)
            return;


        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        if (character == 2)
        {
            if (action == MWInput::MA_Y)
                updateGamepadTooltip(nullptr);
            return;
        }

        if (mWindowNavigator.processInput(action))
        {
            widgetHighlight(mWindowNavigator.getSelectedWidget());
            return;
        }

        switch (action)
        {
        case MWInput::MA_B:
            mCancelButton->eventMouseButtonClick(mCancelButton);
            break;
        case MWInput::MA_Y:
            updateGamepadTooltip(mWindowNavigator.getSelectedWidget());
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }


    /* SelectSkillDialog */

    SelectSkillDialog::SelectSkillDialog()
      : WindowModal("openmw_chargen_select_skill.layout")
      , mSkillId(ESM::Skill::Block)
    {
        // Centre dialog
        center();

        for(int i = 0; i < 9; i++)
        {
            char theIndex = '0'+i;
            getWidget(mCombatSkill[i],  std::string("CombatSkill").append(1, theIndex));
            getWidget(mMagicSkill[i],   std::string("MagicSkill").append(1, theIndex));
            getWidget(mStealthSkill[i], std::string("StealthSkill").append(1, theIndex));
        }

        struct {Widgets::MWSkillPtr widget; ESM::Skill::SkillEnum skillId;} mSkills[3][9] = {
            {
                {mCombatSkill[0], ESM::Skill::Block},
                {mCombatSkill[1], ESM::Skill::Armorer},
                {mCombatSkill[2], ESM::Skill::MediumArmor},
                {mCombatSkill[3], ESM::Skill::HeavyArmor},
                {mCombatSkill[4], ESM::Skill::BluntWeapon},
                {mCombatSkill[5], ESM::Skill::LongBlade},
                {mCombatSkill[6], ESM::Skill::Axe},
                {mCombatSkill[7], ESM::Skill::Spear},
                {mCombatSkill[8], ESM::Skill::Athletics}
            },
            {
                {mMagicSkill[0], ESM::Skill::Enchant},
                {mMagicSkill[1], ESM::Skill::Destruction},
                {mMagicSkill[2], ESM::Skill::Alteration},
                {mMagicSkill[3], ESM::Skill::Illusion},
                {mMagicSkill[4], ESM::Skill::Conjuration},
                {mMagicSkill[5], ESM::Skill::Mysticism},
                {mMagicSkill[6], ESM::Skill::Restoration},
                {mMagicSkill[7], ESM::Skill::Alchemy},
                {mMagicSkill[8], ESM::Skill::Unarmored}
            },
            {
                {mStealthSkill[0], ESM::Skill::Security},
                {mStealthSkill[1], ESM::Skill::Sneak},
                {mStealthSkill[2], ESM::Skill::Acrobatics},
                {mStealthSkill[3], ESM::Skill::LightArmor},
                {mStealthSkill[4], ESM::Skill::ShortBlade},
                {mStealthSkill[5] ,ESM::Skill::Marksman},
                {mStealthSkill[6] ,ESM::Skill::Mercantile},
                {mStealthSkill[7] ,ESM::Skill::Speechcraft},
                {mStealthSkill[8] ,ESM::Skill::HandToHand}
            }
        };

        for (int spec = 0; spec < 3; ++spec)
        {
            for (int i = 0; i < 9; ++i)
            {
                mSkills[spec][i].widget->setSkillId(mSkills[spec][i].skillId);
                mSkills[spec][i].widget->eventClicked += MyGUI::newDelegate(this, &SelectSkillDialog::onSkillClicked);
                ToolTips::createSkillToolTip(mSkills[spec][i].widget, mSkills[spec][i].widget->getSkillId());
                mWindowNavigator.addWidget(mSkills[spec][i].widget);
            }
        }

        for (int spec = 0; spec < 3; ++spec)
        {
            for (int i = 0; i < 9; ++i)
            {
                if (spec < 2)
                    mWindowNavigator.addLeftRightConnection(mSkills[spec][i].widget, mSkills[spec + 1][i].widget);
                if (i < 8)
                    mWindowNavigator.addUpDownConnection(mSkills[spec][i].widget, mSkills[spec][i + 1].widget);
            }
        }

        getWidget(mCancelButton, "CancelButton");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSkillDialog::onCancelClicked);
        mCancelButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &SelectSkillDialog::onKeyButtonPressed);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCancelButton);

        widgetHighlight(mWindowNavigator.getSelectedWidget());
    }

    SelectSkillDialog::~SelectSkillDialog()
    {
    }

    // widget controls

    void SelectSkillDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
    {
        mSkillId = _sender->getSkillId();
        eventItemSelected();
    }

    void SelectSkillDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectSkillDialog::exit()
    {
        eventCancel();
        return true;
    }

    void SelectSkillDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1 && character != 2)
            return;


        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());

        if (character == 2)
        {
            if (action == MWInput::MA_Y)
                updateGamepadTooltip(nullptr);
            return;
        }

        if (mWindowNavigator.processInput(action))
        {
            widgetHighlight(mWindowNavigator.getSelectedWidget());
            return;
        }

        switch (action)
        {
        case MWInput::MA_B:
            mCancelButton->eventMouseButtonClick(mCancelButton);
            break;
        case MWInput::MA_Y:
            updateGamepadTooltip(mWindowNavigator.getSelectedWidget());
            break;
        default:
            MWBase::Environment::get().getWindowManager()->consumeKeyPress(false);
            break;
        }
    }

    /* DescriptionDialog */

    DescriptionDialog::DescriptionDialog()
      : WindowModal("openmw_chargen_class_description.layout")
    {
        // Centre dialog
        center();

        getWidget(mTextEdit, "TextEdit");

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DescriptionDialog::onOkClicked);
        okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sInputMenu1", ""));

        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
    }

    DescriptionDialog::~DescriptionDialog()
    {
    }

    // widget controls

    void DescriptionDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        eventDone(this);
    }

    void setClassImage(MyGUI::ImageBox* imageBox, const std::string &classId)
    {
        std::string classImage = std::string("textures\\levelup\\") + classId + ".dds";
        if (!MWBase::Environment::get().getWindowManager()->textureExists(classImage))
        {
            Log(Debug::Warning) << "No class image for " << classId << ", falling back to default";
            classImage = "textures\\levelup\\warrior.dds";
        }
        imageBox->setImageTexture(classImage);
    }

}
