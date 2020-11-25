#include "controllermanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlmappings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "actionmanager.hpp"
#include "bindingsmanager.hpp"
#include "mousemanager.hpp"

namespace MWInput
{
    MyGUI::KeyCode menuActionToKeyCode(MWInput::MenuAction action);
    float makeAxisRatio(int pressureVal);

    ControllerManager::ControllerManager(BindingsManager* bindingsManager,
            ActionManager* actionManager,
            MouseManager* mouseManager,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile)
        : mBindingsManager(bindingsManager)
        , mActionManager(actionManager)
        , mMouseManager(mouseManager)
        , mJoystickEnabled (Settings::Manager::getBool("enable controller", "Input"))
        , mGamepadCursorSpeed(Settings::Manager::getFloat("gamepad cursor speed", "Input"))
        , mSneakToggleShortcutTimer(0.f)
        , mGamepadZoom(0)
        , mGamepadGuiCursorEnabled(true)
        , mGuiCursorEnabled(true)
        , mJoystickLastUsed(false)
        , mSneakGamepadShortcut(false)
        , mGamepadPreviewMode(false)
        , mRTriggerPressureVal(0.f)
        , mLTriggerPressureVal(0.f)
    {
        if (!controllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(controllerBindingsFile.c_str());
        }

        if (!userControllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(userControllerBindingsFile.c_str());
        }

        // Open all presently connected sticks
        int numSticks = SDL_NumJoysticks();
        for (int i = 0; i < numSticks; i++)
        {
            if (SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                static const int fakeDeviceID = 1;
                controllerAdded(fakeDeviceID, evt);
                Log(Debug::Info) << "Detected game controller: " << SDL_GameControllerNameForIndex(i);
            }
            else
            {
                Log(Debug::Info) << "Detected unusable controller: " << SDL_JoystickNameForIndex(i);
            }
        }

        float deadZoneRadius = Settings::Manager::getFloat("joystick dead zone", "Input");
        deadZoneRadius = std::clamp(deadZoneRadius, 0.f, 0.5f);
        mBindingsManager->setJoystickDeadZone(deadZoneRadius);
    }

