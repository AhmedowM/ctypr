#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Engine Engine;
typedef enum EngineMode { StrictMode, FlowMode, UnknownMode } EngineMode;

// Lifecycle

/// @brief Create a new Engine instance
/// @return A pointer to the newly created Engine instance
// Engine* engineCreate();

/// @brief Create a new Engine instance
/// @param mode Desired engine mode
/// @param timeout Desired engine timeout in seconds
/// @return A pointer to the newly created Engine instance
Engine* engineCreate(EngineMode mode, uint16_t timeout);

/// @brief Destroy an Engine instance
/// @param self The Engine instance to destroy
void engineDestroy(Engine* self);

// Mode

/// @brief Set the engine mode (Strict or Flow)
/// @param self The Engine instance
/// @param mode The desired EngineMode (Strict or Flow)
void engineSetMode(Engine* self, EngineMode mode);

/// @brief Get the engine mode
/// @param self The Engine instance
EngineMode engineGetMode(Engine* self);

// Timeout

/// @brief Set the engine timeout
/// @param self The Engine instance
/// @param timeout The desired timeout value
void engineSetTimeout(Engine* self, uint16_t timeout);

/// @brief Get the engine timeout
/// @param self The Engine instance
/// @return The current timeout value
uint16_t engineGetTimeout(Engine* self);

// State

/// @brief Start the engine
/// @param self The Engine instance
void engineStart(Engine* self);

/// @brief Stop the engine
/// @param self The Engine instance
void engineStop(Engine* self);

/// @brief Pause the engine
/// @param self The Engine instance
void enginePause(Engine* self);

/// @brief Resume the engine
/// @param self The Engine instance
void engineResume(Engine* self);

/// @brief Reset the engine
/// @param self The Engine instance
void engineReset(Engine* self);

// State Queries

/// @brief Check if the engine is currently running
/// @param self The Engine instance
/// @return true if the engine state is RUNNING
bool engineIsRunning(Engine* self);

/// @brief Check if the engine is paused
/// @param self The Engine instance
/// @return true if the engine state is PAUSED
bool engineIsPaused(Engine* self);

/// @brief Check if the engine is idle
/// @param self The Engine instance
/// @return true if the engine state is IDLE
bool engineIsIdle(Engine* self);

/// @brief Check if the engine encountered an error
/// @param self The Engine instance
/// @return true if the engine state is ERROR
bool engineIsError(Engine* self);

/// @brief Check if the session completed successfully
/// @param self The Engine instance
/// @return true if the engine is in IDLE state and stop cause is FINISHED
bool engineIsCompleted(Engine* self);

/// @brief Check if the session timed out
/// @param self The Engine instance
/// @return true if the engine is in IDLE state and stop cause is TIMEOUT
bool engineIsTimedOut(Engine* self);

/// @brief Check if the session was stopped by the user
/// @param self The Engine instance
/// @return true if the engine is in IDLE state and stop cause is USER
bool engineIsStopped(Engine* self);

// Process

/// @brief Process key
/// @param self The Engine instance
/// @param key The input key to process
void engineKeyPress(Engine* self, char key);

/// @brief Process backspace
/// @param self The Engine instance
void engineBackspacePress(Engine* self);

#endif // ENGINE_H