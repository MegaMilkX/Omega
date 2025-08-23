#pragma once

#include "gjk.hpp"
#include "epa.hpp"


template<typename SHAPEA_T, typename SHAPEB_T>
bool GJKEPA_T(
    GJKEPA_SupportGetter<SHAPEA_T> support_getter_a,
    GJKEPA_SupportGetter<SHAPEB_T> support_getter_b,
    GJK_Simplex& simplex,
    EPA_Context& epa_ctx,
    EPA_Result& result
) {
    if (!GJK_T(support_getter_a, support_getter_b, simplex)) {
        return false;
    }

    result = EPA_T(support_getter_a, support_getter_b, epa_ctx, simplex);
    
    return result.valid;
}

