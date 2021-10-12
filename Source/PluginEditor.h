#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <functional>

struct SwitchButton :
    public juce::Component
{
    SwitchButton() :
        name(""),
        onClick(nullptr),
        getState(nullptr),
        state(false)
    {
		setBufferedToImage(true);
	}

    juce::String name;
    std::function<void()> onClick;
    std::function<bool()> getState;
    bool state;
private:

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(2);
        g.setColour(juce::Colour(0x44ffffff));
        if (isMouseOver())
            g.fillRoundedRectangle(bounds, 2);
        if(isMouseButtonDown())
            g.fillRoundedRectangle(bounds, 2);
        g.setColour(juce::Colours::limegreen);
        g.drawRoundedRectangle(bounds, 2, 2);
        g.drawFittedText(name, bounds.toNearestInt(), juce::Justification::centredTop, 1);
        g.drawFittedText(state ? "On" : "Off",
            bounds.toNearestInt(),
            juce::Justification::centredBottom,
            1
        );
    }
    void mouseEnter(const juce::MouseEvent& evt) override { repaint(); }
    void mouseExit(const juce::MouseEvent& evt) override { repaint(); }
    void mouseDown(const juce::MouseEvent& evt) override { repaint(); }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return repaint();
        onClick();
        repaint();
    }
};

struct TextBox :
	public juce::Component,
	public juce::Timer
{
	TextBox(juce::String&& _name,
		std::function<bool(const juce::String& txt)> _onUpdate,
		std::function<juce::String()> onDefaultText,
		juce::String&& _unit = "") :
		onUpdate(_onUpdate),
		onDefault(onDefaultText),
		txt(onDefaultText()),
		txtDefault(txt),
		unit(_unit),
		pos(txt.length()),
		showTick(false)
	{
		setName(_name);
		setWantsKeyboardFocus(true);
		setBufferedToImage(true);
	}
protected:
	std::function<bool(const juce::String& txt)> onUpdate;
	std::function<juce::String()> onDefault;
	juce::String txt, txtDefault, unit;
	int pos;
	bool showTick;

	void mouseUp(const juce::MouseEvent& evt) override {
		if (evt.mouseWasDraggedSinceMouseDown()) return;
		txtDefault = onDefault();
		txt = txtDefault;
		pos = txt.length();
		startTimer(static_cast<int>(1000.f / 1.5f));
		grabKeyboardFocus();
	}

	void paint(juce::Graphics& g) override {
		const auto bounds = getLocalBounds().toFloat().reduced(2);
		g.setColour(juce::Colours::limegreen);
		g.drawRoundedRectangle(bounds, 2, 2);
		if (showTick) {
			const auto txt2Paint = getName() + ": " + txt.substring(0, pos) + "|" + txt.substring(pos) + unit;
			g.drawFittedText(txt2Paint, getLocalBounds(), juce::Justification::centred, 1);
		}
		else
			g.drawFittedText(getName() + ": " + txt + unit, getLocalBounds(), juce::Justification::centred, 1);

	}
	void resized() override {
		const auto width = static_cast<float>(getWidth());
		const auto height = static_cast<float>(getHeight());
	}
	void timerCallback() override {
		if (!hasKeyboardFocus(true)) return;
		showTick = !showTick;
		repaint();
	}

	bool keyPressed(const juce::KeyPress& key) override {
		const auto code = key.getKeyCode();
		if (code == juce::KeyPress::escapeKey) {
			backToDefault();
			repaintWithTick();
		}
		else if (code == juce::KeyPress::backspaceKey) {
			if (txt.length() > 0) {
				txt = txt.substring(0, pos - 1) + txt.substring(pos);
				--pos;
				repaintWithTick();
			}
		}
		else if (code == juce::KeyPress::deleteKey) {
			if (txt.length() > 0) {
				txt = txt.substring(0, pos) + txt.substring(pos + 1);
				repaintWithTick();
			}
		}
		else if (code == juce::KeyPress::leftKey) {
			if (pos > 0) --pos;
			repaintWithTick();
		}
		else if (code == juce::KeyPress::rightKey) {
			if (pos < txt.length()) ++pos;
			repaintWithTick();
		}
		else if (code == juce::KeyPress::returnKey) {
			bool successful = onUpdate(txt);
			if (successful) {
				txtDefault = txt;
				pos = txt.length();
			}
			else
				backToDefault();
			repaintWithTick();
		}
		else {
			// character is text
			txt = txt.substring(0, pos) + key.getTextCharacter() + txt.substring(pos);
			++pos;
			repaintWithTick();
		}
		return true;
	}
	void repaintWithTick() {
		showTick = true;
		repaint();
	}
	void backToDefault() {
		txt = txtDefault;
		pos = txt.length();
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextBox)
};

