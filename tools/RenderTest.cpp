// Proba sen interface de Barullo Opto Comp: a curva (comprimir e limitar), os tempos da célula e a
// súa memoria, as tres células, o detector (agudos e corte), a mestura, as válvulas, o teito, os
// bloques, o bypass, o estado, o A/B, os presets propios e o desfacer; tamén capturas.
//   BORenderTest [saida.wav]                          proba enteira (devolve o número de fallos)
//   BORenderTest --snapshot saida.png [mais]          a interface
//   BORenderTest --demo preset entrada.wav saida.wav  un preset sobre un son
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>

using escoita::U;
static constexpr double SR = 48000;

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

static juce::AudioBuffer<float> procesar (BOProcessor& p, const juce::AudioBuffer<float>& in, double len, int bloque, bool preparar = true)
{
    if (preparar) { p.setPlayConfigDetails (2, 2, SR, bloque); p.prepareToPlay (SR, bloque); }
    const int total = (int) (len * SR);
    juce::AudioBuffer<float> out (2, total), buf (2, bloque);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += bloque)
    {
        const int n = juce::jmin (bloque, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i < in.getNumSamples()) buf.setSample (c, i, in.getSample (c % in.getNumChannels(), pos + i));
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    return out;
}

static void setParam (BOProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}
// a ganancia a 0 dB (o mando, de 0 a 100)
static constexpr float CERO_DB = 100.0f / 3.0f;

static juce::AudioBuffer<float> seno (double f, double rmsDb, double len, double t0 = 0.0, double t1 = 1e9)
{
    juce::AudioBuffer<float> b (2, (int) (len * SR));
    const double a = std::pow (10.0, rmsDb / 20.0) * std::sqrt (2.0);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / SR;
        const float x = (t >= t0 && t < t1) ? (float) (a * std::sin (2 * opto::PI * f * t)) : 0.0f;
        b.setSample (0, i, x); b.setSample (1, i, x);
    }
    return b;
}
// o son de proba: golpes de ruído e un acorde
static juce::AudioBuffer<float> sonDeProba (double len = 2.0)
{
    juce::AudioBuffer<float> b (2, (int) (len * SR));
    b.clear();
    juce::Random rnd (1984);
    for (double t0 : { 0.0, 0.25, 0.5, 0.875 })
        for (int i = 0; i < (int) (0.12 * SR); ++i)
        {
            const int k = (int) (t0 * SR) + i;
            if (k >= b.getNumSamples()) break;
            const float e = 0.5f * std::exp (-(float) i / (float) (0.025 * SR));
            for (int c = 0; c < 2; ++c) b.setSample (c, k, b.getSample (c, k) + e * (rnd.nextFloat() * 2 - 1));
        }
    for (double f : { 65.41, 261.63, 329.63, 392.0 })
        for (int i = 0; i < (int) (0.8 * SR); ++i)
        {
            const int k = (int) (1.1 * SR) + i;
            if (k >= b.getNumSamples()) break;
            const float x = 0.1f * (float) std::sin (2 * opto::PI * f * i / SR) * std::exp (-(float) i / (float) (0.4 * SR)) * juce::jmin (1.0f, (float) i / 100.0f);
            for (int c = 0; c < 2; ++c) b.setSample (c, k, b.getSample (c, k) + x);
        }
    return b;
}
static double rmsDb (const juce::AudioBuffer<float>& b, double t0, double t1)
{
    double e = 0; int n = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = (int) (t0 * SR); i < juce::jmin (b.getNumSamples(), (int) (t1 * SR)); ++i) { e += (double) b.getSample (c, i) * b.getSample (c, i); ++n; }
    return 10.0 * std::log10 (e / juce::jmax (1, n) + 1e-30);
}
static float pico (const juce::AudioBuffer<float>& b) { return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples())); }
static bool finito (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < 2; ++c) for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
static float diferenza (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < juce::jmin (a.getNumSamples(), b.getNumSamples()); ++i) d = juce::jmax (d, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return d;
}

