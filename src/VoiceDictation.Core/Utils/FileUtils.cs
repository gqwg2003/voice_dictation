using System;
using System.IO;
using System.Threading.Tasks;
using System.Collections.Generic;
using NAudio.Wave;
using System.Linq;

namespace VoiceDictation.Core.Utils
{
    /// <summary>
    /// Utility class for file operations
    /// </summary>
    public static class FileUtils
    {
        public static bool IsValidAudioFile(string filePath)
        {
            if (!File.Exists(filePath))
                return false;

            string extension = Path.GetExtension(filePath).ToLowerInvariant();
            return new[] { ".wav", ".mp3", ".ogg", ".flac", ".m4a", ".wma" }.Contains(extension);
        }

        public static bool ConvertToWav(string sourceFile, string targetWavFile)
        {
            try
            {
                if (string.IsNullOrEmpty(sourceFile) || !File.Exists(sourceFile))
                    return false;

                if (Path.GetExtension(sourceFile).ToLowerInvariant() == ".wav")
                {
                    File.Copy(sourceFile, targetWavFile, true);
                    return true;
                }

                using (var reader = new MediaFoundationReader(sourceFile))
                {
                    WaveFileWriter.CreateWaveFile(targetWavFile, reader);
                }

                return File.Exists(targetWavFile);
            }
            catch (Exception)
            {
                return false;
            }
        }

        public static async Task<double> GetAudioDurationAsync(string filePath)
        {
            return await Task.Run(() =>
            {
                try
                {
                    if (!File.Exists(filePath))
                        return 0;

                    using (var reader = new MediaFoundationReader(filePath))
                    {
                        return reader.TotalTime.TotalSeconds;
                    }
                }
                catch (Exception)
                {
                    return 0;
                }
            });
        }

        public static string CreateTempWavFile(string sourceFile)
        {
            try
            {
                if (!IsValidAudioFile(sourceFile))
                    return null;

                string tempFile = Path.Combine(
                    Path.GetTempPath(),
                    $"{Path.GetFileNameWithoutExtension(sourceFile)}_{Guid.NewGuid():N}.wav");

                if (ConvertToWav(sourceFile, tempFile))
                    return tempFile;

                return null;
            }
            catch (Exception)
            {
                return null;
            }
        }

        public static bool SafeDeleteFile(string filePath)
        {
            try
            {
                if (File.Exists(filePath))
                    File.Delete(filePath);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }
        
        public static List<string> GetRecentFiles(string directory, string searchPattern = "*.*", int maxFiles = 5)
        {
            try
            {
                if (!Directory.Exists(directory))
                    return new List<string>();

                return Directory.GetFiles(directory, searchPattern)
                    .OrderByDescending(f => new FileInfo(f).LastWriteTime)
                    .Take(maxFiles)
                    .ToList();
            }
            catch (Exception)
            {
                return new List<string>();
            }
        }
    }
} 