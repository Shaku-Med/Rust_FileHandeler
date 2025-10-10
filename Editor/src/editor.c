#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// JavaScript bridge functions
EM_JS(void, js_log, (const char* message), {
    console.log("[C] " + UTF8ToString(message));
});

EM_JS(void, js_update_progress, (int percentage), {
    if (Module.onExportProgress) {
        Module.onExportProgress(percentage);
    }
});

// Video clip structure
typedef struct {
    char filename[256];
    double start_time;
    double end_time;
    double timeline_position;
    int track_index;
} VideoClip;

// Text layer structure
typedef struct {
    char text[512];
    int x;
    int y;
    int font_size;
    char color[32];
    double start_time;
    double end_time;
} TextLayer;

// Effect structure
typedef struct {
    char type[64];  // "crop", "scale", "blur", "fade"
    int param1;     // width, scale, radius, etc.
    int param2;     // height, etc.
    double start_time;
    double end_time;
} Effect;

// Project structure
typedef struct {
    VideoClip clips[10];
    int clip_count;
    
    TextLayer texts[10];
    int text_count;
    
    Effect effects[20];
    int effect_count;
    
    int output_width;
    int output_height;
    int output_fps;
} EditorProject;

// Global project
EditorProject g_project;

// Initialize project
EMSCRIPTEN_KEEPALIVE
void init_project(int width, int height, int fps) {
    memset(&g_project, 0, sizeof(EditorProject));
    g_project.output_width = width;
    g_project.output_height = height;
    g_project.output_fps = fps;
    js_log("Project initialized");
}

// Add video clip
EMSCRIPTEN_KEEPALIVE
int add_video_clip(const char* filename, double start, double end, double timeline_pos, int track) {
    if (g_project.clip_count >= 10) {
        js_log("Maximum clips reached");
        return -1;
    }
    
    VideoClip* clip = &g_project.clips[g_project.clip_count];
    strncpy(clip->filename, filename, sizeof(clip->filename) - 1);
    clip->start_time = start;
    clip->end_time = end;
    clip->timeline_position = timeline_pos;
    clip->track_index = track;
    
    g_project.clip_count++;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Added clip: %s (%.2fs-%.2fs) at %.2fs on track %d", 
             filename, start, end, timeline_pos, track);
    js_log(msg);
    
    return g_project.clip_count - 1;
}

// Add text layer
EMSCRIPTEN_KEEPALIVE
int add_text_layer(const char* text, int x, int y, int font_size, 
                   const char* color, double start, double end) {
    if (g_project.text_count >= 10) {
        js_log("Maximum text layers reached");
        return -1;
    }
    
    TextLayer* layer = &g_project.texts[g_project.text_count];
    strncpy(layer->text, text, sizeof(layer->text) - 1);
    layer->x = x;
    layer->y = y;
    layer->font_size = font_size;
    strncpy(layer->color, color, sizeof(layer->color) - 1);
    layer->start_time = start;
    layer->end_time = end;
    
    g_project.text_count++;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Added text: '%s' at (%d,%d) from %.2fs to %.2fs", 
             text, x, y, start, end);
    js_log(msg);
    
    return g_project.text_count - 1;
}

// Add effect
EMSCRIPTEN_KEEPALIVE
int add_effect(const char* type, int param1, int param2, double start, double end) {
    if (g_project.effect_count >= 20) {
        js_log("Maximum effects reached");
        return -1;
    }
    
    Effect* effect = &g_project.effects[g_project.effect_count];
    strncpy(effect->type, type, sizeof(effect->type) - 1);
    effect->param1 = param1;
    effect->param2 = param2;
    effect->start_time = start;
    effect->end_time = end;
    
    g_project.effect_count++;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Added effect: %s (%d,%d) from %.2fs to %.2fs", 
             type, param1, param2, start, end);
    js_log(msg);
    
    return g_project.effect_count - 1;
}

