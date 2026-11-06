#include "../Source/BitCrusherDSP.h"
#include <cstdio>
#include <set>
#include <cmath>

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++fails; } else std::printf("ok:   %s\n", msg); } while (0)

int main()
{
    const float pi = 3.14159265f;

    // 1) 16 bits, ÷1 => casi transparente
    {
        twofar::Crusher c; float maxErr = 0.f;
        for (int i = 0; i < 4800; ++i)
        {
            float x = 0.8f * std::sin (2.f * pi * 440.f * i / 48000.f);
            maxErr = std::fmax (maxErr, std::fabs (c.process (x, 16.f, 1.f) - x));
        }
        CHECK (maxErr < 1.0f / 32768.f, "16 bits y rate 1 casi no alteran la señal");
    }

    // 2) 1 bit => solo -1, 0, 1
    {
        twofar::Crusher c; std::set<float> vals;
        for (int i = 0; i < 4800; ++i)
            vals.insert (c.process (0.9f * std::sin (2.f * pi * 220.f * i / 48000.f), 1.f, 1.f));
        bool ok = true; for (float v : vals) ok = ok && (v == -1.f || v == 0.f || v == 1.f);
        CHECK (ok && vals.size() <= 3, "1 bit solo produce -1, 0 y 1");
    }

    // 3) 4 bits => como mucho 2^4 + 1 niveles
    {
        twofar::Crusher c; std::set<float> vals;
        for (int i = 0; i < 48000; ++i)
            vals.insert (c.process (std::sin (2.f * pi * 100.f * i / 48000.f), 4.f, 1.f));
        CHECK ((int) vals.size() <= 17, "4 bits limita los niveles distintos");
    }

    // 4) rate ÷4 => se mantiene el valor 4 muestras
    {
        twofar::Crusher c; bool ok = true; float prev = 0.f; int run = 0;
        for (int i = 0; i < 400; ++i)
        {
            float y = c.process (std::sin (0.05f * i), 16.f, 4.f);
            if (i > 0 && y == prev) ++run; else { if (i > 8 && run != 3) ok = false; run = 0; }
            prev = y;
        }
        CHECK (ok, "rate ÷4 mantiene cada valor 4 muestras");
    }

    // 5) rate fraccionario ÷2.5 => cambia 2 de cada 5 muestras en promedio
    {
        twofar::Crusher c; int changes = 0; float prev = 99.f;
        for (int i = 0; i < 5000; ++i)
        {
            float y = c.process (std::sin (0.37f * i), 16.f, 2.5f);
            if (y != prev) { ++changes; }
            prev = y;
        }
        CHECK (std::fabs (changes / 5000.f - 0.4f) < 0.02f, "rate fraccionario ÷2.5 promedia 0.4 cambios por muestra");
    }

    // 6) crushAmount
    CHECK (twofar::crushAmount (16.f, 1.f) == 0.f, "crushAmount = 0 sin efecto");
    CHECK (std::fabs (twofar::crushAmount (1.f, 1.f) - 1.f) < 1e-6f, "crushAmount = 1 con 1 bit");
    CHECK (std::fabs (twofar::crushAmount (16.f, 64.f) - 1.f) < 1e-6f, "crushAmount = 1 con rate ÷64");

    std::printf(fails ? "\n%d pruebas fallaron\n" : "\nTodas las pruebas pasaron\n", fails);
    return fails ? 1 : 0;
}
