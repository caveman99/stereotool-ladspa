# Stereo Tool LADSPA Plugin

A LADSPA wrapper around the [Thimeo Stereo Tool](https://www.thimeo.com/stereo-tool/)
DSP library. It exposes Stereo Tool's broadcast audio processing as a stereo
LADSPA plugin usable from hosts such as ecasound, liquidsoap, and Audacity.

## Requirements

- A Linux LADSPA host
- The Stereo Tool shared library for your architecture (`libStereoTool_*.so`)
  and headers, from the
  [Thimeo download page](https://www.thimeo.com/stereo-tool/download/)
- A C/C++ toolchain and the LADSPA SDK headers (`ladspa-sdk`)

## Building

Place the matching `libStereoTool_*.so` and its headers in the source
directory, then build:

```bash
make SO=StereoTool_intel64
```

`SO` is the name of the Stereo Tool library to link against, without the
`lib` prefix or `.so` suffix. Use the value for your architecture, for
example `StereoTool_intel64` (x86-64) or `StereoTool_noX11_arm64` (ARM64).
The build produces `stereotool_ladspa.so`.

## Installation

Install the Stereo Tool library where the dynamic loader can find it:

```bash
sudo cp libStereoTool_intel64.so /usr/local/lib/
sudo ldconfig
```

Copy the plugin into a LADSPA search path and point the host at it:

```bash
mkdir -p ~/.ladspa
cp stereotool_ladspa.so ~/.ladspa/
export LADSPA_PATH=~/.ladspa
```

## Configuration

At load time the plugin reads a `settings.sts` preset from its own directory
(the one containing `stereotool_ladspa.so`). Export this file from the Stereo
Tool GUI and place it next to the plugin. The path is fixed because LADSPA
provides no way to pass a configuration path to a plugin.

The preset is parsed by the Stereo Tool library at startup, so the plugin
directory should not be writable by untrusted users.

## Ports

| Port     | Direction | Type  |
| -------- | --------- | ----- |
| Input L  | input     | audio |
| Input R  | input     | audio |
| Output L | output    | audio |
| Output R | output    | audio |

## License

This wrapper is released under the GNU General Public License v3.0; see
[`LICENSE`](LICENSE).

The Stereo Tool DSP library is proprietary and licensed separately by
[Thimeo](https://www.thimeo.com/licensing/). A valid license may be required
to use it.

## Contributing

Issues and pull requests are welcome. Please include a clear description of
the change and build or test instructions.
