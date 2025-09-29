#include "SatProtoComponentAi.hpp"
static unsigned int hb=0;
SatProtoComponentAi::SatProtoComponentAi() {}
void SatProtoComponentAi::ping(){ hb++; }
unsigned int SatProtoComponentAi::heartbeat() const { return hb; }
