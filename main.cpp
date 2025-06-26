#include <iostream>
#include <string>
#include <vector>
#include <algorithm> 
#include <queue>     
#include <cmath>     
#include <fstream>
#include <ctime>
#include <sndfile.h>
#include <cstring>    
#include <dirent.h>   
#include <sys/stat.h> 
#include <pthread.h> 
using namespace std; 


pthread_mutex_t global_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_task = PTHREAD_COND_INITIALIZER;     
pthread_cond_t task_done = PTHREAD_COND_INITIALIZER;      
volatile bool stop_threads = false;                     // Flag to signal threads to terminate
int active_task_cnt = 0;                             // Number of tasks currently in the queue

// Global mutex for safe logging to the shared log file
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to pass task data to threads
struct AudioTask {
    string input_filepath;
    string output_filepath;
    string filename; // filename for logging/display
    float peak_level;
};

queue<AudioTask> task_queue;


class AudioProcessor {
private:
    vector<float> audio_data;
    SF_INFO sf_info;
    string filename;
    ofstream log_file; 

public:
    // Constructor initializes the audio processor
    AudioProcessor(const string& file_path, const string& log_path = "log.txt") : filename(file_path) {
        // Initialize SF_INFO struct to all zeros to prevent issues
        memset(&sf_info, 0, sizeof(sf_info));
        
        // Acquire global log mutex before opening/writing to log file to prevent race conditions
        pthread_mutex_lock(&log_mutex);
        log_file.open(log_path, ios::app);
        if (!log_file.is_open()) {
            cerr << "Could not open the log file " << log_path << endl; // Output if log file fails
        } else {
            // Write timestamp and a separator to the log file
            auto now = time(nullptr);
            log_file << "\n========================================\n";
            log_file << "Processing started for " << filename << ": " << ctime(&now);
            log_file << "==========================================\n";
        }
        pthread_mutex_unlock(&log_mutex); // Release mutex
    }

    // Destructor ensures the log file is closed when the object is destroyed
    ~AudioProcessor() {
        // Acquire global log mutex before closing log file
        pthread_mutex_lock(&log_mutex);
        if (log_file.is_open()) {
            auto now = time(nullptr);
            log_file << "\n========================================\n";
            log_file << "Processing Ended for " << filename << ": " << ctime(&now);
            log_file << "\n========================================\n";
            log_file.close();
        }
        pthread_mutex_unlock(&log_mutex); // Release mutex
    }
    
    // Logs messages to the log file.
    void log(const string& message) {
        // Acquire global log mutex before writing to the log file
        pthread_mutex_lock(&log_mutex);
        if (log_file.is_open()) {
            log_file << message << endl;
            log_file.flush(); // Ensure write to file
        }
        pthread_mutex_unlock(&log_mutex); // Release mutex
    }

    // Loads audio data from the specified file
    bool loadAudio() {
        // Open the audio file for reading
        SNDFILE* infile = sf_open(filename.c_str(), SFM_READ, &sf_info);
        if (!infile) {
            log("Error: Cannot open file " + filename);
            log("libsndfile error: " + string(sf_strerror(nullptr)));
            return false;
        }

        // Calculate total samples (frames * channels) and resize audio_data vector
        sf_count_t total_samples = sf_info.frames * sf_info.channels;
        audio_data.resize(total_samples);

        // Read audio frames into the vector.
        sf_count_t read_count = sf_readf_float(infile, audio_data.data(), sf_info.frames);
        if (read_count != sf_info.frames) {
            log("Warning: Read " + to_string(read_count) + " frames, expected " + to_string(sf_info.frames));
        }

        // Close the audio file
        sf_close(infile);
        return true;
    }

