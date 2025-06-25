#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstdint>

struct WAVHeader {
    char riff[4];           // "RIFF"
    uint32_t chunk_size;    // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t channels;      // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
};

class AudioProcessor {
private:
    std::vector<float> audio_data;
    WAVHeader header;
    std::string filename;

public:
    AudioProcessor(const std::string& file_path) : filename(file_path) {}

    bool loadWAV() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }

        // Read WAV header
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        
        // Validate WAV format
        if (std::string(header.riff, 4) != "RIFF" || 
            std::string(header.wave, 4) != "WAVE" ||
            std::string(header.fmt, 4) != "fmt " ||
            std::string(header.data, 4) != "data") {
            std::cerr << "Error: Invalid WAV file format" << std::endl;
            return false;
        }

        // Calculate number of samples
        uint32_t num_samples = header.data_size / (header.bits_per_sample / 8);
        audio_data.resize(num_samples);

        // Read audio data based on bit depth
        if (header.bits_per_sample == 16) {
            std::vector<int16_t> temp_data(num_samples);
            file.read(reinterpret_cast<char*>(temp_data.data()), header.data_size);
            
            // Convert to float [-1.0, 1.0]
            for (size_t i = 0; i < num_samples; ++i) {
                audio_data[i] = static_cast<float>(temp_data[i]) / 32768.0f;
            }
        } else if (header.bits_per_sample == 32) {
            std::vector<int32_t> temp_data(num_samples);
            file.read(reinterpret_cast<char*>(temp_data.data()), header.data_size);
            
            // Convert to float [-1.0, 1.0]
            for (size_t i = 0; i < num_samples; ++i) {
                audio_data[i] = static_cast<float>(temp_data[i]) / 2147483648.0f;
            }
        } else {
            std::cerr << "Error: Unsupported bit depth: " << header.bits_per_sample << std::endl;
            return false;
        }

        file.close();
        
        std::cout << "Loaded: " << filename << std::endl;
        std::cout << "Channels: " << header.channels << std::endl;
        std::cout << "Sample Rate: " << header.sample_rate << " Hz" << std::endl;
        std::cout << "Bit Depth: " << header.bits_per_sample << " bits" << std::endl;
        std::cout << "Duration: " << static_cast<float>(num_samples) / header.sample_rate / header.channels << " seconds" << std::endl;
        
        return true;
    }

    void normalizePeakMagnitude(float target_peak) {
        if (audio_data.empty()) {
            std::cerr << "Error: No audio data loaded" << std::endl;
            return;
        }

        // Find peak magnitude (absolute maximum value)
        float peak_magnitude = 0.0f;
        for (const float& sample : audio_data) {
            peak_magnitude = std::max(peak_magnitude, std::abs(sample));
        }

        if (peak_magnitude == 0.0f) {
            std::cerr << "Warning: Audio contains only silence" << std::endl;
            return;
        }

        // Calculate normalization factor
        float normalization_factor = target_peak / peak_magnitude;
        
        std::cout << "Original peak magnitude: " << peak_magnitude << std::endl;
        std::cout << "Normalization factor: " << normalization_factor << std::endl;

        // Apply normalization
        for (float& sample : audio_data) {
            sample *= normalization_factor;
        }

        std::cout << "Peak normalization completed!" << std::endl;
    }

    bool saveWAV(const std::string& output_filename) {
        std::ofstream file(output_filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create output file " << output_filename << std::endl;
            return false;
        }

        // Write header (unchanged)
        file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

        // Convert float data back to original bit depth and write
        if (header.bits_per_sample == 16) {
            std::vector<int16_t> temp_data(audio_data.size());
            for (size_t i = 0; i < audio_data.size(); ++i) {
                // Clamp to prevent overflow and convert back
                float clamped = std::max(-1.0f, std::min(1.0f, audio_data[i]));
                temp_data[i] = static_cast<int16_t>(clamped * 32767.0f);
            }
            file.write(reinterpret_cast<const char*>(temp_data.data()), header.data_size);
        } else if (header.bits_per_sample == 32) {
            std::vector<int32_t> temp_data(audio_data.size());
            for (size_t i = 0; i < audio_data.size(); ++i) {
                float clamped = std::max(-1.0f, std::min(1.0f, audio_data[i]));
                temp_data[i] = static_cast<int32_t>(clamped * 2147483647.0f);
            }
            file.write(reinterpret_cast<const char*>(temp_data.data()), header.data_size);
        }

        file.close();
        std::cout << "Saved normalized audio to: " << output_filename << std::endl;
        return true;
    }

    void printStats() {
        if (audio_data.empty()) return;

        float min_val = *std::min_element(audio_data.begin(), audio_data.end());
        float max_val = *std::max_element(audio_data.begin(), audio_data.end());
        float peak = std::max(std::abs(min_val), std::abs(max_val));
        
        // Calculate RMS
        float sum_squares = 0.0f;
        for (const float& sample : audio_data) {
            sum_squares += sample * sample;
        }
        float rms = std::sqrt(sum_squares / audio_data.size());

        std::cout << "\n--- Audio Statistics ---" << std::endl;
        std::cout << "Min value: " << min_val << std::endl;
        std::cout << "Max value: " << max_val << std::endl;
        std::cout << "Peak magnitude: " << peak << std::endl;
        std::cout << "RMS: " << rms << std::endl;
        std::cout << "Peak-to-RMS ratio: " << (rms > 0 ? peak/rms : 0) << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_wav_file> <output_wav_file>" << std::endl;
        std::cout << "Example: " << argv[0] << " input.wav normalized_output.wav" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    // Create audio processor
    AudioProcessor processor(input_file);

    // Load the WAV file
    if (!processor.loadWAV()) {
        return 1;
    }

    // Print original statistics
    std::cout << "\nOriginal audio statistics:" << std::endl;
    processor.printStats();

    // Perform peak normalization (normalize to 1.0 by default)
    processor.normalizePeakMagnitude(0.1f);

    // Print normalized statistics
    std::cout << "\nNormalized audio statistics:" << std::endl;
    processor.printStats();

    // Save the normalized audio
    if (!processor.saveWAV(output_file)) {
        return 1;
    }

    std::cout << "\nProcessing completed successfully!" << std::endl;
    return 0;
}
