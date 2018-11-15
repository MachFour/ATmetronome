cmake_minimum_required(VERSION 3.12)
project(metronome)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)
include_directories(SYSTEM /usr/avr/include)


add_compile_definitions(F_CPU=8000000L)
add_compile_definitions(__AVR_ATmega328P__)

add_executable(metronome
        defines.h
        metronome.cpp
        metronome.h
        muxed_7seg.cpp
        muxed_7seg.h
        timers.cpp
        timers.h
        tone_gen.cpp
        tone_gen.h)