    // Normalizes the audio data to the target peak value(default 1.0f)
    void normalizePeak(float target_peak = 1.0f) {
        if (audio_data.empty()) {
            log("Error: No audio data loaded, cannot normalize.");
            return;
        }

        // Find the absolute peak magnitude in the audio data
        float peak_magnitude = 0.0f;
        for (const float& sample : audio_data) {
            peak_magnitude = max(peak_magnitude, abs(sample));
        }

        if (peak_magnitude == 0.0f) {
            log("Warning: Audio contains only silence.");
            return;
        }

        // Calculate the normalization factor and apply it to all samples
        float normalization_factor = target_peak / peak_magnitude;
        
        log("Original peak magnitude: " + to_string(peak_magnitude));
        log("Normalization factor: " + to_string(normalization_factor));

        for (float& sample : audio_data) {
            sample *= normalization_factor;
        }

        log("Peak normalized to " + to_string(target_peak));
    }

    // Prints various statistics about the audio data to the log file
    void printStats(const string& title) {
        if (audio_data.empty()) {
            log("No audio data to print statistics for.");
            return;
        }

        // Find min, max, and overall peak magnitude
        float min_val = *min_element(audio_data.begin(), audio_data.end());
        float max_val = *max_element(audio_data.begin(), audio_data.end());
        float peak = max(abs(min_val), abs(max_val));
        
        // Calculate RMS (Root Mean Square)
        float sum_squares = 0.0f;
        for (const float& sample : audio_data) {
            sum_squares += sample * sample;
        }
        float rms = sqrt(sum_squares / audio_data.size());

        log("\n--- " + title + " ---");
        log("Min value: " + to_string(min_val));
        log("Max value: + " + to_string(max_val));
        log("Peak magnitude: " + to_string(peak));
        log("RMS: " + to_string(rms));
        // Avoid division by zero
        log("Peak-to-RMS ratio: " + to_string(rms > 0 ? peak / rms : 0.0f));
    }

    // Saves the processed audio data to a new file
    bool saveAudio(const string& output_filename) {
        SF_INFO output_info = sf_info; 
        // Set output format to WAV and float.
        output_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        // Open the output file for writing
        SNDFILE* outfile = sf_open(output_filename.c_str(), SFM_WRITE, &output_info);
        if (!outfile) {
            log("Error: Cannot create output file " + output_filename);
            log("libsndfile error: " + string(sf_strerror(nullptr)));
            return false;
        }

        // Write the audio data to the file
        sf_count_t written = sf_writef_float(outfile, audio_data.data(), sf_info.frames);
        if (written != sf_info.frames) {
            log("Warning: Wrote " + to_string(written) + " frames, expected " + to_string(sf_info.frames));
        }

        // Close the output file
        sf_close(outfile);
        log("Saved to: " + output_filename);
        return true;
    }
};

// Helper function to check if a file has an audio extension
bool isAudioFile(const string& filename) {
    string lower_filename = filename;
    transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    return lower_filename.rfind(".wav") != string::npos;
}

// Worker thread function
void* thread_function(void* arg) {
    while (true) {
        AudioTask task;
        pthread_mutex_lock(&global_queue_mutex); // Acquire lock for queue access

        // Wait if the queue is empty AND threads should not stop yet
        while (task_queue.empty() && !stop_threads) {
            pthread_cond_wait(&new_task, &global_queue_mutex);
        }

        // Check if threads should stop and the queue is empty (all tasks processed/distributed)
        if (stop_threads && task_queue.empty()) {
            pthread_mutex_unlock(&global_queue_mutex);
            break; // Exit thread
        }

        task = task_queue.front();
        task_queue.pop();
        pthread_mutex_unlock(&global_queue_mutex); // Release lock


        AudioProcessor processor(task.input_filepath, "log.txt"); 
        
        if (processor.loadAudio()) {
            processor.printStats("Original Stats for " + task.filename);
            processor.normalizePeak(task.peak_level);
            processor.printStats("Normalized Stats for " + task.filename);
            if (processor.saveAudio(task.output_filepath)) {
                pthread_mutex_lock(&log_mutex);
                cout << "Successfully processed and saved: " << task.output_filepath << endl;
                pthread_mutex_unlock(&log_mutex);
            } else {
                pthread_mutex_lock(&log_mutex);
                cerr << "Failed to save: " << task.output_filepath << endl;
                pthread_mutex_unlock(&log_mutex);
            }
        } else {
            pthread_mutex_lock(&log_mutex);
            cerr << "Failed to load audio" << task.input_filepath << endl;
            pthread_mutex_unlock(&log_mutex);
        }

        // Decrement active task count and signal if all tasks are done
        pthread_mutex_lock(&global_queue_mutex);
        active_task_cnt--;
        if (active_task_cnt == 0 && task_queue.empty()) {
            pthread_cond_signal(&task_done); 
        }
        pthread_mutex_unlock(&global_queue_mutex);
    }
    return nullptr;
}


