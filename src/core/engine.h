#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;
typedef struct Logger Logger;
typedef struct Repository Repository;
typedef struct ContentProvider ContentProvider;

/// @brief Typing modes that control how keystrokes are processed.
typedef enum EngineMode {
    StrictMode,  ///< Incorrect keys do not advance the cursor; must retry the correct key
    FlowMode,    ///< Incorrect keys still advance the cursor; backspace is supported
    UnknownMode  ///< Sentinel for uninitialized or invalid mode
} EngineMode;

/// @brief Configuration for creating an Engine instance.
typedef struct EngineConfig {
    EngineMode mode;                   ///< Required: typing mode (StrictMode or FlowMode)
    uint16_t timeout;                  ///< Session timeout in seconds (0 = no limit)
    ContentProvider* contentProvider;  ///< Required: content provider for session text
    Repository* autoSaveRepo;          ///< Optional: repository for auto-save (NULL to disable)
    bool autoSaveEnabled;              ///< Whether auto-save is enabled (ignored if repo is NULL)
} EngineConfig;

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Create a new Engine instance with the given configuration.
/// @param config Engine configuration (must not be NULL; mode must be valid;
///               contentProvider must not be NULL).
/// @return A pointer to the newly created Engine, or NULL on failure.
Engine* engineCreate(const EngineConfig* config);

/// @brief Destroy an Engine instance and free all associated resources.
/// @param self The Engine instance to destroy (NULL is safe).
void engineDestroy(Engine* self);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger to the engine for diagnostic output.
///        Propagates to child components (content provider, repository).
/// @param self   The Engine instance.
/// @param logger The Logger instance to attach (NULL to detach).
void engineSetLogger(Engine* self, Logger* logger);

// ── Content Provider ─────────────────────────────────────────────────────────

/// @brief Set or replace the content provider used for session text.
/// @param self     The Engine instance.
/// @param provider The ContentProvider to use (NULL is safe, but engineStart will fail).
void engineSetContentProvider(Engine* self, ContentProvider* provider);

// ── Auto-Save ────────────────────────────────────────────────────────────────

/// @brief Enable or disable automatic session persistence on completion/timeout/stop.
/// @param self    The Engine instance.
/// @param repo    The Repository to save into (can be NULL to clear).
/// @param enabled Whether auto-save is enabled.
void engineSetAutoSave(Engine* self, Repository* repo, bool enabled);

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

/// @brief Check if the engine's last stop cause was user-initiated, regardless of current state.
/// @param self The Engine instance.
/// @return true if stop cause is USER (even after state has changed).
bool engineWasStopped(Engine* self);

// ── Keystroke Processing ─────────────────────────────────────────────────────

/// @brief Process a single key press.
/// @param self The Engine instance.
/// @param key  The character that was pressed.
void engineKeyPress(Engine* self, char key);

/// @brief Process a backspace press (flow mode only; no-op in strict mode).
/// @param self The Engine instance.
void engineBackspacePress(Engine* self);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_H
