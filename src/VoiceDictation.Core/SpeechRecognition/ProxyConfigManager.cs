using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.Extensions.Logging;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Manages proxy configuration for speech recognition
    /// </summary>
    public class ProxyConfigManager
    {
        private readonly ILogger _logger;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="ProxyConfigManager"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        public ProxyConfigManager(ILogger logger)
        {
            _logger = logger;
        }
        
        /// <summary>
        /// Creates a dictionary of proxy settings for Python
        /// </summary>
        /// <param name="proxySettings">Proxy settings</param>
        /// <returns>Dictionary of proxy settings</returns>
        public Dictionary<string, string> CreateProxyDictionary(ProxySettings proxySettings)
        {
            var proxyDict = new Dictionary<string, string>();
            
            if (!string.IsNullOrEmpty(proxySettings.HttpProxy))
            {
                proxyDict["http"] = proxySettings.HttpProxy;
            }
            
            if (!string.IsNullOrEmpty(proxySettings.HttpsProxy))
            {
                proxyDict["https"] = proxySettings.HttpsProxy;
            }
            
            if (!string.IsNullOrEmpty(proxySettings.Username) && !string.IsNullOrEmpty(proxySettings.Password))
            {
                foreach (var key in proxyDict.Keys.ToList())
                {
                    var url = proxyDict[key];
                    if (!string.IsNullOrEmpty(url))
                    {
                        if (url.Contains("//") && !url.Contains("@"))
                        {
                            var parts = url.Split(new[] { "//" }, 2, StringSplitOptions.None);
                            proxyDict[key] = $"{parts[0]}//{proxySettings.Username}:{proxySettings.Password}@{parts[1]}";
                        }
                    }
                }
            }
            
            return proxyDict;
        }
        
        /// <summary>
        /// Creates a configuration dictionary for Python recognizer
        /// </summary>
        /// <param name="apiKey">API key</param>
        /// <param name="proxySettings">Proxy settings</param>
        /// <returns>Dictionary of configuration settings</returns>
        public Dictionary<string, object> CreateConfigDictionary(string apiKey, ProxySettings proxySettings)
        {
            var configParams = new Dictionary<string, object>();
            
            if (!string.IsNullOrEmpty(apiKey))
            {
                configParams["apiKey"] = apiKey;
            }
            
            if (proxySettings != null)
            {
                var proxyDict = CreateProxyDictionary(proxySettings);
                if (proxyDict.Count > 0)
                {
                    configParams["proxy"] = proxyDict;
                }
            }
            
            return configParams;
        }
    }
} 