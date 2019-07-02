[//]: # @mainpage Cradle Api Reference

Cradle is a tool for building projects using the components already available on your system if you're building C++ projects. The configuration file is written in C++ and compiled into a binary that builds your project, offering the full power of the C++ language to describe your builds and minimal external tooling. This allows for flexibility when describing complex build processes and easy extensibility using the tools and libraries already available.

### Design

Cradle works by describing the build using C++ and compiling the configuration file into an executable. Running the executable with the specified targets will run the build. To keep that compilation phase simple the Cradle configuration file is a single compilation unit. Cradle is a single header file containing all implementation. Cradle could be extended by "plugins" which would be separate header files that include `cradle.hpp` and provide an API to the configuration file.

All work is organized into tasks represented by a @ref cradle::Task which is easily created by the `cradle::task(...)` function. @ref Task objects have dependency tasks that are specified using `Task::dependsOn(...)` and following tasks specified by `Task::followedBy(...)`.
Every dependent task and its followers must be executed and return `ExecutionResult::SUCCESS` before a given task is executed.

### Extending Cradle

There are a few basic interfaces to keep in mind when extending Cradle:

The @ref cradle::Executor is an interface for execution of tasks. The default implementation is single-threaded, but this could be extended to be parallel or distributed.
It is the responsibility of the @ref cradle::Executor to guarantee that tasks are executed in the right order.

The builder pattern is useful for constructing tasks with many optional and default parameters. @ref cradle_builder.hpp contains a number of abstractions useful for simplifying creating builders.

