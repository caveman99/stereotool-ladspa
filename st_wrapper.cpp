// st_wrapper.cpp

#ifdef __GNUC__
#define __declspec(x)
#endif

#include <cstddef>
#include <cstdint>
#include "ParameterEnum.h"
#include "libStereoTool.h"

extern "C" {

void* StereoTool_Create(int sampleRate) {
    gStereoTool* st = stereoTool_Create(nullptr);
    return static_cast<void*>(st);
}

void StereoTool_Destroy(void* handle) {
    stereoTool_Delete(static_cast<gStereoTool*>(handle));
}

bool StereoTool_LoadPreset(void* handle, const char* filename) {
    return stereoTool_LoadPreset(static_cast<gStereoTool*>(handle), filename, ID_SAVE_ALLSETTINGS);
}

// The interleave scratch buffer (`scratch`, capacity >= count*2 floats) is owned and
// sized by the caller so that no allocation happens on the real-time audio path.
void StereoTool_ProcessFloat(void* handle,
                             float* inL, float* inR,
                             float* outL, float* outR,
                             float* scratch,
                             unsigned long count,
                             int sampleRate) {
    // Never let a C++ exception unwind across this extern "C" boundary into the C caller
    // (that would be undefined behaviour); degrade to silence instead.
    try {
        gStereoTool* st = static_cast<gStereoTool*>(handle);
        const int channels = 2;
        float* buffer = scratch;

        for (unsigned long i = 0; i < count; ++i) {
            buffer[i * 2 + 0] = inL[i];
            buffer[i * 2 + 1] = inR[i];
        }

        stereoTool_Process(st, buffer, count, channels, sampleRate);

        for (unsigned long i = 0; i < count; ++i) {
            outL[i] = buffer[i * 2 + 0];
            outR[i] = buffer[i * 2 + 1];
        }
    } catch (...) {
        for (unsigned long i = 0; i < count; ++i) {
            outL[i] = 0.0f;
            outR[i] = 0.0f;
        }
    }
}

}
