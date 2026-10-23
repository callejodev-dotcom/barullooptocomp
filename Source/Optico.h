// Optico.h — O son de Barullo Opto Comp, sen JUCE: un compresor óptico coma os amplificadores
// niveladores dos anos 60, peza a peza.
//
//   entrada ─┬─ atenuador (resistencia en serie e a fotorresistencia a terra) ─ ganancia ─ válvula ─ saída
//            │                 ▲ condutancia
//            │       fotorresistencia (CdS) ◄── luz ◄── panel electroluminescente ◄── detector
//            └────────────────────────────────────── (realimentación: o detector escoita a saída do atenuador)
//
// A célula é o que lle dá o carácter:
//   - o panel acéndese co sinal do detector (rectificado) e a súa luz medra coma unha potencia del:
//     esa curva fai o xeonllo e a proporción (ao ir por realimentación, a proporción é 1 + a potencia);
//   - a fotorresistencia colle a luz en ~10 ms e sóltaa en dúas fases: a metade nuns 60 ms e o resto
//     amodo, máis amodo canto máis tempo estivo iluminada (a «memoria» da célula);
//   - o atenuador é un divisor: ganancia = 1 / (1 + condutancia).
// O mando de Redución é a ganancia do detector; o de Ganancia, a da etapa de válvulas (12AX7 e 12BH7:
// unha curva suave e un chisco asimétrica, harmónicos pares). Limitar sobe o detector e endurece a curva.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace opto {

constexpr double PI = 3.14159265358979323846;
enum Modo { COMPRIMIR = 0, LIMITAR };
enum Celula { RAPIDA = 0, CLASICA, LENTA, NUM_CELULAS };

struct Axustes
{
    double reducion = 0.4;       // o mando de Redución, 0..1 (0: non comprime)
    double ganancia = 0.5;       // o mando de Ganancia, 0..1
    int modo = COMPRIMIR;
    double agudos = 0.0;         // canto escoita o detector os agudos de máis (0..1): menos «s» e pratos
    double corteDetector = 20;   // Hz: o detector non escoita por debaixo (menos bombeo co baixo)
    double mestura = 1.0;        // compresión en paralelo (0..1)
    int celula = CLASICA;
};

// as ganancias dos dous mandos, en dB (tamén para a pantalla)
inline double detectorDb (double r) { return -24.0 + 60.0 * std::clamp (r, 0.0, 1.0); }   // a do detector (o mando de Redución)
inline double gananciaDb (double g) { return -20.0 + 60.0 * std::clamp (g, 0.0, 1.0); }   // 0,333 → 0 dB

// os tempos de cada célula: ataque, a primeira metade da soltura e a segunda (sen e con memoria)
struct Tempos { double ataque, solturaRapida, solturaLenta, memoria; };
inline const Tempos TEMPOS[NUM_CELULAS] = {
    { 0.008, 0.020, 0.25, 3.0 },   // rápida
    { 0.020, 0.035, 0.45, 4.0 },   // clásica
    { 0.032, 0.065, 0.90, 5.0 },   // lenta
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    void pasoAlto (double fs, double f)
    {
        const double w = 2 * PI * std::clamp (f, 5.0, fs * 0.45) / fs, c = std::cos (w), al = std::sin (w) / (2 * 0.7071);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void andelAgudos (double fs, double f, double db)
    {
        const double A = std::pow (10.0, db / 40), w = 2 * PI * std::clamp (f, 5.0, fs * 0.45) / fs, c = std::cos (w), sn = std::sin (w);
        const double al = sn / 2 * std::sqrt (2.0), r = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + r), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - r),
             (A + 1) - (A - 1) * c + r, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - r);
    }
};

// a célula T4: o panel, a fotorresistencia e a súa memoria
class Celula4
{
public:
    void preparar (double frecuencia) { fs = frecuencia; limpar(); axustar (tipo); }
    void limpar() { luz = g1 = g2 = memoria = 0; }
    void axustar (int c)
    {
        tipo = std::clamp (c, 0, NUM_CELULAS - 1);
        const auto& t = TEMPOS[tipo];
        kPanelSobe = coef (0.0008); kPanelBaixa = coef (0.012);
        kAtaque = coef (t.ataque); kSolta1 = coef (t.solturaRapida);
        lenta = t.solturaLenta; kMemSobe = coef (t.memoria); kMemBaixa = coef (t.memoria * 4.0);
    }
    // a luz que pide o detector (xa coa súa curva) → a condutancia da fotorresistencia
    double paso (double obx)
    {
        // o panel non dá máis luz ca a que dá: satura suave (como moito, uns 30 dB de redución)
        obx = LUZ_MAX * std::tanh (obx / LUZ_MAX);
        luz += (obx - luz) * (obx > luz ? kPanelSobe : kPanelBaixa);
        // as dúas metades da fotorresistencia, en ln (1 + condutancia), que vai coma os dB do que
        // comprime: suben igual e baixan cada unha ao seu tempo (a metade dos dB, axiña; o resto, amodo)
        const double u = std::log1p (luz);
        g1 += (u - g1) * (u > g1 ? kAtaque : kSolta1);
        const double kSolta2 = coef (lenta * (1.0 + 4.0 * memoria));
        g2 += (u - g2) * (u > g2 ? kAtaque : kSolta2);
        const double media = 0.5 * (g1 + g2);
        // a memoria: enche mentres a célula está iluminada e baleira amodo
        const double alvo = std::min (1.0, media * 0.35);
        memoria += (alvo - memoria) * (alvo > memoria ? kMemSobe : kMemBaixa);
        return std::expm1 (media);
    }
    double brillo() const { return luz; }
    double lembranza() const { return memoria; }