// a redución (dB) no estado estable, cun seno a un nivel
static double redSen (float red, int modo, double rms, double f = 1000.0, float agudos = 0, float corte = 20)
{
    BOProcessor p;
    setParam (p, "reducion", red); setParam (p, "modo", (float) modo); setParam (p, "ganancia", CERO_DB);
    setParam (p, "agudos", agudos); setParam (p, "corteDetector", corte); setParam (p, "mestura", 100.0f);
    const auto o = procesar (p, seno (f, rms, 3.0), 3.0, 512);
    return rmsDb (o, 2.5, 3.0) - rms;
}

// os tempos da célula: un golpe de seno (dur s) e despois silencio; a redución mostra a mostra (do VU)
struct Tempos { double gr, ataque, metade, noventa; };
static Tempos tempos (int celula, double dur)
{
    BOProcessor p;
    setParam (p, "reducion", 60.0f); setParam (p, "ganancia", CERO_DB); setParam (p, "celula", (float) celula); setParam (p, "mestura", 100.0f);
    p.setPlayConfigDetails (2, 2, SR, 64); p.prepareToPlay (SR, 64);
    const auto son = seno (1000.0, -9.0, dur);
    std::vector<double> gr;
    juce::AudioBuffer<float> buf (2, 64);
    juce::MidiBuffer m;
    const int total = (int) ((dur + 8.0) * SR);
    for (int pos = 0; pos < total; pos += 64)
    {
        buf.clear();
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 64; ++i) if (pos + i < son.getNumSamples()) buf.setSample (c, i, son.getSample (c, pos + i));
        p.processBlock (buf, m);
        gr.push_back (p.reducion.load());
    }
    const double bloque = 64.0 / SR;
    const size_t fin = (size_t) (dur / bloque) - 1;
    const double g = gr[fin];
    Tempos t { g, -1, -1, -1 };
    for (size_t i = 0; i < gr.size(); ++i) if (gr[i] < 0.63 * g) { t.ataque = (double) i * bloque; break; }
    for (size_t i = fin; i < gr.size(); ++i) if (gr[i] > 0.5 * g) { t.metade = (double) (i - fin) * bloque; break; }
    for (size_t i = fin; i < gr.size(); ++i) if (gr[i] > 0.1 * g) { t.noventa = (double) (i - fin) * bloque; break; }
    return t;
}

