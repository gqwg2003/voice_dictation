using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using VoiceDictation.Core.SpeechRecognition;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// Main view model for the application
    /// </summary>
    public class MainViewModel : ObservableObject
    {
        private readonly ILogger<MainViewModel> _logger;
        private readonly ISpeechRecognizer _speechRecognizer;
        
        private string _recognizedText = string.Empty;
        private string _statusMessage = "Готов к работе";
        private double _recognitionProgress;
        private string _recordingTimeDisplay = "00:00";
        private bool _isRecording;
        private DateTime _recordingStartTime;
        private System.Timers.Timer? _recordingTimer;
        
        public string RecognizedText
        {
            get => _recognizedText;
            set => SetProperty(ref _recognizedText, value);
        }
        
        public string StatusMessage
        {
            get => _statusMessage;
            set => SetProperty(ref _statusMessage, value);
        }
        
        public double RecognitionProgress
        {
            get => _recognitionProgress;
            set => SetProperty(ref _recognitionProgress, value);
        }
        
        public string RecordingTimeDisplay
        {
            get => _recordingTimeDisplay;
            set => SetProperty(ref _recordingTimeDisplay, value);
        }
        
        public ObservableCollection<LanguageViewModel> AvailableLanguages { get; } = new ObservableCollection<LanguageViewModel>();
        
        private LanguageViewModel? _selectedLanguage;
        
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
        
        public IRelayCommand StartRecognitionCommand { get; }
        public IRelayCommand StopRecognitionCommand { get; }
        public IRelayCommand LoadTextCommand { get; }
        public IRelayCommand SaveTextCommand { get; }
        public IRelayCommand ClearTextCommand { get; }
        public IRelayCommand OpenSettingsCommand { get; }
        public IRelayCommand OpenHelpCommand { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class
        /// </summary>
        /// <param name="logger">The logger</param>
        /// <param name="speechRecognizer">The speech recognizer</param>
        public MainViewModel(ILogger<MainViewModel> logger, ISpeechRecognizer speechRecognizer)
        {
            _logger = logger;
            _speechRecognizer = speechRecognizer;
            
            StartRecognitionCommand = new RelayCommand(StartRecognition, CanStartRecognition);
            StopRecognitionCommand = new RelayCommand(StopRecognition, CanStopRecognition);
            LoadTextCommand = new RelayCommand(LoadText);
            SaveTextCommand = new RelayCommand(SaveText);
            ClearTextCommand = new RelayCommand(ClearText);
            OpenSettingsCommand = new RelayCommand(OpenSettings);
            OpenHelpCommand = new RelayCommand(OpenHelp);
            
            _recordingTimer = new System.Timers.Timer(1000);
            _recordingTimer.Elapsed += (s, e) => UpdateRecordingTime();
        }
        
        /// <summary>
        /// Initializes the view model
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        public async Task InitializeAsync()
        {
            try
            {
                _speechRecognizer.SpeechRecognized += SpeechRecognizer_SpeechRecognized;
                _speechRecognizer.SpeechError += SpeechRecognizer_SpeechError;
                _speechRecognizer.StatusChanged += SpeechRecognizer_StatusChanged;
                
                LoadAvailableLanguages();
                
                var defaultLanguage = AvailableLanguages.FirstOrDefault(l => l.Code == "ru-RU") ?? AvailableLanguages.FirstOrDefault();
                if (defaultLanguage != null)
                {
                    SelectedLanguage = defaultLanguage;
                }
                
                StatusMessage = "Готов к работе";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error initializing MainViewModel");
                StatusMessage = $"Ошибка инициализации: {ex.Message}";
            }
        }
        
        /// <summary>
        /// Cleans up resources used by the view model
        /// </summary>
        public void Cleanup()
        {
            if (_isRecording)
            {
                StopRecognition();
            }
            
            _speechRecognizer.SpeechRecognized -= SpeechRecognizer_SpeechRecognized;
            _speechRecognizer.SpeechError -= SpeechRecognizer_SpeechError;
            _speechRecognizer.StatusChanged -= SpeechRecognizer_StatusChanged;
            
            _recordingTimer?.Dispose();
            _recordingTimer = null;
            
            _speechRecognizer.Dispose();
        }
        
        /// <summary>
        /// Loads the available languages for speech recognition
        /// </summary>
        private void LoadAvailableLanguages()
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
                
                _logger.LogInformation("Loaded {Count} languages", AvailableLanguages.Count);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading available languages");
                StatusMessage = $"Ошибка загрузки языков: {ex.Message}";
                
                AvailableLanguages.Add(new LanguageViewModel { Code = "ru-RU", DisplayName = "Русский" });
                AvailableLanguages.Add(new LanguageViewModel { Code = "en-US", DisplayName = "Английский (США)" });
            }
        }
        
        /// <summary>
        /// Starts speech recognition
        /// </summary>
        private async void StartRecognition()
        {
            try
            {
                StatusMessage = "Начинаем распознавание...";
                RecognitionProgress = 0;
                _isRecording = true;
                _recordingStartTime = DateTime.Now;
                
                _recordingTimer?.Start();
                
                await _speechRecognizer.StartContinuousRecognitionAsync();
                
                UpdateCommandsCanExecute();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error starting recognition");
                StatusMessage = $"Ошибка запуска распознавания: {ex.Message}";
                _isRecording = false;
                _recordingTimer?.Stop();
            }
        }
        
        /// <summary>
        /// Stops speech recognition
        /// </summary>
        private async void StopRecognition()
        {
            try
            {
                _recordingTimer?.Stop();
                
                await _speechRecognizer.StopContinuousRecognitionAsync();
                
                _isRecording = false;
                StatusMessage = "Распознавание остановлено";
                
                UpdateCommandsCanExecute();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error stopping recognition");
                StatusMessage = $"Ошибка остановки распознавания: {ex.Message}";
            }
        }
        
        /// <summary>
        /// Loads text from a file
        /// </summary>
        private void LoadText()
        {
            try
            {
                var openFileDialog = new OpenFileDialog
                {
                    Filter = "Текстовые файлы (*.txt)|*.txt|Все файлы (*.*)|*.*",
                    Title = "Открыть текстовый файл"
                };
                
                if (openFileDialog.ShowDialog() == true)
                {
                    RecognizedText = File.ReadAllText(openFileDialog.FileName);
                    StatusMessage = $"Текст загружен из файла: {Path.GetFileName(openFileDialog.FileName)}";
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading text from file");
                StatusMessage = $"Ошибка загрузки файла: {ex.Message}";
            }
        }
        
        /// <summary>
        /// Saves text to a file
        /// </summary>
        private void SaveText()
        {
            try
            {
                var saveFileDialog = new SaveFileDialog
                {
                    Filter = "Текстовые файлы (*.txt)|*.txt|Все файлы (*.*)|*.*",
                    Title = "Сохранить текстовый файл"
                };
                
                if (saveFileDialog.ShowDialog() == true)
                {
                    File.WriteAllText(saveFileDialog.FileName, RecognizedText);
                    StatusMessage = $"Текст сохранен в файл: {Path.GetFileName(saveFileDialog.FileName)}";
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error saving text to file");
                StatusMessage = $"Ошибка сохранения файла: {ex.Message}";
            }
        }
        
        /// <summary>
        /// Clears the recognized text
        /// </summary>
        private void ClearText()
        {
            if (!string.IsNullOrEmpty(RecognizedText))
            {
                var result = MessageBox.Show("Вы уверены, что хотите очистить текст?", "Подтверждение", MessageBoxButton.YesNo, MessageBoxImage.Question);
                
                if (result == MessageBoxResult.Yes)
                {
                    RecognizedText = string.Empty;
                    StatusMessage = "Текст очищен";
                }
            }
        }
        
        /// <summary>
        /// Opens the settings window
        /// </summary>
        private void OpenSettings()
        {
            try
            {
                var serviceProvider = ((App)Application.Current).ServiceProvider;
                
                var settingsViewModel = serviceProvider.GetRequiredService<SettingsViewModel>();
                
                var settingsWindow = new Views.SettingsWindow(settingsViewModel)
                {
                    Owner = Application.Current.MainWindow
                };
                
                settingsWindow.ShowDialog();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error opening settings window");
                MessageBox.Show($"Ошибка открытия окна настроек: {ex.Message}", "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        
        /// <summary>
        /// Opens the help window
        /// </summary>
        private void OpenHelp()
        {
            MessageBox.Show("Для начала записи нажмите кнопку с иконкой микрофона или используйте горячую клавишу Ctrl+Shift+R", "Справка", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        
        /// <summary>
        /// Updates the recording time display
        /// </summary>
        private void UpdateRecordingTime()
        {
            if (_isRecording)
            {
                var elapsed = DateTime.Now - _recordingStartTime;
                
                Application.Current.Dispatcher.Invoke(() =>
                {
                    RecordingTimeDisplay = $"{elapsed.Minutes:00}:{elapsed.Seconds:00}";
                });
            }
        }
        
        /// <summary>
        /// Updates the CanExecute state of commands
        /// </summary>
        private void UpdateCommandsCanExecute()
        {
            ((RelayCommand)StartRecognitionCommand).NotifyCanExecuteChanged();
            ((RelayCommand)StopRecognitionCommand).NotifyCanExecuteChanged();
        }
        
        /// <summary>
        /// Determines whether the StartRecognition command can execute
        /// </summary>
        /// <returns>True if the command can execute, otherwise false</returns>
        private bool CanStartRecognition() => !_isRecording;
        
        /// <summary>
        /// Determines whether the StopRecognition command can execute
        /// </summary>
        /// <returns>True if the command can execute, otherwise false</returns>
        private bool CanStopRecognition() => _isRecording;
        
        /// <summary>
        /// Handles the SpeechRecognized event
        /// </summary>
        private void SpeechRecognizer_SpeechRecognized(object? sender, SpeechRecognizedEventArgs e)
        {
            if (e.Result.IsSuccessful)
            {
                if (!string.IsNullOrEmpty(RecognizedText))
                {
                    RecognizedText += " ";
                }
                
                RecognizedText += e.Result.Text;
                
                Application.Current.Dispatcher.Invoke(() =>
                {
                    StatusMessage = "Распознано: " + e.Result.Text;
                });
            }
        }
        
        /// <summary>
        /// Handles the SpeechError event
        /// </summary>
        private void SpeechRecognizer_SpeechError(object? sender, SpeechErrorEventArgs e)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                StatusMessage = $"Ошибка распознавания: {e.ErrorMessage}";
            });
        }
        
        /// <summary>
        /// Handles the StatusChanged event
        /// </summary>
        private void SpeechRecognizer_StatusChanged(object? sender, SpeechStatusChangedEventArgs e)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                switch (e.Status)
                {
                    case SpeechRecognitionStatus.Listening:
                        RecognitionProgress = 25;
                        StatusMessage = "Слушаю...";
                        break;
                    case SpeechRecognitionStatus.SpeechDetected:
                        RecognitionProgress = 50;
                        StatusMessage = "Речь обнаружена...";
                        break;
                    case SpeechRecognitionStatus.Processing:
                        RecognitionProgress = 75;
                        StatusMessage = "Обработка...";
                        break;
                    case SpeechRecognitionStatus.Recognized:
                        RecognitionProgress = 100;
                        break;
                    case SpeechRecognitionStatus.Error:
                        RecognitionProgress = 0;
                        _recordingTimer?.Stop();
                        _isRecording = false;
                        UpdateCommandsCanExecute();
                        break;
                    case SpeechRecognitionStatus.Idle:
                        RecognitionProgress = 0;
                        break;
                }
                
                if (!string.IsNullOrEmpty(e.StatusDetails))
                {
                    StatusMessage = e.StatusDetails;
                }
            });
        }
    }
    
    /// <summary>
    /// View model for language information
    /// </summary>
    public class LanguageViewModel
    {
        public string Code { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
    }
} 