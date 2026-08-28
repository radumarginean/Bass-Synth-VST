#pragma once

#include <JuceHeader.h>

namespace bassforge::gui
{
/**
    A self-attaching control for one APVTS parameter.

    Chooses its widget from the parameter type: choice -> combo box,
    bool -> toggle, everything else -> rotary knob. A caption label sits
    underneath.
*/
class ParamControl : public juce::Component
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    ParamControl (APVTS& state, const juce::String& paramId, const juce::String& caption)
    {
        auto* param = state.getParameter (paramId);
        jassert (param != nullptr);

        label.setText (caption, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);

        if (dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr)
        {
            combo = std::make_unique<juce::ComboBox>();
            const auto choices = dynamic_cast<juce::AudioParameterChoice*> (param)->choices;
            combo->addItemList (choices, 1);
            combo->setJustificationType (juce::Justification::centred);
            addAndMakeVisible (*combo);
            comboAtt = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, *combo);
            kind = Kind::combo;
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            toggle = std::make_unique<juce::ToggleButton>();
            toggle->setButtonText ({});
            addAndMakeVisible (*toggle);
            toggleAtt = std::make_unique<APVTS::ButtonAttachment> (state, paramId, *toggle);
            kind = Kind::toggle;
        }
        else
        {
            slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                     juce::Slider::TextBoxBelow);
            slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
            addAndMakeVisible (*slider);
            sliderAtt = std::make_unique<APVTS::SliderAttachment> (state, paramId, *slider);
            kind = Kind::knob;
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto labelArea = r.removeFromBottom (14);
        label.setBounds (labelArea);

        switch (kind)
        {
            case Kind::knob:   slider->setBounds (r); break;
            case Kind::combo:  combo->setBounds (r.reduced (2, r.getHeight() / 2 - 12).withHeight (24)); break;
            case Kind::toggle: toggle->setBounds (r.withSizeKeepingCentre (24, 24)); break;
        }
    }

private:
    enum class Kind { knob, combo, toggle };
    Kind kind = Kind::knob;

    juce::Label label;
    std::unique_ptr<juce::Slider>      slider;
    std::unique_ptr<juce::ComboBox>    combo;
    std::unique_ptr<juce::ToggleButton> toggle;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   sliderAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   toggleAtt;
};
} // namespace bassforge::gui
