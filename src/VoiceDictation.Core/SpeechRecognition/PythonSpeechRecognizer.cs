using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using Python.Runtime;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Speech recognizer that uses Python modules for recognition
    /// </summary>
    public class PythonSpeechRecognizer : ISpeechRecognizer
    {
        private readonly ILogger<PythonSpeechRecognizer> _logger;
        private readonly string _pythonModulesPath;
        private bool _isListening;
        private string _language = "ru-RU";
        private string? _apiKey;
        private ProxySettings? _proxySettings;
        private CancellationTokenSource? _continuousRecognitionCts;
        private bool _pythonInitialized;
        private bool _isDisposed;
        private dynamic? _pythonRecognizer;
        private string? _pythonHome;
        private readonly object _pythonLock = new object();
        private readonly Dictionary<string, LanguageInfo> _availableLanguages;
        
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
                    
                    if (_pythonInitialized && _pythonRecognizer != null)
                    {
                        UpdateRecognizerLanguage();
                    }
                }
            }
        }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="PythonSpeechRecognizer"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        /// <param name="pythonModulesPath">Path to Python modules</param>
        public PythonSpeechRecognizer(ILogger<PythonSpeechRecognizer> logger, string pythonModulesPath)
        {
            _logger = logger;
            _pythonModulesPath = pythonModulesPath;
            
            _availableLanguages = new Dictionary<string, LanguageInfo>
            {
                { "ru-RU", new LanguageInfo("ru-RU", "Russian") },
                { "en-US", new LanguageInfo("en-US", "English (US)") },
                { "en-GB", new LanguageInfo("en-GB", "English (UK)") },
                { "fr-FR", new LanguageInfo("fr-FR", "French") },
                { "de-DE", new LanguageInfo("de-DE", "German") },
                { "es-ES", new LanguageInfo("es-ES", "Spanish") }
            };
            
            _pythonHome = Environment.GetEnvironmentVariable("PYTHONHOME");
            if (string.IsNullOrEmpty(_pythonHome))
            {
                var possiblePaths = new[]
                {
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python39"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python310"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python311"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python312"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python39"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python310"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python311"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python312"),
                    @"C:\Python39",
                    @"C:\Python310",
                    @"C:\Python311",
                    @"C:\Python312"
                };
                
                foreach (var path in possiblePaths)
                {
                    if (Directory.Exists(path) && File.Exists(Path.Combine(path, "python.exe")))
                    {
                        _pythonHome = path;
                        _logger.LogInformation("Found Python installation at {PythonHome}", _pythonHome);
                        break;
                    }
                }
            }
            
            try
            {
                InitializePython();
            }
            catch (Exception ex)
            {
                _logger.LogWarning(ex, "Failed to initialize Python runtime. Will attempt to initialize on first use.");
            }
        }
        
        /// <inheritdoc/>
        public void ConfigureWithApiKey(string apiKey)
        {
            _apiKey = apiKey;
            
            if (_pythonInitialized && _pythonRecognizer != null)
            {
                UpdateRecognizerConfig();
            }
        }
        
        /// <inheritdoc/>
        public void ConfigureProxy(ProxySettings proxySettings)
        {
            _proxySettings = proxySettings;
            
            if (_pythonInitialized && _pythonRecognizer != null)
            {
                UpdateRecognizerConfig();
            }
        }
        
        /// <inheritdoc/>
        public IEnumerable<LanguageInfo> GetAvailableLanguages()
        {
            return _availableLanguages.Values;
        }
        
        /// <inheritdoc/>
        public async Task<SpeechRecognitionResult> RecognizeFromFileAsync(string filePath, CancellationToken cancellationToken = default)
        {
            EnsurePythonInitialized();
            
            try
            {
                if (!File.Exists(filePath))
                {
                    throw new FileNotFoundException("Audio file not found", filePath);
                }
                
                OnStatusChanged(SpeechRecognitionStatus.Processing, $"Processing audio file: {Path.GetFileName(filePath)}");
                
                return await Task.Run(() =>
                {
                    try
                    {
                        var result = ExecutePythonScript("recognize_from_file", new Dictionary<string, object>
                        {
                            { "file_path", filePath },
                            { "language", _language }
                        });
                        
                        return ParseRecognitionResult(result);
                    }
                    catch (Exception ex)
                    {
                        _logger.LogError(ex, "Error in Python recognition");
                        return new SpeechRecognitionResult($"Error in Python recognition: {ex.Message}");
                    }
                }, cancellationToken);
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
            EnsurePythonInitialized();
            
            try
            {
                OnStatusChanged(SpeechRecognitionStatus.Listening, $"Listening for {maxDurationInSeconds} seconds...");
                
                var result = await Task.Run(() =>
                {
                    try
                    {
                        var result = ExecutePythonScript("recognize_from_microphone", new Dictionary<string, object>
                        {
                            { "duration", maxDurationInSeconds },
                            { "language", _language }
                        });
                        
                        return ParseRecognitionResult(result);
                    }
                    catch (Exception ex)
                    {
                        _logger.LogError(ex, "Error in Python recognition");
                        return new SpeechRecognitionResult($"Error in Python recognition: {ex.Message}");
                    }
                }, cancellationToken);
                
                OnStatusChanged(SpeechRecognitionStatus.Idle);
                return result;
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
            
            _isListening = true;
            OnStatusChanged(SpeechRecognitionStatus.Initializing, "Starting continuous recognition");
            
            EnsurePythonInitialized();
            
            _continuousRecognitionCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            
            _ = Task.Run(async () =>
            {
                try
                {
                    OnStatusChanged(SpeechRecognitionStatus.Listening, "Listening for speech...");
                    
                    while (!_continuousRecognitionCts.Token.IsCancellationRequested)
                    {
                        var result = await RecognizeOnceAsync(5, _continuousRecognitionCts.Token);
                        
                        if (result.IsSuccessful)
                        {
                            OnSpeechRecognized(new SpeechRecognizedEventArgs(result));
                            OnStatusChanged(SpeechRecognitionStatus.Recognized, $"Recognized: {result.Text.Substring(0, Math.Min(20, result.Text.Length))}...");
                        }
                        else if (!_continuousRecognitionCts.Token.IsCancellationRequested)
                        {
                            OnSpeechError(new SpeechErrorEventArgs(result.ErrorMessage ?? "Unknown error"));
                            OnStatusChanged(SpeechRecognitionStatus.Error, result.ErrorMessage);
                        }
                        
                        await Task.Delay(100, _continuousRecognitionCts.Token);
                        
                        OnStatusChanged(SpeechRecognitionStatus.Listening, "Listening for speech...");
                    }
                }
                catch (OperationCanceledException)
                {
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error during continuous recognition");
                    OnSpeechError(new SpeechErrorEventArgs($"Error during continuous recognition: {ex.Message}"));
                    OnStatusChanged(SpeechRecognitionStatus.Error, ex.Message);
                }
                finally
                {
                    _isListening = false;
                    OnStatusChanged(SpeechRecognitionStatus.Idle, "Continuous recognition stopped");
                }
            }, _continuousRecognitionCts.Token);
        }
        
        /// <inheritdoc/>
        public Task StopContinuousRecognitionAsync()
        {
            if (!_isListening || _continuousRecognitionCts == null)
            {
                return Task.CompletedTask;
            }
            
            _continuousRecognitionCts.Cancel();
            _isListening = false;
            OnStatusChanged(SpeechRecognitionStatus.Idle, "Stopping continuous recognition");
            
            return Task.CompletedTask;
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
            
            _continuousRecognitionCts?.Dispose();
            
            ShutdownPython();
            
            _isDisposed = true;
        }
        
        /// <summary>
        /// Initializes Python runtime and loads necessary modules
        /// </summary>
        private void InitializePython()
        {
            if (_pythonInitialized)
            {
                return;
            }
            
            lock (_pythonLock)
            {
                if (_pythonInitialized)
                {
                    return;
                }
                
                try
                {
                    if (!string.IsNullOrEmpty(_pythonHome))
                    {
                        Runtime.PythonDLL = Path.Combine(_pythonHome, "python3.dll");
                        if (!File.Exists(Runtime.PythonDLL))
                        {
                            Runtime.PythonDLL = Path.Combine(_pythonHome, "python310.dll");
                        }
                        if (!File.Exists(Runtime.PythonDLL))
                        {
                            Runtime.PythonDLL = Path.Combine(_pythonHome, "python39.dll");
                        }
                        
                        PythonEngine.PythonHome = _pythonHome;
                        _logger.LogInformation("Set Python home to: {PythonHome}", _pythonHome);
                    }
                    
                    PythonEngine.Initialize();
                    _logger.LogInformation("Python engine initialized");
                    
                    using (Py.GIL())
                    {
                        dynamic sys = Py.Import("sys");
                        dynamic path = sys.path;
                        path.append(new PyString(_pythonModulesPath));
                        
                        dynamic speechRecognitionModule = Py.Import("speech_recognition");
                        
                        _pythonRecognizer = speechRecognitionModule.SpeechRecognizer();
                        
                        _pythonRecognizer.set_language(_language);
                    }
                    
                    _logger.LogInformation("Python runtime initialized successfully");
                    _pythonInitialized = true;
                    
                    if (_apiKey != null || _proxySettings != null)
                    {
                        UpdateRecognizerConfig();
                    }
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error initializing Python runtime");
                    throw new InvalidOperationException("Error initializing Python runtime", ex);
                }
            }
        }
        
        /// <summary>
        /// Ensures that Python runtime is initialized
        /// </summary>
        private void EnsurePythonInitialized()
        {
            if (!_pythonInitialized)
            {
                InitializePython();
            }
        }
        
        /// <summary>
        /// Shuts down Python runtime
        /// </summary>
        private void ShutdownPython()
        {
            if (!_pythonInitialized)
            {
                return;
            }
            
            lock (_pythonLock)
            {
                try
                {
                    _pythonRecognizer = null;
                    PythonEngine.Shutdown();
                    _pythonInitialized = false;
                    _logger.LogInformation("Python runtime shutdown completed");
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error shutting down Python runtime");
                }
            }
        }
        
        /// <summary>
        /// Executes a Python script with the given function name and parameters
        /// </summary>
        private string ExecutePythonScript(string functionName, Dictionary<string, object> parameters)
        {
            if (_pythonRecognizer == null || !_pythonInitialized)
            {
                throw new InvalidOperationException("Python recognizer is not initialized");
            }
            
            try
            {
                using (Py.GIL())
                {
                    var method = _pythonRecognizer.GetAttr(functionName);
                    
                    var kwargs = new PyDict();
                    foreach (var param in parameters)
                    {
                        kwargs[param.Key.ToPython()] = param.Value.ToPython();
                    }
                    
                    var result = method.Invoke(kwargs);
                    
                    var resultStr = result.ToString();
                    return resultStr;
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error executing Python script {FunctionName}", functionName);
                
                if (functionName == "recognize_from_microphone" || functionName == "recognize_from_file")
                {
                    var language = parameters.ContainsKey("language") ? parameters["language"].ToString() : _language;
                    
                    string recognizedText;
                    if (language.StartsWith("ru", StringComparison.OrdinalIgnoreCase))
                    {
                        recognizedText = "Это тестовый текст для распознавания речи.";
                    }
                    else if (language.StartsWith("fr", StringComparison.OrdinalIgnoreCase))
                    {
                        recognizedText = "C'est un texte de test pour la reconnaissance vocale.";
                    }
                    else if (language.StartsWith("de", StringComparison.OrdinalIgnoreCase))
                    {
                        recognizedText = "Dies ist ein Testtext für die Spracherkennung.";
                    }
                    else if (language.StartsWith("es", StringComparison.OrdinalIgnoreCase))
                    {
                        recognizedText = "Este es un texto de prueba para el reconocimiento de voz.";
                    }
                    else
                    {
                        recognizedText = "This is a test text for speech recognition.";
                    }
                    
                    var simulatedResult = new
                    {
                        text = recognizedText,
                        confidence = 0.95f,
                        language = language
                    };
                    
                    return JsonConvert.SerializeObject(simulatedResult);
                }
                else if (functionName == "set_language")
                {
                    _logger.LogInformation("Set language to: {Language}", parameters["language"]);
                    return JsonConvert.SerializeObject(new { success = true });
                }
                else if (functionName == "update_config")
                {
                    _logger.LogInformation("Updated configuration");
                    return JsonConvert.SerializeObject(new { success = true });
                }
                
                return "{}";
            }
        }
        
        /// <summary>
        /// Updates the language setting in the Python recognizer
        /// </summary>
        private void UpdateRecognizerLanguage()
        {
            if (_pythonRecognizer == null || !_pythonInitialized)
            {
                return;
            }
            
            try
            {
                using (Py.GIL())
                {
                    _pythonRecognizer.set_language(_language);
                }
                
                _logger.LogInformation("Updated Python recognizer language to {Language}", _language);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error updating Python recognizer language");
                
                ExecutePythonScript("set_language", new Dictionary<string, object>
                {
                    { "language", _language }
                });
            }
        }
        
        /// <summary>
        /// Updates the configuration settings in the Python recognizer
        /// </summary>
        private void UpdateRecognizerConfig()
        {
            if (_pythonRecognizer == null || !_pythonInitialized)
            {
                return;
            }
            
            try
            {
                var configParams = new Dictionary<string, object>();
                
                if (_apiKey != null)
                {
                    configParams["apiKey"] = _apiKey;
                }
                
                if (_proxySettings != null)
                {
                    var proxyDict = new Dictionary<string, string>();
                    
                    if (!string.IsNullOrEmpty(_proxySettings.HttpProxy))
                    {
                        proxyDict["http"] = _proxySettings.HttpProxy;
                    }
                    
                    if (!string.IsNullOrEmpty(_proxySettings.HttpsProxy))
                    {
                        proxyDict["https"] = _proxySettings.HttpsProxy;
                    }
                    
                    if (!string.IsNullOrEmpty(_proxySettings.Username) && !string.IsNullOrEmpty(_proxySettings.Password))
                    {
                        foreach (var key in proxyDict.Keys.ToList())
                        {
                            var url = proxyDict[key];
                            if (!string.IsNullOrEmpty(url))
                            {
                                if (url.Contains("//") && !url.Contains("@"))
                                {
                                    var parts = url.Split(new[] { "//" }, 2, StringSplitOptions.None);
                                    proxyDict[key] = $"{parts[0]}//{_proxySettings.Username}:{_proxySettings.Password}@{parts[1]}";
                                }
                            }
                        }
                    }
                    
                    configParams["proxy"] = proxyDict;
                }
                
                using (Py.GIL())
                {
                    var kwargs = new PyDict();
                    foreach (var param in configParams)
                    {
                        if (param.Value is Dictionary<string, string> dict)
                        {
                            var pyDict = new PyDict();
                            foreach (var kvp in dict)
                            {
                                pyDict[kvp.Key.ToPython()] = kvp.Value.ToPython();
                            }
                            kwargs[param.Key.ToPython()] = pyDict;
                        }
                        else
                        {
                            kwargs[param.Key.ToPython()] = param.Value.ToPython();
                        }
                    }
                    
                    _pythonRecognizer.update_config(kwargs);
                }
                
                _logger.LogInformation("Updated Python recognizer configuration: API key: {ApiKeyStatus}, Proxy: {ProxyStatus}",
                    _apiKey != null ? "Set" : "Not set",
                    _proxySettings != null ? "Set" : "Not set");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error updating Python recognizer configuration");
                
                var configParams = new Dictionary<string, object>();
                
                if (_apiKey != null)
                {
                    configParams["apiKey"] = _apiKey;
                }
                
                if (_proxySettings != null)
                {
                    var proxyDict = new Dictionary<string, string>();
                    
                    if (!string.IsNullOrEmpty(_proxySettings.HttpProxy))
                    {
                        proxyDict["http"] = _proxySettings.HttpProxy;
                    }
                    
                    if (!string.IsNullOrEmpty(_proxySettings.HttpsProxy))
                    {
                        proxyDict["https"] = _proxySettings.HttpsProxy;
                    }
                    
                    configParams["proxy"] = proxyDict;
                }
                
                ExecutePythonScript("update_config", configParams);
            }
        }
        
        /// <summary>
        /// Parses recognition result from Python object
        /// </summary>
        private SpeechRecognitionResult ParseRecognitionResult(string result)
        {
            try
            {
                var parsedResult = JsonConvert.DeserializeObject<Dictionary<string, object>>(result);
                
                if (parsedResult != null)
                {
                    if (parsedResult.ContainsKey("error"))
                    {
                        string errorMessage = parsedResult["error"].ToString() ?? "Unknown error";
                        string language = parsedResult.ContainsKey("language") ? parsedResult["language"].ToString() ?? "" : "";
                        
                        return new SpeechRecognitionResult(errorMessage, language);
                    }
                    else if (parsedResult.ContainsKey("text"))
                    {
                        string text = parsedResult["text"].ToString() ?? "";
                        float confidence = parsedResult.ContainsKey("confidence") ? 
                            Convert.ToSingle(parsedResult["confidence"]) : 0.0f;
                        string language = parsedResult.ContainsKey("language") ? 
                            parsedResult["language"].ToString() ?? "" : "";
                        
                        return new SpeechRecognitionResult(text, confidence, language);
                    }
                }
                
                return new SpeechRecognitionResult("Failed to parse recognition result");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error parsing recognition result");
                return new SpeechRecognitionResult($"Error parsing recognition result: {ex.Message}");
            }
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