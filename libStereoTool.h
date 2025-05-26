#pragma once
#ifndef _GENERIC_STEREOTOOL_H
#define _GENERIC_STEREOTOOL_H

// Stereo Tool functions for Generic usage
// (C) Thimeo Holding B.V. 2015-2018

#if defined(_ST_LINUX) || defined(_ST_MAC)
#define __EXPORT __attribute__ ((visibility ("default")))
#else //defined(_ST_LINUX) || defined(_ST_MAC)
#define __EXPORT __declspec(dllexport)
#endif //defined(_ST_LINUX) || defined(_ST_MAC)

class gStereoTool;
class gStereoToolGUI;

// Construction and destruction
extern "C" __EXPORT gStereoTool*    stereoTool_Create              (const char* key=NULL);
extern "C" __EXPORT gStereoTool*    stereoTool_Create2             (const bool has_gui, const char* key = NULL);
extern "C" __EXPORT void            stereoTool_Delete              (gStereoTool* st_instance);

// Audio processing
extern "C" __EXPORT void            stereoTool_Process             (gStereoTool* st_instance, float* samples, int32_t numsamples, int32_t channels, int32_t samplerate);
extern "C" __EXPORT void            stereoTool_ProcessFM           (gStereoTool* st_instance, float* samples, float* fm_samples, int32_t numsamples, int32_t channels, int32_t samplerate, int32_t* fm_oversampling, int32_t* samples_fm_size);

// GUI
#ifdef _ST_GUI
extern "C" __EXPORT gStereoToolGUI* stereoTool_GUI_Create          (gStereoTool* st_instance);
extern "C" __EXPORT void            stereoTool_GUI_Show            (gStereoToolGUI* gui, void* hwnd);
extern "C" __EXPORT void            stereoTool_GUI_Hide            (gStereoToolGUI* gui);
extern "C" __EXPORT void            stereoTool_GUI_SetSize         (gStereoToolGUI* gui, int width, int height);
extern "C" __EXPORT void            stereoTool_GUI_Delete          (gStereoToolGUI* gui);
#endif //_ST_GUI

// R128
extern "C" __EXPORT void            stereoTool_SetR128Mode         (gStereoTool* st_instance, int mode, float r128_truepeak, float gain);
extern "C" __EXPORT float           stereoTool_GetR128Gain         (gStereoTool* st_instance, float r128_target);

// Settings
extern "C" __EXPORT bool            stereoTool_LoadPreset          (gStereoTool* st_instance, const char* filename, int loadsave_type);
extern "C" __EXPORT bool            stereoTool_SavePreset          (gStereoTool* st_instance, const char* filename, int loadsave_type);
extern "C" __EXPORT void            stereoTool_Reset               (gStereoTool* st_instance, int loadsave_type);
extern "C" __EXPORT bool            stereoTool_GetBuiltInPresetName(gStereoTool* st_instance, int pos, int* level, bool* is_preset, char* name); // OBSOLETE, returns FALSE
extern "C" __EXPORT bool            stereoTool_SetBuiltInPreset    (gStereoTool* st_instance, int pos);                                          // OBSOLETE, returns FALSE
extern "C" __EXPORT void            stereoTool_SetRdsPs            (gStereoTool* st_instance, const char* texts, bool now);
extern "C" __EXPORT void            stereoTool_SetRdsRt            (gStereoTool* st_instance, bool on, const char* texts, bool now);
extern "C" __EXPORT void            stereoTool_SetRdsTa            (gStereoTool* st_instance, bool tp, bool ta);
extern "C" __EXPORT bool            stereoTool_GetStsValue         (gStereoTool* st_instance, int index, int subindex, const char** value); // warning: Result is only valid until next call or save preset action inside Stereo Tool!
extern "C" __EXPORT bool            stereoTool_SetStsValue         (gStereoTool* st_instance, int index, int subindex, const char* value);

// Meter readout functions
extern "C" __EXPORT void            stereoTool_SetReadLatency      (gStereoTool* st_instance, float latency);
extern "C" __EXPORT bool            stereoTool_ReadMonoLevel       (gStereoTool* st_instance, int id, float DEPREACATED_latency, bool& modified, float& actual, float& gray, float& black, float& median, int& color);
extern "C" __EXPORT bool            stereoTool_ReadStereoLevel     (gStereoTool* st_instance, int id, float DEPREACATED_latency, bool& modified, float actual[2], float gray[2], float black[2], float median[2], int color[2]);
extern "C" __EXPORT void            stereoTool_UpdateSpectrum      (gStereoTool* st_instance);
extern "C" __EXPORT int             stereoTool_GetSpectrum         (gStereoTool* st_instance, int where, int channel, float latency, int bands_per_octave, float first_band_freq, int number_of_elements, float* output_frequencies, float* output_values);