    static constexpr double LUZ_MAX = 30.0;

private:
    double coef (double s) const { return 1.0 - std::exp (-1.0 / (std::max (1e-5, s) * fs)); }
    double fs = 48000, luz = 0, g1 = 0, g2 = 0, memoria = 0, lenta = 0.45;   // (g1 e g2: as dúas metades, en ln)
    double kPanelSobe = 0.02, kPanelBaixa = 0.002, kAtaque = 0.002, kSolta1 = 0.0003, kMemSobe = 0, kMemBaixa = 0;
    int tipo = CLASICA;
};

// a etapa de válvulas: unha curva suave e un chisco asimétrica (ganancia 1 para sinais pequenos)
inline double valvula (double x)
{
    constexpr double a = 0.9, b = 0.18;
    const double t = std::tanh (b);
    return (std::tanh (a * x + b) - t) / (a * (1.0 - t * t));
}

// todo o aparello, mostra a mostra (estéreo ligado: unha célula para as dúas canles)
class Optico
{
public:
    void preparar (double frecuencia)
    {
        fs = frecuencia;
        celula.preparar (fs);
        limpar();
        axustar (a, true);
    }
    void limpar()
    {
        celula.limpar();
        for (auto& f : detector) f.reset();
        for (auto& f : dc) f.reset();
        ganancia = 1.0;
    }
    void axustar (const Axustes& x, bool forzar = false)
    {
        const bool filtros = forzar || ! std::equal_to<double>() (x.agudos, a.agudos) || ! std::equal_to<double>() (x.corteDetector, a.corteDetector);
        if (forzar || x.celula != a.celula) celula.axustar (x.celula);
        a = x;
        if (filtros)
            for (int c = 0; c < 2; ++c)
            {
                detector[(size_t) c].andelAgudos (fs, 3000.0, 12.0 * std::clamp (a.agudos, 0.0, 1.0));
                detector[(size_t) (c + 2)].pasoAlto (fs, a.corteDetector);
                dc[(size_t) c].pasoAlto (fs, 6.0);
            }
        // o detector: a ganancia do mando e, ao limitar, 6 dB máis e unha curva máis dura
        kDetector = a.reducion <= 0.0 ? 0.0 : std::pow (10.0, (detectorDb (a.reducion) + (a.modo == LIMITAR ? 6.0 : 0.0)) / 20.0) * ESCALA;
        expoente = a.modo == LIMITAR ? 6.0 : 2.0;
        gananciaSaida = std::pow (10.0, gananciaDb (a.ganancia) / 20.0);
    }
    // unha mostra estéreo, no seu sitio
    void procesar (double& l, double& r)
    {
        const double sl = l, sr = r;
        // o atenuador coa ganancia de agora; o detector escoita a súa saída (realimentación)
        double al = l * ganancia, ar = r * ganancia;
        const double dl = detector[2].run (detector[0].run (al)), dr = detector[3].run (detector[1].run (ar));
        const double d = std::max (std::abs (dl), std::abs (dr)) * kDetector;
        const double g = celula.paso (std::pow (d, expoente));
        ganancia = 1.0 / (1.0 + g);
        // a etapa de válvulas, despois da ganancia
        double yl = dc[0].run (valvula (al * gananciaSaida)), yr = dc[1].run (valvula (ar * gananciaSaida));
        // en paralelo: o seco, coa mesma ganancia de saída
        const double m = std::clamp (a.mestura, 0.0, 1.0);
        if (m < 1.0) { yl = yl * m + sl * gananciaSaida * (1.0 - m); yr = yr * m + sr * gananciaSaida * (1.0 - m); }
        l = yl; r = yr;
    }
    double reducionDb() const { return 20.0 * std::log10 (ganancia); }   // ≤ 0
    double brillo() const { return celula.brillo(); }
    double memoria() const { return celula.lembranza(); }
    const Axustes& axustes() const { return a; }

    // o detector, calibrado: co mando a 50 e un seno a −18 dBFS RMS, comprimindo, uns 6 dB
    static constexpr double ESCALA = 6.0;

private:
    double fs = 48000, ganancia = 1.0, kDetector = 0, expoente = 2.0, gananciaSaida = 1.0;
    Axustes a;
    Celula4 celula;
    std::array<Biquad, 4> detector;   // andel de agudos (L, R) e paso alto (L, R)
    std::array<Biquad, 2> dc;
};

} // namespace opto
