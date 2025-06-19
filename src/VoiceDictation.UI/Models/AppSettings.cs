using System;

namespace VoiceDictation.UI.Models
{
    /// <summary>
    /// Application settings wrapper
    /// </summary>
    public class AppSettings
    {
        private static AppSettings? _instance;
        
        public static AppSettings Instance => _instance ??= new AppSettings();
        
        public string PreferredRecognizer { get; set; } = "Python";
        
        /// <summary>
        /// Initializes a new instance of the <see cref="AppSettings"/> class
        /// </summary>
        private AppSettings()
        {
            try
            {
                PreferredRecognizer = Properties.Settings.Default.PreferredRecognizer;
            }
            catch (Exception)
            {
                PreferredRecognizer = "Python";
            }
        }
        
        /// <summary>
        /// Saves settings
        /// </summary>
        public void Save()
        {
            try
            {
                Properties.Settings.Default.PreferredRecognizer = PreferredRecognizer;
                Properties.Settings.Default.Save();
            }
            catch (Exception)
            {
                // Ignore errors
            }
        }
    }
} 