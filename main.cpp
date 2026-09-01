#include <QApplication>
#include <QCoreApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QClipboard>
#include <QRegularExpression>
#include <QMessageBox>
#include <QResizeEvent>
#include <QFontDatabase>
#include <QScrollArea>
#include <QScroller>
#include <QScrollerProperties>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <cmath>

enum class InputMode { Unknown, Touch, Desktop };

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("Naqrah - نقرة");
        setMinimumSize(900, 600);
        setLayoutDirection(Qt::RightToLeft);

        buildUI();
        applyShadcnTheme();
        
        // Parent scroll area: grab kinetic scrolling once, unconditionally.
        if (m_scrollArea) {
            QScroller::grabGesture(m_scrollArea->viewport(), QScroller::TouchGesture);
            QScrollerProperties sp = QScroller::scroller(m_scrollArea->viewport())->scrollerProperties();
            sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0015);
            sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
            sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                               QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
            QScroller::scroller(m_scrollArea->viewport())->setScrollerProperties(sp);
        }

        // Child QTextEdit viewports get touch events routed through us
        if (m_inputText)  m_inputText->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        if (m_outputText) m_outputText->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        if (m_scrollArea) m_scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);

        qApp->installEventFilter(this);
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (width() < 850) {
            m_dynamicLayout->setDirection(QBoxLayout::TopToBottom);
        } else {
            m_dynamicLayout->setDirection(QBoxLayout::LeftToRight);
        }
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        const QEvent::Type type = event->type();

        // ---------- Input-mode detection ----------
        if (type == QEvent::TouchBegin || type == QEvent::TouchUpdate) {
            if (m_currentInputMode != InputMode::Touch) {
                m_currentInputMode = InputMode::Touch;
                updateUIForInputMode(m_currentInputMode);
            }
        } else if (type == QEvent::MouseMove || type == QEvent::MouseButtonPress ||
                   type == QEvent::MouseButtonDblClick) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->source() != Qt::MouseEventSynthesizedBySystem &&
                mouseEvent->source() != Qt::MouseEventSynthesizedByApplication) {
                if (m_currentInputMode != InputMode::Desktop) {
                    m_currentInputMode = InputMode::Desktop;
                    updateUIForInputMode(m_currentInputMode);
                }
            }
        } else if (type == QEvent::KeyPress || type == QEvent::KeyRelease) {
            if (m_currentInputMode != InputMode::Desktop) {
                m_currentInputMode = InputMode::Desktop;
                updateUIForInputMode(m_currentInputMode);
            }
        }

        // ---------- Nested-scroll arbitration for the QTextEdit viewports ----------
        const bool isInputVp  = m_inputText  && obj == m_inputText->viewport();
        const bool isOutputVp = m_outputText && obj == m_outputText->viewport();

        if ((isInputVp || isOutputVp) &&
            (type == QEvent::TouchBegin || type == QEvent::TouchUpdate || type == QEvent::TouchEnd)) {
            
            QTextEdit* edit = isInputVp ? m_inputText : m_outputText;
            auto* te = static_cast<QTouchEvent*>(event);
            
            if (te->points().isEmpty()) return QWidget::eventFilter(obj, event);
            const QTouchEvent::TouchPoint& pt = te->points().first();
            
            if (type == QEvent::TouchBegin) {
                m_touchDecided   = false;
                m_routeToParent  = false;
                m_activeTouchSrc = obj;
                m_touchStartPos  = pt.position();
                return QWidget::eventFilter(obj, event);
            }
            
            if (type == QEvent::TouchUpdate && m_activeTouchSrc == obj) {
                if (!m_touchDecided) {
                    const QPointF delta = pt.position() - m_touchStartPos;
                    if (std::abs(delta.y()) < kSwipeThreshold && std::abs(delta.x()) < kSwipeThreshold) {
                        return QWidget::eventFilter(obj, event);
                    }
                    
                    m_touchDecided = true;
                    const bool focused        = edit->hasFocus();
                    const bool needsInternal  = edit->verticalScrollBar() && edit->verticalScrollBar()->maximum() > 0;
                    const QScrollBar* vbar    = edit->verticalScrollBar();
                    const bool atTop          = !vbar || vbar->value() <= vbar->minimum();
                    const bool atBottom       = !vbar || vbar->value() >= vbar->maximum();
                    const bool swipingDown    = delta.y() > 0;
                    const bool wouldOverscroll = (swipingDown && atTop) || (!swipingDown && atBottom);
                    
                    m_routeToParent = !(focused && needsInternal && !wouldOverscroll);
                }
                
                if (m_routeToParent) {
                    QCoreApplication::sendEvent(m_scrollArea->viewport(), event);
                    return true; 
                }
                return QWidget::eventFilter(obj, event);
            }
            
            if (type == QEvent::TouchEnd && m_activeTouchSrc == obj) {
                if (m_routeToParent) {
                    QCoreApplication::sendEvent(m_scrollArea->viewport(), event);
                    m_activeTouchSrc = nullptr;
                    m_touchDecided   = false;
                    return true;
                }
                m_activeTouchSrc = nullptr;
                m_touchDecided   = false;
                return QWidget::eventFilter(obj, event);
            }
        }
        
        return QWidget::eventFilter(obj, event);
    }

