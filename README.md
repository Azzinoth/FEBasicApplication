# FEBasicApplication

![build](https://github.com/Azzinoth/FEBasicApplication/actions/workflows/Build.yml/badge.svg?branch=master)

This repository serves as a base layer for applications that utilize OpenGL and ImGui. "FE" in the name stands for "Focal Engine". Originally, it was part of the [Focal Engine project](https://github.com/Azzinoth/FocalEngine/), but it has since been moved here and transformed into a standalone module. This makes it easier to incorporate its functionality into other projects.

## Features

- Setting up GLFW and ImGui.
- Providing unique 24-character IDs upon request (useful for resource identification).
- A basic class for time measurement.
- A thread pool class that forms a solid base for creating a job system.
- A logging system with topic categorization and optional file output.
- An abstraction layer for TCP client and server that hides the stream nature of TCP. This allows for the sending and receiving of distinct messages.
- A profiling subsystem that allows for collecting timestamps of functions and/or sections using predefined macros. It supports multithreading, ensuring accurate profiling when all threads have completed their active work. The system also provides JSON export functionality for external visualization and analysis of the collected profiling data.

## Usage

For a simple example of how to use this module, see the [FEBasicApplication Example](https://github.com/Azzinoth/FEBasicApplication-Example).

To add this module to your project, use the following command:

```bash
git submodule add https://github.com/Azzinoth/FEBasicApplication/
```

If you want to move the submodule to a folder named "SubSystems", for example, use the following command:

```bash
git mv FEBasicApplication SubSystems/
```

## Third Party Licenses

This project uses the following third-party libraries:

1) **GLEW**: This library is licensed under a permissive open-source license, similar to the MIT license. The full license text can be found at [GLEW's GitHub repository](https://github.com/nigels-com/glew/blob/master/LICENSE.txt).

2) **GLFW**: This library is licensed under the zlib License. The full license text can be found at [GLFW's GitHub repository](https://github.com/glfw/glfw/blob/master/LICENSE.md).

3) **Dear ImGui**: This library is licensed under the MIT License. The full license text can be found at [Dear ImGui's GitHub repository](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).
