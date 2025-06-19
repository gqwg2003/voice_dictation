using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace VoiceDictation.Network.Proxy
{
    /// <summary>
    /// Interface for managing proxy connections
    /// </summary>
    public interface IProxyManager
    {
        /// <summary>
        /// Event triggered when proxy status changes
        /// </summary>
        event EventHandler<ProxyStatusChangedEventArgs>? StatusChanged;
        
        /// <summary>
        /// Gets a value indicating whether a proxy is currently active
        /// </summary>
        bool IsProxyActive { get; }
        
        /// <summary>
        /// Gets the currently active proxy configuration
        /// </summary>
        ProxyConfig? CurrentProxy { get; }
        
        /// <summary>
        /// Gets all available proxy configurations
        /// </summary>
        /// <returns>List of proxy configurations</returns>
        Task<IEnumerable<ProxyConfig>> GetProxyListAsync();
        
        /// <summary>
        /// Adds a new proxy configuration
        /// </summary>
        /// <param name="config">Proxy configuration to add</param>
        /// <returns>True if added successfully</returns>
        Task<bool> AddProxyAsync(ProxyConfig config);
        
        /// <summary>
        /// Removes a proxy configuration
        /// </summary>
        /// <param name="proxyId">ID of the proxy to remove</param>
        /// <returns>True if removed successfully</returns>
        Task<bool> RemoveProxyAsync(string proxyId);
        
        /// <summary>
        /// Selects and activates a proxy configuration
        /// </summary>
        /// <param name="proxyId">ID of the proxy to select</param>
        /// <returns>True if activated successfully</returns>
        Task<bool> SelectProxyAsync(string proxyId);
        
        /// <summary>
        /// Deactivates the current proxy
        /// </summary>
        /// <returns>True if deactivated successfully</returns>
        Task<bool> DeactivateProxyAsync();
        
        /// <summary>
        /// Tests a proxy connection
        /// </summary>
        /// <param name="proxyId">ID of the proxy to test (uses current if null)</param>
        /// <returns>The test result</returns>
        Task<ProxyTestResult> TestProxyAsync(string? proxyId = null);
        
        /// <summary>
        /// Attempts to detect system proxy settings
        /// </summary>
        /// <returns>Detected proxy configuration or null if not found</returns>
        Task<ProxyConfig?> DetectSystemProxyAsync();
        
        /// <summary>
        /// Loads proxy configurations from a file
        /// </summary>
        /// <param name="filePath">Path to the configuration file</param>
        /// <returns>True if loaded successfully</returns>
        Task<bool> LoadConfigAsync(string filePath);
        
        /// <summary>
        /// Saves proxy configurations to a file
        /// </summary>
        /// <param name="filePath">Path to save the configuration file</param>
        /// <returns>True if saved successfully</returns>
        Task<bool> SaveConfigAsync(string filePath);
    }
    
    /// <summary>
    /// Proxy configuration
    /// </summary>
    public class ProxyConfig
    {
        /// <summary>
        /// Gets or sets the unique identifier for the proxy
        /// </summary>
        public string Id { get; set; } = Guid.NewGuid().ToString();
        
        /// <summary>
        /// Gets or sets the display name for the proxy
        /// </summary>
        public string Name { get; set; } = string.Empty;
        
        /// <summary>
        /// Gets or sets the HTTP proxy URL
        /// </summary>
        public string HttpProxy { get; set; } = string.Empty;
        
        /// <summary>
        /// Gets or sets the HTTPS proxy URL
        /// </summary>
        public string HttpsProxy { get; set; } = string.Empty;
        
        /// <summary>
        /// Gets or sets the proxy username
        /// </summary>
        public string? Username { get; set; }
        
        /// <summary>
        /// Gets or sets the proxy password
        /// </summary>
        public string? Password { get; set; }
        
        /// <summary>
        /// Gets or sets a value indicating whether this is a system proxy
        /// </summary>
        public bool IsSystemProxy { get; set; }
        
        /// <summary>
        /// Gets or sets additional proxy settings
        /// </summary>
        public Dictionary<string, string>? AdditionalSettings { get; set; }
    }
    
    /// <summary>
    /// Result of a proxy test
    /// </summary>
    public class ProxyTestResult
    {
        /// <summary>
        /// Gets a value indicating whether the test was successful
        /// </summary>
        public bool IsSuccessful { get; }
        
        /// <summary>
        /// Gets the response time in milliseconds
        /// </summary>
        public long ResponseTimeMs { get; }
        
        /// <summary>
        /// Gets the error message if the test was not successful
        /// </summary>
        public string? ErrorMessage { get; }
        
        /// <summary>
        /// Gets the proxy configuration that was tested
        /// </summary>
        public ProxyConfig ProxyConfig { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="ProxyTestResult"/> class for successful test
        /// </summary>
        /// <param name="proxyConfig">The proxy configuration</param>
        /// <param name="responseTimeMs">The response time in milliseconds</param>
        public ProxyTestResult(ProxyConfig proxyConfig, long responseTimeMs)
        {
            ProxyConfig = proxyConfig;
            ResponseTimeMs = responseTimeMs;
            IsSuccessful = true;
            ErrorMessage = null;
        }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="ProxyTestResult"/> class for failed test
        /// </summary>
        /// <param name="proxyConfig">The proxy configuration</param>
        /// <param name="errorMessage">The error message</param>
        public ProxyTestResult(ProxyConfig proxyConfig, string errorMessage)
        {
            ProxyConfig = proxyConfig;
            ResponseTimeMs = 0;
            IsSuccessful = false;
            ErrorMessage = errorMessage;
        }
    }
    
    /// <summary>
    /// Event arguments for proxy status changes
    /// </summary>
    public class ProxyStatusChangedEventArgs : EventArgs
    {
        /// <summary>
        /// Gets the proxy status
        /// </summary>
        public ProxyStatus Status { get; }
        
        /// <summary>
        /// Gets the proxy configuration that triggered the status change
        /// </summary>
        public ProxyConfig? ProxyConfig { get; }
        
        /// <summary>
        /// Gets the status message
        /// </summary>
        public string? Message { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="ProxyStatusChangedEventArgs"/> class
        /// </summary>
        /// <param name="status">The proxy status</param>
        /// <param name="proxyConfig">The proxy configuration</param>
        /// <param name="message">The status message</param>
        public ProxyStatusChangedEventArgs(ProxyStatus status, ProxyConfig? proxyConfig = null, string? message = null)
        {
            Status = status;
            ProxyConfig = proxyConfig;
            Message = message;
        }
    }
    
    /// <summary>
    /// Status of proxy connection
    /// </summary>
    public enum ProxyStatus
    {
        /// <summary>
        /// No proxy is active
        /// </summary>
        Inactive,
        
        /// <summary>
        /// Proxy is activating
        /// </summary>
        Activating,
        
        /// <summary>
        /// Proxy is active and connected
        /// </summary>
        Active,
        
        /// <summary>
        /// Proxy is active but has issues
        /// </summary>
        Unstable,
        
        /// <summary>
        /// Proxy is deactivating
        /// </summary>
        Deactivating,
        
        /// <summary>
        /// Proxy error
        /// </summary>
        Error
    }
} 