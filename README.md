# `sputnik-led`
Assumes macOS development environment.

Install `pyenv`:
```
curl -fsSL https://pyenv.run | bash
```

Install a usable Python version:
```
pyenv install 3.13.3
```

Create a `venv`:
```
pyenv virtualenv 3.13.3 sputnik-env
```

Install `platformio` tool:
```
pip install platformio
```

If starting a new project from scratch:
```
pio project init --ide vim --board megaatmega2560
```

Compile:
```
pio run
```

Compile and output `compile_commands.json` for IDE stuff:
```
pio run -t compiledb
```

Compile and flash Arduino:
```
 pio run --target upload
```

## LED notes
* square WS2811 LED color order: `RBG`
* rope WS2811 LED color order: `RGB`
