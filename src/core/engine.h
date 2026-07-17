#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

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

// Process

/// @brief Process key
/// @param self The Engine instance
/// @param key The input key to process
void engineKeyPress(Engine* self, char key);

/// @brief Process backspace
/// @param self The Engine instance
void engineBackspacePress(Engine* self);

#endif // ENGINE_H