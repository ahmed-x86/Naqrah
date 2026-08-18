#include "theme.h"
#include <QApplication>
#include <QCoreApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimer>
#include <QMouseEvent>
#include <QFontDatabase>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QLabel>
#include <QTextEdit>
#include <QClipboard>
#include <QRegularExpression>
#include <QMessageBox>
#include <QScrollArea>
#include <QScroller>

class RippleButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal rippleRadius READ rippleRadius WRITE setRippleRadius)
    Q_PROPERTY(qreal rippleOpacity READ rippleOpacity WRITE setRippleOpacity)

public:
    explicit RippleButton(QWidget* parent = nullptr) : QPushButton(parent) {
        setCursor(Qt::PointingHandCursor);
    }
    qreal rippleRadius() const { return m_rippleRadius; }
    void setRippleRadius(qreal r) { m_rippleRadius = r; update(); }
    qreal rippleOpacity() const { return m_rippleOpacity; }
    void setRippleOpacity(qreal o) { m_rippleOpacity = o; update(); }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QPushButton::mousePressEvent(event);
        m_ripplePos = event->position();
        m_rippleRadius = 0;
        m_rippleOpacity = 0.2;

        auto* radiusAnim = new QPropertyAnimation(this, "rippleRadius");
        radiusAnim->setDuration(350);
        radiusAnim->setStartValue(0);
        radiusAnim->setEndValue(width() * 1.5);
        radiusAnim->setEasingCurve(QEasingCurve::OutQuad);

        auto* opacityAnim = new QPropertyAnimation(this, "rippleOpacity");
        opacityAnim->setDuration(350);
        opacityAnim->setStartValue(0.2);
        opacityAnim->setEndValue(0.0);

        auto* group = new QParallelAnimationGroup(this);
        group->addAnimation(radiusAnim);
        group->addAnimation(opacityAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event);
        if (m_rippleRadius > 0 && m_rippleOpacity > 0) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(rect(), 8, 8);
            painter.setClipPath(path);
            painter.setBrush(QColor(0, 0, 0, static_cast<int>(m_rippleOpacity * 255)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(m_ripplePos, m_rippleRadius, m_rippleRadius);
        }
    }

private:
    QPointF m_ripplePos;
    qreal m_rippleRadius = 0;
    qreal m_rippleOpacity = 0;
};

class GlowWindow : public QWidget {
    Q_OBJECT

public:
    explicit GlowWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("نقرة - للتشكيل");
        setMinimumSize(320, 500); 
        setLayoutDirection(Qt::RightToLeft);

        m_currentGlowPos = QPointF(width() / 2.0, height() / 2.0);
        m_targetGlowPos  = m_currentGlowPos;

        buildMainLayout();
        buildSidebar();

        m_glowTimer = new QTimer(this);
        m_glowTimer->setInterval(16);
        connect(m_glowTimer, &QTimer::timeout, this, [this]() {
            m_currentGlowPos += (m_targetGlowPos - m_currentGlowPos) * 0.08;
            update();
        });
        m_glowTimer->start();

        qApp->installEventFilter(this);
        
        m_theme = ThemeMode::Mocha;
        applyTheme(m_theme);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(m_colors.Base));

        QRadialGradient glow(m_currentGlowPos, 300.0);
        QColor glowColor(m_colors.Accent3);
        
        int alpha = (m_colors.isDark) ? 30 : 80;
        glowColor.setAlpha(alpha);
        glow.setColorAt(0.0, glowColor);
        glowColor.setAlpha(0);
        glow.setColorAt(1.0, glowColor);

        painter.setBrush(QBrush(glow));
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect());
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate) {
            QPointF pos;
            if (event->type() == QEvent::MouseMove) pos = static_cast<QMouseEvent*>(event)->globalPosition();
            else pos = static_cast<QTouchEvent*>(event)->points().first().globalPosition();
            m_targetGlowPos = mapFromGlobal(pos.toPoint());
        }
        if (event->type() == QEvent::MouseButtonPress && m_isSidebarOpen) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->pos().x() < width() - 250) { 
                toggleSidebar();
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        
        if (m_sidebar) {
            if (m_isSidebarOpen) {
                m_sidebar->setGeometry(width() - 250, 0, 250, height());
            } else {
                m_sidebar->setGeometry(width(), 0, 250, height());
            }
        }

        if (width() < height() && width() < 700) {
            m_dynamicLayout->setDirection(QBoxLayout::TopToBottom);
            m_rightPanel->setMinimumHeight(200);
            m_centerPanel->setMinimumHeight(450);
            m_leftPanel->setMinimumHeight(200);
            m_dynamicLayout->setStretchFactor(m_rightPanel, 0);
            m_dynamicLayout->setStretchFactor(m_centerPanel, 0);
            m_dynamicLayout->setStretchFactor(m_leftPanel, 0);
        } else {
            m_dynamicLayout->setDirection(QBoxLayout::RightToLeft);
            m_rightPanel->setMinimumHeight(0);
            m_centerPanel->setMinimumHeight(0);
            m_leftPanel->setMinimumHeight(0);
            m_dynamicLayout->setStretchFactor(m_rightPanel, 2);
            m_dynamicLayout->setStretchFactor(m_centerPanel, 3);
            m_dynamicLayout->setStretchFactor(m_leftPanel, 2);
        }
    }

