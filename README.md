# TinyRTOS

TinyRTOS is a personal learning project: an educational real-time operating
system that I am building from scratch for the STM32H533RE microcontroller and
the Arm Cortex-M33 architecture. I started this project to move beyond using an
RTOS as a black box and to understand what actually happens between a hardware
interrupt, a scheduling decision, and the execution of the next task.

By implementing each subsystem myself; including context switching, system
calls, task scheduling, timing, dynamic memory allocation, and MPU-based memory
protection; I can explore the design choices, constraints, and failure modes that
are often hidden behind established kernels. The goal is not to replace mature
projects such as FreeRTOS or Zephyr, but to develop a deeper and more practical
understanding of embedded systems and real-time kernel design.

The STM32H533RE is the initial reference platform, but TinyRTOS is being
structured so that architecture-specific code remains isolated from the generic
kernel. A long-term goal is to port the RTOS to additional MCU families and CPU
architectures, making portability itself part of the learning process.

> [!IMPORTANT]
> TinyRTOS is a work in progress and is not intended for production or
> safety-critical systems.

## Project status and release plans

TinyRTOS is still evolving and remains open to architectural improvements,
experimentation, code review, and new ideas. Its APIs and internal design may
change as the kernel becomes more complete and experience from real hardware
reveals better implementation choices.

A public release may be published in the future, but only after the public API is stabilized, the
kernel is covered by broader on-target tests, known timing and concurrency edge
cases are addressed, and the documentation is sufficiently complete for other
developers to use the project reliably. Until then, the repository should be
considered an active development version rather than a stable RTOS release.

## Target platform

| Component | Configuration |
| --- | --- |
| Development board | NUCLEO-H533RE |
| MCU | STM32H533RE |
| CPU | Arm Cortex-M33 |
| Toolchain | GNU Arm Embedded (`arm-none-eabi-gcc`) |
| Build system | CMake 3.22+ and Ninja |
| Language | C11 and Arm assembly |
| Scheduler timer | TIM2, 1 MHz free-running counter |
| Console | USART2, 115200 baud, 8-N-1 |

The linker configuration provides 512 KiB of flash, 208 KiB of user RAM, and a
dedicated 64 KiB kernel RAM region.

## Implemented features

### Scheduling and tasks

- Preemptive, fixed-priority scheduling.
- Round-robin scheduling between ready tasks of equal priority.
- A 1 ms scheduling time slice based on a fixed absolute time grid.
- Preemption when a higher-priority task becomes ready.
- Task creation with independently allocated stacks.
- Ready, running, blocked, suspended, terminated, and unused task states.
- Delay-based blocking and wakeup ordering by absolute deadline.
- Support for timer deadlines spanning multiple 32-bit TIM2 counter cycles.
- A fixed capacity of 16 task control blocks, including the idle task.

Time-slice deadlines advance from the previous `timeslice_end`, not from the
current interrupt timestamp. This preserves the scheduler phase and prevents
interrupt latency from accumulating as timing drift.

### Cortex-M33 port

- SVC-based system-call boundary between unprivileged tasks and the kernel.
- PendSV-based context switching.
- PSP for task contexts and MSP for privileged exception handling.
- Explicit interrupt-priority ordering for TIM2, SVC, USART2, and PendSV.
- MPU regions for flash, user data, user heap, privileged kernel RAM, and the
  currently running task stack.

### Timing

- Microsecond TIM2 counter access.
- Millisecond and structured long-duration task delays.
- Atomic timestamp and decomposed uptime APIs.
- Wrap-aware deadline comparison and timer-cycle tracking.
- Recovery when a requested hardware compare deadline has already passed.

### Memory management

- Separate user and kernel heaps defined by the linker script.
- Best-fit allocator with aligned blocks, splitting, and adjacent-block
  coalescing.
- Dedicated allocator for task stacks.
- 32-byte task-stack alignment for MPU region configuration.
- Privileged memory-allocation system calls for unprivileged tasks.

### Console and utilities

- Interrupt-driven USART2 transmission.
- Fixed-capacity descriptor ring buffer for queued transmissions.
- Lightweight formatted output supporting `%d`, `%u`, `%s`, and `%c`.
- Informational, warning, and error logging helpers.
- Doxygen-documented public and internal interfaces.

## Architecture

```text
Application tasks (unprivileged Thread mode, PSP)
        |
        | TinyRTOS public API
        v
SVC system-call boundary
        |
        v
Kernel services (privileged Handler mode, MSP)
  |-- scheduler and task lists
  |-- timing and deadline management
  |-- heap and stack allocators
  |-- MPU configuration
  `-- console services
        |
        +--> TIM2 compare/update IRQs
        +--> USART2 IRQ
        `--> PendSV context switch
```

The timer always programs the earliest relevant event:

```text
next event = min(next time-slice boundary, earliest task wakeup deadline)
```

When the event expires, the scheduler wakes eligible tasks, advances an expired
time-slice boundary from its previous value, selects the highest-priority ready
task, and requests PendSV only when a context switch is required.

## Repository layout

```text
Application/                 Firmware entry point, interrupts, and on-target tests
Drivers/
  GPIO/                      Board GPIO initialization
  TIMER/                     TIM2 time-base and output-compare driver
  UART/                      USART2 interrupt-driven driver
  CMSIS/                     Arm CMSIS dependencies
  STM32H5xx_HAL_Driver/      STM32 HAL dependencies
Kernel/
  API/                       User-facing TinyRTOS APIs
  Inc/                       Internal kernel interfaces
  Port/CortexM33/            Context switching, exceptions, and MPU port
  Src/                       Scheduler, tasks, timing, syscalls, and allocators
Services/                    Console, logging, and lightweight test framework
Utils/                       Ring buffer, alignment, and string utilities
cmake/                       Toolchain and STM32CubeMX CMake integration
STM32H533xx_FLASH.ld         Production linker script and memory partitioning
startup_stm32h533xx.s        Cortex-M33 startup and vector table
startup_functions.c         Data-copy and BSS initialization
```

