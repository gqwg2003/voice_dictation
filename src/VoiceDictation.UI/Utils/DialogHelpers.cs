using Microsoft.Win32;
using System;
using System.IO;
using System.Windows;

namespace VoiceDictation.UI.Utils
{
    /// <summary>
    /// Helper methods for working with dialogs
    /// </summary>
    public static class DialogHelpers
    {

        public static string? ShowAudioSaveFileDialog(string defaultFileName = "recording")
        {
            var dialog = new SaveFileDialog
            {
                Title = "Save Audio File",
                FileName = defaultFileName,
                DefaultExt = ".wav",
                Filter = "WAV Files (*.wav)|*.wav|MP3 Files (*.mp3)|*.mp3|All Files (*.*)|*.*"
            };

            return dialog.ShowDialog() == true ? dialog.FileName : null;
        }

        public static string? ShowAudioOpenFileDialog()
        {
            var dialog = new OpenFileDialog
            {
                Title = "Open Audio File",
                DefaultExt = ".wav",
                Filter = "Audio Files (*.wav;*.mp3)|*.wav;*.mp3|WAV Files (*.wav)|*.wav|MP3 Files (*.mp3)|*.mp3|All Files (*.*)|*.*"
            };

            return dialog.ShowDialog() == true ? dialog.FileName : null;
        }

        public static string? ShowTextOpenDialog()
        {
            var dialog = new OpenFileDialog
            {
                Title = "Open Text File",
                DefaultExt = ".txt",
                Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
            };

            return dialog.ShowDialog() == true ? dialog.FileName : null;
        }

        public static string? ShowTextSaveFileDialog(string defaultFileName = "transcript")
        {
            var dialog = new SaveFileDialog
            {
                Title = "Save Text File",
                FileName = defaultFileName,
                DefaultExt = ".txt",
                Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
            };

            return dialog.ShowDialog() == true ? dialog.FileName : null;
        }

        public static string? ShowAudioNormalizeSaveDialog(string originalFileName)
        {
            string fileName = Path.GetFileNameWithoutExtension(originalFileName);
            string extension = Path.GetExtension(originalFileName);
            string defaultName = $"{fileName}_normalized{extension}";

            var dialog = new SaveFileDialog
            {
                Title = "Save Normalized Audio",
                FileName = defaultName,
                DefaultExt = extension,
                Filter = $"Audio Files (*{extension})|*{extension}|All Files (*.*)|*.*"
            };

            return dialog.ShowDialog() == true ? dialog.FileName : null;
        }

        public static bool ShowConfirmationDialog(string message, string title = "Confirmation")
        {
            return MessageBox.Show(
                message,
                title,
                MessageBoxButton.YesNo,
                MessageBoxImage.Question) == MessageBoxResult.Yes;
        }

        public static void ShowError(string message, string title = "Error")
        {
            MessageBox.Show(message, title, MessageBoxButton.OK, MessageBoxImage.Error);
        }

        public static void ShowInfo(string message, string title = "Information")
        {
            MessageBox.Show(message, title, MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }
} 