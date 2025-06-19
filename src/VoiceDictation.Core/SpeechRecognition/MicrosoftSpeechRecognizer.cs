using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.CognitiveServices.Speech;
using Microsoft.CognitiveServices.Speech.Audio;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Speech recognizer that uses Microsoft Cognitive Services Speech API
    /// </summary>
    public class MicrosoftSpeechRecognizer : ISpeechRecognizer
    {
        private readonly ILogger<MicrosoftSpeechRecognizer> _logger;
        private SpeechConfig? _speechConfig;
        private SpeechRecognizer? _recognizer;
        private bool _isListening;
        private string _language = "ru-RU";
        private string? _region;
        private string? _apiKey;
        private ProxySettings? _proxySettings;
        private CancellationTokenSource? _continuousRecognitionCts;
        private bool _isDisposed;
        
        /// <inheritdoc/>
        public event EventHandler<SpeechRecognizedEventArgs>? SpeechRecognized;
        
        /// <inheritdoc/>
        public event EventHandler<SpeechErrorEventArgs>? SpeechError;
        
        /// <inheritdoc/>
        public event EventHandler<SpeechStatusChangedEventArgs>? StatusChanged;
        
        /// <inheritdoc/>
        public bool IsListening => _isListening;
        
        /// <inheritdoc/>
        public string Language
        {
            get => _language;
            set
            {
                if (_language != value)
                {
                    _language = value;
                    RecreateRecognizerIfNeeded();
                }
            }
        }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="MicrosoftSpeechRecognizer"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        public MicrosoftSpeechRecognizer(ILogger<MicrosoftSpeechRecognizer> logger)
        {
            _logger = logger;
        }
        
        /// <inheritdoc/>
        public void ConfigureWithApiKey(string apiKey)
        {
            if (apiKey.Contains(':'))
            {
                var parts = apiKey.Split(':');
                _apiKey = parts[0];
                _region = parts[1];
            }
            else
            {
                _apiKey = apiKey;
                _region = "westus";
            }
            
            RecreateRecognizerIfNeeded();
        }
        
        /// <inheritdoc/>
        public void ConfigureProxy(ProxySettings proxySettings)
        {
            _proxySettings = proxySettings;
            
            if (!string.IsNullOrEmpty(proxySettings.HttpProxy))
            {
                WebRequest.DefaultWebProxy = new WebProxy(proxySettings.HttpProxy);
                
                if (!string.IsNullOrEmpty(proxySettings.Username) && !string.IsNullOrEmpty(proxySettings.Password))
                {
                    WebRequest.DefaultWebProxy.Credentials = new NetworkCredential(
                        proxySettings.Username, proxySettings.Password);
                }
            }
        }
        
        /// <inheritdoc/>
        public IEnumerable<LanguageInfo> GetAvailableLanguages()
        {
            return new List<LanguageInfo>
            {
                new LanguageInfo("ru-RU", "Russian"),
                new LanguageInfo("en-US", "English (US)"),
                new LanguageInfo("en-GB", "English (UK)"),
                new LanguageInfo("fr-FR", "French"),
                new LanguageInfo("de-DE", "German"),
                new LanguageInfo("es-ES", "Spanish")
            };
        }
        
        /// <inheritdoc/>
        public async Task<SpeechRecognitionResult> RecognizeFromFileAsync(string filePath, CancellationToken cancellationToken = default)
        {
            if (string.IsNullOrEmpty(_apiKey))
            {
                throw new InvalidOperationException("API key not configured. Call ConfigureWithApiKey first.");
            }
            
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException("Audio file not found", filePath);
            }
            
            try
            {
                EnsureSpeechConfigCreated();
                
                OnStatusChanged(SpeechRecognitionStatus.Processing, $"Processing audio file: {Path.GetFileName(filePath)}");
                
                using var audioConfig = AudioConfig.FromWavFileInput(filePath);
                using var fileRecognizer = new SpeechRecognizer(_speechConfig, audioConfig);
                
                var result = await fileRecognizer.RecognizeOnceAsync().ConfigureAwait(false);
                
                return ProcessRecognitionResult(result);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error recognizing speech from file");
                OnSpeechError(new SpeechErrorEventArgs($"Error recognizing speech from file: {ex.Message}"));
                OnStatusChanged(SpeechRecognitionStatus.Error);
                return new SpeechRecognitionResult($"Error recognizing speech: {ex.Message}");
            }
        }
        
        /// <inheritdoc/>
        public async Task<SpeechRecognitionResult> RecognizeOnceAsync(int maxDurationInSeconds = 30, CancellationToken cancellationToken = default)
        {
            if (string.IsNullOrEmpty(_apiKey))
            {
                throw new InvalidOperationException("API key not configured. Call ConfigureWithApiKey first.");
            }
            
            try
            {
                EnsureSpeechConfigCreated();
                
                using var audioConfig = AudioConfig.FromDefaultMicrophoneInput();
                using var micRecognizer = new SpeechRecognizer(_speechConfig, audioConfig);
                
                var stopRecognitionTcs = new TaskCompletionSource<bool>();
                
                var timeoutTask = Task.Delay(maxDurationInSeconds * 1000, cancellationToken)
                    .ContinueWith(_ =>
                    {
                        stopRecognitionTcs.TrySetResult(true);
                    }, cancellationToken);
                
                OnStatusChanged(SpeechRecognitionStatus.Listening, $"Listening for {maxDurationInSeconds} seconds...");
                
                var resultTask = micRecognizer.RecognizeOnceAsync();
                await Task.WhenAny(resultTask, timeoutTask);
                
                if (resultTask.IsCompleted)
                {
                    var result = await resultTask;
                    return ProcessRecognitionResult(result);
                }
                else
                {
                    return new SpeechRecognitionResult("Recognition timed out");
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error recognizing speech from microphone");
                OnSpeechError(new SpeechErrorEventArgs($"Error recognizing speech: {ex.Message}"));
                OnStatusChanged(SpeechRecognitionStatus.Error);
                return new SpeechRecognitionResult($"Error recognizing speech: {ex.Message}");
            }
        }
        
        /// <inheritdoc/>
        public async Task StartContinuousRecognitionAsync(CancellationToken cancellationToken = default)
        {
            if (_isListening)
            {
                return;
            }
            
            if (string.IsNullOrEmpty(_apiKey))
            {
                throw new InvalidOperationException("API key not configured. Call ConfigureWithApiKey first.");
            }
            
            try
            {
                EnsureSpeechConfigCreated();
                EnsureRecognizerCreated();
                
                if (_recognizer == null)
                {
                    throw new InvalidOperationException("Failed to create speech recognizer");
                }
                
                _isListening = true;
                _continuousRecognitionCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                
                SetupRecognitionEventHandlers();
                
                OnStatusChanged(SpeechRecognitionStatus.Initializing, "Starting continuous recognition");
                
                await _recognizer.StartContinuousRecognitionAsync().ConfigureAwait(false);
                
                OnStatusChanged(SpeechRecognitionStatus.Listening, "Listening for speech...");
                
                _ = Task.Run(async () =>
                {
                    try
                    {
                        await Task.Delay(-1, _continuousRecognitionCts.Token);
                    }
                    catch (OperationCanceledException)
                    {
                        await StopContinuousRecognitionAsync();
                    }
                }, _continuousRecognitionCts.Token);
            }
            catch (Exception ex)
            {
                _isListening = false;
                _logger.LogError(ex, "Error starting continuous recognition");
                OnSpeechError(new SpeechErrorEventArgs($"Error starting continuous recognition: {ex.Message}"));
                OnStatusChanged(SpeechRecognitionStatus.Error);
                throw;
            }
        }
        
        /// <inheritdoc/>
        public async Task StopContinuousRecognitionAsync()
        {
            if (!_isListening || _recognizer == null)
            {
                return;
            }
            
            try
            {
                OnStatusChanged(SpeechRecognitionStatus.Processing, "Stopping continuous recognition");
                
                await _recognizer.StopContinuousRecognitionAsync().ConfigureAwait(false);
                
                _continuousRecognitionCts?.Cancel();
                
                RemoveRecognitionEventHandlers();
                
                _isListening = false;
                OnStatusChanged(SpeechRecognitionStatus.Idle, "Continuous recognition stopped");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error stopping continuous recognition");
                OnSpeechError(new SpeechErrorEventArgs($"Error stopping continuous recognition: {ex.Message}"));
            }
        }
        
        /// <inheritdoc/>
        public void Dispose()
        {
            if (_isDisposed)
            {
                return;
            }
            
            if (_isListening)
            {
                StopContinuousRecognitionAsync().Wait();
            }
            
            RemoveRecognitionEventHandlers();
            
            _recognizer?.Dispose();
            _recognizer = null;
            
            _speechConfig = null;
            
            _continuousRecognitionCts?.Dispose();
            _continuousRecognitionCts = null;
            
            _isDisposed = true;
        }
        
        /// <summary>
        /// Creates the speech configuration if not already created
        /// </summary>
        private void EnsureSpeechConfigCreated()
        {
            if (_speechConfig != null)
            {
                return;
            }
            
            if (string.IsNullOrEmpty(_apiKey) || string.IsNullOrEmpty(_region))
            {
                throw new InvalidOperationException("API key and region not configured. Call ConfigureWithApiKey first.");
            }
            
            _speechConfig = SpeechConfig.FromSubscription(_apiKey, _region);
            
            _speechConfig.SpeechRecognitionLanguage = _language;
            
            _speechConfig.OutputFormat = OutputFormat.Detailed;
        }
        
        /// <summary>
        /// Creates the speech recognizer if not already created
        /// </summary>
        private void EnsureRecognizerCreated()
        {
            if (_recognizer != null)
            {
                return;
            }
            
            EnsureSpeechConfigCreated();
            
            var audioConfig = AudioConfig.FromDefaultMicrophoneInput();
            
            _recognizer = new SpeechRecognizer(_speechConfig, audioConfig);
        }
        
        /// <summary>
        /// Recreates the recognizer if needed (e.g., after language change)
        /// </summary>
        private void RecreateRecognizerIfNeeded()
        {
            if (_speechConfig == null)
            {
                return;
            }
            
            _speechConfig.SpeechRecognitionLanguage = _language;
            
            if (_recognizer != null)
            {
                var wasListening = _isListening;
                
                if (wasListening)
                {
                    StopContinuousRecognitionAsync().Wait();
                }
                
                RemoveRecognitionEventHandlers();
                _recognizer.Dispose();
                _recognizer = null;
                
                EnsureRecognizerCreated();
                
                if (wasListening)
                {
                    StartContinuousRecognitionAsync().Wait();
                }
            }
        }
        
        /// <summary>
        /// Sets up event handlers for the speech recognizer
        /// </summary>
        private void SetupRecognitionEventHandlers()
        {
            if (_recognizer == null)
            {
                return;
            }
            
            _recognizer.Recognizing += OnRecognizing;
            _recognizer.Recognized += OnRecognized;
            _recognizer.Canceled += OnCanceled;
            _recognizer.SessionStarted += OnSessionStarted;
            _recognizer.SessionStopped += OnSessionStopped;
        }
        
        /// <summary>
        /// Removes event handlers from the speech recognizer
        /// </summary>
        private void RemoveRecognitionEventHandlers()
        {
            if (_recognizer == null)
            {
                return;
            }
            
            _recognizer.Recognizing -= OnRecognizing;
            _recognizer.Recognized -= OnRecognized;
            _recognizer.Canceled -= OnCanceled;
            _recognizer.SessionStarted -= OnSessionStarted;
            _recognizer.SessionStopped -= OnSessionStopped;
        }
        
        private void OnRecognizing(object? sender, SpeechRecognitionEventArgs e)
        {
            OnStatusChanged(SpeechRecognitionStatus.SpeechDetected, $"Recognizing: {e.Result.Text}");
        }
        
        private void OnRecognized(object? sender, SpeechRecognitionEventArgs e)
        {
            var result = ProcessRecognitionResult(e.Result);
            
            if (result.IsSuccessful)
            {
                OnSpeechRecognized(new SpeechRecognizedEventArgs(result));
                OnStatusChanged(SpeechRecognitionStatus.Recognized, $"Recognized: {result.Text}");
            }
        }
        
        private void OnCanceled(object? sender, SpeechRecognitionCanceledEventArgs e)
        {
            var errorMessage = $"Recognition canceled: {e.Reason}";
            if (e.Reason == CancellationReason.Error)
            {
                errorMessage += $", Error Code: {e.ErrorCode}, Error Details: {e.ErrorDetails}";
            }
            
            _logger.LogWarning(errorMessage);
            OnSpeechError(new SpeechErrorEventArgs(errorMessage, e.ErrorCode.ToString()));
            OnStatusChanged(SpeechRecognitionStatus.Error, errorMessage);
        }
        
        private void OnSessionStarted(object? sender, SessionEventArgs e)
        {
            OnStatusChanged(SpeechRecognitionStatus.Listening, $"Session started: {e.SessionId}");
        }
        
        private void OnSessionStopped(object? sender, SessionEventArgs e)
        {
            OnStatusChanged(SpeechRecognitionStatus.Idle, $"Session stopped: {e.SessionId}");
        }
        
        /// <summary>
        /// Processes recognition result from Microsoft Speech API
        /// </summary>
        private SpeechRecognitionResult ProcessRecognitionResult(Microsoft.CognitiveServices.Speech.SpeechRecognitionResult result)
        {
            switch (result.Reason)
            {
                case ResultReason.RecognizedSpeech:
                    if (!string.IsNullOrWhiteSpace(result.Text))
                    {
                        float confidence = 0.8f;
                        string detectedLanguage = _language;
                        
                        string? nBestJson = null;
                        
                        try
                        {
                            nBestJson = result.Properties?.GetProperty("NBest");
                        }
                        catch (Exception ex)
                        {
                            _logger.LogDebug("Failed to get NBest property: {Message}", ex.Message);
                        }
                        
                        if (!string.IsNullOrEmpty(nBestJson))
                        {
                            var (extractedConfidence, extractedLanguage) = ExtractConfidenceAndLanguageFromNBest(nBestJson);
                            
                            if (extractedConfidence > 0)
                            {
                                confidence = extractedConfidence;
                            }
                            
                            if (!string.IsNullOrEmpty(extractedLanguage))
                            {
                                detectedLanguage = extractedLanguage;
                            }
                        }
                        
                        string processedText = ProcessMixedLanguageText(result.Text, detectedLanguage);
                        
                        return new SpeechRecognitionResult(processedText, confidence, detectedLanguage);
                    }
                    else
                    {
                        return new SpeechRecognitionResult("Recognized but got empty text");
                    }
                    
                case ResultReason.NoMatch:
                    return new SpeechRecognitionResult("No speech could be recognized");
                    
                case ResultReason.Canceled:
                    return new SpeechRecognitionResult("Recognition canceled");
                    
                default:
                    return new SpeechRecognitionResult($"Recognition failed with reason: {result.Reason}");
            }
        }
        
        /// <summary>
        /// Extracts confidence score and language from NBest JSON
        /// </summary>
        private (float Confidence, string Language) ExtractConfidenceAndLanguageFromNBest(string nBestJson)
        {
            try
            {
                var nBestArray = JArray.Parse(nBestJson);
                
                if (nBestArray.Count == 0)
                {
                    return (0f, string.Empty);
                }
                
                var bestResult = nBestArray[0];
                
                float confidence = 0.8f;
                if (bestResult["Confidence"] != null)
                {
                    confidence = bestResult["Confidence"].Value<float>();
                }
                else if (bestResult["confidence"] != null)
                {
                    confidence = bestResult["confidence"].Value<float>();
                }
                
                string language = _language;
                if (bestResult["Language"] != null)
                {
                    language = bestResult["Language"].Value<string>();
                }
                else if (bestResult["language"] != null)
                {
                    language = bestResult["language"].Value<string>();
                }
                else if (bestResult["DetectedLanguage"] != null)
                {
                    language = bestResult["DetectedLanguage"].Value<string>();
                }
                else if (bestResult["detectedLanguage"] != null)
                {
                    language = bestResult["detectedLanguage"].Value<string>();
                }
                
                _logger.LogDebug("Extracted confidence: {Confidence}, language: {Language} from NBest JSON", confidence, language);
                
                return (confidence, language);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error parsing NBest JSON: {Json}", nBestJson);
                return (0.8f, _language); 
            }
        }
        
        /// <summary>
        /// Processes text with mixed language content
        /// </summary>
        private string ProcessMixedLanguageText(string text, string primaryLanguage)
        {
            if (string.IsNullOrWhiteSpace(text))
            {
                return text;
            }
            
            try
            {
                var cyrillic = new HashSet<char>("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ");
                var latin = new HashSet<char>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
                
                bool hasCyrillic = text.Any(c => cyrillic.Contains(c));
                bool hasLatin = text.Any(c => latin.Contains(c));
                
                if (!(hasCyrillic && hasLatin))
                {
                    return text;
                }
                
                var words = text.Split(new[] { ' ', '\t', '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);
                var processedWords = new List<string>();
                
                foreach (var word in words)
                {
                    if (string.IsNullOrWhiteSpace(word))
                    {
                        processedWords.Add(word);
                        continue;
                    }
                    
                    int cyrillicCount = word.Count(c => cyrillic.Contains(c));
                    int latinCount = word.Count(c => latin.Contains(c));
                    
                    string wordScript = cyrillicCount > latinCount ? "cyrillic" : 
                                        latinCount > cyrillicCount ? "latin" : 
                                        "mixed";
                    
                    if (primaryLanguage.StartsWith("ru", StringComparison.OrdinalIgnoreCase))
                    {
                        if (wordScript == "latin")
                        {
                            if (IsLikelyTechnicalTerm(word))
                            {
                                processedWords.Add(word);
                            }
                            else if (word.Length > 0 && char.IsUpper(word[0]))
                            {
                                processedWords.Add(word);
                            }
                            else
                            {
                                processedWords.Add(word);
                            }
                        }
                        else
                        {
                            processedWords.Add(word);
                        }
                    }
                    else
                    {
                        if (wordScript == "cyrillic")
                        {
                            processedWords.Add(word);
                        }
                        else
                        {
                            processedWords.Add(word);
                        }
                    }
                }
                
                return string.Join(" ", processedWords);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error processing mixed language text");
                return text;
            }
        }
        
        /// <summary>
        /// Determines if a word is likely a technical term
        /// </summary>
        private bool IsLikelyTechnicalTerm(string word)
        {
            if (string.IsNullOrWhiteSpace(word) || word.Length < 2)
            {
                return false;
            }
            
            var technicalMarkers = new[] { 
                "api", "sdk", "http", "json", "xml", "url", "sql", "db", 
                "app", "dev", "git", "npm", "css", "html", "js", "py"
            };
            
            string loweredWord = word.ToLowerInvariant();
            
            foreach (var marker in technicalMarkers)
            {
                if (loweredWord.Contains(marker))
                {
                    return true;
                }
            }
            
            bool hasLower = word.Any(char.IsLower);
            bool hasUpper = word.Any(char.IsUpper);
            
            if (hasLower && hasUpper && !word.All(c => char.IsLetterOrDigit(c) || c == '_'))
            {
                bool hasMixedCase = false;
                for (int i = 1; i < word.Length; i++)
                {
                    if (char.IsLower(word[i-1]) && char.IsUpper(word[i]))
                    {
                        hasMixedCase = true;
                        break;
                    }
                }
                
                if (hasMixedCase)
                {
                    return true;
                }
            }
            
            return false;
        }
        
        private void OnSpeechRecognized(SpeechRecognizedEventArgs e)
        {
            SpeechRecognized?.Invoke(this, e);
        }
        
        private void OnSpeechError(SpeechErrorEventArgs e)
        {
            SpeechError?.Invoke(this, e);
        }
        
        private void OnStatusChanged(SpeechRecognitionStatus status, string details = "")
        {
            StatusChanged?.Invoke(this, new SpeechStatusChangedEventArgs(status, details));
        }
    }
} 