#include "RecognitionService.h"
#include <QNetworkAccessManager>

RecognitionService::RecognitionService(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_languageCode("en-US")
    , m_apiKey("")
    , m_useSharedApiKey(false)
    , m_isReady(false)
{
}

RecognitionService::~RecognitionService()
{
    // NetworkAccessManager is deleted automatically by Qt's parent-child mechanism
} 