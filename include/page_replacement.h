#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

struct FrameState {
    int *frames;    // Array of current frames
    int num_frames; // Number of frames
    int ref_page;   // Current page accessed
    int replaced;   // Page replaced, -1 if none
    int fault;      // 1 if page fault, 0 otherwise
};

typedef struct FrameState FrameState;

FrameState* simulate_fifo(const int *refs, int num_refs, int num_frames, int *out_steps);
FrameState* simulate_lru(const int *refs, int num_refs, int num_frames, int *out_steps);
FrameState* simulate_optimal(const int *refs, int num_refs, int num_frames, int *out_steps);
void free_frames_states(FrameState* steps, int count);

#endif
