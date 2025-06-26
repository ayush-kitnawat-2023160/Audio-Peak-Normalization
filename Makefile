# Compiler
CXX = g++

# Linker flags
LDFLAGS = -lsndfile -pthread

# Source directory
SRCDIR = src

# Binary (executable) directory
BINDIR = bin

# The name of the executable
TARGET_NAME = audio_processor 

# Full path to the executable
TARGET = $(BINDIR)/$(TARGET_NAME)

# The source file(s)
SRCS = $(SRCDIR)/main.cpp # Expects main.cpp in the src folder


OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(SRCS)) # If you only have one .cpp, this simplifies

# Default target: builds the executable
all: $(BINDIR) $(TARGET)

# Rule to create the binary directory if it doesn't exist
$(BINDIR):
	@mkdir -p $(BINDIR)
	@echo "Created binary directory: $(BINDIR)"

# Rule to build the executable from the source file(s)

$(TARGET): $(SRCS)
	@echo "Compiling and linking..."
	$(CXX) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful! Executable located at: $(TARGET)"

# Rule to execute the compiled file
run: all
	@echo "--- Running the Audio Normalization Tool ---"
	# Clean up previous log and output directory for a fresh run
	@rm -f log.txt
	@rm -rf normalised_audio
	@touch log.txt # Create a fresh log file
	@mkdir -p normalised_audio # Create a fresh output directory
	@echo "Running: ./$(TARGET) audio normalised_audio 0.1"
	# Execute the program, pass arguments: input_dir output_dir target_peak
	./$(TARGET) audio normalised_audio 0.1
	@echo "--- Run finished ---"
	@echo "Check 'log.txt' for details and 'normalised_audio/' for output."

# Rule to clean up compiled files, executable, and generated directories/logs
clean:
	@echo "--- Cleaning project ---"
	@rm -rf $(BINDIR) # Remove the bin directory
	@rm -rf normalised_audio # Remove the output audio directory
	@rm -f log.txt # Remove the log file
	@rm -f $(SRCDIR)/*.o # Remove any stray object files if they were created in src
	@echo "Cleaned build directory, output audio, and log file."

# Phony targets: ensure that 'all', 'clean', and 'run' are not actual file names
.PHONY: all clean run $(BINDIR)

