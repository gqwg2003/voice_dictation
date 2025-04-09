#include "TextProcessor.h"
#include "../utils/Logger.h"

#include <QDebug>
#include <QStringList>

// External global logger
extern Logger* gLogger;

TextProcessor::TextProcessor(QObject* parent)
    : QObject(parent),
      m_capitalizeFirstSentence(true),
      m_autoCorrect(true),
      m_addPunctuationMarks(true)
{
    initializeLanguageRules();
}

TextProcessor::~TextProcessor()
{
}

QString TextProcessor::processText(const QString& text, const QString& languageCode)
{
    if (text.isEmpty()) {
        return text;
    }
    
    gLogger->info("Processing text with language: " + languageCode.toStdString());
    
    QString processedText = text;
    
    // Apply text processing steps in sequence
    if (m_autoCorrect) {
        processedText = applyAutoCorrect(processedText, languageCode);
    }
    
    if (m_addPunctuationMarks) {
        processedText = addPunctuation(processedText, languageCode);
    }
    
    if (m_capitalizeFirstSentence) {
        processedText = capitalizeFirstLetter(processedText);
    }
    
    // Apply custom substitutions last
    processedText = applySubstitutions(processedText);
    
    return processedText;
}

void TextProcessor::setCapitalizeFirstSentence(bool enable)
{
    m_capitalizeFirstSentence = enable;
}

bool TextProcessor::getCapitalizeFirstSentence() const
{
    return m_capitalizeFirstSentence;
}

void TextProcessor::setAutoCorrect(bool enable)
{
    m_autoCorrect = enable;
}

bool TextProcessor::getAutoCorrect() const
{
    return m_autoCorrect;
}

void TextProcessor::setAddPunctuationMarks(bool enable)
{
    m_addPunctuationMarks = enable;
}

bool TextProcessor::getAddPunctuationMarks() const
{
    return m_addPunctuationMarks;
}

void TextProcessor::addSubstitution(const QString& originalText, const QString& replacement)
{
    if (!originalText.isEmpty()) {
        m_substitutions[originalText] = replacement;
    }
}

void TextProcessor::removeSubstitution(const QString& originalText)
{
    m_substitutions.remove(originalText);
}

QMap<QString, QString> TextProcessor::getSubstitutions() const
{
    return m_substitutions;
}

void TextProcessor::clearSubstitutions()
{
    m_substitutions.clear();
}

QString TextProcessor::capitalizeFirstLetter(const QString& text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    QString result = text;
    result[0] = result[0].toUpper();
    return result;
}

QString TextProcessor::applyAutoCorrect(const QString& text, const QString& languageCode)
{
    QString result = text;
    
    // Apply common corrections for the language
    if (m_languageRules.contains(languageCode)) {
        const auto& corrections = m_languageRules[languageCode].commonCorrections;
        
        QMapIterator<QString, QString> i(corrections);
        while (i.hasNext()) {
            i.next();
            result.replace(QRegularExpression("\\b" + i.key() + "\\b"), i.value());
        }
    }
    
    return result;
}

QString TextProcessor::addPunctuation(const QString& text, const QString& languageCode)
{
    QString result = text;
    
    // If the text doesn't end with a punctuation mark, add a period
    static const QRegularExpression endsWithPunctuation("[.!?]\\s*$");
    
    if (!endsWithPunctuation.match(result).hasMatch()) {
        result.append(".");
    }
    
    return result;
}

QString TextProcessor::applySubstitutions(const QString& text)
{
    QString result = text;
    
    // Apply custom user substitutions
    QMapIterator<QString, QString> i(m_substitutions);
    while (i.hasNext()) {
        i.next();
        result.replace(QRegularExpression("\\b" + i.key() + "\\b"), i.value());
    }
    
    return result;
}

void TextProcessor::initializeLanguageRules()
{
    gLogger->info("Initializing language rules");
    
    // English rules
    LanguageRules enRules;
    enRules.sentenceEndPattern = QRegularExpression("[.!?]\\s+");
    enRules.wordPattern = QRegularExpression("\\b\\w+\\b");
    
    // Common English corrections
    enRules.commonCorrections = {
        {"i", "I"},
        {"dont", "don't"},
        {"cant", "can't"},
        {"wont", "won't"},
        {"im", "I'm"},
        {"didnt", "didn't"},
        {"isnt", "isn't"},
        {"wasnt", "wasn't"},
        {"wouldnt", "wouldn't"},
        {"couldnt", "couldn't"},
        {"shouldnt", "shouldn't"},
        {"its", "it's"},  // Note: This is simplified, context-dependent
        {"thats", "that's"},
        {"whats", "what's"},
        {"hes", "he's"},
        {"shes", "she's"},
        {"theyre", "they're"},
        {"theyll", "they'll"},
        {"youre", "you're"},
        {"youll", "you'll"},
        {"weve", "we've"},
        {"youd", "you'd"}
    };
    
    // Russian rules
    LanguageRules ruRules;
    ruRules.sentenceEndPattern = QRegularExpression("[.!?]\\s+");
    ruRules.wordPattern = QRegularExpression("\\b\\w+\\b");
    
    // Common Russian corrections (examples)
    ruRules.commonCorrections = {
        {"щас", "сейчас"},
        {"че", "что"},
        {"нетути", "нет"},
        {"ваще", "вообще"},
        {"тыщ", "тысяч"},
        {"чо", "что"},
        {"нету", "нет"},
        {"тож", "тоже"},
        {"седня", "сегодня"},
        {"тока", "только"},
        {"инет", "интернет"},
        {"канеш", "конечно"}
    };
    
    // Add rules to map
    m_languageRules["en-US"] = enRules;
    m_languageRules["ru-RU"] = ruRules;
} 