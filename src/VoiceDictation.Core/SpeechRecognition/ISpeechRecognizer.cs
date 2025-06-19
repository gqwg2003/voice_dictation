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
        event EventHandler<SpeechRecognizedEventArgs> SpeechRecognized;
        event EventHandler<SpeechErrorEventArgs> SpeechError;
        event EventHandler<SpeechStatusChangedEventArgs> StatusChanged;
        
        bool IsListening { get; }
        string Language { get; set; }
        
        IEnumerable<LanguageInfo> GetAvailableLanguages();
        
        Task StartContinuousRecognitionAsync(CancellationToken cancellationToken = default);
        Task StopContinuousRecognitionAsync();
        
        Task<SpeechRecognitionResult> RecognizeFromFileAsync(string filePath, CancellationToken cancellationToken = default);
        Task<SpeechRecognitionResult> RecognizeOnceAsync(int maxDurationInSeconds = 30, CancellationToken cancellationToken = default);
        
        void ConfigureWithApiKey(string apiKey);
        void ConfigureProxy(ProxySettings proxySettings);
    }
} 