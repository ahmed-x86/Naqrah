#include "theme.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QLabel>
#include <QColorDialog>

const ThemeColors MochaTheme = {
    "#1e1e2e", "#cdd6f4", "#313244", "#45475a",
    "#b4befe", "#89b4fa", "#cba6f7", "#f5c2e7", "#f38ba8",
    true
};

const ThemeColors LatteTheme = {
    "#eff1f5", "#4c4f69", "#ccd0da", "#bcc0cc",
    "#7287fd", "#1e66f5", "#8839ef", "#ea76cb", "#d20f39",
    false
};

const ThemeColors ShamelaClassicTheme = {
    "#f4ecdf", "#3d2314", "#e6d6c3", "#d8c3a9",
    "#8b5a2b", "#684c31", "#a87c4f", "#5c8065", "#9e2a2b",
    false
};

const ThemeColors NordTheme = {
    "#2e3440", "#d8dee9", "#3b4252", "#434c5e",
    "#88c0d0", "#81a1c1", "#5e81ac", "#a3be8c", "#bf616a",
    true
};

const ThemeColors GruvboxTheme = {
    "#282828", "#ebdbb2", "#3c3836", "#504945",
    "#fabd2f", "#fe8019", "#b8bb26", "#83a598", "#cc241d",
    true
};

ThemeSelectionDialog::ThemeSelectionDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("اختر المظهر");
    setMinimumSize(300, 400); 
    setLayoutDirection(Qt::RightToLeft);

    auto* layout = new QVBoxLayout(this);
    
    QStringList themes = {
        "☕ Mocha", 
        "☀️ Latte", 
        "📜 Shamela Classic", 
        "❄️ Nord", 
        "📦 Gruvbox", 
        "🎨 مخصص (Custom)"
    };
    
    for (int i = 0; i < themes.size(); ++i) {
        auto* btn = new QPushButton(themes[i], this);
        btn->setFixedHeight(50); 
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            selectedTheme = i;
            accept();
        });
        layout->addWidget(btn);
    }
}

CustomThemeDialog::CustomThemeDialog(const ThemeColors& initialColors, QWidget* parent) 
    : QDialog(parent), m_colors(initialColors) {
    
    setWindowTitle("تخصيص الألوان");
    setMinimumSize(320, 500); 
    setLayoutDirection(Qt::RightToLeft);

    auto* mainLayout = new QVBoxLayout(this);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame); 
    
    auto* scrollWidget = new QWidget(scrollArea);
    auto* scrollLayout = new QVBoxLayout(scrollWidget);

    addColorRow(scrollLayout, "الخلفية الأساسية (Base)", &m_colors.Base);
    addColorRow(scrollLayout, "لون الواجهة/الأزرار (Surface)", &m_colors.Surface);
    addColorRow(scrollLayout, "لون النص (Text)", &m_colors.Text);
    addColorRow(scrollLayout, "لون التمرير (Hover)", &m_colors.Hover);
    addColorRow(scrollLayout, "اللون المميز 1 (Accent 1)", &m_colors.Accent1);
    addColorRow(scrollLayout, "اللون المميز 2 (Accent 2)", &m_colors.Accent2);
    addColorRow(scrollLayout, "اللون المميز 3 (Glow / Accent 3)", &m_colors.Accent3);
    addColorRow(scrollLayout, "اللون المميز 4 (Accent 4)", &m_colors.Accent4);
    addColorRow(scrollLayout, "لون التحذير/الخطر (Danger)", &m_colors.Danger);

    auto* chkDark = new QCheckBox("المظهر داكن", this);
    chkDark->setChecked(m_colors.isDark);
    chkDark->setStyleSheet("font-weight: bold; margin-top: 10px;");
    connect(chkDark, &QCheckBox::toggled, this, [this](bool checked){ m_colors.isDark = checked; });
    scrollLayout->addWidget(chkDark);

    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    auto* btnLayout = new QHBoxLayout();
    auto* btnSave = new QPushButton("تطبيق", this);
    auto* btnCancel = new QPushButton("إلغاء", this);
    
    btnSave->setFixedHeight(45); btnSave->setCursor(Qt::PointingHandCursor);
    btnCancel->setFixedHeight(45); btnCancel->setCursor(Qt::PointingHandCursor);
    
    connect(btnSave, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);
}

void CustomThemeDialog::addColorRow(QVBoxLayout* layout, const QString& labelText, QString* colorRef) {
    auto* row = new QHBoxLayout();
    auto* lbl = new QLabel(labelText, this);
    lbl->setStyleSheet("font-weight: bold; font-size: 13px;");
    
    auto* btnColor = new QPushButton(this);
    btnColor->setFixedSize(50, 35);
    btnColor->setCursor(Qt::PointingHandCursor);
    btnColor->setStyleSheet(QString("background-color: %1; border: 2px solid #777; border-radius: 6px;").arg(*colorRef));
    
    connect(btnColor, &QPushButton::clicked, this, [this, btnColor, colorRef]() {
        QColor c = QColorDialog::getColor(QColor(*colorRef), this, "اختر لوناً", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            *colorRef = c.name(QColor::HexArgb); 
            btnColor->setStyleSheet(QString("background-color: %1; border: 2px solid #777; border-radius: 6px;").arg(*colorRef));
        }
    });
    
    row->addWidget(lbl);
    row->addStretch();
    row->addWidget(btnColor);
    layout->addLayout(row);
}