struct Knob :
	public juce::Component
{
	Knob(OversamplingTestAudioProcessor& p, param::ID pID) :
		rap(*p.apvts.getParameter(param::getID(pID))),
		attach(rap, [this](float value) { repaint(); }, nullptr)
	{
		attach.sendInitialUpdate();
		setBufferedToImage(true);
	}
protected:
	juce::RangedAudioParameter& rap;
	juce::ParameterAttachment attach;
	float dragStartValue;

	void paint(juce::Graphics& g) override {
		const auto width = static_cast<float>(getWidth());
		const auto height = static_cast<float>(getHeight());
		const auto piQuart = 3.14f / 4.f;
		const auto value = rap.getValue();
		juce::PathStrokeType strokeType(2.f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
		const juce::Point<float> centre(width * .5f, height * .5f);
		const auto radius = std::min(centre.x, centre.y) - 2.f;
		const auto startAngle = -piQuart * 3.f;
		const auto endAngle = piQuart * 3.f;
		const auto angleRange = endAngle - startAngle;
		const auto valueAngle = startAngle + angleRange * value;

		g.setColour(juce::Colours::limegreen);
		juce::Path pathNorm;
		pathNorm.addCentredArc(centre.x, centre.y, radius, radius,
			0.f, startAngle, endAngle,
			true
		);
		const auto innerRad = radius - 2.f;
		pathNorm.addCentredArc(centre.x, centre.y, innerRad, innerRad,
			0.f, startAngle, endAngle,
			true
		);
		g.strokePath(pathNorm, strokeType);

		const auto valueLine = juce::Line<float>::fromStartAndAngle(centre, radius + 1.f, valueAngle);
		const auto tickBGThiccness = 2.f * 2.f;
		g.setColour(juce::Colours::limegreen);
		g.drawLine(valueLine, tickBGThiccness);
		g.setColour(juce::Colours::limegreen);
		g.drawLine(valueLine.withShortenedStart(radius - 2.f * 3.f), 2.f);
		g.drawFittedText(rap.getName(13), getLocalBounds(), juce::Justification::centredTop, 1);
		g.drawFittedText(rap.getCurrentValueAsText(), getLocalBounds(), juce::Justification::centredBottom, 1);
	}

	void mouseDown(const juce::MouseEvent& evt) override {
		dragStartValue = rap.getValue();
		attach.beginGesture();
	}
	void mouseDrag(const juce::MouseEvent& evt) override {
		const auto height = static_cast<float>(getHeight());
		const auto distY = static_cast<float>(evt.getDistanceFromDragStartY());
		const auto distRatio = distY / height;
		const auto shift = evt.mods.isShiftDown();
		const auto speed = shift ? .1f : .4f;
		const auto newValue = juce::jlimit(0.f, 1.f, dragStartValue - distRatio * speed);
		const auto denormValue = rap.convertFrom0to1(newValue);
		attach.setValueAsPartOfGesture(denormValue);
	}
	void mouseUp(const juce::MouseEvent& evt) override {
		bool mouseClicked = !evt.mouseWasDraggedSinceMouseDown();
		if (mouseClicked) {
			bool revertToDefault = evt.mods.isCtrlDown();
			float value;
			if (revertToDefault)
				value = rap.getDefaultValue();
			else {
				float piQuart = 3.14f / 4.f;
				juce::Point<int> centre(getWidth() / 2, getHeight() / 2);
				const auto angle = centre.getAngleToPoint(evt.getPosition());
				const auto startAngle = piQuart * -3.f;
				const auto endAngle = piQuart * 3.f;
				const auto angleRange = endAngle - startAngle;
				value = juce::jlimit(0.f, 1.f, (angle - startAngle) / angleRange);
			}
			const auto denormValue = rap.convertFrom0to1(value);
			attach.setValueAsPartOfGesture(denormValue);
		}
		attach.endGesture();
	}
};


class OversamplingTestAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OversamplingTestAudioProcessorEditor (OversamplingTestAudioProcessor&);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    OversamplingTestAudioProcessor& audioProcessor;
    SwitchButton oversamplingEnabledButton;

	Knob gain, vibratoFreq, vibratoDepth, wavefolderDrive, saturatorDrive;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OversamplingTestAudioProcessorEditor)
};
