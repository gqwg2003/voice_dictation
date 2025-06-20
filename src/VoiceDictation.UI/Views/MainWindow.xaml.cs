using MahApps.Metro.Controls;
using System;
using System.Windows;
using System.Windows.Input;
using VoiceDictation.UI.ViewModels;
using Microsoft.Extensions.Logging;

namespace VoiceDictation.UI.Views
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : MetroWindow
    {
        private readonly MainViewModel _viewModel;
        private readonly ILogger<MainWindow> _logger;
        
        public MainWindow(MainViewModel viewModel, ILogger<MainWindow> logger)
        {
            InitializeComponent();
            
            _viewModel = viewModel ?? throw new ArgumentNullException(nameof(viewModel));
            DataContext = _viewModel;
            
            Loaded += MainWindow_Loaded;
            Closing += MainWindow_Closing;
            
            PreviewKeyDown += MainWindow_PreviewKeyDown;
            
            _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        }
        
        private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await _viewModel.InitializeAsync();
        }
        
        private void MainWindow_Closing(object? sender, System.ComponentModel.CancelEventArgs e)
        {
            try
            {
                _viewModel.Cleanup();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error during main window cleanup");
            }
        }
        
        /// <summary>
        /// Handles the PreviewKeyDown event to process hotkeys
        /// </summary>
        private void MainWindow_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.R && 
                Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && 
                Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                if (_viewModel.StartRecognitionCommand.CanExecute(null))
                {
                    _viewModel.StartRecognitionCommand.Execute(null);
                    e.Handled = true;
                }
            }
            
            if (e.Key == Key.Escape)
            {
                if (_viewModel.StopRecognitionCommand.CanExecute(null))
                {
                    _viewModel.StopRecognitionCommand.Execute(null);
                    e.Handled = true;
                }
            }
        }
    }
} 