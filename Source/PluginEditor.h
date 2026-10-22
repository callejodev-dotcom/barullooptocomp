#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
    // A familia Barullo: o azul mariño da noite de Vigo, a crema da letra de Escoita e a luz laranxa,
    // nun aparello de estudio (paneis en relevo, pantallas de cristal, visores dourados).
    namespace col
    {
        const juce::Colour noite    { 0xff0c111f };
        const juce::Colour crema    { 0xfffff7c1 };
        const juce::Colour crema2   { 0xffbdb792 };
        const juce::Colour crema3   { 0xff857f67 };
        const juce::Colour laranxa  { 0xffe08b16 };
        const juce::Colour ouro     { 0xffedba23 };
        const juce::Colour verde    { 0xff9dc6a0 };
        const juce::Colour azul     { 0xff8fb0e8 };
        const juce::Colour panel1   { 0xff1c2540 }, panel2 { 0xff111830 };
        const juce::Colour tecla1   { 0xff27314f }, tecla2 { 0xff1a2239 };
        const juce::Colour visor    { 0xff070b16 };
        const juce::Colour cristal1 { 0xff142046 }, cristal2 { 0xff070c1c };
        const juce::Colour lina     { 0xff2a3350 };
    }

    juce::Font body (float size);    // Inter
    juce::Font semi (float size);    // Montserrat SemiBold
    juce::Font cond (float size);    // Montserrat Bold
    juce::Font caps (float size, float tracking = 0.18f);
    juce::String upper (const juce::String&);
    void drawCaps (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, float tracking = 0.18f);
    void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour);
    void paintPanel (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintWell (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintGlass (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintKey (juce::Graphics&, juce::Rectangle<float>, bool over, bool down);
    void paintLed (juce::Graphics&, juce::Point<float>, bool on, juce::Colour = col::laranxa);
    void groupTitle (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour = col::laranxa);

    struct Look : juce::LookAndFeel_V4
    {
        Look();
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        juce::Font getPopupMenuFont() override { return body (15.0f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    };

    // mando co anel de LED e o seu valor nun visor debaixo
    struct Knob : juce::Component
    {
        Knob();
        void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (14)); }
        void paint (juce::Graphics&) override;
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    // o mando grande, negro, coa saia de aluminio e o anel de luces ámbar (Redución e Ganancia)
    struct MandoNegro : juce::Slider
    {
        MandoNegro();
        void paint (juce::Graphics&) override;
    };

    // o VU de agulla: a redución ou a saída, coa inercia dun de verdade (300 ms)
    class Vu : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Vu (BOProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void tick (double dt);
        static float posicion (double db);   // 0..1 na escala (−20 … +3)
    private:
        BOProcessor& proc;
        double pos = 0, vel = 0;
    };

    // a célula: o panel que se acende co que comprime e a fotorresistencia
    class Celula : public juce::Component
    {
    public:
        explicit Celula (BOProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void tick();
    private:
        BOProcessor& proc;
        float luz = 0, red = 0;
    };

    class MainView : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int W = 900, H = 450;
        explicit MainView (BOProcessor&);
        ~MainView() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void timerCallback() override;
        void mouseDown (const juce::MouseEvent&) override;   // (global) cada clic, un paso de desfacer
        void mostrarMais (bool);
        void refreshAll();

    private:
        void presetsMenu();
        void showPresetsMenu();
        void refreshState();

        BOProcessor& proc;
        Look look;
        juce::TooltipWindow tooltips { this, 500 };
        juce::TextButton presetBtn, prevBtn, nextBtn, presetsBtn { "Presets" }, abBtn[2], undoBtn, redoBtn;
        juce::TextButton modoBtn[2], vuBtn[2], celulaBtn[3], maisBtn, newVersion;
        std::unique_ptr<juce::ParameterAttachment> modoAtt, celulaAtt, bypassAtt;
        MandoNegro reducion, ganancia;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reducionAtt, gananciaAtt;
        Knob agudos, corte, mestura;   // «Máis»
        Vu vu;
        Celula celula;
        bool mais = false, acendido = true;
        juce::Rectangle<int> luzArea;
        juce::SharedResourcePointer<escoita::Actualizacions> versions;
    };
}

class BOEditor : public juce::AudioProcessorEditor
{
public:
    explicit BOEditor (BOProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::noite); }
    ui::MainView& view() { return main; }
private:
    BOProcessor& proc;
    ui::MainView main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BOEditor)
};
