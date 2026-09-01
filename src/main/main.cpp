#include "omega_reflect.auto.hpp"

#include "engine.hpp"
#include "test_game/test_game.hpp"
#include "hl2_game/hl2_game.hpp"
#include "terrain_game/terrain_game.hpp"

// TODO: REMOVE THIS !!!
#include "resource_cache/resource_cache.hpp"
#include "static_model/static_model.hpp"

#include "math/fft.hpp"

static void printSamples2(float* samples_before, gfxm::complex* spectrum, gfxm::complex* samples_after, int count) {
    printf("#\tin\tspec\tout\n");
    printf("------------------------------\n");
    for (int i = 0; i < count; ++i) {
        printf("%2i:\t%5.2f\t%5.2f\t%5.2f\n", i, samples_before[i], spectrum[i].real, samples_after[i].real);
    }

    for (int h = count / 2; h >= 0; --h) {
        for (int i = 0; i < count / 2 + 1; ++i) {
            if (i == count / 2) {
                printf("%4i", h);
                continue;
            }
            float s = spectrum[i].mag();
            if (i > count / 2) {
                s = -s;
            }
            if (s + FLT_EPSILON > h) {
                printf("\u00DB\u00DB\u00DB\u00DB"); // Use unicode box \u2589
            } else {
                printf("....");
            }
        }
        printf("\n");
    }
    for (int i = 0; i < count / 2; ++i) {
        printf("%4i", i);
    }
    printf("\n");
}

static void fftTest() {
    const int SAMPLE_COUNT = 32;
    float samples[SAMPLE_COUNT] = { 0 };
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float t = float(i) / float(SAMPLE_COUNT);
        samples[i] = cosf(2.f * gfxm::pi * t * 2.f)
            + cosf(2.f * gfxm::pi * t * 4.f) * .5f
            + sinf(2.f * gfxm::pi * t * 7.f) * .85f
            + cosf(2.f * gfxm::pi * t * 13.f) * .67f;
        if (i > 24) {
            //samples[i] = .0f;
        }
    }

    gfxm::complex spectrum[SAMPLE_COUNT] = { 0 };
    gfxm::complex samples_after[SAMPLE_COUNT] = { 0 };
    
    std::copy(samples, samples + SAMPLE_COUNT, spectrum);
    fft(spectrum, SAMPLE_COUNT);
    
    std::copy(spectrum, spectrum + SAMPLE_COUNT, samples_after);
    fft(samples_after, SAMPLE_COUNT, true);
    //dft(complex_samples, spectrum, SAMPLE_COUNT);
    //idft(spectrum, samples_after, SAMPLE_COUNT);

    printSamples2(samples, spectrum, samples_after, SAMPLE_COUNT);
}

static void indicesTest() {
    const int COUNT = 32;
    int indices[COUNT] = { 0 };
    for (int i = 0; i < COUNT; ++i) {
        indices[i] = i;
    }

    for (int i = 1, j = 0; i < COUNT; ++i) {
        int bit = COUNT >> 1;
        for(; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if(i < j) std::swap(indices[i], indices[j]);
    }

    for (int i = 0; i < COUNT; ++i) {
        printf("%2i\n", indices[i]);
    }
    printf("\n");
}

static void indicesRecursive(int* indices, int count) {
    if (count == 1) {
        return;
    }

    indicesRecursive(&indices[0], count / 2);
    indicesRecursive(&indices[1], count / 2);

    printf("======\n");
    for (int i = 0; i < count / 2; ++i) {
        printf("%2i\n", indices[i]);
        printf("%2i\n", indices[count / 2 + i]);
    }
}
static void indicesTest2() {
    const int COUNT = 32;
    int indices[COUNT] = { 0 };
    for (int i = 0; i < COUNT; ++i) {
        indices[i] = i;
    }
    indicesRecursive(indices, COUNT);
}


#include "engine_runtime/default_runtime.hpp"

int main(int argc, char* argv) {
    cppiReflectInit();

    engineGameInit();

    ConRegistry::get()->registerFloat("phy.gravity", "gravity", 9.8f);
    ConRegistry::get()->registerBool("phy.dbg_draw", "debug draw display", false);

    {
        std::unique_ptr<DefaultRuntime> rt(new DefaultRuntime(
            new TestGameInstance
            //new HL2GameInstance
            //new TerrainGameInstance
        ));
        rt->run();
    }

    ResourceManager::get()->collectGarbage();
    engineGameCleanup();
    return 0;
}


