CXX      := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -I. $(shell sdl2-config --cflags) $(shell pkg-config --cflags opencv4)
LDFLAGS  := $(shell sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lpthread $(shell pkg-config --libs opencv4)

TARGET   := MakingFaces
OBJS     := main.o eye_functions.o face_engine.o macro_engine.o audio_engine.o
PACKS    := pack_default pack_monkeys pack_cats pack_halloween


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.cpp eye_functions.h face_engine.h macro_engine.h audio_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

eye_functions.o: eye_functions.cpp eye_functions.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

face_engine.o: face_engine.cpp face_engine.h eye_functions.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

macro_engine.o: macro_engine.cpp macro_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

audio_engine.o: audio_engine.cpp audio_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

build_packs: $(PACKS)

pack_monkeys: pack_monkeys.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_monkeys.cpp pack_utils.cpp $(LDFLAGS)

pack_cats: pack_cats.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_cats.cpp pack_utils.cpp $(LDFLAGS)

pack_halloween: pack_halloween.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_halloween.cpp pack_utils.cpp $(LDFLAGS)

pack_default: pack_default.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_default.cpp pack_utils.cpp $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) $(PACKS)

.PHONY: all build_packs clean
