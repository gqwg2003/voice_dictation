import requests
import json
import os
import socket
import sys
import time
import logging
import subprocess
from typing import Dict, List, Optional, Any, Tuple

logger = logging.getLogger(__name__)

class ProxyManager:
    """
    Manages proxy connections for the voice dictation application
    """
    
    def __init__(self, config_file: Optional[str] = None):
        self.proxies: Dict[str, Dict[str, Any]] = {}
        self.current_proxy: Optional[Dict[str, str]] = None
        self.config_file = config_file
        
        if config_file and os.path.exists(config_file):
            self.load_config(config_file)
    
    def load_config(self, config_file: str) -> bool:
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
                
            if 'proxies' in config and isinstance(config['proxies'], list):
                for proxy_config in config['proxies']:
                    if 'id' in proxy_config and 'http' in proxy_config:
                        self.proxies[proxy_config['id']] = proxy_config
                
                logger.info(f"Loaded {len(self.proxies)} proxies from config file")
                return True
            else:
                logger.error("Invalid proxy configuration format")
                return False
                
        except Exception as e:
            logger.error(f"Error loading proxy configuration: {str(e)}")
            return False
    
    def save_config(self, config_file: Optional[str] = None) -> bool:
        if not config_file:
            config_file = self.config_file
            
        if not config_file:
            logger.error("No config file specified")
            return False
            
        try:
            config = {
                'proxies': list(self.proxies.values())
            }
            
            with open(config_file, 'w', encoding='utf-8') as f:
                json.dump(config, f, indent=2)
                
            logger.info(f"Saved proxy configuration to {config_file}")
            return True
            
        except Exception as e:
            logger.error(f"Error saving proxy configuration: {str(e)}")
            return False
    
    def add_proxy(self, proxy_id: str, http_proxy: str, https_proxy: Optional[str] = None, 
                  username: Optional[str] = None, password: Optional[str] = None) -> bool:
        if not https_proxy:
            https_proxy = http_proxy
            
        proxy_config = {
            'id': proxy_id,
            'http': http_proxy,
            'https': https_proxy
        }
        
        if username:
            proxy_config['username'] = username
            
        if password:
            proxy_config['password'] = password
            
        self.proxies[proxy_id] = proxy_config
        logger.info(f"Added proxy: {proxy_id}")
        return True
    
    def remove_proxy(self, proxy_id: str) -> bool:
        if proxy_id in self.proxies:
            del self.proxies[proxy_id]
            logger.info(f"Removed proxy: {proxy_id}")
            
            if self.current_proxy and self.current_proxy.get('id') == proxy_id:
                self.current_proxy = None
                
            return True
        else:
            logger.warning(f"Proxy not found: {proxy_id}")
            return False
    
    def get_proxy_list(self) -> List[Dict[str, Any]]:
        return list(self.proxies.values())
    
    def select_proxy(self, proxy_id: str) -> Dict[str, str]:
        if proxy_id not in self.proxies:
            raise KeyError(f"Proxy not found: {proxy_id}")
            
        proxy_config = self.proxies[proxy_id]
        
        proxy_dict = {
            'http': proxy_config['http'],
            'https': proxy_config['https']
        }
        
        if 'username' in proxy_config and 'password' in proxy_config:
            for protocol in ['http', 'https']:
                url = proxy_dict[protocol]
                if '//' in url and '@' not in url.split('//', 1)[1]:
                    parts = url.split('//', 1)
                    proxy_dict[protocol] = f"{parts[0]}//{proxy_config['username']}:{proxy_config['password']}@{parts[1]}"
        
        self.current_proxy = proxy_config
        self.current_proxy['formatted'] = proxy_dict
        
        logger.info(f"Selected proxy: {proxy_id}")
        return proxy_dict
    
    def get_current_proxy(self) -> Optional[Dict[str, str]]:
        if not self.current_proxy:
            return None
            
        if 'formatted' in self.current_proxy:
            return self.current_proxy['formatted']
            
        return self.select_proxy(self.current_proxy['id'])
    
    def test_proxy(self, proxy_id: Optional[str] = None, test_url: str = "https://www.google.com") -> Tuple[bool, str]:
        try:
            if proxy_id:
                proxy_dict = self.select_proxy(proxy_id)
            else:
                proxy_dict = self.get_current_proxy()
                
            if not proxy_dict:
                return False, "No proxy selected"
                
            session = requests.Session()
            session.proxies.update(proxy_dict)
            
            start_time = time.time()
            response = session.get(test_url, timeout=10)
            elapsed_time = time.time() - start_time
            
            if response.status_code == 200:
                return True, f"Proxy is working. Response time: {elapsed_time:.2f}s"
            else:
                return False, f"Proxy test failed. Status code: {response.status_code}"
                
        except requests.exceptions.RequestException as e:
            return False, f"Proxy test failed: {str(e)}"
        except Exception as e:
            return False, f"Error testing proxy: {str(e)}"
    
    def detect_system_proxy(self) -> Optional[Dict[str, str]]:
        if sys.platform.startswith('win'):
            proxy_settings = self._get_windows_proxy_settings()
            if proxy_settings:
                proxy_id = "system_proxy"
                self.add_proxy(
                    proxy_id=proxy_id,
                    http_proxy=proxy_settings.get('http', ''),
                    https_proxy=proxy_settings.get('https', ''),
                    username=proxy_settings.get('username', None),
                    password=proxy_settings.get('password', None)
                )
                return self.select_proxy(proxy_id)
                
        elif sys.platform.startswith('linux'):
            http_proxy = os.environ.get('http_proxy') or os.environ.get('HTTP_PROXY')
            https_proxy = os.environ.get('https_proxy') or os.environ.get('HTTPS_PROXY')
            
            if http_proxy or https_proxy:
                proxy_id = "system_proxy"
                self.add_proxy(
                    proxy_id=proxy_id,
                    http_proxy=http_proxy or '',
                    https_proxy=https_proxy or ''
                )
                return self.select_proxy(proxy_id)
                
        return None
    
    def _get_windows_proxy_settings(self) -> Optional[Dict[str, Any]]:
        try:
            import winreg
            
            proxy_settings = {}
            
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, 
                               r"Software\Microsoft\Windows\CurrentVersion\Internet Settings") as key:
                
                try:
                    proxy_enable, _ = winreg.QueryValueEx(key, "ProxyEnable")
                    if not proxy_enable:
                        return None
                except FileNotFoundError:
                    return None
                    
                try:
                    proxy_server, _ = winreg.QueryValueEx(key, "ProxyServer")
                    if not proxy_server:
                        return None
                        
                    if "=" in proxy_server:
                        for part in proxy_server.split(";"):
                            if "=" in part:
                                protocol, address = part.split("=", 1)
                                protocol = protocol.lower()
                                if protocol in ["http", "https"]:
                                    proxy_settings[protocol] = address
                    else:
                        proxy_settings["http"] = proxy_server
                        proxy_settings["https"] = proxy_server
                        
                except FileNotFoundError:
                    return None
                    
                try:
                    proxy_bypass, _ = winreg.QueryValueEx(key, "ProxyOverride")
                    if proxy_bypass:
                        proxy_settings["no_proxy"] = proxy_bypass.replace(";", ",")
                except FileNotFoundError:
                    pass
                    
                try:
                    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, 
                                       r"Software\Microsoft\Windows\CurrentVersion\Internet Settings\Connections") as conn_key:
                        saved_connections, _ = winreg.QueryValueEx(conn_key, "SavedConnections")
                except (FileNotFoundError, OSError):
                    pass
                    
            return proxy_settings
                
        except (ImportError, OSError) as e:
            logger.error(f"Error detecting Windows proxy settings: {str(e)}")
            return None

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, 
                        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    
    pm = ProxyManager()
    
    system_proxy = pm.detect_system_proxy()
    if system_proxy:
        print(f"Detected system proxy: {system_proxy}")
    else:
        print("No system proxy detected")
        
    pm.add_proxy("test", "http://localhost:8080")
    
    print("\nAvailable proxies:")
    for proxy in pm.get_proxy_list():
        print(f"  - {proxy['id']}: {proxy['http']}")
        
    print("\nTesting proxies:")
    for proxy in pm.get_proxy_list():
        success, message = pm.test_proxy(proxy['id'])
        status = "✓" if success else "✗"
        print(f"  - {proxy['id']}: {status} {message}") 