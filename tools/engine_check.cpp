// Renders the cases written by tools/dump_cases.js with the C++ engine (offline check against the JS engine).
#include "../Source/Engine/SiltEngine.h"
#include "../Source/Engine/ParamTable.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : "cmp"; std::ifstream in(dir + "/cases.txt"); std::string line; int ci = -1;
    silt::Params P; silt::StateSet S {}; struct Ev { double t; int on, n; float v; }; std::vector<Ev> evs;
    while (std::getline(in, line)) {
        std::istringstream ss(line); std::string tag; ss >> tag;
        if (tag == "case") { ss >> ci; P = silt::defaultParams(); evs.clear(); }
        else if (tag == "p") { std::string k; float v; ss >> k >> v; if (float* f = silt::paramField(P, k)) *f = v; else std::fprintf(stderr, "unknown %s\n", k.c_str()); }
        else if (tag == "s") { int st, k; float v; ss >> st >> k >> v; S[st][k] = v; }
        else if (tag == "e") { Ev e; ss >> e.t >> e.on >> e.n >> e.v; evs.push_back(e); }
        else if (tag == "end") {
            silt::Engine eng; eng.prepare(48000);
            const int N = int(std::ceil(5 * 48000 / 128.0)); std::vector<float> out(N * 128 * 2); float L[128], R[128], T[128]; size_t ev = 0;
            auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < N; ++b) { double now = b * 128 / 48000.0;
                // first block: engine needs params before a note-on, as the JS engine does
                if (b == 0) eng.process(P, S, L, R, T, 0);
                while (ev < evs.size() && evs[ev].t <= now) { if (evs[ev].on) eng.noteOn(evs[ev].n, evs[ev].v); else eng.noteOff(evs[ev].n); ++ev; }
                eng.process(P, S, L, R, T, 128);
                for (int i = 0; i < 128; ++i) { out[(b * 128 + i) * 2] = L[i]; out[(b * 128 + i) * 2 + 1] = R[i]; } }
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            std::printf("case %d cpu %.1f%%\n", ci, secs / 5 * 100);
            FILE* f = std::fopen((dir + "/cpp" + std::to_string(ci) + ".f32").c_str(), "wb"); std::fwrite(out.data(), 4, out.size(), f); std::fclose(f);
        }
    }
}
