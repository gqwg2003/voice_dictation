# Whisper Models Directory

This directory is used to store Whisper speech recognition model files that are required for local speech recognition.

## Model Files

Place the following model files in this directory:

- `ggml-base-en.bin` - English language model
- `ggml-base-ru.bin` - Russian language model

## Obtaining Models

The models can be downloaded from the official Whisper repository or converted from the original models using the `whisper.cpp` tools.

### Download Links

You can find pre-converted GGML format models at:
https://huggingface.co/ggerganov/whisper.cpp/tree/main

### Model Sizes

Different model sizes are available based on your needs:
- Tiny: Fastest, less accurate (~75MB)
- Base: Good balance of speed and accuracy (~142MB)
- Small: More accurate, slower (~466MB)
- Medium: High accuracy, slower (~1.5GB)
- Large: Most accurate, slowest (~3GB)

For this application, the base models are recommended for a good balance of speed and accuracy.

## Configuration

The application will automatically search for models in this directory. If models are not found here, it will check other common locations such as:
- Current application directory
- ~/.cache/whisper/ (Linux/Mac)
- /usr/local/share/whisper/
- /usr/share/whisper/ 