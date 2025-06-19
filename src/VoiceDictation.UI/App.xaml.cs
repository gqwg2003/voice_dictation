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

namespace VoiceDictation.UI
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private readonly IServiceProvider _serviceProvider;
        
        public App()
        {
            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Debug()
                .WriteTo.Console()
                .WriteTo.File(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "logs", "log-.txt"), 
                    rollingInterval: RollingInterval.Day)
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
            
            services.AddTransient<ISpeechRecognizer>(provider => 
            {
                var logger = provider.GetRequiredService<ILogger<PythonSpeechRecognizer>>();
                var pythonModulesPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "PythonModules");
                
                return new PythonSpeechRecognizer(logger, pythonModulesPath);
            });
            
            services.AddTransient<ISpeechRecognizer, MicrosoftSpeechRecognizer>();
            
            services.AddTransient<MainViewModel>();
            services.AddTransient<SettingsViewModel>();
            
            services.AddTransient<MainWindow>();
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