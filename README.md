# VoiceDictation

The application is still under development. Why? I've rewritten it from pure C++ to a hybrid language combining C# and Python.

This has simplified the development process and improved the application's performance. Now we have many opportunities to implement tasks that were much more difficult before.

A powerful and intuitive speech recognition application that transforms your voice into text with remarkable accuracy.

## Features

- **Multi-language Support**: Recognize speech in Russian, English, French, German, and Spanish
- **Continuous Recognition**: Capture your speech in real-time with continuous listening mode
- **File Processing**: Convert audio files to text with the same high accuracy
- **Proxy Support**: Work efficiently through corporate networks with full proxy configuration
- **Modern UI**: Clean, intuitive interface built with WPF and Material Design

## Technology Stack

- **Frontend**: WPF with Material Design and MVVM architecture
- **Backend**: .NET 8.0 Core libraries for speech processing
- **Speech Recognition**: Dual engine support with Microsoft Cognitive Services and Python-based recognition
- **Integration**: Seamless Python-to-.NET bridge using Python.NET

## Getting Started

1. Ensure Python 3.9+ is installed with required packages (`pip install -r src/PythonModules/requirements.txt`)
2. Build the solution using Visual Studio 2022 or .NET CLI
3. Launch the application and start dictating!

## License

This project is available under the GPL-3.0 license License. See the LICENSE file for more details.