    void ControllerManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "enable controller")
                mJoystickEnabled = Settings::Manager::getBool("enable controller", "Input");
        }
    }

    void ControllerManager::setJoystickLastUsed(bool enabled)
    {
        mJoystickLastUsed = enabled;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->toggleSelectionHighlights(enabled); // Toggle off when pc controls are used.
    }

    bool ControllerManager::update(float dt)
    {
        if (mGuiCursorEnabled && !(mJoystickLastUsed && !mGamepadGuiCursorEnabled))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight) * 2.0f - 1.0f;
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward) * 2.0f - 1.0f;
            float zAxis = mBindingsManager->getActionValue(A_LookUpDown) * 2.0f - 1.0f;

            xAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));
            yAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            float uiScale = MWBase::Environment::get().getWindowManager()->getScalingFactor();
            float xMove = xAxis * dt * 1500.0f / uiScale * mGamepadCursorSpeed;
            float yMove = yAxis * dt * 1500.0f / uiScale * mGamepadCursorSpeed;

            float mouseWheelMove = -zAxis * dt * 1500.0f;
            if (xMove != 0 || yMove != 0 || mouseWheelMove != 0)
            {
                mMouseManager->injectMouseMove(xMove, yMove, mouseWheelMove);
                mMouseManager->warpMouse();
                MWBase::Environment::get().getWindowManager()->setCursorActive(true);
            }
        }

        // Disable movement in Gui mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running)
        {
            return false;
        }

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        bool triedToMove = false;

        // Configure player movement according to controller input. Actual movement will
        // be done in the physics system.
        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            if (xAxis != 0.5)
            {
                triedToMove = true;
                player.setLeftRight((xAxis - 0.5f) * 2);
            }

            if (yAxis != 0.5)
            {
                triedToMove = true;
                player.setAutoMove (false);
                player.setForwardBackward((0.5f - yAxis) * 2);
            }

            if (triedToMove)
            {
                setJoystickLastUsed(true);
                MWBase::Environment::get().getInputManager()->resetIdleTime();
            }

            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (!isToggleSneak)
            {
                if (mJoystickLastUsed)
                {
                    if (mBindingsManager->actionIsActive(A_Sneak))
                    {
                        if (mSneakToggleShortcutTimer) // New Sneak Button Press
                        {
                            if (mSneakToggleShortcutTimer <= 0.3f)
                            {
                                mSneakGamepadShortcut = true;
                                mActionManager->toggleSneaking();
                            }
                            else
                                mSneakGamepadShortcut = false;
                        }

                        if (!mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        mSneakToggleShortcutTimer = 0.f;
                    }
                    else
                    {
                        if (!mSneakGamepadShortcut && mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        if (mSneakToggleShortcutTimer <= 0.3f)
                            mSneakToggleShortcutTimer += dt;
                    }
                }
                else
                    player.setSneak(mBindingsManager->actionIsActive(A_Sneak));
            }
        }

        return triedToMove;
    }

    void ControllerManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        if (!mJoystickEnabled || mBindingsManager->isDetectingBindingState())
            return;

        MWBase::Environment::get().getLuaManager()->inputEvent(
            {MWBase::LuaManager::InputEvent::ControllerPressed, arg.button});

        setJoystickLastUsed(true);
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (gamepadToGuiControl(arg))
                return;

            if (mGamepadGuiCursorEnabled)
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonPress(SDL_BUTTON_LEFT);
                    if (MyGUI::InputManager::getInstance().getMouseFocusWidget())
                    {
                        MyGUI::Button* b = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                        if (b && b->getEnabled())
                            MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
                    }

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(kc, 0));

        if (!MWBase::Environment::get().getInputManager()->controlsDisabled())
            mBindingsManager->controllerButtonPressed(deviceID, arg);
    }

    void ControllerManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->controllerButtonReleased(deviceID, arg);
            return;
        }

        if (mJoystickEnabled)
        {
            MWBase::Environment::get().getLuaManager()->inputEvent(
                {MWBase::LuaManager::InputEvent::ControllerReleased, arg.button});
        }

        if (!mJoystickEnabled || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        setJoystickLastUsed(true);
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (mGamepadGuiCursorEnabled)
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonRelease(SDL_BUTTON_LEFT);
                    if (mBindingsManager->isDetectingBindingState()) // If the player just triggered binding, don't let button release bind.
                        return;

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));

        mBindingsManager->controllerButtonReleased(deviceID, arg);
    }

    void ControllerManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg)
    {
        if (!mJoystickEnabled || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        setJoystickLastUsed(true);
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            gamepadToGuiControl(arg);
        }
        else if (MWBase::Environment::get().getWorld()->isPreviewModeEnabled() &&
                (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT || arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT))
        {
            // Preview Mode Gamepad Zooming; do not propagate to mBindingsManager
            return;
        }
        mBindingsManager->controllerAxisMoved(deviceID, arg);
    }

    void ControllerManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg)
    {
        mBindingsManager->controllerAdded(deviceID, arg);
    }

    void ControllerManager::controllerRemoved(const SDL_ControllerDeviceEvent &arg)
    {
        mBindingsManager->controllerRemoved(arg);
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerButtonEvent &arg)
    {
        // Presumption of GUI mode will be removed in the future.
        MWInput::MenuAction key = MWInput::MenuAction::MA_None;
        switch (arg.button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                key = MWInput::MenuAction::MA_DPadUp;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                key = MWInput::MenuAction::MA_DPadRight;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                key = MWInput::MenuAction::MA_DPadDown;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                key = MWInput::MenuAction::MA_DPadLeft;
                break;
            case SDL_CONTROLLER_BUTTON_A:
                // If we are using the joystick as a GUI mouse, A must be handled via mouse.
                if (mGamepadGuiCursorEnabled)
                    return false;
                key = MWInput::MenuAction::MA_A;
                break;
            case SDL_CONTROLLER_BUTTON_B:
                if (MyGUI::InputManager::getInstance().isModalAny())
                    MWBase::Environment::get().getWindowManager()->exitCurrentModal();
                else
                    MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
                return true;
            case SDL_CONTROLLER_BUTTON_X:
                key = MWInput::MenuAction::MA_X;
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                key = MWInput::MenuAction::MA_Y;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                key = MWInput::MenuAction::MA_Black;
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                key = MWInput::MenuAction::MA_White;
                break;
            case SDL_CONTROLLER_BUTTON_START:
                key = MWInput::MenuAction::MA_Start;
                break;
            case SDL_CONTROLLER_BUTTON_BACK:
                key = MWInput::MenuAction::MA_Select;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                mGamepadGuiCursorEnabled = !mGamepadGuiCursorEnabled;
                MWBase::Environment::get().getWindowManager()->setCursorActive(mGamepadGuiCursorEnabled);
                return true;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                key = MWInput::MenuAction::MA_JSRightClick;
                break;
            default:
                return false;
        }

        MWBase::Environment::get().getWindowManager()->injectKeyPress(menuActionToKeyCode(key), 1, false); // Uses text '1' to signal a gamepad keypress.
        return true;
    }

    float ControllerManager::getAxisRatio(int action)
    {
        MWInput::MenuAction axis = static_cast<MWInput::MenuAction>(action);

        switch (axis)
        {
            case MA_RTrigger:
                return mRTriggerPressureVal;
            case MA_LTrigger:
                return mLTriggerPressureVal;
            default:
                break;
        }

        return 0.f;
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerAxisEvent &arg)
    {
        switch (arg.axis)
        {
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                if (!mRTriggerPressureVal)
                {
                    // Only inject a single keypress when trigger is first pressed. Prevents studdering scroll actions.
                    mRTriggerPressureVal = makeAxisRatio(arg.value);
                    MWBase::Environment::get().getWindowManager()->injectKeyPress(menuActionToKeyCode(MWInput::MenuAction::MA_RTrigger), 1, true);
                }

                mRTriggerPressureVal = makeAxisRatio(arg.value); // Update axis regardless.
                if (!mRTriggerPressureVal)
                    MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                if (!mLTriggerPressureVal)
                {
                    // Only inject a single keypress when trigger is first pressed. Prevents studdering scroll actions.
                    mLTriggerPressureVal = makeAxisRatio(arg.value);
                    MWBase::Environment::get().getWindowManager()->injectKeyPress(menuActionToKeyCode(MWInput::MenuAction::MA_LTrigger), 1, true);
                }

                mLTriggerPressureVal = makeAxisRatio(arg.value); // Update axis regardless.
                if (!mLTriggerPressureVal)
                    MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
                break;
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_LEFTY:
            case SDL_CONTROLLER_AXIS_RIGHTX:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                // If we are using the joystick as a GUI mouse, process mouse movement elsewhere.
                if (mGamepadGuiCursorEnabled)
                    return false;
                break;
            default:
                return false;
        }

        return true;
    }

    float ControllerManager::getAxisValue(SDL_GameControllerAxis axis) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        constexpr int AXIS_MAX_ABSOLUTE_VALUE = 32768;
        if (cntrl)
            return SDL_GameControllerGetAxis(cntrl, axis) / static_cast<float>(AXIS_MAX_ABSOLUTE_VALUE);
        else
            return 0;
    }

    bool ControllerManager::isButtonPressed(SDL_GameControllerButton button) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl)
            return SDL_GameControllerGetButton(cntrl, button) > 0;
        else
            return false;
    }


    MyGUI::KeyCode menuActionToKeyCode(MWInput::MenuAction action)
    {
        return static_cast<MyGUI::KeyCode::Enum>(static_cast<int>(action));
    }

    float makeAxisRatio(int pressureVal)
    {
        return pressureVal / 32767.f;
    }
}
