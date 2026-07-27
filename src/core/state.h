#ifndef STATE_H
#define STATE_H

typedef struct Engine Engine;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Engine operational states.
typedef enum EngineState {
    ENGINE_IDLE,    ///< Not running, no stop cause set
    ENGINE_RUNNING, ///< Actively processing keystrokes
    ENGINE_PAUSED,  ///< Paused, timer frozen
    ENGINE_ERROR    ///< Error encountered
} EngineState;

/// @brief Reasons the engine stopped.
typedef enum EngineStopCause {
    ENGINE_STOP_CAUSE_NONE,     ///< No stop reason (idle or running)
    ENGINE_STOP_CAUSE_TIMEOUT,  ///< Session exceeded time limit
    ENGINE_STOP_CAUSE_FINISHED, ///< All text typed successfully
    ENGINE_STOP_CAUSE_USER,     ///< User explicitly stopped
    ENGINE_STOP_CAUSE_ERROR,    ///< Terminated due to error
    ENGINE_STOP_CAUSE_UNKNOWN   ///< Unknown stop reason
} EngineStopCause;

/// @brief Combined state and stop cause information.
typedef struct EngineStateInfo {
    EngineState state;          ///< Current engine state
    EngineStopCause stopCause;  ///< Cause of last stop event
} EngineStateInfo;

/// @brief Get current state and stop cause.
/// @param engine Engine instance.
/// @return EngineStateInfo with state and stop cause.
///         Returns ENGINE_ERROR / ENGINE_STOP_CAUSE_ERROR if engine is NULL.
EngineStateInfo engineGetStateInfo(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // STATE_H