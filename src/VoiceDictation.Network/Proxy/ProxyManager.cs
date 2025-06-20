using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace VoiceDictation.Network.Proxy
{
    /// <summary>
    /// Implementation of the proxy manager
    /// </summary>
    public class ProxyManager : IProxyManager
    {
        private readonly ILogger<ProxyManager> _logger;
        private readonly Dictionary<string, ProxyConfig> _proxies = new Dictionary<string, ProxyConfig>();
        private ProxyConfig? _currentProxy;
        private string? _configFilePath;
        private readonly HttpClient _httpClient;
        
        public event EventHandler<ProxyStatusChangedEventArgs>? StatusChanged;
        public bool IsProxyActive => _currentProxy != null;
        public ProxyConfig? CurrentProxy => _currentProxy;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="ProxyManager"/> class
        /// </summary>
        /// <param name="logger">The logger</param>
        /// <param name="httpClientFactory">The HTTP client factory</param>
        public ProxyManager(ILogger<ProxyManager> logger, IHttpClientFactory httpClientFactory)
        {
            _logger = logger;
            _httpClient = httpClientFactory.CreateClient("ProxyTest");
        }
        
        public async Task<IEnumerable<ProxyConfig>> GetProxyListAsync()
        {
            await Task.CompletedTask;
            return _proxies.Values;
        }
        
        public async Task<bool> AddProxyAsync(ProxyConfig config)
        {
            if (string.IsNullOrEmpty(config.Id))
            {
                config.Id = Guid.NewGuid().ToString();
            }
            
            if (string.IsNullOrEmpty(config.Name))
            {
                config.Name = $"Proxy {_proxies.Count + 1}";
            }
            
            if (string.IsNullOrEmpty(config.HttpsProxy) && !string.IsNullOrEmpty(config.HttpProxy))
            {
                config.HttpsProxy = config.HttpProxy;
            }
            
            _proxies[config.Id] = config;
            
            _logger.LogInformation("Added proxy: {ProxyId} ({ProxyName})", config.Id, config.Name);
            
            if (!string.IsNullOrEmpty(_configFilePath))
            {
                await SaveConfigAsync(_configFilePath);
            }
            
            return true;
        }
        
        public async Task<bool> RemoveProxyAsync(string proxyId)
        {
            if (!_proxies.TryGetValue(proxyId, out var proxy))
            {
                _logger.LogWarning("Proxy not found: {ProxyId}", proxyId);
                return false;
            }
            
            if (_currentProxy?.Id == proxyId)
            {
                await DeactivateProxyAsync();
            }
            
            _proxies.Remove(proxyId);
            
            _logger.LogInformation("Removed proxy: {ProxyId} ({ProxyName})", proxyId, proxy.Name);
            
            if (!string.IsNullOrEmpty(_configFilePath))
            {
                await SaveConfigAsync(_configFilePath);
            }
            
            return true;
        }
        
        public async Task<bool> SelectProxyAsync(string proxyId)
        {
            if (!_proxies.TryGetValue(proxyId, out var proxy))
            {
                _logger.LogWarning("Proxy not found: {ProxyId}", proxyId);
                return false;
            }
            
            if (_currentProxy != null)
            {
                await DeactivateProxyAsync();
            }
            
            try
            {
                OnStatusChanged(ProxyStatus.Activating, proxy, $"Activating proxy: {proxy.Name}");
                
                if (!string.IsNullOrEmpty(proxy.HttpProxy))
                {
                    WebRequest.DefaultWebProxy = new WebProxy(proxy.HttpProxy);
                    
                    if (!string.IsNullOrEmpty(proxy.Username) && !string.IsNullOrEmpty(proxy.Password))
                    {
                        WebRequest.DefaultWebProxy.Credentials = new NetworkCredential(proxy.Username, proxy.Password);
                    }
                    else
                    {
                        WebRequest.DefaultWebProxy.Credentials = null;
                    }
                    
                    if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                    {
                        SetSystemProxyWindows(proxy);
                    }
                }
                
                _currentProxy = proxy;
                
                OnStatusChanged(ProxyStatus.Active, proxy, $"Proxy activated: {proxy.Name}");
                
                return true;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error activating proxy: {ProxyId} ({ProxyName})", proxy.Id, proxy.Name);
                OnStatusChanged(ProxyStatus.Error, proxy, $"Error activating proxy: {ex.Message}");
                return false;
            }
        }
        
        public async Task<bool> DeactivateProxyAsync()
        {
            if (_currentProxy == null)
            {
                return true;
            }
            
            var deactivatingProxy = _currentProxy;
            
            try
            {
                OnStatusChanged(ProxyStatus.Deactivating, deactivatingProxy, $"Deactivating proxy: {deactivatingProxy.Name}");
                
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    ClearSystemProxyWindows();
                }
                
                WebRequest.DefaultWebProxy = new WebProxy();
                
                _currentProxy = null;
                
                OnStatusChanged(ProxyStatus.Inactive, deactivatingProxy, $"Proxy deactivated: {deactivatingProxy.Name}");
                
                return true;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error deactivating proxy: {ProxyId} ({ProxyName})", deactivatingProxy.Id, deactivatingProxy.Name);
                OnStatusChanged(ProxyStatus.Error, deactivatingProxy, $"Error deactivating proxy: {ex.Message}");
                return false;
            }
        }
        
        public async Task<ProxyTestResult> TestProxyAsync(string? proxyId = null)
        {
            ProxyConfig proxyToTest;
            
            if (proxyId != null)
            {
                if (!_proxies.TryGetValue(proxyId, out var proxy))
                {
                    _logger.LogWarning("Proxy not found: {ProxyId}", proxyId);
                    return new ProxyTestResult(new ProxyConfig { Id = proxyId }, "Proxy not found");
                }
                
                proxyToTest = proxy;
            }
            else
            {
                proxyToTest = _currentProxy ?? new ProxyConfig { Name = "No proxy" };
            }
            
            try
            {
                var handler = new HttpClientHandler();
                
                if (!string.IsNullOrEmpty(proxyToTest.HttpProxy))
                {
                    handler.Proxy = new WebProxy(proxyToTest.HttpProxy);
                    
                    if (!string.IsNullOrEmpty(proxyToTest.Username) && !string.IsNullOrEmpty(proxyToTest.Password))
                    {
                        handler.Proxy.Credentials = new NetworkCredential(proxyToTest.Username, proxyToTest.Password);
                    }
                }
                
                using var client = new HttpClient(handler, disposeHandler: true);
                client.Timeout = TimeSpan.FromSeconds(10);
                
                const string testUrl = "https://www.google.com";
                
                var stopwatch = System.Diagnostics.Stopwatch.StartNew();
                
                var response = await client.GetAsync(testUrl);
                stopwatch.Stop();
                
                if (response.IsSuccessStatusCode)
                {
                    _logger.LogInformation("Proxy test successful: {ProxyName}, Response time: {ResponseTime}ms", 
                        proxyToTest.Name, stopwatch.ElapsedMilliseconds);
                        
                    return new ProxyTestResult(proxyToTest, stopwatch.ElapsedMilliseconds);
                }
                else
                {
                    _logger.LogWarning("Proxy test failed: {ProxyName}, Status code: {StatusCode}", 
                        proxyToTest.Name, response.StatusCode);
                        
                    return new ProxyTestResult(proxyToTest, $"HTTP error: {response.StatusCode}");
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Proxy test error: {ProxyName}", proxyToTest.Name);
                return new ProxyTestResult(proxyToTest, $"Error: {ex.Message}");
            }
        }
        
        public async Task<ProxyConfig?> DetectSystemProxyAsync()
        {
            try
            {
                var proxyUrl = GetWindowsSystemProxy();
                
                if (string.IsNullOrEmpty(proxyUrl))
                {
                    return null;
                }
                
                var systemProxyId = "system";
                var systemProxy = new ProxyConfig
                {
                    Id = systemProxyId,
                    Name = "System Proxy",
                    HttpProxy = proxyUrl,
                    HttpsProxy = proxyUrl,
                    IsSystemProxy = true
                };
                
                if (_proxies.ContainsKey(systemProxyId))
                {
                    if (_proxies[systemProxyId] != null) 
                    {
                        _proxies[systemProxyId].HttpProxy = proxyUrl;
                        _proxies[systemProxyId].HttpsProxy = proxyUrl;
                    }
                    else 
                    {
                        _proxies[systemProxyId] = systemProxy;
                    }
                }
                else
                {
                    _proxies[systemProxyId] = systemProxy;
                }
                
                _logger.LogInformation("Detected system proxy: {ProxyUrl}", proxyUrl);
                
                return systemProxy;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error detecting system proxy");
                return null;
            }
        }
        
        /// <summary>
        /// Loads proxy configurations
        /// </summary>
        public async Task<IEnumerable<ProxyConfig>> LoadProxiesAsync()
        {
            try
            {
                _logger.LogInformation("Loading proxy configurations");
                
                var loadTask = Task.Run(() =>
                {
                    string configPath = Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                        "VoiceDictation",
                        "proxy_config.json");
                    
                    if (!File.Exists(configPath))
                    {
                        return new List<ProxyConfig>();
                    }
                    
                    string json = File.ReadAllText(configPath);
                    
                    if (string.IsNullOrEmpty(json))
                    {
                        return new List<ProxyConfig>();
                    }
                    
                    var config = JsonConvert.DeserializeObject<ProxyConfigFile>(json);
                    
                    return config?.Proxies ?? new List<ProxyConfig>();
                });
                
                var timeoutTask = Task.Delay(3000);
                
                var completedTask = await Task.WhenAny(loadTask, timeoutTask);
                
                if (completedTask == timeoutTask)
                {
                    OnStatusChanged(ProxyStatus.Error, null, "Загрузка прокси-конфигураций заняла слишком много времени");
                    return new List<ProxyConfig>();
                }
                
                var proxies = await loadTask;
                
                _proxies.Clear();
                foreach (var proxy in proxies)
                {
                    if (!string.IsNullOrEmpty(proxy.Id))
                    {
                        _proxies[proxy.Id] = proxy;
                    }
                }
                
                OnStatusChanged(ProxyStatus.Inactive, null, $"Загружено прокси-конфигураций: {_proxies.Count}");
                
                return _proxies.Values;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading proxy configurations");
                OnStatusChanged(ProxyStatus.Error, null, $"Ошибка загрузки прокси-конфигураций: {ex.Message}");
                throw;
            }
        }
        
        public async Task<bool> SaveConfigAsync(string filePath)
        {
            try
            {
                var config = new ProxyConfigFile
                {
                    Proxies = _proxies.Values.ToList()
                };
                
                var json = JsonConvert.SerializeObject(config, Formatting.Indented);
                await File.WriteAllTextAsync(filePath, json);
                
                _configFilePath = filePath;
                
                _logger.LogInformation("Saved {ProxyCount} proxies to configuration file: {FilePath}", _proxies.Count, filePath);
                
                return true;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error saving proxy configuration to {FilePath}", filePath);
                return false;
            }
        }
        
        /// <summary>
        /// Gets proxy settings from Windows registry
        /// </summary>
        /// <returns>Proxy URL or null if not found</returns>
        private string? GetWindowsSystemProxy()
        {
            try
            {
                using var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(
                    @"Software\Microsoft\Windows\CurrentVersion\Internet Settings", false);
                
                if (key == null)
                {
                    return null;
                }
                
                var proxyEnable = key.GetValue("ProxyEnable");
                if (proxyEnable == null || (int)proxyEnable != 1)
                {
                    return null;
                }
                
                var proxyServer = key.GetValue("ProxyServer") as string;
                if (string.IsNullOrEmpty(proxyServer))
                {
                    return null;
                }
                
                if (proxyServer.Contains("="))
                {
                    foreach (var part in proxyServer.Split(';'))
                    {
                        if (part.StartsWith("http=", StringComparison.OrdinalIgnoreCase))
                        {
                            return part.Substring(5);
                        }
                    }
                    
                    var firstProxy = proxyServer.Split(';')[0];
                    if (firstProxy.Contains("="))
                    {
                        return firstProxy.Split('=')[1];
                    }
                    
                    return null;
                }
                
                return proxyServer;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error getting Windows system proxy settings");
                return null;
            }
        }
        
        /// <summary>
        /// Sets system-wide proxy settings on Windows
        /// </summary>
        /// <param name="proxy">Proxy configuration</param>
        private void SetSystemProxyWindows(ProxyConfig proxy)
        {
            try
            {
                using var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(
                    @"Software\Microsoft\Windows\CurrentVersion\Internet Settings", true);
                
                if (key == null)
                {
                    _logger.LogWarning("Could not open registry key for Internet Settings");
                    return;
                }
                
                key.SetValue("ProxyEnable", 1, Microsoft.Win32.RegistryValueKind.DWord);
                
                if (proxy.HttpProxy == proxy.HttpsProxy)
                {
                    key.SetValue("ProxyServer", proxy.HttpProxy, Microsoft.Win32.RegistryValueKind.String);
                }
                else
                {
                    var proxyString = $"http={proxy.HttpProxy};https={proxy.HttpsProxy}";
                    key.SetValue("ProxyServer", proxyString, Microsoft.Win32.RegistryValueKind.String);
                }
                
                NotifyProxyChange();
                
                _logger.LogInformation("Windows system proxy settings applied: {ProxyId} ({ProxyName})", 
                    proxy.Id, proxy.Name);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error setting Windows system proxy settings for {ProxyId} ({ProxyName})", 
                    proxy.Id, proxy.Name);
            }
        }
        
        /// <summary>
        /// Clears system-wide proxy settings on Windows
        /// </summary>
        private void ClearSystemProxyWindows()
        {
            try
            {
                using var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(
                    @"Software\Microsoft\Windows\CurrentVersion\Internet Settings", true);
                
                if (key == null)
                {
                    _logger.LogWarning("Could not open registry key for Internet Settings");
                    return;
                }
                
                key.SetValue("ProxyEnable", 0, Microsoft.Win32.RegistryValueKind.DWord);
                
                NotifyProxyChange();
                
                _logger.LogInformation("Windows system proxy settings cleared");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error clearing Windows system proxy settings");
            }
        }
        
        /// <summary>
        /// Notifies the system about proxy changes
        /// </summary>
        private void NotifyProxyChange()
        {
            try
            {
                const int INTERNET_OPTION_SETTINGS_CHANGED = 39;
                const int INTERNET_OPTION_REFRESH = 37;
                
                NativeMethods.InternetSetOption(IntPtr.Zero, INTERNET_OPTION_SETTINGS_CHANGED, IntPtr.Zero, 0);
                NativeMethods.InternetSetOption(IntPtr.Zero, INTERNET_OPTION_REFRESH, IntPtr.Zero, 0);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error notifying system about proxy changes");
            }
        }

        private static class NativeMethods
        {
            [System.Runtime.InteropServices.DllImport("wininet.dll", SetLastError = true)]
            public static extern bool InternetSetOption(IntPtr hInternet, int dwOption, IntPtr lpBuffer, int dwBufferLength);
        }
        
        private void OnStatusChanged(ProxyStatus status, ProxyConfig? proxy = null, string? message = null)
        {
            StatusChanged?.Invoke(this, new ProxyStatusChangedEventArgs(status, proxy, message));
        }

        private class ProxyConfigFile
        {
            public List<ProxyConfig> Proxies { get; set; } = new List<ProxyConfig>();
        }

        /// <summary>
        /// Loads proxy configuration from file
        /// </summary>
        /// <param name="filePath">Path to the configuration file</param>
        /// <returns>True if successful, false otherwise</returns>
        public async Task<bool> LoadConfigAsync(string filePath)
        {
            try
            {
                if (!File.Exists(filePath))
                {
                    _logger.LogWarning("Proxy configuration file not found: {FilePath}", filePath);
                    return false;
                }
                
                // Добавляем таймаут для защиты от зависаний
                var loadFileTask = Task.Run(async () => await File.ReadAllTextAsync(filePath));
                var timeoutTask = Task.Delay(5000); // 5 секунд таймаут
                
                var completedTask = await Task.WhenAny(loadFileTask, timeoutTask);
                
                if (completedTask == timeoutTask)
                {
                    _logger.LogWarning("Loading proxy configuration file timed out: {FilePath}", filePath);
                    OnStatusChanged(ProxyStatus.Error, null, "Загрузка конфигурации прокси превысила таймаут");
                    return false;
                }
                
                var json = await loadFileTask;
                
                var config = JsonConvert.DeserializeObject<ProxyConfigFile>(json);
                
                if (config == null || config.Proxies == null)
                {
                    _logger.LogWarning("Invalid proxy configuration file: {FilePath}", filePath);
                    return false;
                }
                
                _proxies.Clear();
                
                foreach (var proxy in config.Proxies)
                {
                    if (!string.IsNullOrEmpty(proxy.Id) && !string.IsNullOrEmpty(proxy.HttpProxy))
                    {
                        _proxies[proxy.Id] = proxy;
                    }
                }
                
                _configFilePath = filePath;
                
                _logger.LogInformation("Loaded {ProxyCount} proxies from configuration file", _proxies.Count);
                
                return true;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error loading proxy configuration from {FilePath}", filePath);
                return false;
            }
        }
    }
} 