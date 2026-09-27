# Build with mingw32-make (llvm-mingw-20260908-ucrt-x86_64)
SRC_DIR = src

# Targets & Sources
TARGET_DUMMY = dummy.exe
SRC_DUMMY = $(SRC_DIR)\dummy.cpp

TARGET_RUN = run.exe
SRC_RUN = $(SRC_DIR)\run.cpp
LDFLAGS_RUN = -mconsole

TARGET_WRAPPER = run_portable.exe
SRC_WRAPPER = $(SRC_DIR)\wrapper.cpp
RC_WRAPPER = $(SRC_DIR)\wrapper.rc
RES_WRAPPER = $(SRC_DIR)\wrapper.res

all: $(TARGET_WRAPPER)

$(TARGET_DUMMY): $(SRC_DUMMY)
	$(CXX) $(CXXFLAGS) $(SRC_DUMMY) -o $(TARGET_DUMMY)

$(TARGET_RUN): $(SRC_RUN)
	$(CXX) $(CXXFLAGS) $(SRC_RUN) -o $(TARGET_RUN) $(LDFLAGS_RUN)

$(RES_WRAPPER): $(RC_WRAPPER) $(TARGET_DUMMY) $(TARGET_RUN)
	windres $(RC_WRAPPER) -O coff -o $(RES_WRAPPER)

$(TARGET_WRAPPER): $(SRC_WRAPPER) $(RES_WRAPPER)
	$(CXX) $(CXXFLAGS) $(SRC_WRAPPER) $(RES_WRAPPER) -o $(TARGET_WRAPPER) -static -O3

clean:
	del /Q $(TARGET_DUMMY) $(TARGET_RUN) $(RES_WRAPPER) $(TARGET_WRAPPER)