using System;
using System.Collections.Generic;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using VoiceDictation.Core.Utils;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Parses and processes speech recognition results
    /// </summary>
    public class RecognitionResultParser
    {
        private readonly ILogger _logger;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="RecognitionResultParser"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        public RecognitionResultParser(ILogger logger)
        {
            _logger = logger;
        }
        
        /// <summary>
        /// Parses a JSON result string into a SpeechRecognitionResult
        /// </summary>
        /// <param name="jsonResult">JSON result string</param>
        /// <returns>A SpeechRecognitionResult object</returns>
        public SpeechRecognitionResult ParseResult(string jsonResult)
        {
            try
            {
                var parsedResult = JsonConvert.DeserializeObject<Dictionary<string, object>>(jsonResult);
                
                if (parsedResult != null)
                {
                    if (parsedResult.ContainsKey("error"))
                    {
                        string errorMessage = parsedResult["error"].ToString() ?? "Unknown error";
                        string language = parsedResult.ContainsKey("language") ? parsedResult["language"].ToString() ?? "" : "";
                        
                        return new SpeechRecognitionResult(errorMessage, language);
                    }
                    else if (parsedResult.ContainsKey("text"))
                    {
                        string text = parsedResult["text"].ToString() ?? "";
                        float confidence = parsedResult.ContainsKey("confidence") ? 
                            Convert.ToSingle(parsedResult["confidence"]) : 0.0f;
                        string language = parsedResult.ContainsKey("language") ? 
                            parsedResult["language"].ToString() ?? "" : "";
                        
                        text = TextProcessingUtils.ProcessMixedLanguageText(text, language);
                        text = TextProcessingUtils.FormatRecognizedText(text);
                        
                        return new SpeechRecognitionResult(text, confidence, language);
                    }
                }
                
                return new SpeechRecognitionResult("Failed to parse recognition result");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error parsing recognition result");
                return new SpeechRecognitionResult($"Error parsing recognition result: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Creates a simulated recognition result for testing purposes
        /// </summary>
        /// <param name="functionName">Name of the function being simulated</param>
        /// <param name="language">Language code</param>
        /// <returns>A JSON string representing a simulated result</returns>
        public string CreateSimulatedResult(string functionName, string language)
        {
            string recognizedText;
            if (language.StartsWith("ru", StringComparison.OrdinalIgnoreCase))
            {
                recognizedText = "Это тестовый текст для распознавания речи.";
            }
            else if (language.StartsWith("fr", StringComparison.OrdinalIgnoreCase))
            {
                recognizedText = "C'est un texte de test pour la reconnaissance vocale.";
            }
            else if (language.StartsWith("de", StringComparison.OrdinalIgnoreCase))
            {
                recognizedText = "Dies ist ein Testtext für die Spracherkennung.";
            }
            else if (language.StartsWith("es", StringComparison.OrdinalIgnoreCase))
            {
                recognizedText = "Este es un texto de prueba para el reconocimiento de voz.";
            }
            else
            {
                recognizedText = "This is a test text for speech recognition.";
            }
            
            if (functionName == "recognize_from_microphone" || functionName == "recognize_from_file")
            {
                var simulatedResult = new
                {
                    text = recognizedText,
                    confidence = 0.95f,
                    language = language
                };
                
                return JsonConvert.SerializeObject(simulatedResult);
            }
            else if (functionName == "set_language")
            {
                return JsonConvert.SerializeObject(new { success = true });
            }
            else if (functionName == "update_config")
            {
                return JsonConvert.SerializeObject(new { success = true });
            }
            
            return "{}";
        }
    }
} 