# ThlHwTestDevices

KiCad designs, firmware and documentation for circuits that support testing of
electrical hardware, such as HASS and HALT testing.

Each design lives in its own folder under [`circuits/`](circuits/). Released
versions get separate folders with a version suffix (for example `V1p3` is
version 1.3), so every released version stays available side by side.

## License

Original work in this repository — the circuit designs, firmware and
documentation — is released under the [MIT License](LICENSE).

### Third-party files

The following are **not** covered by the MIT License. They remain under the
terms set by their original authors or providers:

* **`KiCad/` part libraries** (`footprints/`, `symbols/`,
  `SamacSys_Parts.pretty/`). Many of these footprints and symbols were
  downloaded from part-library services such as SnapEDA and SamacSys
  (Component Search Engine), or copied from the KiCad standard libraries.
  Check the provider's terms before redistributing them.
* **`Support/*.rar`**: setup and utility files for the T-962 reflow oven,
  included for convenience.
* **FatFs** in `circuits/ParasiticV1p0/firmware/lib/fatfs/`, by ChaN, under
  its own BSD-style license
  ([LICENSE.txt](circuits/ParasiticV1p0/firmware/lib/fatfs/LICENSE.txt)).

The PCB files (`*.kicad_pcb`) contain embedded copies of some of these
footprints, as all KiCad boards do. If you are a rights holder and believe a
file here should not be distributed, please open an issue.
