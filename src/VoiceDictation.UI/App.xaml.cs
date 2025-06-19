using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Serilog;
using System;
using System.IO;
using System.Windows;
using VoiceDictation.Core.SpeechRecognition;
using VoiceDictation.Network.Proxy;
using VoiceDictation.UI.ViewModels;
using VoiceDictation.UI.Views;
using VoiceDictation.UI.Models;

namespace VoiceDictation.UI
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private readonly ServiceProvider _serviceProvider;
        
        /// <summary>
        /// Gets the service provider
        /// </summary>
        public ServiceProvider ServiceProvider => _serviceProvider;
        
        public App()
        {
            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Information()
                .WriteTo.Console()
                .WriteTo.File("logs/voice_dictation-.log", rollingInterval: RollingInterval.Day)
                .CreateLogger();
            
            var services = new ServiceCollection();
            ConfigureServices(services);
            
            _serviceProvider = services.BuildServiceProvider();
        }
        
        private void ConfigureServices(IServiceCollection services)
        {
            services.AddLogging(configure => 
            {
                configure.AddSerilog(dispose: true);
            });
            
            services.AddHttpClient();
            services.AddHttpClient("ProxyTest", client => 
            {
                client.Timeout = TimeSpan.FromSeconds(10);
            });
            
            services.AddSingleton<IProxyManager, ProxyManager>();
            
            services.AddTransient<MicrosoftSpeechRecognizer>(provider => 
            {
                var logger = provider.GetRequiredService<ILogger<MicrosoftSpeechRecognizer>>();
                return new MicrosoftSpeechRecognizer(logger);
            });
            
            services.AddTransient<PythonSpeechRecognizer>(provider => 
            {
                var logger = provider.GetRequiredService<ILogger<PythonSpeechRecognizer>>();
                var pythonModulesPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "PythonModules");
                
                return new PythonSpeechRecognizer(logger, pythonModulesPath);
            });
            
            services.AddTransient<ISpeechRecognizer>(provider => 
            {
                // Get preferred recognizer from settings
                string preferredRecognizer = Models.AppSettings.Instance.PreferredRecognizer;
                
                if (preferredRecognizer == "Microsoft")
                {
                    return provider.GetRequiredService<MicrosoftSpeechRecognizer>();
                }
                else
                {
                    return provider.GetRequiredService<PythonSpeechRecognizer>();
                }
            });
            
            services.AddTransient<MainViewModel>();
            services.AddTransient<SettingsViewModel>();
            
            services.AddTransient<MainWindow>();
            services.AddTransient<SettingsWindow>();
        }
        
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            
            var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
            mainWindow.Show();
        }
        
        protected override void OnExit(ExitEventArgs e)
        {
            Log.CloseAndFlush();
            base.OnExit(e);
        }
    }
} 