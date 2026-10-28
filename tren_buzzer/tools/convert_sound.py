#!/usr/bin/env python3
"""Convierte un MP3 a PCM crudo (44.1kHz/16-bit estereo) y lo agrega como
arreglo C a src/audio_data.cpp / include/audio_data.h.

Uso: python3 tools/convert_sound.py <nombre_variable> <ruta.mp3>
Ejemplo: python3 tools/convert_sound.py campana sounds/campana.mp3
"""
import subprocess
import sys
import wave
import tempfile
import os

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CPP_PATH = os.path.join(PROJECT_DIR, "src", "audio_data.cpp")
H_PATH = os.path.join(PROJECT_DIR, "include", "audio_data.h")


def mp3_to_pcm_bytes(mp3_path):
    with tempfile.NamedTemporaryFile(suffix=".wav") as tmp:
        subprocess.run([
            "gst-launch-1.0", "-q",
            "filesrc", f"location={mp3_path}", "!",
            "decodebin", "!", "audioconvert", "!", "audioresample", "!",
            "audio/x-raw,format=S16LE,channels=2,rate=44100", "!",
            "wavenc", "!", "filesink", f"location={tmp.name}",
        ], check=True)
        wf = wave.open(tmp.name, "rb")
        frames = wf.readframes(wf.getnframes())
        wf.close()
    return frames


def load_existing_sounds():
    """Lee audio_data.h para saber que variables ya existen."""
    names = []
    if os.path.exists(H_PATH):
        with open(H_PATH) as f:
            for line in f:
                if line.startswith("extern const uint8_t "):
                    names.append(line.split()[3].rstrip("[];"))
    return names


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    var_name, mp3_path = sys.argv[1], sys.argv[2]

    existing = [n for n in load_existing_sounds() if n != var_name]
    frames = mp3_to_pcm_bytes(mp3_path)
    print(f"{var_name}: {len(frames)} bytes PCM (mod4={len(frames) % 4})")

    # Guardamos todos los sonidos existentes + el nuevo, regenerando todo el archivo.
    sounds = {n: None for n in existing}
    sounds[var_name] = frames

    # Si ya existia el sonido, hay que re-leer su data cruda desde el .cpp viejo
    # es mas simple pedir que se vuelva a correr este script para cada sonido
    # si se quiere regenerar todos. Aqui solo anexamos el nuevo si no existia.
    with open(CPP_PATH, "a" if existing else "w") as f:
        if not existing:
            f.write('#include "audio_data.h"\n\n')
        f.write(f"const uint32_t {var_name}_pcm_len = {len(frames)};\n")
        f.write(f"const uint8_t {var_name}_pcm[] = {{\n")
        for i in range(0, len(frames), 24):
            chunk = frames[i:i + 24]
            f.write(",".join(str(b) for b in chunk) + ",\n")
        f.write("};\n\n")

    with open(H_PATH, "a" if existing else "w") as f:
        if not existing:
            f.write("#pragma once\n#include <Arduino.h>\n\n")
        f.write(f"extern const uint8_t {var_name}_pcm[];\n")
        f.write(f"extern const uint32_t {var_name}_pcm_len;\n")

    print(f"Agregado '{var_name}' a audio_data.cpp / audio_data.h")


if __name__ == "__main__":
    main()
