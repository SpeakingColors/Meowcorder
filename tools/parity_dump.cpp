// Prints a fingerprint of every operation on a fixed test sound, for tools/parity.js to compare with the JavaScript engine.
#include "../src/core/MeowCore.h"
#include <cstdio>
using namespace mc;
int main()
{
    Audio a; a.sr = 48000; a.resize (24000);
    for (size_t i = 0; i < a.frames(); ++i)
    {
        const double t = (double) i / 48000.0;
        a.ch[0][i] = (float) (0.4 * std::sin (2 * 3.14159265358979 * 440 * t) + 0.2 * std::sin (2 * 3.14159265358979 * 3100 * t + 1.0) + (i % 6000 < 300 ? 0.3 * std::exp (-(double) (i % 6000) / 80.0) : 0.0));
        a.ch[1][i] = (float) (0.35 * std::sin (2 * 3.14159265358979 * 440 * t + 0.4) + 0.25 * std::sin (2 * 3.14159265358979 * 2300 * t));
    }
    for (int op = 0; op < opCount; ++op)
    {
        Audio b = a; float p[kMaxOpParams]; defaultParams (op, p);
        const size_t e = applyOp (b, op, 2000, 20000, p);
        double s0 = 0, s1 = 0, q = 0; for (size_t i = 0; i < b.frames(); ++i) { s0 += b.ch[0][i]; s1 += b.ch[1][i]; q += (double) b.ch[0][i] * b.ch[0][i] + (double) b.ch[1][i] * b.ch[1][i]; }
        std::printf ("%s %zu %zu %.6f %.6f %.6f %.6f %.6f\n", kOps[op].id, b.frames(), e, s0, s1, q, b.ch[0][b.frames() / 2], b.ch[1][b.frames() / 3]);
    }
}
