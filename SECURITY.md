# Политика безопасности

## О приложении
"Голосовой Ввод" - приложение для голосового распознавания текста, которое конвертирует речь в текст с использованием передовых технологий распознавания.

## Поддерживаемые версии

| Версия | Статус | Примечания |
| ---------------- | --------------- | ------------------ |
| v1.0.x            | ✅ Поддерживается | Текущая стабильная версия |
| v0.9.x            | ✅ Поддерживается | Скоро прекратит поддержку |
| v0.x.x-alpha, v0.x.x-beta | ⚠️ Pre-release | For testing only |
| < v0.9.0          | ❌ Не поддерживается | Устаревшие версии |

> **Примечание:** Текущая версия приложения отображается в заголовке окна и статусной строке.
> The application follows [Semantic Versioning](https://semver.org/) standard (vMAJOR.MINOR.PATCH).

## Сообщение об уязвимостях

### Процесс сообщения
1. **Важно:** Не раскрывайте информацию об уязвимости публично
2. Отправьте подробное описание на: **gbog.masters@gmail.com**
3. В сообщении укажите:
   - Версию приложения
   - Детальное описание проблемы
   - Шаги для воспроизведения
   - Потенциальные последствия
   - Возможные решения (если есть)

### Ожидаемые сроки
| Этап | Срок |
| ------------ | ---------------- |
| Подтверждение получения | 48 часов |
| Начальная оценка | 7 дней |
| Исправление | 14-30 дней (зависит от сложности) |

## Обработка данных

### Типы обрабатываемых данных
- **Аудиозаписи:** Временно, только во время распознавания
- **Распознанный текст:** Хранится локально в приложении

### Политика конфиденциальности
- Мы не сохраняем ваши аудиозаписи
- При использовании Google Speech Recognition API данные передаются согласно их политике конфиденциальности
- Приложение не отправляет данные третьим лицам, кроме API распознавания

## Рекомендации по безопасности

### Для пользователей
1. **Обновления:** Регулярно обновляйте приложение до последней версии
2. **Конфиденциальные данные:** Не используйте для диктовки конфиденциальной информации
3. **Очистка данных:** Используйте кнопку "Очистить" после завершения работы
4. **Безопасные соединения:** Используйте приложение только в безопасных сетях
5. **Микрофон:** Предоставляйте доступ к микрофону только когда это необходимо
6. **Буфер обмена:** Очищайте буфер обмена после использования конфиденциальных данных
7. **Логи:** Регулярно очищайте логи приложения в директории `logs/`

### Для разработчиков
1. **Валидация ввода:** Всегда проверяйте все входные данные
2. **Обработка ошибок:** Используйте блоки try-except для безопасной обработки ошибок
3. **Логирование:** Не логируйте конфиденциальные данные
4. **Управление зависимостями:** Регулярно обновляйте зависимости
5. **Защита кода:** Храните конфиденциальные данные в переменных окружения
6. **Проверка прав:** Проверяйте права доступа к файлам и директориям
7. **Санитизация вывода:** Очищайте выходные данные от потенциально опасного контента

## Контактная информация
- Email для вопросов безопасности: gbog.masters@gmail.com
- Сайт проекта: [URL сайта]

---

# Security Policy

## About Application
"Voice Input" - an application for voice recognition that converts speech to text using advanced recognition technologies.

## Supported Versions

| Version | Status | Notes |
| ---------------- | --------------- | ------------------ |
| v1.0.x            | ✅ Supported | Current stable version |
| v0.9.x            | ✅ Supported | Support ending soon |
| v0.x.x-alpha, v0.x.x-beta | ⚠️ Pre-release | For testing only |
| < v0.9.0          | ❌ Not supported | Deprecated versions |

> **Note:** Current version is displayed in the window header and status bar.
> The application follows [Semantic Versioning](https://semver.org/) standard (vMAJOR.MINOR.PATCH).

## Reporting Vulnerabilities

### Reporting Process
1. **Important:** Do not disclose vulnerability information publicly
2. Send detailed description to: **gbog.masters@gmail.com**
3. Include in your report:
   - Application version
   - Detailed problem description
   - Steps to reproduce
   - Potential consequences
   - Possible solutions (if any)

### Expected Timeframes
| Stage | Timeframe |
| ------------ | ---------------- |
| Receipt confirmation | 48 hours |
| Initial assessment | 7 days |
| Fix release | 14-30 days (depends on complexity) |

## Data Processing

### Types of Processed Data
- **Audio recordings:** Temporarily, only during recognition
- **Recognized text:** Stored locally in the application

### Privacy Policy
- We do not store your audio recordings
- When using Google Speech Recognition API, data is processed according to their privacy policy
- The application does not share data with third parties except recognition APIs

## Security Recommendations

### For Users
1. **Updates:** Regularly update the application to the latest version
2. **Sensitive data:** Do not use for dictating confidential information
3. **Data clearing:** Use the "Clear" button after finishing your work
4. **Secure connections:** Use the application only on secure networks
5. **Microphone:** Grant microphone access only when necessary
6. **Clipboard:** Clear clipboard after using sensitive data
7. **Logs:** Regularly clear application logs in the `logs/` directory

### For Developers
1. **Input validation:** Always validate all input data
2. **Error handling:** Use try-except blocks for safe error handling
3. **Logging:** Do not log sensitive data
4. **Dependency management:** Regularly update dependencies
5. **Code protection:** Store sensitive data in environment variables
6. **Permission checks:** Verify file and directory access rights
7. **Output sanitization:** Clean output data from potentially dangerous content

## Contact Information
- Email for security issues: gbog.masters@gmail.com
- Project website: [Website URL]