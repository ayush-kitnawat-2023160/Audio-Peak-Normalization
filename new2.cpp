#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sndfile.h>

class AudioProcessor {
private:
    std::vector<float> audio_data;
    SF_INFO sf_info;
    std::string filename;

public:
    AudioProcessor(const std::string& file_path) : filename(file_path) {
        memset(&sf_info, 0, sizeof(sf_info));
    }

    bool loadAudio() {
        SNDFILE* infile = sf_open(filename.c_str(), SFM_READ, &sf_info);
        if (!infile) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            std::cerr << "libsndfile error: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        sf_count_t total_samples = sf_info.frames * sf_info.channels;
        audio_data.resize(total_samples);

        sf_count_t read_count = sf_readf_float(infile, audio_data.data(), sf_info.frames);
        if (read_count != sf_info.frames) {
            std::cerr << "Warning: Read " << read_count << " frames, expected " << sf_info.frames << std::endl;
        }

        sf_close(infile);

        std::cout << "Loaded: " << filename << std::endl;
        std::cout << "Channels: " << sf_info.channels << ", Sample Rate: " << sf_info.samplerate << " Hz" << std::endl;
        std::cout << "Duration: " << static_cast<double>(sf_info.frames) / sf_info.samplerate << " seconds" << std::endl;

        return true;
    }

    void normalizePeak(float target_peak = 1.0f) {
        if (audio_data.empty()) {
            std::cerr << "Error: No audio data loaded" << std::endl;
            return;
        }

        // Find peak magnitude
        float peak_magnitude = 0.0f;
        for (const float& sample : audio_data) {
            peak_magnitude = std::max(peak_magnitude, std::abs(sample));
        }

        if (peak_magnitude == 0.0f) {
            std::cerr << "Warning: Audio contains only silence" << std::endl;
            return;
        }

        // Calculate and apply normalization
        float normalization_factor = target_peak / peak_magnitude;
        
        std::cout << "Original peak: " << peak_magnitude << std::endl;
        std::cout << "Normalization factor: " << normalization_factor << std::endl;

        for (float& sample : audio_data) {
            sample *= normalization_factor;
        }

        std::cout << "Peak normalized to " << target_peak << std::endl;
    }

    bool saveAudio(const std::string& output_filename) {
        SF_INFO output_info = sf_info;
        output_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        SNDFILE* outfile = sf_open(output_filename.c_str(), SFM_WRITE, &output_info);
        if (!outfile) {
            std::cerr << "Error: Cannot create output file " << output_filename << std::endl;
            std::cerr << "libsndfile error: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        sf_count_t written = sf_writef_float(outfile, audio_data.data(), sf_info.frames);
        if (written != sf_info.frames) {
            std::cerr << "Warning: Wrote " << written << " frames, expected " << sf_info.frames << std::endl;
        }

        sf_close(outfile);
        std::cout << "Saved to: " << output_filename << std::endl;
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file> [peak_level]" << std::endl;
        std::cout << "Example: " << argv[0] << " input.wav output.wav 0.9" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    float peak_level = (argc > 3) ? std::stof(argv[3]) : 1.0f;

    AudioProcessor processor(input_file);

    if (!processor.loadAudio()) {
        return 1;
    }

    processor.normalizePeak(peak_level);

    if (!processor.saveAudio(output_file)) {
        return 1;
    }

    std::cout << "Peak normalization completed!" << std::endl;
    return 0;
}