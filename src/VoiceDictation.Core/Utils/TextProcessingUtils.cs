using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
using System.Linq;

namespace VoiceDictation.Core.Utils
{
    /// <summary>
    /// Utility class for text processing operations
    /// </summary>
    public static class TextProcessingUtils
    {
        public static bool ContainsMixedScript(string text)
        {
            if (string.IsNullOrEmpty(text))
                return false;

            bool hasCyrillic = text.Any(c => c >= '\u0400' && c <= '\u04FF');
            bool hasLatin = text.Any(c => (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));

            return hasCyrillic && hasLatin;
        }

        public static bool IsLikelyTechnicalTerm(string word)
        {
            if (string.IsNullOrEmpty(word) || word.Length < 2)
                return false;

            string[] techMarkers = {
                "api", "sdk", "http", "json", "xml", "url", "sql", "db",
                "app", "dev", "git", "npm", "css", "html", "js", "py",
                "ui", "cli", "id", "ip", "yaml", "toml", "ssl", "ssh",
                "ftp", "rest", "oauth", "jwt", "spa", "cdn", "cors"
            };

            string wordLower = word.ToLowerInvariant();

            if (techMarkers.Any(marker => wordLower.Contains(marker)))
                return true;

            bool hasLower = word.Any(char.IsLower);
            bool hasUpper = word.Any(char.IsUpper);
            bool hasDigit = word.Any(char.IsDigit);

            if (hasLower && hasUpper)
            {
                for (int i = 1; i < word.Length; i++)
                {
                    if (char.IsLower(word[i - 1]) && char.IsUpper(word[i]))
                        return true;
                }
            }

            if (hasDigit && (hasLower || hasUpper))
                return true;

            if (word.Contains('_') || word.Contains('-'))
                return true;

            return false;
        }

        public static string TransliterateLatinToCyrillic(string text)
        {
            if (string.IsNullOrEmpty(text))
                return text;

            var translitMap = new Dictionary<string, string>
            {
                {"a", "а"}, {"b", "б"}, {"v", "в"}, {"g", "г"}, {"d", "д"}, {"e", "е"},
                {"yo", "ё"}, {"zh", "ж"}, {"z", "з"}, {"i", "и"}, {"j", "й"}, {"k", "к"},
                {"l", "л"}, {"m", "м"}, {"n", "н"}, {"o", "о"}, {"p", "п"}, {"r", "р"},
                {"s", "с"}, {"t", "т"}, {"u", "у"}, {"f", "ф"}, {"h", "х"}, {"ts", "ц"},
                {"ch", "ч"}, {"sh", "ш"}, {"sch", "щ"}, {"y", "ы"}, {"e", "э"}, {"yu", "ю"},
                {"ya", "я"},
                // Capital letters
                {"A", "А"}, {"B", "Б"}, {"V", "В"}, {"G", "Г"}, {"D", "Д"}, {"E", "Е"},
                {"Yo", "Ё"}, {"Zh", "Ж"}, {"Z", "З"}, {"I", "И"}, {"J", "Й"}, {"K", "К"},
                {"L", "Л"}, {"M", "М"}, {"N", "Н"}, {"O", "О"}, {"P", "П"}, {"R", "Р"},
                {"S", "С"}, {"T", "Т"}, {"U", "У"}, {"F", "Ф"}, {"H", "Х"}, {"Ts", "Ц"},
                {"Ch", "Ч"}, {"Sh", "Ш"}, {"Sch", "Щ"}, {"Y", "Ы"}, {"E", "Э"}, {"Yu", "Ю"},
                {"Ya", "Я"}
            };

            foreach (var pair in translitMap.OrderByDescending(p => p.Key.Length))
            {
                text = text.Replace(pair.Key, pair.Value);
            }

            return text;
        }

        public static string ProcessMixedLanguageText(string text, string primaryLanguage)
        {
            if (string.IsNullOrEmpty(text) || !ContainsMixedScript(text))
                return text;

            var words = text.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            var processedWords = new List<string>(words.Length);

            foreach (var word in words)
            {
                if (string.IsNullOrEmpty(word))
                {
                    processedWords.Add(word);
                    continue;
                }

                bool hasCyrillic = word.Any(c => c >= '\u0400' && c <= '\u04FF');
                bool hasLatin = word.Any(c => (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));

                if (hasCyrillic && hasLatin)
                {
                    int cyrillicCount = word.Count(c => c >= '\u0400' && c <= '\u04FF');
                    int latinCount = word.Count(c => (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));

                    if (primaryLanguage.StartsWith("ru", StringComparison.OrdinalIgnoreCase))
                    {
                        if (latinCount > cyrillicCount && !IsLikelyTechnicalTerm(word))
                        {
                            processedWords.Add(TransliterateLatinToCyrillic(word));
                        }
                        else
                        {
                            processedWords.Add(word);
                        }
                    }
                    else
                    {
                        processedWords.Add(word);
                    }
                }
                else
                {
                    processedWords.Add(word);
                }
            }

            return string.Join(" ", processedWords);
        }

        public static string FormatRecognizedText(string text)
        {
            if (string.IsNullOrEmpty(text))
                return text;

            var sentenceRegex = new Regex(@"(^|[.!?]\s+)([a-zа-яё])", RegexOptions.Compiled);
            text = sentenceRegex.Replace(text, m => m.Groups[1].Value + m.Groups[2].Value.ToUpperInvariant());

            return text;
        }
    }
} 