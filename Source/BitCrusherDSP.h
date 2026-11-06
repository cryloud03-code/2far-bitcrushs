#pragma once
// Núcleo DSP del bitcrusher. No depende de JUCE, así se puede probar solo.
#include <cmath>

namespace twofar
{
    // Un canal: reducción de bits (cuantización) + reducción de sample rate (sample & hold).
    struct Crusher
    {
        void reset() noexcept { counter = 0.0f; held = 0.0f; }

        // bits: 1..16 (acepta decimales), rateDiv: 1..N (mantiene cada muestra N veces)
        float process (float x, float bits, float rateDiv) noexcept
        {
            counter += 1.0f;
            if (counter >= rateDiv)
            {
                counter -= rateDiv;
                if (counter >= 1.0f) counter = 0.0f;   // por seguridad si rateDiv < 1
                held = quantise (x, bits);
            }
            return held;
        }

        static float quantise (float x, float bits) noexcept
        {
            const float q = std::exp2 (bits - 1.0f);
            return std::floor (x * q + 0.5f) / q;
        }

        float counter = 0.0f;
        float held = 0.0f;
    };

    // Nivel "de fiesta" 0..1 usado por la interfaz (luces del bus).
    inline float crushAmount (float bits, float rateDiv) noexcept
    {
        const float a = (16.0f - bits) / 15.0f;                      // 0 = 16 bits, 1 = 1 bit
        const float b = std::log (rateDiv) / std::log (64.0f);       // 0 = ÷1, 1 = ÷64
        const float aa = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
        const float bb = b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);
        return 1.0f - (1.0f - aa) * (1.0f - bb);
    }
}
