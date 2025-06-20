using Microsoft.Extensions.Logging;
using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows;
using VoiceDictation.Core.Utils;
using VoiceDictation.UI.Utils;
using VoiceDictation.Core.SpeechRecognition;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// View model for text operations
    /// </summary>
    public class TextOperationsViewModel
    {
        private readonly ILogger _logger;
        private readonly ISpeechRecognizer _speechRecognizer;
        private readonly Action<string> _setStatusMessage;
        private readonly Func<string> _getRecognizedText;
        private readonly Action<string> _setRecognizedText;

        public bool AutoCapitalization { get; set; } = true;
  
        public bool AutoFormatting { get; set; } = true;

        public bool AutoTransliterate { get; set; } = true;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="TextOperationsViewModel"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        /// <param name="speechRecognizer">Speech recognizer</param>
        /// <param name="setStatusMessage">Action to set status message</param>
        /// <param name="getRecognizedText">Function to get recognized text</param>
        /// <param name="setRecognizedText">Action to set recognized text</param>
        public TextOperationsViewModel(
            ILogger logger,
            ISpeechRecognizer speechRecognizer,
            Action<string> setStatusMessage,
            Func<string> getRecognizedText,
            Action<string> setRecognizedText)
        {
            _logger = logger;
            _speechRecognizer = speechRecognizer;
            _setStatusMessage = setStatusMessage;
            _getRecognizedText = getRecognizedText;
            _setRecognizedText = setRecognizedText;
        }
        
        /// <summary>
        /// Loads text from a file
        /// </summary>
        public void LoadText()
        {
            UIHelpers.SafeExecute(() =>
            {
                string? filePath = DialogHelpers.ShowTextOpenDialog();
                
                if (!string.IsNullOrEmpty(filePath) && File.Exists(filePath))
                {
                    string text = File.ReadAllText(filePath);
                    _setRecognizedText(text);
                }
            }, _logger, "Error loading text file");
        }
        
        /// <summary>
        /// Saves text to a file
        /// </summary>
        public void SaveText()
        {
            UIHelpers.SafeExecute(() =>
            {
                string? filePath = DialogHelpers.ShowTextSaveFileDialog();
                
                if (!string.IsNullOrEmpty(filePath))
                {
                    File.WriteAllText(filePath, _getRecognizedText());
                }
            }, _logger, "Error saving text file");
        }
        
        /// <summary>
        /// Clears the recognized text
        /// </summary>
        public void ClearText()
        {
            UIHelpers.SafeExecute(() =>
            {
                if (!string.IsNullOrEmpty(_getRecognizedText()))
                {
                    var result = MessageBox.Show("Вы уверены, что хотите очистить текст?", 
                        "Подтверждение", MessageBoxButton.YesNo, MessageBoxImage.Question);
                    
                    if (result == MessageBoxResult.Yes)
                    {
                        _setRecognizedText(string.Empty);
                        _setStatusMessage("Текст очищен");
                    }
                }
            }, _logger, "Error clearing text", message => _setStatusMessage(message));
        }
        
        /// <summary>
        /// Formats the recognized text
        /// </summary>
        public void FormatText()
        {
            UIHelpers.SafeExecute(() =>
            {
                string text = _getRecognizedText();
                if (string.IsNullOrEmpty(text))
                {
                    return;
                }
                
                string formattedText = text;
                
                formattedText = TextProcessingUtils.FormatRecognizedText(formattedText);
                
                if (AutoTransliterate)
                {
                    formattedText = TextProcessingUtils.ProcessMixedLanguageText(
                        formattedText, _speechRecognizer.Language);
                }
                
                _setRecognizedText(formattedText);
                _setStatusMessage("Текст отформатирован");
            }, _logger, "Error formatting text", message => _setStatusMessage(message));
        }
        
        /// <summary>
        /// Processes the recognized text and applies formatting if enabled
        /// </summary>
        /// <param name="text">Text to process</param>
        /// <returns>Processed text</returns>
        public string ProcessRecognizedText(string text)
        {
            if (string.IsNullOrEmpty(text))
                return text;
                
            if (AutoFormatting)
            {
                text = TextProcessingUtils.FormatRecognizedText(text);
            }
            
            if (AutoTransliterate)
            {
                text = TextProcessingUtils.ProcessMixedLanguageText(text, _speechRecognizer.Language);
            }
            
            return text;
        }
        
        /// <summary>
        /// Handles text recognition result
        /// </summary>
        /// <param name="result">Speech recognition result</param>
        /// <returns>True if text was successfully processed</returns>
        public bool HandleRecognitionResult(SpeechRecognitionResult result)
        {
            if (!result.IsSuccessful)
                return false;
            
            string newText = ProcessRecognizedText(result.Text);
            string existingText = _getRecognizedText();
            
            if (!string.IsNullOrEmpty(existingText))
            {
                _setRecognizedText(existingText + " " + newText);
            }
            else
            {
                _setRecognizedText(newText);
            }
            
            return true;
        }
    }
} 