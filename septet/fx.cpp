// Compatibility object for general FX lookups of the Septet filter ID.
// Instrument voices use the separate filter factories.
#include "core.h"
namespace septet {
void initialize(State& state) { state.reserved = 0; }
void process(State&, opxy::Channel, opxy::Channel, opxy::Parameters) {}
}
