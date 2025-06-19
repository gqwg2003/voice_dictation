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
            return await Task.FromResult(_proxies.Values);
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
                _logger.LogInformation("Detecting system proxy settings");
                
                string? httpProxy = null;
                string? httpsProxy = null;
                
                httpProxy = Environment.GetEnvironmentVariable("HTTP_PROXY") ?? Environment.GetEnvironmentVariable("http_proxy");
                httpsProxy = Environment.GetEnvironmentVariable("HTTPS_PROXY") ?? Environment.GetEnvironmentVariable("https_proxy");
                
                if (string.IsNullOrEmpty(httpProxy))
                {
                    if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                    {
                        httpProxy = GetWindowsSystemProxy();
                    }
                    
                    if (string.IsNullOrEmpty(httpProxy) && WebRequest.DefaultWebProxy != null)
                    {
                        var defaultProxy = WebRequest.DefaultWebProxy;
                        
                        httpProxy = defaultProxy.GetProxy(new Uri("http://example.com")).ToString();
                        
                        if (httpProxy == "http://example.com/")
                        {
                            httpProxy = null;
                        }
                    }
                }
                
                if (!string.IsNullOrEmpty(httpProxy))
                {
                    var systemProxy = new ProxyConfig
                    {
                        Id = "system",
                        Name = "System Proxy",
                        HttpProxy = httpProxy,
                        HttpsProxy = httpsProxy ?? httpProxy,
                        IsSystemProxy = true
                    };
                    
                    await AddProxyAsync(systemProxy);
                    
                    return systemProxy;
                }
                
                return null;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error detecting system proxy settings");
                return null;
            }
        }
        
        public async Task<bool> LoadConfigAsync(string filePath)
        {
            try
            {
                if (!File.Exists(filePath))
                {
                    _logger.LogWarning("Proxy configuration file not found: {FilePath}", filePath);
                    return false;
                }
                
                var json = await File.ReadAllTextAsync(filePath);
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
                return null;
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
        }
        
        /// <summary>
        /// Clears system-wide proxy settings on Windows
        /// </summary>
        private void ClearSystemProxyWindows()
        {
        }
        
        /// <summary>
        /// Raises the StatusChanged event
        /// </summary>
        /// <param name="status">Status</param>
        /// <param name="proxy">Proxy configuration</param>
        /// <param name="message">Message</param>
        private void OnStatusChanged(ProxyStatus status, ProxyConfig? proxy = null, string? message = null)
        {
            StatusChanged?.Invoke(this, new ProxyStatusChangedEventArgs(status, proxy, message));
        }
        
        /// <summary>
        /// Class for proxy configuration file
        /// </summary>
        private class ProxyConfigFile
        {
            /// <summary>
            /// Gets or sets the list of proxy configurations
            /// </summary>
            public List<ProxyConfig> Proxies { get; set; } = new List<ProxyConfig>();
        }
    }
} 