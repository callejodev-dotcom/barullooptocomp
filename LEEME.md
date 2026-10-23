**Barullo Opto Comp** é un compresor óptico, coma os amplificadores niveladores dos anos 60: unha luz e unha fotorresistencia deciden canto baixar, e por iso comprime tan suave. Dous mandos grandes (*Redución* e *Ganancia*), *Comprimir / Limitar* e un VU de agulla; detrás de **MÁIS**, o detector, a mestura en paralelo e a célula.

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3, CLAP e app | `BarulloOptoComp-…-mac.pkg` | `BarulloOptoComp-…-mac.zip` |
| **Windows** (64 bits): VST3, CLAP e app | `BarulloOptoComp-…-windows-setup.exe` | `BarulloOptoComp-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2, CLAP e app | — | `BarulloOptoComp-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Que ten

- **Redución**: canto escoita o detector (a 0, nada). Co mando a 50 e un son a −18 dBFS, uns 6 dB.
- **Ganancia**: o volume despois da célula, pola etapa de válvulas (de −20 a +40 dB; o visor dío en dB). Con moita, as válvulas engaden harmónicos pares.
- **Comprimir / Limitar**: comprimindo, uns 3:1 nos picos e un xeonllo moi suave; limitando, o detector escoita 6 dB máis e a curva é moito máis dura (uns 7:1).
- **O VU**: amosa canto comprime (en 0, nada; cara á esquerda, os dB que baixa) ou a saída (0 VU = −18 dBFS), coa inercia dun de verdade.
- **A célula**: vese acenderse co que comprime.
- **MÁIS**:
  - *Agudos no detector*: escoita máis os agudos e baixan antes os «s» e os pratos (coma o axuste que levaban por detrás);
  - *Corte do detector*: non escoita por debaixo (ata 300 Hz), para que o baixo e o bombo non fagan bombear o resto;
  - *Mestura*: compresión en paralelo;
  - *Célula*: rápida, clásica ou lenta.
- **9 presets de fábrica**: Voz suave, Voz pop, Baixo, Bus de batería, Guitarra acústica, Pegar a mestura, Limitador, Piano e Esmagado en paralelo. Son tamén os programas do DAW.
- **Presets propios, A/B e desfacer** (do núcleo común; `.bopreset`, en Documentos/Barullo Opto Comp/Presets).
- A luz de acendido, ao carón do nome, é o bypass: o son pasa tal cal (coa latencia).
- Un **limitador de seguridade** á saída: nunca pasa de −1 dBFS.

## Como se fixo

`Source/Optico.h`, sen JUCE, peza a peza:

```
entrada ─┬─ atenuador (resistencia en serie e a fotorresistencia a terra) ─ ganancia ─ válvula ─ saída
         │                 ▲ condutancia
         │       fotorresistencia ◄── luz ◄── panel electroluminescente ◄── detector
         └────────────────────────────── (realimentación: o detector escoita a saída do atenuador)
```

- **O panel** acéndese co sinal do detector rectificado, e a súa luz medra coma unha potencia del (ao cadrado comprimindo, á sexta limitando) ata que satura (como moito, uns 30 dB de redución). Ao ir por realimentación, esa potencia é a curva: proporción 1 + potencia e un xeonllo que sae só.
- **A fotorresistencia** colle a luz en ~10 ms e sóltaa en dúas metades (contadas en dB): a primeira nun centenar de ms e a segunda amodo, máis amodo canto máis tempo estivo iluminada (a memoria da célula: tras uns segundos comprimindo, tarda o dobre en volver).
- **O atenuador** é un divisor: ganancia = 1 / (1 + condutancia).
- **As válvulas**: unha curva suave e un chisco asimétrica (harmónicos pares), a dobre frecuencia de mostraxe para que os harmónicos non volvan como ruído.
- **Estéreo ligado**: unha célula para as dúas canles.

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/Optico.h` | O compresor: a célula, o detector e as válvulas |
| `Source/Presets.h` | Os presets de fábrica |
| `Source/PluginProcessor.*` | O plugin: parámetros, presets, estado e o sobremostrado |
| `Source/PluginEditor.*` | A interface: o VU, a célula e os mandos |
| `tools/RenderTest.cpp` | A proba |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.4.0): o texto en galego, o aviso das versións novas, os presets propios, o A/B e o desfacer (`escoita::Estado`), o limitador de seguridade (`escoita::Teito`), a CI e os instaladores.

## Probas

- **Ao abrir**: o primeiro preset, cos parámetros nos seus valores de partida.
- **A curva**: comprimindo, uns 6 dB a −18 dBFS co mando a 50 e unha proporción de 2–3,5:1; limitando, máis de 4,5:1. Baixo ou coa Redución a 0, nada.
- **A célula clásica**: o ataque (3–20 ms), a primeira metade da soltura (40–250 ms), o resto (0,5–3 s) e a memoria (tras 5 s comprimindo, solta polo menos un 30 % máis amodo). As tres células, en orde.
- **O detector**: cos agudos de máis, un 6 kHz comprímese 3 dB máis; co corte a 300 Hz, un 50 Hz case non.
- **Mestura a 0**: o seco. **Válvulas**: o segundo harmónico. **Teito**: −1 dBFS.
- **Bloques** (512 e 37), **bypass** (tal cal coa latencia), **presets**, **mono**, **estado**, **A/B**, **presets propios**, **desfacer** e **programas do DAW**.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator.

Na CI van en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Compilar

Nun Mac: `./build_mac.sh` (compila e instala no teu usuario). Á man:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBARULLO_TESTS=ON
cmake --build build --config Release
build/BORenderTest_artefacts/Release/BORenderTest
```

Outras ordes da proba: `--snapshot saida.png [mais]` (a interface) e `--demo preset entrada.wav saida.wav`.

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

## Créditos e licenzas

- **Barullo Opto Comp é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`). Usa JUCE 8 coa súa licenza AGPLv3.
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
- Non ten relación con ningún fabricante: é un compresor óptico feito desde cero, coa idea dos de antes.