static void snapshot (const juce::File& png, bool mais)
{
    BOProcessor p;
    p.setPlayConfigDetails (2, 2, SR, 512);
    p.prepareToPlay (SR, 512);
    p.cargarPreset (1);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<BOEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    if (mais) ce->view().mostrarMais (true);
    const auto son = seno (220.0, -10.0, 2.0);
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 90; ++blk)
    {
        buf.clear();
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, son, c, (blk * 512) % (son.getNumSamples() - 512), 512);
        juce::MidiBuffer m; p.processBlock (buf, m);
        ce->view().timerCallback();
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 2) { snapshot (juce::File (juce::String (argv[2])), argc > 3 && juce::String (argv[3]) == "mais"); return 0; }
    if (mode == "--demo" && argc > 4)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (juce::String (argv[3]))));
        if (r == nullptr) { std::printf ("non se pode ler %s\n", argv[3]); return 1; }
        juce::AudioBuffer<float> in ((int) r->numChannels, (int) r->lengthInSamples);
        r->read (&in, 0, (int) r->lengthInSamples, 0, true, true);
        BOProcessor p;
        p.cargarPreset (atoi (argv[2]));
        p.setPlayConfigDetails (2, 2, r->sampleRate, 512); p.prepareToPlay (r->sampleRate, 512);
        const int L = p.getLatencySamples();
        juce::AudioBuffer<float> out (2, in.getNumSamples() + L), buf (2, 512);
        juce::MidiBuffer m;
        for (int pos = 0; pos < out.getNumSamples(); pos += 512)
        {
            const int n = juce::jmin (512, out.getNumSamples() - pos);
            buf.setSize (2, n, false, false, true); buf.clear();
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) if (pos + i < in.getNumSamples()) buf.setSample (c, i, in.getSample (c % in.getNumChannels(), pos + i));
            p.processBlock (buf, m);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        }
        juce::AudioBuffer<float> sen (2, in.getNumSamples());
        for (int c = 0; c < 2; ++c) sen.copyFrom (c, 0, out, c, L, in.getNumSamples());
        writeWav (juce::File (juce::String (argv[4])), sen, r->sampleRate);
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const juce::String& what, const juce::String& extra = {})
    {
        std::printf ("%-52s %s %s\n", what.toRawUTF8(), ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    {   // ao abrir: o primeiro preset, cos parámetros nos seus valores de partida
        BOProcessor p;
        juce::String mal;
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                if (std::abs (r->getValue() - r->getDefaultValue()) > 1e-6f) mal << r->getParameterID() << " ";
        check (mal.isEmpty() && p.getPreset() == 0 && ! p.presetEditado(), "ao abrir: o primeiro preset, nos valores de partida", mal);
    }
    {   // a curva: comprimindo, uns 6 dB a −18 dBFS co mando a 50 e proporción suave; limitando, moito máis dura
        const double c18 = redSen (50.0f, 0, -18.0), c12 = redSen (50.0f, 0, -12.0), c6 = redSen (50.0f, 0, -6.0);
        const double l12 = redSen (50.0f, 1, -12.0), l6 = redSen (50.0f, 1, -6.0);
        const double rc = 6.0 / (6.0 + (c6 - c12)), rl = 6.0 / (6.0 + (l6 - l12));
        check (c18 < -4.5 && c18 > -7.5 && rc > 2.0 && rc < 3.6 && rl > 4.5, U ("curva: comprimir ~3:1 e limitar máis dura"),
               juce::String::formatted ("(%.1f dB a -18; %.1f:1 e %.1f:1)", c18, rc, rl));
        const double baixo = redSen (50.0f, 0, -40.0), cero = redSen (0.0f, 0, -18.0);   // (máis forte, xa saturan as válvulas)
        check (baixo > -0.6 && std::abs (cero) < 0.15, U ("sen comprimir: baixo e coa Redución a 0"),
               juce::String::formatted ("(%.2f e %.2f dB)", baixo, cero));
    }
    {   // os tempos da célula clásica e a memoria (un golpe longo solta máis amodo); as tres en orde
        const auto curto = tempos (1, 0.3), longo = tempos (1, 5.0);
        const bool ok = curto.ataque > 0.002 && curto.ataque < 0.02 && curto.metade > 0.04 && curto.metade < 0.25
                        && curto.noventa > 0.5 && curto.noventa < 3.0 && longo.noventa > curto.noventa * 1.3;
        check (ok, U ("célula clásica: ataque, soltura en dúas fases e memoria"),
               juce::String::formatted ("(%.1f ms; 50 %% en %.0f ms; 90 %% en %.2f s / tras 5 s, %.2f s)", curto.ataque * 1000, curto.metade * 1000, curto.noventa, longo.noventa));
        const auto r = tempos (0, 0.3), l = tempos (2, 0.3);
        check (r.metade < curto.metade && curto.metade < l.metade && r.noventa < curto.noventa && curto.noventa < l.noventa,
               U ("as tres células: rápida, clásica e lenta"), juce::String::formatted ("(%.2f / %.2f / %.2f s)", r.noventa, curto.noventa, l.noventa));
    }
    {   // o detector: os agudos de máis apertan antes un 6 kHz; o corte deixa pasar o baixo
        const double sen = redSen (50.0f, 0, -18.0, 6000.0, 0.0f), con = redSen (50.0f, 0, -18.0, 6000.0, 100.0f);
        const double b20 = redSen (50.0f, 0, -18.0, 50.0, 0.0f, 20.0f), b300 = redSen (50.0f, 0, -18.0, 50.0, 0.0f, 300.0f);
        check (con < sen - 3.0 && b300 > b20 + 3.0, "detector: agudos e corte", juce::String::formatted ("(6 kHz %.1f / %.1f dB; 50 Hz %.1f / %.1f dB)", sen, con, b20, b300));
    }
    {   // a mestura: a 0, o comprimido non se oe (o mesmo nivel ca o seco)
        BOProcessor p;
        setParam (p, "reducion", 80.0f); setParam (p, "ganancia", CERO_DB); setParam (p, "mestura", 0.0f);
        const auto o = procesar (p, seno (1000.0, -10.0, 2.0), 2.0, 512);
        const double d = rmsDb (o, 1.5, 2.0) + 10.0;
        check (std::abs (d) < 0.2, "mestura a 0: o seco", juce::String::formatted ("(%.2f dB)", d));
    }
    {   // as válvulas: con ganancia, un seno colle o segundo harmónico (asimétricas)
        BOProcessor p;
        setParam (p, "reducion", 0.0f); setParam (p, "ganancia", 70.0f);
        const auto o = procesar (p, seno (200.0, -24.0, 1.0), 1.0, 512);
        double re = 0, im = 0, re1 = 0, im1 = 0;
        for (int i = (int) (0.5 * SR); i < (int) SR; ++i)
        {
            const double x = o.getSample (0, i), a2 = 2 * opto::PI * 400.0 * i / SR, a1 = 2 * opto::PI * 200.0 * i / SR;
            re += x * std::cos (a2); im += x * std::sin (a2); re1 += x * std::cos (a1); im1 += x * std::sin (a1);
        }
        const double h2 = 20.0 * std::log10 (std::sqrt (re * re + im * im) / std::sqrt (re1 * re1 + im1 * im1));
        check (h2 > -50.0 && h2 < -10.0, U ("válvulas: o segundo harmónico"), juce::String::formatted ("(%.0f dB)", h2));
    }
    {   // o teito: ganancia ao máximo e un son forte non pasan de −1 dBFS
        BOProcessor p;
        setParam (p, "reducion", 0.0f); setParam (p, "ganancia", 100.0f);
        auto son = sonDeProba (2.0); son.applyGain (1.8f);
        const auto o = procesar (p, son, 2.0, 512);
        check (finito (o) && pico (o) <= 0.8913f, "teito a -1 dBFS", juce::String::formatted ("(pico %.4f)", pico (o)));
    }
    {   // bloques: con bloques de 512 e de 37 sae o mesmo (o sobremostrado de JUCE, en Intel, leva a cero
        // ao final de cada bloque os estados por debaixo de 1e-8: unha milmillonésima)
        auto render = [&] (int bloque) { BOProcessor p; p.cargarPreset (3); return procesar (p, sonDeProba (2.0), 3.0, bloque); };
        const auto x = render (512), y = render (37);
        const float d = diferenza (x, y);
        check (d < 1e-5f && finito (x) && pico (x) > 0.05f, "mesmo son con bloques de 512 e de 37", juce::String::formatted ("(%.1e)", (double) d));
    }
    {   // bypass: o son tal cal, atrasado a latencia
        BOProcessor p;
        setParam (p, "reducion", 90.0f); setParam (p, "bypass", 1.0f);
        const auto son = sonDeProba (1.0);
        const auto o = procesar (p, son, 1.0, 512);
        const int L = p.getLatencySamples();
        float d = 0;
        for (int c = 0; c < 2; ++c) for (int i = L; i < o.getNumSamples(); ++i) d = juce::jmax (d, std::abs (o.getSample (c, i) - son.getSample (c, i - L)));
        check (std::equal_to<float>() (d, 0.0f), "bypass: o son tal cal (coa latencia)", "(" + juce::String (L) + " mostras)");
    }
    {   // os presets: todos soan, sen valores non numéricos nin pasar do teito
        int mal = 0; float peor = 0;
        for (int i = 0; i < opto::NUM_PRESETS; ++i)
        {
            BOProcessor p; p.cargarPreset (i);
            const auto o = procesar (p, sonDeProba (2.0), 2.0, 512);
            if (! finito (o) || pico (o) < 0.01f || p.presetEditado()) { ++mal; std::printf ("   mal: %s\n", opto::PRESETS[i].nome); }
            peor = juce::jmax (peor, pico (o));
        }
        check (mal == 0 && peor <= 0.8913f, "os presets soan", "(" + juce::String (opto::NUM_PRESETS) + ")");
    }
    {   // mono → estéreo
        BOProcessor p;
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (juce::AudioChannelSet::mono()); l.outputBuses.add (juce::AudioChannelSet::stereo());
        const bool soporta = p.setBusesLayout (l);
        p.prepareToPlay (SR, 512);
        juce::AudioBuffer<float> buf (2, 512);
        const auto son = sonDeProba (0.5);
        bool fin = true;
        for (int pos = 0; pos + 512 <= son.getNumSamples(); pos += 512)
        {
            buf.clear(); buf.copyFrom (0, 0, son, 0, pos, 512);
            juce::MidiBuffer m; p.processBlock (buf, m);
            fin = fin && std::isfinite (buf.getMagnitude (0, 512));
        }
        check (soporta && fin, U ("mono a estéreo"));
    }
    {   // estado
        BOProcessor a;
        a.cargarPreset (2);
        setParam (a, "ganancia", 55.0f); a.uiScale = 0.8f; a.vu = 1;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        BOProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto x = procesar (a, sonDeProba (1.0), 1.5, 512), y = procesar (b, sonDeProba (1.0), 1.5, 512);
        check (diferenza (x, y) < 1e-6f && b.getPreset() == 2 && b.presetEditado() && b.vu == 1 && std::abs (b.uiScale - 0.8f) < 1e-6f, "estado gardado e recuperado",
               "(" + juce::String ((int) mb.getSize()) + " bytes)");
    }
    {   // A/B e presets propios
        BOProcessor p;
        p.cargarPreset (0);
        p.estado.cambiarLado(); p.cargarPreset (6); p.estado.cambiarLado();
        bool ok = p.estado.lado() == 0 && p.getPreset() == 0 && ! p.presetEditado();
        p.estado.cambiarLado();
        ok = ok && p.getPreset() == 6;
        check (ok, "A/B (os mandos e o preset)");
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("barullooptocomp_presets_proba");
        dir.deleteRecursively();
        p.estado.setCarpeta (dir);
        setParam (p, "reducion", 77.0f);
        const bool gardado = p.estado.gardar ("Proba: limitador forte");
        p.cargarPreset (3);
        const auto list = p.estado.presets();
        const bool cargado = list.size() == 1 && list[0].hasFileExtension ("bopreset") && p.estado.cargar (list[0]);
        check (gardado && cargado && std::abs (p.apvts.getRawParameterValue ("reducion")->load() - 77.0f) < 0.01f, "presets propios: gardar e cargar");
        dir.deleteRecursively();
    }
    {   // desfacer
        BOProcessor p;
        p.undo.clearUndoHistory();
        p.estado.novoPaso(); p.cargarPreset (4); p.apvts.copyState(); p.estado.novoPaso();
        setParam (p, "reducion", 90.0f); p.apvts.copyState(); p.estado.novoPaso();
        p.undo.undo();
        const bool u1 = p.getPreset() == 4 && ! p.presetEditado();
        p.undo.undo();
        const bool u2 = p.getPreset() == 0 && ! p.presetEditado();
        check (u1 && u2, "desfacer (preset e mandos)");
    }
    {   // os programas do DAW son os presets
        BOProcessor p;
        p.setCurrentProgram (5);
        check (p.getNumPrograms() == opto::NUM_PRESETS && p.getCurrentProgram() == 5 && p.getProgramName (5) == U (opto::PRESETS[5].nome), "programas do DAW = presets");
    }
    return fails;
}
