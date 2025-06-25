#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <fstream>
#include <ctime>
#include <sndfile.h>
#include <cstring>
using namespace std;

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
            cerr << "Could  not open the log file " << log_path << endl;
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
	    log_file << "Processsing Ended: "<< ctime(&now);
            log_file << "\n========================================\n";
            log_file.close();
        }
    }

    // Logs messages to both standard output and the log file
    void log(const string& message) {
        //cout << message << endl; // Output to console
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
            if (log_file.is_open()) {
                log_file << "libsndfile error: " << sf_strerror(nullptr) << endl;
            }
            cerr << "libsndfile error: " << sf_strerror(nullptr) << endl;
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
            log("Error: No audio data loaded");
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

    // Prints various statistics about the audio data
    void printStats(const string& title ) {
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
            if (log_file.is_open()) {
                log_file << "libsndfile error: " << sf_strerror(nullptr) << endl;
            }
            cerr << "libsndfile error: " << sf_strerror(nullptr) << endl;
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

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <input_file> <output_file> [peak_level]" << endl;
        cout << "Example: " << argv[0] << " input.wav output.wav 0.9" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = argv[2];
    // Convert peak_level from string to float, default to 1.0f if not provided
    float peak_level = (argc > 3) ? stof(argv[3]) : 1.0f;

    // Create an AudioProcessor object
    AudioProcessor processor(input_file);

    // Load audio data; exit if loading fails
    if (!processor.loadAudio()) {
        return 1;
    }

    // Print original audio statistics
    cout << "\nOriginal audio statistics:" << endl;
    processor.printStats("Original Stats");

    // Perform peak normalization
    processor.normalizePeak(peak_level);

    // Print normalized audio statistics
    cout << "\nNormalized audio statistics:" << endl;
    processor.printStats("Normalised Stats");

    // Save the processed audio; exit if saving fails
    if (!processor.saveAudio(output_file)) {
        return 1;
    }

    cout << "Peak normalization completed!" << endl;
    return 0;
}

