#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using escoita::U;

namespace
{
    constexpr int ANACO = 512;   // o sobremostrado traballa en anacos de ata 512 mostras
}

BOProcessor::BOProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloOptoComp", createLayout())
{
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pReducion = p ("reducion"); pGanancia = p ("ganancia"); pModo = p ("modo"); pAgudos = p ("agudos"); pCorte = p ("corteDetector");
    pMestura = p ("mestura"); pCelula = p ("celula"); pBypass = p ("bypass");
    nome = U (opto::PRESETS[0].nome);
}

juce::AudioProcessorValueTreeState::ParameterLayout BOProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const auto& p0 = opto::PRESETS[0];
    auto real = [] (const char* id, const String& nomeP, NormalisableRange<float> r, float def, const char* unidade, int decimais = 1)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nomeP, r, def, AudioParameterFloatAttributes().withLabel (unidade)
                   .withStringFromValueFunction ([decimais] (float v, int) { return String (v, decimais); }));
    };
    // o texto pode volver do DAW noutra codificación («Clásica»): compárase só o que é ASCII
    auto escolla = [] (const char* id, const String& nomeP, StringArray ops, int def)
    {
        auto ascii = [] (const String& s) { String r; for (auto c : s) if (c < 128) r << String::charToString (c); return r; };
        return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nomeP, ops, def,
                   AudioParameterChoiceAttributes().withValueFromStringFunction ([ops, ascii] (const String& s)
                   {
                       for (int k = 0; k < ops.size(); ++k) if (s == ops[k]) return (float) k;
                       for (int k = 0; k < ops.size(); ++k) if (ascii (s) == ascii (ops[k])) return (float) k;
                       return 0.0f;
                   }));
    };
    auto centro = [] (float mn, float mx, float c) { NormalisableRange<float> r (mn, mx); r.setSkewForCentre (c); return r; };
    l.add (real ("reducion", U ("Redución"), { 0.0f, 100.0f }, (float) p0.reducion, "", 1));
    l.add (real ("ganancia", "Ganancia", { 0.0f, 100.0f }, (float) p0.ganancia, "", 1));
    l.add (escolla ("modo", "Modo", { "Comprimir", "Limitar" }, p0.modo));
    l.add (real ("agudos", "Agudos no detector", { 0.0f, 100.0f }, (float) p0.agudos * 100.0f, "%"));
    l.add (real ("corteDetector", "Corte do detector", centro (20.0f, 300.0f, 80.0f), (float) p0.corte, "Hz", 0));
    l.add (real ("mestura", "Mestura", { 0.0f, 100.0f }, (float) p0.mestura * 100.0f, "%"));
    l.add (escolla ("celula", U ("Célula"), { U ("Rápida"), U ("Clásica"), "Lenta" }, p0.celula));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bypass", 1 }, "Bypass", StringArray { "Off", "On" }, 0));
    return l;
}

// ================================================================ presets
int BOProcessor::getNumPrograms() { return opto::NUM_PRESETS; }
const juce::String BOProcessor::getProgramName (int i) { return U (opto::PRESETS[juce::jlimit (0, opto::NUM_PRESETS - 1, i)].nome); }

static std::vector<std::pair<const char*, float>> valoresDe (const opto::Preset& p)
{
    return { { "reducion", (float) p.reducion }, { "ganancia", (float) p.ganancia }, { "modo", (float) p.modo }, { "agudos", (float) p.agudos * 100.0f },
             { "corteDetector", (float) p.corte }, { "mestura", (float) p.mestura * 100.0f }, { "celula", (float) p.celula } };
}

void BOProcessor::cargarPreset (int i)
{
    i = juce::jlimit (0, opto::NUM_PRESETS - 1, i);
    const auto antes = std::make_pair (preset, nome);
    for (auto [id, v] : valoresDe (opto::PRESETS[i]))
        if (auto* prm = apvts.getParameter (id))
        {
            const float n = prm->convertTo0to1 (v);
            if (std::abs (prm->getValue() - n) < 1.0e-6f) continue;
            prm->beginChangeGesture(); prm->setValueNotifyingHost (n); prm->endChangeGesture();
        }
    const auto despois = std::make_pair (i, U (opto::PRESETS[i].nome));
    estado.cambio (U ("preset"), [this, despois] { preset = despois.first; nome = despois.second; },
                                 [this, antes] { preset = antes.first; nome = antes.second; });
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

bool BOProcessor::presetEditado() const
{
    const int i = juce::jlimit (0, opto::NUM_PRESETS - 1, preset);
    if (nome != U (opto::PRESETS[i].nome)) return false;
    for (auto [id, v] : valoresDe (opto::PRESETS[i]))
        if (auto* prm = apvts.getParameter (id))
            if (std::abs (prm->getValue() - prm->convertTo0to1 (v)) > 0.002f) return true;
    return false;
}

escoita::Estado::Opcions BOProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "bopreset";
    o.excluir = { "bypass" };
    o.extra = [this] { juce::ValueTree t ("Preset"); t.setProperty ("numero", preset, nullptr); t.setProperty ("nome", nome, nullptr); return t; };
    o.aplicarExtra = [this] (const juce::ValueTree& t) { preset = juce::jlimit (0, opto::NUM_PRESETS - 1, (int) t.getProperty ("numero", 0)); nome = t.getProperty ("nome", nome).toString(); };
    return o;
}

