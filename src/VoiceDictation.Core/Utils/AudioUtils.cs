using System;
using System.IO;
using System.Threading.Tasks;
using NAudio.Wave;
using NAudio.Wave.SampleProviders;

namespace VoiceDictation.Core.Utils
{
    /// <summary>
    /// Utility class for audio processing operations
    /// </summary>
    public static class AudioUtils
    {

        public static bool NormalizeAudio(string sourceFile, string targetFile, float targetPeakAmplitude = 0.95f)
        {
            try
            {
                if (!File.Exists(sourceFile))
                    return false;

                float peakAmplitude = 0;
                using (var reader = new AudioFileReader(sourceFile))
                {
                    var buffer = new float[reader.WaveFormat.SampleRate * reader.WaveFormat.Channels];
                    int read;
                    while ((read = reader.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        for (int i = 0; i < read; i++)
                        {
                            var abs = Math.Abs(buffer[i]);
                            if (abs > peakAmplitude)
                                peakAmplitude = abs;
                        }
                    }
                }

                if (Math.Abs(peakAmplitude - targetPeakAmplitude) < 0.05)
                {
                    if (sourceFile != targetFile)
                        File.Copy(sourceFile, targetFile, true);
                    return true;
                }

                float gainFactor = targetPeakAmplitude / peakAmplitude;
                
                using (var reader = new AudioFileReader(sourceFile))
                {
                    var volumeSampleProvider = new VolumeSampleProvider(reader);
                    volumeSampleProvider.Volume = gainFactor;
                    
                    WaveFileWriter.CreateWaveFile16(targetFile, volumeSampleProvider);
                }

                return File.Exists(targetFile);
            }
            catch (Exception)
            {
                return false;
            }
        }

        public static bool TrimSilence(string sourceFile, string targetFile, float silenceThreshold = 0.02f)
        {
            try
            {
                if (!File.Exists(sourceFile))
                    return false;

                using (var reader = new AudioFileReader(sourceFile))
                {
                    int startPos = FindNonSilenceStart(reader, silenceThreshold);
                    int endPos = FindNonSilenceEnd(reader, silenceThreshold);
                    
                    if (startPos >= endPos)
                    {
                        return false;
                    }
                    
                    reader.Position = 0;
                    
                    using (var writer = new WaveFileWriter(targetFile, reader.WaveFormat))
                    {
                        int bytesToSkip = startPos * reader.WaveFormat.BlockAlign;
                        reader.Position = bytesToSkip;
                        
                        int samplesToRead = endPos - startPos;
                        int bytesToRead = samplesToRead * reader.WaveFormat.BlockAlign;
                        
                        byte[] buffer = new byte[bytesToRead];
                        int bytesRead = reader.Read(buffer, 0, buffer.Length);
                        
                        writer.Write(buffer, 0, bytesRead);
                    }
                }

                return File.Exists(targetFile);
            }
            catch (Exception)
            {
                return false;
            }
        }

        private static int FindNonSilenceStart(AudioFileReader reader, float threshold)
        {
            reader.Position = 0;
            var buffer = new float[1024];
            int read;
            int position = 0;
            
            while ((read = reader.Read(buffer, 0, buffer.Length)) > 0)
            {
                for (int i = 0; i < read; i++)
                {
                    if (Math.Abs(buffer[i]) > threshold)
                    {
                        return position + i;
                    }
                }
                
                position += read;
            }
            
            return 0; // No non-silent audio found
        }

        private static int FindNonSilenceEnd(AudioFileReader reader, float threshold)
        {
            reader.Position = 0;
            var buffer = new float[1024];
            int read;
            int position = 0;
            int lastNonSilencePos = 0;
            
            while ((read = reader.Read(buffer, 0, buffer.Length)) > 0)
            {
                for (int i = 0; i < read; i++)
                {
                    if (Math.Abs(buffer[i]) > threshold)
                    {
                        lastNonSilencePos = position + i;
                    }
                }
                
                position += read;
            }
            
            return lastNonSilencePos > 0 ? lastNonSilencePos + 1 : position; // Add 1 to include the last non-silent sample
        }

        public static bool ConvertAudioFormat(string sourceFile, string targetFile)
        {
            try
            {
                string targetExtension = Path.GetExtension(targetFile).ToLowerInvariant();
                
                if (targetExtension == ".wav")
                {
                    return FileUtils.ConvertToWav(sourceFile, targetFile);
                }
                else
                {
                    string tempWavFile = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName() + ".wav");
                    
                    try
                    {
                        if (!FileUtils.ConvertToWav(sourceFile, tempWavFile))
                            return false;
                        
                        File.Copy(tempWavFile, targetFile);
                        return true;
                    }
                    finally
                    {
                        FileUtils.SafeDeleteFile(tempWavFile);
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }
        }
    }
} 