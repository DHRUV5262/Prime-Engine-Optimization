# Prime-Engine-Optimization


A highly optimized game engine project focusing on core systems, physics integration, and thread management. This repository contains the source code for the **Prime Engine**, featuring advanced optimizations for game loops and component management.

## 🚀 Features

### Core Systems
- **Partial Body Physics**: Custom implementation of partial body physics for lightweight collision responses.
- **Physics Components**: Modular physics component architecture allowing for flexible entity behaviors.
- **Thread Management**: 
  - Dedicated **Game Thread** for logic updates.
  - Separate **Render Thread** for drawing operations.
  - Job system for parallel task execution.

### Character & Gameplay
- **Character Controller**: Robust character movement and interaction logic located in the `Character` folder.
- **Game Scenes**: Pre-configured scenes demonstrating engine capabilities.
- **Feature Set**: Comprehensive set of features currently active in the game build.

## 📂 Project Structure

- **`Code/`**: Main source code for the engine.
  - **`PartialBody/`**: Implementation of partial rigid body physics.
  - **`PhysicsComponent/`**: Physics component logic and integration.
  - **`Character/`**: Character controller, animation states, and input handling.
  - **`GameThreads/`**: Logic for multi-threaded game loop execution.

## 🛠️ Build Instructions

*(Add instructions here on how to build the project, e.g., using CMake or Visual Studio)*

1. Clone the repository:
   ```bash
   git clone https://github.com/DHRUV5262/Prime-Engine-Optimization.git
   ```
2. Open the solution in your IDE.
3. Build for **Release** configuration for maximum performance.

## 🤝 Contributing

Contributions are welcome! Please fork the repository and submit a pull request for any optimizations or feature additions.

## 📄 License

*(Add your license here, e.g., MIT, Apache 2.0)*
