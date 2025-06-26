# Audio Normalization Tool Overview

This document provides a comprehensive overview of the provided C++ audio processing application, which leverages multithreading to normalize audio files within a specified directory.

## 1. Introduction

This C++ program is designed to automate the process of normalizing audio files (specifically `.wav` files) to a target peak level. It utilizes a thread pool to process multiple audio files concurrently, significantly speeding up the normalization of large batches of files from the *ESC-50: Environmental Sound Dataset* or similar collections. The application also generates a detailed log of its operations to a `log.txt` file.

## 2. Key Components and Functionality

The application is structured around two main parts: the `AudioProcessor` class for individual file processing and a custom thread pool implementation for concurrency.

### 2.1. AudioProcessor Class

The `AudioProcessor` class handles the core audio loading, processing, and saving functionalities for individual audio files:

* **Constructor and Destructor**: Initializes `SF_INFO` to zeros and manages the opening and closing of a dedicated log stream for each `AudioProcessor` instance, ensuring proper timestamped entries in a shared `log.txt` file.
* **Logging**: The `log` method provides a thread-safe way to write messages to the `log.txt` file, using a `pthread_mutex_t` (`log_mutex`) to prevent race conditions when multiple threads attempt to write simultaneously.
* **Audio Loading (`loadAudio`)**: Uses `libsndfile` to load audio data from `.wav` files into a `std::vector<float>`. `libsndfile` automatically handles the conversion from various integer bit depths (e.g., 16-bit PCM) to a floating-point representation (typically in the range of `[-1.0, 1.0]`).
* **Peak Normalization (`normalizePeak`)**: This method first finds the current maximum absolute amplitude (peak) of the loaded audio. It then calculates a scaling factor to adjust all samples so that this peak reaches a specified `target_peak` level (defaulting to `1.0f`).
    * *Edge Case Handling*: Includes a check for silent audio (peak magnitude exactly `0.0f`), in which case normalization is skipped to prevent division by zero.
* **Statistics (`printStats`)**: Logs various audio statistics such as minimum sample value, maximum sample value, peak magnitude, RMS (Root Mean Square), and the peak-to-RMS ratio to the `log.txt` file.
* **Audio Saving (`saveAudio`)**: Saves the processed audio data to a new `.wav` file using `libsndfile`. The output file's format (e.g., WAV, PCM, channels, sample rate) is preserved from the input file's `SF_INFO` struct. `libsndfile` handles the conversion from the internal `float` representation back to the specified output format's bit depth and includes built-in clamping to prevent clipping.

### 2.2. Multithreading (Thread Pool)

The application employs a custom, simple thread pool to handle concurrent audio file processing:

* **Task Queue**: A `std::queue<AudioTask>` named `task_queue` stores `AudioTask` structs. Each `AudioTask` contains the necessary information for processing a single audio file (input path, output path, filename, and target peak level).
* **Worker Threads**: Multiple `pthread_t` threads (defined by the `thread_function`) are created at the start of the program. These threads continuously monitor and pull tasks from the `task_queue`.
* **Synchronization Mechanisms**:
    * `pthread_mutex_t global_queue_mutex`: A mutex that protects access to the `task_queue` and the `active_task_cnt` variable. This prevents race conditions when multiple threads try to add or remove tasks simultaneously.
    * `pthread_cond_t new_task`: A condition variable used to signal worker threads. When new tasks are added to the `task_queue`, the main thread broadcasts on this condition variable, waking up idle worker threads.
    * `pthread_cond_t task_done`: A condition variable used to signal the main thread. When a worker thread completes a task and the `active_task_cnt` becomes zero (meaning all tasks are processed or distributed), it signals on this condition variable, allowing the main thread to proceed with shutdown.
    * `volatile bool stop_threads`: A flag that, when set to `true`, signals all worker threads to gracefully exit their processing loop after completing their current task or if the queue becomes empty.
    * `int active_task_cnt`: A counter that keeps track of the total number of audio files that need to be processed (tasks pushed to the queue). The main thread waits until this count reaches zero before initiating thread shutdown.
    * `pthread_mutex_t log_mutex`: A dedicated mutex (separate from `global_queue_mutex`) specifically for protecting access to the shared `log.txt` file and console output (`std::cout`, `std::cerr`). This ensures that log messages from different threads don't interleave or corrupt the output.