private:
    void buildMainLayout() {
        m_mainContent = new QWidget(this);
        auto* root = new QVBoxLayout(m_mainContent);
        root->setContentsMargins(15, 10, 15, 10);
        root->setSpacing(12);

        auto* windowLayout = new QVBoxLayout(this);
        windowLayout->setContentsMargins(0, 0, 0, 0);
        windowLayout->addWidget(m_mainContent);

        m_topBar = new QWidget(this);
        auto* topRow = new QHBoxLayout(m_topBar);
        topRow->setContentsMargins(0, 0, 0, 0);

        m_hamburgerBtn = new QPushButton("☰");
        m_hamburgerBtn->setObjectName("hamburgerBtn");
        m_hamburgerBtn->setCursor(Qt::PointingHandCursor);
        m_hamburgerBtn->setFont(QFont("Segoe UI", 18, QFont::Bold));
        connect(m_hamburgerBtn, &QPushButton::clicked, this, &GlowWindow::toggleSidebar);

        QLabel* titleLabel = new QLabel("نقرة للتشكيل");
        titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

        topRow->addWidget(m_hamburgerBtn);
        topRow->addWidget(titleLabel);
        topRow->addStretch();
        root->addWidget(m_topBar);

        m_stackedWidget = new QStackedWidget(this);

        m_mainMenuPage = new QWidget(this);
        auto* mainPageLayout = new QVBoxLayout(m_mainMenuPage);
        mainPageLayout->setContentsMargins(0, 0, 0, 0);
        
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("background: transparent;");
        
        QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);

        QWidget* containerWidget = new QWidget();
        containerWidget->setStyleSheet("background: transparent;");
        m_dynamicLayout = new QBoxLayout(QBoxLayout::RightToLeft, containerWidget);
        m_dynamicLayout->setContentsMargins(0, 10, 0, 10);
        m_dynamicLayout->setSpacing(20);

        m_rightPanel = new QWidget();
        auto* rightCol = new QVBoxLayout(m_rightPanel);
        rightCol->setContentsMargins(0, 0, 0, 0);
        QLabel* lblInput = new QLabel("الصق نص هنا:");
        lblInput->setStyleSheet("font-weight: bold;");
        m_inputText = new QTextEdit();
        m_inputText->setPlaceholderText("أدخل الكلمات التي تود تشكيلها هنا...");
        m_btnStartTashkeel = new RippleButton();
        m_btnStartTashkeel->setText("ابدأ التشكيل");
        m_btnStartTashkeel->setObjectName("btnPrimary");
        m_btnStartTashkeel->setFixedHeight(55); 
        connect(m_btnStartTashkeel, &QPushButton::clicked, this, &GlowWindow::startTashkeel);
        rightCol->addWidget(lblInput);
        rightCol->addWidget(m_inputText);
        rightCol->addWidget(m_btnStartTashkeel);

        m_centerPanel = new QWidget();
        auto* centerCol = new QVBoxLayout(m_centerPanel);
        centerCol->setContentsMargins(0, 0, 0, 0);
        m_lblCurrentWord = new QLabel("الكلمة");
        m_lblCurrentWord->setAlignment(Qt::AlignCenter);
        m_lblCurrentWord->setMinimumHeight(120);
        m_lblCurrentWord->setWordWrap(true);
        m_lblCurrentWord->setStyleSheet("font-size: 45px; font-weight: bold; background: transparent;");
        
        QGridLayout* gridMarks = new QGridLayout();
        gridMarks->setSpacing(10);
        
        struct DiacriticMark { QString name; QString unicode; };
        QList<DiacriticMark> marks = {
            {"فتحة", "َ"}, {"ضمة", "ُ"}, {"كسرة", "ِ"},
            {"سكون", "ْ"}, {"شدة", "ّ"},
            {"تنوين فتح", "ً"}, {"تنوين ضم", "ٌ"}, {"تنوين كسر", "ٍ"}
        };
        
        int row = 0, col = 0;
        for (const auto& mark : marks) {
            RippleButton* btnMark = new RippleButton();
            btnMark->setText(mark.name + " (" + mark.unicode + ")");
            btnMark->setObjectName("btnMark");
            btnMark->setFixedHeight(55); 
            connect(btnMark, &QPushButton::clicked, [this, mark]() { applyMark(mark.unicode); });
            gridMarks->addWidget(btnMark, row, col);
            col++;
            if (col > 2) { col = 0; row++; }
        }

        m_btnNextChar = new RippleButton();
        m_btnNextChar->setText("الحرف التالي");
        m_btnNextChar->setObjectName("btnNext");
        m_btnNextChar->setFixedHeight(60);
        connect(m_btnNextChar, &QPushButton::clicked, this, &GlowWindow::advanceChar);

        centerCol->addWidget(m_lblCurrentWord);
        centerCol->addLayout(gridMarks);
        centerCol->addStretch();
        centerCol->addWidget(m_btnNextChar);

        m_leftPanel = new QWidget();
        auto* leftCol = new QVBoxLayout(m_leftPanel);
        leftCol->setContentsMargins(0, 0, 0, 0);
        auto* leftTopRow = new QHBoxLayout();
        QLabel* lblOutput = new QLabel("النص المشكّل:");
        lblOutput->setStyleSheet("font-weight: bold;");
        m_btnCopy = new RippleButton();
        m_btnCopy->setText("نسخ");
        m_btnCopy->setObjectName("btnCopy");
        m_btnCopy->setFixedHeight(45);
        connect(m_btnCopy, &QPushButton::clicked, this, [this]() {
            QGuiApplication::clipboard()->setText(m_outputText->toPlainText());
            QMessageBox::information(this, "نجاح", "تم نسخ النص بنجاح.");
        });
        leftTopRow->addWidget(lblOutput);
        leftTopRow->addStretch();
        leftTopRow->addWidget(m_btnCopy);

        m_outputText = new QTextEdit();
        m_outputText->setReadOnly(true);
        leftCol->addLayout(leftTopRow);
        leftCol->addWidget(m_outputText);

        m_dynamicLayout->addWidget(m_rightPanel);
        m_dynamicLayout->addWidget(m_centerPanel);
        m_dynamicLayout->addWidget(m_leftPanel);

        scrollArea->setWidget(containerWidget);
        mainPageLayout->addWidget(scrollArea);

        m_stackedWidget->addWidget(m_mainMenuPage);

        m_settingsPage = new QWidget(this);
        auto* settingsLayout = new QVBoxLayout(m_settingsPage);
        settingsLayout->setContentsMargins(0, 0, 0, 0);
        settingsLayout->setSpacing(12);

        m_btnToggleTheme = new RippleButton(); 
        m_btnToggleTheme->setObjectName("btnSetting"); 
        m_btnToggleTheme->setFixedHeight(55);

        connect(m_btnToggleTheme, &QPushButton::clicked, this, [this]() {
            ThemeSelectionDialog themeDlg(this);
            themeDlg.setStyleSheet(this->styleSheet());
            if (themeDlg.exec() == QDialog::Accepted) {
                int selection = themeDlg.selectedTheme;
                if (selection >= 0 && selection <= 4) { 
                    m_isCustomTheme = false;
                    applyTheme(static_cast<ThemeMode>(selection));
                } else if (selection == 5) {  
                    CustomThemeDialog customDlg(m_colors, this);
                    customDlg.setStyleSheet(this->styleSheet());
                    if (customDlg.exec() == QDialog::Accepted) {
                        applyCustomTheme(customDlg.m_colors);
                    }
                }
            }
        });

        settingsLayout->addWidget(m_btnToggleTheme);
        settingsLayout->addStretch();
        m_stackedWidget->addWidget(m_settingsPage);

        root->addWidget(m_stackedWidget);
    }

    void buildSidebar() {
        m_sidebar = new QWidget(this);
        m_sidebar->setObjectName("sidebar");
        m_sidebar->setGeometry(width(), 0, 250, height()); 

        auto* layout = new QVBoxLayout(m_sidebar);
        layout->setContentsMargins(10, 60, 10, 20);
        layout->setSpacing(8);

        m_btnNavMain = new RippleButton();
        m_btnNavMain->setText("🏠  التشكيل");
        m_btnNavMain->setObjectName("btnSidebarItem");

        m_btnNavSettings = new RippleButton();
        m_btnNavSettings->setText("⚙️  الإعدادات");
        m_btnNavSettings->setObjectName("btnSidebarItem");
        
        layout->addWidget(m_btnNavMain);
        layout->addWidget(m_btnNavSettings);
        layout->addStretch();

        connect(m_btnNavMain, &QPushButton::clicked, this, [this]{
            m_stackedWidget->setCurrentIndex(0);
            toggleSidebar();
        });
        connect(m_btnNavSettings, &QPushButton::clicked, this, [this]{
            m_stackedWidget->setCurrentIndex(1);
            toggleSidebar();
        });
    }

    void toggleSidebar() {
        m_isSidebarOpen = !m_isSidebarOpen;
        if (m_isSidebarOpen) { m_sidebar->raise(); }

        auto* anim = new QPropertyAnimation(m_sidebar, "geometry");
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::OutQuint);

        if (m_isSidebarOpen) {
            anim->setStartValue(QRect(width(), 0, 250, height()));
            anim->setEndValue(QRect(width() - 250, 0, 250, height()));
        } else {
            anim->setStartValue(QRect(width() - 250, 0, 250, height()));
            anim->setEndValue(QRect(width(), 0, 250, height()));
        }
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startTashkeel() {
        QString text = m_inputText->toPlainText().trimmed();
        if (text.isEmpty()) return;

        m_words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        m_processedWords.clear();
        m_currentWordIdx = 0;
        m_currentCharIdx = 0;
        m_currentWordProcessed = "";
        m_outputText->clear();
        updateCenterView();
    }

    void applyMark(const QString& mark) {
        if (m_currentWordIdx >= m_words.size()) return;
        QString currentWord = m_words[m_currentWordIdx];
        
        if (mark == "ّ") {
            m_currentWordProcessed += currentWord[m_currentCharIdx] + mark;
            updateCenterView(true);
        } else {
            if (m_isWaitingForMarkAfterShadda) {
                m_currentWordProcessed += mark;
            } else {
                m_currentWordProcessed += currentWord[m_currentCharIdx] + mark;
            }
            advanceCharLogic();
        }
    }

    void advanceChar() {
        if (m_currentWordIdx >= m_words.size()) return;
        QString currentWord = m_words[m_currentWordIdx];
        if (!m_isWaitingForMarkAfterShadda) {
            m_currentWordProcessed += currentWord[m_currentCharIdx];
        }
        advanceCharLogic();
    }

    void advanceCharLogic() {
        m_currentCharIdx++;
        m_isWaitingForMarkAfterShadda = false;
        
        QString currentWord = m_words[m_currentWordIdx];
        if (m_currentCharIdx >= currentWord.length()) {
            m_processedWords.append(m_currentWordProcessed);
            m_outputText->setPlainText(m_processedWords.join(" "));
            
            m_currentWordIdx++;
            m_currentCharIdx = 0;
            m_currentWordProcessed = "";
            
            if (m_currentWordIdx >= m_words.size()) {
                m_lblCurrentWord->setText("انتهى النص!");
                m_btnNextChar->setText("إنهاء");
                return;
            }
        }
        updateCenterView();
    }

    void updateCenterView(bool waitingAfterShadda = false) {
        if (m_currentWordIdx >= m_words.size()) return;
        
        m_isWaitingForMarkAfterShadda = waitingAfterShadda;
        QString currentWord = m_words[m_currentWordIdx];
        
        QString html = "<div style='direction: rtl;'>";
        for(int i = 0; i < m_currentCharIdx; i++) html += currentWord[i];
        
        html += QString("<span style='color: %1; text-decoration: underline;'>%2</span>")
                .arg(m_colors.Danger).arg(currentWord[m_currentCharIdx]);
                
        if (waitingAfterShadda) html += "ّ";
                
        for(int i = m_currentCharIdx + 1; i < currentWord.length(); i++) html += currentWord[i];
        
        html += "</div>";
        m_lblCurrentWord->setText(html);

        if (m_currentCharIdx == currentWord.length() - 1) m_btnNextChar->setText("الكلمة التالية");
        else m_btnNextChar->setText("الحرف التالي");
    }

    void applyTheme(ThemeMode mode) {
        m_theme = mode;
        m_isCustomTheme = false;
        
        switch(m_theme) {
            case ThemeMode::Latte:          m_colors = LatteTheme; break;
            case ThemeMode::ShamelaClassic: m_colors = ShamelaClassicTheme; break;
            case ThemeMode::Nord:           m_colors = NordTheme; break;
            case ThemeMode::Gruvbox:        m_colors = GruvboxTheme; break;
            case ThemeMode::Mocha:
            default:                        m_colors = MochaTheme; break; 
        }
        applyCurrentColors();
    }

    void applyCustomTheme(const ThemeColors& customColors) {
        m_colors = customColors;
        m_isCustomTheme = true;
        applyCurrentColors();
    }

    void applyCurrentColors() {
        QString themeName;
        if (m_isCustomTheme) themeName = "🎨 مخصص";
        else {
            switch(m_theme) {
                case ThemeMode::Latte:          themeName = "☀️ Latte"; break;
                case ThemeMode::ShamelaClassic: themeName = "📜 Shamela Classic"; break;
                case ThemeMode::Nord:           themeName = "❄️ Nord"; break;
                case ThemeMode::Gruvbox:        themeName = "📦 Gruvbox"; break;
                case ThemeMode::Mocha:
                default:                        themeName = "☕ Mocha"; break;
            }
        }
        m_btnToggleTheme->setText("المظهر: " + themeName);

        QString qss = QStringLiteral(R"(
            QWidget { color: %1; }
            QDialog { background-color: %2; }
            QPushButton { font-size: 15px; font-weight: bold; border: none; border-radius: 8px; padding: 12px; }
            
            QTextEdit { 
                background-color: %3; 
                color: %1; 
                border: 1px solid %4; 
                border-radius: 8px; 
                padding: 10px; 
                font-size: 16px;
            }

            QPushButton#hamburgerBtn { background: transparent; color: %1; padding: 4px; border-radius: 4px; }
            QPushButton#hamburgerBtn:hover { background: %4; }

            QWidget#sidebar { background-color: %3; border-left: 1px solid %4; }
            QPushButton#btnSidebarItem { background: transparent; color: %1; text-align: right; padding: 12px 20px; }
            QPushButton#btnSidebarItem:hover { background: %4; }
            
            QPushButton#btnPrimary { background: %7; color: %2; }
            QPushButton#btnPrimary:hover { background: %8; color: %2; }

            QPushButton#btnMark { background: %3; color: %6; font-size: 18px; }
            QPushButton#btnMark:hover { background: %6; color: %2; }
            
            QPushButton#btnNext { background: %5; color: %2; font-size: 18px; }
            QPushButton#btnNext:hover { background: %1; color: %2; }

            QPushButton#btnCopy { background: transparent; border: 1px solid %6; color: %6; }
            QPushButton#btnCopy:hover { background: %6; color: %2; }

            QPushButton#btnSetting { background: %3; color: %1; }
            QPushButton#btnSetting:hover { background: %4; }
        )")
        .arg(m_colors.Text, m_colors.Base, m_colors.Surface, m_colors.Hover, m_colors.Accent1, m_colors.Accent2, m_colors.Accent3, m_colors.Accent4, m_colors.Danger);

        this->setStyleSheet(qss);
        if (m_words.size() > 0) updateCenterView(m_isWaitingForMarkAfterShadda);
        update();
    }

    QPointF m_currentGlowPos, m_targetGlowPos;
    QTimer* m_glowTimer{};
    ThemeMode m_theme; 
    ThemeColors m_colors;
    bool m_isCustomTheme = false;
    
    QStringList m_words;
    QStringList m_processedWords;
    int m_currentWordIdx = 0;
    int m_currentCharIdx = 0;
    QString m_currentWordProcessed = "";
    bool m_isWaitingForMarkAfterShadda = false; 

    bool m_isSidebarOpen = false;
    QWidget *m_sidebar{};
    QWidget *m_mainContent{};
    QWidget *m_topBar{};
    QStackedWidget *m_stackedWidget{};
    QWidget *m_mainMenuPage{}, *m_settingsPage{};
    QPushButton *m_hamburgerBtn{};
    RippleButton *m_btnNavMain{}, *m_btnNavSettings{};
    RippleButton *m_btnToggleTheme{};

    QBoxLayout* m_dynamicLayout{};
    QWidget* m_rightPanel{};
    QWidget* m_centerPanel{};
    QWidget* m_leftPanel{};

    QTextEdit *m_inputText{}, *m_outputText{};
    QLabel *m_lblCurrentWord{};
    RippleButton *m_btnStartTashkeel{}, *m_btnNextChar{}, *m_btnCopy{};
};

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
    qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    
    QApplication app(argc, argv);
    
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(12);
    font.setFamily("Segoe UI, Tahoma, Noto Sans Arabic, sans-serif");
    app.setFont(font);

    GlowWindow window;
    window.show();
    return app.exec();
}
#include "main.moc"