opto::Axustes BOProcessor::axustes() const
{
    opto::Axustes a;
    a.reducion = pReducion->load() / 100.0;
    a.ganancia = pGanancia->load() / 100.0;
    a.modo = (int) std::lround (pModo->load());
    a.agudos = pAgudos->load() / 100.0;
    a.corteDetector = pCorte->load();
    a.mestura = pMestura->load() / 100.0;
    a.celula = (int) std::lround (pCelula->load());
    return a;
}

// ================================================================ audio
bool BOProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void BOProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    // a válvula, a dobre frecuencia (os seus harmónicos non volven como ruído)
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os->initProcessing ((size_t) ANACO);
    optico.preparar (sr * 2.0);
    optico.axustar (axustes(), true);
    latencia = (int) std::lround (os->getLatencyInSamples());
    setLatencySamples (latencia);
    secoL.assign ((size_t) latencia + 1, 0.0f); secoR.assign ((size_t) latencia + 1, 0.0f); secoW = 0;
    const int n = std::max (samplesPerBlock, ANACO);
    secoOutL.assign ((size_t) n, 0.0f); secoOutR.assign ((size_t) n, 0.0f); monoR.assign ((size_t) n, 0.0f);
    teito.preparar (sr, 0.080, 0.891);
    bypassS = pBypass->load() > 0.5f ? 1.0 : 0.0;
}

void BOProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    float* L = buffer.getWritePointer (0);
    float* R = outs > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr && ins < 2) std::copy (L, L + n, R);   // mono → estéreo
    if (n > (int) secoOutL.size()) { secoOutL.resize ((size_t) n); secoOutR.resize ((size_t) n); monoR.resize ((size_t) n); }
    const bool mono = R == nullptr;
    if (mono) { std::copy (L, L + n, monoR.begin()); R = monoR.data(); }

    optico.axustar (axustes());

    // o seco (para o bypass), atrasado o que tarda o sobremostrado
    float pe = 0;
    const int N = (int) secoL.size();
    for (int i = 0; i < n; ++i)
    {
        pe = std::max ({ pe, std::abs (L[i]), std::abs (R[i]) });
        secoL[(size_t) secoW] = L[i]; secoR[(size_t) secoW] = R[i];
        const int ri = (secoW + 1) % N;
        secoOutL[(size_t) i] = secoL[(size_t) ri]; secoOutR[(size_t) i] = secoR[(size_t) ri];
        secoW = (secoW + 1) % N;
    }

    double grMin = 0;
    for (int pos = 0; pos < n; pos += ANACO)
    {
        const int k = std::min (ANACO, n - pos);
        float* ch[2] = { L + pos, R + pos };
        juce::dsp::AudioBlock<float> bloque (ch, 2, (size_t) k);
        auto alto = os->processSamplesUp (bloque);
        float* a = alto.getChannelPointer (0);
        float* b = alto.getChannelPointer (1);
        for (size_t i = 0; i < alto.getNumSamples(); ++i)
        {
            double l = a[i], r = b[i];
            optico.procesar (l, r);
            a[i] = (float) l; b[i] = (float) r;
            grMin = std::min (grMin, optico.reducionDb());
        }
        os->processSamplesDown (bloque);
    }

    const double bypassT = pBypass->load() > 0.5f ? 1.0 : 0.0;
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    double suma = 0;
    for (int i = 0; i < n; ++i)
    {
        bypassS += (bypassT - bypassS) * sm;
        if (std::abs (bypassS - bypassT) < 1e-6) bypassS = bypassT;
        double l = L[i], r = R[i];
        teito.procesar (l, r);
        l = l * (1.0 - bypassS) + secoOutL[(size_t) i] * bypassS;
        r = r * (1.0 - bypassS) + secoOutR[(size_t) i] * bypassS;
        L[i] = (float) l; R[i] = (float) r;
        suma += 0.5 * (std::abs (l) + std::abs (r));
    }
    if (mono) for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int ch = 2; ch < outs; ++ch) buffer.clear (ch, 0, n);
    entrada.store (pe);
    reducion.store ((float) grMin);
    brillo.store ((float) (1.0 - std::exp (-optico.brillo())));
    saidaMedia.store ((float) (n > 0 ? suma / n : 0.0));
}

// ================================================================ estado
void BOProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", preset, nullptr);
    st.setProperty ("nomePreset", nome, nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    st.setProperty ("vu", vu, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BOProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    vu = juce::jlimit (0, 1, (int) tree.getProperty ("vu", 0));
    preset = juce::jlimit (0, opto::NUM_PRESETS - 1, (int) tree.getProperty ("preset", 0));
    nome = tree.getProperty ("nomePreset", U (opto::PRESETS[preset].nome)).toString();
    apvts.replaceState (tree);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));   // que o DAW volva ler os valores
}

juce::AudioProcessorEditor* BOProcessor::createEditor() { return new BOEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BOProcessor(); }
#endif