* **Directory Traversal**: The `main` function iterates through a specified input directory, identifies `.wav` files, creates `AudioTask` objects for each, and pushes them onto the `task_queue`.
* **Graceful Shutdown**: The main thread waits for all tasks to be completed. Once done, it sets `stop_threads` to `true` and signals all workers one last time to ensure they exit cleanly. Finally, it joins all worker threads to ensure they have terminated before the program exits, and destroys all mutexes and condition variables to free system resources.

## 3. Dependencies

To compile and run this program, you will need the following libraries:

* **libsndfile**: A C library for reading and writing files containing sampled sound (e.g., WAV, AIFF, FLAC, Ogg/Vorbis). This library abstracts away the complexities of different audio file formats.

    **Installation**:

    * Debian/Ubuntu:

        ```bash
        sudo apt-get install libsndfile1-dev
        ```

    * Fedora/RHEL:

        ```bash
        sudo dnf install libsndfile-devel
        ```

    * macOS (Homebrew):

        ```bash
        brew install libsndfile
        ```

    * Windows:
        Download pre-compiled binaries or build from source (see [libsndfile documentation](https://libsndfile.github.io/libsndfile/)). It's often easier to use package managers like `vcpkg` or development environments like MinGW/MSYS2 for `libsndfile` installation on Windows.

* **Pthreads (POSIX Threads)**: This library provides the necessary functions for creating and managing threads, mutexes, and condition variables.

    * Available by default on most Unix-like systems (Linux, macOS).
    * On Windows, you typically use `pthread-win32` or compile with a MinGW/Cygwin environment that includes pthreads support.

## 4. How to Compile and Run

This section details how to compile the multithreaded program using a `Makefile` and how to run it to process a directory of audio files.

### 4.1. Project Setup

1.  **Create Project Directory**: Create a main directory (e.g., `multithreaded_audio_normalizer`).
2.  **Create Source Directory**: Inside the main directory, create a `src` subdirectory.
3.  **Place Source File**: Save the C++ code (your multithreaded code) as `main.cpp` inside the `src/` directory (e.g., `multithreaded_audio_normalizer/src/main.cpp`).
4. **Create Bin Director**: Inside the main directory, create a `bin` subdirectory that will hold the executable `audio_normalised` file.
4.  **Place Makefile**: Save a `Makefile` that compiles this multithreaded code as `Makefile` in the main directory (e.g., `multithreaded_audio_normalizer/Makefile`).

5.  **Provide Input Audio Directory**: The program expects an input directory containing `.wav` files.
    * For testing with the ESC-50 dataset, place the unzipped dataset's `audio` folder into a `data/` directory within your project (e.g., `multithreaded_audio_normalizer/data/ESC-50-master/audio/`).
    * The program will automatically create the specified output directory if it doesn't exist.
6.  **Run Script**: Now in the `multithreaded_audio_normalizer`directory run ```bash
make clean run``` 

### 4.2. Compile and Run Commands

To compile the program, navigate to the directory containing the source file (`main.cpp`) and use a C++ compiler (e.g. `g++`):

```bash
g++ -o audio_normalizer main.cpp -lsndfile -lpthread
./audio_normalizer audio normalised_audio 0.1 // The value (0.1) it the targeted peak value default it is 1.0 
```
Or
```bash
make clean run
```


## 5. Current Limitations and Future Enhancements

* **Fixed Number of Threads**: The number of worker threads is currently hardcoded in `main.cpp`.
* **Simple Thread Pool**: The thread pool implementation is basic; more advanced features like dynamic thread scaling or task prioritization are not included.


**Future Enhancements:**

* **Dynamic Thread Count**: Allow the number of threads to be configured via command-line arguments.
* **Advanced Thread Pool**: Integrate a more sophisticated thread pool library (e.g., `ThreadPool` from `progschj/ThreadPool` or `boost::asio::thread_pool`) for better management and features.
* **Additional Normalization Methods**: Extend functionality to include RMS normalization, loudness normalization (e.g., EBU R128), or dynamic range compression.
* **CUDA Integration**: For extremely large datasets, porting the core `normalizePeak` function to CUDA kernels could offer significant performance gains by leveraging GPU paralleli