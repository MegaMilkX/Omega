#pragma once

#include <memory>


namespace xui {


    template<typename T>
    using Ref = std::shared_ptr<T>;


}