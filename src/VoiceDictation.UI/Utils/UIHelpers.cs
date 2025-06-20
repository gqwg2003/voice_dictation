using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using Microsoft.Extensions.Logging;
using System.Threading.Tasks;

namespace VoiceDictation.UI.Utils
{
    /// <summary>
    /// Helper methods for UI operations
    /// </summary>
    public static class UIHelpers
    {
        public static void RunOnUIThread(Action action)
        {
            if (Application.Current.Dispatcher.CheckAccess())
            {
                action();
            }
            else
            {
                Application.Current.Dispatcher.Invoke(action);
            }
        }

        public static DispatcherOperation RunOnUIThreadAsync(Action action)
        {
            return Application.Current.Dispatcher.InvokeAsync(action);
        }

        public static void SetWaitCursor()
        {
            RunOnUIThread(() => Mouse.OverrideCursor = Cursors.Wait);
        }

        public static void ResetCursor()
        {
            RunOnUIThread(() => Mouse.OverrideCursor = null);
        }

        public static void ExecuteWithWaitCursor(Action action)
        {
            try
            {
                SetWaitCursor();
                action();
            }
            finally
            {
                ResetCursor();
            }
        }

        public static void FocusTextBox(TextBox textBox)
        {
            if (textBox == null)
                return;

            textBox.Focus();
            textBox.CaretIndex = textBox.Text?.Length ?? 0;
        }

        public static Window? GetActiveWindow()
        {
            if (Application.Current == null)
                return null;

            foreach (Window window in Application.Current.Windows)
            {
                if (window.IsActive)
                    return window;
            }

            return Application.Current.MainWindow;
        }

        // Методы из Core.Utils.UIHelpers
        public static void ShowErrorMessage(ILogger logger, string logMessage, Exception ex, string? userMessage = null)
        {
            logger.LogError(ex, logMessage);
            
            string message = userMessage ?? $"Ошибка: {ex.Message}";
            MessageBox.Show(message, "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        
        public static void ShowInfoMessage(string message, string title = "Информация")
        {
            MessageBox.Show(message, title, MessageBoxButton.OK, MessageBoxImage.Information);
        }
        
        public static void ShowWarningMessage(string message, string title = "Предупреждение")
        {
            MessageBox.Show(message, title, MessageBoxButton.OK, MessageBoxImage.Warning);
        }
        
        public static void SafeExecute(Action action, ILogger logger, string errorLogMessage, 
            Action<string>? statusSetter = null, Action<Exception>? exceptionHandler = null)
        {
            try
            {
                action();
            }
            catch (Exception ex)
            {
                logger.LogError(ex, errorLogMessage);
                
                statusSetter?.Invoke($"Ошибка: {ex.Message}");
                exceptionHandler?.Invoke(ex);
            }
        }
        
        public static async Task SafeExecuteAsync(Func<Task> action, ILogger logger, string errorLogMessage, 
            Action<string>? statusSetter = null, Action<Exception>? exceptionHandler = null)
        {
            try
            {
                await action();
            }
            catch (Exception ex)
            {
                logger.LogError(ex, errorLogMessage);
                
                statusSetter?.Invoke($"Ошибка: {ex.Message}");
                exceptionHandler?.Invoke(ex);
            }
        }
        
        public static string FormatFilePath(string path, int maxLength = 40)
        {
            if (string.IsNullOrEmpty(path))
                return string.Empty;
                
            if (path.Length <= maxLength)
                return path;
                
            string fileName = System.IO.Path.GetFileName(path);
            string directory = System.IO.Path.GetDirectoryName(path) ?? string.Empty;
            
            if (fileName.Length >= maxLength - 5)
                return "..." + fileName.Substring(Math.Max(0, fileName.Length - maxLength + 5));
                
            int charsForDirectory = maxLength - fileName.Length - 5;
            string truncatedDirectory = directory.Substring(0, Math.Min(charsForDirectory, directory.Length));
            
            return truncatedDirectory + "..." + fileName;
        }
    }
} 