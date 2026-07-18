#ifndef STATE_H
#define STATE_H

typedef struct Engine Engine;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Engine operational states.
typedef enum EngineState {
    ENGINE_IDLE,    ///< Engine is idle (not running, paused, or in error)
    ENGINE_RUNNING, ///< Engine is actively processing keystrokes
    ENGINE_PAUSED,  ///< Engine is paused, timing is stopped
    ENGINE_ERROR    ///< Engine encountered an error
} EngineState;

/// @brief Reasons why the engine stopped, for use alongside EngineState.
typedef enum EngineStopCause {
    ENGINE_STOP_CAUSE_NONE,     ///< No stop reason (engine is idle or running)
    ENGINE_STOP_CAUSE_TIMEOUT,  ///< Session exceeded configured time limit
    ENGINE_STOP_CAUSE_FINISHED, ///< All text was typed successfully
    ENGINE_STOP_CAUSE_USER,     ///< User explicitly stopped the session
    ENGINE_STOP_CAUSE_ERROR,    ///< Session terminated due to an error
    ENGINE_STOP_CAUSE_UNKNOWN   ///< Unknown stop reason
} EngineStopCause;

/// @brief Combined state information returned by engineGetStateInfo.
typedef struct EngineStateInfo {
    EngineState state;          ///< Current state of the engine
    EngineStopCause stopCause;  ///< Cause of the last stop event
} EngineStateInfo;

/// @brief Get the current combined state and stop cause of the engine.
/// @param engine The Engine instance.
/// @return An EngineStateInfo struct with the current state and stop cause.
///         Returns ENGINE_ERROR / ENGINE_STOP_CAUSE_ERROR if engine is NULL.
EngineStateInfo engineGetStateInfo(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // STATE_H
