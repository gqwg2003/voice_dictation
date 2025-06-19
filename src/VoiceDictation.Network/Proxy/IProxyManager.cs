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
        event EventHandler<ProxyStatusChangedEventArgs>? StatusChanged;
        bool IsProxyActive { get; }
        ProxyConfig? CurrentProxy { get; }
        
        Task<IEnumerable<ProxyConfig>> GetProxyListAsync();
        Task<bool> AddProxyAsync(ProxyConfig config);
        Task<bool> RemoveProxyAsync(string proxyId);
        
        Task<bool> SelectProxyAsync(string proxyId);
        Task<bool> DeactivateProxyAsync();
        Task<ProxyTestResult> TestProxyAsync(string? proxyId = null);
        
        Task<ProxyConfig?> DetectSystemProxyAsync();
        Task<bool> LoadConfigAsync(string filePath);
        Task<bool> SaveConfigAsync(string filePath);
    }
    
    /// <summary>
    /// Proxy configuration
    /// </summary>
    public class ProxyConfig
    {
        public string Id { get; set; } = Guid.NewGuid().ToString();
        public string Name { get; set; } = string.Empty;
        public string HttpProxy { get; set; } = string.Empty;
        public string HttpsProxy { get; set; } = string.Empty;
        public string? Username { get; set; }
        public string? Password { get; set; }
        public bool IsSystemProxy { get; set; }
        public Dictionary<string, string>? AdditionalSettings { get; set; }
    }
    
    /// <summary>
    /// Result of a proxy test
    /// </summary>
    public class ProxyTestResult
    {
        public bool IsSuccessful { get; }
        public long ResponseTimeMs { get; }
        public string? ErrorMessage { get; }
        public ProxyConfig ProxyConfig { get; }
        
        public ProxyTestResult(ProxyConfig proxyConfig, long responseTimeMs)
        {
            ProxyConfig = proxyConfig;
            ResponseTimeMs = responseTimeMs;
            IsSuccessful = true;
            ErrorMessage = null;
        }
        
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
        public ProxyStatus Status { get; }
        public ProxyConfig? ProxyConfig { get; }
        public string? Message { get; }
        
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
        Inactive,
        Activating,
        Active,
        Unstable,
        Deactivating,
        
        Error
    }
} 