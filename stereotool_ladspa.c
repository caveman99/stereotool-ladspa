/* stereotool_ladspa.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <ladspa.h>

#ifdef __cplusplus
extern "C" {
#endif
void* StereoTool_Create(int sampleRate);
void StereoTool_Destroy(void* handle);
bool StereoTool_LoadPreset(void* handle, const char* filename);
void StereoTool_ProcessFloat(void* handle,
                             float* inL, float* inR,
                             float* outL, float* outR,
                             float* scratch,
                             unsigned long count,
                             int sampleRate
                             );
#ifdef __cplusplus
}
#endif

#define STEREO_TOOL_UNIQUE_ID 5941
#define STEREO_TOOL_LABEL "stereotool"
#define STEREO_TOOL_NAME "StereoTool LADSPA Wrapper"
#define STEREO_TOOL_MAKER "Thomas Göttgens"
#define STEREO_TOOL_COPYRIGHT "GPLv3"

enum {
    PORT_AUDIO_INPUT_L,
    PORT_AUDIO_INPUT_R,
    PORT_AUDIO_OUTPUT_L,
    PORT_AUDIO_OUTPUT_R,
    PORT_COUNT
};

typedef struct {
    float *input_l;
    float *input_r;
    float *output_l;
    float *output_r;
    void *st_handle;
    int sample_rate;
    float *scratch;              /* interleaved L/R scratch buffer */
    unsigned long scratch_frames; /* capacity of scratch, in frames */
} StereoToolHandle;

static LADSPA_Handle instantiate(const LADSPA_Descriptor *desc, unsigned long s_rate) {
    /* calloc so unconnected ports / scratch start as NULL rather than indeterminate. */
    StereoToolHandle *handle = calloc(1, sizeof(StereoToolHandle));
    if (!handle) {
        return NULL;
    }
    handle->sample_rate = (int)s_rate;
    handle->st_handle = StereoTool_Create(handle->sample_rate);
    if (!handle->st_handle) {
        /* Fail instantiation rather than hand back a half-initialised instance whose
           run() would forward a NULL handle into the DSP library. */
        free(handle);
        return NULL;
    }

    Dl_info info;
    if (dladdr((void *)instantiate, &info) && info.dli_fname) {
        char path[PATH_MAX];
        strncpy(path, info.dli_fname, PATH_MAX);
        path[PATH_MAX - 1] = '\0';
        char *dir = dirname(path);

        char preset_path[PATH_MAX];
        snprintf(preset_path, PATH_MAX, "%s/settings.sts", dir);
        fprintf(stderr, "Loading preset from: %s\n", preset_path);
        if (handle->st_handle && preset_path[0]) {
            bool ok = StereoTool_LoadPreset(handle->st_handle, preset_path);
            if (!ok) {
                fprintf(stderr, "Failed to load preset\n");
            }
        }
    }

    return (LADSPA_Handle)handle;
}

static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data) {
    StereoToolHandle *handle = (StereoToolHandle *)instance;
    switch (port) {
        case PORT_AUDIO_INPUT_L:  handle->input_l = data; break;
        case PORT_AUDIO_INPUT_R:  handle->input_r = data; break;
        case PORT_AUDIO_OUTPUT_L: handle->output_l = data; break;
        case PORT_AUDIO_OUTPUT_R: handle->output_r = data; break;
    }
}

static void run(LADSPA_Handle instance, unsigned long sample_count) {
    StereoToolHandle *handle = (StereoToolHandle *)instance;

    /* A well-behaved host connects every port before run(); guard against one that
       does not, so we never dereference an unconnected (NULL) buffer. */
    if (!handle->input_l || !handle->input_r ||
        !handle->output_l || !handle->output_r) {
        return;
    }

    /* Grow the interleave scratch buffer if needed. This only allocates when the block
       size increases (typically once), keeping the steady-state run() allocation-free as
       required by LADSPA_PROPERTY_HARD_RT_CAPABLE. */
    if (sample_count > handle->scratch_frames) {
        /* Overflow-safe size check before the 2*sizeof(float) multiply. */
        if (sample_count > (ULONG_MAX / (2 * sizeof(float)))) {
            return;
        }
        float *buf = realloc(handle->scratch, sample_count * 2 * sizeof(float));
        if (!buf) {
            /* Keep the existing (smaller) buffer and skip this block rather than
               corrupt memory. */
            return;
        }
        handle->scratch = buf;
        handle->scratch_frames = sample_count;
    }

    StereoTool_ProcessFloat(
        handle->st_handle,
        handle->input_l,
        handle->input_r,
        handle->output_l,
        handle->output_r,
        handle->scratch,
        sample_count,
        handle->sample_rate
    );
}

static void cleanup(LADSPA_Handle instance) {
    StereoToolHandle *handle = (StereoToolHandle *)instance;
    StereoTool_Destroy(handle->st_handle);
    free(handle->scratch);
    free(handle);
}

static LADSPA_Descriptor *descriptor = NULL;

static LADSPA_PortDescriptor port_descriptors[PORT_COUNT] = {
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const char *port_names[PORT_COUNT] = {
    "Input L",
    "Input R",
    "Output L",
    "Output R"
};

static LADSPA_PortRangeHint port_range_hints[PORT_COUNT];

__attribute__((constructor))
static void init() {
    descriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
    if (!descriptor) {
        /* Leave descriptor NULL; ladspa_descriptor() then reports no plugins instead
           of crashing the host at library load. */
        return;
    }
    descriptor->UniqueID = STEREO_TOOL_UNIQUE_ID;
    descriptor->Label = STEREO_TOOL_LABEL;
    descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    descriptor->Name = STEREO_TOOL_NAME;
    descriptor->Maker = STEREO_TOOL_MAKER;
    descriptor->Copyright = STEREO_TOOL_COPYRIGHT;

    descriptor->PortCount = PORT_COUNT;
    descriptor->PortDescriptors = port_descriptors;
    descriptor->PortNames = port_names;
    descriptor->PortRangeHints = port_range_hints;

    descriptor->instantiate = instantiate;
    descriptor->connect_port = connect_port;
    descriptor->run = run;
    descriptor->cleanup = cleanup;
    descriptor->run_adding = NULL;
    descriptor->set_run_adding_gain = NULL;
    descriptor->deactivate = NULL;
    descriptor->activate = NULL;
}

__attribute__((destructor))
static void fini() {
    free(descriptor);
}

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
    return (index == 0) ? descriptor : NULL;
}
