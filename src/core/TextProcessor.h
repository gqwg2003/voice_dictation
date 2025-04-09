#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QRegularExpression>
#include <memory>

class TextProcessor : public QObject {
    Q_OBJECT

public:
    explicit TextProcessor(QObject* parent = nullptr);
    ~TextProcessor();

    // Process text with language-specific rules
    QString processText(const QString& text, const QString& languageCode);
    
    // Text formatting options
    void setCapitalizeFirstSentence(bool enable);
    bool getCapitalizeFirstSentence() const;
    
    void setAutoCorrect(bool enable);
    bool getAutoCorrect() const;
    
    void setAddPunctuationMarks(bool enable);
    bool getAddPunctuationMarks() const;
    
    // Custom substitutions
    void addSubstitution(const QString& originalText, const QString& replacement);
    void removeSubstitution(const QString& originalText);
    QMap<QString, QString> getSubstitutions() const;
    void clearSubstitutions();

private:
    // Text processing methods
    QString capitalizeFirstLetter(const QString& text);
    QString applyAutoCorrect(const QString& text, const QString& languageCode);
    QString addPunctuation(const QString& text, const QString& languageCode);
    QString applySubstitutions(const QString& text);
    
    // Initialize language-specific rules
    void initializeLanguageRules();

private:
    // Configuration
    bool m_capitalizeFirstSentence;
    bool m_autoCorrect;
    bool m_addPunctuationMarks;
    
    // Custom substitutions
    QMap<QString, QString> m_substitutions;
    
    // Language-specific rules
    struct LanguageRules {
        QMap<QString, QString> commonCorrections;
        QRegularExpression sentenceEndPattern;
        QRegularExpression wordPattern;
    };
    
    QMap<QString, LanguageRules> m_languageRules;
}; 