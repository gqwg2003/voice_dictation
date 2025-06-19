using System;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Information about a supported language
    /// </summary>
    public class LanguageInfo
    {
        /// <summary>
        /// Gets the language code (e.g., "en-US", "ru-RU")
        /// </summary>
        public string Code { get; }
        
        /// <summary>
        /// Gets the display name of the language
        /// </summary>
        public string DisplayName { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="LanguageInfo"/> class
        /// </summary>
        /// <param name="code">Language code</param>
        /// <param name="displayName">Display name</param>
        public LanguageInfo(string code, string displayName)
        {
            Code = code;
            DisplayName = displayName;
        }
    }
    
    /// <summary>
    /// Proxy settings for speech recognition
    /// </summary>
    public class ProxySettings
    {
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
    }
    
    /// <summary>
    /// Status of speech recognition
    /// </summary>
    public enum SpeechRecognitionStatus
    {
        /// <summary>
        /// Speech recognizer is idle
        /// </summary>
        Idle,
        
        /// <summary>
        /// Speech recognizer is initializing
        /// </summary>
        Initializing,
        
        /// <summary>
        /// Speech recognizer is listening
        /// </summary>
        Listening,
        
        /// <summary>
        /// Speech recognizer is processing
        /// </summary>
        Processing,
        
        /// <summary>
        /// Speech recognizer has detected speech
        /// </summary>
        SpeechDetected,
        
        /// <summary>
        /// Speech recognizer has recognized speech
        /// </summary>
        Recognized,
        
        /// <summary>
        /// Speech recognizer has encountered an error
        /// </summary>
        Error
    }
    
    /// <summary>
    /// Event arguments for speech recognition results
    /// </summary>
    public class SpeechRecognizedEventArgs : EventArgs
    {
        /// <summary>
        /// Gets the speech recognition result
        /// </summary>
        public SpeechRecognitionResult Result { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SpeechRecognizedEventArgs"/> class
        /// </summary>
        /// <param name="result">The speech recognition result</param>
        public SpeechRecognizedEventArgs(SpeechRecognitionResult result)
        {
            Result = result;
        }
    }
    
    /// <summary>
    /// Event arguments for speech recognition errors
    /// </summary>
    public class SpeechErrorEventArgs : EventArgs
    {
        /// <summary>
        /// Gets the error message
        /// </summary>
        public string ErrorMessage { get; }
        
        /// <summary>
        /// Gets the error code
        /// </summary>
        public string ErrorCode { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SpeechErrorEventArgs"/> class
        /// </summary>
        /// <param name="errorMessage">The error message</param>
        /// <param name="errorCode">The error code</param>
        public SpeechErrorEventArgs(string errorMessage, string errorCode = "")
        {
            ErrorMessage = errorMessage;
            ErrorCode = errorCode;
        }
    }
    
    /// <summary>
    /// Event arguments for speech status changes
    /// </summary>
    public class SpeechStatusChangedEventArgs : EventArgs
    {
        /// <summary>
        /// Gets the status
        /// </summary>
        public SpeechRecognitionStatus Status { get; }
        
        /// <summary>
        /// Gets additional status information
        /// </summary>
        public string StatusDetails { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SpeechStatusChangedEventArgs"/> class
        /// </summary>
        /// <param name="status">The status</param>
        /// <param name="statusDetails">Additional status details</param>
        public SpeechStatusChangedEventArgs(SpeechRecognitionStatus status, string statusDetails = "")
        {
            Status = status;
            StatusDetails = statusDetails;
        }
    }
    
    /// <summary>
    /// Result of speech recognition
    /// </summary>
    public class SpeechRecognitionResult
    {
        /// <summary>
        /// Gets the recognized text
        /// </summary>
        public string Text { get; }
        
        /// <summary>
        /// Gets the confidence level (0.0 - 1.0)
        /// </summary>
        public float Confidence { get; }
        
        /// <summary>
        /// Gets the language code
        /// </summary>
        public string Language { get; }
        
        /// <summary>
        /// Gets a value indicating whether recognition was successful
        /// </summary>
        public bool IsSuccessful { get; }
        
        /// <summary>
        /// Gets the error message (if any)
        /// </summary>
        public string? ErrorMessage { get; }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SpeechRecognitionResult"/> class
        /// </summary>
        /// <param name="text">Recognized text</param>
        /// <param name="confidence">Confidence level</param>
        /// <param name="language">Language code</param>
        public SpeechRecognitionResult(string text, float confidence, string language)
        {
            Text = text;
            Confidence = confidence;
            Language = language;
            IsSuccessful = true;
            ErrorMessage = null;
        }
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SpeechRecognitionResult"/> class with an error
        /// </summary>
        /// <param name="errorMessage">Error message</param>
        /// <param name="language">Language code</param>
        public SpeechRecognitionResult(string errorMessage, string language = "")
        {
            Text = string.Empty;
            Confidence = 0.0f;
            Language = language;
            IsSuccessful = false;
            ErrorMessage = errorMessage;
        }
    }
} 