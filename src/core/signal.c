#include "signal_internal.h"

#include <string.h>

void signalInit(Signal* sig) {
    if (sig) memset(sig, 0, sizeof(Signal));
}

int signalConnect(Signal* sig, EngineCallback callback, void* userData) {
    if (!sig || !callback) return -1;
    for (int i = 0; i < SIGNAL_MAX_SLOTS; i++) {
        if (!sig->slots[i].active) {
            sig->slots[i].active = true;
            sig->slots[i].callback = callback;
            sig->slots[i].userData = userData;
            return i;
        }
    }
    return -1;
}

void signalDisconnect(Signal* sig, int slotId) {
    if (!sig || slotId < 0 || slotId >= SIGNAL_MAX_SLOTS) return;
    sig->slots[slotId].active = false;
}

void signalEmit(Signal* sig, Engine* engine) {
    if (!sig) return;
    for (int i = 0; i < SIGNAL_MAX_SLOTS; i++) {
        if (sig->slots[i].active && sig->slots[i].callback) {
            sig->slots[i].callback(engine, sig->slots[i].userData);
        }
    }
}

void signalClear(Signal* sig) {
    if (sig) memset(sig, 0, sizeof(Signal));
}