## Public API overview

Most application-facing headers are located in `Kernel/API/Inc`. Scheduler
startup is declared in `Kernel/Inc/scheduler.h`.

| Area | Main API |
| --- | --- |
| Tasks | `tiny_task_create`, `tiny_task_suspend`, `tiny_task_resume` |
| Delays | `tiny_delay_ms`, `tiny_long_delay` |
| Time | `tiny_get_timestamp`, `tiny_get_tick`, `tiny_get_uptime` |
| Time arithmetic | `tiny_add_uptime`, `tiny_subtract_uptime`, `tiny_add_duration`, `tiny_subtract_duration` |
| Memory | `tiny_malloc`, `tiny_free` |
| Console | `tiny_print` |
| Scheduler | `tiny_scheduler_start` |

Example:

```c
#include "scheduler.h"
#include "stdout.h"
#include "task.h"
#include "timing.h"

static void worker(void* argument) {
    (void)argument;

    for (;;) {
        tiny_print("worker is running\n");
        tiny_delay_ms(10U);
    }
}

static TinyStatus_t start_application(void) {
    TaskHandle_t worker_handle;

    if (tiny_task_create(&worker_handle, worker, NULL, 2U, 512U) != TINY_OK) {
        return TINY_FAIL;
    }

    return tiny_scheduler_start();
}
```

This example assumes that platform, heap, MPU, and scheduler initialization has
already completed.

Higher numeric values represent higher task priorities. Task entry functions
must not return during normal operation; returning terminates the task through
the kernel task-exit path.

## Building

### Prerequisites

Install the following tools and make them available on `PATH`:

- CMake 3.22 or newer;
- Ninja;
- GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`, `objcopy`, and `size`);
- a flashing/debugging tool compatible with the NUCLEO-H533RE, such as
  STM32CubeProgrammer, STM32CubeIDE, or OpenOCD.

### Debug firmware

```sh
cmake --preset Debug
cmake --build --preset Debug
```

The main outputs are:

```text
build/Debug/firmware.elf
build/Debug/firmware.map
```

### Release firmware

```sh
cmake --preset Release
cmake --build --preset Release
```

Additional `RelWithDebInfo` and `MinSizeRel` presets are available in
`CMakePresets.json`.

## Tests

TinyRTOS uses an on-target unit-test firmware. Configure and build it with:

```sh
cmake --preset Test
cmake --build --preset Test
```

The resulting `build/Test/firmware.elf` must be flashed to the board. Test
results are emitted through USART2. The current test preset builds the scheduler
test suite, including priority selection, round-robin rotation, deadline
comparison, timer rearming, and delay behavior across counter overflow.

## Synchronization roadmap

Synchronization is the next major kernel milestone. The planned work includes:

- atomic task blocking and wakeup primitives shared by kernel wait objects;
- binary and counting semaphores;
- mutex ownership and priority inheritance;
- event flags;
- message queues;
- timeout support integrated with the existing absolute-deadline lists;
- tests for contention, timeout races, wakeup ordering, and priority inversion.

Until these mechanisms are available, applications must avoid unsynchronized
shared mutable state between tasks and must not assume that mutex, semaphore, or
queue semantics are provided by the kernel.

## Portability roadmap

TinyRTOS currently targets the STM32H533RE and Arm Cortex-M33. Future work will
separate the remaining target-specific assumptions behind well-defined port and
driver interfaces, then introduce ports for other MCU families and processor
architectures. Each port will provide its own startup code, context-switching
implementation, interrupt integration, memory map, timer backend, and optional
memory-protection support while reusing the generic scheduler and kernel
services.

## Development workflow

The project uses `clang-format` for C and C++ formatting. Python is used only to
manage the local `pre-commit` hook, with [`uv`](https://docs.astral.sh/uv/) as
the Python package and environment manager. The dependencies are declared in
`pyproject.toml` and locked in `uv.lock`.

```sh
uv sync
uv run pre-commit install
uv run pre-commit run --all-files
```

Keep public interfaces documented with Doxygen comments and build both the
`Debug` and `Test` presets before submitting changes.

## Author and contact

TinyRTOS is designed and developed by **Hamdane BRINI**.

- LinkedIn: [Hamdane BRINI](https://www.linkedin.com/in/hamdane-brini-60b840290/)

## Third-party components

STM32 HAL and CMSIS sources are included under `Drivers/`. They remain subject
to their respective license files distributed in those directories.

> [!NOTE]
> Some project files still contain sections originally generated by STM32CubeMX
> or provided from STM32 project templates. This generated foundation is being
> reviewed and may be progressively refactored as TinyRTOS evolves. **The
> TinyRTOS kernel itself is entirely written from scratch by Hamdane BRINI; no
> kernel code is generated by STM32CubeMX or taken from another RTOS.** Generated
> content is limited to platform initialization, vendor startup support, and parts of the build-system integration.

Files that still contain generated code or generated project structure:

- `Application/Src/main.c`
- `Application/Src/stm32h5xx_hal_msp.c`
- `Application/Src/stm32h5xx_it.c`
- `Application/Src/system_stm32h5xx.c`
- `Application/Inc/main.h`
- `Application/Inc/stm32h5xx_hal_conf.h`
- `Application/Inc/stm32h5xx_it.h`
- `startup_stm32h533xx.s`
- `CMakeLists.txt`
- `cmake/stm32cubemx/CMakeLists.txt`
