#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "Optico.h"

// Barullo Opto Comp: un compresor óptico. Dous mandos (Redución e Ganancia) e Comprimir/Limitar,
// coma os de antes; detrás de «Máis», os agudos e o corte do detector, a mestura en paralelo e a célula.
// Os presets de fábrica son os programas.
class BOProcessor : public juce::AudioProcessor
{
public:
    BOProcessor();
    ~BOProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo Opto Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int index) override { cargarPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::UndoManager undo;   // desfacer e refacer (declárase antes ca o apvts)
    juce::AudioProcessorValueTreeState apvts;
    // presets propios (.bopreset en Documentos/Barullo Opto Comp/Presets), A/B e desfacer, do núcleo
    escoita::Estado estado { apvts, &undo, opcionsEstado() };

    void cargarPreset (int i);
    int getPreset() const { return preset; }
    juce::String nomePreset() const { return nome; }
    bool presetEditado() const;

    opto::Axustes axustes() const;

    // para a interface (escríbese desde o fío de audio)
    std::atomic<float> reducion { 0 }, brillo { 0 }, saidaMedia { 0 }, entrada { 0 };
    float uiScale = 1.0f;
    int vu = 0;   // o que amosa o VU: 0 a redución, 1 a saída (vai co estado)

private:
    escoita::Estado::Opcions opcionsEstado();
    int preset = 0;
    juce::String nome;

    opto::Optico optico;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    escoita::Teito teito;
    double sr = 48000, bypassS = 0;
    int latencia = 0;
    std::vector<float> secoL, secoR, secoOutL, secoOutR, monoR;
    int secoW = 0;
    std::atomic<float> *pReducion = nullptr, *pGanancia = nullptr, *pModo = nullptr, *pAgudos = nullptr, *pCorte = nullptr,
                       *pMestura = nullptr, *pCelula = nullptr, *pBypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BOProcessor)
};
