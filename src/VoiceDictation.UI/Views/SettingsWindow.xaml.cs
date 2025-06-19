using MahApps.Metro.Controls;
using System;
using System.Windows;
using VoiceDictation.UI.ViewModels;

namespace VoiceDictation.UI.Views
{
    /// <summary>
    /// Interaction logic for SettingsWindow.xaml
    /// </summary>
    public partial class SettingsWindow : MetroWindow
    {
        private readonly SettingsViewModel _viewModel;
        
        public SettingsWindow(SettingsViewModel viewModel)
        {
            InitializeComponent();
            
            _viewModel = viewModel ?? throw new ArgumentNullException(nameof(viewModel));
            DataContext = _viewModel;
            
            Loaded += SettingsWindow_Loaded;
            Closing += SettingsWindow_Closing;
        }
        
        private async void SettingsWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await _viewModel.InitializeAsync();
        }
        
        private void SettingsWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            _viewModel.Cleanup();
        }
        

        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
} 