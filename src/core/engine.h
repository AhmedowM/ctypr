#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Engine Engine;
typedef struct Logger Logger;

/// @brief Typing modes that control how keystrokes are processed.
typedef enum EngineMode {
    StrictMode,  ///< Incorrect keys do not advance the cursor; must retry the correct key
    FlowMode,    ///< Incorrect keys still advance the cursor; backspace is supported
    UnknownMode  ///< Sentinel for uninitialized or invalid mode
} EngineMode;

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Create a new Engine instance.
/// @param mode    Desired typing mode (StrictMode or FlowMode).
/// @param timeout Maximum session duration in seconds (0 = no limit).
/// @return A pointer to the newly created Engine, or NULL on allocation failure.
Engine* engineCreate(EngineMode mode, uint16_t timeout);

/// @brief Destroy an Engine instance and free all associated resources.
/// @param self The Engine instance to destroy (NULL is safe).
void engineDestroy(Engine* self);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger to the engine for diagnostic output.
/// @param self   The Engine instance.
/// @param logger The Logger instance to attach (NULL to detach).
void engineSetLogger(Engine* self, Logger* logger);

// ── Mode ─────────────────────────────────────────────────────────────────────

/// @brief Set the typing mode.
/// @param self The Engine instance.
/// @param mode The desired EngineMode (StrictMode or FlowMode).
void engineSetMode(Engine* self, EngineMode mode);

/// @brief Get the current typing mode.
/// @param self The Engine instance.
/// @return The current EngineMode, or UnknownMode if self is NULL.
EngineMode engineGetMode(Engine* self);

// ── Timeout ──────────────────────────────────────────────────────────────────

/// @brief Set the session timeout.
/// @param self    The Engine instance.
/// @param timeout Maximum session duration in seconds (0 = no limit).
void engineSetTimeout(Engine* self, uint16_t timeout);

/// @brief Get the current session timeout.
/// @param self The Engine instance.
/// @return The timeout in seconds, or 0 if self is NULL.
uint16_t engineGetTimeout(Engine* self);

// ── State Control ────────────────────────────────────────────────────────────

/// @brief Start a new typing session.
/// @param self The Engine instance.
void engineStart(Engine* self);

/// @brief Stop the current session (user-initiated stop).
/// @param self The Engine instance.
void engineStop(Engine* self);

/// @brief Pause the current session, freezing the timer.
/// @param self The Engine instance.
void enginePause(Engine* self);

/// @brief Resume a paused session, restarting the timer.
/// @param self The Engine instance.
void engineResume(Engine* self);

/// @brief Reset the engine to idle state, clearing session data.
/// @param self The Engine instance.
void engineReset(Engine* self);

// ── State Queries ────────────────────────────────────────────────────────────

/// @brief Check if the engine is currently running.
/// @param self The Engine instance.
/// @return true if state is ENGINE_RUNNING.
bool engineIsRunning(Engine* self);

/// @brief Check if the engine is paused.
/// @param self The Engine instance.
/// @return true if state is ENGINE_PAUSED.
bool engineIsPaused(Engine* self);

/// @brief Check if the engine is idle (not running, no stop cause set).
/// @param self The Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is NONE.
bool engineIsIdle(Engine* self);

/// @brief Check if the engine is in an error state.
/// @param self The Engine instance.
/// @return true if state is ENGINE_ERROR.
bool engineIsError(Engine* self);

/// @brief Check if the session completed successfully (all text typed).
/// @param self The Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is FINISHED.
bool engineIsCompleted(Engine* self);

/// @brief Check if the session timed out.
/// @param self The Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is TIMEOUT.
bool engineIsTimedOut(Engine* self);

/// @brief Check if the session was stopped by the user.
/// @param self The Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is USER.
bool engineIsStopped(Engine* self);

// ── Keystroke Processing ─────────────────────────────────────────────────────

/// @brief Process a single key press.
/// @param self The Engine instance.
/// @param key  The character that was pressed.
void engineKeyPress(Engine* self, char key);

/// @brief Process a backspace press (flow mode only; no-op in strict mode).
/// @param self The Engine instance.
void engineBackspacePress(Engine* self);

#endif // ENGINE_H
