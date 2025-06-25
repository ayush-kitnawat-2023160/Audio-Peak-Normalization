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
using namespace std;

class AudioProcessor {
private:
    vector<float> audio_data;
    SF_INFO sf_info; // SF_INFO is a C struct defined by libsndfile
    string filename;
    ofstream log_file;

public:
    // Constructor
    AudioProcessor(const std::string& file_path, const std::string& log_path = "log.txt");

    // Destructor
    ~AudioProcessor();

    // Member functions
    void log(const std::string& message);
    bool loadAudio();
    void normalizePeak(float target_peak = 1.0f);
    void printStats(const std::string& title);
    bool saveAudio(const std::string& output_filename);
};
