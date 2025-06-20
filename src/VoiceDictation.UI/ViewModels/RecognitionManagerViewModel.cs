using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using Microsoft.Extensions.Logging;
using VoiceDictation.Core.SpeechRecognition;
using VoiceDictation.UI.Utils;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// View model for managing speech recognition
    /// </summary>
    public class RecognitionManagerViewModel : ObservableObject
    {
        private readonly ILogger _logger;
        private readonly ISpeechRecognizer _speechRecognizer;
        private readonly Action<string> _setStatusMessage;
        private readonly Action<double> _setRecognitionProgress;
        
        private bool _isRecording;
        private DateTime _recordingStartTime;
        private System.Timers.Timer? _recordingTimer;
        private int _recordingDuration = 10;
        private string _recordingTimeDisplay = "00:00";
        private LanguageViewModel? _selectedLanguage;
        
        /// <summary>
        /// Gets or sets the recording time display
        /// </summary>
        public string RecordingTimeDisplay
        {
            get => _recordingTimeDisplay;
            set => SetProperty(ref _recordingTimeDisplay, value);
        }
        
        /// <summary>
        /// Gets or sets the recording duration
        /// </summary>
        public int RecordingDuration
        {
            get => _recordingDuration;
            set => SetProperty(ref _recordingDuration, value);
        }

        public bool IsRecording => _isRecording;
        public ObservableCollection<LanguageViewModel> AvailableLanguages { get; } = new ObservableCollection<LanguageViewModel>();

        public LanguageViewModel? SelectedLanguage
        {
            get => _selectedLanguage;
            set
            {
                if (SetProperty(ref _selectedLanguage, value) && value != null)
                {
                    _speechRecognizer.Language = value.Code;
                }
            }
        }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="RecognitionManagerViewModel"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        /// <param name="speechRecognizer">Speech recognizer</param>
        /// <param name="setStatusMessage">Action to set status message</param>
        /// <param name="setRecognitionProgress">Action to set recognition progress</param>
        public RecognitionManagerViewModel(
            ILogger logger, 
            ISpeechRecognizer speechRecognizer,
            Action<string> setStatusMessage,
            Action<double> setRecognitionProgress)
        {
            _logger = logger;
            _speechRecognizer = speechRecognizer;
            _setStatusMessage = setStatusMessage;
            _setRecognitionProgress = setRecognitionProgress;
            
            _recordingTimer = new System.Timers.Timer(1000);
            _recordingTimer.Elapsed += (s, e) => UpdateRecordingTime();
        }
        
        /// <summary>
        /// Loads available languages for speech recognition
        /// </summary>
        public void LoadAvailableLanguages()
        {
            AvailableLanguages.Clear();
            
            try
            {
                foreach (var language in _speechRecognizer.GetAvailableLanguages())
                {
                    AvailableLanguages.Add(new LanguageViewModel
                    {
                        Code = language.Code,
                        DisplayName = language.DisplayName
                    });
                }
                
                var defaultLanguage = AvailableLanguages.FirstOrDefault(l => l.Code == "ru-RU") ?? AvailableLanguages.FirstOrDefault();
                if (defaultLanguage != null)
                {
                    SelectedLanguage = defaultLanguage;
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading available languages");
            }
        }
        
        /// <summary>
        /// Starts speech recognition
        /// </summary>
        public async Task StartRecognitionAsync()
        {
            await UIHelpers.SafeExecuteAsync(async () =>
            {
                _setStatusMessage("Начинаем распознавание...");
                _setRecognitionProgress(0);
                _isRecording = true;
                _recordingStartTime = DateTime.Now;
                
                _recordingTimer?.Start();
                
                await _speechRecognizer.StartContinuousRecognitionAsync();
                
                NotifyCommandsCanExecuteChanged?.Invoke();
            }, _logger, "Error starting recognition", message => _setStatusMessage(message), ex =>
            {
                _isRecording = false;
                _recordingTimer?.Stop();
            });
        }
        
        /// <summary>
        /// Stops speech recognition
        /// </summary>
        public async Task StopRecognitionAsync()
        {
            await UIHelpers.SafeExecuteAsync(async () =>
            {
                _recordingTimer?.Stop();
                
                await _speechRecognizer.StopContinuousRecognitionAsync();
                
                _isRecording = false;
                _setStatusMessage("Распознавание остановлено");
                
                NotifyCommandsCanExecuteChanged?.Invoke();
            }, _logger, "Error stopping recognition", message => _setStatusMessage(message));
        }
        
        /// <summary>
        /// Updates the recording time display
        /// </summary>
        private void UpdateRecordingTime()
        {
            if (_isRecording)
            {
                var elapsed = DateTime.Now - _recordingStartTime;
                
                System.Windows.Application.Current.Dispatcher.Invoke(() =>
                {
                    RecordingTimeDisplay = $"{elapsed.Minutes:00}:{elapsed.Seconds:00}";
                    
                    if (RecordingDuration > 0)
                    {
                        double progress = (elapsed.TotalSeconds % RecordingDuration) / RecordingDuration * 100;
                        _setRecognitionProgress(progress);
                    }
                });
            }
        }
    
        public bool CanStartRecognition() => !_isRecording;

        public bool CanStopRecognition() => _isRecording;

        public event Action? NotifyCommandsCanExecuteChanged;

        public void Cleanup()
        {
            if (_isRecording)
            {
                StopRecognitionAsync().Wait();
            }
            
            _recordingTimer?.Dispose();
            _recordingTimer = null;
        }
    }
} 