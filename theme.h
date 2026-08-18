#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QDialog>
#include <QVBoxLayout>

enum class ThemeMode { Mocha, Latte, ShamelaClassic, Nord, Gruvbox };

struct ThemeColors {
    QString Base, Text, Surface, Hover;
    QString Accent1, Accent2, Accent3, Accent4, Danger;
    bool isDark; 
};

extern const ThemeColors MochaTheme;
extern const ThemeColors LatteTheme;
extern const ThemeColors ShamelaClassicTheme;
extern const ThemeColors NordTheme;
extern const ThemeColors GruvboxTheme;

class ThemeSelectionDialog : public QDialog {
    Q_OBJECT
public:
    int selectedTheme = -1;
    explicit ThemeSelectionDialog(QWidget* parent = nullptr);
};

class CustomThemeDialog : public QDialog {
    Q_OBJECT
public:
    ThemeColors m_colors;
    explicit CustomThemeDialog(const ThemeColors& initialColors, QWidget* parent = nullptr);

private:
    void addColorRow(QVBoxLayout* layout, const QString& labelText, QString* colorRef);
};

#endif