// Build FFmpeg filter complex string
void build_filter_complex(char* filter_buffer, size_t buffer_size) {
    char temp[4096];
    int filter_idx = 0;
    
    filter_buffer[0] = '\0';
    
    // Process each video clip
    for (int i = 0; i < g_project.clip_count; i++) {
        VideoClip* clip = &g_project.clips[i];
        
        // Trim clip
        snprintf(temp, sizeof(temp), 
                "[%d:v]trim=start=%.2f:end=%.2f,setpts=PTS-STARTPTS[v%d];",
                i, clip->start_time, clip->end_time, filter_idx);
        strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
        filter_idx++;
    }
    
    // Apply effects
    for (int i = 0; i < g_project.effect_count; i++) {
        Effect* effect = &g_project.effects[i];
        
        if (strcmp(effect->type, "crop") == 0) {
            snprintf(temp, sizeof(temp),
                    "[v%d]crop=%d:%d[v%d];",
                    filter_idx - 1, effect->param1, effect->param2, filter_idx);
            strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
            filter_idx++;
        }
        else if (strcmp(effect->type, "scale") == 0) {
            snprintf(temp, sizeof(temp),
                    "[v%d]scale=%d:%d[v%d];",
                    filter_idx - 1, effect->param1, effect->param2, filter_idx);
            strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
            filter_idx++;
        }
        else if (strcmp(effect->type, "blur") == 0) {
            snprintf(temp, sizeof(temp),
                    "[v%d]boxblur=%d[v%d];",
                    filter_idx - 1, effect->param1, filter_idx);
            strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
            filter_idx++;
        }
    }
    
    // Add text layers
    for (int i = 0; i < g_project.text_count; i++) {
        TextLayer* text = &g_project.texts[i];
        
        snprintf(temp, sizeof(temp),
                "[v%d]drawtext=text='%s':x=%d:y=%d:fontsize=%d:fontcolor=%s:"
                "enable='between(t,%.2f,%.2f)'[v%d];",
                filter_idx - 1, text->text, text->x, text->y, 
                text->font_size, text->color,
                text->start_time, text->end_time, filter_idx);
        strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
        filter_idx++;
    }
    
    // Concatenate clips if multiple
    if (g_project.clip_count > 1) {
        strncat(filter_buffer, "[v0]", buffer_size - strlen(filter_buffer) - 1);
        for (int i = 1; i < g_project.clip_count; i++) {
            snprintf(temp, sizeof(temp), "[v%d]", i);
            strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
        }
        snprintf(temp, sizeof(temp), "concat=n=%d:v=1:a=0[vout]", g_project.clip_count);
        strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
    } else {
        snprintf(temp, sizeof(temp), "[v%d]copy[vout]", filter_idx - 1);
        strncat(filter_buffer, temp, buffer_size - strlen(filter_buffer) - 1);
    }
}

// Export project to HLS
EMSCRIPTEN_KEEPALIVE
const char* export_to_hls(int segment_duration, const char* quality) {
    static char command[8192];
    char filter_complex[4096];
    
    js_log("Building FFmpeg command...");
    js_update_progress(10);
    
    // Build filter complex
    build_filter_complex(filter_complex, sizeof(filter_complex));
    
    // Start command
    strcpy(command, "ffmpeg");
    
    // Add input files
    for (int i = 0; i < g_project.clip_count; i++) {
        char input[512];
        snprintf(input, sizeof(input), " -i %s", g_project.clips[i].filename);
        strcat(command, input);
    }
    
    // Add filter complex
    strcat(command, " -filter_complex \"");
    strcat(command, filter_complex);
    strcat(command, "\"");
    
    // Map output
    strcat(command, " -map [vout]");
    
    // Video codec settings based on quality
    if (strcmp(quality, "high") == 0) {
        strcat(command, " -c:v libx264 -preset medium -crf 18");
    } else if (strcmp(quality, "medium") == 0) {
        strcat(command, " -c:v libx264 -preset fast -crf 23");
    } else {
        strcat(command, " -c:v libx264 -preset veryfast -crf 28");
    }
    
    // HLS settings
    char hls_opts[256];
    snprintf(hls_opts, sizeof(hls_opts),
            " -hls_time %d -hls_list_size 0 -hls_segment_filename segment%%03d.ts -f hls output.m3u8",
            segment_duration);
    strcat(command, hls_opts);
    
    js_log("FFmpeg command ready");
    js_update_progress(20);
    
    return command;
}

// Get project info
EMSCRIPTEN_KEEPALIVE
int get_clip_count() {
    return g_project.clip_count;
}

EMSCRIPTEN_KEEPALIVE
int get_text_count() {
    return g_project.text_count;
}

EMSCRIPTEN_KEEPALIVE
int get_effect_count() {
    return g_project.effect_count;
}

// Clear project
EMSCRIPTEN_KEEPALIVE
void clear_project() {
    memset(&g_project, 0, sizeof(EditorProject));
    js_log("Project cleared");
}