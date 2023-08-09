#pragma once

#include "attribute.hpp"
#include "parse_state.hpp"
#include "parse_exception.hpp"
#include "attribute/attribute_interp.hpp"


bool eat_attribute_specifier(parse_state& ps, attribute_specifier& attr_spec = attribute_specifier());
bool eat_attribute_specifier_seq(parse_state& ps, attribute_specifier& attr_spec = attribute_specifier());