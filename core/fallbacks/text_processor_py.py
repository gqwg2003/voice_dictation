#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Python-реализация TextProcessor для работы в чистом Python режиме.
"""

import re
import logging
from typing import List, Dict, Optional, Tuple

logger = logging.getLogger(__name__)

class TextProcessor:
    """
    Python-версия класса для обработки текста.
    Реализует тот же интерфейс, что и C++ версия для обеспечения совместимости.
    """
    
    @staticmethod
    def process(text: str, custom_terms: List[str] = None, similarity_threshold: float = 0.65) -> str:
        """
        Основной метод обработки текста.
        
        Args:
            text: Входной текст для обработки
            custom_terms: Список пользовательских терминов для коррекции
            similarity_threshold: Порог сходства для нечеткого поиска
            
        Returns:
            Обработанный текст
        """
        if not text:
            return text
            
        custom_terms = custom_terms or []
        
        # Создаем копию текста для обработки
        result = text
        
        # Применяем пользовательские термины
        result = TextProcessor.apply_custom_terms(result, custom_terms, similarity_threshold)
        
        # Исправляем пунктуацию
        result = TextProcessor._fix_punctuation(result)
        
        # Исправляем регистр
        result = TextProcessor._fix_capitalization(result)
        
        return result
    
    @staticmethod
    def apply_custom_terms(text: str, custom_terms: List[str], similarity_threshold: float = 0.65) -> str:
        """
        Улучшенная версия для работы с пользовательскими терминами.
        
        Args:
            text: Входной текст
            custom_terms: Список пользовательских терминов
            similarity_threshold: Порог сходства для нечеткого поиска
            
        Returns:
            Текст с примененными пользовательскими терминами
        """
        if not text or not custom_terms:
            return text
            
        result = text
        words = TextProcessor._split_words(text)
        
        for term in custom_terms:
            # Для точного совпадения
            pattern = r'\b' + re.escape(term) + r'\b'
            try:
                # re.IGNORECASE - аналог std::regex_constants::icase
                result = re.sub(pattern, term, result, flags=re.IGNORECASE)
            except re.error:
                # Игнорируем ошибки регулярных выражений
                continue
                
            # Для нечеткого совпадения
            for word in words:
                if len(word) > 2 and len(term) > 2:
                    similarity = TextProcessor._levenshtein_distance(
                        TextProcessor._to_lower(word), 
                        TextProcessor._to_lower(term)
                    )
                    if similarity >= similarity_threshold and similarity < 1.0:
                        try:
                            word_pattern = r'\b' + re.escape(word) + r'\b'
                            result = re.sub(word_pattern, term, result, flags=re.IGNORECASE)
                        except re.error:
                            # Игнорируем ошибки регулярных выражений
                            continue
                            
        return result
    
    @staticmethod
    def post_process_text(text: str, fix_case: bool = True, fix_punct: bool = True) -> str:
        """
        Постобработка текста.
        
        Args:
            text: Входной текст
            fix_case: Исправлять регистр
            fix_punct: Исправлять пунктуацию
            
        Returns:
            Обработанный текст
        """
        if not text:
            return text
            
        result = text
        
        if fix_punct:
            result = TextProcessor._fix_punctuation(result)
            
        if fix_case:
            result = TextProcessor._fix_capitalization(result)
            
        return result
    
    @staticmethod
    def detect_language(text: str) -> str:
        """
        Определение языка текста.
        
        Args:
            text: Входной текст
            
        Returns:
            Код языка: 'ru', 'en' или 'unknown'
        """
        if not text:
            return 'unknown'
            
        # Проверяем наличие кириллических символов
        if TextProcessor._contains_cyrillic(text):
            return 'ru'
            
        # По умолчанию считаем английским
        return 'en'
    
    @staticmethod
    def _levenshtein_distance(s1: str, s2: str) -> float:
        """
        Расчет расстояния Левенштейна между двумя строками.
        
        Returns:
            Нормализованное сходство строк (0.0 - 1.0)
        """
        if not s1 or not s2:
            return 0.0
            
        # Создаем матрицу расстояний
        len1, len2 = len(s1), len(s2)
        d = [[0] * (len2 + 1) for _ in range(len1 + 1)]
        
        # Инициализация
        for i in range(len1 + 1):
            d[i][0] = i
        for j in range(len2 + 1):
            d[0][j] = j
            
        # Заполнение матрицы
        for i in range(1, len1 + 1):
            for j in range(1, len2 + 1):
                if s1[i-1] == s2[j-1]:
                    d[i][j] = d[i-1][j-1]
                else:
                    d[i][j] = min(
                        d[i-1][j] + 1,      # удаление
                        d[i][j-1] + 1,      # вставка
                        d[i-1][j-1] + 1     # замена
                    )
                    
        # Нормализация результата
        max_len = max(len1, len2)
        if max_len == 0:
            return 1.0
        return 1.0 - d[len1][len2] / max_len
    
    @staticmethod
    def _to_lower(s: str) -> str:
        """Преобразовать строку в нижний регистр."""
        return s.lower()
    
    @staticmethod
    def _contains_cyrillic(text: str) -> bool:
        """Проверка, содержит ли строка кириллические символы."""
        return bool(re.search('[\u0400-\u04FF]', text))
    
    @staticmethod
    def _split_words(text: str) -> List[str]:
        """Разделение текста на слова."""
        # Аналог C++ split_words
        return re.findall(r'\b[a-zA-Z\u0400-\u04FF\'-]+\b', text)
    
    @staticmethod
    def _fix_capitalization(text: str) -> str:
        """Исправление регистра первых букв предложений."""
        if not text:
            return text
            
        # Разбиваем на предложения и обрабатываем каждое
        sentences = re.split(r'([.!?]\s+)', text)
        result = ""
        
        for i in range(0, len(sentences)):
            if i % 2 == 0:  # Это текст предложения
                sentence = sentences[i]
                if sentence and sentence[0].isalpha():
                    # Делаем первую букву заглавной
                    sentence = sentence[0].upper() + sentence[1:]
                result += sentence
            else:  # Это разделитель
                result += sentences[i]
                
        # Обрабатываем первое предложение, если текст не пустой
        if result and result[0].isalpha():
            result = result[0].upper() + result[1:]
            
        return result
    
    @staticmethod
    def _fix_punctuation(text: str) -> str:
        """Исправление пунктуации."""
        if not text:
            return text
            
        # Заменяем множественные пробелы на один
        result = re.sub(r'\s+', ' ', text)
        
        # Убираем пробелы перед знаками препинания
        result = re.sub(r'\s+([.,!?:;])', r'\1', result)
        
        # Добавляем пробелы после знаков препинания, если за ними следует буква
        result = re.sub(r'([.,!?:;])([a-zA-Z\u0400-\u04FF])', r'\1 \2', result)
        
        return result 