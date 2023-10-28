#include "textinput.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include "ustring.hpp"

#include "controllegend.hpp"

namespace MWGui
{

    TextInputDialog::TextInputDialog()
        : WindowModal("openmw_text_input.layout")
    {
        // Centre dialog
        center();

        getWidget(mTextEdit, "TextEdit");
        mTextEdit->eventEditSelectAccept += newDelegate(this, &TextInputDialog::onTextAccepted);

        getWidget(mOkButton, "OKButton");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);
        mOkButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &TextInputDialog::onKeyButtonPressed);

        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);

        mUsesHighlightOffset = true;
    }

    void TextInputDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(
                toUString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", {})));
        else
            okButton->setCaption(
                toUString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", {})));
    }

    void TextInputDialog::setTextLabel(std::string_view label)
    {
        setText("LabelT", label);
    }

    void TextInputDialog::onOpen()
    {
        WindowModal::onOpen();
        // Make sure the edit box has focus unless we're using the joystick
        if (MWBase::Environment::get().getInputManager()->joystickLastUsed())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);
        else 
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);

        widgetHighlight(mTextEdit);
    }

    // widget controls

    void TextInputDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if (mTextEdit->getCaption().empty())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage37}");
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
        }
        else
            eventDone(this);
    }

    void TextInputDialog::onTextAccepted(MyGUI::EditBox* _sender)
    {
        onOkClicked(_sender);

        // To do not spam onTextAccepted() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    std::string TextInputDialog::getTextInput() const
    {
        return mTextEdit->getCaption();
    }

    void TextInputDialog::setTextInput(const std::string& text)
    {
        mTextEdit->setCaption(text);
    }

    void TextInputDialog::onFrame(float dt)
    {
        // we never want to focus the name edit field when using the controller
        if (MWBase::Environment::get().getInputManager()->joystickLastUsed() &&
                !MWBase::Environment::get().getWindowManager()->virtualKeyboardVisible())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);
    }

    void TextInputDialog::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        // Gamepad controls only.
        if (character != 1)
            return;

        MWBase::Environment::get().getWindowManager()->consumeKeyPress(true);
        MWInput::MenuAction action = static_cast<MWInput::MenuAction>(key.getValue());
        if (action == MWInput::MA_A) // open virtual keyboard
            MWBase::Environment::get().getWindowManager()->startVirtualKeyboard(mTextEdit, [this] { onOkClicked(mOkButton); });
        else if (action == MWInput::MA_X) // accept input
            onOkClicked(mOkButton);
    }
    
    ControlSet TextInputDialog::getControlLegendContents()
    {
        return {
            {
                MenuControl{MWInput::MenuAction::MA_A, "Select"}
            },
            {
                MenuControl{MWInput::MenuAction::MA_X, "Accept"},
            }
        };
    }

}
