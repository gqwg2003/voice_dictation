using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Interface for speech recognition services
    /// </summary>
    public interface ISpeechRecognizer : IDisposable
    {
        /// <summary>
        /// Event triggered when speech is recognized
        /// </summary>
        event EventHandler<SpeechRecognizedEventArgs> SpeechRecognized;
        
        /// <summary>
        /// Event triggered when speech recognition encounters an error
        /// </summary>
        event EventHandler<SpeechErrorEventArgs> SpeechError;
        
        /// <summary>
        /// Event triggered when the speech recognition status changes
        /// </summary>
        event EventHandler<SpeechStatusChangedEventArgs> StatusChanged;
        
        /// <summary>
        /// Gets a value indicating whether the recognizer is currently listening
        /// </summary>
        bool IsListening { get; }
        
        /// <summary>
        /// Gets or sets the language for speech recognition
        /// </summary>
        string Language { get; set; }
        
        /// <summary>
        /// Gets available languages for speech recognition
        /// </summary>
        /// <returns>List of available languages</returns>
        IEnumerable<LanguageInfo> GetAvailableLanguages();
        
        /// <summary>
        /// Starts continuous speech recognition
        /// </summary>
        /// <param name="cancellationToken">Optional cancellation token</param>
        /// <returns>A task representing the asynchronous operation</returns>
        Task StartContinuousRecognitionAsync(CancellationToken cancellationToken = default);
        
        /// <summary>
        /// Stops continuous speech recognition
        /// </summary>
        /// <returns>A task representing the asynchronous operation</returns>
        Task StopContinuousRecognitionAsync();
        
        /// <summary>
        /// Recognizes speech from a file
        /// </summary>
        /// <param name="filePath">Path to the audio file</param>
        /// <param name="cancellationToken">Optional cancellation token</param>
        /// <returns>Recognition result</returns>
        Task<SpeechRecognitionResult> RecognizeFromFileAsync(string filePath, CancellationToken cancellationToken = default);
        
        /// <summary>
        /// Recognizes speech from microphone (single utterance)
        /// </summary>
        /// <param name="maxDurationInSeconds">Maximum duration in seconds</param>
        /// <param name="cancellationToken">Optional cancellation token</param>
        /// <returns>Recognition result</returns>
        Task<SpeechRecognitionResult> RecognizeOnceAsync(int maxDurationInSeconds = 30, CancellationToken cancellationToken = default);
        
        /// <summary>
        /// Configures the recognizer with API key
        /// </summary>
        /// <param name="apiKey">API key for the speech service</param>
        void ConfigureWithApiKey(string apiKey);
        
        /// <summary>
        /// Configures the recognizer with proxy settings
        /// </summary>
        /// <param name="proxySettings">Proxy settings</param>
        void ConfigureProxy(ProxySettings proxySettings);
    }
} 