// Behavior checking functions
extern "C" __EXPORT int             stereoTool_GetNofOutputs       (gStereoTool* st_instance);
extern "C" __EXPORT bool            stereoTool_CheckLicenseValid   (gStereoTool* st_instance);
extern "C" __EXPORT bool            stereoTool_GetUnlicensedUsedFeatures(gStereoTool* st_instance, char* text, int text_maxlen);

// Customer specific functions (available to everyone, but probably not useful for most)
extern "C" __EXPORT void            stereoTool_EnableInternalSoundCard(bool enabled); /* MUST BE CALLED BEFORE CREATING ANY STEREO TOOL INSTANCES */
typedef void(*STEREO_TOOL_CALLBACK) (void*, gStereoTool*, void*);

// Version info
extern "C" __EXPORT int             stereoTool_GetSoftwareVersion  ();
extern "C" __EXPORT int             stereoTool_GetApiVersion       ();

// All other functions
extern "C" __EXPORT int             stereoTool_GetLatency2         (gStereoTool* st_instance, int32_t samplerate, bool feed_silence);

// Old functions still available for compatibility
extern "C" __EXPORT int             stereoTool_GetLatency          (gStereoTool* st_instance);
extern "C" __EXPORT void            stereoTool_Wakeup              (gStereoTool* st_instance, int32_t channels, int32_t samplerate);

#endif //_GENERIC_STEREOTOOL_H

// ---------- Construction and destruction

// stereoTool_Create
//   Creates a gStereoTool object. May be called multiple times to create multiple objects.
//   A license key ("<abcdef>") can be passed if the calling program wants to handle licenses.
//   If so, each instance needs a separate license.

// stereoTool_Delete
//   Removes an existing gStereoTool object

// ---------- Processing functions

// stereoTool_Process
//   Processes a block of audio samples.
//   samples is an interleaved (left, right, left, right if using 2 channels) block of samples, of size numsamples * channels.
//   Channels can be 1-8.
//   The output is returned in this same array, and is always the same size as the input.
//   Note that processing causes a delay, so the first output samples will be 0.
//   For live processing this doesn't really matter, but if you're writing the output to a file which needs to be perfectly aligned with the input, use GetLatency2.

// stereoTool_ProcessFM
//   Processes a block of samples, for both streaming/non-FM as FM, simultaneously.
//   samples, numsamples, channels and samplerate are the same as for Process.
//   fm_samples contains samples_fm_size samples of MPX output (1 channel), at a higher sample rate (samplerate * fm_oversampling).
//   The host application needs to make sure to allocate a sufficiently large buffer.
//   Processing is done in separate threads, and due to this the FM output audio is not always available when the function returns.
//   So, it may take more calls before audio is returned, and the amount of returned audio per call can be bigger.
//   The maximum possible output size is: nr_of_channels * (nr_of_input_samples + 6144 * (nr_of_threads + 1)) * 4.
//   (6144 is the maximum processing block size, 4 is the maximum oversampling value).
//   Using the current values, at most 7 threads, it would be: number_of_channels * (nr_of_input_samples + 49152) * 4.
//   If not enough buffer size is allocated, the program may crash.
//   We suggest that you add an assert to verify that the resulting samples_fm_size value is never too big.
//   Important: This function only returns 2 channels if the parameter set is configured to do so! See GetNofOutputs.

// ---------- GUI

// stereoTool_GUI_Create
//   Creates a Stereo Tool GUI window.
//   Each gStereoTool instance can have only one GUI window.
//   Not available in the Linux non-X11 binaries.

// stereoTool_GUI_Show
//   Shows the Stereo Tool GUI window.
//   Windows only: If hwnd is a window handle (not NULL), the window is shown as a subwindow of an existing host application window.

// stereoTool_GUI_Hide
//   Hides the Stereo Tool GUI window.

// stereoTool_GUI_SetSize
//   Sets the size of the Stereo Tool GUI window.

// stereoTool_GUI_Delete
//   Cleans up (completely removes) the Stereo Tool GUI window.

// ---------- R128

// stereoTool_SetR128Mode
//   R128 normalization: Set mode (0 - no R128, 1 - first stage (normal processing), 2 - later (possibly final) stage (clipping only).

// stereoTool_GetR128Gain
//   Returns the calculated gain, needed to reach the target level.
//   Call this after each stage.
//   If this is within 0.1 dB (approx. 0.99-1.01), you're done and don't have to send it through again.
//   Normally, 2 stages (1x mode 1, 1x mode 2) suffices.

// ---------- Settings

// stereoTool_LoadPreset
//   Loads a preset from file.
//   loadsave_type can be (from ParameterEnum.h):
//   * ID_SAVE_TOTALINI     - ALL settings that are in the file, including configuration settings
//   * ID_SAVE_ALLSETTINGS  - ALL settings except configuration settings
//   * ID_SAVE_AUDIOFM      - All audio repair and processing settings, plus FM settings
//   * ID_SAVE_AUDIO        - All audio repair and processing settings (not FM)
//   * ID_SAVE_PROCESSING   - All audio processing (but not repair) settings
//   * ID_SAVE_REPAIR       - Audio repair section settings
//   * ID_SAVE_REPAIR_NOPNR - Audio repair section settings, without the Dehummer settings
//   * ID_SUBLEVEL_PNR      - Only Dehummer settings

