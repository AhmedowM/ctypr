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

/// @brief Typing modes controlling keystroke behavior.
typedef enum EngineMode {
    StrictMode,  ///< Wrong keys block progress; must type the correct key
    FlowMode,    ///< Wrong keys advance cursor; backspace allowed
    UnknownMode  ///< Sentinel for uninitialized/invalid mode
} EngineMode;

/// @brief Engine creation configuration.
typedef struct EngineConfig {
    EngineMode mode;                   ///< Required: StrictMode or FlowMode
    uint16_t timeout;                  ///< Session timeout in seconds (0 = no limit)
    ContentProvider* contentProvider;  ///< Required: provides session text
    Repository* autoSaveRepo;          ///< Optional: repository for auto-save
    bool autoSaveEnabled;              ///< Whether auto-save is active
} EngineConfig;

/// @brief Thread safety: Engine is NOT thread-safe. Callers must provide
///        external synchronization when using the same instance from multiple threads.

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Create an Engine instance.
/// @param config Configuration (must not be NULL; mode and contentProvider required).
/// @return New Engine pointer, or NULL on allocation failure.
Engine* engineCreate(const EngineConfig* config);

/// @brief Destroy an Engine and free all resources.
/// @param self Engine instance (NULL is safe).
void engineDestroy(Engine* self);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a logger for diagnostic output. Propagates to content provider and repository.
/// @param self   Engine instance.
/// @param logger Logger to attach (NULL to detach).
void engineSetLogger(Engine* self, Logger* logger);

// ── Content Provider ─────────────────────────────────────────────────────────

/// @brief Replace the content provider.
/// @param self     Engine instance.
/// @param provider New content provider (NULL is safe; engineStart will fail if unset).
void engineSetContentProvider(Engine* self, ContentProvider* provider);

// ── Auto-Save ────────────────────────────────────────────────────────────────

/// @brief Configure automatic session persistence.
/// @param self    Engine instance.
/// @param repo    Repository for saving (NULL to disable).
/// @param enabled Whether auto-save is active.
void engineSetAutoSave(Engine* self, Repository* repo, bool enabled);

// ── Mode ─────────────────────────────────────────────────────────────────────

/// @brief Set the typing mode.
/// @param self Engine instance.
/// @param mode StrictMode or FlowMode.
void engineSetMode(Engine* self, EngineMode mode);

/// @brief Get the current typing mode.
/// @param self Engine instance.
/// @return Current mode, or UnknownMode if self is NULL.
EngineMode engineGetMode(Engine* self);

// ── Timeout ──────────────────────────────────────────────────────────────────

/// @brief Set the session timeout.
/// @param self    Engine instance.
/// @param timeout Timeout in seconds (0 = no limit).
void engineSetTimeout(Engine* self, uint16_t timeout);

/// @brief Get the current session timeout.
/// @param self Engine instance.
/// @return Timeout in seconds, or 0 if self is NULL.
uint16_t engineGetTimeout(Engine* self);

// ── Session Control ──────────────────────────────────────────────────────────

/// @brief Start a new typing session. Loads content from the provider.
/// @param self Engine instance.
void engineStart(Engine* self);

/// @brief Stop the current session (user-initiated).
/// @param self Engine instance.
void engineStop(Engine* self);

/// @brief Pause the session, freezing the timer.
/// @param self Engine instance.
void enginePause(Engine* self);

/// @brief Resume a paused session.
/// @param self Engine instance.
void engineResume(Engine* self);

/// @brief Reset engine to idle state, clearing session data.
/// @param self Engine instance.
void engineReset(Engine* self);

/// @brief Manually advance the session timer and check for timeout.
///        Useful in game loops without keystroke input.
/// @param self Engine instance.
void engineTick(Engine* self);

// ── State Queries ────────────────────────────────────────────────────────────

/// @brief Check if engine is running.
/// @param self Engine instance.
/// @return true if state is ENGINE_RUNNING.
bool engineIsRunning(Engine* self);

/// @brief Check if engine is paused.
/// @param self Engine instance.
/// @return true if state is ENGINE_PAUSED.
bool engineIsPaused(Engine* self);

/// @brief Check if engine is idle (not running, no stop cause).
/// @param self Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is NONE.
bool engineIsIdle(Engine* self);

/// @brief Check if engine is in error state.
/// @param self Engine instance.
/// @return true if state is ENGINE_ERROR.
bool engineIsError(Engine* self);

/// @brief Check if session completed (all text typed).
/// @param self Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is FINISHED.
bool engineIsCompleted(Engine* self);

/// @brief Check if session timed out.
/// @param self Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is TIMEOUT.
bool engineIsTimedOut(Engine* self);

/// @brief Check if session was stopped by user.
/// @param self Engine instance.
/// @return true if state is ENGINE_IDLE and stop cause is USER.
bool engineIsStopped(Engine* self);

/// @brief Check if last stop cause was user-initiated (regardless of current state).
/// @param self Engine instance.
/// @return true if stop cause is USER.
bool engineWasStopped(Engine* self);

// ── Keystroke Processing ─────────────────────────────────────────────────────

/// @brief Process a key press.
/// @param self Engine instance.
/// @param key  Character pressed.
void engineKeyPress(Engine* self, char key);

/// @brief Process a backspace press (FlowMode only; no-op in StrictMode).
/// @param self Engine instance.
void engineBackspacePress(Engine* self);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_H