cmake_minimum_required(VERSION 3.22)

project(LibraryManager VERSION 1.0.1)

# Windows-only build configuration
# This project is designed to build on Windows with Visual Studio

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory
add_subdirectory(JUCE)

# SQLite3 - Add it as a JUCE module or find it on the system
find_package(SQLite3)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found on system, will use JUCE's built-in SQLite")
endif()

# Find Chromaprint library
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CHROMAPRINT libchromaprint)
    if(CHROMAPRINT_FOUND)
        message(STATUS "Found Chromaprint: ${CHROMAPRINT_LIBRARIES}")
    else()
        message(WARNING "Chromaprint not found - fingerprinting features will be disabled")
    endif()
else()
    message(WARNING "pkg-config not found - cannot detect Chromaprint")
endif()

# Add Tracktion Engine (temporarily disabled due to version compatibility issues)
# add_subdirectory(tracktion_engine/modules)

# Create the GUI application
juce_add_gui_app(LibraryManager
    PRODUCT_NAME "Library Manager"
    COMPANY_NAME "uniQuE-ui"
    BUNDLE_ID "com.uniqueui.librarymanager"
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
    BLUETOOTH_PERMISSION_ENABLED FALSE
    FILE_SHARING_ENABLED TRUE
    DOCUMENT_BROWSER_ENABLED TRUE)

# Add source files
target_sources(LibraryManager
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/DatabaseManager.cpp
        Source/DatabaseManager.h
        Source/FileScanner.cpp
        Source/FileScanner.h
        Source/AnalysisWorker.cpp
        Source/AnalysisWorker.h
        Source/AcoustIDFingerprinter.cpp
        Source/AcoustIDFingerprinter.h
        Source/LibraryTableComponent.cpp
        Source/LibraryTableComponent.h
        Source/PlaylistTreeComponent.cpp
        Source/PlaylistTreeComponent.h
        Source/OnboardingComponent.cpp
        Source/OnboardingComponent.h
        Source/RekordboxExporter.cpp
        Source/RekordboxExporter.h
        Source/BatchMetadataEditor.cpp
        Source/BatchMetadataEditor.h
        Source/WaveformComponent.cpp
        Source/WaveformComponent.h
        Source/AudioPreviewComponent.cpp
        Source/AudioPreviewComponent.h
        Source/SeratoExporter.cpp
        Source/SeratoExporter.h
        Source/TraktorExporter.cpp
        Source/TraktorExporter.h
        Source/CuePointEditorComponent.cpp
        Source/CuePointEditorComponent.h)

# Add binary data (if any resources are added later)
# juce_add_binary_data(LibraryManagerData
#     SOURCES
#         # Add your resource files here
# )

# Link JUCE modules
target_link_libraries(LibraryManager
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Link Tracktion Engine modules (temporarily disabled)
# target_link_libraries(LibraryManager
#     PRIVATE
#         tracktion::tracktion_engine)

# Link SQLite if found on system
if(SQLite3_FOUND)
    target_link_libraries(LibraryManager PRIVATE SQLite::SQLite3)
    target_compile_definitions(LibraryManager PRIVATE USING_SYSTEM_SQLITE=1)
endif()

# Link Chromaprint if found
if(CHROMAPRINT_FOUND)
    target_include_directories(LibraryManager PRIVATE ${CHROMAPRINT_INCLUDE_DIRS})
    target_link_libraries(LibraryManager PRIVATE ${CHROMAPRINT_LIBRARIES})
    target_compile_definitions(LibraryManager PRIVATE HAVE_CHROMAPRINT=1)
endif()

# Compiler definitions
target_compile_definitions(LibraryManager
    PRIVATE
        # JUCE configuration
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LibraryManager,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:LibraryManager,JUCE_VERSION>"
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_DARK_SPLASH_SCREEN=1
        # GPLv3 License mode
        JUCE_USE_GPL_V3=1)

# Set the build output directory
set_target_properties(LibraryManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Platform-specific settings - Windows only
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(LibraryManager PRIVATE NOMINMAX)
    
    # Set Visual Studio startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LibraryManager)
    
    # Multi-processor compilation for MSVC
    if(MSVC)
        target_compile_options(LibraryManager PRIVATE /MP)
        # Set warning level
        target_compile_options(LibraryManager PRIVATE /W4)
    endif()
else()
    message(FATAL_ERROR "This project is configured for Windows-only builds. Please use a Windows system with Visual Studio.")
endif()
