# 2far Bitcrusher (VST3 para Windows / FL Studio)

Bitcrusher estilo panameño: **diablo rojo** con luces de fiesta por dentro que se
encienden más fuerte mientras más bitcrush le pongas.

## Qué hace

| Perilla | Qué controla |
|---------|--------------|
| BITS    | Profundidad de bits (16 = limpio, 1 = destrucción total) |
| RATE    | Reducción de sample rate (1 = normal, 64 = muy sucio) |
| MIX     | Seco / mojado |
| OUTPUT  | Volumen de salida (-24 a +6 dB) |

Interfaz:
- El **bus** rebota y se inclina; las ventanas se llenan de luces de discoteca y
  siluetas bailando, los domos rosados y los faros parpadean. Todo escala con el bitcrush
  y late con el nivel de audio.
- La imagen del anime y la foto con la bandera se **pixelan** a medida que subes el bitcrush.
- Medidor "PARTY" con colores de la bandera, confeti de estrellas, reflectores giratorios
  y una cinta con **"2FAR MADE THIS"**.

## Compilar (Windows)

### Opción A: en tu PC
1. Instala **Visual Studio 2022** (carga de trabajo "Desarrollo para el escritorio con C++"),
   **CMake** y **Git**.
2. Doble clic en `build_windows.bat` (la primera vez descarga JUCE, tarda unos minutos).
3. El plugin queda en `build\TwoFarBitcrusher_artefacts\Release\VST3\2far Bitcrusher.vst3`.

### Opción B: sin instalar nada (GitHub Actions)
1. Sube esta carpeta a un repositorio nuevo de GitHub.
2. Pestaña **Actions** -> "Build 2far Bitcrusher (Windows VST3)" -> espera que termine.
3. Descarga el artefacto `2far-Bitcrusher-VST3-Windows`.

## Instalar en FL Studio
1. Copia la carpeta **`2far Bitcrusher.vst3`** completa a `C:\Program Files\Common Files\VST3`
   (pide permisos de administrador).
2. En FL Studio: **Options -> Manage plugins -> Find installed plugins**.
3. Búscalo como "2far Bitcrusher" y ponlo en un canal del Mixer.

## Pruebas del DSP

```
g++ -std=c++17 tests/dsp_test.cpp -o dsp_test && ./dsp_test
```

## Estructura

- `Source/BitCrusherDSP.h`: el efecto (sin dependencias).
- `Source/PluginProcessor.*`: parámetros y audio.
- `Source/PluginEditor.*`: toda la interfaz animada.
- `Assets/`: las 3 imágenes (se incrustan dentro del plugin).
