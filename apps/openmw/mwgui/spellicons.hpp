#ifndef MWGUI_SPELLICONS_H
#define MWGUI_SPELLICONS_H

#include <string>
#include <vector>

#include "../mwmechanics/magiceffects.hpp"

namespace MyGUI
{
    class Widget;
    class ImageBox;
}
namespace ESM
{
    struct ENAMstruct;
    struct EffectList;
}

namespace MWGui
{

    // information about a single magic effect source as required for display in the tooltip
    struct MagicEffectInfo
    {
        MagicEffectInfo()
            : mMagnitude(0)
            , mRemainingTime(0.f)
            , mTotalTime(0.f)
            , mPermanent(false)
        {
        }
        std::string mSource; // display name for effect source (e.g. potion name)
        MWMechanics::EffectKey mKey;
        int mMagnitude;
        float mRemainingTime;
        float mTotalTime;
        bool mPermanent; // the effect is permanent
    };

    class SpellIcons
    {
    public:
        void updateWidgets(MyGUI::Widget* parent, bool adjustSize);

    private:
        std::map<int, MyGUI::ImageBox*> mWidgetMap;
    };

}

#endif