// stereoTool_Reset
//   Reset to default settings.
//   For loadsave_type, see LoadPreset.

// stereoTool_GetBuiltInPresetName OBSOLETE, returns FALSE
//   Fetch the name of a built-in preset.
//   This will give the total preset tree (categories, is_preset==false, and actual presets).

// stereoTool_SetBuiltInPreset OBSOLETE, returns FALSE
//   Select a built-in preset.

// stereoTool_SetRdsPs
//   Set RDS PS text

// stereoTool_SetRdsRt
//   Set RDS RT text

// stereoTool_SetRdsTa
//   Set RDS TA value

// stereoTool_GetStsValue
//   Get a value in the same format that's used in the .sts settings file.

// stereoTool_SetStsValue
//   Set a value in the same format that's used in the .sts settings file.

// ---------- Meter readout functions

// stereoTool_SetReadLatency
//   Sets the latency, used for functions that read meters, such as stereoTool_ReadMonoLevel.

// stereoTool_ReadMonoLevel
//   Read a GUI meter with only 1 value
//   Note: DEPRECATED_latency overrides the set latency, but it's better to use the
//         new stereoTool_SetReadLatency function instead, and pass -1 here.

// stereoTool_ReadStereoLevel
//   Read a stereo GUI meter (left/right)
//   Note: DEPRECATED_latency overrides the set latency, but it's better to use the
//         new stereoTool_SetReadLatency function instead, and pass -1 here.

// stereoTool_UpdateSpectrum
//   Updates spectra for GetSpectrum.
//   Call this once, then all the GetSpectrum outputs will be in sync.

// stereoTool_GetSpectrum
//   Returns a spectrum, useful for displaying.

// ---------- Behavior checking functions

// stereoTool_CheckLicenseValid
//   Check if the license is valid for the current settings.
//   Some audio must have been passed through the chain before this works.

// stereoTool_GetUnlicensedUsedFeatures
//   Check if the license is valid for the current settings.
//   Some audio must have been passed through the chain before this works.
//   Returns true with an empty text string if there are no issues, false with
//   a warning message text string listing the unused features if there are issues.

// stereoTool_GetNofOutputs
//   Returns whether 1 or 2 (streaming + FM) outputs are being generated.
//   Some audio must have been passed through the chain before this works.

// ---------- Customer specific functions (available to everyone, but probably not useful for most)

// stereoTool_StartSoundCardThread
//   Starts sound card I/O.
//   Only for the binaries with sound card support (_audio).

// stereoTool_StopSoundCardThread
//   Stops sound card I/O.
//   Only for the binaries with sound card support (_audio).

// ---------- All other functions

// stereoTool_GetLatency2
//   Returns the processing latency (in samples per channel) for the active preset.
//   Only use this directly after startup, it sends some silence through the chain to find out how big the latecy is (if feed_silence it true).
//   Without feed_silence, the return value is only reliable if some audio has been passed through the chain after loading the preset.

// ---------- Version info

// stereoTool_GetSoftwareVersion
//   Gets software version (example: 912).

// stereoTool_GetApiVersion
//   Gets the version of this API. This number is updated when new functions are added.

// ---------- Old functions still available for compatibility

// stereoTool_GetLatency
//   Returns the processing latency (in samples per channel) for the active preset.
//   Deprecated, only works after some audio has been passed through the chain. See GetLatency2.

// stereoTool_Wakeup
//   Sends a few samples of silence through the chain to update some values (latency, among others).
//   Should not be needed anymore, use GetLatency2 instead.

// ***
// *** Simple example program
// ***

// This (untested) code creates a Stereo Tool instance, loads a preset, turns the equalizer on, sends 2 different beep tones through it on the left and right channel, and closes it.

// gStereoTool* st = stereoTool_Create(NULL);
// bool r = stereoTool_LoadPreset(st, "C:\\temp\\generic_stereo_tool.sts", ID_SAVE_ALLSETTINGS);
// assert(r); // stop if loading preset failed
// stereoTool_SetStsValue(st, PARAM_equalizer_on, 0, "1");
// float smp[1024 * 2];
// float sin_l = 0;
// float sin_r = 0;
// for (int i=0; i<100; i++)
// {
//     for (int s=0; s<1024; s++)
//     {
//         smp[2 * s    ] = sin(sin_l);
//         smp[2 * s + 1] = sin(sin_r);
//         sin_l += .1f;
//         sin_r += .11f;
//     }
//     stereoTool_Process(st, smp, 1024, 2, 44100);
// }
// stereoTool_Delete(st);
