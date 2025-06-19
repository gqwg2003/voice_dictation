using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.Logging;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using VoiceDictation.Core.SpeechRecognition;
using VoiceDictation.Network.Proxy;

namespace VoiceDictation.UI.ViewModels
{
    /// <summary>
    /// View model for application settings
    /// </summary>
    public class SettingsViewModel : ObservableObject
    {
        private readonly ILogger<SettingsViewModel> _logger;
        private readonly ISpeechRecognizer _speechRecognizer;
        private readonly IProxyManager _proxyManager;
        
        private string _apiKey = string.Empty;
        private bool _isBusy;
        private string _statusMessage = string.Empty;
        
        /// <summary>
        /// Gets or sets the API key for speech recognition
        /// </summary>
        public string ApiKey
        {
            get => _apiKey;
            set => SetProperty(ref _apiKey, value);
        }
        
        /// <summary>
        /// Gets or sets a value indicating whether the view model is busy
        /// </summary>
        public bool IsBusy
        {
            get => _isBusy;
            set => SetProperty(ref _isBusy, value);
        }
        
        /// <summary>
        /// Gets or sets the status message
        /// </summary>
        public string StatusMessage
        {
            get => _statusMessage;
            set => SetProperty(ref _statusMessage, value);
        }
        
        /// <summary>
        /// Gets the available proxies
        /// </summary>
        public ObservableCollection<ProxyConfig> Proxies { get; } = new ObservableCollection<ProxyConfig>();
        
        private ProxyConfig? _selectedProxy;
        
        /// <summary>
        /// Gets or sets the selected proxy
        /// </summary>
        public ProxyConfig? SelectedProxy
        {
            get => _selectedProxy;
            set => SetProperty(ref _selectedProxy, value);
        }
        
        private string _newProxyName = string.Empty;
        private string _newProxyUrl = string.Empty;
        private string _newProxyUsername = string.Empty;
        private string _newProxyPassword = string.Empty;
        
        /// <summary>
        /// Gets or sets the name for a new proxy
        /// </summary>
        public string NewProxyName
        {
            get => _newProxyName;
            set => SetProperty(ref _newProxyName, value);
        }
        
        /// <summary>
        /// Gets or sets the URL for a new proxy
        /// </summary>
        public string NewProxyUrl
        {
            get => _newProxyUrl;
            set => SetProperty(ref _newProxyUrl, value);
        }
        
        /// <summary>
        /// Gets or sets the username for a new proxy
        /// </summary>
        public string NewProxyUsername
        {
            get => _newProxyUsername;
            set => SetProperty(ref _newProxyUsername, value);
        }
        
        /// <summary>
        /// Gets or sets the password for a new proxy
        /// </summary>
        public string NewProxyPassword
        {
            get => _newProxyPassword;
            set => SetProperty(ref _newProxyPassword, value);
        }
        
        /// <summary>
        /// Gets the command to save the API key
        /// </summary>
        public IRelayCommand SaveApiKeyCommand { get; }
        
        /// <summary>
        /// Gets the command to test the API key
        /// </summary>
        public IRelayCommand TestApiKeyCommand { get; }
        
        /// <summary>
        /// Gets the command to load proxies
        /// </summary>
        public IRelayCommand LoadProxiesCommand { get; }
        
        /// <summary>
        /// Gets the command to add a new proxy
        /// </summary>
        public IRelayCommand AddProxyCommand { get; }
        
        /// <summary>
        /// Gets the command to remove a proxy
        /// </summary>
        public IRelayCommand<ProxyConfig> RemoveProxyCommand { get; }
        
        /// <summary>
        /// Gets the command to select a proxy
        /// </summary>
        public IRelayCommand<ProxyConfig> SelectProxyCommand { get; }
        
        /// <summary>
        /// Gets the command to test a proxy
        /// </summary>
        public IRelayCommand<ProxyConfig> TestProxyCommand { get; }
        
