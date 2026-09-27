# Build with mingw32-make (llvm-mingw-20260908-ucrt-x86_64)
SRC_DIR = src

# Targets
TARGET_DUMMY = dummy.exe
SRC_DUMMY = $(SRC_DIR)/dummy.cpp

TARGET_RUN = run.exe
SRC_RUN = $(SRC_DIR)/run.cpp
LDFLAGS_RUN = -mconsole

all: $(TARGET_DUMMY) $(TARGET_RUN)

$(TARGET_DUMMY): $(SRC_DUMMY)
	$(CXX) $(CXXFLAGS) $(SRC_DUMMY) -o $(TARGET_DUMMY)

$(TARGET_RUN): $(SRC_RUN)
	$(CXX) $(CXXFLAGS) $(SRC_RUN) -o $(TARGET_RUN) $(LDFLAGS_RUN)

clean:
	del /Q $(TARGET_DUMMY) $(TARGET_RUN)