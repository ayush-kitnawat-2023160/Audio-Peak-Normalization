#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <fstream>
#include <ctime>
#include <sndfile.h>
#include <cstring>  // For memset
#include <dirent.h> // For directory operations (Unix-like systems)
#include <sys/stat.h> // For checking if a path is a directory (Unix-like systems)

using namespace std; // Using namespace std; for convenience as per user's last provided code

class AudioProcessor {
private:
    vector<float> audio_data;
    SF_INFO sf_info;
    string filename;
    ofstream log_file;

public:
    // Constructor initializes the audio processor
    AudioProcessor(const std::string& file_path, const std::string& log_path = "log.txt") : filename(file_path) {
        // Initialize SF_INFO struct to all zeros to prevent issues
        memset(&sf_info, 0, sizeof(sf_info));
        
        // Open log file in append mode. This ensures previous logs are not overwritten.
        log_file.open(log_path, ios::app);
        if (!log_file.is_open()) {
            cerr << "Could not open the log file " << log_path << endl;
        } else {
            // Write timestamp and a separator to the log file for better organization
            auto now = time(nullptr);
            log_file << "\n========================================\n";
            log_file << "Processing started: " << ctime(&now); // ctime returns a newline
            log_file << "==========================================\n";
        }
    }

    // Destructor ensures the log file is closed when the object is destroyed
    ~AudioProcessor() {
        if (log_file.is_open()) {
            auto now = time(nullptr);
            log_file << "\n========================================\n";
            log_file << "Processing Ended: " << ctime(&now);
            log_file << "\n========================================\n";
            log_file.close();
        }
    }
    
// Logs messages to the log file. Console output is reserved for main's progress.
    void log(const string& message) {
        if (log_file.is_open()) {
            log_file << message << std::endl; // Output to log file
            log_file.flush(); // Ensure immediate write to file
        }
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

        // Read audio frames into the vector. sf_readf_float reads normalized float samples.
        sf_count_t read_count = sf_readf_float(infile, audio_data.data(), sf_info.frames);
        if (read_count != sf_info.frames) {
            log("Warning: Read " + to_string(read_count) + " frames, expected " + to_string(sf_info.frames));
        }

        // Close the audio file
        sf_close(infile);

        // Log file information
        log("Loaded: " + filename);
        log("Channels: " + to_string(sf_info.channels) + ", Sample Rate: " + to_string(sf_info.samplerate) + " Hz");
        log("Duration: " + to_string(static_cast<double>(sf_info.frames) / sf_info.samplerate) + " seconds");

        return true;
    }

    // Normalizes the audio data to a target peak level (default 1.0f)
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
            log("Warning: Audio contains only silence, cannot normalize.");
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
        log("Max value: " + to_string(max_val));
        log("Peak magnitude: " + to_string(peak));
        log("RMS: " + to_string(rms));
        // Avoid division by zero if RMS is very small or zero
        log("Peak-to-RMS ratio: " + to_string(rms > 0 ? peak / rms : 0.0f));
    }

    // Saves the processed audio data to a new file
    bool saveAudio(const std::string& output_filename) {
        SF_INFO output_info = sf_info; // Copy original info
        // Set output format to WAV and float. This ensures high quality output.
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

// Helper function to check if a file has a common audio extension
bool isAudioFile(const string& filename) {
    string lower_filename = filename;
    transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    return lower_filename.rfind(".wav") != string::npos ||
           lower_filename.rfind(".flac") != string::npos ||
           lower_filename.rfind(".ogg") != string::npos ||
           lower_filename.rfind(".aiff") != string::npos ||
           lower_filename.rfind(".mp3") != string::npos; // libsndfile might not support all these directly, but good to filter
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <input_directory> <output_directory> [peak_level]" << endl;
        cout << "Example: " << argv[0] << " audio_inputs normalized_outputs 0.9" << endl;
        return 1;
    }

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

    // Attempt to create output directory if it doesn't exist
    if (stat(output_dir_path.c_str(), &sb) != 0) { // Directory does not exist
        #ifdef _WIN32
            // For Windows, use _mkdir or CreateDirectory
            if (_mkdir(output_dir_path.c_str()) != 0) {
                cerr << "Error: Could not create output directory " << output_dir_path << endl;
                return 1;
            }
        #else
            // For Unix-like systems, use mkdir
            if (mkdir(output_dir_path.c_str(), 0777) != 0) { // 0777 for rwx permissions for all
                cerr << "Error: Could not create output directory " << output_dir_path << endl;
                return 1;
            }
        #endif
        cout << "Created output directory: " << output_dir_path << endl;
    } else if (!S_ISDIR(sb.st_mode)) { // Path exists but is not a directory
        cerr << "Error: Output path '" << output_dir_path << "' exists but is not a directory." << endl;
        return 1;
    }


    DIR *dir;
    struct dirent *ent;
    int files_processed = 0;

    if ((dir = opendir(input_dir_path.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string filename = ent->d_name;
            // Skip current directory (.) and parent directory (..)
            if (filename == "." || filename == "..") {
                continue;
            }

            string full_input_path = input_dir_path + "/" + filename;
            string full_output_path = output_dir_path + "/normalised_" + filename;

            // Check if it's a regular file and has an audio extension
            struct stat file_sb;
            if (stat(full_input_path.c_str(), &file_sb) == 0 && S_ISREG(file_sb.st_mode) && isAudioFile(filename)) {
                cout << "\n--- Processing: " << filename << " ---" << endl;
                AudioProcessor processor(full_input_path, "log.txt"); // Each processor logs to the same file

                if (processor.loadAudio()) {
                    processor.printStats("Original Stats for " + filename);
                    processor.normalizePeak(peak_level);
                    processor.printStats("Normalized Stats for " + filename);
                    if (processor.saveAudio(full_output_path)) {
                        cout << "Successfully processed and saved: " << full_output_path << endl;
                        files_processed++;
                    } else {
                        cerr << "Failed to save: " << full_output_path << endl;
                    }
                } else {
                    cerr << "Failed to load: " << full_input_path << endl;
                }
            }
        }
        closedir(dir);
    } else {
        // Could not open directory
        cerr << "Error: Could not open directory " << input_dir_path << endl;
        perror("opendir"); // Print system error message
        return 1;
    }

    cout << "\nBatch processing completed. Total files processed: " << files_processed << endl;
    return 0;
}