private:
    void updateUIForInputMode(InputMode mode) {
        if (mode == InputMode::Touch) {
            if (m_gridMarks) m_gridMarks->setSpacing(16);
            for (auto* btn : m_markButtons) {
                if (btn) btn->setMinimumHeight(64);
            }
            if (m_btnStartTashkeel) m_btnStartTashkeel->setMinimumHeight(56);
            if (m_btnNextChar)      m_btnNextChar->setMinimumHeight(56);
            if (m_btnCopy)          m_btnCopy->setMinimumHeight(56);
        } else if (mode == InputMode::Desktop) {
            if (m_gridMarks) m_gridMarks->setSpacing(8);
            for (auto* btn : m_markButtons) {
                if (btn) btn->setMinimumHeight(44);
            }
            if (m_btnStartTashkeel) m_btnStartTashkeel->setMinimumHeight(44);
            if (m_btnNextChar)      m_btnNextChar->setMinimumHeight(44);
            if (m_btnCopy)          m_btnCopy->setMinimumHeight(44);
        }
    }

    void applyShadcnTheme() {
        QString qss = R"(
            QWidget {
                background-color: #09090b;
                color: #f4f4f5;
                font-family: "Segoe UI", Tahoma, "Noto Sans Arabic", sans-serif;
            }
            QWidget#header {
                background-color: #09090b;
                border-bottom: 1px solid #27272a;
            }
            QLabel#headerTitle {
                font-size: 18px;
                font-weight: bold;
                color: #f4f4f5;
                letter-spacing: 0.5px;
            }
            QWidget#card {
                background-color: #121215;
                border: 1px solid #27272a;
                border-radius: 12px;
            }
            QLabel#cardTitle {
                font-size: 15px;
                font-weight: bold;
                color: #f4f4f5;
                background-color: transparent;
            }
            QLabel#cardSubtitle {
                font-size: 12px;
                color: #a1a1aa;
                background-color: transparent;
            }
            QTextEdit {
                background-color: #09090b;
                color: #f4f4f5;
                border: 1px solid #27272a;
                border-radius: 8px;
                padding: 12px;
                font-size: 16px;
                selection-background-color: #27272a;
            }
            QTextEdit:focus {
                border: 1px solid #a1a1aa;
            }
            QPushButton#btnSecondary {
                background-color: #18181b;
                color: #f4f4f5;
                border: 1px solid #27272a;
                border-radius: 6px;
                padding: 10px 16px;
                font-weight: bold;
                font-size: 14px;
            }
            QPushButton#btnSecondary:hover {
                background-color: #27272a;
            }
            QPushButton#btnSecondary:pressed {
                background-color: #3f3f46;
            }
            QPushButton#btnMark {
                background-color: #18181b;
                color: #f4f4f5;
                border: 1px solid #27272a;
                border-radius: 8px;
                padding: 14px 10px;
                font-size: 15px;
                font-weight: bold;
            }
            QPushButton#btnMark:hover {
                background-color: #27272a;
            }
            QPushButton#btnMark:pressed {
                background-color: #3f3f46;
            }
            QLabel#currentWordLabel {
                font-size: 42px;
                font-weight: bold;
                color: #f4f4f5;
                background: transparent;
            }
            QScrollBar:vertical {
                width: 0px; 
                background: transparent;
            }
        )";
        setStyleSheet(qss);
    }

    void buildUI() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Header
        QWidget* header = new QWidget();
        header->setObjectName("header");
        header->setFixedHeight(65);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(24, 0, 24, 0);
        
        QLabel* logoBlock = new QLabel();
        logoBlock->setFixedSize(22, 22);
        logoBlock->setStyleSheet("background-color: #f4f4f5; border-radius: 4px;");
        
        QLabel* title = new QLabel("نقرة");
        title->setObjectName("headerTitle");
        
        headerLayout->addWidget(logoBlock);
        headerLayout->addSpacing(10);
        headerLayout->addWidget(title);
        headerLayout->addStretch();
        
        mainLayout->addWidget(header);

        // Scroll Area setup
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setFrameShape(QFrame::NoFrame);
        m_scrollArea->setStyleSheet("background: transparent;");

        QWidget* contentWidget = new QWidget();
        contentWidget->setStyleSheet("background: transparent;");
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(24, 24, 24, 24);

        m_dynamicLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        m_dynamicLayout->setSpacing(24);

        // 1. Input Card
        QWidget* inputCard = createCard("النص المدخل", "أدخل النص الذي تود تشكيله هنا");
        auto* inputLayout = static_cast<QVBoxLayout*>(inputCard->layout());
        
        m_inputText = new QTextEdit(inputCard);
        m_inputText->setPlaceholderText("اكتب أو الصق النص هنا...");
        m_inputText->setMinimumHeight(150);
        m_inputText->setMaximumHeight(220);
        m_inputText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_inputText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
        m_inputText->viewport()->installEventFilter(this);
        m_inputText->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        
        m_btnStartTashkeel = new QPushButton("ابدأ التشكيل", inputCard);
        m_btnStartTashkeel->setObjectName("btnSecondary"); 
        m_btnStartTashkeel->setCursor(Qt::PointingHandCursor);
        m_btnStartTashkeel->setMinimumHeight(44);
        m_btnStartTashkeel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        inputLayout->addWidget(m_inputText);
        inputLayout->addSpacing(12);
        inputLayout->addWidget(m_btnStartTashkeel);
        connect(m_btnStartTashkeel, &QPushButton::clicked, this, &MainWindow::startTashkeel);

        // 2. Tashkeel Card
        QWidget* tashkeelCard = createCard("لوحة التشكيل", "اختر الحركات المناسبة للحرف المظلل");
        auto* tashkeelLayout = static_cast<QVBoxLayout*>(tashkeelCard->layout());
        
        m_lblCurrentWord = new QLabel("الكلمة");
        m_lblCurrentWord->setObjectName("currentWordLabel");
        m_lblCurrentWord->setAlignment(Qt::AlignCenter);
        m_lblCurrentWord->setMinimumHeight(120);
        m_lblCurrentWord->setWordWrap(true);

        m_gridMarks = new QGridLayout();
        m_gridMarks->setSpacing(10); 

        struct DiacriticMark { QString name; QString unicode; };
        QList<DiacriticMark> marks = {
            {"فتحة", "َ"}, {"ضمة", "ُ"}, {"كسرة", "ِ"},
            {"سكون", "ْ"}, {"شدة", "ّ"},
            {"تنوين فتح", "ً"}, {"تنوين ضم", "ٌ"}, {"تنوين كسر", "ٍ"}
        };

        int row = 0, col = 0;
        for (const auto& mark : marks) {
            QPushButton* btnMark = new QPushButton(mark.name + " (ـ" + mark.unicode + ")");
            btnMark->setObjectName("btnMark");
            btnMark->setCursor(Qt::PointingHandCursor);
            btnMark->setMinimumHeight(44); 
            btnMark->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            connect(btnMark, &QPushButton::clicked, [this, mark]() { applyMark(mark.unicode); });
            m_gridMarks->addWidget(btnMark, row, col);
            m_markButtons.append(btnMark);
            col++;
            if (col > 1) { col = 0; row++; }
        }

        m_btnNextChar = new QPushButton("الحرف التالي");
        m_btnNextChar->setObjectName("btnSecondary");
        m_btnNextChar->setCursor(Qt::PointingHandCursor);
        m_btnNextChar->setMinimumHeight(44);
        m_btnNextChar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(m_btnNextChar, &QPushButton::clicked, this, &MainWindow::advanceChar);

        tashkeelLayout->addWidget(m_lblCurrentWord);
        tashkeelLayout->addSpacing(10);
        tashkeelLayout->addLayout(m_gridMarks);
        tashkeelLayout->addSpacing(20);
        tashkeelLayout->addWidget(m_btnNextChar);

        // 3. Output Card
        QWidget* outputCard = createCard("النتيجة", "النص النهائي المشكّل");
        auto* outputLayout = static_cast<QVBoxLayout*>(outputCard->layout());
        
        m_outputText = new QTextEdit();
        m_outputText->setReadOnly(true);
        m_outputText->setMinimumHeight(150);
        m_outputText->setMaximumHeight(220); 
        m_outputText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_outputText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
        m_outputText->viewport()->installEventFilter(this);
        m_outputText->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        
        m_btnCopy = new QPushButton("نسخ النص");
        m_btnCopy->setObjectName("btnSecondary");
        m_btnCopy->setCursor(Qt::PointingHandCursor);
        m_btnCopy->setMinimumHeight(44);
        m_btnCopy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        connect(m_btnCopy, &QPushButton::clicked, this, [this]() {
            QGuiApplication::clipboard()->setText(m_outputText->toPlainText());
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(this->styleSheet() + "QLabel{color:#f4f4f5; background:transparent;} QMessageBox{background-color:#121215; border:1px solid #27272a;} QPushButton{background-color:#f4f4f5; color:#09090b; padding:8px 16px; border-radius:6px; font-weight:bold;}");
            msgBox.setText("تم نسخ النص بنجاح!");
            msgBox.exec();
        });

        outputLayout->addWidget(m_outputText);
        outputLayout->addSpacing(12);
        outputLayout->addWidget(m_btnCopy);

        m_dynamicLayout->addWidget(inputCard, 1);
        m_dynamicLayout->addWidget(tashkeelCard, 1);
        m_dynamicLayout->addWidget(outputCard, 1);

        contentLayout->addLayout(m_dynamicLayout);
        
        m_scrollArea->setWidget(contentWidget);
        mainLayout->addWidget(m_scrollArea);
    }

    QWidget* createCard(const QString& titleText, const QString& subtitleText) {
        QWidget* card = new QWidget();
        card->setObjectName("card");
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(4);

        QLabel* title = new QLabel(titleText);
        title->setObjectName("cardTitle");
        
        QLabel* subtitle = new QLabel(subtitleText);
        subtitle->setObjectName("cardSubtitle");

        layout->addWidget(title);
        layout->addWidget(subtitle);
        layout->addSpacing(12);
        
        return card;
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
        m_btnNextChar->setText("الحرف التالي");
        updateCenterView();
    }

    void applyMark(const QString& mark) {
        if (m_currentWordIdx >= m_words.size()) return;
        QString currentWord = m_words[m_currentWordIdx];
        if (m_currentCharIdx >= currentWord.length()) return;

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
        if (m_currentCharIdx >= currentWord.length()) return;

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
        if (currentWord.isEmpty()) return;

        QString html = "<div style='direction: rtl;'>";
        for (int i = 0; i < m_currentCharIdx; i++) html += currentWord[i];

        html += QString("<span style='color: #ef4444; text-decoration: underline;'>%1</span>")
                .arg(currentWord[m_currentCharIdx]);

        if (waitingAfterShadda) html += "ّ";

        for (int i = m_currentCharIdx + 1; i < currentWord.length(); i++) html += currentWord[i];

        html += "</div>";
        m_lblCurrentWord->setText(html);

        if (m_currentCharIdx == currentWord.length() - 1) m_btnNextChar->setText("الكلمة التالية");
        else m_btnNextChar->setText("الحرف التالي");
    }

    // --- Variables for new Touch Arbitration ---
    InputMode m_currentInputMode = InputMode::Unknown;
    QPointF   m_touchStartPos;
    bool      m_touchDecided   = false; 
    bool      m_routeToParent  = false; 
    QObject*  m_activeTouchSrc = nullptr;
    static constexpr int kSwipeThreshold = 12;

    QBoxLayout* m_dynamicLayout{};
    QTextEdit *m_inputText{}, *m_outputText{};
    QLabel *m_lblCurrentWord{};
    QPushButton *m_btnStartTashkeel{}, *m_btnNextChar{}, *m_btnCopy{};
    
    QScrollArea* m_scrollArea{};
    QGridLayout* m_gridMarks{};
    QList<QPushButton*> m_markButtons;

    QStringList m_words;
    QStringList m_processedWords;
    int m_currentWordIdx = 0;
    int m_currentCharIdx = 0;
    QString m_currentWordProcessed = "";
    bool m_isWaitingForMarkAfterShadda = false;
};

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Naqrah");
    app.setOrganizationName("Naqrah");

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(11);
    font.setFamily("Segoe UI, Tahoma, Noto Sans Arabic, sans-serif");
    app.setFont(font);

    MainWindow window;
    window.show();
    return app.exec();
}
#include "main.moc"