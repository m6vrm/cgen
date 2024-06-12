#include <debug.hpp>
#include <poost/assert.hpp>

namespace cgen {

auto node_dump(const YAML::Node &node) -> std::string {
    YAML::Emitter emitter;
    emitter.SetSeqFormat(YAML::Flow);
    emitter.SetMapFormat(YAML::Flow);
    emitter << node;
    POOST_ASSERT(emitter.good(), "invalid node to emit");
    return emitter.c_str();
}

} // namespace cgen
