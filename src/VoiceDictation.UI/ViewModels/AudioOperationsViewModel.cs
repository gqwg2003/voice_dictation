using System;
using CommunityToolkit.Mvvm.ComponentModel;
using Microsoft.Extensions.Logging;
using System.IO;
using VoiceDictation.Core.Utils;
using VoiceDictation.UI.Utils;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// View model for audio operations
    /// </summary>
    public class AudioOperationsViewModel
    {
        private readonly ILogger _logger;
        private readonly Action<string> _setStatusMessage;

        public string CurrentAudioFilePath { get; set; } = string.Empty;
        
        public string LastSavedFilePath { get; set; } = string.Empty;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioOperationsViewModel"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        /// <param name="setStatusMessage">Action to set status message</param>
        public AudioOperationsViewModel(ILogger logger, Action<string> setStatusMessage)
        {
            _logger = logger;
            _setStatusMessage = setStatusMessage;
        }
        
        /// <summary>
        /// Processes an audio file for speech recognition
        /// </summary>
        /// <returns>WAV file path if successful, otherwise null</returns>
        public string PreprocessAudioFile()
        {
            try 
            {
                string? filePath = DialogHelpers.ShowAudioOpenFileDialog();
                
                if (string.IsNullOrEmpty(filePath) || !File.Exists(filePath))
                    return string.Empty;
                
                CurrentAudioFilePath = filePath;
                
                if (!FileUtils.IsValidAudioFile(CurrentAudioFilePath))
                {
                    UIHelpers.ShowWarningMessage("Выбранный файл не является поддерживаемым аудио файлом.");
                    return string.Empty;
                }
                
                // Преобразуем в WAV, если нужно
                if (Path.GetExtension(CurrentAudioFilePath).ToLowerInvariant() != ".wav")
                {
                    string wavFilePath = FileUtils.CreateTempWavFile(CurrentAudioFilePath);
                    if (string.IsNullOrEmpty(wavFilePath))
                    {
                        UIHelpers.ShowWarningMessage("Не удалось конвертировать аудио файл в формат WAV.");
                        return string.Empty;
                    }
                    
                    return wavFilePath;
                }
                
                return CurrentAudioFilePath;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error preprocessing audio file");
                return string.Empty;
            }
        }
        
        /// <summary>
        /// Exports the current audio to a file
        /// </summary>
        public void ExportAudio()
        {
            UIHelpers.SafeExecute(() =>
            {
                if (string.IsNullOrEmpty(CurrentAudioFilePath))
                {
                    UIHelpers.ShowInfoMessage("Нет аудио для экспорта.");
                    return;
                }
                
                string? filePath = DialogHelpers.ShowAudioSaveFileDialog(
                    Path.GetFileNameWithoutExtension(CurrentAudioFilePath));
                    
                if (!string.IsNullOrEmpty(filePath))
                {
                    if (CurrentAudioFilePath != filePath)
                    {
                        File.Copy(CurrentAudioFilePath, filePath, true);
                        LastSavedFilePath = filePath;
                        _setStatusMessage($"Аудио экспортировано: {Path.GetFileName(filePath)}");
                    }
                }
            }, _logger, "Error exporting audio file", null, ex =>
            {
                UIHelpers.ShowWarningMessage("Не удалось экспортировать аудио файл.");
            });
        }
        
        /// <summary>
        /// Normalizes the volume of the current audio file
        /// </summary>
        public void NormalizeAudio()
        {
            UIHelpers.SafeExecute(() =>
            {
                if (string.IsNullOrEmpty(CurrentAudioFilePath))
                {
                    UIHelpers.ShowInfoMessage("Нет аудио для нормализации.");
                    return;
                }
                
                string? filePath = DialogHelpers.ShowAudioNormalizeSaveDialog(CurrentAudioFilePath);
                    
                if (!string.IsNullOrEmpty(filePath))
                {
                    bool success = AudioUtils.NormalizeAudio(CurrentAudioFilePath, filePath);
                    
                    if (success)
                    {
                        LastSavedFilePath = filePath;
                        _setStatusMessage($"Аудио нормализовано: {Path.GetFileName(filePath)}");
                    }
                    else
                    {
                        UIHelpers.ShowWarningMessage("Не удалось нормализовать аудио файл.");
                    }
                }
            }, _logger, "Error normalizing audio file", null, ex =>
            {
                UIHelpers.ShowWarningMessage("Не удалось нормализовать аудио файл.");
            });
        }

        public bool CanExportAudio() => !string.IsNullOrEmpty(CurrentAudioFilePath);

        public bool CanNormalizeAudio() => !string.IsNullOrEmpty(CurrentAudioFilePath);
    }
} 