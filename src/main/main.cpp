#include "omega_reflect.auto.hpp"

#include "engine.hpp"
#include "game/game_test.hpp"

// TODO: REMOVE THIS !!!
#include "resource_cache/resource_cache.hpp"
#include "static_model/static_model.hpp"


int main(int argc, char* argv) {
    cppiReflectInit();

    engineGameInit();

    // TODO: REMOVE THIS !!!
    resAddCache<StaticModel>(new resCacheDefault<StaticModel>());

    ENGINE_INIT_DATA run_data = { 0 };
    run_data.game = new GameTest;
    run_data.primary_viewport = new Viewport(gfxm::rect(0, 0, 1, 1), 0, 0, false);
    run_data.primary_player = new LocalPlayer(run_data.primary_viewport, 0);

    engineGameRun(run_data);
    
    engineGameCleanup();
    return 0;
}
