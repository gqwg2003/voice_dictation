using System;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Information about a supported language
    /// </summary>
    public class LanguageInfo
    {
        public string Code { get; }
        public string DisplayName { get; }
        
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
        public string HttpProxy { get; set; } = string.Empty;
        public string HttpsProxy { get; set; } = string.Empty;
        public string? Username { get; set; }
        public string? Password { get; set; }
    }
    
    /// <summary>
    /// Status of speech recognition
    /// </summary>
    public enum SpeechRecognitionStatus
    {
        Idle,
        Initializing,
        Listening,
        Processing,
        SpeechDetected,
        Recognized,
        Error
    }
    
    /// <summary>
    /// Event arguments for speech recognition results
    /// </summary>
    public class SpeechRecognizedEventArgs : EventArgs
    {
        public SpeechRecognitionResult Result { get; }
        
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
        public string ErrorMessage { get; }
        public string ErrorCode { get; }
        
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
        public SpeechRecognitionStatus Status { get; }
        public string StatusDetails { get; }
        
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
        public string Text { get; }
        public float Confidence { get; }
        public string Language { get; }
        public bool IsSuccessful { get; }
        public string? ErrorMessage { get; }
        
        public SpeechRecognitionResult(string text, float confidence, string language)
        {
            Text = text;
            Confidence = confidence;
            Language = language;
            IsSuccessful = true;
            ErrorMessage = null;
        }
        
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