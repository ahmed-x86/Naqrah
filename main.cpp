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

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("Naqrah - نقرة");
        setMinimumSize(900, 600);
        setLayoutDirection(Qt::RightToLeft);

        buildUI();
        applyShadcnTheme();
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

private:
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
            QPushButton#btnPrimary {
                background-color: #f4f4f5;
                color: #09090b;
                border: none;
                border-radius: 6px;
                padding: 10px 16px;
                font-weight: bold;
                font-size: 14px;
            }
            QPushButton#btnPrimary:hover {
                background-color: #e4e4e7;
            }
            QPushButton#btnPrimary:pressed {
                background-color: #d4d4d8;
            }
            QPushButton#btnSecondary {
                background-color: #18181b;
                color: #f4f4f5;
                border: 1px solid #27272a;
                border-radius: 6px;
                padding: 10px 16px;
                font-weight: bold;
                font-size: 13px;
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
                background: transparent;
                width: 8px;
                margin: 0;
            }
            QScrollBar::handle:vertical {
                background: #27272a;
                border-radius: 4px;
                min-height: 24px;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0;
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

        // Content Container
        QWidget* contentWidget = new QWidget();
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(32, 32, 32, 32);

        // Main 3-column layout
        m_dynamicLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        m_dynamicLayout->setSpacing(24);

        // 1. Input Card
        QWidget* inputCard = createCard("النص المدخل", "أدخل النص الذي تود تشكيله هنا");
        auto* inputLayout = static_cast<QVBoxLayout*>(inputCard->layout());
        
        m_inputText = new QTextEdit();
        m_inputText->setPlaceholderText("اكتب أو الصق النص هنا...");
        m_btnStartTashkeel = new QPushButton("تشكيل");
        m_btnStartTashkeel->setObjectName("btnPrimary");
        m_btnStartTashkeel->setCursor(Qt::PointingHandCursor);
        m_btnStartTashkeel->setMinimumHeight(44);
        
        inputLayout->addWidget(m_inputText);
        inputLayout->addSpacing(12);
        inputLayout->addWidget(m_btnStartTashkeel);
        connect(m_btnStartTashkeel, &QPushButton::clicked, this, &MainWindow::startTashkeel);

        // 2. Tashkeel (Center) Card
        QWidget* tashkeelCard = createCard("لوحة التشكيل", "اختر الحركات المناسبة للحرف المظلل");
        auto* tashkeelLayout = static_cast<QVBoxLayout*>(tashkeelCard->layout());
        
        m_lblCurrentWord = new QLabel("الكلمة");
        m_lblCurrentWord->setObjectName("currentWordLabel");
        m_lblCurrentWord->setAlignment(Qt::AlignCenter);
        m_lblCurrentWord->setMinimumHeight(120);
        m_lblCurrentWord->setWordWrap(true);

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
            QPushButton* btnMark = new QPushButton(mark.name + " (ـ" + mark.unicode + ")");
            btnMark->setObjectName("btnMark");
            btnMark->setCursor(Qt::PointingHandCursor);
            btnMark->setMinimumHeight(52);
            connect(btnMark, &QPushButton::clicked, [this, mark]() { applyMark(mark.unicode); });
            gridMarks->addWidget(btnMark, row, col);
            col++;
            if (col > 1) { col = 0; row++; }
        }

        m_btnNextChar = new QPushButton("الحرف التالي");
        m_btnNextChar->setObjectName("btnSecondary");
        m_btnNextChar->setCursor(Qt::PointingHandCursor);
        m_btnNextChar->setMinimumHeight(44);
        connect(m_btnNextChar, &QPushButton::clicked, this, &MainWindow::advanceChar);

        tashkeelLayout->addWidget(m_lblCurrentWord);
        tashkeelLayout->addSpacing(10);
        tashkeelLayout->addLayout(gridMarks);
        tashkeelLayout->addSpacing(16);
        tashkeelLayout->addWidget(m_btnNextChar);

        // 3. Output Card
        QWidget* outputCard = createCard("النتيجة", "النص النهائي المشكّل");
        auto* outputLayout = static_cast<QVBoxLayout*>(outputCard->layout());
        
        m_outputText = new QTextEdit();
        m_outputText->setReadOnly(true);
        m_btnCopy = new QPushButton("نسخ النص");
        m_btnCopy->setObjectName("btnSecondary");
        m_btnCopy->setCursor(Qt::PointingHandCursor);
        m_btnCopy->setMinimumHeight(44);
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
        mainLayout->addWidget(contentWidget);
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

    // --- Backend Logic Functions ---
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

    QBoxLayout* m_dynamicLayout{};
    QTextEdit *m_inputText{}, *m_outputText{};
    QLabel *m_lblCurrentWord{};
    QPushButton *m_btnStartTashkeel{}, *m_btnNextChar{}, *m_btnCopy{};

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
