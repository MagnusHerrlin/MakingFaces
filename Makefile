CXX      := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -I. $(shell sdl2-config --cflags) $(shell pkg-config --cflags opencv4)
LDFLAGS  := $(shell sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lpthread $(shell pkg-config --libs opencv4)

TARGET   := MakingFaces
OBJS     := main.o eye_functions.o face_engine.o animation_engine.o audio_engine.o stage_engine.o stage_behaviors.o stage_actors.o
PACKS    := pack_people pack_misc pack_monkeys pack_cats pack_halloween

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

all: $(TARGET) $(PACKS)

main.o: main.cpp main.h tft_compat.h eye_functions.h face_engine.h animation_engine.h audio_engine.h stage_engine.h stage_behaviors.h stage_actors.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

eye_functions.o: eye_functions.cpp eye_functions.h main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

face_engine.o: face_engine.cpp face_engine.h eye_functions.h main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

animation_engine.o: animation_engine.cpp animation_engine.h main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

audio_engine.o: audio_engine.cpp audio_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

stage_engine.o: stage_engine.cpp stage_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

stage_behaviors.o: stage_behaviors.cpp stage_behaviors.h stage_engine.h audio_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

stage_actors.o: stage_actors.cpp stage_actors.h stage_behaviors.h stage_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

pack_monkeys: pack_monkeys.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_monkeys.cpp pack_utils.cpp $(LDFLAGS)

pack_cats: pack_cats.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_cats.cpp pack_utils.cpp $(LDFLAGS)

pack_halloween: pack_halloween.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_halloween.cpp pack_utils.cpp $(LDFLAGS)

pack_misc: pack_misc.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_misc.cpp pack_utils.cpp $(LDFLAGS)

pack_people: pack_people.cpp pack_utils.cpp pack_utils.h
	$(CXX) $(CXXFLAGS) -o $@ pack_people.cpp pack_utils.cpp $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) $(PACKS)

.PHONY: all clean
