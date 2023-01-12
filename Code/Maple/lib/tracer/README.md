# Tracer

A C / C++ *nix / Windows Stack trace generator.

Full Doxygen documentation: https://mensinda.github.io/tracer/html/index.html

## Build Status

| OS      | Status |
|---------|--------|
| Linux   | [![Build Status](https://travis-ci.org/mensinda/tracer.svg?branch=master)](https://travis-ci.org/mensinda/tracer) |
| Windows | [![Build status](https://ci.appveyor.com/api/projects/status/lwnc7tv8qy2af7ck?svg=true)](https://ci.appveyor.com/project/mensinda/tracer) |

# About the C wrapper

Tracer is written in C++11, but has an integrated C wrapper. All (supported) C++ class objects can be acquired with
`tr_get<ClassName>` functions and must be destroyed with `tr_free<ClassName>` functions.

All (supported) methodes can then be called with `tr_<ClassName>__<functionName>`.

The doxygen generated documentation can be found here: https://mensinda.github.io/tracer/html/tracer_8h.html

# Usage

## Simple setup

This will print a stack trace using the default Printer (FancyPrinter) and the default
Config (TracerHandler::Config)

```cpp
TracerHandler::getTracer()->defaultSetup();
```

or in C:

```c
tr_defaultSetup();
```

This will generate the following output for the segFaultTest (in test/segFaultTest):

![Tracer Screenshot][sc1]

## More advanced setup

With this setup it is possible to customize the output and signal handler.

```cpp
auto *tHandler = TracerHandler::getTracer(); // Get the singleton
auto  cfg      = tHandler->getConfig();      // Get the current config
// Edit cfg

tHandler->setConfig(cfg);                    // Update the config
auto printer = PrinterContainer::fancy();    // Generates a printer
// Edit printer config

tHandler->setup(std::move(printer));         // Sets things up. Now the signal handler is setup
```

or using C:

```c
tr_TracerHandler_t *      trTH = tr_getTracerHandler();             // Get the handler
tr_TracerHandler_Config_t cfg  = tr_TracerHandler__getConfig(trTH); // get the current config

// edit cfg
tr_TracerHandler__setConfig(trTH, cfg);                             // Update the config

tr_Printer_t *systemPrinter = tr_getPrinter__system();              // Generates a printer
// Edit printer config
tr_TracerHandler__setup(trTH, systemPrinter);                       // Sets things up. Now the signal handler is setup

// Do some cleanup
tr_freePrinter(systemPrinter);
tr_freeTracerHandler(trTH);
```

# Status

## Supported Operating Systems

 - Linux
 - Windows

It currently does not work on Mac OS because of ASLR. Pull requests are welcome :D

FreeBSD is currently not tested.

## Backend status

|   OS    | Backend            | Type | Status             |
|---------|--------------------|------|--------------------|
| Windows | WIN32              | `TB` | :heavy_check_mark: |
| Linux   | libunwind          | `T-` | :heavy_check_mark: |
|         | glibc bactrace     | `T-` | :heavy_check_mark: |
|         | libelf / elfutils  | `-D` | :heavy_check_mark: |
|         | libbfd / binutils  | `-D` | :heavy_check_mark: |
|         | addr2line fallback | `-D` | :warning:          |

Types:
  - `T-` Stack trace generator
  - `-D` Debug information extractor
  - `TD` Both

Status:
  - :heavy_check_mark: works and testet
  - :warning: works but not recommended
  - :heavy_exclamation_mark: broken

[sc1]: https://raw.githubusercontent.com/mensinda/tracer/master/docs/screenshot1.png