int main(int argc, char* argv[]) {

    string input_dir_path = argv[1];
    string output_dir_path = argv[2];
    float peak_level = (argc > 3) ? stof(argv[3]) : 1.0f;

    cout << "Processing audio files from: " << input_dir_path << endl;
    cout << "Saving normalized files to: " << output_dir_path << endl;
    cout << "Target peak level: " << peak_level << endl;

    // Check if input_dir_path is a directory
    struct stat sb;
    if (stat(input_dir_path.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        cerr << "Error: Input path '" << input_dir_path << "' is not a valid directory." << endl;
        return 1;
    }

 
 
    int num_threads = 4; // Number of threads
    vector<pthread_t> threads(num_threads);

    DIR *dir;
    struct dirent *ent;
    
    // Populate the task queue
    if ((dir = opendir(input_dir_path.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string filename = ent->d_name;
            if (filename == "." || filename == "..") {
                continue;
            }

            string full_input_path = input_dir_path + "/" + filename;
            string full_output_path = output_dir_path + "/normalised_" + filename; // Prefix output files

            struct stat file_sb;
            if (stat(full_input_path.c_str(), &file_sb) == 0 && S_ISREG(file_sb.st_mode) && isAudioFile(filename)) {
                pthread_mutex_lock(&global_queue_mutex); // Protect queue access while adding tasks
                task_queue.push({full_input_path, full_output_path, filename, peak_level});
                active_task_cnt++; // Increment count of tasks to process
                pthread_mutex_unlock(&global_queue_mutex);
            }
        }
        closedir(dir);
    } else {
        cerr << "Error: Could not open directory " << input_dir_path << endl;
        perror("opendir");
        return 1;
    }

    if (active_task_cnt == 0) {
        cout << "No audio files found to process." << endl;
        return 0;
    }

    // Create worker threads
    for (int i = 0; i < num_threads; ++i) {
        if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
            cerr << "Error: Could not create thread " << i << endl;
            return 1;
        }
    }

    // Signal worker threads that tasks are available
    pthread_cond_broadcast(&new_task); // Wake up all waiting workers

    // Wait for all tasks to be completed
    pthread_mutex_lock(&global_queue_mutex);
    // Wait until active_task_cnt drops to 0 
    while (active_task_cnt > 0) { 
        pthread_cond_wait(&task_done, &global_queue_mutex);
    }
    pthread_mutex_unlock(&global_queue_mutex);

    // Signal worker threads to stop and join them
    pthread_mutex_lock(&global_queue_mutex);
    stop_threads = true;
    pthread_cond_broadcast(&new_task); // Wake up any threads still waiting for tasks
    pthread_mutex_unlock(&global_queue_mutex);

    // Join all threads to ensure they complete execution
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Destroy mutexes and condition variables to release resources
    pthread_mutex_destroy(&global_queue_mutex);
    pthread_cond_destroy(&new_task);
    pthread_cond_destroy(&task_done);
    pthread_mutex_destroy(&log_mutex);

    return 0;
}

