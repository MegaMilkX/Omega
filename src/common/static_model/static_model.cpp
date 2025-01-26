#include "static_model.hpp"
#include "static_model_instance.hpp"


HSHARED<StaticModelInstance> StaticModel::createInstance() {
    HSHARED<StaticModelInstance> handle(HANDLE_MGR<StaticModelInstance>::acquire());

    *handle = std::move(StaticModelInstance(this));

    return handle;
}

