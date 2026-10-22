// Presets.h — Os presets de fábrica de Barullo Opto Comp. O primeiro é o de partida (os parámetros saen del).
#pragma once
#include "Optico.h"

namespace opto {

struct Preset
{
    const char* nome;
    const char* descricion;
    double reducion, ganancia;   // os dous mandos, 0..100
    int modo;
    double agudos, corte, mestura;
    int celula;
};

inline const Preset PRESETS[] = {
    //  nome                descrición                                                                         red   gan   modo       agudos corte mest  célula
    { "Voz suave",          "A voz sempre diante sen que se note: uns 5 dB, soltando amodo.",                    45,  42,  COMPRIMIR, 0.15,  60, 1.00, CLASICA },
    { "Voz pop",            "Máis apertada e máis preto: ata 10 dB, cos «s» un pouco máis tranquilos.",           60,  48,  COMPRIMIR, 0.35,  80, 1.00, CLASICA },
    { "Baixo",              "Cada nota ao mesmo volume, sen perder o golpe: a célula lenta.",                     55,  45,  COMPRIMIR, 0.00,  20, 1.00, LENTA },
    { "Bus de batería",     "Apertada e en paralelo: corpo e sala sen perder os golpes.",                          70,  44,  COMPRIMIR, 0.00, 100, 0.55, RAPIDA },
    { "Guitarra acústica",  "Parella e redonda, cos rasgueos máis igualados.",                                     45,  41,  COMPRIMIR, 0.20,  80, 1.00, CLASICA },
    { "Pegar a mestura",    "Só un par de dB, moi amodo: a mestura xunta, sen bombear.",                          30,  37,  COMPRIMIR, 0.00, 120, 1.00, LENTA },
    { "Limitador",          "Os picos, a raia: limitar coa célula rápida.",                                      55,  47,  LIMITAR,   0.10,  40, 1.00, RAPIDA },
    { "Piano",              "Un piano máis parello e máis preto, sen que deixe de respirar.",                     40,  40,  COMPRIMIR, 0.05,  40, 1.00, LENTA },
    { "Esmagado en paralelo","Moita compresión mesturada co seco: densidade sen perder a dinámica.",              85,  46,  COMPRIMIR, 0.00,  60, 0.40, RAPIDA },
};

constexpr int NUM_PRESETS = (int) (sizeof (PRESETS) / sizeof (PRESETS[0]));

} // namespace opto
