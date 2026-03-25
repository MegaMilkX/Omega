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

#include "resource_manager/resource_manager.hpp"

uint32_t hash(uint32_t a)
{
    a = (a+0x7ed55d16u) + (a<<12);
    a = (a^0xc761c23cu) ^ (a>>19);
    a = (a+0x165667b1u) + (a<<5);
    a = (a+0xd3a2646cu) ^ (a<<9);
    a = (a+0xfd7046c5u) + (a<<3);
    a = (a^0xb55a4f09u) ^ (a>>16);
    return a;
}
void normalKeyTest() {
    const int WIDTH = 512;
    const int HEIGHT = 512;
    const int GRANULARITY = 4;
    std::vector<uint32_t> pixels;
    pixels.resize(WIDTH * HEIGHT);
    for (int y = 0; y < HEIGHT; ++y) {
        float angle_y = (y) / float(HEIGHT) * gfxm::pi - gfxm::pi * .5f;
        for (int x = 0; x < WIDTH; ++x) {
            float angle_x = (x) / float(WIDTH) * gfxm::pi * 2.f;
            float nx = cosf(angle_x) * cosf(angle_y);
            float ny = sinf(angle_x) * cosf(angle_y);
            float nz = sinf(angle_y);

            gfxm::vec3 N = gfxm::vec3(nx, ny, nz);

            //float u = gfxm::dot(N, gfxm::vec3(.0f, .0f, 1.0f));
            //float v = gfxm::dot(N, gfxm::vec3(1.0f, .0f, .0f));
            //float w = gfxm::dot(N, gfxm::vec3(.0f, .0f, -1.f));
            /*
            float pitch = asinf(-ny);
            float yaw = atan2f(nx, nz);
            float u = nx;
            float v = pitch / gfxm::pi;
            float w = .0f;
            */
            /*
            uint32_t col = 0xFF000000;
            {
            int G = 16;
            int ix = (nx + 1.0f) * .5f * G;
            int iy = (ny + 1.0f) * .5f * G;
            int iz = (nz + 1.0f) * .5f * G;
            col |= ix;
            col |= iy << 8;
            col |= iz << 16;
            }*/

            float u = nx;
            float v = ny;
            float w = nz;

            int iu = u * float(GRANULARITY);
            int iv = v * float(GRANULARITY);
            int iw = w * float(GRANULARITY);
            u = iu / float(GRANULARITY);
            v = iv / float(GRANULARITY);
            w = iw / float(GRANULARITY);
            u = (u + 1.f) * .5f;
            v = (v + 1.f) * .5f;
            w = (w + 1.f) * .5f;

            uint32_t h = gfxm::make_rgba32(u, v, w, 1.f);
            h = hash(h);
            pixels[x + y * WIDTH] = h;

            /*
            const int Qb = 5;
            const int Q = (1 << Qb) - 1;
            gfxm::vec3 np = (N + gfxm::vec3(1.f, 1.f, 1.f)) * .5f;
            gfxm::ivec3 nq(np.x * Q, np.y * Q, np.z * Q);
            uint32_t m = uint32_t(nq.x) | uint32_t(nq.y << Qb) | uint32_t(nq.z << (Qb * 2));
            uint32_t h = hash(m);
            pixels[x + y * WIDTH] = gfxm::make_rgba32((h & 255) / 255.f, ((h >> 8) & 255) / 255.f, ((h >> 16) & 255) / 255.f, 1.f);
            */

            //pixels[x + y * WIDTH] = col;
        }
    }
    stbi_write_png("normalkey.png", WIDTH, HEIGHT, 4, pixels.data(), 0);
    //return 0;
}

#include "base64/base64.hpp"

int main(int argc, char* argv) {
    /*
    {
        auto paths = fsFindAllFiles(".", "*.skeleton");
        for (int i = 0; i < paths.size(); ++i) {
            LOG_DBG(paths[i]);
            Skeleton skl;
            file_reader in(paths[i], e_skeleton);
            if (!skl.load(in)) {
                LOG_ERR("failed to load skeleton for conversion: " << paths[i]);
                continue;
            }
            //skl.save(paths[i]);
        }
        Sleep(2000);
        return 0;
    }*/

    /*
    {
        auto paths = fsFindAllFiles(".", "*.anim");
        auto paths2 = fsFindAllFiles(".", "*.animation");
        paths.insert(paths.end(), paths2.begin(), paths2.end());
        for (int i = 0; i < paths.size(); ++i) {
            std::ifstream f(paths[i]);
            if (!f) {
                LOG_ERR("Failed to open: " << paths[i]);
                continue;
            }
            nlohmann::json json;
            try {
                json << f;
            } catch (...) {
                LOG_ERR("Not a json file: " << paths[i]);
                continue;
            }
            f.close();

            if (!json.is_object()) {
                LOG_ERR("json expected to be an object: " << paths[i]);
                continue;
            }
            auto it = json.find("imported");
            if (it == json.end()) {
                LOG_ERR("not an uaf file: " << paths[i]);
                continue;
            }
            LOG_DBG(paths[i]);

            std::string encoded = it.value().get<std::string>();
            std::vector<char> decoded;
            if (!base64_decode(encoded.data(), encoded.size(), decoded)) {
                LOG_ERR("failed to decode base64: " << paths[i]);
                continue;
            }
            
            FILE* fout = fopen(paths[i].c_str(), "wb");
            if (!fout) {
                LOG_ERR("failed to open for writing: " << paths[i]);
                continue;
            }
            fwrite(decoded.data(), decoded.size(), 1, fout);
            fclose(fout);
        }
        return 0;
    }*/

    {
        ResourceRef<TextData> ref = loadResource<TextData>("file://text/text.json");
        if (ref) {
            LOG_DBG("ref is valid");
            ref->print();
        } else {
            LOG_DBG("ref is invalid");
        }
    }

    cppiReflectInit();

    engineGameInit();

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


