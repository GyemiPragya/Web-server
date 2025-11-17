#include "page_replacement.h"
#include <stdlib.h>
#include <string.h>

// Utility to copy frames
static void copy_frames(int *dest, const int *src, int n) {
    for (int i = 0; i < n; i++) dest[i] = src[i];
}

// Free allocated memory of steps
void free_frame_states(FrameState* steps, int count) {
    for (int i=0; i < count; i++) {
        free(steps[i].frames);
    }
    free(steps);
}

// FIFO Page Replacement
FrameState* simulate_fifo(const int *refs, int num_refs, int num_frames, int *out_steps) {
    FrameState *steps = malloc(sizeof(FrameState) * num_refs);
    int *frames = malloc(sizeof(int) * num_frames);
    for (int i=0; i<num_frames; i++) frames[i] = -1;
    int cur = 0;
    int contained_count = 0;

    for (int i=0; i<num_refs; i++) {
        int page = refs[i];
        FrameState state;
        state.frames = malloc(sizeof(int) * num_frames);
        state.ref_page = page;
        state.replaced = -1;
        state.num_frames = num_frames;

        int found = 0;
        for (int j=0; j<num_frames; j++) {
            if (frames[j] == page) {
                found = 1;
                break;
            }
        }

        if (!found) {
            state.fault = 1;
            if (contained_count < num_frames) {
                frames[cur] = page;
                cur = (cur + 1) % num_frames;
                contained_count++;
            } else {
                state.replaced = frames[cur];
                frames[cur] = page;
                cur = (cur + 1) % num_frames;
            }
        } else {
            state.fault = 0;
        }
        copy_frames(state.frames, frames, num_frames);
        steps[i] = state;
    }
    free(frames);
    *out_steps = num_refs;
    return steps;
}

// LRU Page Replacement
FrameState* simulate_lru(const int *refs, int num_refs, int num_frames, int *out_steps) {
    FrameState *steps = malloc(sizeof(FrameState) * num_refs);
    int *frames = malloc(sizeof(int) * num_frames);
    int *last_used = malloc(sizeof(int) * num_frames);
    for (int i=0; i<num_frames; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    for (int i=0; i<num_refs; i++) {
        int page = refs[i];
        FrameState state;
        state.frames = malloc(sizeof(int) * num_frames);
        state.ref_page = page;
        state.replaced = -1;
        state.num_frames = num_frames;
        int found = 0;

        for (int j=0; j<num_frames; j++) {
            if (frames[j] == page) {
                found = 1;
                last_used[j] = i;
                break;
            }
        }

        if (!found) {
            state.fault = 1;
            int empty_idx = -1;
            for (int j=0; j<num_frames; j++) {
                if (frames[j] == -1) {
                    empty_idx = j;
                    break;
                }
            }
            if (empty_idx != -1) {
                frames[empty_idx] = page;
                last_used[empty_idx] = i;
            } else {
                int lru_idx = 0;
                int min_time = last_used[0];
                for (int j=1; j<num_frames; j++) {
                    if (last_used[j] < min_time) {
                        min_time = last_used[j];
                        lru_idx = j;
                    }
                }
                state.replaced = frames[lru_idx];
                frames[lru_idx] = page;
                last_used[lru_idx] = i;
            }
        } else {
            state.fault = 0;
        }
        copy_frames(state.frames, frames, num_frames);
        steps[i] = state;
    }
    free(frames);
    free(last_used);
    *out_steps = num_refs;
    return steps;
}

// Optimal Page Replacement
FrameState* simulate_optimal(const int *refs, int num_refs, int num_frames, int *out_steps) {
    FrameState *steps = malloc(sizeof(FrameState) * num_refs);
    int *frames = malloc(sizeof(int) * num_frames);
    for (int i=0; i<num_frames; i++) frames[i] = -1;

    for (int i=0; i<num_refs; i++) {
        int page = refs[i];
        FrameState state;
        state.frames = malloc(sizeof(int) * num_frames);
        state.ref_page = page;
        state.replaced = -1;
        state.num_frames = num_frames;
        int found = 0;

        for (int j=0; j<num_frames; j++) {
            if (frames[j] == page) {
                found = 1;
                break;
            }
        }

        if (!found) {
            state.fault = 1;
            int empty_idx = -1;
            for (int j=0; j<num_frames; j++) {
                if (frames[j] == -1) {
                    empty_idx = j;
                    break;
                }
            }
            if (empty_idx != -1) {
                frames[empty_idx] = page;
            } else {
                int replace_idx = -1;
                int farthest = i;
                for (int j=0; j<num_frames; j++) {
                    int k;
                    for (k=i+1; k<num_refs; k++)
                        if (refs[k] == frames[j]) break;
                    if (k == num_refs) { // not found later
                        replace_idx = j;
                        break;
                    }
                    if (k > farthest) {
                        farthest = k;
                        replace_idx = j;
                    }
                }
                state.replaced = frames[replace_idx];
                frames[replace_idx] = page;
            }
        } else {
            state.fault = 0;
        }
        copy_frames(state.frames, frames, num_frames);
        steps[i] = state;
    }
    free(frames);
    *out_steps = num_refs;
    return steps;
}
