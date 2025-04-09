#include "AudioVisualizer.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>

AudioVisualizer::AudioVisualizer(QWidget *parent)
    : QWidget(parent),
      m_barColor(64, 196, 255),
      m_barCount(DEFAULT_BAR_COUNT),
      m_barSpacing(DEFAULT_BAR_SPACING)
{
    // Initialize with empty levels
    m_levels.resize(m_barCount, 0.0f);
    
    // Set widget properties
    setMinimumHeight(40);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    // Set background to be transparent
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(true);
    
    // Set background color
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 30));
    setPalette(pal);
}

AudioVisualizer::~AudioVisualizer()
{
    // Nothing to do here
}

void AudioVisualizer::updateLevels(const std::vector<float>& levels)
{
    // Make sure we have data
    if (levels.empty()) {
        return;
    }
    
    // Resample input levels to match our bar count
    m_levels.resize(m_barCount);
    
    if (levels.size() == m_barCount) {
        // Direct copy if sizes match
        std::copy(levels.begin(), levels.end(), m_levels.begin());
    } else {
        // Interpolate to fit
        float ratio = static_cast<float>(levels.size()) / m_barCount;
        for (int i = 0; i < m_barCount; ++i) {
            float pos = i * ratio;
            int idx = static_cast<int>(pos);
            float frac = pos - idx;
            
            if (idx + 1 < levels.size()) {
                m_levels[i] = levels[idx] * (1.0f - frac) + levels[idx + 1] * frac;
            } else {
                m_levels[i] = levels.back();
            }
        }
    }
    
    // Trigger repaint
    update();
}

void AudioVisualizer::clear()
{
    // Reset all levels to zero
    std::fill(m_levels.begin(), m_levels.end(), 0.0f);
    update();
}

void AudioVisualizer::setBarColor(const QColor& color)
{
    m_barColor = color;
    update();
}

void AudioVisualizer::setBarCount(int count)
{
    if (count > 0) {
        m_barCount = count;
        m_levels.resize(count, 0.0f);
        update();
    }
}

void AudioVisualizer::setBarSpacing(int spacing)
{
    if (spacing >= 0) {
        m_barSpacing = spacing;
        update();
    }
}

void AudioVisualizer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Get widget dimensions
    int width = this->width();
    int height = this->height();
    
    // Calculate bar width
    float totalBarWidth = static_cast<float>(width) - (m_barCount - 1) * m_barSpacing;
    float barWidth = totalBarWidth / m_barCount;
    
    // Draw each bar
    for (int i = 0; i < m_barCount; ++i) {
        // Calculate bar height
        float level = std::min(std::max(m_levels[i], 0.0f), 1.0f);
        int barHeight = static_cast<int>(height * level);
        
        // Calculate bar position
        int x = static_cast<int>(i * (barWidth + m_barSpacing));
        int y = height - barHeight;
        
        // Create gradient for bar color
        QLinearGradient gradient(0, height, 0, 0);
        gradient.setColorAt(0, m_barColor);
        gradient.setColorAt(1, m_barColor.lighter(150));
        
        // Draw the bar
        painter.fillRect(x, y, static_cast<int>(barWidth), barHeight, gradient);
    }
}

void AudioVisualizer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
} 