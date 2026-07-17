#ifndef SIGNAL_INTERNAL_H
#define SIGNAL_INTERNAL_H

#include "signal.h"

#define SIGNAL_MAX_SLOTS 5

typedef struct SignalSlot {
    bool active;
    EngineCallback callback;
    void* userData;
} SignalSlot;

struct Signal {
    SignalSlot slots[SIGNAL_MAX_SLOTS];
};

#endif
