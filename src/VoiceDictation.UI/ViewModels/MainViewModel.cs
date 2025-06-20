using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows;
using VoiceDictation.Core.SpeechRecognition;
using VoiceDictation.Core.Utils;
using VoiceDictation.UI.Utils;
using VoiceDictation.UI.Views;
using Microsoft.Extensions.Logging.Abstractions;
using VoiceDictation.Network.Proxy;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// Main view model for the application
    /// </summary>
    public class MainViewModel : ObservableObject
    {
        private readonly ILogger<MainViewModel> _logger;
        private readonly ISpeechRecognizer _speechRecognizer;
        
        private readonly RecognitionManagerViewModel _recognitionManager;
        private readonly TextOperationsViewModel _textOperations;
        private readonly AudioOperationsViewModel _audioOperations;
        
        private string _recognizedText = string.Empty;
        private string _statusMessage = "Готов к работе";
        private double _recognitionProgress;
        
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
        
        // Properties delegated to _recognitionManager
        public string RecordingTimeDisplay
        {
            get => _recognitionManager.RecordingTimeDisplay;
            set => _recognitionManager.RecordingTimeDisplay = value;
        }
        
        public int RecordingDuration
        {
            get => _recognitionManager.RecordingDuration;
            set => _recognitionManager.RecordingDuration = value;
        }
        
        // Properties delegated to _textOperations
        public bool AutoCapitalization
        {
            get => _textOperations.AutoCapitalization;
            set => _textOperations.AutoCapitalization = value;
        }
        
        public bool AutoFormatting
        {
            get => _textOperations.AutoFormatting;
            set => _textOperations.AutoFormatting = value;
        }
        
        public bool AutoTransliterate
        {
            get => _textOperations.AutoTransliterate;
            set => _textOperations.AutoTransliterate = value;
        }
        
        // Properties delegated to _audioOperations
        public string CurrentAudioFilePath
        {
            get => _audioOperations.CurrentAudioFilePath;
            private set => _audioOperations.CurrentAudioFilePath = value;
        }
        
        // Collections delegated to _recognitionManager
        public System.Collections.ObjectModel.ObservableCollection<LanguageViewModel> AvailableLanguages => _recognitionManager.AvailableLanguages;
        
        public LanguageViewModel? SelectedLanguage
        {
            get => _recognitionManager.SelectedLanguage;
            set => _recognitionManager.SelectedLanguage = value;
                }
        
        // Commands
        public IRelayCommand StartRecognitionCommand { get; }
        public IRelayCommand StopRecognitionCommand { get; }
        public IRelayCommand LoadTextCommand { get; }
        public IRelayCommand SaveTextCommand { get; }
        public IRelayCommand ClearTextCommand { get; }
        public IRelayCommand OpenSettingsCommand { get; }
        public IRelayCommand OpenHelpCommand { get; }
        public IRelayCommand ProcessAudioFileCommand { get; }
        public IRelayCommand FormatTextCommand { get; }
        public IRelayCommand ExportAudioCommand { get; }
        public IRelayCommand NormalizeAudioCommand { get; }
        
        public MainViewModel(ILogger<MainViewModel> logger, ISpeechRecognizer speechRecognizer)
        {
            _logger = logger;
            _speechRecognizer = speechRecognizer;

            _recognitionManager = new RecognitionManagerViewModel(
                logger, 
                speechRecognizer, 
                msg => StatusMessage = msg,
                progress => RecognitionProgress = progress);
                
            _textOperations = new TextOperationsViewModel(
                logger,
                speechRecognizer,
                msg => StatusMessage = msg,
                () => RecognizedText,
                text => RecognizedText = text);
                
            _audioOperations = new AudioOperationsViewModel(
                logger,
                msg => StatusMessage = msg);

            _recognitionManager.NotifyCommandsCanExecuteChanged += () => {
                ((RelayCommand)StartRecognitionCommand).NotifyCanExecuteChanged();
                ((RelayCommand)StopRecognitionCommand).NotifyCanExecuteChanged();
            };

            StartRecognitionCommand = new RelayCommand(StartRecognition, CanStartRecognition);
            StopRecognitionCommand = new RelayCommand(StopRecognition, CanStopRecognition);
            LoadTextCommand = new RelayCommand(_textOperations.LoadText);
            SaveTextCommand = new RelayCommand(_textOperations.SaveText);
            ClearTextCommand = new RelayCommand(_textOperations.ClearText);
            OpenSettingsCommand = new RelayCommand(OpenSettings);
            OpenHelpCommand = new RelayCommand(OpenHelp);
            ProcessAudioFileCommand = new RelayCommand(ProcessAudioFile);
            FormatTextCommand = new RelayCommand(_textOperations.FormatText);
            ExportAudioCommand = new RelayCommand(_audioOperations.ExportAudio, _audioOperations.CanExportAudio);
            NormalizeAudioCommand = new RelayCommand(_audioOperations.NormalizeAudio, _audioOperations.CanNormalizeAudio);
        }
        
        /// <summary>
        /// Initializes the view model
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        public async Task InitializeAsync()
        {
            try
            {
                await Task.Yield();
                
                if (_speechRecognizer != null)
                {
                    _speechRecognizer.SpeechRecognized += SpeechRecognizer_SpeechRecognized;
                    _speechRecognizer.SpeechError += SpeechRecognizer_SpeechError;
                    _speechRecognizer.StatusChanged += SpeechRecognizer_StatusChanged;
                }
                
                _recognitionManager?.LoadAvailableLanguages();
                
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
            if (_speechRecognizer != null)
            {
                _speechRecognizer.SpeechRecognized -= SpeechRecognizer_SpeechRecognized;
                _speechRecognizer.SpeechError -= SpeechRecognizer_SpeechError;
                _speechRecognizer.StatusChanged -= SpeechRecognizer_StatusChanged;
                
                _speechRecognizer.Dispose();
            }
            
            _recognitionManager?.Cleanup();
        }
        
        private void StartRecognition()
        {
            _ = _recognitionManager.StartRecognitionAsync();
            }
        
        private void StopRecognition()
        {
            _ = _recognitionManager.StopRecognitionAsync();
        }
        
        private bool CanStartRecognition() => _recognitionManager.CanStartRecognition();
        
        private bool CanStopRecognition() => _recognitionManager.CanStopRecognition();
        
        private void OpenSettings()
        {
            try
                {
                var proxyManager = ((App)Application.Current).ServiceProvider.GetRequiredService<IProxyManager>();
                
                var loggerFactory = ((App)Application.Current).ServiceProvider.GetRequiredService<ILoggerFactory>();
                var settingsLogger = loggerFactory.CreateLogger<SettingsViewModel>();
                
                var settingsWindow = new Views.SettingsWindow(
                    new SettingsViewModel(settingsLogger, _speechRecognizer, proxyManager)
                );
                
                settingsWindow.Owner = Application.Current.MainWindow;
                settingsWindow.WindowStartupLocation = WindowStartupLocation.CenterOwner;
                
                _logger.LogInformation("Opening settings window");
                settingsWindow.ShowDialog();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error opening settings window");
                MessageBox.Show($"Ошибка открытия окна настроек: {ex.Message}", "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        
        private void OpenHelp()
        {
            UIHelpers.ShowInfoMessage(
                "Для начала записи нажмите кнопку с иконкой микрофона или используйте горячую клавишу Ctrl+Shift+R", 
                "Справка");
        }
        
        private void SpeechRecognizer_SpeechRecognized(object? sender, SpeechRecognizedEventArgs e)
        {
            UIHelpers.SafeExecute(() =>
            {
                _textOperations.HandleRecognitionResult(e.Result);
                
                Application.Current.Dispatcher.Invoke(() =>
                {
                    StatusMessage = "Распознано: " + e.Result.Text;
                });
            }, _logger, "Error handling speech recognized event");
        }
        
        private void SpeechRecognizer_SpeechError(object? sender, SpeechErrorEventArgs e)
        {
            UIHelpers.SafeExecute(() =>
            {
            Application.Current.Dispatcher.Invoke(() =>
            {
                StatusMessage = $"Ошибка распознавания: {e.ErrorMessage}";
            });
            }, _logger, "Error handling speech error event");
        }
        
        private void SpeechRecognizer_StatusChanged(object? sender, SpeechStatusChangedEventArgs e)
        {
            UIHelpers.SafeExecute(() =>
            {
            Application.Current.Dispatcher.Invoke(() =>
            {
                switch (e.Status)
                {
                        case SpeechRecognitionStatus.Initializing:
                            StatusMessage = "Инициализация...";
                            break;
                        case SpeechRecognitionStatus.Idle:
                            StatusMessage = "Готов к работе";
                            RecognitionProgress = 0;
                            break;
                    case SpeechRecognitionStatus.Listening:
                            StatusMessage = $"Слушаю...";
                        break;
                    case SpeechRecognitionStatus.Processing:
                        StatusMessage = "Обработка...";
                        break;
                    case SpeechRecognitionStatus.Recognized:
                            StatusMessage = "Распознано";
                        break;
                    case SpeechRecognitionStatus.Error:
                            StatusMessage = "Ошибка распознавания";
                        break;
                }
                });
            }, _logger, "Error handling speech status changed event");
        }
        
        private async void ProcessAudioFile()
        {
            await UIHelpers.SafeExecuteAsync(async () =>
            {
                string wavFilePath = _audioOperations.PreprocessAudioFile();
                if (string.IsNullOrEmpty(wavFilePath))
                    return;
                
                bool needsCleanup = wavFilePath != _audioOperations.CurrentAudioFilePath;
                
                try
                {
                    StatusMessage = $"Обрабатываю файл: {System.IO.Path.GetFileName(_audioOperations.CurrentAudioFilePath)}...";
                    var result = await _speechRecognizer.RecognizeFromFileAsync(wavFilePath);
                    
                    if (result.IsSuccessful)
                {
                        string newText = _textOperations.ProcessRecognizedText(result.Text);
                        
                        if (!string.IsNullOrEmpty(RecognizedText))
                        {
                            RecognizedText += Environment.NewLine + Environment.NewLine + newText;
                        }
                        else
                        {
                            RecognizedText = newText;
                        }
                        
                        StatusMessage = $"Файл распознан: {System.IO.Path.GetFileName(_audioOperations.CurrentAudioFilePath)}";
                    }
                    else
                    {
                        StatusMessage = $"Ошибка распознавания: {result.ErrorMessage}";
                    }
                }
                finally
                {
                    if (needsCleanup && !string.IsNullOrEmpty(wavFilePath))
                    {
                        FileUtils.SafeDeleteFile(wavFilePath);
                }
                }
            }, _logger, "Error processing audio file", message => StatusMessage = message, ex => 
            {
                UIHelpers.ShowErrorMessage(_logger, "Error processing audio file", ex,
                    $"Ошибка обработки аудио файла: {ex.Message}");
            });
        }
    }
} 