CXX = g++

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

