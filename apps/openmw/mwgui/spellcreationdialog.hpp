#ifndef MWGUI_SPELLCREATION_H
#define MWGUI_SPELLCREATION_H

#include <memory>

#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>

#include "referenceinterface.hpp"
#include "windowbase.hpp"

namespace Gui
{
    class MWList;
}

namespace MWGui
{

    class SelectSkillDialog;
    class SelectAttributeDialog;

    class EditEffectDialog : public WindowModal
    {
    public:
        EditEffectDialog();

        void onOpen() override;
        bool exit() override;

        void setConstantEffect(bool constant);

        void setSkill(ESM::RefId skill);
        void setAttribute(ESM::RefId attribute);

        void newEffect(const ESM::MagicEffect* effect);
        void editEffect(ESM::ENAMstruct effect);
        typedef MyGUI::delegates::MultiDelegate<ESM::ENAMstruct> EventHandle_Effect;

        EventHandle_Effect eventEffectAdded;
        EventHandle_Effect eventEffectModified;
        EventHandle_Effect eventEffectRemoved;

    protected:
        MyGUI::IntCoord highlightOffset() override { return MyGUI::IntCoord(MyGUI::IntPoint(-4, -4), MyGUI::IntSize(8, 8)); };

        ControlSet getControlLegendContents() override;

        MyGUI::Button* mCancelButton;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mDeleteButton;

        MyGUI::Button* mRangeButton;

        MyGUI::Widget* mDurationBox;
        MyGUI::Widget* mMagnitudeBox;
        MyGUI::Widget* mAreaBox;

        MyGUI::TextBox* mMagnitudeMinValue;
        MyGUI::TextBox* mMagnitudeMaxValue;
        MyGUI::TextBox* mDurationValue;
        MyGUI::TextBox* mAreaValue;

        MyGUI::ScrollBar* mMagnitudeMinSlider;
        MyGUI::ScrollBar* mMagnitudeMaxSlider;
        MyGUI::ScrollBar* mDurationSlider;
        MyGUI::ScrollBar* mAreaSlider;

        MyGUI::TextBox* mAreaText;

        MyGUI::ImageBox* mEffectImage;
        MyGUI::TextBox* mEffectName;

        bool mEditing;

    protected:
        void onRangeButtonClicked(MyGUI::Widget* sender);
        void onDeleteButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);

        void onMagnitudeMinChanged(MyGUI::ScrollBar* sender, size_t pos);
        void onMagnitudeMaxChanged(MyGUI::ScrollBar* sender, size_t pos);
        void onDurationChanged(MyGUI::ScrollBar* sender, size_t pos);
        void onAreaChanged(MyGUI::ScrollBar* sender, size_t pos);
        void setMagicEffect(const ESM::MagicEffect* effect);

        void updateBoxes();

    protected:

        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

        ESM::ENAMstruct mEffect;
        ESM::ENAMstruct mOldEffect;

        const ESM::MagicEffect* mMagicEffect;

        bool mConstantEffect;
    private:
        void changeHighlight(int amount);
        void changeSlider(int amount);

        // returns true if the targeted widget was visible
        bool widgetHighlight(int index);

        MyGUI::Widget* getHighlightedWidget();

        // 0: Range
        // 1,2: Magnitude
        // 3: Duration
        // 4: Area
        unsigned int mHighlight;
    };

    class EffectEditorBase
    {
    public:
        enum Type
        {
            Spellmaking,
            Enchanting
        };

        EffectEditorBase(Type type);
        virtual ~EffectEditorBase();

        void setConstantEffect(bool constant);

    protected:
        std::map<int, short> mButtonMapping; // maps button ID to effect ID

        Gui::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;

        EditEffectDialog mAddEffectDialog;
        std::unique_ptr<SelectAttributeDialog> mSelectAttributeDialog;
        std::unique_ptr<SelectSkillDialog> mSelectSkillDialog;

        int mSelectedEffect;
        short mSelectedKnownEffectId;

        bool mConstantEffect;

        std::vector<ESM::ENAMstruct> mEffects;

        void onEffectAdded(ESM::ENAMstruct effect);
        void onEffectModified(ESM::ENAMstruct effect);
        void onEffectRemoved(ESM::ENAMstruct effect);

        void onAvailableEffectClicked(MyGUI::Widget* sender);

        void onAttributeOrSkillCancel();
        void onSelectAttribute();
        void onSelectSkill();

        void onEditEffect(MyGUI::Widget* sender);

        void updateEffectsView();

        void startEditing();
        void setWidgets(Gui::MWList* availableEffectsList, MyGUI::ScrollView* usedEffectsView);

        virtual void notifyEffectsChanged() {}

    private:
        Type mType;
    };

    class SpellCreationDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        SpellCreationDialog();

        void onOpen() override;
        void clear() override { resetReference(); }

        void onFrame(float dt) override;

        void setPtr(const MWWorld::Ptr& actor) override;

        std::string_view getWindowIdForLua() const override { return "SpellCreationDialog"; }

    protected:
        void onReferenceUnavailable() override;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onBuyButtonClicked(MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox* sender);

        void notifyEffectsChanged() override;

        ControlSet getControlLegendContents() override;

        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

        MyGUI::IntCoord highlightOffset() override;

        void widgetHighlight(unsigned int index);
        // divided into three sets:
        // 0: the spell name
        // 1 to n: the available spell effects
        // n+1 to m: the spell effects added to the current spell
        unsigned int mHighlight;


        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

        ESM::Spell mSpell;
    };

}

#endif
