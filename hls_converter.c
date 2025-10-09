#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// JavaScript bridge to call FFmpeg.wasm from C
EM_JS(int, js_convert_to_hls, (const char* input_file, int segment_duration), {
    const inputFile = UTF8ToString(input_file);
    
    // This will be called from JavaScript side where FFmpeg.wasm is loaded
    if (Module.convertToHLS) {
        return Module.convertToHLS(inputFile, segment_duration);
    }
    return -1;
});

EM_JS(void, js_log, (const char* message), {
    console.log(UTF8ToString(message));
});

typedef struct {
    char input_filename[256];
    int segment_duration;
    char output_playlist[256];
} HLSConfig;

EMSCRIPTEN_KEEPALIVE
int convert_video_to_hls(const char* input_file, int segment_duration) {
    char log_msg[512];
    
    snprintf(log_msg, sizeof(log_msg), "C Program: Starting HLS conversion for: %s", input_file);
    js_log(log_msg);
    
    snprintf(log_msg, sizeof(log_msg), "C Program: Segment duration: %d seconds", segment_duration);
    js_log(log_msg);
    
    // Call JavaScript bridge to FFmpeg
    int result = js_convert_to_hls(input_file, segment_duration);
    
    if (result == 0) {
        js_log("C Program: HLS conversion completed successfully");
    } else {
        js_log("C Program: HLS conversion failed");
    }
    
    return result;
}

EMSCRIPTEN_KEEPALIVE
HLSConfig* create_config(const char* input_file, int segment_duration) {
    HLSConfig* config = (HLSConfig*)malloc(sizeof(HLSConfig));
    if (!config) return NULL;
    
    strncpy(config->input_filename, input_file, sizeof(config->input_filename) - 1);
    config->input_filename[sizeof(config->input_filename) - 1] = '\0';
    
    config->segment_duration = segment_duration;
    
    strncpy(config->output_playlist, "output.m3u8", sizeof(config->output_playlist) - 1);
    config->output_playlist[sizeof(config->output_playlist) - 1] = '\0';
    
    return config;
}

EMSCRIPTEN_KEEPALIVE
void free_config(HLSConfig* config) {
    if (config) {
        free(config);
    }
}

EMSCRIPTEN_KEEPALIVE
const char* get_output_playlist(HLSConfig* config) {
    return config ? config->output_playlist : NULL;
}

EMSCRIPTEN_KEEPALIVE
int get_segment_duration(HLSConfig* config) {
    return config ? config->segment_duration : 0;
}

EMSCRIPTEN_KEEPALIVE
void log_message(const char* message) {
    js_log(message);
}