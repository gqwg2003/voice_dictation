#pragma once

#include <QWidget>
#include <QColor>
#include <vector>

class AudioVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit AudioVisualizer(QWidget *parent = nullptr);
    ~AudioVisualizer();
    
    void updateLevels(const std::vector<float>& levels);
    void clear();
    
    // Appearance settings
    void setBarColor(const QColor& color);
    void setBarCount(int count);
    void setBarSpacing(int spacing);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Data
    std::vector<float> m_levels;
    
    // Appearance
    QColor m_barColor;
    int m_barCount;
    int m_barSpacing;
    
    // Constants
    static const int DEFAULT_BAR_COUNT = 32;
    static const int DEFAULT_BAR_SPACING = 2;
}; 