        /// <summary>
        /// Gets the command to detect system proxy
        /// </summary>
        public IRelayCommand DetectSystemProxyCommand { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SettingsViewModel"/> class
        /// </summary>
        /// <param name="logger">The logger</param>
        /// <param name="speechRecognizer">The speech recognizer</param>
        /// <param name="proxyManager">The proxy manager</param>
        public SettingsViewModel(ILogger<SettingsViewModel> logger, ISpeechRecognizer speechRecognizer, IProxyManager proxyManager)
        {
            _logger = logger;
            _speechRecognizer = speechRecognizer;
            _proxyManager = proxyManager;
            
            SaveApiKeyCommand = new RelayCommand(SaveApiKey);
            TestApiKeyCommand = new RelayCommand(TestApiKey);
            LoadProxiesCommand = new AsyncRelayCommand(LoadProxiesAsync);
            AddProxyCommand = new AsyncRelayCommand(AddProxyAsync, CanAddProxy);
            RemoveProxyCommand = new AsyncRelayCommand<ProxyConfig>(RemoveProxyAsync);
            SelectProxyCommand = new AsyncRelayCommand<ProxyConfig>(SelectProxyAsync);
            TestProxyCommand = new AsyncRelayCommand<ProxyConfig>(TestProxyAsync);
            DetectSystemProxyCommand = new AsyncRelayCommand(DetectSystemProxyAsync);
            
            _proxyManager.StatusChanged += ProxyManager_StatusChanged;
        }
        
        /// <summary>
        /// Initializes the view model
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        public async Task InitializeAsync()
        {
            try
            {
                IsBusy = true;
                
                // Load proxies
                await LoadProxiesAsync();
                
                StatusMessage = "Настройки загружены";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error initializing SettingsViewModel");
                StatusMessage = $"Ошибка инициализации: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Cleans up resources used by the view model
        /// </summary>
        public void Cleanup()
        {
            _proxyManager.StatusChanged -= ProxyManager_StatusChanged;
        }
        
        /// <summary>
        /// Saves the API key
        /// </summary>
        private void SaveApiKey()
        {
            try
            {
                if (string.IsNullOrWhiteSpace(ApiKey))
                {
                    StatusMessage = "Введите API ключ";
                    return;
                }
                
                // Configure speech recognizer with API key
                _speechRecognizer.ConfigureWithApiKey(ApiKey);
                
                StatusMessage = "API ключ сохранен";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error saving API key");
                StatusMessage = $"Ошибка сохранения API ключа: {ex.Message}";
            }
        }
        
        /// <summary>
        /// Tests the API key
        /// </summary>
        private async void TestApiKey()
        {
            if (string.IsNullOrWhiteSpace(ApiKey))
            {
                StatusMessage = "Введите API ключ";
                return;
            }
            
            try
            {
                IsBusy = true;
                StatusMessage = "Тестирование API ключа...";
                
                // Configure speech recognizer with API key
                _speechRecognizer.ConfigureWithApiKey(ApiKey);
                
                var result = await _speechRecognizer.RecognizeOnceAsync(5);
                
                if (result.IsSuccessful)
                {
                    StatusMessage = "API ключ успешно протестирован";
                }
                else
                {
                    StatusMessage = $"Ошибка тестирования API ключа: {result.ErrorMessage}";
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error testing API key");
                StatusMessage = $"Ошибка тестирования API ключа: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Loads the available proxies
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task LoadProxiesAsync()
        {
            try
            {
                IsBusy = true;
                StatusMessage = "Загрузка прокси...";
                
                Proxies.Clear();
                
                var proxies = await _proxyManager.GetProxyListAsync();
                
                foreach (var proxy in proxies)
                {
                    Proxies.Add(proxy);
                }
                
                if (_proxyManager.IsProxyActive && _proxyManager.CurrentProxy != null)
                {
                    SelectedProxy = _proxyManager.CurrentProxy;
                }
                
                StatusMessage = $"Загружено {Proxies.Count} прокси";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading proxies");
                StatusMessage = $"Ошибка загрузки прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Determines whether a new proxy can be added
        /// </summary>
        /// <returns>True if a proxy can be added, otherwise false</returns>
        private bool CanAddProxy()
        {
            return !string.IsNullOrWhiteSpace(NewProxyUrl);
        }
        
        /// <summary>
        /// Adds a new proxy
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task AddProxyAsync()
        {
            if (string.IsNullOrWhiteSpace(NewProxyUrl))
            {
                StatusMessage = "Введите URL прокси";
                return;
            }
            
            try
            {
                IsBusy = true;
                StatusMessage = "Добавление прокси...";
                
                // Create new proxy config
                var proxyConfig = new ProxyConfig
                {
                    Id = Guid.NewGuid().ToString(),
                    Name = string.IsNullOrWhiteSpace(NewProxyName) ? $"Прокси {Proxies.Count + 1}" : NewProxyName,
                    HttpProxy = NewProxyUrl,
                    HttpsProxy = NewProxyUrl,
                    Username = NewProxyUsername,
                    Password = NewProxyPassword
                };
                
                // Add to proxy manager
                await _proxyManager.AddProxyAsync(proxyConfig);
                
                Proxies.Add(proxyConfig);
                
                NewProxyName = string.Empty;
                NewProxyUrl = string.Empty;
                NewProxyUsername = string.Empty;
                NewProxyPassword = string.Empty;
                
                StatusMessage = $"Прокси {proxyConfig.Name} добавлен";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error adding proxy");
                StatusMessage = $"Ошибка добавления прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Removes a proxy
        /// </summary>
        /// <param name="proxy">The proxy to remove</param>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task RemoveProxyAsync(ProxyConfig? proxy)
        {
            if (proxy == null)
            {
                return;
            }
            
            try
            {
                IsBusy = true;
                StatusMessage = $"Удаление прокси {proxy.Name}...";
                
                await _proxyManager.RemoveProxyAsync(proxy.Id);
                
                Proxies.Remove(proxy);
                
                if (SelectedProxy == proxy)
                {
                    SelectedProxy = null;
                }
                
                StatusMessage = $"Прокси {proxy.Name} удален";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error removing proxy {ProxyId}", proxy.Id);
                StatusMessage = $"Ошибка удаления прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Selects a proxy
        /// </summary>
        /// <param name="proxy">The proxy to select</param>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task SelectProxyAsync(ProxyConfig? proxy)
        {
            if (proxy == null)
            {
                await _proxyManager.DeactivateProxyAsync();
                SelectedProxy = null;
                StatusMessage = "Прокси отключен";
                return;
            }
            
            try
            {
                IsBusy = true;
                StatusMessage = $"Выбор прокси {proxy.Name}...";
                
                await _proxyManager.SelectProxyAsync(proxy.Id);
                
                SelectedProxy = proxy;
                
                var proxySettings = new ProxySettings
                {
                    HttpProxy = proxy.HttpProxy,
                    HttpsProxy = proxy.HttpsProxy,
                    Username = proxy.Username,
                    Password = proxy.Password
                };
                
                _speechRecognizer.ConfigureProxy(proxySettings);
                
                StatusMessage = $"Прокси {proxy.Name} выбран";
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error selecting proxy {ProxyId}", proxy.Id);
                StatusMessage = $"Ошибка выбора прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Tests a proxy
        /// </summary>
        /// <param name="proxy">The proxy to test</param>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task TestProxyAsync(ProxyConfig? proxy)
        {
            if (proxy == null)
            {
                return;
            }
            
            try
            {
                IsBusy = true;
                StatusMessage = $"Тестирование прокси {proxy.Name}...";
                
                var result = await _proxyManager.TestProxyAsync(proxy.Id);
                
                if (result.IsSuccessful)
                {
                    StatusMessage = $"Прокси {proxy.Name} успешно протестирован (время отклика: {result.ResponseTimeMs} мс)";
                }
                else
                {
                    StatusMessage = $"Ошибка тестирования прокси {proxy.Name}: {result.ErrorMessage}";
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error testing proxy {ProxyId}", proxy.Id);
                StatusMessage = $"Ошибка тестирования прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Detects the system proxy
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        private async Task DetectSystemProxyAsync()
        {
            try
            {
                IsBusy = true;
                StatusMessage = "Определение системного прокси...";
                
                var systemProxy = await _proxyManager.DetectSystemProxyAsync();
                
                if (systemProxy != null)
                {
                    if (!Proxies.Any(p => p.Id == systemProxy.Id))
                    {
                        Proxies.Add(systemProxy);
                    }
                    
                    StatusMessage = $"Системный прокси обнаружен: {systemProxy.HttpProxy}";
                }
                else
                {
                    StatusMessage = "Системный прокси не обнаружен";
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error detecting system proxy");
                StatusMessage = $"Ошибка определения системного прокси: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }
        
        /// <summary>
        /// Handles the StatusChanged event of the proxy manager
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="e">The event arguments</param>
        private void ProxyManager_StatusChanged(object? sender, ProxyStatusChangedEventArgs e)
        {
            if (!string.IsNullOrEmpty(e.Message))
            {
                StatusMessage = e.Message;
            }
            
            if (e.Status == ProxyStatus.Active && e.ProxyConfig != null)
            {
                SelectedProxy = e.ProxyConfig;
            }
            else if (e.Status == ProxyStatus.Inactive)
            {
                SelectedProxy = null;
            }
        }
    }
} 