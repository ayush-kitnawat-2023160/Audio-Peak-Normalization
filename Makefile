# Define the C++ compiler
CXX = g++

# Compiler flags:
# -Wall: Enable all standard warnings
# -Wextra: Enable extra warnings
# -std=c++11: Use C++11 standard (required for some features like std::to_string)
# -O2: Optimization level 2
#CXXFLAGS = -Wall -Wextra -std=c++11 -O2

# Linker flags:
# -lsndfile: Link with the libsndfile library
LDFLAGS = -lsndfile

# The name of the executable
TARGET = audio_processor

# The source file
SRCS = main3.cpp

# Default target: builds the executable
all: $(TARGET)

# Rule to build the executable from the source file
$(TARGET): $(SRCS)
	$(CXX) $(SRCS) -o $(TARGET) $(LDFLAGS)

#Rule to execute the compiled file
run: all
	@rm -f log.txt
	@rm -rf normalised_audio
	@touch log.txt
	@mkdir normalised_audio
	./$(TARGET) audio normalised_audio 0.1

# Rule to clean up compiled files and the executable
clean:
	rm -rf normalised_audio
	rm -f log.txt
	rm -f $(TARGET) *.o

# Phony targets: ensure that 'all' and 'clean' are not actual file names
.PHONY: all clean

