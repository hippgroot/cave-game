# Architecture

## Singletons

The foundation of the app is a set of singletons that control how and when data gets moved

### Thread Bus

The `ThreadBus` object is a singleton that holds `Channel`s which are used for thread-to-thread communication.

`Channel`s are thread-safe queues of a single data type and can be registered and unregistered from the `ThreadBus` whenever needed.

### Job Pool

The `JobPool` object is a singleton that manages the dispatch of `Task`s across a pool of worker threads.

A `Task` has a status, priority, a function that it runs, and a result. When constructed, it immediately adds itself to the `JobPool`. It can have its priority updated, be cancelled, status checked, and its data retrieved.

## Threading

1. Client thread (main thread)
2. Render thread -- always renders one frame behind the main thread
3. Game thread -- a ticked thread (20 ticks per second)

The JobPool allocates the rest of the available hardware concurrency. It will always have a minimum of a single worker thread just in case the game is run on a computer with 3 or less logical cores.

## Client

There are a few jobs of the client:

1. Mediate between the game and the renderer. The client simulates frame N while the renderer renders frame N-1
2. Draw the GUI and handle window input events
3. Handle the connection method between the game and the client (Singleplayer or Multiplayer)

### Screen Stack

The `ScreenStack` draws `Screen`s in stack order (bottom to top).

A `Screen` is a state of GUI. It has a draw function which calls all of the necessary ImGui methods. It also manages its own state of input.

### Connection

The client either has a local connection or a tcp connection. Local connections use the `ThreadBus` to communicate with the game thread. TCP connections serialize data to and from an established peer.

### Settings

The settings that persist across all worlds. Saved to a json file when modifying and read from a json file at the start of the game.

## Game

The `Game` object's sole job is to simulate the game state on a fixed timer state (20 ticks a second default). It receives an Input state from the client and gives back a list of events for the client to process.

## Renderer

The renderer receives a list of added and removed meshes from the client via the `ThreadBus` to add to the render list. it signals through the `ThreadBus` when the client can start processing frame N while it renders frame N-1.
