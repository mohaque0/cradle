[//]: # @mainpage Cradle Api Reference

Cradle is a tool for building projects using the components already available on your system if you're building C++ projects. The configuration file is written in C++ and compiled into a binary that builds your project, offering the full power of the C++ language to describe your builds and minimal external tooling. This allows for flexibility when describing complex build processes and easy extensibility using the tools and libraries already available.

### Design

Cradle works by describing the build using C++ and compiling the configuration file into an executable. Running the executable with the specified targets will run the build. To keep that compilation phase simple the Cradle configuration file is a single compilation unit. Cradle is a single header file containing all implementation. Cradle could be extended by "plugins" which would be separate header files that include `cradle.hpp` and provide an API to the configuration file.
