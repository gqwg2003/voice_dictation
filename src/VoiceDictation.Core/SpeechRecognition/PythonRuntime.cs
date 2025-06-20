using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Extensions.Logging;
using Python.Runtime;

namespace VoiceDictation.Core.SpeechRecognition
{
    /// <summary>
    /// Manages Python runtime initialization and execution
    /// </summary>
    public class PythonRuntime : IDisposable
    {
        private readonly ILogger _logger;
        private readonly string _pythonModulesPath;
        private bool _pythonInitialized;
        private bool _isDisposed;
        private string? _pythonHome;
        private readonly object _pythonLock = new object();
        
        /// <summary>
        /// Initializes a new instance of the <see cref="PythonRuntime"/> class
        /// </summary>
        /// <param name="logger">Logger instance</param>
        /// <param name="pythonModulesPath">Path to Python modules</param>
        public PythonRuntime(ILogger logger, string pythonModulesPath)
        {
            _logger = logger;
            _pythonModulesPath = pythonModulesPath;
            
            _pythonHome = Environment.GetEnvironmentVariable("PYTHONHOME");
            if (string.IsNullOrEmpty(_pythonHome))
            {
                var possiblePaths = new[]
                {
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python39"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python310"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python311"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Python312"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python39"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python310"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python311"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Python312"),
                    @"C:\Python39",
                    @"C:\Python310",
                    @"C:\Python311",
                    @"C:\Python312"
                };
                
                foreach (var path in possiblePaths)
                {
                    if (Directory.Exists(path) && File.Exists(Path.Combine(path, "python.exe")))
                    {
                        _pythonHome = path;
                        _logger.LogInformation("Found Python installation at {PythonHome}", _pythonHome);
                        break;
                    }
                }
            }
            
            try
            {
                Initialize();
            }
            catch (Exception ex)
            {
                _logger.LogWarning(ex, "Failed to initialize Python runtime. Will attempt to initialize on first use.");
            }
        }
        
        /// <summary>
        /// Gets a value indicating whether Python runtime is initialized
        /// </summary>
        public bool IsInitialized => _pythonInitialized;
        
        /// <summary>
        /// Initializes Python runtime and loads necessary modules
        /// </summary>
        public void Initialize()
        {
            if (_pythonInitialized)
            {
                return;
            }
            
            lock (_pythonLock)
            {
                if (_pythonInitialized)
                {
                    return;
                }
                
                try
                {
                    if (!string.IsNullOrEmpty(_pythonHome))
                    {
                        Runtime.PythonDLL = Path.Combine(_pythonHome, "python3.dll");
                        if (!File.Exists(Runtime.PythonDLL))
                        {
                            Runtime.PythonDLL = Path.Combine(_pythonHome, "python310.dll");
                        }
                        if (!File.Exists(Runtime.PythonDLL))
                        {
                            Runtime.PythonDLL = Path.Combine(_pythonHome, "python39.dll");
                        }
                        
                        PythonEngine.PythonHome = _pythonHome;
                        _logger.LogInformation("Set Python home to: {PythonHome}", _pythonHome);
                    }
                    
                    PythonEngine.Initialize();
                    _logger.LogInformation("Python engine initialized");
                    
                    using (Py.GIL())
                    {
                        dynamic sys = Py.Import("sys");
                        dynamic path = sys.path;
                        path.append(new PyString(_pythonModulesPath));
                    }
                    
                    _logger.LogInformation("Python runtime initialized successfully");
                    _pythonInitialized = true;
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error initializing Python runtime");
                    throw new InvalidOperationException("Error initializing Python runtime", ex);
                }
            }
        }
        
        /// <summary>
        /// Ensures that Python runtime is initialized
        /// </summary>
        public void EnsureInitialized()
        {
            if (!_pythonInitialized)
            {
                Initialize();
            }
        }
        
        /// <summary>
        /// Executes a Python function with the given parameters
        /// </summary>
        public string ExecutePythonFunction(dynamic pythonObject, string functionName, Dictionary<string, object> parameters)
        {
            if (!_pythonInitialized)
            {
                throw new InvalidOperationException("Python runtime is not initialized");
            }
            
            lock (_pythonLock)
            {
                try
                {
                    using (Py.GIL())
                    {
                        var method = pythonObject.GetAttr(functionName);
                        
                        var kwargs = new PyDict();
                        foreach (var param in parameters)
                        {
                            kwargs[param.Key.ToPython()] = param.Value.ToPython();
                        }
                        
                        var result = method.Invoke(kwargs);
                        
                        var resultStr = result.ToString();
                        return resultStr;
                    }
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error executing Python function {FunctionName}", functionName);
                    throw;
                }
            }
        }
        
        /// <summary>
        /// Imports a Python module
        /// </summary>
        public dynamic ImportModule(string moduleName)
        {
            EnsureInitialized();
            
            lock (_pythonLock)
            {
                try
                {
                    using (Py.GIL())
                    {
                        return Py.Import(moduleName);
                    }
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error importing Python module {ModuleName}", moduleName);
                    throw;
                }
            }
        }
        
        /// <summary>
        /// Shuts down Python runtime
        /// </summary>
        public void Shutdown()
        {
            if (!_pythonInitialized)
            {
                return;
            }
            
            lock (_pythonLock)
            {
                try
                {
                    PythonEngine.Shutdown();
                    _pythonInitialized = false;
                    _logger.LogInformation("Python runtime shutdown completed");
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error shutting down Python runtime");
                }
            }
        }
        
        /// <summary>
        /// Disposes the Python runtime
        /// </summary>
        public void Dispose()
        {
            if (_isDisposed)
            {
                return;
            }
            
            Shutdown();
            
            _isDisposed = true;
        }
    }
} 