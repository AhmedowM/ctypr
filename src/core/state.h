#ifndef STATE_H
#define STATE_H

typedef struct Engine Engine; // Forward declaration of Engine struct

typedef enum EngineState {
    ENGINE_IDLE,
    ENGINE_RUNNING,
    ENGINE_PAUSED,
    ENGINE_ERROR
} EngineState;

typedef enum EngineStopCause {
    ENGINE_STOP_CAUSE_NONE,
    ENGINE_STOP_CAUSE_TIMEOUT,
    ENGINE_STOP_CAUSE_FINISHED,
    ENGINE_STOP_CAUSE_USER,
    ENGINE_STOP_CAUSE_ERROR,
    ENGINE_STOP_CAUSE_UNKNOWN
} EngineStopCause;

typedef struct EngineStateInfo {
    EngineState state;          ///< Current state of the engine
    EngineStopCause stopCause;  ///< Cause of the last stop event
} EngineStateInfo;

EngineStateInfo engineGetStateInfo(Engine* engine);

#endif // STATE_H